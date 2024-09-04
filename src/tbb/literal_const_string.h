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

#ifndef __TBB_literal_const_string
#define __TBB_literal_const_string

#include "oneapi/tbb/detail/_config.h"
#include "oneapi/tbb/detail/_assert.h"

namespace tbb {
namespace detail {
//should we increase it here ?
namespace r1 {

class literal_const_string{
    const char* m_str;
    std::size_t m_sz;
public:
    template<std::size_t N>
    literal_const_string(const char (&s)[N]): m_str(s), m_sz(N-1){
        __TBB_ASSERT( s[N-1] == 0, "passed in char array should be a string, i.e. terminated with zero" );
    }

    std::size_t size() const {return m_sz;}

    //returned c-style string is guaranteed to be null terminated
    const char* c_str() const {return m_str;}
};

} // namespace r1
} // namespace detail
} // namespace tbb

#endif /* __TBB_literal_const_string */
