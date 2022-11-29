#include <stdio.h>

#define UNITY_INCLUDE_PRINT_FORMATTED

#include <unity.h>

#include "thread-tests.h"
#include "queue.h"

void test_NonThreaded()
{
    int val = 777;
    //void *ref = &val;
    //void **ref2 = NULL;
    int* val2 = NULL;
    int** val3 = &val2;

    //printf("Hello %p\n", ref);

    queue_t* q = queue_new(10);

    queue_push(q, &val);
    queue_pop(q, (void**) val3);

    TEST_ASSERT_EQUAL(val, *val2);

    //printf("Hello, val2 = %d\n", *val2);

    queue_delete(&q);

    TEST_ASSERT_NULL(q);
}

void test_boundaries()
{
    queue_t* q = queue_new(4);

    union
    {
        void* raw;
        ptrdiff_t v;
    } u;

    queue_push(q, (void*) 1);
    queue_push(q, (void*) 2);
    queue_push(q, (void*) 3);

    TEST_ASSERT_FALSE(queue_full(q));

    queue_pop(q, &u.raw);
    TEST_ASSERT_EQUAL(1, u.v);

    queue_pop(q, &u.raw);
    TEST_ASSERT_EQUAL(2, u.v);

    queue_push(q, (void*) 4);
    queue_push(q, (void*) 5);

    TEST_ASSERT_FALSE(queue_full(q));

    queue_push(q, (void*) 6);

    TEST_ASSERT_TRUE(queue_full(q));

    queue_pop(q, &u.raw);
    TEST_ASSERT_EQUAL(3, u.v);

    queue_pop(q, &u.raw);
    TEST_ASSERT_EQUAL(4, u.v);

    queue_pop(q, &u.raw);
    TEST_ASSERT_EQUAL(5, u.v);

    queue_pop(q, &u.raw);
    TEST_ASSERT_EQUAL(6, u.v);

    TEST_ASSERT_TRUE(queue_empty(q));

    queue_delete(&q);

    q = queue_new(3);

    queue_push(q, (void*) 1);
    queue_push(q, (void*) 2);
    queue_push(q, (void*) 3);

    TEST_ASSERT_TRUE(queue_full(q));

    queue_delete(&q);
}


void queue_tests()
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_NonThreaded);
    RUN_TEST(test_boundaries);
    RUN_TEST(test_basic_thread);
    RUN_TEST(test_advanced_thread);
    RUN_TEST(test_assignment_thread);
}
