// All references see README.md
#include <string.h>

#include <sys/stat.h>

#include <logger.h>
#include <err.h>

#include "http/context.h"
#include <http/pipeline.h>
#include <http/header-constants.h>
#include "http/statemachine/request.h"
#include "http/response.h"

#include <http/internal/request.h>
#include <http/pipeline/message.h>

#include "http/pipeline/fileserver.h"

// As per [8] CRLF needed in HTTP


void http_fileserver_get(const HttpContext* context, void* arg)
{
    const HttpRequest* r = context->request;

    char uri[128];
    char* saveptr;

    strcpy(uri, r->uri);

    const char* filename = strtok_r(uri, "/", &saveptr);

    if(filename != NULL)
    {
        // [11]
        struct stat st;
        stat(filename, &st);
        //size = st.st_size;

        FILE* file = fopen(filename, "r");

        LOG_TRACE("http_fileserver_get: retrieving file %s", filename);

        if(file == NULL)
        {
            http_response_emit_generic(context, HTTP_RESPONSE_NOT_FOUND);
            return;
        }

        FILE* out = context->transport.connfile;

        http_response_emit_generic(context, HTTP_RESPONSE_OK);

        // Different platforms' printf get ornery with different off_t,
        // so yank it out into a known variable size
        long filesize = st.st_size;

        http_response_emit_stream_header_int(context, HTTP_HEADER_FIELD_CONTENT_LENGTH, filesize);

        fputs("\r\n", out); // No further headers
        fflush(out);    // for big files and/or slow connections, this is helpful

        // DEBT: Re-use buf from earlier state machine
        unsigned char buf[256];
        size_t n;

        do
        {
            n = fread(buf, 1, sizeof(buf), file);
            size_t written = fwrite(buf, 1, n, out);

            if(written != n)
                warn("written != n");
        }
        while(n == sizeof(buf));

        fclose(file);
    }
}


static void http_fileserver_put_internal(const HttpRequest* r)
{

}



// DEBT: Needs actual chunk implementation - right now it expects all of
// 'body' to be delivered in one message
void http_fileserver_put(const StateMachineMessage* m, void* arg)
{
    const HttpContext* context = &m->sm->const_context;
    const HttpRequest* r = context->request;

    char uri[128];
    char* saveptr;

    strcpy(uri, r->uri);

    const char* filename = strtok_r(uri, "/", &saveptr);

    LOG_INFO("http_fileserver_put: filename=%s", filename);

    if(filename != NULL)
    {
        FILE* out = fopen(filename, "w");

        fwrite(context->request->body, 1, context->content_length, out);

        fclose(out);

        http_response_emit_generic(context, HTTP_RESPONSE_CREATED);
    }
}



/*
void http_fileserver_init(HttpRequestDispatcher* d)
{
    // NOTE: This is expected to operate in conjunction with HttpServer, which already
    // calls placement new
    //http_request_dispatcher_placement_new(d);

    d->options.use_body = true;

    http_request_dispatcher_add_handler(d, HTTP_METHOD_GET, http_fileserver_get, NULL);
    http_request_dispatcher_add_handler(d, HTTP_METHOD_PUT, http_fileserver_put, NULL);
}
*/

static void http_fileserver_stream_pipeline_request(const StateMachineMessage* m, void* arg)
{
    const HttpContext* c = &m->sm->const_context;

    switch(m->state)
    {
        case HTTP_REQUEST_INIT:
            break;

        case HTTP_REQUEST_BODY:
            if(c->request->method == HTTP_METHOD_PUT)
            {
                http_fileserver_put(m, NULL);
            }
            break;

        case HTTP_REQUEST_DONE:
            switch(c->request->method)
            {
                case HTTP_METHOD_GET:
                    http_fileserver_get(c, NULL);
                    break;

                default: break;
            }
            break;

        default:
            break;
    }
}

void http_fileserver_stream_pipeline(const HttpPipelineMessage* m, void* arg)
{
    if(m->m != HTTP_PIPELINE_REQUEST) return;

    http_fileserver_stream_pipeline_request(m->request, arg);
}

void http_fileserver_deinit()
{
    // Not needed at the moment because http_request_dispatcher_delete frees up all necessary things so far
}

/*
HttpRequestDispatcher* http_server_add_fileserver(HttpServer* server)
{
    // DEBT: Well organized, but clumsy in that we can potentially add an uninitialized dispatcher.
    // in other words, the polar opposite of RAII
    HttpRequestDispatcher* d = http_server_add_dispatcher(server, "file server");
    http_fileserver_init(d);
    return d;
}
*/

