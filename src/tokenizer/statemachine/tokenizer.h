#pragma once

#include "enum.h"
#include "fwd.h"

void token_smout_init(TokenStateMachineOut* smo, char* buf, unsigned sz);
void token_smout_reset(TokenStateMachineOut* smo);

void tokenizer_init(Tokenizer* smh, const char* in, unsigned in_sz, const char *delim, TokenStateEolMode eol_mode);

/// Useful to continue tokenizing in scratch mode against a brand new buffer
/// @param t
/// @param buf
/// @param sz
void tokenizer_set_input(Tokenizer* t, const char* buf, unsigned sz);

/// Sets output buffer for scratch mode
/// @param smh
/// @param out
/// @param sz
void tokenizer_set_output(Tokenizer* smh, char* out, unsigned sz);

void tokenizer_reset(Tokenizer* t);
const char* tokenizer_get_token(Tokenizer* sm);
unsigned short tokenizer_get_last_token_length(const Tokenizer* sm);
void tokenizer_deinit(Tokenizer* smh);