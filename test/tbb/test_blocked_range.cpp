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

#include "common/test.h"
#include "common/utils.h"
#include "common/utils_report.h"
#include "common/range_based_for_support.h"
#include "common/config.h"
#include "common/concepts_common.h"

#include "tbb/blocked_range.h"
#include "tbb/blocked_range2d.h"
#include "tbb/blocked_range3d.h"
#define TBB_PREVIEW_BLOCKED_RANGE_ND 1
#include "tbb/blocked_rangeNd.h"

//! \file test_blocked_range.cpp
//! \brief Test for [algorithms.blocked_range] specification

#include <utility> //for std::pair
#include <functional>
#include <vector>

//! Testing blocked_range with range based for
//! \brief \ref interface
TEST_CASE("Range based for") {
    using namespace range_based_for_support_tests;

    const std::size_t sequence_length = 100;
    std::size_t int_array[sequence_length] = {0};

    for (std::size_t i = 0; i < sequence_length; ++i) {
        int_array[i] = i + 1;
    }
    const tbb::blocked_range<std::size_t*> r(int_array, int_array + sequence_length, 1);

    CHECK_MESSAGE(range_based_for_accumulate<std::size_t>(r, std::plus<std::size_t>(), std::size_t(0))
            == gauss_summ_of_int_sequence(sequence_length), "incorrect accumulated value generated via range based for ?");
}

//! Proportional split does not overflow with blocked range
//! \brief \ref error_guessing \ref boundary
TEST_CASE("Proportional split overflow") {
    using tbb::blocked_range;
    using tbb::proportional_split;

    blocked_range<std::size_t> r1(0, std::size_t(-1) / 2);
    std::size_t size = r1.size();
    std::size_t begin = r1.begin();
    std::size_t end = r1.end();

    proportional_split p(1, 3);
    blocked_range<std::size_t> r2(r1, p);

    // overflow-free computation
    std::size_t parts = p.left() + p.right();
    std::size_t int_part = size / parts;
    std::size_t fraction = size - int_part * parts; // fraction < parts
    std::size_t right_idx = int_part * p.right() + fraction * p.right() / parts + 1;
    std::size_t newRangeBegin = end - right_idx;

    // Division in 'right_idx' very likely is inexact also.
    std::size_t tolerance = 1;
    std::size_t diff = (r2.begin() < newRangeBegin) ? (newRangeBegin - r2.begin()) : (r2.begin() - newRangeBegin);
    bool is_split_correct = diff <= tolerance;
    bool test_passed = (r1.begin() == begin && r1.end() == r2.begin() && is_split_correct &&
                        r2.end() == end);
    if (!test_passed) {
        REPORT("Incorrect split of blocked range[%lu, %lu) into r1[%lu, %lu) and r2[%lu, %lu), "
               "must be r1[%lu, %lu) and r2[%lu, %lu)\n", begin, end, r1.begin(), r1.end(), r2.begin(), r2.end(), begin, newRangeBegin, newRangeBegin, end);
        CHECK(test_passed);
    }
}

#if __TBB_CPP20_CONCEPTS_PRESENT

template <bool ExpectSatisfies, typename... Types>
    requires (... && (utils::well_formed_instantiation<tbb::blocked_range, Types> == ExpectSatisfies))
void test_blocked_range_constraint() {}

template <bool ExpectSatisfies, typename... Types>
    requires (... && (utils::well_formed_instantiation<tbb::blocked_range2d, Types, Types> == ExpectSatisfies))
void test_blocked_range2d_constraint() {}

template <typename... Types>
    requires (... && (utils::well_formed_instantiation<tbb::blocked_range2d, Types, test_concepts::Dummy> == false))
void test_blocked_range2d_col_invalid_constraint() {}

template <typename... Types>
    requires (... && (utils::well_formed_instantiation<tbb::blocked_range2d, test_concepts::Dummy, Types> == false))
void test_blocked_range2d_row_invalid_constraint() {}

template <bool ExpectSatisfies, typename... Types>
    requires (... && (utils::well_formed_instantiation<tbb::blocked_range3d, Types, Types, Types> == ExpectSatisfies))
void test_blocked_range3d_constraint() {}

template <typename... Types>
    requires (... && (utils::well_formed_instantiation<tbb::blocked_range3d, test_concepts::Dummy, Types, Types> == false))
void test_blocked_range3d_page_invalid_constraint() {}

template <typename... Types>
    requires (... && (utils::well_formed_instantiation<tbb::blocked_range3d, Types, test_concepts::Dummy, Types> == false))
void test_blocked_range3d_row_invalid_constraint() {}

template <typename... Types>
    requires (... && (utils::well_formed_instantiation<tbb::blocked_range3d, Types, Types, test_concepts::Dummy> == false))
void test_blocked_range3d_col_invalid_constraint() {}

template <typename T>
concept well_formed_blocked_range_Nd_instantiation_basic = requires {
    typename tbb::blocked_rangeNd<T, 1>;
};

template <typename... Types>
concept well_formed_blocked_range_Nd_instantiation = ( ... && well_formed_blocked_range_Nd_instantiation_basic<Types> );

