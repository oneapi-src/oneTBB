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

#include "thread_dispatcher.h"
#include "threading_control.h"


namespace tbb {
namespace detail {
namespace r1 {

bool governor::does_client_join_workers(const rml::tbb_client &client) {
    return ((const thread_dispatcher&)client).must_join_workers();
}

void thread_dispatcher::acknowledge_close_connection() {
    my_threading_control.destroy();
}

::rml::job* thread_dispatcher::create_one_job() {
    unsigned short index = ++my_first_unused_worker_idx;
    __TBB_ASSERT( index > 0, nullptr);
    ITT_THREAD_SET_NAME(_T("TBB Worker Thread"));
    // index serves as a hint decreasing conflicts between workers when they migrate between arenas
    thread_data* td = new(cache_aligned_allocate(sizeof(thread_data))) thread_data{ index, true };
    __TBB_ASSERT( index <= my_num_workers_hard_limit, nullptr);
    my_threading_control.register_thread(*td);
    return td;
}

} // namespace r1
} // namespace detail
} // namespace tbb
