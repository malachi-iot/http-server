#include <logger.h>

#include <unistd.h>

#include "unit-tests.h"

#include "domain-socket.h"
#include "http/header.h"

int domain_listen_fd = -1;

void setUp()
{
    logger_initConsoleLogger(stderr);
    domain_listen_fd = create_domain_socket();
    http_header_init();
}


void tearDown()
{
    close(domain_listen_fd);
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(array_tests);
    RUN_TEST(http_header_tests);
    http_fileserver_tests();
    http_pipeline_tests();
    http_request_tests();
    http_response_tests();
    audit_tests();
    experimental_tests();
    tokenizer_tests();
    UNITY_END();

    return 0;
}
