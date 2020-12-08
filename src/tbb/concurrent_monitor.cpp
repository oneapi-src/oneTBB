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

#include "concurrent_monitor.h"

namespace tbb {
namespace detail {
namespace r1 {

concurrent_monitor::~concurrent_monitor() {
    abort_all();
    __TBB_ASSERT(waitset_ec.empty(), "waitset not empty?");
}

void concurrent_monitor::cancel_wait( wait_node& node ) {
    // possible skipped wakeup will be pumped in the following prepare_wait()
    node.my_skipped_wakeup_flag = true;
    // try to remove node from waitset
    bool in_list = node.my_is_in_list.load(std::memory_order_relaxed);
    if (in_list) {
        tbb::spin_mutex::scoped_lock l(mutex_ec);
        if (node.my_is_in_list.load(std::memory_order_relaxed)) {
            waitset_ec.remove((base_list::base_node&)node);
            // node is removed from waitset, so there will be no wakeup
            node.my_is_in_list.store(false, std::memory_order_relaxed);
            node.my_skipped_wakeup_flag = false;
        }
    }
}

void concurrent_monitor::notify_one_relaxed() {
    if (waitset_ec.empty()) {
        return;
    }

    base_node* n;
    const base_node* end = waitset_ec.end();
    {
        tbb::spin_mutex::scoped_lock l(mutex_ec);
        epoch.store(epoch.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
        n = waitset_ec.front();
        if (n != end) {
            waitset_ec.remove(*n);
            ++to_wait_node(n)->my_notifiers;
            to_wait_node(n)->my_is_in_list.store(false, std::memory_order_relaxed);
        }
    }

    if (n != end) {
        to_wait_node(n)->notify_impl();
        --to_wait_node(n)->my_notifiers;
    }
}

void concurrent_monitor::notify_all_relaxed() {
    if (waitset_ec.empty()) {
        return;
    }

    base_list temp;
    const base_node* end;
    {
        tbb::spin_mutex::scoped_lock l(mutex_ec);
        epoch.store(epoch.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
        waitset_ec.flush_to(temp);
        end = temp.end();
        for (base_node* n = temp.front(); n != end; n = n->next) {
            ++to_wait_node(n)->my_notifiers;
            to_wait_node(n)->my_is_in_list.store(false, std::memory_order_relaxed);
        }
    }

    base_node* nxt;
    for (base_node* n = temp.front(); n != end; n=nxt) {
        nxt = n->next;
        to_wait_node(n)->notify_impl();
        --to_wait_node(n)->my_notifiers;
    }
#if TBB_USE_ASSERT
    temp.clear();
#endif
}

void concurrent_monitor::abort_all_relaxed() {
    if (waitset_ec.empty()) {
        return;
    }

    base_list temp;
    const base_node* end;
    {
        tbb::spin_mutex::scoped_lock l(mutex_ec);
        epoch.store(epoch.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
        waitset_ec.flush_to(temp);
        end = temp.end();
        for (base_node* n = temp.front(); n != end; n = n->next) {
            ++to_wait_node(n)->my_notifiers;
            to_wait_node(n)->my_is_in_list.store(false, std::memory_order_relaxed);
        }
    }

    base_node* nxt;
    for (base_node* n = temp.front(); n != end; n = nxt) {
        nxt = n->next;
        to_wait_node(n)->my_is_aborted = true;
        to_wait_node(n)->notify_impl();
        --to_wait_node(n)->my_notifiers;
    }
#if TBB_USE_ASSERT
    temp.clear();
#endif
}

} // namespace r1
} // namespace detail
} // namespace tbb

