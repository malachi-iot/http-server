#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <logger.h>

#include "http/audit.h"
#include <http/header.h>
#include <http/request.h>
#include <http/response.h>

#include <http/context.h>
#include <http/internal/request.h>
#include <http/internal/response.h>

static int audit_fd;

int http_audit_init(const char* filename, bool truncate)
{
    // DEBT: Interestingly, it seems that we must remember to give OURSELVES write permission, otherwise
    // the 2nd time we open this file we can't write to it even though 1st time we can - debt because
    // I need to doublecheck if that's really true

    int oflag = O_CREAT | O_WRONLY;

    oflag |= truncate ? O_TRUNC : O_APPEND;

    audit_fd = open(filename,
        O_CREAT | O_WRONLY,
        S_IWGRP | S_IRGRP | S_IRUSR | S_IWUSR | S_IROTH);

    LOG_INFO("audit_init: logging to %s", filename);

    return audit_fd;
}

void http_audit_deinit()
{
    // As per [6.4] these are safe to call from sigint

    fsync(audit_fd);    // close does not by default flush the buffer [18]
    close(audit_fd);
}

void http_audit_entry(const HttpContext* context)
{
    const HttpRequest* r = context->request;
    const HttpResponse* resp = context->response;
    const char* desc = get_http_status_code_description(resp->status);

    LOG_TRACE("http_audit: response code=%s", desc ? desc : "null");

    // DEBT: Thread safety a consideration not yet fully handled (test to see if 'write' REALLY is atomic
    // as they say it is)

    char s[128];
    const char* requestId = http_headers_get_header_value(&r->headers, HTTP_HEADER_FIELD_REQUEST_ID);

    if(requestId == NULL)
        requestId = "0";

    desc = get_http_method_string(r->method);

    // sprintf'ing all at once instead of piecemeal writing in theory lets us use write atomic behavior
    // to be MT safe
    int len = sprintf(s, "%s,%s,%u,%s\n",
        desc ? desc : "null",
        r->uri, resp->status, requestId);
    write(audit_fd, s, len);
}