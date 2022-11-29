#include <ctype.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <logger.h>

#include "http/pipeline.h"
#include "http/header.h"
#include "http/request.h"
#include "http/statemachine/request.h"

#include "http/internal/request.h"

// Not yet used
typedef struct
{
    bool done : 1;
    bool err : 1;

} StateMachineResult;


// EXPERIMENTAL
typedef struct BufferedSpan
{
    char* buf1;
    char* buf2;
    bool buf1_active;
    char* gpos;
    char* epos;
    unsigned current_size;

} BufferedSpan;

void buffered_span_move_left(BufferedSpan* s)
{
    char* buf = s->buf1_active ? s->buf1 : s->buf2;
    memcpy(buf, s->gpos, s->current_size);
    s->gpos = buf;
}


//static const char* const TEST = HTTP_HEADER_FIELD_CONTENT_LENGTH;
//static const intptr_t TEST = (intptr_t) HTTP_HEADER_FIELD_CONTENT_LENGTH;

static void http_request_state_machine_internal(HttpRequestStateMachine* sm, int read);



static int http_request_state_machine_process_body(HttpRequestStateMachine* sm, const char* buf, int available)
{
    HttpWritableContext* c = &sm->context;
    StateMachineMessage* m = &sm->message;
    m->position = sm->content_position;
    sm->content_position += available;

    // TODO: Do resize magic here rather than request

    m->body.buf = buf;
    m->body.sz = available;

    if(sm->content_position > c->content_length)
    {
        LOG_WARN("http_request: calc error");
        sm->state = HTTP_REQUEST_DONE;
    }
    else if(sm->content_position == c->content_length)
    {
        sm->state = HTTP_REQUEST_DONE;
    }

    return available;
}

// DEBT: No active code tests this
int http_request_state_machine_process_line(HttpRequestStateMachine* sm, char* buf, int available)
{
    HttpRequestStates state = sm->state;
    StateMachineMessage* m = &sm->message;
    HttpWritableContext* c = &sm->context;
    char* saveptr;
    int retval = 0;

    //LOG_TRACE("http_request sm: %s", buf);

    switch(state)
    {
        case HTTP_REQUEST_INIT:
            state = HTTP_REQUEST_LINE;
            break;

        case HTTP_REQUEST_LINE:
        {
            const char* verb = strtok_r(buf, " ", &saveptr);
            const char* http_str = strtok_r(NULL, " ", &saveptr);
            const char* version = strtok_r(NULL, " \r", &saveptr);

            LOG_DEBUG("http_request verb: %s:%s:%s", verb, http_str, version);

            m->version = version;
            m->method = verb;
            m->path = http_str;

            state = HTTP_REQUEST_HEADERS_START;
            retval = saveptr - buf;

            break;
        }

        case HTTP_REQUEST_HEADERS_START:
            state = HTTP_REQUEST_HEADERS_LINESTART;
            break;

        case HTTP_REQUEST_HEADERS_LINESTART:
            if(*buf == 0)   // we trim whitespace off the end, so empty string really is empty
            {
                /*
                // Empty line means headers are done

                // Now that we've gathered all the headers, get an early peek at what dispatcher
                // wants to handle this request.  Useful for knowing what to do with 'body', plus
                // not too much later we'll need this anyway to perform the dispatch itself
                if(sm->server != NULL)
                    // It's possible to operate state machine without a full server/dispatcher environment
                    sm->dispatcher = http_server_get_dispatcher(sm->server, &sm->const_context);
                */

                state = c->content_length > 0 ? HTTP_REQUEST_BODY : HTTP_REQUEST_DONE;
            }
            else
                // headers continue
                state = HTTP_REQUEST_HEADERS_LINE;

            break;

        case HTTP_REQUEST_HEADERS_LINE:
        {
            const char* key = strtok_r(buf, ":", &saveptr);
            const char* value = strtok_r(NULL, " ", &saveptr);

            m->header.key = key;
            m->header.value = value;
            m->matched_key = get_http_header_key(key, true);

            if(m->matched_key == HTTP_HEADER_FIELD_CONTENT_LENGTH)
            {
                c->content_length = atoi(value);
                LOG_TRACE("http_request content-length=%u", c->content_length);
            }

            state = HTTP_REQUEST_HEADERS_LINESTART;
            break;
        }

        case HTTP_REQUEST_BODY:
            return http_request_state_machine_process_body(sm, buf, available);

        case HTTP_REQUEST_DONE:
            retval = -1;
            break;

        default: break;
    }

    sm->state = state;
    return retval;
}