//! \brief \ref error_guessing
TEST_CASE("constraints for blocked_range value") {
    using namespace test_concepts::blocked_range_value;
    using const_iterator = typename std::vector<int>::const_iterator;

    test_blocked_range_constraint</*Expected = */true,
                                  Correct, char, int, std::size_t, const_iterator>();

    test_blocked_range_constraint</*Expected = */false,
                                  NonCopyable, NonCopyAssignable, NonDestructible,
                                  NoOperatorLess, OperatorLessNonConst, WrongReturnOperatorLess,
                                  NoOperatorMinus, OperatorMinusNonConst, WrongReturnOperatorMinus,
                                  NoOperatorPlus, OperatorPlusNonConst, WrongReturnOperatorPlus>();
}

//! \brief \ref error_guessing
TEST_CASE("constraints for blocked_range2d value") {
    using namespace test_concepts::blocked_range_value;
    using const_iterator = typename std::vector<int>::const_iterator;

    test_blocked_range2d_constraint</*Expected = */true,
                                    Correct, char, int, std::size_t, const_iterator>();

    test_blocked_range2d_constraint</*Expected = */false,
                                    NonCopyable, NonCopyAssignable, NonDestructible,
                                    NoOperatorLess, OperatorLessNonConst, WrongReturnOperatorLess,
                                    NoOperatorMinus, OperatorMinusNonConst, WrongReturnOperatorMinus,
                                    NoOperatorPlus, OperatorPlusNonConst, WrongReturnOperatorPlus>();

    test_blocked_range2d_row_invalid_constraint<Correct, char, int, std::size_t, const_iterator>();
    test_blocked_range2d_col_invalid_constraint<Correct, char, int, std::size_t, const_iterator>();
}

//! \brief \ref error_guessing
TEST_CASE("constraints for blocked_range3d value") {
    using namespace test_concepts::blocked_range_value;
    using const_iterator = typename std::vector<int>::const_iterator;

    test_blocked_range3d_constraint</*Expected = */true,
                                    Correct, char, int, std::size_t, const_iterator>();

    test_blocked_range3d_constraint</*Expected = */false,
                                    NonCopyable, NonCopyAssignable, NonDestructible,
                                    NoOperatorLess, OperatorLessNonConst, WrongReturnOperatorLess,
                                    NoOperatorMinus, OperatorMinusNonConst, WrongReturnOperatorMinus,
                                    NoOperatorPlus, OperatorPlusNonConst, WrongReturnOperatorPlus>();

    test_blocked_range3d_page_invalid_constraint<Correct, char, int, std::size_t, const_iterator>();
    test_blocked_range3d_row_invalid_constraint<Correct, char, int, std::size_t, const_iterator>();
    test_blocked_range3d_col_invalid_constraint<Correct, char, int, std::size_t, const_iterator>();
}

//! \brief \ref error_guessing
TEST_CASE("constraints for blocked_rangeNd value") {
    using namespace test_concepts::blocked_range_value;
    using const_iterator = typename std::vector<int>::const_iterator;

    static_assert(well_formed_blocked_range_Nd_instantiation<Correct, char, int, std::size_t, const_iterator>);

    static_assert(!well_formed_blocked_range_Nd_instantiation<NonCopyable, NonCopyAssignable, NonDestructible,
                                                              NoOperatorLess, OperatorLessNonConst, WrongReturnOperatorLess,
                                                              NoOperatorMinus, OperatorMinusNonConst, WrongReturnOperatorMinus,
                                                              NoOperatorPlus, OperatorPlusNonConst, WrongReturnOperatorPlus>);
}

#endif // __TBB_CPP20_CONCEPTS_PRESENT

using Value = int;

void test_first_touch() {
    std::vector<Value> container;
    int start = 0;
    int end = 10;

    // Assuming blocked_range is a class that you have defined with begin() and end() methods
    tbb::blocked_range<int> range(start, end);

    try {
        // Assuming first_touch is a method of blocked_range or a similar class
        range.first_touch(container);

        // Verify that the container is filled correctly
        assert(container.size() == static_cast<size_t>(end - start));
        for (int i = 0; i < container.size(); ++i) {
            assert(container[i] == i);  // Check each element's value
        }
        std::cout << "first_touch test passed\n";
    } catch (const std::exception& e) {
        std::cerr << "first_touch test failed: " << e.what() << '\n';
    }
}

void test_execute_for() {
    // Using tbb::numa_partitioner with tbb::static_partitioner as the base partitioner
    tbb::numa_partitioner<tbb::static_partitioner> partitioner;
    int start = 0;
    int end = 100;

    // Assuming Range is a class that defines the range
    tbb::blocked_range<int> range(start, end);

    int result = 0; // To store the sum of the range
    auto body = [&result](const tbb::blocked_range<int>& subrange) {
        for (auto i = subrange.begin(); i != subrange.end(); ++i) {
            result += i;  // Summing the values in the range
        }
    };
    try {
        partitioner.execute_for(range, body);
        // Verify the result (sum of integers from start to end-1)
        int expected_sum = (start + end - 1) * (end - start) / 2;
        assert(result == expected_sum);
        std::cout << "execute_for test passed with result: " << result << '\n';
    } catch (const std::exception& e) {
        std::cerr << "execute_for test failed: " << e.what() << '\n';
    }
}

//! \brief \ref error_guessing
TEST_CASE("first-touch blocked range") {
  test_first_touch();
  test_execute_for();
}
