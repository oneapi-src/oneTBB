/*
    Copyright (c) 2005-2023 Intel Corporation

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

#ifndef _TBB_arena_fixed_size_H
#define _TBB_arena_fixed_size_H

#include <atomic>
#include <cstring>

#include "oneapi/tbb/detail/_task.h"
#include "oneapi/tbb/detail/_utils.h"
#include "oneapi/tbb/spin_mutex.h"

#include "scheduler_common.h"
#include "intrusive_list.h"
#include "task_stream.h"
#include "arena_slot.h"
#include "rml_tbb.h"
#include "mailbox.h"
#include "governor.h"
#include "concurrent_monitor.h"
#include "observer_proxy.h"
#include "thread_control_monitor.h"
#include "threading_control_client.h"

namespace tbb {
namespace prototype {
namespace r1 {

class task_dispatcher;
class task_group_context;
class threading_control;
class allocate_root_with_context_proxy;

struct stack_anchor_type {
    stack_anchor_type() = default;
    stack_anchor_type(const stack_anchor_type&) = delete;
};


class arena_fixed_size: public tbb::detail::r1::arena
{
public:

    //! Constructor
    arena_fixed_size(tbb::detail::r1::threading_control* control, unsigned max_num_workers, unsigned num_reserved_slots, unsigned priority_level);
    //! Allocate an instance of arena.
    static arena_fixed_size& allocate_arena(tbb::detail::r1::threading_control* control, unsigned num_slots, unsigned num_reserved_slots,
        unsigned priority_level);

    static arena_fixed_size& create(tbb::detail::r1::threading_control* control, unsigned num_slots, unsigned num_reserved_slots, unsigned arena_priority_level, tbb::detail::d1::constraints constraints = tbb::detail::d1::constraints{});


}; // class arena_fixed_size

} //namespace r1
} // namespace prototype
} // namespace tbb

#endif /* _TBB_arena_fixed_size_H */
