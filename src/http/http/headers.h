// Handles aggregation of HttpHeader
// Broken out because some code doesn't care about the HttpHeader content, just adding it
#pragma once

#include "fwd.h"

#include "array.h"

typedef struct _HttpHeaders
{
    Array headers;

} HttpHeaders;

void http_headers_placement_new(HttpHeaders* h, int initial_capacity);
void http_headers_placement_delete(const HttpHeaders* h);
const HttpHeader* http_headers_add_header(HttpHeaders* h, const char* key, const char* value);

// Gets an array to all available header values - one must inspect headers.count to know precisely how many
const HttpHeader* http_headers_get_headers(const HttpHeaders* h);
// Gets a particular header from the header list
const HttpHeader* http_headers_get_header(const HttpHeaders* h, const char* key);
const char* http_headers_get_header_value(const HttpHeaders* h, const char* key);
