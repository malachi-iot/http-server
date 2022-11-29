#pragma once

#include "../enum.h"
#include "../headers.h"

#include "header.h"


typedef enum
{
    HTTP_REQUEST_BODY_NONE = 0,
    HTTP_REQUEST_BODY_MALLOC,
    HTTP_REQUEST_BODY_TRANSPORT

} HttpRequestBodyModes;


typedef struct _HttpRequest
{
    const char* uri;
    HttpMethods method;
    HttpHeaders headers;

    // body in one contiguous memory chunk.  See flags below for usage
    const char* body;

    union
    {
        struct
        {
            /// auto populates 'body' with malloc'd buffer when true.  Maximum size is
            /// limited to 10MB to prevent DoS attacks
            /// when false, one must either:
            /// 1. manually read from transport (i.e. context::connfd or context::connfile, not recommended)
            /// 2. read via pipeline chunks
            //unsigned use_body: 1;
            /// auto populates 'body' reusing transport buffer if available.  Maximum size
            /// is limited to transport buffer size
            //unsigned use_transport_buffer: 1;

            // Defaults to transport mode since it's zero overhead.
            HttpRequestBodyModes body_mode : 4;
        };

        unsigned raw;
    }   flags;

} HttpRequest;

// Doesn't directly do any dynamic allocation, and therefore doesn't keep headers and URI around -
// people further down the pipeline must read things out immediately
typedef struct _HttpLightweightRequest
{
    HttpMethods method;

    // last read header, key-matched
    // TODO: This may not be of any use since now we have matched_key right in state message
    HttpHeader header;

} HttpLightweightRequest;


