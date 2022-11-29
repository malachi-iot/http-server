#include <ctype.h>
#include <fcntl.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <logger.h>

#include "http/pipeline.h"
#include "http/header.h"
#include "http/request.h"
#include "http/statemachine/request.h"

#include "http/internal/request.h"


// DEBT: Does not know or care how uri is allocated, may be an issue later.
void http_request_placement_new(HttpRequest* r, HttpMethods m, const char* uri)
{
    r->uri = uri;
    r->method = m;
    r->body = NULL;

    r->flags.raw = 0;
    r->flags.body_mode = HTTP_REQUEST_BODY_TRANSPORT;

    http_headers_placement_new(&r->headers, 10);
}

HttpRequest* http_request_new(HttpMethods m, const char* uri)
{
    int uri_sz = strlen(uri) + 1;
    char* copied_uri;

    char* raw = malloc(sizeof(HttpRequest) + uri_sz);
    copied_uri = raw + sizeof(HttpRequest);
    memcpy(copied_uri, uri, uri_sz);

    HttpRequest* r = (HttpRequest*)raw;

    http_request_placement_new(r, m, uri);

    return r;
}

void http_request_placement_delete(const HttpRequest* r)
{
    http_headers_placement_delete(&r->headers);
}

void http_request_delete(const HttpRequest* r)
{
    // DEBT: All this casting away const is no good
    http_request_placement_delete(r);

    free((void*)r);
}

static const char* const methods[] =
{
    "APPEND",
    "GET",
    "HEAD",
    "PATCH",
    "POST",
    "PUT"
};

const char* get_http_method_string(HttpMethods m)
{
    if(m >= sizeof(methods) / sizeof(const char*))
        return NULL;

    return methods[m];
}


HttpMethods get_http_method_from_string(const char* s)
{
    for(int i = 0; i < sizeof(methods) / sizeof(const char*); i++)
        if(strcmp(methods[i], s) == 0) return i;

    return HTTP_METHOD_INVALID;
}

bool get_http_method_string_sanity_check()
{
    const char* previous_s = NULL;

    // Iterate numerically through all the enum possibilities
    for(int i = 0; i < HTTP_METHOD_INVALID_UPPER; ++i)
    {
        const char* s = get_http_method_string(i);
        if(s == NULL)
            return false;

        // Convention is for them to be alphabetical, so that we can easily catch
        // incorrect placement of new items
        if(previous_s != NULL)
        {
            int result = strcmp(previous_s, s);

            if(result >= 0) return false;
        }

        // This will basically always pass
        HttpMethods m = get_http_method_from_string(s);
        if(m != i)
        {
            return false;
        }

        previous_s = s;
    }

    return true;
}


