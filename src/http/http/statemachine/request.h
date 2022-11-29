#pragma once

#include "bool.h"
#include "enum.h"
#include "../context.h"
#include "../internal/header.h"
#include <util.h>
#include <statemachine/enum.h>
#include <statemachine/tokenizer.h>
#include <statemachine/internal/tokenizer.h>



typedef struct TokenStateMachine
{
    const char* delim;

    struct
    {
        TokenStates state : 8;
        TokenStateEolMode eol_mode : 4;
    };

} TokenStateMachine;


typedef struct Accumulator
{
    Span span;
    unsigned short accumulated;

} Accumulator;



typedef struct _StateMachineMessage
{
    HttpRequestStateMachine* sm;
    HttpRequestStates state;

    union
    {
        // Valid during request LINE processing
        struct
        {
            const char* version;
            const char* method;
            const char* path;
        };

        // Valid during HEADER processing
        struct
        {
            HttpHeader header;
            // State machine needs to match key to determine content_length, so share
            // the love. This differs from 'key' since that one is raw
            // from incoming buffer and isn't tolower'd or matched
            // DEBT: Story isn't quite that simple during socket/chunked processing
            const char* matched_key;
        };

        // Valid during BODY processing
        struct
        {
            ConstSpan body;
            unsigned position;
        };
    };

}   StateMachineMessage;


// DEBT: I really want to contain this inside http_request.c but that obviates placement new
typedef struct _HttpRequestStateMachine
{
    union
    {
        WritableHttpContext context;
        const HttpContext const_context;
    };
    HttpRequestStates state;
    // NOTE: getline may reallocate 'buf' to be bigger than original
    // requested size - but buf.sz will reflect it
    Span buf;

    // AKA last processed state result
    StateMachineMessage message;

    // How far in to body content we are
    unsigned content_position;

    // Chunk vs line oriented processing
    //bool chunk_mode;
    //TokenStateMachine tsm;
    // for key & value processing in chunk mode
    //char secondary[256];
    //unsigned short secondary_pos;

    // Replacement for above TokenStateMachine
    Tokenizer tokenizer;

} HttpRequestStateMachine;


bool http_request_state_machine(HttpRequestStateMachine* sm);
///
/// @param sm
/// @param read
/// @return
/// > 0 = number of bytes consumed
/// = 0 = no bytes consumed, needs more processing (pure state change)
/// < 0 = done processing completely
int http_request_state_machine_process_line(HttpRequestStateMachine* sm, char* buf, int read);
int http_request_state_machine_process_chunk(HttpRequestStateMachine* sm, const char* buf, int available);

void http_request_state_machine_placement_new(HttpRequestStateMachine* sm, const HttpTransport* transport, int buf_size);
void http_request_state_machine_placement_delete(HttpRequestStateMachine* sm);

const char* get_http_request_state_string(HttpRequestStates s);