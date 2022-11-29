// Code base to handle a particular HTTP header
#pragma once

#include "bool.h"
#include "header-constants.h"

#include "internal/header.h"



void http_header_init();
void http_header_deinit();

/// Adds a copy of header key for caching
/// @param key
/// @return the copy of 'key'
const char* add_http_header_key(const char* key);

/// Retrieves a header key from our key cache
/// @param key
/// @param force_case false means 'key' is already lower case, 'true' makes a copy of key and turns it lower case
/// @return
const char* get_http_header_key(const char* key, bool force_case);

/// Specialized const key version where key itself is static allocated
/// @param key
void add_http_header_const_key(const char* key);
/// After calling 'add_http_header_const_key', calling this is necessary.  May be called repeatedly
/// (though not recommended)
void http_header_const_key_sort();

void http_header_placement_new(HttpHeader* h, const char* key, const char* value);
void http_header_placement_delete(HttpHeader* h);

