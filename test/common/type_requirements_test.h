/*
    Copyright (c) 2022 Intel Corporation

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

struct CreateFlag {};

struct Creator {
    struct Deleter {
        template <typename T>
        void operator()(T* ptr) const {
            delete ptr;
        }
    };

    template <typename T, typename... Args>
    static std::unique_ptr<T, Deleter> create_ptr(Args&&... args) {
        return std::unique_ptr<T, Deleter>{new T(CreateFlag{}, std::forward<Args>(args)...)};
    }
};

template <typename T, typename... Args>
std::unique_ptr<T, Creator::Deleter> create_ptr(Args&&... args) {
    return Creator::create_ptr<T>(std::forward<Args>(args)...);
}

struct MinRange {
    MinRange(const MinRange&) = default;
    MinRange(MinRange&, oneapi::tbb::split) {}
    ~MinRange() = default;

    bool empty() const { return true; }
    bool is_divisible() const { return false; }

    MinRange() = delete;
    MinRange& operator=(const MinRange&) = delete;

private:
    MinRange(CreateFlag) {}
    friend struct Creator;
}; // struct MinRange

struct NonDestructible {
    NonDestructible() = delete;
    NonDestructible(const NonDestructible&) = delete;
    NonDestructible& operator=(const NonDestructible&) = delete;
protected:
    NonDestructible(CreateFlag) {}
    ~NonDestructible() = default;
    friend struct Creator;
}; // struct NonDestructible

struct OnlyDestructible {
    ~OnlyDestructible() = default;

    OnlyDestructible() = delete;
    OnlyDestructible(const OnlyDestructible&) = delete;
    OnlyDestructible& operator=(const OnlyDestructible&) = delete;

protected:
    OnlyDestructible(CreateFlag) {}
    friend struct Creator;
}; // struct OnlyDestructible

struct CopyConstructibleAndDestructible : OnlyDestructible {
    CopyConstructibleAndDestructible(const CopyConstructibleAndDestructible&) : OnlyDestructible(CreateFlag{}) {}
private:
    CopyConstructibleAndDestructible(CreateFlag) : OnlyDestructible(CreateFlag{}) {}
    friend struct Creator;
}; // struct CopyConstructibleAndDestructible

// Value for parallel_reduce and parallel_scan
struct MinValue {
    MinValue() = delete;
    MinValue(const MinValue&) = default;
    ~MinValue() = default;
    MinValue& operator=(const MinValue&) = default;
private:
    MinValue(CreateFlag) {}
    friend struct Creator;
}; // struct MinValue

struct MinReduction {
    MinValue operator()(const MinValue& x, const MinValue&) const { return x; }

    MinReduction() = delete;
    MinReduction(const MinReduction&) = delete;
    MinReduction& operator=(const MinReduction&) = delete;

private:
    MinReduction(CreateFlag) {}
    ~MinReduction() = default;
    friend struct Creator;
}; // struct MinReduction

template <typename Iterator>
struct MinContainerBasedSequence {
    Iterator begin() const { return Iterator{nullptr}; }
    Iterator end() const { return Iterator{nullptr}; }

    MinContainerBasedSequence() = delete;
    MinContainerBasedSequence(const MinContainerBasedSequence&) = delete;
    MinContainerBasedSequence& operator=(const MinContainerBasedSequence&) = delete;
private:
    MinContainerBasedSequence(CreateFlag) {}
    ~MinContainerBasedSequence() = default;
    friend struct Creator;
}; // struct MinContainerBasedSequence

template <typename... Args>
struct MinFunctionObject {
    void operator()(Args...) const {}

    MinFunctionObject() = delete;
    MinFunctionObject(const MinFunctionObject&) = delete;
    MinFunctionObject& operator=(const MinFunctionObject&) = delete;
private:
    MinFunctionObject(CreateFlag) {}
    ~MinFunctionObject() = default;
    friend struct Creator;
}; // struct MinFunctionObject

template <typename T>
struct MinCompare {
    bool operator()(const T&, const T&) const { return true; }

    MinCompare() = delete;
    MinCompare(const MinCompare&) = default;
    MinCompare& operator=(const MinCompare&) = delete;

    ~MinCompare() = default;
private:
    MinCompare(CreateFlag) {}
    friend struct Creator;
}; // struct MinCompare

struct MinBlockedRangeValue {
    MinBlockedRangeValue(const MinBlockedRangeValue&) = default;
    MinBlockedRangeValue& operator=(const MinBlockedRangeValue&) = default;
    ~MinBlockedRangeValue() = default;

    MinBlockedRangeValue() = delete;

    bool operator<(const MinBlockedRangeValue& other) const { return real_value < other.real_value; }
    int operator-(const MinBlockedRangeValue& other) const { return real_value - other.real_value; }
    MinBlockedRangeValue operator+(int k) const { return MinBlockedRangeValue(CreateFlag{}, real_value + k); }
private:
    MinBlockedRangeValue(CreateFlag, int rv) : real_value(rv) {}
    friend struct Creator;
    
    int real_value;
}; // struct MinBlockedRangeValue

} // namespace test_req

#endif // __TBB_test_common_type_requirements_test_H_
