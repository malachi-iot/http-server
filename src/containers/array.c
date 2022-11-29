// All references see README.md
#include <err.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include "bool.h"
#include "array.h"


void array_placement_new(Array* a, int elem_size, int initial_max)
{
    if(initial_max == 0)
        a->buffer = NULL;
    else
        a->buffer = malloc(elem_size * initial_max);

    a->elem_size = elem_size;
    a->max = initial_max;
    a->count = 0;
}

Array* array_new(int elem_size, int initial_max)
{
    Array* a = malloc(sizeof(Array));
    array_placement_new(a, elem_size, initial_max);
    return a;
}

void array_placement_delete(const Array* a)
{
    free(a->buffer);
}

void array_reset(Array* array)
{
    free(array->buffer);
    array->buffer = NULL;
    array->count = 0;
}


void array_delete(Array* a)
{
    array_placement_delete(a);
    free(a);
}

void* array_end(Array* array)
{
    if(array->count == array->max)
    {
        if(array->max <= 1)
            array->max = 5;
        else
            array->max += array->max / 2;

        array->buffer = realloc(array->buffer, array->max * array->elem_size);
    }
    else if(array->count > array->max)
    {
        // We expect tight control over count - external modification can mess us up i.e. we don't
        // grow correctly
        warn("array count grew larger than expected");
    }

    return array->buffer + (array->count * array->elem_size);
}



void* array_add(Array* array, void* elem)
{
    void* buffer_elem = array_end(array);

    if(elem)    memcpy(buffer_elem, elem, array->elem_size);

    ++array->count;

    return buffer_elem;
}

void* array_get(Array* array, int index)
{
    int es = array->elem_size;
    void* buf = array->buffer + (index * es);
    return buf;
}


void array_get_copy(Array* array, int index, void* output)
{
    int es = array->elem_size;
    void* buf = array->buffer + (index * es);
    memcpy(output, buf, es);
}


void* array_bsearch(Array* a, const void* key, int (*comp)(const void*, const void*))
{
    void* result = (char*)bsearch(key, a->buffer,
                                  a->count, a->elem_size, comp);

    return result;
}

void array_qsort(Array* array, array_comp comp)
{
    // Guidance from [2]
    qsort(array->buffer, array->count, array->elem_size, comp);
}


SharedPtr* shared_ptr_new(void* value, int sz)
{
    SharedPtr* p;

    if(sz == 0)
    {
        p = malloc(sizeof(SharedPtr));

        p->value = value;
        p->inline_allocated = 0;
    }
    else
    {
        char* raw = malloc(sizeof(SharedPtr) + sz);

        p = (SharedPtr*)raw;
        p->value = raw + sizeof(SharedPtr);
        p->inline_allocated = 1;
    }

    p->reference_count = 1;

    return p;
}


void shared_ptr_delete(SharedPtr* p)
{
    if(--p->reference_count == 0)
    {
        if(!p->inline_allocated)
            free(p->value);

        free(p);
    }
}


void shared_ptr_ref(SharedPtr* p)
{
    ++p->reference_count;
}

void searchable_array_placement_new(SearchableArray* a, unsigned elem_size, unsigned max, array_comp comp)
{
    array_placement_new(&a->a, elem_size, max);

    a->comp = comp;
    a->limit = 0;
}


void searchable_array_placement_delete(SearchableArray* a)
{
    array_placement_delete(&a->a);
}


bool searchable_array_should_sort(SearchableArray* a)
{
    return a->a.count == a->a.max;
}


void searchable_array_sort(SearchableArray* a)
{
    array_qsort(&a->a, a->comp);
    a->limit = a->a.count;
}


void* searchable_array_add(SearchableArray* array, void* elem)
{
    void* added = array_add(&array->a, elem);

    if(elem != NULL)
    {
        if(searchable_array_should_sort(array))
        {
            searchable_array_sort(array);
        }
    }

    return added;
}


void* searchable_array_search(SearchableArray* a, const void* key, array_comp comp)
{
    if(comp == NULL)
        comp = a->comp;

    int i = a->limit;
    char* buf = a->a.buffer;
    const unsigned elem_size = a->a.elem_size;

    void* result = bsearch(key, buf, i, elem_size, comp);

    if(result == NULL)
    {   // If bsearch can't find anything, then brute force search in our overflow area
        buf += elem_size * i;

        for(; i < a->a.count; ++i)
        {
            if(comp(key, buf) == 0) return buf;
            buf += elem_size;
        }
    }

    return result;
}
