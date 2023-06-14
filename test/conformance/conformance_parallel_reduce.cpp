/*
    Copyright (c) 2005-2023 Intel Corporation

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

#include "common/parallel_reduce_common.h"
#include "common/concurrency_tracker.h"
#include "common/test_invoke.h"

#include "../tbb/test_partitioner.h"

//! \file conformance_parallel_reduce.cpp
//! \brief Test for [algorithms.parallel_reduce algorithms.parallel_deterministic_reduce] specification

class RotOp {
public:
    using Type = int;
    int operator() ( int x, int i ) const {
        return ( x<<1 ) ^ i;
    }
    int join( int x, int y ) const {
        return operator()( x, y );
    }
};

template <class Op>
struct ReduceBody {
    using result_type = typename Op::Type;
    result_type my_value;

    ReduceBody() : my_value() {}
    ReduceBody( ReduceBody &, oneapi::tbb::split ) : my_value() {}

    void operator() ( const oneapi::tbb::blocked_range<int>& r ) {
        utils::ConcurrencyTracker ct;
        for ( int i = r.begin(); i != r.end(); ++i ) {
            Op op;
            my_value = op(my_value, i);
        }
    }

    void join( const ReduceBody& y ) {
        Op op;
        my_value = op.join(my_value, y.my_value);
    }
};

template <class Partitioner>
void TestDeterministicReductionFor() {
    const int N = 1000;
    const oneapi::tbb::blocked_range<int> range(0, N);
    using BodyType = ReduceBody<RotOp>;
    using Type = RotOp::Type;

    BodyType benchmark_body;
    deterministic_reduce_invoker(range, benchmark_body, Partitioner());
    for ( int i=0; i<100; ++i ) {
        BodyType measurement_body;
        deterministic_reduce_invoker(range, measurement_body, Partitioner());
        REQUIRE_MESSAGE( benchmark_body.my_value == measurement_body.my_value,
        "parallel_deterministic_reduce behaves differently from run to run" );

        Type lambda_measurement_result = deterministic_reduce_invoker<Type>( range,
            [](const oneapi::tbb::blocked_range<int>& br, Type value) -> Type {
                utils::ConcurrencyTracker ct;
                for ( int ii = br.begin(); ii != br.end(); ++ii ) {
                    RotOp op;
                    value = op(value, ii);
                }
                return value;
            },
            [](const Type& v1, const Type& v2) -> Type {
                RotOp op;
                return op.join(v1,v2);
            },
            Partitioner()
        );
        REQUIRE_MESSAGE( benchmark_body.my_value == lambda_measurement_result,
            "lambda-based parallel_deterministic_reduce behaves differently from run to run" );
    }
}

//! Test that deterministic reduction returns the same result during several measurements
//! \brief \ref requirement \ref interface
TEST_CASE("Test deterministic reduce correctness") {
    for ( auto concurrency_level : utils::concurrency_range() ) {
        oneapi::tbb::global_control control(oneapi::tbb::global_control::max_allowed_parallelism, concurrency_level);
        TestDeterministicReductionFor<oneapi::tbb::simple_partitioner>();
        TestDeterministicReductionFor<oneapi::tbb::static_partitioner>();
        TestDeterministicReductionFor<utils_default_partitioner>();
    }
}

//! Test partitioners interaction with various ranges
//! \brief \ref requirement \ref interface
TEST_CASE("Test partitioners interaction with various ranges") {
    using namespace test_partitioner_utils::interaction_with_range_and_partitioner;
    for ( auto concurrency_level : utils::concurrency_range() ) {
        oneapi::tbb::global_control control(oneapi::tbb::global_control::max_allowed_parallelism, concurrency_level);

        test_partitioner_utils::SimpleReduceBody body;
        oneapi::tbb::affinity_partitioner ap;

        parallel_reduce(Range1(/*assert_in_split*/ true, /*assert_in_proportional_split*/ false), body, ap);
        parallel_reduce(Range6(false, true), body, ap);

        parallel_reduce(Range1(/*assert_in_split*/ true, /*assert_in_proportional_split*/ false), body, oneapi::tbb::static_partitioner());
        parallel_reduce(Range6(false, true), body, oneapi::tbb::static_partitioner());

        parallel_reduce(Range1(/*assert_in_split*/ false, /*assert_in_proportional_split*/ true), body, oneapi::tbb::simple_partitioner());
        parallel_reduce(Range6(false, true), body, oneapi::tbb::simple_partitioner());

        parallel_reduce(Range1(/*assert_in_split*/ false, /*assert_in_proportional_split*/ true), body, oneapi::tbb::auto_partitioner());
        parallel_reduce(Range6(false, true), body, oneapi::tbb::auto_partitioner());

        parallel_deterministic_reduce(Range1(/*assert_in_split*/true, /*assert_in_proportional_split*/ false), body, oneapi::tbb::static_partitioner());
        parallel_deterministic_reduce(Range6(false, true), body, oneapi::tbb::static_partitioner());

        parallel_deterministic_reduce(Range1(/*assert_in_split*/false, /*assert_in_proportional_split*/ true), body, oneapi::tbb::simple_partitioner());
        parallel_deterministic_reduce(Range6(false, true), body, oneapi::tbb::simple_partitioner());
    }
}

