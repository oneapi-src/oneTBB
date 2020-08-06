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

#ifndef __TBB_concurrent_unordered_set_H
#define __TBB_concurrent_unordered_set_H

#include "detail/_concurrent_unordered_base.h"
#include "tbb_allocator.h"

namespace tbb {
namespace detail {
namespace d1 {

template <typename Key, typename Hash, typename KeyEqual, typename Allocator, bool AllowMultimapping>
struct concurrent_unordered_set_traits {
    using key_type = Key;
    using value_type = key_type;
    using allocator_type = Allocator;
    using hash_compare_type = hash_compare<key_type, Hash, KeyEqual>;
    static constexpr bool allow_multimapping = AllowMultimapping;

    static constexpr const key_type& get_key( const value_type& value ) {
        return value;
    }
}; // class concurrent_unordered_set_traits

template <typename Key, typename Hash, typename KeyEqual, typename Allocator>
class concurrent_unordered_multiset;

template <typename Key, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>,
          typename Allocator = tbb::tbb_allocator<Key>>
class concurrent_unordered_set
    : public concurrent_unordered_base<concurrent_unordered_set_traits<Key, Hash, KeyEqual, Allocator, false>>
{
    using traits_type = concurrent_unordered_set_traits<Key, Hash, KeyEqual, Allocator, false>;
    using base_type = concurrent_unordered_base<traits_type>;
public:
    using key_type = typename base_type::key_type;
    using value_type = typename base_type::value_type;
    using size_type = typename base_type::size_type;
    using difference_type = typename base_type::difference_type;
    using hasher = typename base_type::hasher;
    using key_equal = typename base_type::key_equal;
    using allocator_type = typename base_type::allocator_type;
    using reference = typename base_type::reference;
    using const_reference = typename base_type::const_reference;
    using pointer = typename base_type::pointer;
    using const_pointer = typename base_type::const_pointer;
    using iterator = typename base_type::iterator;
    using const_iterator = typename base_type::const_iterator;
    using local_iterator = typename base_type::local_iterator;
    using const_local_iterator = typename base_type::const_local_iterator;
    using node_type = typename base_type::node_type;

    // Include constructors of base_type;
    using base_type::base_type;

    template <typename OtherHash, typename OtherKeyEqual>
    void merge( concurrent_unordered_set<key_type, OtherHash, OtherKeyEqual, allocator_type>& source ) {
        this->internal_merge(source);
    }

    template <typename OtherHash, typename OtherKeyEqual>
    void merge( concurrent_unordered_set<key_type, OtherHash, OtherKeyEqual, allocator_type>&& source ) {
        this->internal_merge(std::move(source));
    }

    template <typename OtherHash, typename OtherKeyEqual>
    void merge( concurrent_unordered_multiset<key_type, OtherHash, OtherKeyEqual, allocator_type>& source ) {
        this->internal_merge(source);
    }

    template <typename OtherHash, typename OtherKeyEqual>
    void merge( concurrent_unordered_multiset<key_type, OtherHash, OtherKeyEqual, allocator_type>&& source ) {
        this->internal_merge(std::move(source));
    }
}; // class concurrent_unordered_set

#if __TBB_CPP17_DEDUCTION_GUIDES_PRESENT

template <typename Value>
using cu_set_def_allocator = tbb::tbb_allocator<Value>;

template <template <typename...> class Set, typename T, typename... Args>
using cu_set_type = Set<T,
                        std::conditional_t<(sizeof...(Args) > 0) && !is_allocator_v<pack_element_t<0, Args...>>,
                                           pack_element_t<0, Args...>, std::hash<T>>,
                        std::conditional_t<(sizeof...(Args) > 1) && !is_allocator_v<pack_element_t<1, Args...>>,
                                           pack_element_t<1, Args...>, std::equal_to<T>>,
                        std::conditional_t<(sizeof...(Args) > 0) && is_allocator_v<pack_element_t<sizeof...(Args) - 1, Args...>>,
                                           pack_element_t<sizeof...(Args) - 1, Args...>, cu_set_def_allocator<T>>>;

template <typename Key, typename Hash, typename KeyEq, typename Allocator>
concurrent_unordered_set( const concurrent_unordered_set<Key, Hash, KeyEq, Allocator>&, const Allocator& )
-> concurrent_unordered_set<Key, Hash, KeyEq, Allocator>;

// Deduction guide for the constructor from two iterators
template <typename I>
concurrent_unordered_set( I, I )
-> cu_set_type<concurrent_unordered_set, iterator_value_t<I>>;

// Deduction guide for the constructor from two iterators and hasher/equal/allocator
template <typename I, typename... Args>
concurrent_unordered_set( I, I, std::size_t, Args... )
-> cu_set_type<concurrent_unordered_set, iterator_value_t<I>, Args...>;

// Deduction guide for the constructor from an initializer_list
template <typename T>
concurrent_unordered_set( std::initializer_list<T> )
-> cu_set_type<concurrent_unordered_set, T>;

// Deduction guide for the constructor from an initializer_list and hasher/equal/allocator
template <typename T, typename... Args>
concurrent_unordered_set( std::initializer_list<T>, std::size_t, Args... )
-> cu_set_type<concurrent_unordered_set, T, Args...>;

#endif // __TBB_CPP17_DEDUCTION_GUIDES_PRESENT

template <typename Key, typename Hash, typename KeyEqual, typename Allocator>
void swap( concurrent_unordered_set<Key, Hash, KeyEqual, Allocator>& lhs,
           concurrent_unordered_set<Key, Hash, KeyEqual, Allocator>& rhs ) {
    lhs.swap(rhs);
}

template <typename Key, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>,
          typename Allocator = tbb::tbb_allocator<Key>>
class concurrent_unordered_multiset
    : public concurrent_unordered_base<concurrent_unordered_set_traits<Key, Hash, KeyEqual, Allocator, true>>
{
    using traits_type = concurrent_unordered_set_traits<Key, Hash, KeyEqual, Allocator, true>;
    using base_type = concurrent_unordered_base<traits_type>;
public:
    using key_type = typename base_type::key_type;
    using value_type = typename base_type::value_type;
    using size_type = typename base_type::size_type;
    using difference_type = typename base_type::difference_type;
    using hasher = typename base_type::hasher;
    using key_equal = typename base_type::key_equal;
    using allocator_type = typename base_type::allocator_type;
    using reference = typename base_type::reference;
    using const_reference = typename base_type::const_reference;
    using pointer = typename base_type::pointer;
    using const_pointer = typename base_type::const_pointer;
    using iterator = typename base_type::iterator;
    using const_iterator = typename base_type::const_iterator;
    using local_iterator = typename base_type::local_iterator;
    using const_local_iterator = typename base_type::const_local_iterator;
    using node_type = typename base_type::node_type;

    // Include constructors of base_type;
    using base_type::base_type;

    template <typename OtherHash, typename OtherKeyEqual>
    void merge( concurrent_unordered_set<key_type, OtherHash, OtherKeyEqual, allocator_type>& source ) {
        this->internal_merge(source);
    }

    template <typename OtherHash, typename OtherKeyEqual>
    void merge( concurrent_unordered_set<key_type, OtherHash, OtherKeyEqual, allocator_type>&& source ) {
        this->internal_merge(std::move(source));
    }

    template <typename OtherHash, typename OtherKeyEqual>
    void merge( concurrent_unordered_multiset<key_type, OtherHash, OtherKeyEqual, allocator_type>& source ) {
        this->internal_merge(source);
    }

    template <typename OtherHash, typename OtherKeyEqual>
    void merge( concurrent_unordered_multiset<key_type, OtherHash, OtherKeyEqual, allocator_type>&& source ) {
        this->internal_merge(std::move(source));
    }
}; // class concurrent_unordered_multiset

#if __TBB_CPP17_DEDUCTION_GUIDES_PRESENT
template <typename Key, typename Hash, typename KeyEq, typename Allocator>
concurrent_unordered_multiset( const concurrent_unordered_multiset<Key, Hash, KeyEq, Allocator>&, const Allocator& )
-> concurrent_unordered_multiset<Key, Hash, KeyEq, Allocator>;

// Deduction guide for the constructor from two iterators
template <typename I>
concurrent_unordered_multiset( I, I )
-> cu_set_type<concurrent_unordered_multiset, iterator_value_t<I>>;

// Deduction guide for the constructor from two iterators and hasher/equal/allocator
template <typename I, typename... Args>
concurrent_unordered_multiset( I, I, std::size_t, Args... )
-> cu_set_type<concurrent_unordered_multiset, iterator_value_t<I>, Args...>;

// Deduction guide for the constructor from an initializer_list
template <typename T>
concurrent_unordered_multiset( std::initializer_list<T> )
-> cu_set_type<concurrent_unordered_multiset, T>;

// Deduction guide for the constructor from an initializer_list and hasher/equal/allocator
template <typename T, typename... Args>
concurrent_unordered_multiset( std::initializer_list<T>, std::size_t, Args... )
-> cu_set_type<concurrent_unordered_multiset, T, Args...>;

#endif // __TBB_CPP17_DEDUCTION_GUIDES_PRESENT

template <typename Key, typename Hash, typename KeyEqual, typename Allocator>
void swap( concurrent_unordered_multiset<Key, Hash, KeyEqual, Allocator>& lhs,
           concurrent_unordered_multiset<Key, Hash, KeyEqual, Allocator>& rhs ) {
    lhs.swap(rhs);
}

} // namespace d1
} // namespace detail

inline namespace v1 {

using detail::d1::concurrent_unordered_set;
using detail::d1::concurrent_unordered_multiset;
using detail::split;

} // inline namespace v1
} // namespace tbb

#endif // __TBB_concurrent_unordered_set_H
