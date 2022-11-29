#include <memory.h>

#include <unity.h>

#include "test-data.h"

#include <statemachine/tokenizer.h>
#include <statemachine/internal/tokenizer.h>


static void http_tokenizer_core_scratch_test()
{
    char buf[128];
    memset(buf, 0, sizeof(buf));
    StateMachine2 sm;
    TokenStateMachineOut smo;
    token_sm2_init(&sm, " ", TOKEN_STATE_EOLMODE_CRLF);
    // DEBT: Do both flavors of FEATURE_TOKENIZER_OUT_FNPTR testing
    token_smout_init(&smo, buf, sizeof(buf));
    ConstSpan in = { req2, sizeof(req2) - 1 };
    while(sm.state != TOKEN_STATE_EOL && sm.state != TOKEN_STATE_DELIM)
        token_sm2_process(&sm, in, &smo);
    TEST_ASSERT_EQUAL_STRING("PUT", buf);
}


// This was pre mark-mode.  Keep it around for non-mark-mode flavor
unsigned token_sm2_get_legacy(StateMachine2* sm, ConstSpan* in, TokenStateMachineOut* smo)
{
    unsigned processed = 0;

    // auto reset, of sorts
    token_sm2_reset(sm, NULL);
    bool direct_mode = smo->out == NULL;

    if(direct_mode)
    {
        smo->out_start = NULL;
        smo->out_end = NULL;
    }

    while(sm->state != TOKEN_STATE_EOL && sm->state != TOKEN_STATE_DELIM && in->sz > 0)
    {
        processed = token_sm2_process(sm, *in, smo);

        in->buf += processed;
        in->sz -= processed;

        if(direct_mode)
        {
            if(sm->state == TOKEN_STATE_TOKEN)
                // DEBT: Experimenting with this
                // we don't WANT use out_start because reset operation picks that up, but we are
                // we don't use out because token_sm2_processor itself treats that as a scratch mode flag
                // DEBT: Naughty const-away cast
                smo->out_start = (char*)in->buf;
            else if(sm->state == TOKEN_STATE_TOKEN_END)
                smo->out_end = in->buf;
        }
    }

    if(direct_mode)
        return smo->out_end - smo->out_start;
    else
        return smo->out - smo->out_start;
}


unsigned token_sm2_get(StateMachine2* sm, ConstSpan* in, TokenStateMachineOut* smo, char** accumulator)
{
    unsigned processed = 0;

    // auto reset, of sorts
    if(sm->state == TOKEN_STATE_EOL || sm->state == TOKEN_STATE_DELIM)
    {
        token_sm2_reset(sm, NULL);
    }

    const bool direct_mode = smo->out == NULL;

    while(sm->state != TOKEN_STATE_EOL && sm->state != TOKEN_STATE_DELIM && in->sz > 0)
    {
        processed = token_sm2_process(sm, *in, smo);

        in->buf += processed;
        in->sz -= processed;

        if(sm->state == TOKEN_STATE_TOKEN && accumulator && *accumulator == NULL)
            *accumulator = smo->out;
    }

    if(direct_mode)
        return smo->out_end - smo->out_start;
    else if(accumulator)
        return smo->out - *accumulator;
    else
        return smo->out - smo->out_start;
}



static void http_tokenizer_core_direct_test()
{
    StateMachine2 sm;
    TokenStateMachineOut smo;
    token_sm2_init(&sm, " ", TOKEN_STATE_EOLMODE_CRLF);
    // DEBT: Do both flavors of FEATURE_TOKENIZER_OUT_FNPTR testing
    token_smout_init(&smo, NULL, 0);
    ConstSpan in = { req2, sizeof(req2) - 1 };

    unsigned length = 0;
    char** buf = &smo.out_start;

    length = token_sm2_get(&sm, &in, &smo, NULL);

    TEST_ASSERT_NOT_NULL(*buf);
    TEST_ASSERT_EQUAL(3, length);

    length = token_sm2_get(&sm, &in, &smo, NULL);

    //TEST_ASSERT_EQUAL_STRING("PUT", buf);
    TEST_ASSERT_NOT_NULL(*buf);
    TEST_ASSERT_EQUAL(TEST_PATH_LENGTH, length);

    length = token_sm2_get(&sm, &in, &smo, NULL);

    TEST_ASSERT_EQUAL(8, length);
}


static void http_tokenizer_tests()
{
    char buf[128];
    Tokenizer smh;

    tokenizer_init(&smh, req2, sizeof(req2) - 1, " ", TOKEN_STATE_EOLMODE_CRLF);
    tokenizer_set_output(&smh, buf, sizeof(buf));

    smh.sm.null_terminate = true;

    const char* token;

    token = tokenizer_get_token(&smh);

    TEST_ASSERT_EQUAL_STRING("PUT", token);

    token = tokenizer_get_token(&smh);

    token = tokenizer_get_token(&smh);

    TEST_ASSERT_EQUAL_STRING("HTTP/1.0", token);

    token_sm2_reset(&smh.sm, " :");

    token = tokenizer_get_token(&smh);

    token = tokenizer_get_token(&smh);

    TEST_ASSERT_EQUAL_STRING("4", token);
    TEST_ASSERT_EQUAL(TOKEN_STATE_EOL, smh.sm.state);

    token = tokenizer_get_token(&smh);

    TEST_ASSERT_EQUAL(0, tokenizer_get_last_token_length(&smh));
    TEST_ASSERT_EQUAL(TOKEN_STATE_EOL, smh.sm.state);

    tokenizer_deinit(&smh);
}


void tokenizer_tests()
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(http_tokenizer_core_scratch_test);
    RUN_TEST(http_tokenizer_core_direct_test);
    RUN_TEST(http_tokenizer_tests);
}