#if __TBB_CPP17_INVOKE_PRESENT

template <typename Body, typename Reduction>
void test_preduce_invoke_basic(const Body& body, const Reduction& reduction) {
    const std::size_t iterations = 100000;
    const std::size_t result = iterations * (iterations - 1) / 2;

    test_invoke::SmartRange<test_invoke::SmartValue> range(0, iterations);
    test_invoke::SmartValue identity(0);

    CHECK(result == oneapi::tbb::parallel_reduce(range, identity, body, reduction).get());
    CHECK(result == oneapi::tbb::parallel_reduce(range, identity, body, reduction, oneapi::tbb::simple_partitioner()).get());
    CHECK(result == oneapi::tbb::parallel_reduce(range, identity, body, reduction, oneapi::tbb::auto_partitioner()).get());
    CHECK(result == oneapi::tbb::parallel_reduce(range, identity, body, reduction, oneapi::tbb::static_partitioner()).get());
    oneapi::tbb::affinity_partitioner aff;
    CHECK(result == oneapi::tbb::parallel_reduce(range, identity, body, reduction, aff).get());

    CHECK(result == oneapi::tbb::parallel_deterministic_reduce(range, identity, body, reduction).get());
    CHECK(result == oneapi::tbb::parallel_deterministic_reduce(range, identity, body, reduction, oneapi::tbb::simple_partitioner()).get());
    CHECK(result == oneapi::tbb::parallel_deterministic_reduce(range, identity, body, reduction, oneapi::tbb::static_partitioner()).get());
}

//! Test that parallel_reduce uses std::invoke to run the body
//! \brief \ref interface \ref requirement
TEST_CASE("parallel_[deterministic_]reduce and std::invoke") {
    auto regular_reduce = [](const test_invoke::SmartRange<test_invoke::SmartValue>& range, const test_invoke::SmartValue& idx) {
        test_invoke::SmartValue result = idx;
        for (auto i = range.begin(); i.get() != range.end().get(); ++i) {
            result = result + i;
        }
        return result;
    };
    auto regular_join = [](const test_invoke::SmartValue& lhs, const test_invoke::SmartValue& rhs) {
        return lhs + rhs;
    };

    test_preduce_invoke_basic(&test_invoke::SmartRange<test_invoke::SmartValue>::reduction, &test_invoke::SmartValue::operator+);
    test_preduce_invoke_basic(&test_invoke::SmartRange<test_invoke::SmartValue>::reduction, regular_join);
    test_preduce_invoke_basic(regular_reduce, &test_invoke::SmartValue::operator+);
}

#endif
