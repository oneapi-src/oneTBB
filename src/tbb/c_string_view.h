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

#ifndef __TBB_c_string_view
#define __TBB_c_string_view

// Support for dynamic loading entry points from other shared libraries.

#include "oneapi/tbb/detail/_config.h"

namespace tbb {
namespace detail {
//should we increase it here ?
namespace r1 {

class c_string_view{
    const char* str;
    std::size_t sz;
public:
    template<std::size_t N>
    c_string_view(const char (&s)[N]): str(s), sz(N-1){}
    c_string_view(const char* s, std::size_t s_size) : str(s), sz{s_size} {};

    std::size_t size() const { return sz;}

    const char* c_str() const {return str;}
};

} // namespace r1
} // namespace detail
} // namespace tbb

#endif /* __TBB_c_string_view */
