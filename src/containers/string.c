#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "string.h"

char* trim_end(char* s, int len)
{
    char* v = s + len;

    while(v > s && isspace(*--v))
        *v = 0;

    return s;
}

// TODO: Rename to strdup after ifdef'ing to not collide with POSIX and C23 version
// As per
// https://en.cppreference.com/w/c/string/byte/strdup
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/strdup.html
char* malloc_string(const char* s)
{
    const int sz = strlen(s) + 1;
    char* buf = malloc(sz);
    memcpy(buf, s, sz);
    return buf;
}


void tolower_string(char* s)
{
    for(;*s != 0; ++s)
        *s = tolower(*s);
}


void tolower_string_copy(char* dst, const char* src)
{
    for(;*src != 0; ++src, ++dst)
        *dst = tolower(*src);

    *dst = 0;
}