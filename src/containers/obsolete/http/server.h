// The aggregate of all the dispatchers is a server
#pragma once

#include "fwd.h"
#include "array.h"

#if FEATURE_PGLX10_AUTO_INTERNAL
#include "internal/server.h"
#endif

void http_server_placement_new(HttpServer* server);
void http_server_placement_delete(HttpServer* server);
HttpRequestDispatcher* http_server_add_dispatcher(HttpServer* server, const char* name);
HttpRequestDispatcher* http_server_get_dispatcher(const HttpServer* server, const HttpContext* context);

// DEBT: Handling both as we transition to pipeline
void http_server_handle_incoming_request(HttpServer* server, HttpPipeline* pipeline, int connfd);
