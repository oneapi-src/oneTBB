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

// Program for basic correctness testing of assembly-language routines.
#include "harness_defs.h"
//for ICC builtins mode the test will be skipped as
//macro __TBB_GCC_BUILTIN_ATOMICS_PRESENT used to define __TBB_TEST_SKIP_GCC_BUILTINS_MODE
//will not be defined (it is explicitly disabled for ICC)
#if __TBB_TEST_SKIP_GCC_BUILTINS_MODE
#include "harness.h"
int TestMain() {
    REPORT("Known issue: GCC builtins aren't available\n");
    return Harness::Skipped;
}
#else

#include "tbb/task.h"

#include <new>
#include "harness.h"

using tbb::internal::reference_count;

//TODO: remove this function when atomic function __TBB_XXX are dropped
//! Test __TBB_CompareAndSwapW
static void TestCompareExchange() {
    ASSERT( intptr_t(-10)<10, "intptr_t not a signed integral type?" );
    REMARK("testing __TBB_CompareAndSwapW\n");
    for( intptr_t a=-10; a<10; ++a )
        for( intptr_t b=-10; b<10; ++b )
            for( intptr_t c=-10; c<10; ++c ) {
// Workaround for a bug in GCC 4.3.0; and one more is below.
#if __TBB_GCC_OPTIMIZER_ORDERING_BROKEN
                intptr_t x;
                __TBB_store_with_release( x, a );
#else
                intptr_t x = a;
#endif
                intptr_t y = __TBB_CompareAndSwapW(&x,b,c);
                ASSERT( y==a, NULL );
                if( a==c )
                    ASSERT( x==b, NULL );
                else
                    ASSERT( x==a, NULL );
            }
}

//TODO: remove this function when atomic function __TBB_XXX are dropped
//! Test __TBB___TBB_FetchAndIncrement and __TBB___TBB_FetchAndDecrement
static void TestAtomicCounter() {
    // "canary" is a value used to detect illegal overwrites.
    const reference_count canary = ~(uintptr_t)0/3;
    REMARK("testing __TBB_FetchAndIncrement\n");
    struct {
        reference_count prefix, i, suffix;
    } x;
    x.prefix = canary;
    x.i = 0;
    x.suffix = canary;
    for( int k=0; k<10; ++k ) {
        reference_count j = __TBB_FetchAndIncrementWacquire((volatile void *)&x.i);
        ASSERT( x.prefix==canary, NULL );
        ASSERT( x.suffix==canary, NULL );
        ASSERT( x.i==k+1, NULL );
        ASSERT( j==k, NULL );
    }
    REMARK("testing __TBB_FetchAndDecrement\n");
    x.i = 10;
    for( int k=10; k>0; --k ) {
        reference_count j = __TBB_FetchAndDecrementWrelease((volatile void *)&x.i);
        ASSERT( j==k, NULL );
        ASSERT( x.i==k-1, NULL );
        ASSERT( x.prefix==canary, NULL );
        ASSERT( x.suffix==canary, NULL );
    }
}

static void TestTinyLock() {
    REMARK("testing __TBB_LockByte\n");
    __TBB_atomic_flag flags[16];
    for( unsigned int i=0; i<16; ++i )
        flags[i] = (__TBB_Flag)i;
#if __TBB_GCC_OPTIMIZER_ORDERING_BROKEN
    __TBB_store_with_release( flags[8], 0 );
#else
    flags[8] = 0;
#endif
    __TBB_LockByte(flags[8]);
    for( unsigned int i=0; i<16; ++i )
        #ifdef __sparc
        ASSERT( flags[i]==(i==8?0xff:i), NULL );
        #else
        ASSERT( flags[i]==(i==8?1:i), NULL );
        #endif
    __TBB_UnlockByte(flags[8]);
    for( unsigned int i=0; i<16; ++i )
        ASSERT( flags[i] == (i==8?0:i), NULL );
}

static void TestLog2() {
    REMARK("testing __TBB_Log2\n");
    for( uintptr_t i=1; i; i<<=1 ) {
        for( uintptr_t j=1; j<1<<16; ++j ) {
            if( uintptr_t k = i*j ) {
                uintptr_t actual = __TBB_Log2(k);
                const uintptr_t ONE = 1; // warning suppression again
                ASSERT( k >= ONE<<actual, NULL );
                ASSERT( k>>1 < ONE<<actual, NULL );
            }
        }
    }
}

static void TestPause() {
    REMARK("testing __TBB_Pause\n");
    __TBB_Pause(1);
}

static void TestTimeStamp() {
    REMARK("testing __TBB_time_stamp");
#if defined(__TBB_time_stamp)
    tbb::internal::machine_tsc_t prev = __TBB_time_stamp();
    for ( int i=0; i<1000; ++i ) {
        tbb::internal::machine_tsc_t curr = __TBB_time_stamp();
        ASSERT(curr>prev, "__TBB_time_stamp has returned non-monotonically increasing quantity");
        prev=curr;
    }
    REMARK("\n");
#else
    REMARK(" skipped\n");
#endif
}

int TestMain () {
    __TBB_TRY {
        TestLog2();
        TestTinyLock();
        TestCompareExchange();
        TestAtomicCounter();
        TestPause();
        TestTimeStamp();
    } __TBB_CATCH(...) {
        ASSERT(0,"unexpected exception");
    }
    return Harness::Done;
}
#endif // __TBB_TEST_SKIP_BUILTINS_MODE
