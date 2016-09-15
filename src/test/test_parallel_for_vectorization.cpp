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

// The test checks if the vectorization happens when PPL-style parallel_for is
// used. The test implements two ideas:
// 1. "pragma always assert" issues a compiler-time error if the vectorization
// cannot be produced;
// 2. "#pragma ivdep" has a peculiarity which also can be used for detection of
// successful vectorization. See the comment below.

// For now, only Intel(R) C++ Compiler 12.0 and later is supported. Also, no
// sense to run the test in debug mode.
#define HARNESS_SKIP_TEST ( __INTEL_COMPILER < 1200  || TBB_USE_DEBUG )

// __TBB_ASSERT_ON_VECTORIZATION_FAILURE enables "pragma always assert" for
// Intel(R) C++ Compiler.
#define __TBB_ASSERT_ON_VECTORIZATION_FAILURE ( !HARNESS_SKIP_TEST )
#include "tbb/parallel_for.h"
#include "tbb/task_scheduler_init.h"

#include "harness.h"
#include "harness_assert.h"

class Body : NoAssign {
    int &sum;
public:
    Body( int& s ) : sum(s) {}
    void operator() ( int i ) const {
        sum += i / i;
    }
};

int TestMain () {
    // Should be big enough that the partitioner generated at least a one range
    // with a size greater than 1. See the comment below.
    const int N = 10000;
    int sum = 1;
    tbb::task_scheduler_init init(1);
    tbb::parallel_for( 1, N, Body(sum) );

    // The ppl-style parallel_for implementation has pragma ivdep before the
    // range loop. This pragma suppresses the dependency about "sum" in "Body".
    // Thus the vectorizer should generate code which just add to "sum" only
    // one iteration of the range (despite the real number of iterations in the
    // range). So "sum" is just number of calls of "Body". And it should be
    // less than N if at least one range was greater than 1.
    ASSERT( sum < N, "The loop was not vectorized." );

    return  Harness::Done;
}
