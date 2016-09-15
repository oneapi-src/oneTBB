/*
    Copyright 2005-2016 Intel Corporation.  All Rights Reserved.

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

// Test whether scalable_allocator complies with the requirements in 20.1.5 of ISO C++ Standard (1998).

#define __TBB_EXTRA_DEBUG 1 // enables additional checks
#define TBB_PREVIEW_MEMORY_POOL 1

#include "harness_assert.h"
#if !__TBB_SOURCE_DIRECTLY_INCLUDED
// harness_allocator.h requires atimics. We do not want dependency
// to TBB library to get atomics, so add rudimentary implementation of them.
#include "harness_tbb_independence.h"
#endif
#include "tbb/memory_pool.h"
#include "tbb/scalable_allocator.h"

#define HARNESS_TBBMALLOC_THREAD_SHUTDOWN 1
// the actual body of the test is there:
#include "test_allocator.h"
#include "harness_allocator.h"

#if _MSC_VER
#include "tbb/machine/windows_api.h"
#endif /* _MSC_VER */

typedef static_counting_allocator<tbb::memory_pool_allocator<char> > cnt_alloc_t;
typedef local_counting_allocator<std::allocator<char> > cnt_provider_t;
class MinimalAllocator : cnt_provider_t {
public:
    typedef char value_type;
    MinimalAllocator() {
        REMARK("%p::ctor\n", this);
    }
    MinimalAllocator(const MinimalAllocator&s) : cnt_provider_t(s) {
        REMARK("%p::ctor(%p)\n", this, &s);
    }
    ~MinimalAllocator() {
        REMARK("%p::dtor: alloc=%u/%u free=%u/%u\n", this,
            unsigned(items_allocated),unsigned(allocations),
            unsigned(items_freed), unsigned(frees) );
        ASSERT(allocations==frees && items_allocated==items_freed,0);
        if( allocations ) { // non-temporal copy
            // TODO: describe consumption requirements
            ASSERT(items_allocated>cnt_alloc_t::items_allocated, 0);
        }
    }
    void *allocate(size_t sz) {
        void *p = cnt_provider_t::allocate(sz);
        REMARK("%p::allocate(%u) = %p\n", this, unsigned(sz), p);
        return p;
    }
    void deallocate(void *p, size_t sz) {
        ASSERT(allocations>frees,0);
        REMARK("%p::deallocate(%p, %u)\n", this, p, unsigned(sz));
        cnt_provider_t::deallocate(cnt_provider_t::pointer(p), sz);
    }
};

class NullAllocator {
public:
    typedef char value_type;
    NullAllocator() { }
    NullAllocator(const NullAllocator&) { }
    ~NullAllocator() { }
    void *allocate(size_t) { return NULL; }
    void deallocate(void *, size_t) { ASSERT(0, NULL); }
};

void TestZeroSpaceMemoryPool()
{
    tbb::memory_pool<NullAllocator> pool;
    bool allocated = pool.malloc(16) || pool.malloc(9*1024);
    ASSERT(!allocated, "Allocator with no memory must not allocate anything.");
}

#if !TBB_USE_EXCEPTIONS
struct FixedPool {
    void  *buf;
    size_t size;
    bool   used;
    FixedPool(void *buf, size_t size) : buf(buf), size(size), used(false) {}
};

static void *fixedBufGetMem(intptr_t pool_id, size_t &bytes)
{
    if (((FixedPool*)pool_id)->used)
        return NULL;

    ((FixedPool*)pool_id)->used = true;
    bytes = ((FixedPool*)pool_id)->size;
    return ((FixedPool*)pool_id)->buf;
}
#endif

/* test that pools in small space are either usable or not created
   (i.e., exception raised) */
