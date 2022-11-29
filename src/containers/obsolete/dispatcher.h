// Dispatcher is a kind of sub-server so that your HTTP server can do
// more than just one thing i.e. server up files on one URL and control lights on another
#pragma once

#include "fwd.h"

#include "array.h"
#include "bool.h"
#include "enum.h"

typedef struct
{
    //unsigned dispatcher_ran : 1;
    /// Indicates how many pipelines engaged in processing - not an indicator of failure,
    /// only a general indicator of how much processing the pipeline performed
    unsigned pipelines_ran : 6;

} PipelineResponse;

void http_request_dispatcher_placement_new(HttpRequestDispatcher* d, const char* name);
void http_request_dispatcher_placement_delete(HttpRequestDispatcher* d);

/// Within the context of a dispatcher one has exactly one handler per method/verb
/// @param d
/// @param m
/// @param handler
/// @return false if could not be dispatched, otherwise true
void http_request_dispatcher_add_handler(HttpRequestDispatcher* d, HttpMethods m, http_request_fnptr handler, void* arg);
void http_request_dispatcher_add_pipeline(HttpRequestDispatcher* d, http_request_fnptr handler, void* arg);
/// Adds handler alias in the current pipeline location
/// @param d
/// @remarks Use this if you wish the handler to happen in the middle of surrounding pipeline operations
void http_request_dispatcher_add_handler_placeholder(HttpRequestDispatcher* d);
PipelineResponse http_request_dispatcher_pipeline(HttpRequestDispatcher* d, const HttpContext* r);
bool http_request_dispatcher_dispatch(HttpRequestDispatcher* d, const HttpContext* r);
bool http_request_dispatcher_on_state(HttpRequestDispatcher* d, const StateMachineMessage* m);
