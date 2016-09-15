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

// Test whether cache_aligned_allocator works with some of the host's STL containers.

#include "tbb/cache_aligned_allocator.h"
#include "tbb/tbb_allocator.h"

#define HARNESS_NO_PARSE_COMMAND_LINE 1
// the real body of the test is there:
#include "test_allocator.h"

template<>
struct is_zero_filling<tbb::zero_allocator<void> > {
    static const bool value = true;
};

// Test that NFS_Allocate() throws bad_alloc if cannot allocate memory.
void Test_NFS_Allocate_Throws() {
#if TBB_USE_EXCEPTIONS && !__TBB_THROW_ACROSS_MODULE_BOUNDARY_BROKEN
    using namespace tbb::internal;

    // First, allocate a reasonably big amount of memory, big enough
    // to not cause warp around in system allocator after adding object header
    // during address2 allocation.
    const size_t itemsize = 1024;
    const size_t nitems   = 1024;
    void *address1 = NULL;
    try {
        address1 = NFS_Allocate( nitems, itemsize, NULL );
    } catch( ... ) {
        // intentionally empty
    }
    ASSERT( address1, "NFS_Allocate unable to obtain 1024*1024 bytes" );

    bool exception_caught = false;
    try {
        // Try allocating more memory than left in the address space; should cause std::bad_alloc
        (void) NFS_Allocate( 1, ~size_t(0) - itemsize*nitems + NFS_GetLineSize(), NULL);
    } catch( std::bad_alloc ) {
        exception_caught = true;
    } catch( ... ) {
        ASSERT( __TBB_EXCEPTION_TYPE_INFO_BROKEN, "Unexpected exception type (std::bad_alloc was expected)" );
        exception_caught = true;
    }
    ASSERT( exception_caught, "NFS_Allocate did not throw bad_alloc" );

    try {
        NFS_Free( address1 );
    } catch( ... ) {
        ASSERT( false, "NFS_Free did not accept the address obtained with NFS_Allocate" );
    }
#endif /* TBB_USE_EXCEPTIONS && !__TBB_THROW_ACROSS_MODULE_BOUNDARY_BROKEN */
}

int TestMain () {
    int result = TestMain<tbb::cache_aligned_allocator<void> >();
    result += TestMain<tbb::tbb_allocator<void> >();
    result += TestMain<tbb::zero_allocator<void> >();
    ASSERT( !result, NULL );
    Test_NFS_Allocate_Throws();
    return Harness::Done;
}
