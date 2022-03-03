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

#include <array>
#include <cstring>
#include <algorithm> // for std::min, std::fill

#include "literal_const_string.h"

namespace tbb {
namespace detail {
//should we increase it here ?
namespace r1 {

template<std::size_t N>
class static_string{

    static_assert(N > 0,"no point in creating static_string with no storage allocated");
    std::array<char, N + 1> ar = {0};
    std::size_t m_size = 0;

public:

    //While initialization of ar can be omitted due to C++ rules, gcc 4.8 insist on it
    static_string(std::size_t sz = 0) : ar{0}, m_size (sz) {};


    std::size_t size()  const noexcept      { return m_size;}
    std::size_t max_size() const noexcept   { return ar.max_size() -1;}

    //FIXME: cover with tests
    void clear() noexcept {
        m_size = 0;
        ar[m_size] = 0;
    }

    const char* c_str() const noexcept{
        //__TBB_ASSERT( m_size < (ar.max_size() - 1) );
        //__TBB_ASSERT( ar[m_size] == 0 );

        return ar.data();
    }

    //FIXME: cover this with tests
    char* data() noexcept{
        //__TBB_ASSERT( m_size < (ar.max_size() - 1) );
        //__TBB_ASSERT( ar[m_size] == 0 );

        return ar.data();
    }

    //FIXME: cover this with tests
    void resize(size_t new_size, char ch = 0) noexcept{
        //__TBB_ASSERT( m_size < (ar.max_size() - 1) );

        if ( new_size > m_size ) {
            std::fill_n(ar.begin() + m_size, new_size - m_size, ch);
            new_size = std::min(new_size, this->max_size());
        }


        m_size = new_size;
        ar[m_size] = 0;
    }

    static_string& append(const char* s,  std::size_t size) noexcept {
        //clamp the result string to fit into the buffer
        //__TBB_ASSERT( m_size < (ar.max_size() - 1) );
        auto symbols_to_write = std::min(size, ar.max_size() - m_size - 1);

        std::memcpy(ar.data() + m_size, s, symbols_to_write);
        m_size += symbols_to_write;
        //Ensure the string is null terminated
        ar[m_size] = 0;

        return *this;
    }

    static_string& operator+=(const literal_const_string& csv) noexcept {
        this->append(csv.c_str(), csv.size());
        return *this;
    }

    static_string& operator=(const literal_const_string& csv) noexcept {
        clear();
        *this += csv;
        return *this;
    }

    //FIXME: cover with tests
    template<std::size_t N1>
    static_string& operator=(const static_string<N1>& ss) noexcept {
        clear();
        this->append(ss.c_str(), ss.size());
        return *this;
    }


};
} // namespace r1
} // namespace detail
} // namespace tbb

#endif /* __TBB_static_string */