void TestSmallFixedSizePool()
{
    char *buf;
    bool allocated = false;

    for (size_t sz = 0; sz < 64*1024; sz = sz? 3*sz : 3) {
        buf = (char*)malloc(sz);
#if TBB_USE_EXCEPTIONS
        try {
            tbb::fixed_pool pool(buf, sz);
/* Check that pool is usable, i.e. such an allocation exists,
   that can be fulfilled from the pool. 16B allocation fits in 16KB slabs,
   so it requires at least 16KB. Requirement of 9KB allocation is more modest.
*/
            allocated = pool.malloc( 16 ) || pool.malloc( 9*1024 );
        } catch (std::invalid_argument) {
            ASSERT(!sz, "expect std::invalid_argument for zero-sized pool only");
        } catch (...) {
            ASSERT(0, "wrong exception type;");
        }
#else
/* Do not test high-level pool interface because pool ctor emit exception
   on creation failure. Instead test same functionality via low-level interface.
   TODO: add support for configuration with disabled exceptions to pools.
*/
        rml::MemPoolPolicy pol(fixedBufGetMem, NULL, 0, /*fixedSizePool=*/true,
                               /*keepMemTillDestroy=*/false);
        rml::MemoryPool *pool;
        FixedPool fixedPool(buf, sz);

        rml::MemPoolError ret = pool_create_v1((intptr_t)&fixedPool, &pol, &pool);

        if (ret == rml::POOL_OK) {
            allocated = pool_malloc(pool, 16) || pool_malloc(pool, 9*1024);
            pool_destroy(pool);
        } else
            ASSERT(ret == rml::NO_MEMORY, "Expected that pool either valid "
                                     "or have no memory to be created");
#endif
        free(buf);
    }
    ASSERT(allocated, "Maximal buf size should be enough to create working fixed_pool");
#if TBB_USE_EXCEPTIONS
    try {
        tbb::fixed_pool pool(NULL, 10*1024*1024);
        ASSERT(0, "Useless allocator with no memory must not be created");
    } catch (std::invalid_argument) {
    } catch (...) {
        ASSERT(0, "wrong exception type; expected invalid_argument");
    }
#endif
}

int TestMain () {
#if _MSC_VER && !__TBBMALLOC_NO_IMPLICIT_LINKAGE && !__TBB_WIN8UI_SUPPORT
    #ifdef _DEBUG
        ASSERT(!GetModuleHandle("tbbmalloc.dll") && GetModuleHandle("tbbmalloc_debug.dll"),
            "test linked with wrong (non-debug) tbbmalloc library");
    #else
        ASSERT(!GetModuleHandle("tbbmalloc_debug.dll") && GetModuleHandle("tbbmalloc.dll"),
            "test linked with wrong (debug) tbbmalloc library");
    #endif
#endif /* _MSC_VER && !__TBBMALLOC_NO_IMPLICIT_LINKAGE */
    int result = TestMain<tbb::scalable_allocator<void> >();
    {
        tbb::memory_pool<tbb::scalable_allocator<int> > pool;
        result += TestMain(tbb::memory_pool_allocator<void>(pool) );
    }{
        tbb::memory_pool<MinimalAllocator> pool;
        cnt_alloc_t alloc(( tbb::memory_pool_allocator<char>(pool) )); // double parentheses to avoid function declaration
        result += TestMain(alloc);
    }{
        static char buf[1024*1024*4];
        tbb::fixed_pool pool(buf, sizeof(buf));
        const char *text = "this is a test";// 15 bytes
        char *p1 = (char*)pool.malloc( 16 );
        ASSERT(p1, NULL);
        strcpy(p1, text);
        char *p2 = (char*)pool.realloc( p1, 15 );
        ASSERT( p2 && !strcmp(p2, text), "realloc broke memory" );

        result += TestMain(tbb::memory_pool_allocator<void>(pool) );

        // try allocate almost entire buf keeping some reasonable space for internals
        char *p3 = (char*)pool.realloc( p2, sizeof(buf)-128*1024 );
        ASSERT( p3, "defragmentation failed" );
        ASSERT( !strcmp(p3, text), "realloc broke memory" );
        for( size_t sz = 10; sz < sizeof(buf); sz *= 2) {
            ASSERT( pool.malloc( sz ), NULL);
            pool.recycle();
        }

        result += TestMain(tbb::memory_pool_allocator<void>(pool) );
    }
    TestSmallFixedSizePool();
    TestZeroSpaceMemoryPool();

    ASSERT( !result, NULL );
    return Harness::Done;
}
