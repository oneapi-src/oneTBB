/*
    Copyright (c) 2020-2021 Intel Corporation

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
#include "common/utils_concurrency_limit.h"
#include "common/type_requirements_test.h"
#include "common/iterator.h"

#include "oneapi/tbb/parallel_sort.h"
#include "oneapi/tbb/global_control.h"

#include <vector>

//! \file conformance_parallel_sort.cpp
//! \brief Test for [algorithms.parallel_sort]

const int vector_size = 10000;

std::vector<int> get_random_vector() {
    std::vector<int> result(vector_size);
    for (int i = 0; i < vector_size; ++i)
        result[i] = rand() % vector_size;
    return result;
}

struct MinSortable {
    MinSortable(MinSortable&&) = default;
    MinSortable& operator=(MinSortable&&) = default;

    MinSortable() = delete;
    MinSortable(const MinSortable&) = delete;
    MinSortable& operator=(const MinSortable&) = delete;

    ~MinSortable() = default;
protected:
    MinSortable(test_req::CreateFlag) {}
    friend struct test_req::Creator;
}; // struct MinSortable

void swap(MinSortable&, MinSortable&) {}

struct MinLessThanSortable : MinSortable {
    bool operator<(const MinLessThanSortable&) const { return true; }
private:
    MinLessThanSortable(test_req::CreateFlag) : MinSortable(test_req::CreateFlag{}) {}
    friend struct test_req::Creator;
}; // struct MinLessThanSortable

//! Iterator based range sorting test (default comparator)
//! \brief \ref requirement \ref interface
TEST_CASE("Iterator based range sorting test (default comparator)") {
    for ( auto concurrency_level : utils::concurrency_range() ) {
        oneapi::tbb::global_control control(oneapi::tbb::global_control::max_allowed_parallelism, concurrency_level);

        auto test_vector = get_random_vector();
        oneapi::tbb::parallel_sort(test_vector.begin(), test_vector.end());

        for(auto it = test_vector.begin(); it != test_vector.end() - 1; ++it)
            REQUIRE_MESSAGE(*it <= *(it+1), "Testing data not sorted");
    }
}

//! Iterator based range sorting test (greater comparator)
//! \brief \ref requirement \ref interface
TEST_CASE ("Iterator based range sorting test (greater comparator)") {
    for ( auto concurrency_level : utils::concurrency_range() ) {
        oneapi::tbb::global_control control(oneapi::tbb::global_control::max_allowed_parallelism, concurrency_level);

        auto test_vector = get_random_vector();
        oneapi::tbb::parallel_sort(test_vector.begin(), test_vector.end(), std::greater<int>());

        for(auto it = test_vector.begin(); it != test_vector.end() - 1; ++it)
            REQUIRE_MESSAGE(*it >= *(it+1), "Testing data not sorted");
    }
}

//! Range sorting test (default comparator)
//! \brief \ref requirement \ref interface
TEST_CASE ("Range sorting test (default comparator)") {
    for ( auto concurrency_level : utils::concurrency_range() ) {
        oneapi::tbb::global_control control(oneapi::tbb::global_control::max_allowed_parallelism, concurrency_level);

        auto test_vector = get_random_vector();
        oneapi::tbb::parallel_sort(test_vector);

        for(auto it = test_vector.begin(); it != test_vector.end() - 1; ++it)
            REQUIRE_MESSAGE(*it <= *(it+1), "Testing data not sorted");
    }
}

//! Range sorting test (greater comparator)
//! \brief \ref requirement \ref interface
TEST_CASE ("Range sorting test (greater comparator)") {
    for ( auto concurrency_level : utils::concurrency_range() ) {
        oneapi::tbb::global_control control(oneapi::tbb::global_control::max_allowed_parallelism, concurrency_level);

        auto test_vector = get_random_vector();
        oneapi::tbb::parallel_sort(test_vector, std::greater<int>());

        for(auto it = test_vector.begin(); it != test_vector.end() - 1; ++it)
            REQUIRE_MESSAGE(*it >= *(it+1), "Testing data not sorted");
    }
}

//! Testing parallel_sort type requirements
//! \brief \ref requirement
TEST_CASE("parallel_sort type requirements") {
    MinSortable value = test_req::create<MinSortable>();
    MinLessThanSortable less_value = test_req::create<MinLessThanSortable>();

    utils::RandomIterator<MinSortable> random_it(&value);
    utils::RandomIterator<MinLessThanSortable> random_less_it(&less_value);

    using seq_type = test_req::MinContainerBasedSequence<decltype(random_it)>;
    using less_seq_type = test_req::MinContainerBasedSequence<decltype(random_less_it)>;

    seq_type* seq_ptr = test_req::create_ptr<seq_type>();
    less_seq_type* less_seq_ptr = test_req::create_ptr<less_seq_type>();

    using compare_type = test_req::MinCompare<MinSortable>;
    compare_type compare = test_req::create<compare_type>();

    oneapi::tbb::parallel_sort(random_less_it, random_less_it);
    oneapi::tbb::parallel_sort(random_it, random_it, compare);

    oneapi::tbb::parallel_sort(*less_seq_ptr);
    oneapi::tbb::parallel_sort(*seq_ptr, compare);

    test_req::delete_ptr(seq_ptr);
    test_req::delete_ptr(less_seq_ptr);
}
