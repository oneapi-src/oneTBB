/*
    Copyright (c) 2005-2016 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.




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
