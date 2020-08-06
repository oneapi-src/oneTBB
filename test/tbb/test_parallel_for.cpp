/*
    Copyright (c) 2005-2020 Intel Corporation

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

#include "common/test.h"
#include "common/config.h"
#include "common/utils.h"
#include "common/utils_concurrency_limit.h"
#include "common/utils_report.h"
#include "common/vector_types.h"
#include "common/cpu_usertime.h"
#include "common/spin_barrier.h"

#include "tbb/tick_count.h"
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
#include "tbb/test_partitioner.h"

#include <cstdio>
#include <vector>
#include <sstream>

//! \file test_parallel_for.cpp
//! \brief Test for [algorithms.parallel_for] specification

#if _MSC_VER
#pragma warning (push)
#if __TBB_MSVC_UNREACHABLE_CODE_IGNORED
    // Suppress pointless "unreachable code" warning.
    #pragma warning (disable: 4702)
#endif
#if defined(_Wp64)
    // Workaround for overzealous compiler warnings in /Wp64 mode
    #pragma warning (disable: 4267)
#endif
#define _SCL_SECURE_NO_WARNINGS
#endif //#if _MSC_VER


#if (HAVE_m128 || HAVE_m256)
template<typename ClassWithVectorType>
struct SSE_Functor {
    ClassWithVectorType* Src, * Dst;
    SSE_Functor( ClassWithVectorType* src, ClassWithVectorType* dst ) : Src(src), Dst(dst) {}

    void operator()( tbb::blocked_range<int>& r ) const {
        for( int i=r.begin(); i!=r.end(); ++i )
            Dst[i] = Src[i];
    }
};

//! Test that parallel_for works with stack-allocated __m128
template<typename ClassWithVectorType>
void TestVectorTypes() {
    const int aSize = 300;
    ClassWithVectorType Array1[aSize], Array2[aSize];
    for( int i=0; i<aSize; ++i ) {
        // VC8 does not properly align a temporary value; to work around, use explicit variable
        ClassWithVectorType foo(i);
        Array1[i] = foo;
    }
    tbb::parallel_for( tbb::blocked_range<int>(0,aSize), SSE_Functor<ClassWithVectorType>(Array1, Array2) );
    for( int i=0; i<aSize; ++i ) {
        ClassWithVectorType foo(i);
        CHECK( Array2[i]==foo ) ;
    }
}
#endif /* HAVE_m128 || HAVE_m256 */

struct TestSimplePartitionerStabilityFunctor {
  std::vector<int> & ranges;
  TestSimplePartitionerStabilityFunctor(std::vector<int> & theRanges):ranges(theRanges){}
  void operator()(tbb::blocked_range<size_t>& r)const{
      ranges.at(r.begin()) = 1;
  }
};
void TestSimplePartitionerStability(){
    const std::size_t repeat_count= 10;
    const std::size_t rangeToSplitSize=1000000;
    const std::size_t grainsizeStep=rangeToSplitSize/repeat_count;
    typedef TestSimplePartitionerStabilityFunctor FunctorType;

    for (std::size_t i=0 , grainsize=grainsizeStep; i<repeat_count;i++, grainsize+=grainsizeStep){
        std::vector<int> firstSeries(rangeToSplitSize,0);
        std::vector<int> secondSeries(rangeToSplitSize,0);

        tbb::parallel_for(tbb::blocked_range<size_t>(0,rangeToSplitSize,grainsize),FunctorType(firstSeries),tbb::simple_partitioner());
        tbb::parallel_for(tbb::blocked_range<size_t>(0,rangeToSplitSize,grainsize),FunctorType(secondSeries),tbb::simple_partitioner());
        std::stringstream str; str<<i;
        CHECK_MESSAGE(firstSeries==secondSeries, ("splitting range with tbb::simple_partitioner must be reproducible; i=" +str.str()).c_str() );
    }
}

namespace various_range_implementations {

using namespace test_partitioner_utils;
using namespace test_partitioner_utils::TestRanges;

// Body ensures that initial work distribution is done uniformly through affinity mechanism and not through work stealing
class Body {
    utils::SpinBarrier &m_sb;
public:
    Body(utils::SpinBarrier& sb) : m_sb(sb) { }
    Body(Body& b, tbb::split) : m_sb(b.m_sb) { }

