#pragma once

#include <stdio.h>

#include "array.h"
#include "bool.h"
#include "enum.h"
#include "context.h"

//#include "internal/request.h"

extern const char HTTP_PIPELINE_REQUEST_POPULATOR_ID[];

const char* get_http_method_string(HttpMethods m);
HttpMethods get_http_method_from_string(const char*);
HttpRequest* http_request_new(HttpMethods m, const char* uri);
void http_request_delete(const HttpRequest* r);

void http_pipeline_add_request(HttpPipeline* pipeline);
void http_pipeline_add_lightweight_request(HttpPipeline* pipeline);
