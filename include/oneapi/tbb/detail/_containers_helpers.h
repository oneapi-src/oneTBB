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

#ifndef __TBB_detail__containers_helpers_H
#define __TBB_detail__containers_helpers_H

#include "_template_helpers.h"
#include "_allocator_traits.h"
#include <type_traits>
#include <memory>
#include <functional>

namespace tbb {
namespace detail {
inline namespace d0 {

template <typename Compare, typename = void>
struct comp_is_transparent : std::false_type {};

template <typename Compare>
struct comp_is_transparent<Compare, tbb::detail::void_t<typename Compare::is_transparent>> : std::true_type {};

template <typename Key, typename Hasher, typename KeyEqual, typename = void >
struct has_transparent_key_equal : std::false_type { using type = KeyEqual; };

template <typename Key, typename Hasher, typename KeyEqual>
struct has_transparent_key_equal<Key, Hasher, KeyEqual, tbb::detail::void_t<typename Hasher::transparent_key_equal>> : std::true_type {
    using type = typename Hasher::transparent_key_equal;
    static_assert(comp_is_transparent<type>::value, "Hash::transparent_key_equal::is_transparent is not valid or does not denote a type.");
    static_assert((std::is_same<KeyEqual, std::equal_to<Key>>::value ||
        std::is_same<typename Hasher::transparent_key_equal, KeyEqual>::value), "KeyEqual is a different type than equal_to<Key> or Hash::transparent_key_equal.");
 };

template <typename IteratorCategory, typename Iterator>
using is_iterator = typename std::is_base_of<IteratorCategory, typename std::iterator_traits<Iterator>::iterator_category>::type;

} // inline namespace d0
} // namespace detail
} // namespace tbb

#endif // __TBB_detail__containers_helpers_H
