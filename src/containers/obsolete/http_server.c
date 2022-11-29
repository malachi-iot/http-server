#include <stdio.h>
#include <stddef.h>
#include <err.h>

#include <logger.h>

#include "http/dispatcher.h"
#include "http/header.h"
#include "http/pipeline.h"
#include "http/request.h"
#include "http/request-statemachine.h"
#include "http/response.h"
#include "http/server.h"

#include "http/internal/dispatcher.h"
#include "http/internal/server.h"

void http_server_placement_new(HttpServer* server)
{
    // Call this aggressively just to ensure it does get initialized
    http_header_init();

    array_placement_new(&server->dispatchers, sizeof(HttpRequestDispatcher), 4);
}


void http_server_placement_delete(HttpServer* server)
{
    HttpRequestDispatcher* d = (HttpRequestDispatcher*)server->dispatchers.buffer;
    int count = server->dispatchers.count;

    while(count--)
    {
        http_request_dispatcher_placement_delete(d);
        ++d;
    }

    array_placement_delete(&server->dispatchers);
}


HttpRequestDispatcher* http_server_add_dispatcher(HttpServer* server, const char* name)
{
    HttpRequestDispatcher* dispatcher = array_add(&server->dispatchers, NULL);

    http_request_dispatcher_placement_new(dispatcher, name);

    return dispatcher;
}


HttpRequestDispatcher* http_server_get_dispatcher(const HttpServer* server, const HttpContext* context)
{
    HttpRequestDispatcher* d = (HttpRequestDispatcher*)server->dispatchers.buffer;
    int count = server->dispatchers.count;

    while(count--)
    {
        if(d->can_handle_request == NULL) return d;
        if(d->can_handle_request(context, NULL)) return d;

        ++d;
    }

    return NULL;
}


// DEBT: Make an http_pipeline_handle_incoming_request
void http_server_handle_incoming_request(HttpServer* server, HttpPipeline* pipeline, int connfd)
{
    HttpRequestStateMachine sm;
    HttpResponse response;

    http_request_state_machine_placement_new(&sm, server, connfd, 512);

    HttpWritableContext* c = &sm.context;

    if(pipeline)
        http_pipeline_begin(pipeline, c);

    // DEBT: Not the greatest place to allocate response, but ought to do.  This keeps
    // the context object itself lightweight - which may or may not matter
    c->response = &response;
    sm.message.sm = &sm;

    while(http_request_state_machine(&sm) == false)
    {
        if(pipeline)
            http_pipeline_on_state(pipeline, &sm.message);
        else
            sm.dispatcher2(&sm.message, NULL);
    }

    if(pipeline)
    {
        // pipeline mode is done at this point
        http_pipeline_on_state(pipeline, &sm.message);
        http_pipeline_end(pipeline, c);
        http_request_state_machine_placement_delete(&sm);
        return;
    }
    else
        sm.dispatcher2(&sm.message, NULL);

    if(sm.dispatcher == NULL)
    {
        http_response_emit_generic(&sm.const_context, HTTP_RESPONSE_NOT_IMPLEMENTED);
    }
    else
    {
        HttpRequestDispatcher* d = (HttpRequestDispatcher*) server->dispatchers.buffer;
        int count = server->dispatchers.count;

        while(count--)
        {
            PipelineResponse r = http_request_dispatcher_pipeline(d, &sm.const_context);

            if(r.pipelines_ran != d->pipeline.count)
            {
                // It's possible for people to spam incorrect API requests at us, so it's not an error or a warning
                LOG_INFO("http_server_handle_incoming_request: not all pipelines successful");
            }

            ++d;
        }
    }

    http_request_state_machine_placement_delete(&sm);
}


