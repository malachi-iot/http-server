#include <unity.h>

#include <stdio.h>

#include <http/audit.h>

#include <http/context.h>
#include <http/header-constants.h>
#include <http/headers.h>
#include <http/request.h>

#include <http/internal/request.h>
#include <http/internal/response.h>

static void audit_tests_1()
{
    HttpRequest* r = http_request_new(HTTP_METHOD_PUT, "synthetic");
    HttpResponse response;


    // NOTE: Interestingly, anonymous union seems to work in struct
    // but not out here --
    union
    {
        WritableHttpContext context;
        const HttpContext const_context;
    } u;

    u.context.request = r;
    u.context.response = &response;

    response.status = 404;

    int fd = http_audit_init("audit1.txt", false);

    TEST_ASSERT_NOT_EQUAL(-1, fd);

    http_audit_entry(&u.const_context);

    r->method = HTTP_METHOD_GET;
    http_headers_add_header(&r->headers, HTTP_HEADER_FIELD_REQUEST_ID, "7");

    http_audit_entry(&u.const_context);

    http_audit_deinit();

    http_request_delete(r);
}


static void audit_tests_2()
{
}


void audit_tests()
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(audit_tests_1);
    RUN_TEST(audit_tests_2);
}