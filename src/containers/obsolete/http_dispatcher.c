// All references pertain to README.md
#include <logger.h>

#include "http/dispatcher.h"
#include "http/handler.h"
#include "http/response.h"

#include "http/request-statemachine.h"

#include "http/internal/dispatcher.h"
#include "http/internal/request.h"

void http_request_dispatcher_dispatch_from_pipeline(const HttpContext* context, void* arg)
{
    HttpRequestDispatcher* d = arg;

    http_request_dispatcher_dispatch(d, context);
}

static int handler_compare(const void* lhs, const void* rhs)
{
    const HttpRequestHandler* l = lhs;
    const HttpRequestHandler* r = rhs;

    if(l->method < r->method) return -1;

    return l->method > r->method;
}


void http_request_dispatcher_placement_new(HttpRequestDispatcher* d, const char* name)
{
    array_placement_new(&d->pipeline, sizeof(HttpPipelineHandler), 0);
    array_placement_new(&d->handlers, sizeof(HttpRequestHandler), 4);

    d->options.raw = 0;
    d->can_handle_request = NULL;
    d->name = name;
    d->data = NULL;
}


void http_request_dispatcher_placement_delete(HttpRequestDispatcher* d)
{
    array_placement_delete(&d->pipeline);
    array_placement_delete(&d->handlers);
}


void http_request_dispatcher_add_handler(HttpRequestDispatcher* d, HttpMethods m, http_request_fnptr handler, void* arg)
{
    HttpRequestHandler* h = array_add(&d->handlers, NULL);

    h->handler = handler;
    h->method = m;
    h->arg = arg;
    h->on_state = NULL;

    array_qsort(&d->handlers, handler_compare);
}

void http_request_dispatcher_add_pipeline(HttpRequestDispatcher* d, http_request_fnptr handler, void* arg)
{
    HttpPipelineHandler* h = array_add(&d->pipeline, NULL);

    // DEBT: Make a placement new

    h->on_state = NULL;
    h->handler = handler;
    h->arg = arg;
}


void http_request_dispatcher_add_handler_placeholder(HttpRequestDispatcher* d)
{
    http_request_dispatcher_add_pipeline(d, NULL, NULL);
}


PipelineResponse http_request_dispatcher_pipeline(HttpRequestDispatcher* d, const HttpContext* context)
{
    HttpPipelineHandler* h = (HttpPipelineHandler *)d->pipeline.buffer;
    int count = d->pipeline.count, success = 0;
    bool did_dispatcher = false;
    bool can_handle_request = d->can_handle_request == NULL || d->can_handle_request(context, NULL);

    while(count--)
    {
        if(h->handler == NULL)
        {
            // Whether we actually executed a dispatcher or not, we've handled the dispatcher
            // use case
            did_dispatcher = true;

            if(can_handle_request)
            {
                success += http_request_dispatcher_dispatch(d, context);
            }
        }
        else
        {
            h->handler(context, h->arg);
            ++success;
        }
        ++h;
    }

    PipelineResponse r;

    if(did_dispatcher == false && can_handle_request == true)
    {
        success += http_request_dispatcher_dispatch(d, context);
    }

    //r.dispatcher_ran = did_dispatcher;
    r.pipelines_ran = success;

    return r;
}


bool http_request_dispatcher_dispatch(HttpRequestDispatcher* d, const HttpContext* context)
{
    // TODO: Likely phase this out, pipelines seem better
    if(d->options.dispatch_via_fnptr)
    {   // EXPERIMENTAL
        if(d->can_handle_request(context, NULL))
        {
            if(d->handlers.count != 1)
            {
                // TODO: Do some assert code and sprinkle it around
                LOG_ERROR("http_request_dispatcher_dispatch: incorrect dispatcher count found");
                return false;
            }

            HttpRequestHandler* h = (HttpRequestHandler*)d->handlers.buffer;
            h->handler(context, h->arg);
            return true;
        }
    }
    HttpRequestHandler key;
    const HttpRequest* r = context->request;
    key.method = r->verb;
    key.handler = NULL;     // Just so in debugger we can see which one is the key

    HttpRequestHandler* h = array_bsearch(&d->handlers, &key, handler_compare);

    if(h != NULL)
    {
        LOG_TRACE("http_request_dispatcher_dispatch: passing on to %s:%s", d->name, get_http_method_string(r->verb));
        h->handler(context, h->arg);
        return true;
    }
    else
    {
        http_response_emit_generic(context, HTTP_RESPONSE_METHOD_NOT_ALLOWED);
        return false;
    }
}


// DEBT: Intermediate code, on its way out - use http_pipeline_on_state instead
bool http_request_dispatcher_on_state(HttpRequestDispatcher* d, const StateMachineMessage* m)
{
    HttpPipelineHandler* h = (HttpPipelineHandler *)d->pipeline.buffer;
    int count = d->pipeline.count, success = 0;

    while(count--)
    {
        if(h->on_state != NULL)
        {
            h->on_state(m, NULL);
        }
        ++h;
    }

    return success;
}


/*
static void dispatcher2(const StateMachineMessage* m, void* arg)
{
    http_pipeline_diagnostic(m, NULL);
    http_pipeline_request_populator(m, NULL);

    // DEBT: We don't want to hardwire to this all the time
    HttpRequestDispatcher* d = (HttpRequestDispatcher*)m->sm->server->dispatchers.buffer;

    http_request_dispatcher_on_state(d, m);

    //m->sm->server
}
*/
