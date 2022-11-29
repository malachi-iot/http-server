#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

//#include <unistd.h>
//#include <sys/types.h>

#include "queue.h"

// traditional 'eat a slot' flavor:
// empty   - head == tail
// full    - (head == tail - 1) or (head == end && tail == storage)

// counter flavor is self-explanatory
// NOTE: We only employ a counter for diagnostics, it's not used for full/empty detection

// technique we use closely resembles the answer given at [8]
// empty    - tail == null
// full     - tail == head

#if !FEATURE_QUEUE_TAILFLAG && !defined(USE_COUNTER)
#define USE_COUNTER 1
#endif

#define USE_DEDICATED_CV 1

typedef struct queue
{
    void** storage;
    void** head;
    void** tail;            // we almost can const this, but we use the same incrementer on head and tail - and head cannot be const, which means incrementer must stay non const
    void* const* end;       // const here disallows assignment to end
#if USE_COUNTER
    int count;
#endif

    pthread_mutex_t mutex;

    // As per [9] I like dedicated variables a little better - reduce spurious wakeups for buffers which
    // are constantly getting filled and emptied and blocked on both conditions
#if USE_DEDICATED_CV
    pthread_cond_t cv_pushed, cv_popped;
#else
    // Guidance from [3] and [4] and cherry picked guidance from [2] (it's got some issues)
    pthread_cond_t cv;
#endif

}   queue_t;


// NOTE: Not doing placement new version because in this case we are only doing one
// malloc.  Yes, placement new would still be more efficient but not by much.
queue_t* queue_new(int size)
{
    queue_t* q = (queue_t*) malloc(sizeof(queue_t) + (sizeof(void*) * size));
    q->storage = (void**)(q + 1);
    q->head = q->storage;
#if FEATURE_QUEUE_TAILFLAG
    q->tail = NULL;
#else
    q->tail = q->head;
#endif
#if USE_COUNTER
    q->count = 0;
#endif

    q->end = q->storage + size - 1;

    // [1] for guidance
    pthread_mutex_init(&q->mutex, NULL);
#if USE_DEDICATED_CV
    pthread_cond_init(&q->cv_pushed, NULL);
    pthread_cond_init(&q->cv_popped, NULL);
#else
    pthread_cond_init(&q->cv, NULL);
#endif


    return q;
}

static void queue_delete_helper(queue_t* q)
{
#if USE_DEDICATED_CV
    pthread_cond_destroy(&q->cv_pushed);
    pthread_cond_destroy(&q->cv_popped);
#else
    pthread_cond_destroy(&q->cv);
#endif
    pthread_mutex_destroy(&q->mutex);

    free(q);
}

void queue_delete(queue_t **q)
{
    queue_delete_helper(*q);
    *q = NULL;
}

static bool is_empty(const queue_t* q)
{
#if FEATURE_QUEUE_TAILFLAG
    return q->tail == NULL;
#else
    return q->head == q->tail;
#endif
}

static bool is_full(const queue_t* q)
{
#if FEATURE_QUEUE_TAILFLAG
    return q->head == q->tail;
#else
    // "eat one slot" approach
    return (q->head == q->tail - 1) | (q->head == q->end && q->tail == q->storage);
#endif
}

// p triple pointer:
// 1. addr of struct in which it lives
// 2. addr of position in storage
// 3. addr of user-supplied value
static void increment(const queue_t* q, void *** p)
{
    if(++*p > q->end) *p = q->storage;
}

static int lock(queue_t* q)
{
    return pthread_mutex_lock(&q->mutex);
}

static int unlock(queue_t* q)
{
    return pthread_mutex_unlock(&q->mutex);
}


bool queue_push(queue_t *q, void *elem)
{
    lock(q);

#if FEATURE_QUEUE_TAILFLAG
    if(is_empty(q))
    {
        // tail no longer NULL, means we have something in our queue
        q->tail = q->head;
    }
    else
#endif

    while(is_full(q))
    {
#if USE_DEDICATED_CV
        pthread_cond_wait(&q->cv_popped, &q->mutex);
#else
        pthread_cond_wait(&q->cv, &q->mutex);
#endif
    }

    *q->head = elem;
    increment(q, &q->head);

#if USE_COUNTER
    ++q->count;
#endif

    // signaling here rather than after unlock so that any waiting pop can begin its unblock process a little
    // sooner -AND- stands a better chance of mutex'ing in before another push occurs
#if USE_DEDICATED_CV
    pthread_cond_signal(&q->cv_pushed);
#else
    pthread_cond_signal(&q->cv);
#endif

    unlock(q);

    return true;
}


bool queue_pop(queue_t *q, void **elem)
{
    lock(q);

    while(is_empty(q))
    {
        // neither compile
        //pid_t tid = syscall(SYS_gettid);
        //pid_t tid = gettid();

        //pthread_t t = pthread_self();

        //fprintf(stderr, "[%lu] Queue empty\n", t);
#if USE_DEDICATED_CV
        pthread_cond_wait(&q->cv_pushed, &q->mutex);
#else
        pthread_cond_wait(&q->cv, &q->mutex);
#endif
        //fprintf(stderr, "[%lu] Queue activity signaled\n", t);
    }

    *elem = *q->tail;
    increment(q, &q->tail);

#if FEATURE_QUEUE_TAILFLAG
    // If we pop to the point we've met the head again, we know we're empty now
    // (that part is not experimental, just hard to record)
    if(q->head == q->tail)
        q->tail = NULL;
#endif

#if USE_COUNTER
    --q->count;
#endif

#if USE_DEDICATED_CV
    pthread_cond_signal(&q->cv_popped);
#else
    pthread_cond_signal(&q->cv);
#endif


    unlock(q);

    return true;
}

// Non standard functions

bool queue_full(const queue_t* q)
{
    return is_full(q);
}

bool queue_empty(const queue_t* q)
{
    return is_empty(q);
}

