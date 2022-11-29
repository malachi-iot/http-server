#pragma once

typedef enum
{
    TOKEN_STATE_UNSTARTED,
    TOKEN_STATE_IDLE,       // TODO: Rename this to reflect delim/whitespace eater
    TOKEN_STATE_TOKEN,
    TOKEN_STATE_TOKEN_END,
    TOKEN_STATE_CR,
    TOKEN_STATE_LF,
    TOKEN_STATE_EOL,
    TOKEN_STATE_DELIM

} TokenStates;


typedef enum
{
    TOKEN_STATE_EOLMODE_CR,
    TOKEN_STATE_EOLMODE_LF,
    TOKEN_STATE_EOLMODE_CRLF,
} TokenStateEolMode;


