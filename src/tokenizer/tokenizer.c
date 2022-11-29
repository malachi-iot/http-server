#include <memory.h>
#include <stdlib.h>

#include "statemachine/tokenizer.h"
#include "statemachine/internal/tokenizer.h"


void token_sm2_reset(StateMachine2* sm, const char* delim)
{
    sm->state = TOKEN_STATE_UNSTARTED;
    if(delim)
        sm->delim = delim;
}


void token_sm2_init(StateMachine2* sm, const char* delim, TokenStateEolMode eol_mode)
{
    sm->eol_mode = eol_mode;
    sm->null_terminate = false;
    sm->accumulate_out = false;
    sm->autoselect = false;
    token_sm2_reset(sm, delim);
}


// TODO: Consider combining with token_sm2_eol_eval
static bool is_eol_start(const StateMachine2* sm, char c)
{
    switch(c)
    {
        case 10: return sm->eol_mode == TOKEN_STATE_EOLMODE_LF;
        case 13: return sm->eol_mode != TOKEN_STATE_EOLMODE_LF;
        default: return false;
    }
}

// stops if EOL begin is found
static unsigned token_sm2_skip_delim(const StateMachine2* sm, ConstSpan in)
{
    const char* buf = in.buf;
    const char* delim = sm->delim;
    unsigned i = 0;

    for(; i < in.sz && strchr(delim, buf[i]) && !is_eol_start(sm, buf[i]); ++i)
    {

    }

    return i;
}

// Returns true if either delimiter or EOL start character is found
inline static bool token_sm2_find_delim_helper(const StateMachine2* sm, char c)
{
    return strchr(sm->delim, c) == NULL && !is_eol_start(sm, c);
}

// stops if EOL begin is found
static unsigned token_sm2_find_delim(StateMachine2* sm, ConstSpan in, TokenStateMachineOut* dummy)
{
    unsigned i = 0;

    for(; i < in.sz && token_sm2_find_delim_helper(sm, in.buf[i]); ++i);

    return i;
}


// stops if EOL begin is found
static unsigned token_sm2_find_delim_out(StateMachine2* sm, ConstSpan in, TokenStateMachineOut* smo)
{
    const char* buf = in.buf;
    const char* const buf_end = buf + in.sz;
    char* out = smo->out;

    for(; buf < buf_end && token_sm2_find_delim_helper(sm, *buf); ++buf)
    {
        if(out < smo->out_end)
            *out++ = *buf;
        else
        {
            // DEBT: Indicate overflow tried to happen here
        }
    }

    smo->out = out;

    return buf - in.buf;
}


/// Evaluates the *FIRST* character of EOL
/// @return true if EOL start character found
/// DEBT: Side effects, updates sm->state
static bool token_sm2_eol_eval(StateMachine2* sm, char c)
{
    switch(c)
    {
        case 10:
            switch(sm->eol_mode)
            {
                case TOKEN_STATE_EOLMODE_LF:
                    sm->state = TOKEN_STATE_EOL;
                    break;

                    // CR or CRLF EOL mode is undefined behavior, for now.  We'll set a floating LF state, but
                    // that is experimental at this point
                default:
                    sm->state = TOKEN_STATE_LF;
                    // NOTE: We probably don't even need a TOKEN_STATE_LF since LFCR combo is not generally a thing
                    // Since this isn't an identified EOL state, return false
                    return false;
            }
            return true;

        case 13:
            switch(sm->eol_mode)
            {
                case TOKEN_STATE_EOLMODE_CRLF:
                    sm->state = TOKEN_STATE_CR;
                    break;

                case TOKEN_STATE_EOLMODE_CR:
                    sm->state = TOKEN_STATE_EOL;
                    break;

                default:
                    // DEBT: not sure what we should do if CR is encountered in LF only mode
                    break;
            }
            return true;
    }

    return false;
}

static void token_sm2_null_terminate(TokenStateMachineOut* smo, const char* buf)
{
    // in fnptr environment, SMO is always set, but in non fnptr environment
    // its presence or not determines what output mode is active
#if FEATURE_TOKENIZER_OUT_FNPTR
    if(smo->out)
#else
    if(smo)
#endif
    {
        *smo->out = 0;
        ++smo->out;     // Important during accumulator mode where many tokens share same output buffer
    }
    else
        // DANGER!  Only use null_terminate on buffers which truly are writable
        *(char*)buf = 0;
}


