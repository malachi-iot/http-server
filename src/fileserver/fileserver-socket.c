// All references see REFERENCES.md
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <logger.h>
#include <errno.h>
#include <err.h>

#include "http/context.h"
#include "http/pipeline.h"
#include <http/pipeline/message.h>
#include "http/header-constants.h"
#include <http/request.h>
#include "http/response.h"

#include <http/internal/request.h>

#include "http/pipeline/fileserver.h"
#include "http/pipeline/internal/fileserver.h"

// TODO: Need to do file locking so that GET returns a 'resource unavailable' or similar (or blocks)
// if a PUT is happening at the exact same time on that file - see [17] and its children and be aware
// that those examples favor *process* locking not *thread* locking

typedef const HttpSocketFileserverContext* LocalContext;

// Runs a little differently than PUT since we don't have to wait for body on get
static void http_fileserver_on_state_get(const StateMachineMessage* m, LocalContext c)
{
    const HttpContext* context = &m->sm->const_context;

    if(c->file_fd == -1)
    {
        HttpResponseCodes status;

        switch(c->errno_)
        {
            case ENOENT:
                status = HTTP_RESPONSE_NOT_FOUND;
                break;

            case EACCES:
                status = HTTP_RESPONSE_SERVICE_UNAVAILABLE;
                break;

            default:
                status = HTTP_RESPONSE_INTERNAL_SERVER_ERROR;
                break;
        }
        http_response_emit_socket_generic(context, status);
        return;
    }
    else
        http_response_emit_socket_generic(context, HTTP_RESPONSE_OK);

    char* buf = m->sm->buf.buf;
    unsigned buf_sz = m->sm->buf.sz;

    http_response_emit_socket_header_int(context,
        HTTP_HEADER_FIELD_CONTENT_LENGTH,
        c->st.st_size);

    write(context->transport.connfd, "\r\n", 2);

    int n = 0;

    while(n < c->st.st_size)
    {
        ssize_t read_bytes = read(c->file_fd, buf, buf_sz);
        n = write(context->transport.connfd, buf, read_bytes);

        if(read_bytes <= 0 || n <= 0)
        {   // Protect ourselves from an infinite loop here
            LOG_ERROR("http_fileserver_on_state_get: couldn't emit file");
            return;
        }

        n += read_bytes;
    }
}


static void http_fileserver_on_state_body_put(const StateMachineMessage* m, LocalContext c)
{
    if(c->file_fd == -1) return;

    const HttpContext* context = &m->sm->const_context;
    //const HttpRequest* r = context->request;

    size_t n = context->content_length - m->position;

    if(n > m->body.sz)
        n = m->body.sz;

    ssize_t written = write(c->file_fd, m->body.buf, n);

    // DEBT: Do something more significant about write errors
    if(written != n)
        LOG_INFO("http_fileserver_on_state_body_put: didn't write all characters");

    if(m->position == 0)
    {
        // DEBT: There are better places to emit this
        http_response_emit_generic(context, HTTP_RESPONSE_CREATED);
    }
}

static void http_fileserver_on_state_line(const StateMachineMessage* m, HttpSocketFileserverContext* _c)
{
    const HttpContext* c = &m->sm->const_context;

    char uri[128];
    char* saveptr;
    // DEBT: We make this copy so as to not assume that underlying uri can be written to
    // That is a good thing.  However, in our lightweight environments, we may want to demand writable uri
    strcpy(uri, m->path);
    // DEBT: Pretty weak path processing
    const char* filename = strtok_r(uri, "/", &saveptr);

    // DEBT: Decouple from Request, or at least switch to LightweightRequest
    switch(c->request->method)
    {
        case HTTP_METHOD_GET:
            // [11]
            stat(filename, &_c->st);

            _c->file_fd = open(filename, O_RDONLY);
            _c->errno_ = errno;

            const unsigned sz = _c->st.st_size;

            LOG_INFO("http_socket_fileserver: filename=%s, size=%u", filename, sz);
            break;

        case HTTP_METHOD_PUT:
        {
            // thanks weak ass linking
            // https://stackoverflow.com/questions/2245193/why-does-open-create-my-file-with-the-wrong-permissions

            const int flags = O_CREAT | O_WRONLY | O_TRUNC |    // Effectively making a 'creat()' call
                    0;
            const int permissions =
                    S_IRUSR | S_IWUSR |                         // Give user read/write permissions
                    S_IROTH |                                   // Experimental
                    //S_IRGRP | S_IWGRP;                          // Give group read/write permissions
                    0;

            _c->file_fd = open(filename, flags, permissions);
            _c->errno_ = errno;

            LOG_INFO("http_socket_fileserver: filename=%s, handle=%d", filename, _c->file_fd);
            break;
        }

        default: break;
    }
}


static void http_fileserver_socket_pipeline_request(const StateMachineMessage* m, void* arg)
{
    const HttpContext* c = &m->sm->const_context;
    HttpSocketFileserverContext* _c = arg;

    switch(m->state)
    {
        case HTTP_REQUEST_LINE:
            http_fileserver_on_state_line(m, _c);
            break;

        case HTTP_REQUEST_BODY:
            if(c->request->method == HTTP_METHOD_PUT)
                http_fileserver_on_state_body_put(m, _c);
            break;

        case HTTP_REQUEST_DONE:
            if(c->request->method == HTTP_METHOD_GET)
            {
                // DEBT: Would rather put this up right at 'LINE' area, but writing back to socket while still
                // reading for rest of pipeline causes unexpected behaviors
                http_fileserver_on_state_get(m, _c);
            }
            else if(c->request->method == HTTP_METHOD_PUT)
            {
                if(_c->file_fd == -1)
                    http_response_emit_generic(c, HTTP_RESPONSE_INTERNAL_SERVER_ERROR);
            }
            break;

        default:
            break;
    }
}

void http_fileserver_socket_pipeline(const HttpPipelineMessage* m, void* arg)
{
    HttpSocketFileserverContext* c = (HttpSocketFileserverContext*) m->context_data;

    switch(m->m)
    {
        case HTTP_PIPELINE_STARTUP:
#if FEATURE_PIPELINE_HANDLER_STATE
            //m->handler->state = HTTP_PIPELINE_HANDLER_ERROR;
#endif
            if(http_pipeline_get(m->context->pipeline, HTTP_PIPELINE_REQUEST_POPULATOR_ID) == NULL)
                LOG_ERROR("http_fileserver_socket_pipeline: can't init, dependent request populator not found");
            break;

        case HTTP_PIPELINE_INIT:
            c->file_fd = -1;
            break;

        case HTTP_PIPELINE_REQUEST:
            http_fileserver_socket_pipeline_request(m->request, c);
            break;

        case HTTP_PIPELINE_DEINIT:
            close(c->file_fd);
            break;

        default: break;
    }
}