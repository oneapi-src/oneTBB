/*
    Copyright (c) 2024 Intel Corporation

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

#include "oneapi/tbb/detail/_config.h"
#include "oneapi/tbb/tbb_allocator.h"
#include "oneapi/tbb/task_group.h"
#include "governor.h"
#include "thread_data.h"


namespace tbb {
namespace detail {
namespace r1 {

void __TBB_EXPORTED_FUNC set_top_group_task(d1::task* t) {
    governor::get_thread_data()->my_task_group_tasks.push(t);
}

d1::task* __TBB_EXPORTED_FUNC get_top_group_task() {
    auto td = governor::get_thread_data();
    return td->my_task_group_tasks.empty() ? nullptr : td->my_task_group_tasks.top();
}

void __TBB_EXPORTED_FUNC remove_top_group_task() {
    governor::get_thread_data()->my_task_group_tasks.pop();
}

}
}
}