unsigned token_sm2_process(StateMachine2* sm, const ConstSpan in, TokenStateMachineOut* smo)
{
    TokenStates s = sm->state;
    unsigned processed;
    const char* buf = in.buf;

    switch(s)
    {
        case TOKEN_STATE_UNSTARTED:
            s = TOKEN_STATE_IDLE;
            processed = 0;
#if FEATURE_TOKENIZER_MARK_OUTPUT
            // in mark mode, part of our per token startup is to clear outputs
            if(smo) token_smout_reset(smo);
#endif
            break;

            // Semi-whitespace phase, eats up anything in delim
        case TOKEN_STATE_IDLE:
            processed = token_sm2_skip_delim(sm, in);

            if(processed < in.sz)
            {
                buf += processed;

                if(token_sm2_eol_eval(sm, *buf))
                {
                    // If EOL start found, eat it
                    ++processed;
                    s = sm->state;

                    // In this case, token processing is overwith so do null termination
                    if(sm->null_terminate) token_sm2_null_terminate(smo, buf);
                }
                else
                {
                    s = TOKEN_STATE_TOKEN;
#if FEATURE_TOKENIZER_MARK_OUTPUT
                    if(smo && smo->out == NULL) smo->out_start = (char*)buf;
#endif
                }
            }
            break;

            // Looks again for delim, acquiring token as we go
        case TOKEN_STATE_TOKEN:
#if FEATURE_TOKENIZER_OUT_FNPTR
            processed = smo->out_fn(sm, in, smo);
#else
            processed = smo == NULL ?
                token_sm2_find_delim(sm, in, NULL) :
                token_sm2_find_delim_out(sm, in, smo);
#endif

            if(processed < in.sz)
            {
                buf += processed;
                s = TOKEN_STATE_TOKEN_END;
#if FEATURE_TOKENIZER_MARK_OUTPUT
                if(smo && smo->out == NULL) smo->out_end = buf;
#endif
            }
            break;

        case TOKEN_STATE_TOKEN_END:

            // delimiter or CR found
            if(token_sm2_eol_eval(sm, *buf))
            {
                // Even if EOL processing turns out to not find a real EOL, ending token processing at *suspected*
                // EOL works well enough

                // If EOL start found, eat it
                processed = 1;
                s = sm->state;
            }
            else
            {
                processed = 0;
                s = TOKEN_STATE_DELIM;
            }

            if(sm->null_terminate) token_sm2_null_terminate(smo, buf);
            break;

        case TOKEN_STATE_LF:
            // DEBT - LF only state undefined, not treated as EOL so eat the LF here
            processed = 1;
            break;

        case TOKEN_STATE_CR:
            processed = 1;
            switch(sm->eol_mode)
            {
                case TOKEN_STATE_EOLMODE_CRLF:
                    if(*buf == 10)
                        s = TOKEN_STATE_EOL;
                    break;

                case TOKEN_STATE_EOLMODE_CR:
                case TOKEN_STATE_EOLMODE_LF:
                    // DEBT: Should not reach here, since initial encounter of CR in CR mode
                    // immediately promotes to EOL state
                    // DEBT: Not sure what we should do if we get CR during LF-only processing
                    break;
            }
            break;

        default:
            processed = 0;
            break;
    }

    sm->state = s;
    return processed;
}

void token_sm2_deinit(StateMachine2* sm)
{
}

void token_smout_init(TokenStateMachineOut* smo, char* buf, unsigned sz)
{
    smo->out_start = buf;
    smo->out = buf;
    smo->out_end = buf + sz;
#if FEATURE_TOKENIZER_OUT_FNPTR
    if(sz > 0)
        smo->out_fn = token_sm2_find_delim_out;
    else
        smo->out_fn = token_sm2_find_delim;
#endif
}

void token_smout_reset(TokenStateMachineOut* smo)
{
    // Comparing to null incase of "mark output" mode (name TBD)
    if(smo->out != NULL)
        smo->out = smo->out_start;
#if FEATURE_TOKENIZER_MARK_OUTPUT
    else
    {
        smo->out_start = NULL;
        smo->out_end = NULL;
    }
#endif
}


