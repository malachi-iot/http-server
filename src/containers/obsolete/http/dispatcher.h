#pragma once

#include "bool.h"

#include "../fwd.h"
#include "../../array.h"
#include "../enum.h"
#include "../request-statemachine.h"

#include "pipeline.h"


typedef bool (*can_handle_request_fnptr)(const HttpContext* context, void* arg);

typedef struct
{
    HttpMethods method;
    http_request_on_state_fnpts on_state;
    http_request_fnptr handler;

    void* arg;

}   HttpRequestHandler;


typedef struct _HttpRequestDispatcher
{
    const char* name;

    // Unlike handlers, server runs all dispatcher pipelines *always*
    // unless NO dispatcher matched at all
    Array pipeline;

    union
    {
        Array handlers;
        //HttpRequestHandler handler;
    };

    union
    {
        struct
        {
            unsigned use_body : 1;
            /// Normally dispatch happens based on method/verb, but one can instead route through
            /// can_handle_request to determine whether to activate.  In this case, 'handler'
            /// is activated and 'handlers' is not used
            unsigned dispatch_via_fnptr : 1;
        };
        unsigned raw;
    } options;

    // NULL means accepts all requests
    can_handle_request_fnptr can_handle_request;

    // dispatcher-specific data storage.  Not yet used
    void* data;

}   HttpRequestDispatcher;
