#pragma once

#include "../fwd.h"

#include "../statemachine/request.h"


typedef enum
{
    HTTP_PIPELINE_UNINITIALIZED,
    HTTP_PIPELINE_STARTUP,
    HTTP_PIPELINE_INIT,                         ///< request-context
    HTTP_PIPELINE_BEGIN = HTTP_PIPELINE_INIT,   ///< request-context
    HTTP_PIPELINE_REQUEST,
    HTTP_PIPELINE_RESPONSE,
    HTTP_PIPELINE_DEINIT,                       ///< request-context
    HTTP_PIPELINE_END = HTTP_PIPELINE_DEINIT,   ///< request-context
    HTTP_PIPELINE_SHUTDOWN,
}   HttpPipelineMessages;


// DEBT: Naming kind of clumsy
typedef struct _HttpPipelineResponseMessage
{
    HttpResponseStates state;
    HttpResponseCodes status;
} HttpPipelineResponseMessage;

typedef struct _HttpPipelineMessage
{
    HttpPipelineMessages m;

    union
    {
        const HttpContext* context;
        WritableHttpContext* writable_context;
    };

    // Per-request data
    void* context_data;
    const HttpPipelineHandler* handler;

    union
    {
        StateMachineMessage* request;
        const HttpPipelineResponseMessage* response;
    };

} HttpPipelineMessage;

