#pragma once

// We're basically adding to existing string.h this way
#include_next <string.h>

char* trim_end(char* s, int len);

/// malloc a copy of a string
/// @param s
/// @return copy of s
char* malloc_string(const char* s);

// tolower's a string in place
void tolower_string(char* s);

// tolower's an copies a string
void tolower_string_copy(char* dest, const char* src);