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

#include "common/parallel_for_each_common.h"
#include "common/type_requirements_test.h"

//! \file conformance_parallel_for_each.cpp
//! \brief Test for [algorithms.parallel_for_each] specification

//! Test input access iterator support
//! \brief \ref requirement \ref interface
TEST_CASE("Input iterator support") {
    for ( auto concurrency_level : utils::concurrency_range() ) {
        oneapi::tbb::global_control control(oneapi::tbb::global_control::max_allowed_parallelism, concurrency_level);

        for( size_t depth = 0; depth <= depths_nubmer; ++depth ) {
            g_tasks_expected = 0;
            for ( size_t i=0; i < depth; ++i )
                g_tasks_expected += FindNumOfTasks(g_depths[i].value());
            TestIterator_Const<utils::InputIterator<value_t>>(depth);
            TestIterator_Move<utils::InputIterator<value_t>>(depth);
#if __TBB_CPP14_GENERIC_LAMBDAS_PRESENT
            TestGenericLambdasCommon<utils::InputIterator<value_t>>(depth);
#endif
        }
    }
}

//! Test container based overload
//! \brief \ref requirement \ref interface
TEST_CASE("Container based overload - input iterator based container") {
    container_based_overload_test_case<utils::InputIterator, incremental_functor_const>(/*expected_value*/0);
}

const size_t elements = 10000;
const size_t init_sum = 0;
std::atomic<size_t> element_counter;

template<size_t K>
struct set_to {
    void operator()(size_t& x) const {
        x = K;
        ++element_counter;
    }
};

#include "common/range_based_for_support.h"
#include <functional>
#include <deque>

template<typename... Context>
void WorkProducingTest(Context&... context) {
    for ( auto concurrency_level : utils::concurrency_range() ) {
        oneapi::tbb::global_control control(oneapi::tbb::global_control::max_allowed_parallelism, concurrency_level);

        using namespace range_based_for_support_tests;
        std::deque<size_t> v(elements, 0);

        element_counter = 0;
        oneapi::tbb::parallel_for_each(v.begin(), v.end(), set_to<0>(), context...);
        REQUIRE_MESSAGE((element_counter == v.size() && element_counter == elements),
            "not all elements were set");
        REQUIRE_MESSAGE(range_based_for_accumulate(v, std::plus<size_t>(), init_sum) == init_sum,
            "elements of v not all ones");

        element_counter = 0;
        oneapi::tbb::parallel_for_each(v, set_to<1>(), context...);
        REQUIRE_MESSAGE((element_counter == v.size() && element_counter == elements),
            "not all elements were set");
        REQUIRE_MESSAGE(range_based_for_accumulate(v, std::plus<size_t>(), init_sum) == v.size(),
            "elements of v not all ones");

        element_counter = 0;
        oneapi::tbb::parallel_for_each(oneapi::tbb::blocked_range<std::deque<size_t>::iterator>(v.begin(), v.end()), set_to<0>(), context...);
        REQUIRE_MESSAGE((element_counter == v.size() && element_counter == elements),
            "not all elements were set");
        REQUIRE_MESSAGE(range_based_for_accumulate(v, std::plus<size_t>(), init_sum) == init_sum,
            "elements of v not all zeros");
    }
}

//! Test that all elements were produced
//! \brief \ref requirement \ref stress
TEST_CASE("Test that all elements in range were produced through body (without task_group_context)") {
    WorkProducingTest();
}

//! Test that all elements were produced (with task_group_context)
//! \brief \ref requirement \ref interface \ref stress
TEST_CASE("Test that all elements in range were produced through body (with task_group_context)") {
    oneapi::tbb::task_group_context context;
    WorkProducingTest(context);
}

//! Move iterator test for class that supports both move and copy semantics
//! \brief \ref requirement \ref interface
TEST_CASE("Move Semantics Test | Item: MovePreferable") {
    DoTestMoveSemantics<TestMoveSem::MovePreferable>();
}

//!  Move semantic test for move only class
//! \brief \ref requirement \ref interface
TEST_CASE("Move Semantics | Item: MoveOnly") {
    //  parallel_for_each uses is_copy_constructible to support non-copyable types
    DoTestMoveSemantics<TestMoveSem::MoveOnly>();
}

namespace test_req {

template <typename ItemType>
struct MinForEachBody : MinObj {
    using MinObj::MinObj;
    void operator()(const ItemType&) const {}
};

template <typename ItemType>
struct MinForEachFeederBody : MinObj {
    using MinObj::MinObj;
    void operator()(const ItemType&, oneapi::tbb::feeder<ItemType>& feeder) const {
        ItemType item(construct);
        feeder.add(item);
    }
};

} // namespace test_req

template <typename Iterator, typename Body>
void run_parallel_for_each_overloads() {
    using value_type = typename std::iterator_traits<Iterator>::value_type;

    value_type value(test_req::construct);

    Iterator it(&value);

    test_req::MinSequence<Iterator> sequence(test_req::construct, it);
    const auto& const_sequence = sequence;

    Body body(test_req::construct);
    oneapi::tbb::task_group_context ctx;

    oneapi::tbb::parallel_for_each(it, it, body);
    oneapi::tbb::parallel_for_each(it, it, body, ctx);

    oneapi::tbb::parallel_for_each(sequence, body);
    oneapi::tbb::parallel_for_each(sequence, body, ctx);

    oneapi::tbb::parallel_for_each(const_sequence, body);
    oneapi::tbb::parallel_for_each(const_sequence, body, ctx);
}

//! Test parallel_for_each type requirements
//! \brief \ref requirement
TEST_CASE("parallel_for_each type requirements") {
    // value_type should be copy constructible for input iterators
    using MinInputIterator = utils::InputIterator<test_req::CopyConstructible>;
    using MinForwardIterator = utils::ForwardIterator<test_req::MinObj>;
    using MinRandomIterator = utils::RandomIterator<test_req::MinObj>;

    // value_type should be copy/move constructible to be used in feeder.add and feeder task
    using FeederForwardIterator = utils::ForwardIterator<test_req::CopyConstructible>;
    using FeederRandomIterator = utils::RandomIterator<test_req::CopyConstructible>;

    using MinBody = test_req::MinForEachBody<test_req::MinObj>;
    using CopyBody = test_req::MinForEachBody<test_req::CopyConstructible>;
    using FeederBody = test_req::MinForEachFeederBody<test_req::CopyConstructible>;

    run_parallel_for_each_overloads<MinInputIterator, CopyBody>();
    run_parallel_for_each_overloads<MinInputIterator, FeederBody>();

    run_parallel_for_each_overloads<MinForwardIterator, MinBody>();
    run_parallel_for_each_overloads<FeederForwardIterator, FeederBody>();

    run_parallel_for_each_overloads<MinRandomIterator, MinBody>();
    run_parallel_for_each_overloads<FeederRandomIterator, FeederBody>();
}
