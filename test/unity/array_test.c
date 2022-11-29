#include "unit-tests.h"

#include "array.h"
#include "http/header.h"

int int_compare(const void* lhs, const void* rhs)
{
    int l = *(const int*) lhs;
    int r = *(const int*) rhs;

    if(l < r) return -1;

    return l > r;
}

static void http_header_array_tests()
{
    Array a;

    array_placement_new(&a, sizeof(HttpHeader), 4);

    TEST_ASSERT_EQUAL(0, a.count);
    TEST_ASSERT_EQUAL(4, a.max);
    TEST_ASSERT_EQUAL(sizeof(HttpHeader), a.elem_size);

    array_placement_delete(&a);
}

static void int_array_tests()
{
    Array a;

    array_placement_new(&a, sizeof(int), 4);

    TEST_ASSERT_EQUAL(0, a.count);
    TEST_ASSERT_EQUAL(4, a.max);
    TEST_ASSERT_EQUAL(sizeof(int), a.elem_size);

    int val = 5;
    array_add(&a, &val);

    TEST_ASSERT_EQUAL(val, *(int*)a.buffer);

    array_placement_delete(&a);
}

static void searchable_array_tests()
{
    SearchableArray a;
    int* result;

    searchable_array_placement_new(&a, sizeof(int), 4, int_compare);

    int v = 5;
    result = searchable_array_search(&a, &v, NULL);
    TEST_ASSERT_NULL(result);

    searchable_array_add(&a, &v);
    result = searchable_array_search(&a, &v, NULL);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(v, *result);

    v = 15;
    searchable_array_add(&a, &v);
    v = 30;
    searchable_array_add(&a, &v);
    v = 45;
    searchable_array_add(&a, &v);
    v = 60;
    searchable_array_add(&a, &v);
    v = 30;
    result = searchable_array_search(&a, &v, NULL);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(v, *result);

    v = 60;
    result = searchable_array_search(&a, &v, NULL);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(v, *result);

    searchable_array_placement_delete(&a);
}

// TODO: Move this elsewhere when we have got socket code running
char* get_line(char* request, int bytes, int* i) {
    request += *i;
    char* buf = request;

    while(*i < bytes)
    {
        ++(*i);

        // HTTP we typically see CRLF, so really we are looking
        // for CR here.  Reaching here is certainly EOL
        if(*buf == '\n' || *buf == '\r') {
            *buf = 0;
            ++buf;

            // HTTP we typically see CRLF, so really we are looking
            // for LF here
            if(*buf == '\n' || *buf == '\r')
            {
                ++(*i);
                ++buf;
            }

            return request;
        }

        ++buf;
    }

    return NULL;
}

#include <string.h>

char dummydata[] = "TEST LINE 1\r\n"
                  "TEST LINE 2\r\n"
                  "TEST LINE 3\r\n";

void array_tests()
{
    int i = 0;
    char* line;

    int sz = strlen(dummydata);

    line = get_line(dummydata, sz, &i);
    line = get_line(dummydata, sz, &i);
    line = get_line(dummydata, sz, &i);
    line = get_line(dummydata, sz, &i);

    http_header_array_tests();
    int_array_tests();
    searchable_array_tests();
}