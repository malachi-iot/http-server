#include <stdio.h>
#include <stdlib.h>

#include <logger.h>

#include "http/context.h"
#include "http/header.h"
#include "http/pipeline.h"
#include <http/response.h>

#include "http/pipeline/message.h"
#include "http/statemachine/request.h"


#include "http/internal/pipeline.h"
#include "http/internal/response.h"

#define SETUP_ARRAY_ITERATOR(array, type) \
type* v = (type*)(array)->buffer;   \
const int v_count = (array)->count;   \
type* const v_end = v + v_count;

#define ARRAY_ITERATE_BEGIN(array, type) SETUP_ARRAY_ITERATOR(array, type) \
while(v < v_end) {
#define ARRAY_ITERATE_END() ++v; }

void http_pipeline_placement_new(HttpPipeline* p)
{
    // Call this aggressively just to ensure it does get initialized
    http_header_init();

    array_placement_new(&p->pipeline, sizeof(HttpPipelineHandler), 4);
}

void http_pipeline_placement_delete(HttpPipeline* p)
{
    array_placement_delete(&p->pipeline);
}


static void on_state_dummy_handler(const StateMachineMessage* m, void* arg)
{

}

static void http_pipeline_dummy_handler(const HttpPipelineMessage* m, void* arg)
{

}



const HttpPipelineHandler* http_pipeline_add(HttpPipeline* p,
    const char* name,
    http_pipeline_fnptr fn,
    const HttpPipelineHandlerOptions* options)
{
    HttpPipelineHandler* h = array_add(&p->pipeline, NULL);

    //h->on_state = on_state_dummy_handler;
    h->handler = fn;
    h->name = name;
    if(options)
    {
        h->context_size = options->context_size;
        h->arg = options->arg;
    }
    else
    {
        h->context_size = 0;
        h->arg = NULL;
    }

#if FEATURE_PIPELINE_HANDLER_STATE
    h->state = HTTP_PIPELINE_HANDLER_UNINITIALIZED;
#endif

    return h;
}

static unsigned http_pipeline_storage_size(const HttpPipeline* p)
{
    HttpPipelineHandler* h = (HttpPipelineHandler *)p->pipeline.buffer;
    int count = p->pipeline.count;
    unsigned context_size = 0;

    while(count--)
    {
        context_size += h->context_size;
        ++h;
    }

    return context_size;
}

/// Sets up pointers in HttpPipelineStorage
/// @param p
/// @param ps
/// @param raw buffer of size (context_size + p->pipeline.count * sizeof(void*))
/// @param context_size
static void http_pipeline_storage_placement_new(HttpPipelineStorage* ps, HttpPipeline* p, char* raw,
    unsigned context_size)
{
    if(context_size == 0) return;

    HttpPipelineHandler* h = (HttpPipelineHandler *)p->pipeline.buffer;
    //HttpPipelineHandler* h_begin = h;
    int count = p->pipeline.count;

    ps->raw = raw;
    ps->storage = (void**)raw;

    int i = 0;
    unsigned pos = count * sizeof(void*);

    while(count--)
    {
        if(h->context_size)
        {
            char* adjusted = raw + pos;
            ps->storage[i] = adjusted;
            pos += h->context_size;
        }
        else
        {
            ps->storage[i] = NULL;
        }

        i++;
        ++h;
    }
}

void http_pipeline_begin(HttpPipeline* p, HttpPipelineMessage* m, HttpWritableContext* context)
{
    HttpPipelineStorage* ps = &context->pipeline_storage;
    unsigned context_size = http_pipeline_storage_size(p);

    if(context_size > 0)
    {
        const unsigned pos = p->pipeline.count * sizeof(void*);

        char* raw = malloc(pos + context_size);

        http_pipeline_storage_placement_new(ps, p, raw, context_size);
    }
    else
    {
        ps->raw = NULL;
        ps->storage = NULL;
    }

    context->pipeline = p;

    m->writable_context = context;
    m->m = HTTP_PIPELINE_BEGIN;
    http_pipeline_message((HttpContext*)context, m);
}


void http_pipeline_end(const HttpContext* context, HttpPipelineMessage* m)
{
    m->m = HTTP_PIPELINE_END;
    http_pipeline_message(context, m);

    const HttpPipelineStorage* s = &context->pipeline_storage;

    if(s->raw != NULL)
        free(s->raw);
}

const HttpPipelineHandler* http_pipeline_get(const HttpPipeline* p, const char* name)
{
    ARRAY_ITERATE_BEGIN(&p->pipeline, const HttpPipelineHandler);

    if(name == v->name) return v;

    ARRAY_ITERATE_END();

    return NULL;
}


static void http_pipeline_message_internal(const HttpPipeline* p, HttpPipelineMessage* m,
    const HttpPipelineStorage* storage)
{
    int i = 0;  // for getting allocated pipeline-specific request context
    void** s = storage ? storage->storage : NULL;

    ARRAY_ITERATE_BEGIN(&p->pipeline, const HttpPipelineHandler);
    {
        void* pipeline_context = s != NULL ? s[i] : NULL;

        m->handler = v;
        m->context_data = pipeline_context;

        v->handler(m, v->arg);
        ++i;
    }
    ARRAY_ITERATE_END();
}

void http_pipeline_message(const HttpContext* c, HttpPipelineMessage* m)
{
    http_pipeline_message_internal(c->pipeline, m, &c->pipeline_storage);
}


void http_pipeline_startup(const HttpPipeline* p)
{
    LOG_DEBUG("http_pipeline_startup: %p", p);

    HttpPipelineMessage m;
    HttpWritableContext c;

    c.pipeline = p;

    m.m = HTTP_PIPELINE_STARTUP;
    m.writable_context = &c;
    http_pipeline_message_internal(p, &m, NULL);
}

void http_pipeline_shutdown(const HttpPipeline* p)
{
    LOG_DEBUG("http_pipeline_shutdown: %p", p);

    HttpPipelineMessage m;
    HttpWritableContext c;

    c.pipeline = p;

    m.m = HTTP_PIPELINE_SHUTDOWN;
    m.writable_context = &c;
    http_pipeline_message_internal(p, &m, NULL);
}





void http_pipeline_handle_incoming_request(HttpPipeline* pipeline, const HttpTransport* transport)
{
    LOG_TRACE("http_pipeline_handle_incoming_request: entry");

    HttpRequestStateMachine sm;
    HttpResponse response;

    // DEBT: Grab this 512 from somewhere, maybe a pipeline option ability?
    http_request_state_machine_placement_new(&sm, transport, 512);

    HttpWritableContext* c = &sm.context;
    const HttpContext* cc = &sm.const_context;

    HttpPipelineMessage pm;

    http_pipeline_begin(pipeline, &pm, c);

    HttpPipelineResponseMessage hprm;

    // DEBT: Not the greatest place to allocate response, but ought to do.  This keeps
    // the context object itself lightweight - which may or may not matter
    c->response = &response;
    http_response_placement_new(&response);

    // EXPERIMENTAL, not sure we really need a full response init phase
    hprm.state = HTTP_RESPONSE_INIT;
    pm.m = HTTP_PIPELINE_RESPONSE;
    pm.response = &hprm;
    http_pipeline_message(cc, &pm);

    pm.m = HTTP_PIPELINE_REQUEST;
    pm.request = &sm.message;

    while(http_request_state_machine(&sm) == false)
    {
        http_pipeline_message(cc, &pm);
    }

    http_pipeline_message(cc, &pm);

    if(c->response != NULL && c->response->status == 0)
    {
        http_response_emit_generic(&sm.const_context, HTTP_RESPONSE_SERVICE_UNAVAILABLE);
    }

    hprm.state = HTTP_RESPONSE_DONE;

    pm.m = HTTP_PIPELINE_RESPONSE;
    pm.response = &hprm;

    http_pipeline_message(cc, &pm);

    http_response_placement_delete(&response);

    http_pipeline_end(cc, &pm);
    http_request_state_machine_placement_delete(&sm);

    LOG_TRACE("http_pipeline_handle_incoming_request: exit");
}