/// @return true if direct mode, false if scratch mode
static bool tokenizer_autoselect(Tokenizer* smh, unsigned in_sz, bool is_in_const)
{
    if(smh->out.out == NULL)
    {
        // we have no scratch buffer, so direct mode is a foregone conclusion
        return true;
    }

    // input buffer is truly const (i.e. writing to it would cause malfunctions or crashes)
    // but we have a null terminate directive, so we must go into scratch mode
    if(is_in_const && smh->sm.null_terminate)
        return false;

    unsigned scratch_sz = smh->out.out_end - smh->out.out_start;

    // If input size actually exceeds our scratch buffer, then overlap protection does not matter
    if(in_sz > scratch_sz)
    {
        return true;
    }

    // by this point, we've determined:
    // 1. we have a scratch buffer
    // 2. it's smaller than input size
    // 3. we aren't the special case of null terminating and true const
    return false;
}



//void tokenizer_init(Tokenizer* smh, ConstSpan in, const char *delim, TokenStateEolMode eol_mode)
void tokenizer_init(Tokenizer* smh, const char* in, unsigned in_sz, const char *delim, TokenStateEolMode eol_mode)
{
    smh->processed = 0;
    smh->in.buf = in;
    smh->in.sz = in_sz;
    token_smout_init(&smh->out, NULL, 0);

    token_sm2_init(&smh->sm, delim, eol_mode);
}


void tokenizer_deinit(Tokenizer* smh)
{
    token_sm2_deinit(&smh->sm);
}


void tokenizer_set_output(Tokenizer* smh, char* out, unsigned sz)
{
    token_smout_init(&smh->out, out, sz);
}


void tokenizer_set_input(Tokenizer* t, const char* buf, unsigned sz)
{
    t->in.buf = buf;
    t->in.sz = sz;
    t->processed = 0;
}


void tokenizer_reset(Tokenizer* smh)
{
    //char** out = &smh->out.out;

    token_sm2_reset(&smh->sm, NULL);

    // If using scratch buffer, auto reset our output back to effectively out_begin
    // which is 'token'
    // DEBT: We know this cast is safe since 'out' mode always uses a writable buffer, but even
    // so the less const-away casting we do the better
    // NOTE: accumulated_out mode means smh->token doesn't actually point to beginning
    //if(*out) *out = (char*)smh->token;
    // When not accumulating, then reset output back to beginning of scratch buffer (if it exists)
    if(!smh->sm.accumulate_out) token_smout_reset(&smh->out);
}


const char* tokenizer_get_token(Tokenizer* sm)
{
    TokenStates s = sm->sm.state;

    // We auto reset here instead of at the end so that consumers have the
    // chance to see whether we hit an EOL
    // DEBT: Should we consider runtime flagging auto reset?  Might be a bit overdoing it
    if(s == TOKEN_STATE_EOL || s == TOKEN_STATE_DELIM)
    {
        // auto reset time
        tokenizer_reset(sm);
        s = sm->sm.state;
    }

    // Be sure to assign 'out' helper after potential reset
    char* out = sm->out.out;

    while(s != TOKEN_STATE_EOL && s != TOKEN_STATE_DELIM && sm->in.sz > 0)
    {
#if FEATURE_TOKENIZER_OUT_FNPTR
        unsigned processed = token_sm2_process(&sm->sm, sm->in, &sm->out);
#else
        unsigned processed = token_sm2_process(&sm->sm, sm->in, out ? &sm->out : NULL);
#endif

        sm->processed += processed;

        switch(s)
        {
            case TOKEN_STATE_UNSTARTED:
                sm->token = out ? out : sm->in.buf;
                sm->length = 0;
                break;

            case TOKEN_STATE_IDLE:
                // This is where we skip initial whitespace.  Useful to know when
                // using direct buffer rather than temp out
                if(out == NULL)
                    sm->token += processed;
                break;

            case TOKEN_STATE_TOKEN:
                sm->length += processed;
                break;

            default: break;
        }

        sm->in.buf += processed;
        sm->in.sz -= processed;
        s = sm->sm.state;
    }

    // done! we have a token

    return sm->token;
}


unsigned short tokenizer_get_last_token_length(const Tokenizer* sm)
{
    return sm->length;
}