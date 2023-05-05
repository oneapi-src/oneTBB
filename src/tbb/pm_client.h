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

#ifndef _TBB_pm_client_H
#define _TBB_pm_client_H

#include "arena.h"

namespace tbb {
namespace detail {
namespace r1 {

class pm_client {
public:
    pm_client(arena& a) : my_arena(a) {}

    //! The index in the array of per priority lists of arenas this object is in.
    unsigned priority_level() {
        return my_arena.priority_level();
    }

    bool is_top_priority() {
        return my_is_top_priority.load(std::memory_order_relaxed);
    }

    void set_top_priority(bool b) {
        return my_is_top_priority.store(b, std::memory_order_relaxed);
    }

    int min_workers() const {
        __TBB_ASSERT(my_min_workers >= 0, nullptr);
        return my_min_workers;
    }

    int max_workers() const {
        __TBB_ASSERT(my_max_workers >= 0, nullptr);
        return my_max_workers;
    }

    int update_request(int mandatory_delta, int workers_delta) {
        auto min_max_workers = my_arena.update_request(mandatory_delta, workers_delta);
        int delta = min_max_workers.second - my_max_workers;
        set_workers(min_max_workers.first, min_max_workers.second);
        return delta;
    }

protected:
    void set_workers(int mn_w, int mx_w) {
        __TBB_ASSERT(mn_w >= 0, nullptr);
        __TBB_ASSERT(mx_w >= 0, nullptr);
        my_min_workers = mn_w;
        my_max_workers = mx_w;
    }

    arena& my_arena;
    std::atomic<bool> my_is_top_priority{ false };
    int my_min_workers{0};
    int my_max_workers{0};
};

} // namespace r1
} // namespace detail
} // namespace tbb

#endif // _TBB_pm_client_H