int http_request_state_machine_process_chunk(HttpRequestStateMachine* sm, const char* buf, int available)
{
    HttpRequestStates state = sm->state;
    StateMachineMessage* m = &sm->message;
    HttpWritableContext* c = &sm->context;
    Tokenizer* t = &sm->tokenizer;
    tokenizer_set_input(t, buf, available);
    int retval = 0;
    m->state = state;
    // DEBT: I'd prefer a more explicit EOF, but for the short term the socket way of indicating
    // EOF will do [21]
    const bool eof = available == 0;

    // Although unusual, it is permissible for a lone HTTP request line to appear without
    // headers.  In that case, only eof can tell us when we're done
    if(eof)
    {
        sm->state = HTTP_REQUEST_DONE;
        return 0;
    }

    switch(state)
    {
        case HTTP_REQUEST_INIT:
            state = HTTP_REQUEST_LINE;
            // DEBT: Probably better to do this elsewhere
            t->sm.null_terminate = true;
            break;

        case HTTP_REQUEST_LINE:
            // Set tokenizer mode so that if using a scratch buffer, we keep adding to it
            // rather than resetting back to beginning per token
            t->sm.accumulate_out = true;
            // DEBT: Do this truly chunk style
            // For now we presume request line is 100% present in buf
            m->method = tokenizer_get_token(t);
            m->path = tokenizer_get_token(t);
            m->version = tokenizer_get_token(t);

            state = HTTP_REQUEST_HEADERS_LINESTART;

            // From here on out default one-token-per-scratch-buffer mode is what we want
            t->sm.accumulate_out = false;
            token_sm2_reset(&t->sm, " :");
            // FIX: This needs to happen more automatically, its auto call is bypassed since we
            // manually reset sm2 above
            token_smout_reset(&t->out);
            break;

        case HTTP_REQUEST_HEADERS_LINESTART:
            m->header.key = tokenizer_get_token(t);
            switch(t->sm.state)
            {
                case TOKEN_STATE_EOL:
                    // if we reach EOL and nothing really tokenized, we're done with headers
                    if(tokenizer_get_last_token_length(t) == 0)
                    {
                        state = c->content_length > 0 ? HTTP_REQUEST_BODY : HTTP_REQUEST_DONE;
                    }
                    // DEBT: if data is present but not a key, what do we do?
                    break;

                case TOKEN_STATE_DELIM:
                    state = HTTP_REQUEST_HEADERS_KEY_FOUND;
                    break;

                default:
                    state = HTTP_REQUEST_HEADERS_KEY_CHUNK;
                    break;
            }

            break;

        case HTTP_REQUEST_HEADERS_KEY_CHUNK:
            m->header.key = tokenizer_get_token(t);
            if(t->sm.state == TOKEN_STATE_DELIM)
            {
                state = HTTP_REQUEST_HEADERS_KEY_FOUND;
            }
            // DEBT: This can get botched up by EOL appearing here, so we need to handle that
            break;

        case HTTP_REQUEST_HEADERS_KEY_FOUND:
            // DEBT: If we want to instead pass on original un-lowered key we may have to keep
            // accumulated mode active
            m->header.key = get_http_header_key(m->header.key, true);
            state = HTTP_REQUEST_HEADERS_VALUE_CHUNK;
            break;

        case HTTP_REQUEST_HEADERS_VALUE_CHUNK:
            m->header.value = tokenizer_get_token(t);
            if(t->sm.state == TOKEN_STATE_EOL)
                state = HTTP_REQUEST_HEADERS_VALUE_FOUND;

            break;

        case HTTP_REQUEST_HEADERS_VALUE_FOUND:
            if(m->header.key == HTTP_HEADER_FIELD_CONTENT_LENGTH)
            {
                c->content_length = atoi(m->header.value);
            }

            state = HTTP_REQUEST_HEADERS_LINE;
            break;

        case HTTP_REQUEST_HEADERS_LINE:
            state = HTTP_REQUEST_HEADERS_LINESTART;
            break;

        case HTTP_REQUEST_BODY:
            return http_request_state_machine_process_body(sm, buf, available);

        default:
            break;
    }

    retval += t->processed;

    sm->state = state;
    return retval;
}




void http_request_state_machine_placement_new(HttpRequestStateMachine* sm,
    const HttpTransport* transport, int buf_size)
{
    http_context_placement_new(&sm->context, transport);

    sm->buf.sz = buf_size;
    sm->buf.buf = malloc(buf_size);
    sm->state = HTTP_REQUEST_INIT;

    sm->content_position = 0;
    sm->message.sm = sm;

    //sm->chunk_mode = false;
    //sm->secondary_pos = 0;

    tokenizer_init(&sm->tokenizer, NULL, 0, " ", TOKEN_STATE_EOLMODE_CRLF);
}


void http_request_state_machine_placement_delete(HttpRequestStateMachine* sm)
{
    free(sm->buf.buf);

    http_context_placement_delete(&sm->context);
}
