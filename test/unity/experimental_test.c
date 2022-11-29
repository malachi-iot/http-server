#include <string.h>

#include <unity.h>

#include <util.h>

typedef struct DoubleBuffer
{
    Span underlying;
    Span temporary;

    // ala std streambuf - "current character (get pointer) in the get area"
    const char* gptr;
    // ala std streambuf - "current character (put pointer) in the put area"
    char* pptr;

    // When building token, we use this
    char* temp_pos;

} DoubleBuffer;

// like strpbrk - but does not look for null termination, only looks for delimiters or up
// to end
// NOTE: matching esoteric behavior of strpbrk which returns a non const version of 'target'
char* mempbrk(const char* target, const char* breakset, const char* end)
{
    const char* const breakset_start = breakset;
    const char* const breakset_end = breakset + strlen(breakset);

    while(target < end)
    {
        char c = *target;

        // More or less strchr here - inlining for performance
        for(breakset = breakset_start; breakset < breakset_end; ++breakset)
            if(c == *breakset)  return (char*)target;

        ++target;
    }

    return NULL;
}

void double_buffer_placement_new(DoubleBuffer* db, Span underlying, Span temporary)
{
    db->underlying = underlying;
    db->temporary = temporary;
    db->gptr = underlying.buf;
    db->pptr = underlying.buf;
}

char* double_buffer_memtok_begin(DoubleBuffer* db, const char* delim, char** token_end)
{
    const char* buf = db->gptr;
    const char* start = buf;
    const char* end = db->pptr;

    *token_end = mempbrk(buf, delim, end);

    // If no end of token found, prep temporary buffer in anticipation of token
    // found at end phase
    if(*token_end == NULL)
    {
        // Copy the entire remainder of primary buffer into temporary
        unsigned sz = end - start;
        memcpy(db->temporary.buf, start, sz);
        db->temp_pos = db->temporary.buf + sz;
        return NULL;
    }

    // Otherwise if end of token WAS found, token_end reflects it so return immediately

    return (char*)start;
}

// If temporary buffer is ultimately returned, then token_end points to end of temporary
// buffer.
char* double_buffer_memtok_end(DoubleBuffer* db, const char* delim, char** token_end)
{
    // *token_end == NULL means we didn't yet find a token on _being phase
    if(*token_end != NULL)
    {
        // If we DID find a token, then finish up positioning immediately
        const char* buf = db->gptr;
        db->gptr = *token_end;
        return (char*)buf;
    }

    // It is presumed that reaching here means primary buffer has been repopulated with new
    // data and to continue looking there to find our token.  It is also presumed we are
    // at gpos=0 (zero read position) in primary buffer now

    const char* buf = db->gptr;
    const char* start = buf;
    const char* end = db->pptr;

    // Look in the newly repopulated buffer for token delimiter
    *token_end = mempbrk(buf, delim, end);

    if(*token_end != NULL)
    {
        // If found, append to temporary buffer
        unsigned sz = *token_end - start;
        memcpy(db->temp_pos, start, sz);

        // This tells us finally how many bytes are in temp_pos - nothing is null terminated
        db->temp_pos += sz;
        *token_end = db->temp_pos;
        return db->temporary.buf;
    }

    return NULL;
}


char* double_buffer_reset_primary(DoubleBuffer* db)
{
    char* buf = db->underlying.buf;
    db->gptr = buf;
    db->pptr = buf;
    return buf;
}


// allocates count bytes for writing - not sure how useful this is really
char* double_buffer_request(DoubleBuffer* db, unsigned count)
{
    char* pptr = db->pptr + count;
    char* epptr = db->underlying.buf + db->underlying.sz;
    if(pptr <= epptr)
    {
        char* temp = db->pptr;
        db->pptr = pptr;
        return temp;
    }

    return NULL;
}



static const char test_str1[] = "Hello World!  How are you today?";
#define TEST_SYNTHETIC_HEADER_FIELD "Too-Long-To-Accept-Type"
static const char test_synthetic_header[] =
    TEST_SYNTHETIC_HEADER_FIELD ": 'some super long extremely unrealistic type size'";


static void test_double_buffer()
{
    DoubleBuffer db;
    Span primary;
    Span temporary;

    char buf_p[16];
    char buf_t[64];

    primary.buf = buf_p;
    primary.sz = sizeof(buf_p);
    temporary.buf = buf_t;
    temporary.sz = sizeof(buf_t);
    int pos = 0;
    const int max1 = sizeof(test_synthetic_header) - 1;
    const int max2 = sizeof(buf_p);

    double_buffer_placement_new(&db, primary, temporary);

    char* b = double_buffer_request(&db, max2);

    TEST_ASSERT_NOT_NULL(b);

    memcpy(b, test_synthetic_header, max2);

    pos += max2;
    char* state;
    char* b2;

    b2 = double_buffer_memtok_begin(&db, ":", &state);

    TEST_ASSERT_NULL(b2);

    memcpy(b, test_synthetic_header + pos, max2);

    pos += max2;

    b2 = double_buffer_memtok_end(&db, ":", &state);

    TEST_ASSERT_EQUAL_STRING(TEST_SYNTHETIC_HEADER_FIELD, b2);

    char* v = mempbrk(test_str1, "!", test_str1 + sizeof(test_str1) - 1);

    TEST_ASSERT_NOT_NULL(v);
    TEST_ASSERT_EQUAL('!', *v);
}

void experimental_tests()
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(test_double_buffer);
}