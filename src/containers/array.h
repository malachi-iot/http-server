#pragma once

typedef int (*array_comp)(const void*, const void*);


typedef struct _Array
{
    char* buffer;
    unsigned short elem_size;
    unsigned short count;
    unsigned short max;

} Array;


// Tracks sorted and unsorted elements and searches both - this way one isn't
// doing a qsort every time one adds to the array
typedef struct _SearchableArray
{
    Array a;
    // how far in the sorted part goes
    unsigned limit;
    array_comp comp;

} SearchableArray;


typedef struct
{
    void* value;

    struct
    {
        unsigned reference_count : 15;
        unsigned inline_allocated : 1;
    };

} SharedPtr;


void array_placement_new(Array* a, int elem_size, int initial_max);
Array* array_new(int elem_size, int initial_max);
// peers one past the end of the array, expanding array size if necessary.  This
// is a low level call and even less atomic than other operations, so prefer to call 'array_add'
// in sparse mode
void* array_end(Array* array);

/// Adds element to end of array
/// @param array
/// @param elem element to copy, or NULL for sparse mode
/// @return pointer to added array element
/// @remarks can operate in 'sparse mode' where elem == null, meaning no elem copy happens
void* array_add(Array* array, void* elem);
void array_get_copy(Array* array, int index, void* output);
void* array_get(Array* array, int index);
void* array_bsearch(Array* array, const void* key, array_comp);
void array_qsort(Array* array, array_comp);
void array_reset(Array* array);
void array_delete(Array* array);
void array_placement_delete(const Array* array);

// initializes either with given value OR size - determined by size
SharedPtr* shared_ptr_new(void* value, int sz);
void shared_ptr_ref(SharedPtr* p);
void shared_ptr_delete(SharedPtr* p);

void searchable_array_placement_new(SearchableArray* a, unsigned elem_size, unsigned max, array_comp comp);
void searchable_array_placement_delete(SearchableArray* a);
void* searchable_array_add(SearchableArray* array, void* elem);
void* searchable_array_search(SearchableArray* array, const void* key, array_comp);

