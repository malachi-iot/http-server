#pragma once

#include "bool.h"

// Treats 'tail'==nullptr as flag for empty queue
#ifndef FEATURE_QUEUE_TAILFLAG
#define FEATURE_QUEUE_TAILFLAG 1
#endif

typedef struct queue queue_t;
queue_t *queue_new(int size);
void queue_delete(queue_t **q);
bool queue_push(queue_t *q, void *elem);
bool queue_pop(queue_t *q, void **elem);

// non standard

bool queue_full(const queue_t* q);
bool queue_empty(const queue_t* q);
