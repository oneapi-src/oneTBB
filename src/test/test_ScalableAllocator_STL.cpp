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

// Test whether scalable_allocator works with some of the host's STL containers.

#define HARNESS_NO_PARSE_COMMAND_LINE 1
#define __TBB_EXTRA_DEBUG 1 // enables additional checks
#define TBB_PREVIEW_MEMORY_POOL 1

#include "harness_assert.h"
#include "tbb/memory_pool.h"
#include "tbb/scalable_allocator.h"

// The actual body of the test is there:
#include "test_allocator_STL.h"

int TestMain () {
    TestAllocatorWithSTL<tbb::scalable_allocator<void> >();
    tbb::memory_pool<tbb::scalable_allocator<int> > mpool;
    TestAllocatorWithSTL(tbb::memory_pool_allocator<void>(mpool) );
    static char buf[1024*1024*4];
    tbb::fixed_pool fpool(buf, sizeof(buf));
    TestAllocatorWithSTL(tbb::memory_pool_allocator<void>(fpool) );
    return Harness::Done;
}
