#pragma once

#define CRLF "\r\n"

#define TEST_PATH_PREFIX "/"
#define TEST_PATH_NAME "test-unity"
#define TEST_PATH_SUFFIX ".txt"
#define TEST_PATH_LENGTH (sizeof(TEST_PATH_PREFIX) - 1 + sizeof(TEST_PATH_NAME) - 1 + sizeof(TEST_PATH_SUFFIX) - 1)
#define TEST_DATA_CORE  "hi2u"
#define TEST_DATA_CORE_LENGTH   "4"

static const char req1[] = "GET /" TEST_PATH_NAME TEST_PATH_SUFFIX " HTTP/1.0" CRLF;
static const char req2[] = "PUT /" TEST_PATH_NAME TEST_PATH_SUFFIX " HTTP/1.0" CRLF
                           "Content-Length: " TEST_DATA_CORE_LENGTH CRLF
                           CRLF
                           TEST_DATA_CORE;
static const char req3_1[] = "PUT /test-unity2.txt HTTP/1.0" CRLF
    "Accept: */*" CRLF
    "Content-Len";

static const char req3_2[] = "gth: 14" CRLF
    CRLF
    TEST_DATA_CORE;

static const char req3_3[] = "0123456789";

