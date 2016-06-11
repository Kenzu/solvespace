//-----------------------------------------------------------------------------
// Utility functions used by the Unix port. Notably, our memory allocation;
// we use two separate allocators, one for long-lived stuff and one for
// stuff that gets freed after every regeneration of the model, to save us
// the trouble of freeing the latter explicitly.
//
// Copyright 2008-2013 Jonathan Westhues.
// Copyright 2013 Daniel Richard G. <skunk@iSKUNK.ORG>
//-----------------------------------------------------------------------------
#include <time.h>
#include <execinfo.h>
#ifdef __APPLE__
#   include <strings.h> // for strcasecmp
#endif

#include "solvespace.h"

namespace SolveSpace {

void dbp(const char *str, ...)
{
    va_list f;
    static char buf[1024*50];
    va_start(f, str);
    vsnprintf(buf, sizeof(buf), str, f);
    va_end(f);

    fputs(buf, stderr);
    fputc('\n', stderr);
}

void assert_failure(const char *file, unsigned line, const char *function,
                    const char *condition, const char *message) {
    fprintf(stderr, "File %s, line %u, function %s:\n", file, line, function);
    fprintf(stderr, "Assertion '%s' failed: ((%s) == false).\n", message, condition);

    static void *ptrs[1024] = {};
    size_t nptrs = backtrace(ptrs, sizeof(ptrs) / sizeof(ptrs[0]));
    char **syms = backtrace_symbols(ptrs, nptrs);

    fprintf(stderr, "Backtrace:\n");
    if(syms != NULL) {
        for(size_t i = 0; i < nptrs; i++) {
            fprintf(stderr, "%2zu: %s\n", i, syms[i]);
        }
    } else {
        for(size_t i = 0; i < nptrs; i++) {
            fprintf(stderr, "%2zu: %p\n", i, ptrs[i]);
        }
    }

    abort();
}

bool PathEqual(const std::string &a, const std::string &b)
{
#if defined(__APPLE__)
    // Case-sensitivity is actually per-volume on OS X,
    // but it is tedious to implement and test for little benefit.
    return !strcasecmp(a.c_str(), b.c_str());
#else
    return a == b;
#endif
}

FILE *ssfopen(const std::string &filename, const char *mode)
{
    ssassert(filename.length() == strlen(filename.c_str()),
             "Unexpected null byte in middle of a path");
    return fopen(filename.c_str(), mode);
}

void ssremove(const std::string &filename)
{
    ssassert(filename.length() == strlen(filename.c_str()),
             "Unexpected null byte in middle of a path");
    remove(filename.c_str());
}

//-----------------------------------------------------------------------------
// A separate heap, on which we allocate expressions. Maybe a bit faster,
// since fragmentation is less of a concern, and it also makes it possible
// to be sloppy with our memory management, and just free everything at once
// at the end.
//-----------------------------------------------------------------------------

typedef struct _AllocTempHeader AllocTempHeader;

typedef struct _AllocTempHeader {
    AllocTempHeader *prev;
    AllocTempHeader *next;
} AllocTempHeader;

static AllocTempHeader *Head = NULL;

void *AllocTemporary(size_t n)
{
    AllocTempHeader *h =
        (AllocTempHeader *)malloc(n + sizeof(AllocTempHeader));
    h->prev = NULL;
    h->next = Head;
    if(Head) Head->prev = h;
    Head = h;
    memset(&h[1], 0, n);
    return (void *)&h[1];
}

void FreeTemporary(void *p)
{
    AllocTempHeader *h = (AllocTempHeader *)p - 1;
    if(h->prev) {
        h->prev->next = h->next;
    } else {
        Head = h->next;
    }
    if(h->next) h->next->prev = h->prev;
    free(h);
}

void FreeAllTemporary(void)
{
    AllocTempHeader *h = Head;
    while(h) {
        AllocTempHeader *f = h;
        h = h->next;
        free(f);
    }
    Head = NULL;
}

void *MemAlloc(size_t n) {
    void *p = malloc(n);
    ssassert(p != NULL, "Cannot allocate memory");
    return p;
}

void MemFree(void *p) {
    free(p);
}

void InitHeaps(void) {
    /* nothing to do */
}

};
