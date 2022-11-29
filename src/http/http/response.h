#pragma once

#include <stdio.h>

#include "fwd.h"
#include "enum.h"
#include <array.h>

#if FEATURE_PGLX10_AUTO_INTERNAL
#include "internal/response.h"
#endif

const char* get_http_status_code_description(HttpResponseCodes code);


void http_response_placement_new(HttpResponse* r);
HttpResponse* http_response_new();
void http_response_placement_delete(HttpResponse* r);
void http_response_serialize(HttpResponse* r, FILE* out);

void http_response_emit_generic(const HttpContext* context, HttpResponseCodes code);

void http_response_emit_socket_generic(const HttpContext* context, HttpResponseCodes code);
void http_response_emit_socket_header(const HttpContext* context, const char* key, const char* value);
void http_response_emit_socket_header_int(const HttpContext* context, const char* key, int value);
void http_response_emit_stream_header_int(const HttpContext* context, const char* key, long int value);