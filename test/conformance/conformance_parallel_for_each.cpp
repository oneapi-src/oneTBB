/*
    Copyright (c) 2005-2021 Intel Corporation

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

struct MinPForEachBody {
    void operator()(const test_req::OnlyDestructible&) const {}

    MinPForEachBody() = delete;
    MinPForEachBody(const MinPForEachBody&) = delete;
    MinPForEachBody& operator=(const MinPForEachBody&) = delete;
private:
    MinPForEachBody(test_req::CreateFlag) {}
    ~MinPForEachBody() = default;
    friend struct test_req::Creator;
}; // struct MinPForEachBody

template <typename... Args>
void run_parallel_for_each_overloads(Args&&... args) {
    oneapi::tbb::task_group_context ctx;

    oneapi::tbb::parallel_for_each(args...);
    oneapi::tbb::parallel_for_each(args..., ctx);
}

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

//! Test parallel_for_each type requirements
//! \brief \ref requirement
TEST_CASE("parallel_for_each type requirements") {
    test_req::CopyConstructibleAndDestructible value1 = test_req::create<test_req::CopyConstructibleAndDestructible>();
    test_req::OnlyDestructible value2 = test_req::create<test_req::OnlyDestructible>();

    utils::InputIterator<test_req::CopyConstructibleAndDestructible> input_it(&value1);
    utils::ForwardIterator<test_req::OnlyDestructible> forward_it(&value2);
    utils::RandomIterator<test_req::OnlyDestructible> random_it(&value2);

    using input_seq_type = test_req::MinContainerBasedSequence<decltype(input_it)>;
    using forward_seq_type = test_req::MinContainerBasedSequence<decltype(forward_it)>;
    using random_seq_type = test_req::MinContainerBasedSequence<decltype(random_it)>;

    input_seq_type* input_seq_ptr = test_req::create_ptr<input_seq_type>();
    forward_seq_type* forward_seq_ptr = test_req::create_ptr<forward_seq_type>();
    random_seq_type* random_seq_ptr = test_req::create_ptr<random_seq_type>();

    const input_seq_type& const_input_seq_ref = *input_seq_ptr;
    const forward_seq_type& const_forward_seq_ref = *forward_seq_ptr;
    const random_seq_type& const_random_seq_ref = *random_seq_ptr;

    MinPForEachBody* body_ptr = test_req::create_ptr<MinPForEachBody>();
    
    run_parallel_for_each_overloads(input_it, input_it, *body_ptr);
    run_parallel_for_each_overloads(*input_seq_ptr, *body_ptr);
    run_parallel_for_each_overloads(const_input_seq_ref, *body_ptr);

    run_parallel_for_each_overloads(forward_it, forward_it, *body_ptr);
    run_parallel_for_each_overloads(*forward_seq_ptr, *body_ptr);
    run_parallel_for_each_overloads(const_forward_seq_ref, *body_ptr);

    run_parallel_for_each_overloads(random_it, random_it, *body_ptr);
    run_parallel_for_each_overloads(*random_seq_ptr, *body_ptr);
    run_parallel_for_each_overloads(const_random_seq_ref, *body_ptr);
    
    test_req::delete_ptr(body_ptr);
    test_req::delete_ptr(input_seq_ptr);
    test_req::delete_ptr(forward_seq_ptr);
    test_req::delete_ptr(random_seq_ptr);
}
