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

#ifndef __TBB_global_control_H
#define __TBB_global_control_H

#include "detail/_config.h"

#include "detail/_assert.h"

#include <cstddef>

namespace tbb {
namespace detail {

namespace d1 {
    class global_control;
}

namespace r1 {
void __TBB_EXPORTED_FUNC create(d1::global_control&);
void __TBB_EXPORTED_FUNC destroy(d1::global_control&);
std::size_t __TBB_EXPORTED_FUNC global_control_active_value(int);
struct global_control_impl;
}

namespace d1 {

class global_control {
public:
    enum parameter {
        max_allowed_parallelism,
        thread_stack_size,
        parameter_max // insert new parameters above this point
    };

    global_control(parameter p, std::size_t value) :
        my_value(value), my_next(NULL), my_param(p) {
        __TBB_ASSERT(my_param < parameter_max, "Invalid parameter");
#if __TBB_WIN8UI_SUPPORT && (_WIN32_WINNT < 0x0A00)
        // For Windows 8 Store* apps it's impossible to set stack size
        if (p==thread_stack_size)
            return;
#elif __TBB_x86_64 && (_WIN32 || _WIN64)
        if (p==thread_stack_size)
            __TBB_ASSERT_RELEASE((unsigned)value == value, "Stack size is limited to unsigned int range");
#endif
        if (my_param==max_allowed_parallelism)
            __TBB_ASSERT_RELEASE(my_value>0, "max_allowed_parallelism cannot be 0.");
        r1::create(*this);
    }

    ~global_control() {
        __TBB_ASSERT(my_param < parameter_max, "Invalid parameter. Probably the object was corrupted.");
#if __TBB_WIN8UI_SUPPORT && (_WIN32_WINNT < 0x0A00)
        // For Windows 8 Store* apps it's impossible to set stack size
        if (my_param==thread_stack_size)
            return;
#endif
        r1::destroy(*this);
    }

    static std::size_t active_value(parameter p) {
        __TBB_ASSERT(p < parameter_max, "Invalid parameter");
        return r1::global_control_active_value((int)p);
    }
private:
    std::size_t   my_value;
    global_control *my_next;
    parameter my_param;

    friend struct r1::global_control_impl;
};

} // namespace d1
} // namespace detail

inline namespace v1 {
using detail::d1::global_control;
} // namespace v1

} // namespace tbb

#endif // __TBB_global_control_H