    template <typename Range>
    void operator()(Range& r) const {
        INFO("Executing range [" << r.begin() << ", " << r.end() << "]");
        m_sb.timedWait(10); // waiting for all threads
    }
};

namespace correctness {

/* Testing only correctness (that is parallel_for does not hang) */
template <typename RangeType, bool /* feedback */, bool ensure_non_emptiness>
void test() {
    RangeType range( 0, utils::get_platform_max_threads(), NULL, false, ensure_non_emptiness );
    tbb::affinity_partitioner ap;
    tbb::parallel_for( range, SimpleBody(), ap );
}

} // namespace correctness

namespace uniform_distribution {

/* Body of parallel_for algorithm would hang if non-uniform work distribution happened  */
template <typename RangeType, bool feedback, bool ensure_non_emptiness>
void test() {
    static const std::size_t thread_num = utils::get_platform_max_threads();
    utils::SpinBarrier sb( thread_num );
    RangeType range(0, thread_num, NULL, feedback, ensure_non_emptiness);
    const Body sync_body( sb );
    tbb::affinity_partitioner ap;
    tbb::parallel_for( range, sync_body, ap );
    tbb::parallel_for( range, sync_body, tbb::static_partitioner() );
}

} // namespace uniform_distribution

void test() {
    const bool provide_feedback = false;
    const bool ensure_non_empty_range = true;

    // BlockedRange does not take into account feedback and non-emptiness settings but uses the
    // tbb::blocked_range implementation
    uniform_distribution::test<BlockedRange, !provide_feedback, !ensure_non_empty_range>();
    using correctness::test;

    {
        test<RoundedDownRange, provide_feedback, ensure_non_empty_range>();
        test<RoundedDownRange, provide_feedback, !ensure_non_empty_range>();
    }

    {
        test<RoundedUpRange, provide_feedback, ensure_non_empty_range>();
        test<RoundedUpRange, provide_feedback, !ensure_non_empty_range>();
    }

    // Testing that parallel_for algorithm works with such weird ranges
    correctness::test<Range1_2, /* provide_feedback= */ false, !ensure_non_empty_range>();
    correctness::test<Range1_999, /* provide_feedback= */ false, !ensure_non_empty_range>();
    correctness::test<Range999_1, /* provide_feedback= */ false, !ensure_non_empty_range>();

    // The following ranges do not comply with the proportion suggested by partitioner. Therefore
    // they have to provide the proportion in which they were actually split back to partitioner and
    // ensure theirs non-emptiness
    test<Range1_2, provide_feedback, ensure_non_empty_range>();
    test<Range1_999, provide_feedback, ensure_non_empty_range>();
    test<Range999_1, provide_feedback, ensure_non_empty_range>();
}

} // namespace various_range_implementations


#if TBB_USE_EXCEPTIONS && !__TBB_THROW_ACROSS_MODULE_BOUNDARY_BROKEN && TBB_REVAMP_TODO
//! Testing exceptions
//! \brief \ref requirement
TEST_CASE("Exceptions support") {
    for ( int p = MinThread; p <= MaxThread; ++p ) {
        if ( p > 0 ) {
            tbb::global_control control(tbb::global_control::max_allowed_parallelism, p);
            TestExceptionsSupport();
        }
    }
}
#endif /* TBB_USE_EXCEPTIONS && !__TBB_THROW_ACROSS_MODULE_BOUNDARY_BROKEN */

#if __TBB_TASK_GROUP_CONTEXT && TBB_REVAMP_TODO
//! Testing cancellation
//! \brief \ref requirement
TEST_CASE("Cancellation") {
    for ( int p = MinThread; p <= MaxThread; ++p ) {
        if ( p > 1 ) {
            tbb::global_control control(tbb::global_control::max_allowed_parallelism, p);
            TestCancellation();
        }
    }
}
#endif /* __TBB_TASK_GROUP_CONTEXT */

//! Testing cancellation
//! \brief \ref error_guessing
TEST_CASE("Vector types") {
#if HAVE_m128
    TestVectorTypes<ClassWithSSE>();
#endif
#if HAVE_m256
    if (have_AVX()) TestVectorTypes<ClassWithAVX>();
#endif
}

//! Testing workers going to sleep
//! \brief \ref resource_usage
TEST_CASE("That all workers sleep when no work") {
    const std::size_t N = 100000;
    std::atomic<int> counter{};

    tbb::parallel_for(std::size_t(0), N, [&](std::size_t) {
        for (volatile int i = 0; i < 1000; ++i) {
            ++counter;
        }
    }, tbb::simple_partitioner());
    TestCPUUserTime(utils::get_platform_max_threads());
}

//! Testing simple partitioner stability
//! \brief \ref error_guessing
TEST_CASE("Simple partitioner stability") {
    TestSimplePartitionerStability();
}

//! Testing various range implementations
//! \brief \ref requirement
TEST_CASE("Various range implementations") {
    various_range_implementations::test();
}

#if _MSC_VER
#pragma warning (pop)
#endif
