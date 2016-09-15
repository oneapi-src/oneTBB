/*
    Copyright 2005-2015 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks. Threading Building Blocks is free software;
    you can redistribute it and/or modify it under the terms of the GNU General Public License
    version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
    distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See  the GNU General Public License for more details.   You should have received a copy of
    the  GNU General Public License along with Threading Building Blocks; if not, write to the
    Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA

    As a special exception,  you may use this file  as part of a free software library without
    restriction.  Specifically,  if other files instantiate templates  or use macros or inline
    functions from this file, or you compile this file and link it with other files to produce
    an executable,  this file does not by itself cause the resulting executable to be covered
    by the GNU General Public License. This exception does not however invalidate any other
    reasons why the executable file might be covered by the GNU General Public License.
*/

#ifdef __cplusplus
#error For testing purpose, this file should be compiled with a C compiler, not C++
#endif /*__cplusplus */

#include "tbb/scalable_allocator.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h> /* for atexit */

/*
 *  The test is to check if the scalable_allocator.h and its functions
 *  can be used from pure C programs; also some regression checks are done
 */

#if __linux__
/* huge pages supported only under Linux so far */
const int ExpectedResultHugePages = TBBMALLOC_OK;
#else
const int ExpectedResultHugePages = TBBMALLOC_NO_EFFECT;
#endif

#if __TBB_SOURCE_DIRECTLY_INCLUDED
#include "../tbbmalloc/tbbmalloc_internal_api.h"
#else
#define __TBB_mallocProcessShutdownNotification()
#endif

/* test that it's possible to call allocation function from atexit
   after mallocProcessShutdownNotification() called */
static void MyExit(void) {
    void *p = scalable_malloc(32);
    assert(p);
    scalable_free(p);
    __TBB_mallocProcessShutdownNotification();
}

int main(void) {
    size_t i, j;
    int curr_mode, res;
    void *p1, *p2;

    atexit( MyExit );
    for ( curr_mode = 0; curr_mode<=1; curr_mode++) {
        assert(ExpectedResultHugePages ==
               scalable_allocation_mode(TBBMALLOC_USE_HUGE_PAGES, !curr_mode));
        p1 = scalable_malloc(10*1024*1024);
        assert(p1);
        assert(ExpectedResultHugePages ==
               scalable_allocation_mode(TBBMALLOC_USE_HUGE_PAGES, curr_mode));
        scalable_free(p1);
    }
    /* note that huge pages (if supported) are still enabled at this point */
#if __TBB_SOURCE_DIRECTLY_INCLUDED
    assert(TBBMALLOC_OK ==
           scalable_allocation_mode(TBBMALLOC_INTERNAL_SOURCE_INCLUDED, 0));
#endif

    for( i=0; i<=1<<16; ++i) {
        p1 = scalable_malloc(i);
        if( !p1 )
            printf("Warning: there should be memory but scalable_malloc returned NULL\n");
        scalable_free(p1);
    }
    p1 = p2 = NULL;
    for( i=1024*1024; ; i/=2 )
    {
        scalable_free(p1);
        p1 = scalable_realloc(p2, i);
        p2 = scalable_calloc(i, 32);
        if (p2) {
            if (i<sizeof(size_t)) {
                for (j=0; j<i; j++)
                    assert(0==*((char*)p2+j));
            } else {
                for (j=0; j<i; j+=sizeof(size_t))
                    assert(0==*((size_t*)p2+j));
            }
        }
        scalable_free(p2);
        p2 = scalable_malloc(i);
        if (i==0) break;
    }
    for( i=1; i<1024*1024; i*=2 )
    {
        scalable_free(p1);
        p1 = scalable_realloc(p2, i);
        p2 = scalable_malloc(i);
    }
    scalable_free(p1);
    scalable_free(p2);
    res = scalable_allocation_command(TBBMALLOC_CLEAN_ALL_BUFFERS, NULL);
    assert(res == TBBMALLOC_OK);
    res = scalable_allocation_command(TBBMALLOC_CLEAN_THREAD_BUFFERS, NULL);
    /* expect all caches cleaned before, so got nothing from CLEAN_THREAD_BUFFERS */
    assert(res == TBBMALLOC_NO_EFFECT);
    /* check that invalid param argument give expected result*/
    res = scalable_allocation_command(TBBMALLOC_CLEAN_THREAD_BUFFERS,
                                      (void*)(intptr_t)1);
    assert(res == TBBMALLOC_INVALID_PARAM);
    __TBB_mallocProcessShutdownNotification();
    printf("done\n");
    return 0;
}
