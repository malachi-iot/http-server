#include <unistd.h>

#include <logger.h>

#include "http/header.h"
#include <http/pipeline.h>
#include "http/request.h"
#include <http/response.h>

#include <http/pipeline/message.h>

#include "http/internal/request.h"

#include "threadpool.h"
#include "http/statemachine/request.h"

const char* http_request_states_to_string(HttpRequestStates s)
{
    switch(s)
    {
        case HTTP_REQUEST_LINE: return "HTTP_REQUEST_LINE";
        case HTTP_REQUEST_BODY: return "HTTP_REQUEST_BODY";
        case HTTP_REQUEST_DONE: return "HTTP_REQUEST_DONE";
        default: return NULL;
    }
}

static void* dummy(void* arg)
{
    LOG_DEBUG("Dummy started %d", (intptr_t) arg);
    return arg;
}

void debug_stuff(ThreadPool* thread_pool)
{
    threadpool_queue(thread_pool, dummy, (void*) 77);
    threadpool_queue(thread_pool, dummy, (void*) 78);

    HttpRequest* r = http_request_new(HTTP_METHOD_GET, "http://test.com");
    SharedPtr* p2 = shared_ptr_new(r, 0);
    shared_ptr_delete(p2);

    const char* k1 = get_http_header_key("key1", false);
    const char* k2 = get_http_header_key("key2", false);
}

static void http_pipeline_diagnostic_request(const StateMachineMessage* m, void* arg)
{
    HttpRequestStateMachine* sm = m->sm;
    const HttpContext* c = &sm->const_context;
    const char* desc = http_request_states_to_string(m->state);

    LOG_TRACE("http_pipeline_diagnostic: state=%d (%s)", m->state, desc ? desc : "null");

    switch(m->state)
    {
        case HTTP_REQUEST_LINE:
            LOG_DEBUG("http_pipeline_diagnostic: request method raw=%s/cooked=%s",
                m->method,
                get_http_method_string(c->request->method));
            break;

        case HTTP_REQUEST_HEADERS_KEY_FOUND:
            LOG_TRACE("http_pipeline_diagnostic: header key=%s",
                m->header.key);
            break;

        case HTTP_REQUEST_HEADERS_VALUE_FOUND:
            LOG_TRACE("http_pipeline_diagnostic: header value=%s",
                m->header.value);
            break;

        case HTTP_REQUEST_HEADERS_LINE:
            LOG_DEBUG("http_pipeline_diagnostic: header %s:%s",
                      m->header.key,
                      m->header.value);
            break;

        case HTTP_REQUEST_BODY:
            LOG_DEBUG("http_pipeline_diagnostic: body %u/%u/%u",
                      m->position,
                      sm->content_position,
                      sm->context.content_length);
            break;

        default:
            break;
    }
}


static void http_pipeline_diagnostic_response(const HttpPipelineResponseMessage* m)
{
    switch(m->state)
    {
        case HTTP_RESPONSE_LINE:
        {
            const char* desc = get_http_status_code_description(m->status);

            LOG_DEBUG("http_pipeline_diagnostic_response: response status = %u (%s)",
                      m->status, desc ? desc : "null");
            break;
        }

        default: break;
    }
}

void http_pipeline_diagnostic(const HttpPipelineMessage* m, void* arg)
{
    switch(m->m)
    {
        case HTTP_PIPELINE_STARTUP:
            LOG_INFO("http_pipeline_diagnostic: initializing");
            break;

        case HTTP_PIPELINE_REQUEST:
            http_pipeline_diagnostic_request(m->request, arg);
            break;

        case HTTP_PIPELINE_RESPONSE:
            http_pipeline_diagnostic_response(m->response);
            break;

        case HTTP_PIPELINE_END:
            break;

        default: break;
    }
}


void http_pipeline_add_diagnostic(HttpPipeline* pipeline)
{
    http_pipeline_add(pipeline, "diagnostic", http_pipeline_diagnostic, NULL);
}

