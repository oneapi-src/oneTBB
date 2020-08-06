/*
    Copyright (c) 2019-2020 Intel Corporation

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

#ifndef __TBB_concurrent_set_H
#define __TBB_concurrent_set_H

#include "detail/_concurrent_skip_list.h"
#include "tbb_allocator.h"
#include <functional>
#include <utility>

namespace tbb {
namespace detail {
namespace d1 {

template<typename Key, typename KeyCompare, typename RandomGenerator, typename Allocator, bool AllowMultimapping>
struct set_traits {
    static constexpr std::size_t max_level = RandomGenerator::max_level;
    using random_level_generator_type = RandomGenerator;
    using key_type = Key;
    using value_type = key_type;
    using compare_type = KeyCompare;
    using value_compare = compare_type;
    using reference = value_type&;
    using const_reference = const value_type&;
    using allocator_type = Allocator;

    static constexpr bool allow_multimapping = AllowMultimapping;

    static const key_type& get_key(const_reference val) {
        return val;
    }

    static value_compare value_comp(compare_type comp) { return comp; }
}; // struct set_traits

template <typename Key, typename Compare, typename Allocator>
class concurrent_multiset;

template <typename Key, typename Compare = std::less<Key>, typename Allocator = tbb::tbb_allocator<Key>>
class concurrent_set : public concurrent_skip_list<set_traits<Key, Compare, concurrent_geometric_level_generator<32>, Allocator, false>> {
    using base_type = concurrent_skip_list<set_traits<Key, Compare, concurrent_geometric_level_generator<32>, Allocator, false>>;
public:
    using key_type = Key;
    using value_type = typename base_type::value_type;
    using size_type = typename base_type::size_type;
    using difference_type = typename base_type::difference_type;
    using key_compare = Compare;
    using value_compare = typename base_type::value_compare;
    using allocator_type = Allocator;

    using reference = typename base_type::reference;
    using const_reference = typename base_type::const_reference;
    using pointer = typename base_type::pointer;
    using const_pointer = typename base_type::const_pointer;

    using iterator = typename base_type::iterator;
    using const_iterator = typename base_type::const_iterator;

    using node_type = typename base_type::node_type;

    // Include constructors of base_type
    using base_type::base_type;

    template<typename OtherCompare>
    void merge(concurrent_set<key_type, OtherCompare, Allocator>& source) {
        this->internal_merge(source);
    }

    template<typename OtherCompare>
    void merge(concurrent_set<key_type, OtherCompare, Allocator>&& source) {
        this->internal_merge(std::move(source));
    }

    template<typename OtherCompare>
    void merge(concurrent_multiset<key_type, OtherCompare, Allocator>& source) {
        this->internal_merge(source);
    }

    template<typename OtherCompare>
    void merge(concurrent_multiset<key_type, OtherCompare, Allocator>&& source) {
        this->internal_merge(std::move(source));
    }
}; // class concurrent_set

#if __TBB_CPP17_DEDUCTION_GUIDES_PRESENT

template<template<typename...> class Set, typename Key, typename... Args>
using concurrent_set_type = Set<Key,
                                std::conditional_t< (sizeof...(Args) > 0) && !is_allocator_v<pack_element_t<0, Args...>>,
                                                     pack_element_t<0, Args...>,
                                                     std::less<Key>>,
                                std::conditional_t< (sizeof...(Args) > 0) && is_allocator_v<pack_element_t<sizeof...(Args) - 1, Args...>>,
                                                     pack_element_t<sizeof...(Args) - 1, Args...>,
                                                     tbb::tbb_allocator<Key>>>;

template<typename It, typename... Args>
concurrent_set(It, It, Args...)
-> concurrent_set_type<concurrent_set, iterator_value_t<It>, Args...>;

template<typename Key, typename... Args>
concurrent_set(std::initializer_list<Key>, Args...)
-> concurrent_set_type<concurrent_set, Key, Args...>;

#endif // __TBB_CPP17_DEDUCTION_GUIDES_PRESENT

template <typename Key, typename Compare, typename Allocator>
void swap( concurrent_set<Key, Compare, Allocator>& lhs,
           concurrent_set<Key, Compare, Allocator>& rhs )
{
    lhs.swap(rhs);
}

template <typename Key, typename Compare = std::less<Key>, typename Allocator = tbb::tbb_allocator<Key>>
class concurrent_multiset : public concurrent_skip_list<set_traits<Key, Compare, concurrent_geometric_level_generator<32>, Allocator, true>> {
    using base_type = concurrent_skip_list<set_traits<Key, Compare, concurrent_geometric_level_generator<32>, Allocator, true>>;
public:
    using key_type = Key;
    using value_type = typename base_type::value_type;
    using size_type = typename base_type::size_type;
    using difference_type = typename base_type::difference_type;
    using key_compare = Compare;
    using value_compare = typename base_type::value_compare;
    using allocator_type = Allocator;

    using reference = typename base_type::reference;
    using const_reference = typename base_type::const_reference;
    using pointer = typename base_type::pointer;
    using const_pointer = typename base_type::const_pointer;

    using iterator = typename base_type::iterator;
    using const_iterator = typename base_type::const_iterator;

    using node_type = typename base_type::node_type;

    // Include constructors of base_type;
    using base_type::base_type;

    template<typename OtherCompare>
    void merge(concurrent_set<key_type, OtherCompare, Allocator>& source) {
        this->internal_merge(source);
    }

    template<typename OtherCompare>
    void merge(concurrent_set<key_type, OtherCompare, Allocator>&& source) {
        this->internal_merge(std::move(source));
    }

    template<typename OtherCompare>
    void merge(concurrent_multiset<key_type, OtherCompare, Allocator>& source) {
        this->internal_merge(source);
    }

    template<typename OtherCompare>
    void merge(concurrent_multiset<key_type, OtherCompare, Allocator>&& source) {
        this->internal_merge(std::move(source));
    }
}; // class concurrent_multiset

#if __TBB_CPP17_DEDUCTION_GUIDES_PRESENT

template<typename It, typename... Args>
concurrent_multiset(It, It, Args...)
-> concurrent_set_type<concurrent_multiset, iterator_value_t<It>, Args...>;

template<typename Key, typename... Args>
concurrent_multiset(std::initializer_list<Key>, Args...)
-> concurrent_set_type<concurrent_multiset, Key, Args...>;

#endif // __TBB_CPP17_DEDUCTION_GUIDES_PRESENT

template <typename Key, typename Compare, typename Allocator>
void swap( concurrent_multiset<Key, Compare, Allocator>& lhs,
           concurrent_multiset<Key, Compare, Allocator>& rhs )
{
    lhs.swap(rhs);
}

} // namespace d1
} // namespace detail

inline namespace v1 {

using detail::d1::concurrent_set;
using detail::d1::concurrent_multiset;
using detail::split;

} // inline namespace v1
} // namespace tbb

#endif // __TBB_concurrent_set_H
