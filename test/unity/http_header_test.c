#include <unity.h>

#include "array.h"
#include "http/header.h"

static void array_of_header_tests()
{
    Array a;

    array_placement_new(&a, sizeof(HttpHeader), 4);

    HttpHeader h;

    http_header_placement_new(&h, "key1", "hi2u");

    array_add(&a, &h);

    http_header_placement_new(&h, "key2", "hi2u2");

    array_add(&a, &h);

    HttpHeader* _h = array_get(&a, 0);

    http_header_placement_delete(_h);

    _h = array_get(&a, 1);

    http_header_placement_delete(_h);

    array_placement_delete(&a);
}

void http_header_tests()
{
    HttpHeader h;

    http_header_placement_new(&h, "key1", "hi2u");
    http_header_placement_delete(&h);

    array_of_header_tests();
}