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

#include "tbb/detail/_config.h"
#include "tbb/detail/_template_helpers.h"

#include "tbb/global_control.h"
#include "tbb/spin_mutex.h"

#include "governor.h"
#include "market.h"
#include "misc.h"

#include <atomic>

namespace tbb {
namespace detail {
namespace r1 {

class control_storage {
    friend struct global_control_impl;
    friend std::size_t global_control_active_value(int);
protected:
    std::size_t my_active_value;
    std::atomic<global_control*> my_head;
    spin_mutex my_list_mutex;
public:
    virtual std::size_t default_value() const = 0;
    virtual void apply_active() const {}
    virtual bool is_first_arg_preferred(std::size_t a, std::size_t b) const {
        return a>b; // prefer max by default
    }
    virtual std::size_t active_value() const {
        return my_head.load(std::memory_order_acquire)? my_active_value : default_value();
    }
};

class alignas(max_nfs_size) allowed_parallelism_control : public control_storage {
    virtual std::size_t default_value() const override {
        return max(1U, governor::default_num_threads());
    }
    virtual bool is_first_arg_preferred(std::size_t a, std::size_t b) const override {
        return a<b; // prefer min allowed parallelism
    }
    virtual void apply_active() const override {
        __TBB_ASSERT( my_active_value>=1, NULL );
        // -1 to take master into account
        market::set_active_num_workers( my_active_value-1 );
    }
    virtual std::size_t active_value() const override {
/* Reading of my_active_value is not synchronized with possible updating
   of my_head by other thread. It's ok, as value of my_active_value became
   not invalid, just obsolete. */
        if (!my_head.load(std::memory_order_acquire))
            return default_value();
        // non-zero, if market is active
        const std::size_t workers = market::max_num_workers();
        // We can't exceed market's maximal number of workers.
        // +1 to take master into account
        return workers? min(workers+1, my_active_value): my_active_value;
    }
public:
    std::size_t active_value_if_present() const {
        return my_head.load(std::memory_order_acquire)? my_active_value : 0;
    }
};

class alignas(max_nfs_size) stack_size_control : public control_storage {
    virtual std::size_t default_value() const override {
        return ThreadStackSize;
    }
    virtual void apply_active() const override {
#if __TBB_WIN8UI_SUPPORT && (_WIN32_WINNT < 0x0A00)
        __TBB_ASSERT( false, "For Windows 8 Store* apps we must not set stack size" );
#endif
    }
};

static allowed_parallelism_control allowed_parallelism_ctl;
static stack_size_control stack_size_ctl;

static control_storage *controls[] = {&allowed_parallelism_ctl, &stack_size_ctl};

unsigned market::app_parallelism_limit() {
    return allowed_parallelism_ctl.active_value_if_present();
}

struct global_control_impl {
    static void create(d1::global_control& gc) {
        __TBB_ASSERT_RELEASE(gc.my_param < global_control::parameter_max, NULL);
        control_storage* const c = controls[gc.my_param];

        spin_mutex::scoped_lock lock(c->my_list_mutex);
        if (!c->my_head.load(std::memory_order_acquire) || c->is_first_arg_preferred(gc.my_value, c->my_active_value)) {
            c->my_active_value = gc.my_value;
            // to guarantee that apply_active() is called with current active value,
            // calls it here and in internal_destroy() under my_list_mutex
            c->apply_active();
        }
        gc.my_next = c->my_head.load(std::memory_order_acquire);
        // publish my_head, at this point my_active_value must be valid
        c->my_head.store(&gc, std::memory_order_release);
    }

    static void destroy(d1::global_control& gc) {
        global_control* prev = 0;

        __TBB_ASSERT_RELEASE(gc.my_param < global_control::parameter_max, NULL);
        control_storage* const c = controls[gc.my_param];
        __TBB_ASSERT(c->my_head.load(std::memory_order_relaxed), NULL);

        // Concurrent reading and changing global parameter is possible.
        // In this case, my_active_value may not match current state of parameters.
        // This is OK because:
        // 1) my_active_value is either current or previous
        // 2) my_active_value is current on internal_destroy leave
        spin_mutex::scoped_lock lock(c->my_list_mutex);
        std::size_t new_active = (std::size_t) - 1, old_active = c->my_active_value;

        if (c->my_head.load(std::memory_order_acquire) != &gc)
            new_active = c->my_head.load(std::memory_order_acquire)->my_value;
        else if (c->my_head.load(std::memory_order_acquire)->my_next)
            new_active = c->my_head.load(std::memory_order_acquire)->my_next->my_value;
        // if there is only one element, new_active will be set later
        for (global_control* curr = c->my_head.load(std::memory_order_acquire); curr; prev = curr, curr = curr->my_next) {
            if (curr == &gc) {
                if (prev) {
                    prev->my_next = gc.my_next;
                } else {
                    c->my_head.store(gc.my_next, std::memory_order_release);
                }
            } else {
                if (c->is_first_arg_preferred(curr->my_value, new_active)) {
                    new_active = curr->my_value;
                }
            }
        }

        if (!c->my_head.load(std::memory_order_acquire)) {
            __TBB_ASSERT(new_active == (std::size_t) - 1, NULL);
            new_active = c->default_value();
        }
        if (new_active != old_active) {
            c->my_active_value = new_active;
            c->apply_active();
        }
    }
};

void __TBB_EXPORTED_FUNC create(d1::global_control& gc) {
    global_control_impl::create(gc);
}
void __TBB_EXPORTED_FUNC destroy(d1::global_control& gc) {
    global_control_impl::destroy(gc);
}

std::size_t __TBB_EXPORTED_FUNC global_control_active_value(int param) {
    __TBB_ASSERT_RELEASE(param < global_control::parameter_max, NULL);
    return controls[param]->active_value();
}

} // namespace r1
} // namespace detail
} // namespace tbb
