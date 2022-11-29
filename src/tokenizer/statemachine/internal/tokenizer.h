#pragma once

#include <bool.h>
#include <util.h>
#include "../enum.h"
#include "../out.h"

// Pros: powerful options for handling output including immediate to-lower or direct to transport
// Cons: mandates all operations do have this output structure
#define FEATURE_TOKENIZER_OUT_FNPTR 1

// Pros: helpful marker system to place start/end of found token into token out area directly
// Cons: Adds unexpected complexity to TokenStateMachineOut support functions, and arguably
//       the start/end can be handled elsewhere.  Also, mandates presence of out_start much like accumulator does
#define FEATURE_TOKENIZER_MARK_OUTPUT 1

// Adds ability to runtime select whether scratch buffer always starts at 0 or continues to accumulate
// across tokens.  Feature flagged because in theory entire memory footprint might shrink without
// this feature - that code is not written though, so leave this to on always for now.
#define FEATURE_TOKENIZER_ACCUMULATOR 1


typedef struct StateMachine2
{
    const char* delim;
    struct
    {
        TokenStates state : 8;
        TokenStateEolMode eol_mode : 8;

        // Null terminate strtok style (side effects if not using temp buffer)
        // NOTE: Beware that we treat everything as const char* so this is going to bypass that!
        bool null_terminate : 1;
#if FEATURE_TOKENIZER_ACCUMULATOR
        // Normally output modes reset output buffer back to the start.  Set this to true
        // to instead keep accumulating into the output buffer.  If true, be sure to manually
        // initiate a tokenizer_reset or flip this back to false at some point
        // DEBT: this flag is actually only used by Tokenizer (wrapper) and shouldn't be in state machine,
        // unless we move the 'token' variable into TokenStateMachineOut
        bool accumulate_out : 1;
#endif
        // For higher level Tokenizer, auto selects scratch or direct (default to false)
        bool autoselect : 1;
    };

} StateMachine2;


typedef struct TokenStateMachineOut
{
    // scratch mode: for reset operations - normally Tokenizer::token was doing this but in accumulate only mode
    // it can't.
    // direct mode: start of token
    char* out_start;

    // scratch mode: current position of output to token scratch buffer
    // direct mode: NULL (indicates direct mode)
    char* out;

    // scratch mode: upper boundary of token scratch buffer
    // direct mode: upper boundary of found token
    const char* out_end;

#if FEATURE_TOKENIZER_OUT_FNPTR
    // when we reach TOKEN_STATE_TOKEN, this is called
    // by default it looks for delimiter or EOF start character and reports how
    // far in it got.  See 'token_delim_fnptr' documentation for convention details
    token_delim_fnptr out_fn;
#endif

} TokenStateMachineOut;


typedef struct Tokenizer
{
    StateMachine2 sm;
    // Either:
    // 1. the beginning position of out.out if out.out is not NULL (scratch buffer mode, not accumulated)
    // 2. the beginning position of 'in.buf' at the start of TOKEN_STATE_TOKEN (direct buffer mode)
    // 3. the start token position in out.out, moving forward until manually reset (scratch buffer mode, accumulated)
    const char* token;

    ConstSpan in;
    unsigned short length;      // token length
    unsigned short processed;
    TokenStateMachineOut out;

} Tokenizer;


void token_sm2_init(StateMachine2* sm, const char* delim, TokenStateEolMode eol_mode);
unsigned token_sm2_process(StateMachine2* sm, const ConstSpan in, TokenStateMachineOut* smo);
void token_sm2_reset(StateMachine2* sm, const char* delim);

