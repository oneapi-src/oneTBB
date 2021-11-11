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

#ifndef __TBB_static_string
#define __TBB_static_string

#include "oneapi/tbb/detail/_config.h"

#include "c_string_view.h"
#include <array>
#include <cstring>
#include <algorithm> // for min
namespace tbb {
namespace detail {
//should we increase it here ?
namespace r1 {

template<std::size_t N>
class static_string{
    std::array<char, N> ar = {0};
    std::size_t m_size = 0;

public:
    std::size_t size()  const noexcept      { return m_size;}
    std::size_t max_size() const noexcept   { return ar.max_size();}

    const char* c_str() const noexcept{
        //__TBB_ASSERT(ar.max_size() == 0 ? m_size < (ar.max_size() - 1) : m_size == 0);
        //__TBB_ASSERT(ar.max_size() == 0 ? ar[m_size] == 0 : true);

        return ar.data();
    }

    static_string& operator+=(const c_string_view& csv) noexcept {
        std::strncat(ar.data(), csv.c_str(), ar.max_size() - m_size -1);
        m_size = std::min(m_size + csv.size(), ar.max_size() - 1);
        return *this;
    }

    static_string& operator=(const c_string_view& csv) noexcept {
        //clamp the result string to fit into the buffer
        auto symbols_to_write = std::min(csv.size(), ar.max_size() - 1);

        std::strncpy( ar.data(), csv.c_str(), symbols_to_write);
        //Ensure the string is null terminated
        ar.back() = 0;
        m_size = symbols_to_write;
        return *this;
    }

};
} // namespace r1
} // namespace detail
} // namespace tbb

#endif /* __TBB_static_string */
