// All references see README.md
#include <assert.h>
#include <memory.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

//#include <bits/syscall.h>

#include <logger.h>

#include "queue.h"
#include "threadpool.h"

#ifdef ESP_PLATFORM
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

static void* _worker(void* arg)
{
    ThreadPool* thread_pool = arg;
    queue_t* q = thread_pool->queue;
    WorkerMessage* m;

    // No love [10]
    //syscall(SYS_gettid);

#if ESP_PLATFORM
    // NOTE: Doesn't link
    //unsigned t = uxTaskGetTaskNumber(NULL);
    static unsigned t = 0;
    ++t;
    LOG_INFO("Worker thread starting: %u", t);
#else
    // DEBT: A little redundant since logger also tells us what thread it's on (NICE)
    // however I'd like to know how they did that and use it myself
    pthread_t t = pthread_self();

    LOG_INFO("Worker thread starting: %ld", t);
#endif

    bool shutdown = false;

    do
    {
        //void** p = &m;
        // DEBT: Obnoxious typecast
        queue_pop(q, (void**)&m);

        shutdown = m->flags.shutdown;

        if(m->delegate)
        {
            m->delegate(m->arg);

            // DEBT: I really hate magically knowing to free WorkerMessage here.  Would much prefer
            // a shared ptr, a value copy, or even a unique ptr if possible
            // For example, this is NOT dynamically allocated on shutdown.....
            free(m);
        }
    }
    while(!shutdown);

    LOG_DEBUG("Worker thread shutdown");

    return NULL;
}

void threadpool_placement_new(ThreadPool* p, int count, worker_routine worker, void* arg)
{
    assert(("need a thread_count", count > 0));

    int s, num_threads = count;
    pthread_attr_t attr;
    //ssize_t stack_size;
    void *res;

    array_placement_new(&p->threads, sizeof(pthread_t), count);

    if(worker == NULL)
    {
        worker = _worker;
        arg = p;
    }

    p->queue = queue_new(count * 4);

    s = pthread_attr_init(&attr);

    for(int i = 0; i < count; ++i)
    {
        pthread_t _pt;
        pthread_t* pt = &_pt;// array_get(&p->threads, i);

        // Guidance from [4] and also has interesting getopt usage
        pthread_create(pt, &attr, worker, arg);

        array_add(&p->threads, pt);
    }
}

ThreadPool* threadpool_new(int count, worker_routine worker, void* arg)
{
    ThreadPool* p = malloc(sizeof(ThreadPool));
    threadpool_placement_new(p, count, worker, arg);
    return p;
}


// NOTE: No printfs here since [6.1] tells us not to during a sigint
void threadpool_placement_delete(ThreadPool* t, int signo)
{
    WorkerMessage shutdown;

    shutdown.delegate = NULL;
    shutdown.flags.shutdown = 1;

    for(int i = 0; i < t->threads.count; ++i)
    {
        queue_push(t->queue, &shutdown);
    }

    for(int i = 0; i < t->threads.count; ++i)
    {
        pthread_t* pt = array_get(&t->threads, i);
        void *ret;

        pthread_join(*pt, &ret);
    }

    if(signo == 0)
    {
        array_placement_delete(&t->threads);
        queue_delete(&t->queue);
    }
}

void threadpool_delete(ThreadPool* t, int signo)
{
    threadpool_placement_delete(t, signo);
    if(signo == 0)
    {
        free(t);
    }
}

void threadpool_queue(ThreadPool* t, worker_routine w, void* arg)
{
    WorkerMessage* m = malloc(sizeof(WorkerMessage));
    m->flags.shutdown = 0;
    m->arg = arg;
    m->delegate = w;

    queue_push(t->queue, m);
}