// All references are in README.md
#pragma once

#include <pthread.h>

#include "array.h"
#include "bool.h"
#include "fwd.h"

// NOTE: Check out posix MQ's if we don't feel like using our own threadqueue [3] [3.1]

typedef struct queue queue_t;

typedef void *(*worker_routine)(void *);

typedef struct
{
    worker_routine delegate;
    void* arg;
    struct
    {
        unsigned shutdown : 1;

    } flags;

} WorkerMessage;

typedef struct ThreadPool
{
    queue_t* queue;
    Array threads;

} ThreadPool;

ThreadPool* threadpool_new(int count, worker_routine, void* arg);
void threadpool_placement_new(ThreadPool* pool, int count, worker_routine, void* arg);
void threadpool_delete(ThreadPool* t, int signo);
void threadpool_placement_delete(ThreadPool* t, int signo);

/// Queue up a worker to execute on the thread pool
/// @param t
/// @param arg
void threadpool_queue(ThreadPool* t, worker_routine, void* arg);
void threadpool_signal_shutdown(ThreadPool* t);

