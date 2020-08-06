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

#include <atomic>

#include "common/parallel_reduce_common.h"
#include "common/cpu_usertime.h"

//! \file test_parallel_reduce.cpp
//! \brief Test for [algorithms.parallel_reduce algorithms.parallel_deterministic_reduce] specification

using ValueType = uint64_t;

struct Sum {
    template<typename T>
    T operator() ( const T& v1, const T& v2 ) const {
        return v1 + v2;
    }
};

struct Accumulator {
    ValueType operator() ( const tbb::blocked_range<ValueType*>& r, ValueType value ) const {
        for ( ValueType* pv = r.begin(); pv != r.end(); ++pv )
            value += *pv;
        return value;
    }
};

class ParallelSumTester {
public:
    ParallelSumTester( const ParallelSumTester& ) = default;
    void operator=( const ParallelSumTester& ) = delete;

    ParallelSumTester() : m_range(nullptr, nullptr) {
        m_array = new ValueType[unsigned(count)];
        for ( ValueType i = 0; i < count; ++i )
            m_array[i] = i + 1;
        m_range = tbb::blocked_range<ValueType*>( m_array, m_array + count );
    }
    ~ParallelSumTester() { delete[] m_array; }

    template<typename Partitioner>
    void CheckParallelReduce() {
        Partitioner partitioner;
        ValueType result1 = reduce_invoker<ValueType>( m_range, Accumulator(), Sum(), partitioner );
        REQUIRE_MESSAGE( result1 == expected, "Wrong parallel summation result" );
        ValueType result2 = reduce_invoker<ValueType>( m_range,
            [](const tbb::blocked_range<ValueType*>& r, ValueType value) -> ValueType {
                for ( const ValueType* pv = r.begin(); pv != r.end(); ++pv )
                    value += *pv;
                return value;
            },
            Sum(),
            partitioner
        );
        REQUIRE_MESSAGE( result2 == expected, "Wrong parallel summation result" );
    }
private:
    ValueType* m_array;
    tbb::blocked_range<ValueType*> m_range;
    static const ValueType count, expected;
};

const ValueType ParallelSumTester::count = 1000000;
const ValueType ParallelSumTester::expected = count * (count + 1) / 2;


//! Test parallel summation correctness
//! \brief \ref stress
TEST_CASE("Test parallel summation correctness") {
    ParallelSumTester pst;
    pst.CheckParallelReduce<utils_default_partitioner>();
    pst.CheckParallelReduce<tbb::simple_partitioner>();
    pst.CheckParallelReduce<tbb::auto_partitioner>();
    pst.CheckParallelReduce<tbb::affinity_partitioner>();
    pst.CheckParallelReduce<tbb::static_partitioner>();
}

static std::atomic<long> ForkCount;
static std::atomic<long> FooBodyCount;

//! Class with public interface that is exactly minimal requirements for Range concept
class MinimalRange {
    size_t begin, end;
    friend class FooBody;
    explicit MinimalRange( size_t i ) : begin(0), end(i) {}
    template <typename Partitioner_> friend void TestSplitting( std::size_t nthread );
public:
    MinimalRange( MinimalRange& r, tbb::split ) : end(r.end) {
        begin = r.end = (r.begin+r.end)/2;
    }
    bool is_divisible() const {return end-begin>=2;}
    bool empty() const {return begin==end;}
};

//! Class with public interface that is exactly minimal requirements for Body of a parallel_reduce
class FooBody {
private:
    FooBody( const FooBody& );          // Deny access
    void operator=( const FooBody& );   // Deny access
    template <typename Partitioner_> friend void TestSplitting( std::size_t nthread );
    //! Parent that created this body via split operation.  NULL if original body.
    FooBody* parent;
    //! Total number of index values processed by body and its children.
    size_t sum;
    //! Number of join operations done so far on this body and its children.
    long join_count;
    //! Range that has been processed so far by this body and its children.
    size_t begin, end;
    //! True if body has not yet been processed at least once by operator().
    bool is_new;
    //! 1 if body was created by split; 0 if original body.
    int forked;
    FooBody() {++FooBodyCount;}
public:
    ~FooBody() {
        forked = 0xDEADBEEF;
        sum=0xDEADBEEF;
        join_count=0xDEADBEEF;
        --FooBodyCount;
    }
    FooBody( FooBody& other, tbb::split ) {
        ++FooBodyCount;
        ++ForkCount;
        sum = 0;
        parent = &other;
        join_count = 0;
        is_new = true;
        forked = 1;
    }

    void init() {
        sum = 0;
        parent = nullptr;
        join_count = 0;
        is_new = true;
        forked = 0;
        begin = ~size_t(0);
        end = ~size_t(0);
    }

