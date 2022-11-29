#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <logger.h>

#include "http/context.h"
#include "http/header.h"
#include "http/pipeline.h"
#include "http/pipeline/message.h"
#include "http/response.h"

// As per [8] CRLF needed in HTTP

// General all purpose descriptions
const char* get_http_status_code_description(HttpResponseCodes code)
{
    switch(code)
    {
        case HTTP_RESPONSE_OK: return "OK";
        case HTTP_RESPONSE_CREATED: return "Content Created";
        case HTTP_RESPONSE_BAD_REQUEST: return "Bad Request";
        case HTTP_RESPONSE_NOT_FOUND: return "Not Found";
        case HTTP_RESPONSE_NOT_IMPLEMENTED: return "Not Implemented";
        case HTTP_RESPONSE_METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case HTTP_RESPONSE_SERVICE_UNAVAILABLE: return "Service Unavailable";
        default: return NULL;
    }
}

static const char CRLF[] = "\r\n";
static const char HTTP_1_1[] = "HTTP/1.1 ";

// HTTP/1.1 defines this as "Status-Line" [19.1]
// HTTP/2 onward this is superseded by "Control Data" [19]
static void http_response_emit_socket_generic_internal(const HttpContext* context, HttpResponseCodes code, const char* desc)
{
    int connfd = context->transport.connfd;

    write(connfd, HTTP_1_1, sizeof(HTTP_1_1) - 1);

    char s[16];
    int len = sprintf(s, "%u ", code);
    write(connfd, s, len);

    if(desc != NULL)    write(connfd, desc, strlen(desc));
    
    write(connfd, CRLF, 2);

    if(context->response == NULL)
        LOG_DEBUG("http_response_emit_generic: unexpected NULL response");
    else
        context->response->status = code;
}

void http_response_emit_socket_generic(const HttpContext* context, HttpResponseCodes code)
{
    const char* desc = get_http_status_code_description(code);

    LOG_TRACE("http_response_emit_socket_generic: %u (%s)", code, desc);

    http_response_emit_socket_generic_internal(context, code, desc);
}

void http_response_emit_socket_header(const HttpContext* context, const char* key, const char* value)
{
    int connfd = context->transport.connfd;

    write(connfd, key, strlen(key));
    write(connfd, ":", 1);
    write(connfd, value, strlen(value));
    write(connfd, CRLF, 2);
}

void http_response_emit_socket_header_int(const HttpContext* context, const char* key, int value)
{
    int connfd = context->transport.connfd;

    write(connfd, key, strlen(key));
    char s[32];
    int len = sprintf(s, ":%d", value);
    write(connfd, s, len);
    write(connfd, CRLF, 2);
}


static void http_response_emit_stream_generic(const HttpContext* context, HttpResponseCodes code)
{
    FILE* out = context->transport.connfile;

    fprintf(out, "HTTP/1.0 %u", code);
    const char* desc = get_http_status_code_description(code);

    LOG_TRACE("http_response_emit_stream_generic: %u (%s)", code, desc);

    if(desc != NULL)
    {
        fputc(' ', out);
        fputs(desc, out);
    }
    fputs("\r\n", out);

    if(context->response == NULL)
        LOG_DEBUG("http_response_emit_generic: unexpected NULL response");
    else
        context->response->status = code;
}

void http_pipeline_response_message(const HttpContext* context, const HttpPipelineResponseMessage* prm)
{
    HttpPipelineMessage pm;

    pm.m = HTTP_PIPELINE_RESPONSE;
    pm.response = prm;
    pm.context = context;

    http_pipeline_message(context, &pm);
}

void http_response_emit_generic(const HttpContext* context, HttpResponseCodes code)
{
    HttpPipelineResponseMessage prm;

    prm.state = HTTP_RESPONSE_LINE;
    prm.status = code;

    http_pipeline_response_message(context, &prm);

    switch(context->transport.type)
    {
        case HTTP_TRANSPORT_SOCKET:
            http_response_emit_socket_generic(context, code);
            break;

        case HTTP_TRANSPORT_STREAM:
            http_response_emit_stream_generic(context, code);
            break;

        default:
            break;
    }
}

void http_response_placement_new(HttpResponse* r)
{
    http_headers_placement_new(&r->headers, 0);

    r->status = HTTP_RESPONSE_UNINITIALIZED;
}


HttpResponse* http_response_new()
{
    HttpResponse* r = malloc(sizeof(HttpResponse));

    http_response_placement_new(r);

    return r;
}


void http_response_placement_delete(HttpResponse* r)
{
    http_headers_placement_delete(&r->headers);
}


void http_response_delete(HttpResponse* r)
{
    http_response_placement_delete(r);
    free(r);
}


static const char* http_header = "HTTP/1.1 %u %s\n";

void http_headers_write(const HttpHeaders* headers, FILE* out)
{
    const HttpHeader* h = http_headers_get_headers(headers);
    int count = headers->headers.count;

    while(count--)
    {
        fprintf(out, "%s:%s\n", h->key, h->value);

        ++h;
    }
}


void http_response_write(HttpResponse* r, FILE* out)
{
    fprintf(out, http_header, r->status, get_http_status_code_description(r->status));

    http_headers_write(&r->headers, out);
}