/*
    Copyright (c) 2020 Intel Corporation

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

#ifndef __TBB_test_common_test_comparisons_H
#define __TBB_test_common_test_comparisons_H

#include "test.h"

namespace comparisons_testing {

template <bool ExpectEqual, bool ExpectLess, typename T>
void testTwoWayComparisons( const T& lhs, const T& rhs ) {
    REQUIRE_MESSAGE((lhs < rhs == ExpectLess),
                    "Incorrect 2-way comparison result for less operation");
    REQUIRE_MESSAGE((lhs <= rhs == (ExpectLess || ExpectEqual)),
                    "Incorrect 2-way comparison result for less or equal operation");
    bool ExpectGreater = ExpectEqual ? false : !ExpectLess;
    REQUIRE_MESSAGE((lhs > rhs == ExpectGreater),
                    "Incorrect 2-way comparison result for greater operation");
    REQUIRE_MESSAGE((lhs >= rhs == (ExpectGreater || ExpectEqual)),
                    "Incorrect 2-way comparison result for greater or equal operation");
}

template <bool ExpectEqual, typename T>
void testEqualityComparisons( const T& lhs, const T& rhs ) {
    REQUIRE_MESSAGE((lhs == rhs) == ExpectEqual,
                    "Incorrect 2-way comparison result for equal operation");
    REQUIRE_MESSAGE((lhs != rhs) == !ExpectEqual,
                    "Incorrect 2-way comparison result for unequal operation");
}

template <bool ExpectEqual, bool ExpectLess, typename T>
void testEqualityAndLessComparisons( const T& lhs, const T& rhs ) {
    testEqualityComparisons<ExpectEqual>(lhs, rhs);
    testTwoWayComparisons<ExpectEqual, ExpectLess>(lhs, rhs);
}

class TwoWayComparable {
public:
    TwoWayComparable() : n(0) {
        reset();
    }

    TwoWayComparable( std::size_t num ) : n(num) {
        reset();
    }

    static void reset() {
        equal_called = false;
        unequal_called = false;
        less_called = false;
        greater_called = false;
        less_or_equal_called = false;
        greater_or_equal_called = false;
    }

    static bool equal_called;
    static bool unequal_called;
    static bool less_called;
    static bool greater_called;
    static bool less_or_equal_called;
    static bool greater_or_equal_called;

    friend bool operator==( const TwoWayComparable& lhs, const TwoWayComparable& rhs ) {
        equal_called = true;
        return lhs.n == rhs.n;
    }

    friend bool operator!=( const TwoWayComparable& lhs, const TwoWayComparable& rhs ) {
        unequal_called = true;
        return lhs.n != rhs.n;
    }

    friend bool operator<( const TwoWayComparable& lhs, const TwoWayComparable& rhs ) {
        less_called = true;
        return lhs.n < rhs.n;
    }

    friend bool operator>( const TwoWayComparable& lhs, const TwoWayComparable& rhs ) {
        greater_called = true;
        return lhs.n > rhs.n;
    }

    friend bool operator<=( const TwoWayComparable& lhs, const TwoWayComparable& rhs ) {
        less_or_equal_called = true;
        return lhs.n <= rhs.n;
    }

    friend bool operator>=( const TwoWayComparable& lhs, const TwoWayComparable& rhs ) {
        greater_or_equal_called = true;
        return lhs.n >= rhs.n;
    }

protected:
    std::size_t n;
}; // struct TwoWayComparable

bool TwoWayComparable::equal_called = false;
bool TwoWayComparable::unequal_called = false;
bool TwoWayComparable::less_called = false;
bool TwoWayComparable::greater_called = false;
bool TwoWayComparable::less_or_equal_called = false;
bool TwoWayComparable::greater_or_equal_called = false;

// This function should be executed after comparing to objects, containing TwoWayComparables
// using one of the comparison operators (<=>, <, >, <=, >=)
void check_two_way_comparison() {
    REQUIRE_MESSAGE(TwoWayComparable::less_called,
                    "operator < was not called during the comparison");
    REQUIRE_MESSAGE(!TwoWayComparable::greater_called,
                    "operator > was called during the comparison");
    REQUIRE_MESSAGE(!TwoWayComparable::less_or_equal_called,
                    "operator <= was called during the comparison");
    REQUIRE_MESSAGE(!TwoWayComparable::greater_or_equal_called,
                    "operator >= was called during the comparison");
    REQUIRE_MESSAGE(!(TwoWayComparable::equal_called),
                    "operator == was called during the comparison");
    REQUIRE_MESSAGE(!(TwoWayComparable::unequal_called),
                    "operator == was called during the comparison");
    TwoWayComparable::reset();
}

} // namespace comparisons_testing

#endif // __TBB_test_common_test_comparisons_H
