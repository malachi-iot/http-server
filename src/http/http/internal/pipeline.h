#pragma once

#include "bool.h"

#include "../fwd.h"
#include <array.h>

#define FEATURE_PIPELINE_HANDLER_STATE 1

typedef enum
{
    HTTP_PIPELINE_HANDLER_UNINITIALIZED,
    HTTP_PIPELINE_HANDLER_NOMINAL,
    HTTP_PIPELINE_HANDLER_ERROR

}   HttpPipelineHandlerState;

typedef struct _HttpPipelineHandler
{
    const char* name;

    //http_request_on_state_fnptr on_state;
    http_pipeline_fnptr handler;

    // Specialized values for general operation of this handler
    void* arg;

    // Size of context to allocate per request
    unsigned context_size;

#if FEATURE_PIPELINE_HANDLER_STATE
    HttpPipelineHandlerState state;
#endif

}   HttpPipelineHandler;


typedef struct _HttpPipeline
{
    Array pipeline;

} HttpPipeline;



