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

// Support for dynamic loading entry points from other shared libraries.

#include "oneapi/tbb/detail/_config.h"

namespace tbb {
namespace detail {
//should we increase it here ?
namespace r1 {

class literal_const_string{
    const char* str;
    std::size_t sz;
public:
    template<std::size_t N>
    literal_const_string(const char (&s)[N]): str(s), sz(N-1){}

    std::size_t size() const {return sz;}

    //returned c-style string is guaranteed to be null terminated
    const char* c_str() const {return str;}
};

} // namespace r1
} // namespace detail
} // namespace tbb

#endif /* __TBB_literal_const_string */
