#pragma once

#include <stdio.h>

#include "fwd.h"
#include "bool.h"
#include "enum.h"
#include "transport.h"

// In theory we could make HttpRequest itself in place, but its architecture requires
// a malloc anyway to hold on to URI, so we'll keep using HttpRequest* which is
// the expected practice anyway

typedef struct _HttpPipelineStorage
{
    void* raw;
    void** storage;

} HttpPipelineStorage;


typedef struct _HttpContext
{
    const HttpTransport transport;

    union
    {
        const HttpRequest* const request;
        const HttpLightweightRequest* const lightweight_request;
    };
    HttpResponse* const response;

    const unsigned content_length;

    const HttpPipeline* const pipeline;
    const HttpPipelineStorage pipeline_storage;

} HttpContext;

// EXPERIMENTAL
typedef struct _HttpWritableContext
{
    HttpTransport transport;

    union
    {
        HttpRequest* request;
        HttpLightweightRequest* lightweight_request;
    };
    HttpResponse* response;

    // DEBT: Would prefer to always grab this from headers, but it's convenient to have it here
    unsigned content_length;

    const HttpPipeline* pipeline;
    HttpPipelineStorage pipeline_storage;

} WritableHttpContext;

void http_context_placement_new(WritableHttpContext* context, const HttpTransport* transport);
void http_context_placement_delete(WritableHttpContext* context);