#include <memory.h>

#include <unity.h>

#include "test-data.h"

#include "array.h"
#include "http/request.h"
#include "http/statemachine/request.h"

bool get_http_method_string_sanity_check();


static void http_request_method_to_string_tests()
{
    TEST_ASSERT_EQUAL_STRING("APPEND", get_http_method_string(HTTP_METHOD_APPEND));
    TEST_ASSERT_EQUAL_STRING("GET", get_http_method_string(HTTP_METHOD_GET));
    TEST_ASSERT_EQUAL_STRING("PUT", get_http_method_string(HTTP_METHOD_PUT));

    TEST_ASSERT_EQUAL(HTTP_METHOD_APPEND, get_http_method_from_string("APPEND"));
    TEST_ASSERT_EQUAL(HTTP_METHOD_PUT, get_http_method_from_string("PUT"));

    TEST_ASSERT_TRUE(get_http_method_string_sanity_check());
}




static HttpTransport transport = { HTTP_TRANSPORT_NONE };


static void http_statemachine_tests()
{
    HttpRequestStateMachine sm;
    http_request_state_machine_placement_new(&sm, &transport, 1024);
    // DEBT: rename sm.buf to sm.transport_buf and only use for specific operations
    char* buf = sm.buf.buf;
    unsigned sz = sizeof(req2) - 1;

    memcpy(buf, req2, sz);

    HttpRequestStates state;

    // DEBT: Re-do this to do actual line processing, vs old chunk processing it
    // was designed for
    return;

    while(sm.state != HTTP_REQUEST_DONE)
    {
        int move_forward = http_request_state_machine_process_line(&sm, buf, sz);
        buf += move_forward;
        sz -= move_forward;
    }

    http_request_state_machine_placement_delete(&sm);
}


static void http_statemachine_chunk_test1()
{
    HttpRequestStateMachine sm;
    http_request_state_machine_placement_new(&sm, &transport, 1024);
    // DEBT: rename sm.buf to sm.transport_buf and only use for specific operations
    char* buf = sm.buf.buf;
    unsigned sz = sizeof(req2) - 1;
    tokenizer_init(&sm.tokenizer, buf, sz, " ", TOKEN_STATE_EOLMODE_CRLF);

    memcpy(buf, req2, sz);

    HttpRequestStates state;

    while(sm.state != HTTP_REQUEST_DONE)
    {
        int move_forward = http_request_state_machine_process_chunk(&sm, buf, sz);
        buf += move_forward;
        sz -= move_forward;
    }

    http_request_state_machine_placement_delete(&sm);
}


static void http_statemachine_chunk_test2()
{
    const char* chunks[] = { req3_1, req3_2, req3_3 };
    char scratch_buf[128];

    HttpRequestStateMachine sm;
    http_request_state_machine_placement_new(&sm, &transport, 1024);
    //sm.chunk_mode = true;
    // DEBT: Do two different versions, one which uses transport buffer (direct mode)
    // and one which uses this output buffer (scratchpad mode)
    tokenizer_set_output(&sm.tokenizer, scratch_buf, sizeof(scratch_buf));

    for(int i = 0; i < sizeof(chunks) / sizeof(chunks[0]); ++i)
    {
        const char* buf = chunks[i];
        unsigned sz = strlen(chunks[i]);

        memcpy(sm.buf.buf, buf, sz);

        while(sm.state != HTTP_REQUEST_DONE && sz > 0)
        {
            int move_forward = http_request_state_machine_process_chunk(&sm, buf, sz);
            buf += move_forward;
            sz -= move_forward;
        }
    }

    TEST_ASSERT_EQUAL(HTTP_REQUEST_DONE, sm.state);

    http_request_state_machine_placement_delete(&sm);
}



static void http_statemachine_req1()
{
    HttpRequestStateMachine sm;
    http_request_state_machine_placement_new(&sm, &transport, 1024);

    char* buf = sm.buf.buf;
    unsigned sz = sizeof(req1) - 1;

    // DEBT: This is still confusing having to set this manually
    tokenizer_set_output(&sm.tokenizer, buf, sz);

    memcpy(buf, req1, sz);

    while(sm.state != HTTP_REQUEST_DONE)
    {
        int move_forward = http_request_state_machine_process_chunk(&sm, buf, sz);
        buf += move_forward;
        sz -= move_forward;
    }

    http_request_state_machine_placement_delete(&sm);
}


void http_request_tests()
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(http_request_method_to_string_tests);
    RUN_TEST(http_statemachine_tests);
    RUN_TEST(http_statemachine_req1);
    RUN_TEST(http_statemachine_chunk_test1);
    RUN_TEST(http_statemachine_chunk_test2);
}