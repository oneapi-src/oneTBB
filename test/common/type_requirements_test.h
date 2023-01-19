/*
    Copyright (c) 2023 Intel Corporation

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

#ifndef __TBB_test_common_type_requirements_test_H_
#define __TBB_test_common_type_requirements_test_H_

#include "oneapi/tbb/blocked_range.h" // for oneapi::tbb::split
#include <atomic>
#include <memory>

namespace test_req {

struct ConstructT {};

constexpr ConstructT construct{};

struct MinObj {
    ~MinObj() = default;

    MinObj() = delete;
    MinObj(const MinObj&) = delete;
    MinObj(MinObj&&) = delete;

    MinObj& operator=(const MinObj&) = delete;
    MinObj& operator=(MinObj&&) = delete;

#if __TBB_CPP20_COMPARISONS_PRESENT
    friend bool operator==(const MinObj&, const MinObj&) = delete;
    friend auto operator<=>(const MinObj&, const MinObj&) = delete;
#endif

    MinObj(ConstructT) {}
}; // struct MinObj

struct CopyConstructible : MinObj {
    using MinObj::MinObj;
    CopyConstructible(const CopyConstructible&) : MinObj(construct) {}
};

struct Copyable : MinObj {
    using MinObj::MinObj;
    Copyable(const Copyable&) : MinObj(construct) {}
    Copyable& operator=(const Copyable&) { return *this; }
};

struct MinRange : MinObj {
    using MinObj::MinObj;
    MinRange(const MinRange&) : MinObj(construct) {}
    ~MinRange() {}

    bool empty() const { return true; }
    bool is_divisible() const { return false; }

    MinRange(MinRange&, oneapi::tbb::split) : MinObj(construct) {}
};

struct MinBlockedRangeValue : MinObj {
    using MinObj::MinObj;

    MinBlockedRangeValue(const MinBlockedRangeValue&) : MinObj(construct) {}
    ~MinBlockedRangeValue() {}

    void operator=(const MinBlockedRangeValue&) {}

    friend bool operator<(const MinBlockedRangeValue&, const MinBlockedRangeValue&) { return false; }
    friend std::size_t operator-(const MinBlockedRangeValue&, const MinBlockedRangeValue&) { return 0; }
    friend MinBlockedRangeValue operator+(const MinBlockedRangeValue&, std::size_t) { return MinBlockedRangeValue(construct); }
};

template <typename Iterator>
struct MinSequence : MinObj {
    using MinObj::MinObj;
    Iterator begin() const { return my_it; }
    Iterator end() const { return my_it; }

    MinSequence(ConstructT, Iterator it) : MinObj(construct), my_it(it) {}
private:
    Iterator my_it;
};

} // namespace test_req

#endif // __TBB_test_common_type_requirements_test_H_
