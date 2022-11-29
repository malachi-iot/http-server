#pragma once

#include "fwd.h"

#if FEATURE_PGLX10_AUTO_INTERNAL
#include "internal/pipeline.h"
#endif

typedef struct _HttpPipelineHandlerOptions
{
    void* arg;

    /// This amount of space is allocated *per request*
    unsigned context_size;

}   HttpPipelineHandlerOptions;

void http_pipeline_placement_new(HttpPipeline* p);
void http_pipeline_placement_delete(HttpPipeline* p);

const HttpPipelineHandler* http_pipeline_add(HttpPipeline* p, const char* name, http_pipeline_fnptr fn,
    const HttpPipelineHandlerOptions* options);
const HttpPipelineHandler* http_pipeline_get(const HttpPipeline* p, const char* name);

void http_pipeline_startup(const HttpPipeline* p);
void http_pipeline_shutdown(const HttpPipeline* p);

/// Allocates per-request storage and fires off HTTP_PIPELINE_INIT
/// @param p
/// @param m uses existing HttpPipelineMessage allocation and populates with _INIT message
/// @param ps
/// @param context initializes 'm->context' with this value
/// @remarks Can be thought of as an HttpPipelineMessage and HttpPipelineStorage placement new
void http_pipeline_begin(HttpPipeline* p, HttpPipelineMessage* m, HttpWritableContext* context);
/// Frees per-request storage
/// @param p
/// @param m
/// @param ps
void http_pipeline_end(const HttpContext* c, HttpPipelineMessage* m);


void http_pipeline_handle_incoming_request(HttpPipeline* pipeline, const HttpTransport* transport);

void http_pipeline_message(const HttpContext* p, HttpPipelineMessage* m);
void http_pipeline_response_message(const HttpContext* context, const HttpPipelineResponseMessage* prm);
