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

#ifndef __TBB_concurrent_unordered_map_H
#define __TBB_concurrent_unordered_map_H

#include "detail/_concurrent_unordered_base.h"
#include "tbb_allocator.h"
#include <functional>

namespace tbb {
namespace detail {
namespace d1 {

template <typename Key, typename T, typename Hash, typename KeyEqual, typename Allocator, bool AllowMultimapping>
struct concurrent_unordered_map_traits {
    using value_type = std::pair<const Key, T>;
    using key_type = Key;
    using allocator_type = Allocator;
    using hash_compare_type = hash_compare<Key, Hash, KeyEqual>;
    static constexpr bool allow_multimapping = AllowMultimapping;

    static constexpr const key_type& get_key( const value_type& value ) {
        return value.first;
    }
}; // struct concurrent_unordered_map_traits

template <typename Key, typename T, typename Hash, typename KeyEqual, typename Allocator>
class concurrent_unordered_multimap;

template <typename Key, typename T, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>,
          typename Allocator = tbb::tbb_allocator<std::pair<const Key, T>> >
class concurrent_unordered_map
    : public concurrent_unordered_base<concurrent_unordered_map_traits<Key, T, Hash, KeyEqual, Allocator, false>>
{
    using traits_type = concurrent_unordered_map_traits<Key, T, Hash, KeyEqual, Allocator, false>;
    using base_type = concurrent_unordered_base<traits_type>;
public:
    using key_type = typename base_type::key_type;
    using mapped_type = T;
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

    // Include constructors of base type
    using base_type::base_type;

    // Observers
    mapped_type& operator[]( const key_type& key ) {
        iterator where = this->find(key);

        if (where == this->end()) {
            where = this->emplace(std::piecewise_construct, std::forward_as_tuple(key), std::tuple<>()).first;
        }
        return where->second;
    }

    mapped_type& operator[]( key_type&& key ) {
        iterator where = this->find(key);

        if (where == this->end()) {
            where = this->emplace(std::piecewise_construct, std::forward_as_tuple(std::move(key)), std::tuple<>()).first;
        }
        return where->second;
    }

    mapped_type& at( const key_type& key ) {
        iterator where = this->find(key);

        if (where == this->end()) {
            throw_exception(exception_id::invalid_key);
        }
        return where->second;
    }

    const mapped_type& at( const key_type& key ) const {
        const_iterator where = this->find(key);

        if (where == this->end()) {
            throw_exception(exception_id::out_of_range);
        }
        return where->second;
    }

    using base_type::insert;

    template<typename P>
    typename std::enable_if<std::is_constructible<value_type, P&&>::value,
                            std::pair<iterator, bool>>::type insert( P&& value ) {
        return this->emplace(std::forward<P>(value));
    }

    template<typename P>
    typename std::enable_if<std::is_constructible<value_type, P&&>::value,
                            iterator>::type insert( const_iterator hint, P&& value ) {
        return this->emplace_hint(hint, std::forward<P>(value));
    }

    template <typename OtherHash, typename OtherKeyEqual>
    void merge( concurrent_unordered_map<key_type, mapped_type, OtherHash, OtherKeyEqual, allocator_type>& source ) {
        this->internal_merge(source);
    }

    template <typename OtherHash, typename OtherKeyEqual>
    void merge( concurrent_unordered_map<key_type, mapped_type, OtherHash, OtherKeyEqual, allocator_type>&& source ) {
        this->internal_merge(std::move(source));
    }

    template <typename OtherHash, typename OtherKeyEqual>
    void merge( concurrent_unordered_multimap<key_type, mapped_type, OtherHash, OtherKeyEqual, allocator_type>& source ) {
        this->internal_merge(source);
    }

    template <typename OtherHash, typename OtherKeyEqual>
    void merge( concurrent_unordered_multimap<key_type, mapped_type, OtherHash, OtherKeyEqual, allocator_type>&& source ) {
        this->internal_merge(std::move(source));
    }
}; // class concurrent_unordered_map

#if __TBB_CPP17_DEDUCTION_GUIDES_PRESENT

template <typename Key, typename Mapped>
using cu_map_def_allocator = tbb::tbb_allocator<std::pair<const Key, Mapped>>;

template <template <typename...> class Map, typename Key, typename Mapped, typename... Args>
using cu_map_type = Map<Key, Mapped,
                        std::conditional_t<(sizeof...(Args) > 0) && !is_allocator_v<pack_element_t<0, Args...>>,
                                           pack_element_t<0, Args...>, std::hash<Key>>,
                        std::conditional_t<(sizeof...(Args) > 1) && !is_allocator_v<pack_element_t<1, Args...>>,
                                           pack_element_t<1, Args...>, std::equal_to<Key>>,
                        std::conditional_t<(sizeof...(Args) > 0) && is_allocator_v<pack_element_t<sizeof...(Args) - 1, Args...>>,
                                           pack_element_t<sizeof...(Args) - 1, Args...>,
                                           cu_map_def_allocator<Key, Mapped>>>;

// Deduction guide for copy constructor with additional allocator parameter
// TODO: Investigate why implicit deduction guides are not generated
//       for concurrent_unordered_map( const concurrent_unordered_map&, Allocator).
template <typename Key, typename Val, typename Hash, typename KeyEq, typename Allocator>
concurrent_unordered_map( const concurrent_unordered_map<Key, Val, Hash, KeyEq, Allocator>&, const Allocator& )
-> concurrent_unordered_map<Key, Val, Hash, KeyEq, Allocator>;

// Deduction guide for the constructor from two iterators
template <typename I>
concurrent_unordered_map( I, I )
-> cu_map_type<concurrent_unordered_map, iterator_key_t<I>, iterator_mapped_t<I>>;

// Deduction guide for the constructor from two iterators and hasher/equal/allocator
template <typename I, typename... Args>
concurrent_unordered_map( I, I, std::size_t, Args... )
-> cu_map_type<concurrent_unordered_map, iterator_key_t<I>, iterator_mapped_t<I>, Args...>;

// Deduction guide for the constructor from an initializer_list
template <typename Key, typename Mapped>
concurrent_unordered_map( std::initializer_list<std::pair<const Key, Mapped>> )
-> cu_map_type<concurrent_unordered_map, Key, Mapped>;

// Deduction guide for the constructor from an initializer_list and hasher/equal/allocator
template <typename Key, typename Mapped, typename... Args>
concurrent_unordered_map( std::initializer_list<std::pair<const Key, Mapped>>, std::size_t, Args... )
-> cu_map_type<concurrent_unordered_map, Key, Mapped, Args...>;

#endif // __TBB_CPP17_DEDUCTION_GUIDES_PRESENT

template <typename Key, typename T, typename Hash, typename KeyEqual, typename Allocator>
void swap( concurrent_unordered_map<Key, T, Hash, KeyEqual, Allocator>& lhs,
           concurrent_unordered_map<Key, T, Hash, KeyEqual, Allocator>& rhs ) {
    lhs.swap(rhs);
}

template <typename Key, typename T, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>,
          typename Allocator = tbb::tbb_allocator<std::pair<const Key, T>> >
class concurrent_unordered_multimap
    : public concurrent_unordered_base<concurrent_unordered_map_traits<Key, T, Hash, KeyEqual, Allocator, true>>
{
    using traits_type = concurrent_unordered_map_traits<Key, T, Hash, KeyEqual, Allocator, true>;
    using base_type = concurrent_unordered_base<traits_type>;
public:
    using key_type = typename base_type::key_type;
    using mapped_type = T;
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

    // Include constructors of base type
    using base_type::base_type;

    using base_type::insert;

    template <typename P>
    typename std::enable_if<std::is_constructible<value_type, P&&>::value,
                            std::pair<iterator, bool>>::type insert( P&& value ) {
        return this->emplace(std::forward<P>(value));
    }

    template<typename P>
    typename std::enable_if<std::is_constructible<value_type, P&&>::value,
                            iterator>::type insert( const_iterator hint, P&& value ) {
        return this->emplace_hint(hint, std::forward<P&&>(value));
    }

    template <typename OtherHash, typename OtherKeyEqual>
    void merge( concurrent_unordered_map<key_type, mapped_type, OtherHash, OtherKeyEqual, allocator_type>& source ) {
        this->internal_merge(source);
    }

    template <typename OtherHash, typename OtherKeyEqual>
    void merge( concurrent_unordered_map<key_type, mapped_type, OtherHash, OtherKeyEqual, allocator_type>&& source ) {
        this->internal_merge(std::move(source));
    }

    template <typename OtherHash, typename OtherKeyEqual>
    void merge( concurrent_unordered_multimap<key_type, mapped_type, OtherHash, OtherKeyEqual, allocator_type>& source ) {
        this->internal_merge(source);
    }

    template <typename OtherHash, typename OtherKeyEqual>
    void merge( concurrent_unordered_multimap<key_type, mapped_type, OtherHash, OtherKeyEqual, allocator_type>&& source ) {
        this->internal_merge(std::move(source));
    }
}; // class concurrent_unordered_multimap

#if __TBB_CPP17_DEDUCTION_GUIDES_PRESENT

// Deduction guide for copy constructor with additional allocator parameter
// TODO: Investigate why implicit deduction guides are not generated
//       for concurrent_unordered_multimap( const concurrent_unordered_multimap&, Allocator).
template <typename Key, typename Val, typename Hash, typename KeyEq, typename Allocator>
concurrent_unordered_multimap( const concurrent_unordered_multimap<Key, Val, Hash, KeyEq, Allocator>&, const Allocator& )
-> concurrent_unordered_multimap<Key, Val, Hash, KeyEq, Allocator>;

// Deduction guide for the constructor from two iterators
template <typename I>
concurrent_unordered_multimap( I, I )
-> cu_map_type<concurrent_unordered_multimap, iterator_key_t<I>, iterator_mapped_t<I>>;

// Deduction guide for the constructor from two iterators and hasher/equal/allocator
template <typename I, typename... Args>
concurrent_unordered_multimap( I, I, std::size_t, Args... )
-> cu_map_type<concurrent_unordered_multimap, iterator_key_t<I>, iterator_mapped_t<I>, Args...>;

// Deduction guide for the constructor from an initializer_list
template <typename Key, typename Mapped>
concurrent_unordered_multimap( std::initializer_list<std::pair<const Key, Mapped>> )
-> cu_map_type<concurrent_unordered_multimap, Key, Mapped>;

// Deduction guide for the constructor from an initializer_list and hasher/equal/allocator
template <typename Key, typename Mapped, typename... Args>
concurrent_unordered_multimap( std::initializer_list<std::pair<const Key, Mapped>>, std::size_t, Args... )
-> cu_map_type<concurrent_unordered_multimap, Key, Mapped, Args...>;

#endif // __TBB_CPP17_DEDUCTION_GUIDES_PRESENT

template <typename Key, typename T, typename Hash, typename KeyEqual, typename Allocator>
void swap( concurrent_unordered_multimap<Key, T, Hash, KeyEqual, Allocator>& lhs,
           concurrent_unordered_multimap<Key, T, Hash, KeyEqual, Allocator>& rhs ) {
    lhs.swap(rhs);
}

} // namespace d1
} // namespace detail

inline namespace v1 {

using detail::d1::concurrent_unordered_map;
using detail::d1::concurrent_unordered_multimap;
using detail::split;

} // inline namespace v1
} // namespace tbb

#endif // __TBB_concurrent_unordered_map_H
