#include <unity.h>

#include <unistd.h>

#include <http/pipeline.h>
#include <http/request.h>
#include <http/pipeline/fileserver.h>

#include "domain-socket.h"
#include "test-data.h"

extern int domain_listen_fd;


static void test1()
{
    const char filename[] = TEST_PATH_NAME TEST_PATH_SUFFIX;

    HttpPipeline pipeline;
    HttpTransport transport = { HTTP_TRANSPORT_SOCKET };

    http_pipeline_placement_new(&pipeline);

    http_pipeline_add_request(&pipeline);
    http_pipeline_add_fileserver(&pipeline, true);

    int client_fd = create_domain_socket_client();
    transport.connfd = accept_domain_socket(domain_listen_fd);

    unlink(filename);

    ssize_t written = write(client_fd, req2, sizeof(req2) - 1);

    TEST_ASSERT_EQUAL(sizeof(req2) - 1, written);

    http_pipeline_handle_incoming_request(&pipeline, &transport);

    TEST_ASSERT_EQUAL(0, close(transport.connfd));
    TEST_ASSERT_EQUAL(0, close(client_fd));

    http_pipeline_placement_delete(&pipeline);

    // TODO: Need to read back in file and make sure its content matches
}

void http_fileserver_tests()
{
    UnitySetTestFile(__FILE__);

    RUN_TEST(test1);
}