#pragma once

// Picks up shared's fwd
// NOTE: Generates warnings because http/fwd.h is not quite the same as ./fwd.h
//#include_next "fwd.h"

#ifndef FEATURE_PGLX10_AUTO_INTERNAL
#define FEATURE_PGLX10_AUTO_INTERNAL 1
#endif

typedef struct queue queue_t;
typedef struct _Array Array;

typedef struct _HttpRequest HttpRequest;
typedef struct _HttpLightweightRequest HttpLightweightRequest;
typedef struct _HttpResponse HttpResponse;
typedef struct _HttpContext HttpContext;
typedef struct _HttpWritableContext HttpWritableContext;
typedef struct _HttpHeader HttpHeader;
typedef struct _HttpHeaders HttpHeaders;
typedef struct _HttpPipeline HttpPipeline;
typedef struct _HttpPipelineMessage HttpPipelineMessage;
typedef struct _HttpPipelineStorage HttpPipelineStorage;
typedef struct _HttpPipelineHandler HttpPipelineHandler;
typedef struct _HttpTransport HttpTransport;
typedef struct _HttpPipelineResponseMessage HttpPipelineResponseMessage;

typedef struct _HttpRequestStateMachine HttpRequestStateMachine;
typedef struct _StateMachineMessage StateMachineMessage;

typedef void (*http_pipeline_fnptr)(const HttpPipelineMessage* m, void* arg);
typedef void (*http_request_on_state_fnptr)(const StateMachineMessage* m, void* request_context);