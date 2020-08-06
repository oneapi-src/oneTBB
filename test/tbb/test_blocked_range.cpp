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
#include "common/utils.h"
#include "common/utils_report.h"
#include "common/range_based_for_support.h"
#include "common/config.h"

#include "tbb/blocked_range.h"

//! \file test_blocked_range.cpp
//! \brief Test for [algorithms.blocked_range] specification

#include <utility> //for std::pair
#include <functional>

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