    void join( FooBody& s ) {
        REQUIRE( s.forked==1 );
        REQUIRE( this!=&s );
        REQUIRE( this==s.parent );
        REQUIRE( end==s.begin );
        end = s.end;
        sum += s.sum;
        join_count += s.join_count + 1;
        s.forked = 2;
    }
    void operator()( const MinimalRange& r ) {
        for( size_t k=r.begin; k<r.end; ++k )
            ++sum;
        if( is_new ) {
            is_new = false;
            begin = r.begin;
        } else
            REQUIRE( end==r.begin );
        end = r.end;
    }
};

template<typename Partitioner>
void TestSplitting( std::size_t nthread ) {
    ForkCount = 0;
    long join_count = 0;
    Partitioner partitioner;
    for( size_t i=0; i<=1000; ++i ) {
        FooBody f;
        f.init();
        REQUIRE_MESSAGE( FooBodyCount==1, "Wrong initial BodyCount value" );
        reduce_invoker(MinimalRange(i), f, partitioner);

        if (nthread == 1) REQUIRE_MESSAGE(ForkCount==0, "Body was split during 1 thread execution");

        join_count += f.join_count;
        REQUIRE_MESSAGE( FooBodyCount==1, "Some copies of FooBody was not removed after reduction");
        REQUIRE_MESSAGE( f.sum==i, "Incorrect reduction" );
        REQUIRE_MESSAGE( f.begin==(i==0 ? ~size_t(0) : 0), "Incorrect range borders" );
        REQUIRE_MESSAGE( f.end==(i==0 ? ~size_t(0) : i), "Incorrect range borders" );
    }
}

//! Test splitting range and body during reduction, test that all workers sleep when no work
//! \brief \ref resource_usage \ref error_guessing
TEST_CASE("Test splitting range and body during reduction, test that all workers sleep when no work") {
    for ( auto concurrency_level : utils::concurrency_range() ) {
        tbb::global_control control(tbb::global_control::max_allowed_parallelism, concurrency_level);

        TestSplitting<tbb::simple_partitioner>(concurrency_level);
        TestSplitting<tbb::static_partitioner>(concurrency_level);
        TestSplitting<tbb::auto_partitioner>(concurrency_level);
        TestSplitting<tbb::affinity_partitioner>(concurrency_level);
        TestSplitting<utils_default_partitioner>(concurrency_level);

        // Test that all workers sleep when no work
        TestCPUUserTime(concurrency_level);
    }
}

//! Define overloads of parallel_deterministic_reduce that accept "undesired" types of partitioners
namespace unsupported {
    template<typename Range, typename Body>
    void parallel_deterministic_reduce(const Range&, Body&, const tbb::auto_partitioner&) { }
    template<typename Range, typename Body>
    void parallel_deterministic_reduce(const Range&, Body&, tbb::affinity_partitioner&) { }

    template<typename Range, typename Value, typename RealBody, typename Reduction>
    Value parallel_deterministic_reduce(const Range& , const Value& identity, const RealBody& , const Reduction& , const tbb::auto_partitioner&) {
        return identity;
    }
    template<typename Range, typename Value, typename RealBody, typename Reduction>
    Value parallel_deterministic_reduce(const Range& , const Value& identity, const RealBody& , const Reduction& , tbb::affinity_partitioner&) {
        return identity;
    }
}

struct Body {
    float value;
    Body() : value(0) {}
    Body(Body&, tbb::split) { value = 0; }
    void operator()(const tbb::blocked_range<int>&) {}
    void join(Body&) {}
};

//! Check that other types of partitioners are not supported (auto, affinity)
//! In the case of "unsupported" API unexpectedly sneaking into namespace tbb,
//! this test should result in a compilation error due to overload resolution ambiguity
//! \brief \ref negative \ref error_guessing
TEST_CASE("Test Unsupported Partitioners") {
    using namespace tbb;
    using namespace unsupported;
    Body body;
    parallel_deterministic_reduce(blocked_range<int>(0, 10), body, tbb::auto_partitioner());

    tbb::affinity_partitioner ap;
    parallel_deterministic_reduce(blocked_range<int>(0, 10), body, ap);

    parallel_deterministic_reduce(
        blocked_range<int>(0, 10),
        0,
        [](const blocked_range<int>&, int init)->int {
            return init;
        },
        [](int x, int y)->int {
            return x + y;
        },
        tbb::auto_partitioner()
    );
    parallel_deterministic_reduce(
        blocked_range<int>(0, 10),
        0,
        [](const blocked_range<int>&, int init)->int {
            return init;
        },
        [](int x, int y)->int {
            return x + y;
        },
        ap
    );
}
