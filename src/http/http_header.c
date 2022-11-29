#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <logger.h>

#include "array.h"
#include "string.h"     // Just a formality, this actually gets included above with <string.h>

#include "http/header.h"
#include "http/headers.h"

#if !defined(__clang__) && defined(__GCC__)
#define FEATURE_OBJSTACK 1
// As per https://www.gnu.org/software/libc/manual/html_node/Preparing-for-Obstacks.html
// Do this for our header values per request (including response header values, if we care)
#include <obstack.h>
#endif

// MUST be lower case
const char* const HTTP_HEADER_FIELD_ACCEPT = "accept";
const char HTTP_HEADER_FIELD_CONTENT_LENGTH[] = "content-length";
const char* const HTTP_HEADER_FIELD_CONTENT_TYPE = "content-type";
const char HTTP_HEADER_FIELD_HOST[] = "host";
const char* const HTTP_HEADER_FIELD_REQUEST_ID = "request-id";


static SearchableArray header_keys;
static Array const_header_keys;

static pthread_mutex_t header_keys_mutex;

static bool is_http_header_initialized()
{
    return const_header_keys.max > 0;
}

static int compare_strings (const void *a, const void *b)
{
    const char* const * lhs = a;
    const char* const * rhs = b;
    return strcmp(*lhs, *rhs);
}



static const char* add_http_header_key_internal(SearchableArray* keys, const char* key)
{
    assert(is_http_header_initialized());

    //printf("add_http_header_key: %s\n", key);

    const char* copied_key = malloc_string(key);

    pthread_mutex_lock(&header_keys_mutex);
    searchable_array_add(keys, &copied_key);
    pthread_mutex_unlock(&header_keys_mutex);

    return copied_key;
}


void add_http_header_const_key(const char* key)
{
    array_add(&const_header_keys, &key);
}


void http_header_const_key_sort()
{
    array_qsort(&const_header_keys, compare_strings);
}


const char* add_http_header_key(const char* key)
{
    // qsort/bsearch guidance from [2]
    return add_http_header_key_internal(&header_keys, key);
}


const char* get_http_header_key(const char* key, bool force_case)
{
    assert(is_http_header_initialized());

    char copied[64];

    // Since [8.1] tells us case insensitivity is the norm, we internally
    // treat everything as lower case
    if(force_case)
    {
        tolower_string_copy(copied, key);
        key = copied;
    }

    char** result;

    result = array_bsearch(&const_header_keys, &key, compare_strings);

    if(result != NULL)
        return *result;

    pthread_mutex_lock(&header_keys_mutex);
    searchable_array_search(&header_keys, &key, NULL);
    pthread_mutex_unlock(&header_keys_mutex);

    if(result == NULL) return add_http_header_key(key);

    return *result;
}



static void delete_http_header_keys()
{
    pthread_mutex_lock(&header_keys_mutex);
    for(int i = 0; i < header_keys.a.count; ++i)
    {
        char** copied_key = array_get(&header_keys.a, i);

        free(*copied_key);
    }
    searchable_array_placement_delete(&header_keys);
    pthread_mutex_unlock(&header_keys_mutex);
}


void http_header_placement_new(HttpHeader* h, const char* key, const char* value)
{
    LOG_TRACE("http_header new: %s:%s", key, value);

    // DEBT: For now, copying and lowercasing the key, just to make life easy.
    // would be better to tolower it up further in the chain when we know that's
    // what's needed (i.e. http_request state machine)
    h->key = get_http_header_key(key, true);
    h->value = malloc_string(value);
}


void http_header_placement_delete(HttpHeader* h)
{
    LOG_TRACE("header delete: %s:%p", h->key, h->value);

    free((void*)h->value);
}


void http_header_init()
{
    // repeat-callable, so that we can call this from pipeline init too (since we could run multiple servers)
    if(is_http_header_initialized()) return;

    pthread_mutex_init(&header_keys_mutex, NULL);

    searchable_array_placement_new(&header_keys, sizeof(char*), 10, compare_strings);
    array_placement_new(&const_header_keys, sizeof(const char*), 10);

    // This one is just for debug
    add_http_header_key("key1");

    add_http_header_const_key(HTTP_HEADER_FIELD_CONTENT_TYPE);
    add_http_header_const_key(HTTP_HEADER_FIELD_CONTENT_LENGTH);
    add_http_header_const_key(HTTP_HEADER_FIELD_ACCEPT);
    add_http_header_const_key(HTTP_HEADER_FIELD_HOST);
    add_http_header_const_key(HTTP_HEADER_FIELD_REQUEST_ID);

    http_header_const_key_sort();
}

void http_header_deinit()
{
    // Just a formality in case someone is trying to use the system while we're shutting down
    array_reset(&const_header_keys);

    delete_http_header_keys();

    pthread_mutex_destroy(&header_keys_mutex);

    array_placement_delete(&const_header_keys);
}

void http_headers_placement_new(HttpHeaders* h, int initial_capacity)
{
    array_placement_new(&h->headers, sizeof(HttpHeader), 10);
}


void http_headers_placement_delete(const HttpHeaders* h)
{
    HttpHeader* header = (HttpHeader*)h->headers.buffer;
    HttpHeader* const h_end = header + h->headers.count;

    for(; header < h_end; ++header)
    {
        http_header_placement_delete(header);
    }

    array_placement_delete(&h->headers);
}


const HttpHeader* http_headers_add_header(HttpHeaders* h, const char* key, const char* value)
{
    HttpHeader* header = array_add(&h->headers, NULL);
    http_header_placement_new(header, key, value);
    return header;
}

const HttpHeader* http_headers_get_headers(const HttpHeaders* h)
{
    return (const HttpHeader*)h->headers.buffer;
}


// DEBT: We might consider sorting these and doing a bsearch, but I really doubt there is much benefit and
// we might want to retain order of appearance
const HttpHeader* http_headers_get_header(const HttpHeaders* h, const char* key)
{
    const HttpHeader* header = (const HttpHeader*)h->headers.buffer;
    unsigned count = h->headers.count;

    while(count--)
    {
        // Just incase we are checking for one of our const versions, which is not unusual
        if(header->key == key) return header;

        if(strcmp(header->key, key) == 0) return header;
    }

    return NULL;
}

const char* http_headers_get_header_value(const HttpHeaders* h, const char* key)
{
    const HttpHeader* header = http_headers_get_header(h, key);

    if(header == NULL) return NULL;

    return header->value;
}
