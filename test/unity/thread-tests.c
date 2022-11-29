// Specifically, queue + thread interactions

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define UNITY_INCLUDE_PRINT_FORMATTED

#include <unity.h>

#include "queue.h"
#include "thread-tests.h"

#define handle_error_en(en, msg) \
               do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
               do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define passert(en, msg) if(en != 0) handle_error_en(en, msg)

#define NS_TO_MS(ns)    (ns * 1000000L)

static void* basic_thread(void* arg)
{
    queue_t* q = (queue_t*) arg;

    struct timespec rqtp = { 0, NS_TO_MS(100) };

    nanosleep(&rqtp, NULL);

    queue_push(q, (void*) 7);
    queue_push(q, (void*) 77);

    return arg;
}

void test_basic_thread()
{
    pthread_t t;
    void* v;
    queue_t* q = queue_new(10);

    // Thread creation guidance from [5]
    int s = pthread_create(&t, NULL, &basic_thread, q);

    passert(s, "pthread_create");

    queue_pop(q, &v);

    TEST_ASSERT_EQUAL(7, v);

    s = pthread_join(t, NULL);

    passert(s, "pthread_join");

    queue_pop(q, &v);

    TEST_ASSERT_EQUAL(77, v);

    queue_delete(&q);
}

static void* advanced_thread1(void* arg)
{
    queue_t* q = (queue_t*) arg;
    void* v;

    queue_pop(q, &v);

    return v;
}

static void* advanced_thread2(void* arg)
{
    queue_t* q = (queue_t*) arg;

    queue_push(q, (void*) 7);
    queue_push(q, (void*) 77);

    return NULL;
}

typedef struct t3_response
{
    int count_7;
    int count_77;

} t3_response_t;

static void* advanced_thread3(void* arg)
{
    queue_t* q = (queue_t*) arg;
    static t3_response_t r;

    union _u
    {
        void* v;
        int _v;
    } u;

    queue_pop(q, &u.v);

    TEST_ASSERT(u._v == 7 || u._v == 77);

    if(u._v == 7)
        ++r.count_7;
    else
        ++r.count_77;

    return &r;
}

static t3_response_t* test_advanced_thread_iteration(queue_t* q)
{
    pthread_t t[3];

    int s;
    void* v[3];

    s = pthread_create(&t[0], NULL, &advanced_thread1, q);
    passert(s, "pthread_create");
    s = pthread_create(&t[1], NULL, &advanced_thread2, q);
    passert(s, "pthread_create");
    s = pthread_create(&t[2], NULL, &advanced_thread3, q);
    passert(s, "pthread_create");

    for(int i = 0; i < 3; ++i)
    {
        s = pthread_join(t[i], &v[i]);
        passert(s, "pthread_join");
    }

    t3_response_t* r = (t3_response_t*) v[2];

    return r;
}


void test_advanced_thread()
{
    queue_t* q = queue_new(3);

    t3_response_t* r;

    for(int i = 0; i < 100; ++i)
        r = test_advanced_thread_iteration(q);

    TEST_ASSERT_GREATER_OR_EQUAL(1, r->count_7);
    TEST_ASSERT_GREATER_OR_EQUAL(1, r->count_77);

    // In debugger, 77 is always encountered WAY MORE than 7 because (I think)
    // thread1 grabs up the 7 right away.  Outside of debugger though it's a much more even match
    //TEST_ASSERT_GREATER_OR_EQUAL(r->count_77, r->count_7);

    TEST_PRINTF("7=%d, 77=%d", r->count_7, r->count_77);

    queue_delete(&q);
}


typedef struct assignment_thread_arg
{
    int id;
    queue_t* q;
    int counter;

} assignment_thread_arg_t;

typedef union assignment_thread_value
{
    struct
    {
        int id : 8;
        int counter : 8;

    }   v;

    ptrdiff_t raw;
    void* ptr;

} assignment_thread_value_t;

static const int assignment_count_to = 4;

static void* assignment_thread(void* arg)
{
    assignment_thread_arg_t* a = arg;
    queue_t* q = a->q;

    for(int i = 0; i < assignment_count_to; i++)
    {
        assignment_thread_value_t v = {{a->id, i}};

        queue_push(q, (void*) v.raw);
    }

    return arg;
}

void test_assignment_thread_iteration(queue_t* q)
{
    assignment_thread_arg_t
        a1 = { 0, q, 0 },
        a2 = { 1, q, 0 };

    assignment_thread_arg_t a[] = { a1, a2 };
    int a_count = 2;

    pthread_t t[a_count];
    int s;

    for(int i = 0; i < a_count; i++)
    {
        s = pthread_create(&t[i], NULL, &assignment_thread, &a[i]);
        passert(s, "pthread_create");
    }

    for(int i = 0; i < a_count; i++)
        pthread_join(t[i], NULL);

    for(int i = 0; i < a_count * assignment_count_to; i++)
    {
        assignment_thread_value_t v;

        queue_pop(q, &v.ptr);

        TEST_ASSERT_EQUAL(a[v.v.id].counter, v.v.counter);

        ++a[v.v.id].counter;
    }
}


void test_assignment_thread()
{
    queue_t* q = queue_new(assignment_count_to * 2);

    for(int i = 0; i < 100; i++)
    {
        //test_assignment_thread_iteration(q);
    }

    queue_delete(&q);
}

