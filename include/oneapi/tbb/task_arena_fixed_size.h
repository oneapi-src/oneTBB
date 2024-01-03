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

#ifndef __TBB_task_arena_fixed_size_H
#define __TBB_task_arena_fixed_size_H

#include "detail/_config.h"

#include "detail/_aligned_space.h"
#include "detail/_attach.h"
#include "detail/_exception.h"
#include "detail/_namespace_injection.h"
#include "detail/_small_object_pool.h"
#include "detail/_task.h"
#include "detail/_utils.h"

#include "detail/_task_handle.h"
#include "oneapi/tbb/task_arena.h"
#include <iostream>

namespace tbb {
namespace prototype {

namespace d1 {
class task_arena_fixed_size_base;
} // namespace d1

namespace r1 {
class arena_fixed_size;
struct task_arena_fixed_size_impl;

TBB_EXPORT void __TBB_EXPORTED_FUNC initialize(d1::task_arena_fixed_size_base&);
TBB_EXPORT void __TBB_EXPORTED_FUNC terminate(d1::task_arena_fixed_size_base&);

} // namespace r1 

namespace d1 {

//! One-time initialization states
enum class do_once_state {
    uninitialized = 0,      ///< No execution attempts have been undertaken yet
    pending,                ///< A thread is executing associated do-once routine
    executed,               ///< Do-once routine has been executed
    initialized = executed  ///< Convenience alias
};

using slot_id = unsigned short;
static constexpr unsigned num_priority_levels = 3;
static constexpr int priority_stride = INT_MAX / (num_priority_levels + 1);
class task_arena_fixed_size_base {
    friend struct r1::task_arena_fixed_size_impl;
public:
    enum class priority : int {
        low = 1 * priority_stride,
        normal = 2 * priority_stride,
        high = 3 * priority_stride
    };
protected:

    std::atomic<do_once_state> my_initialization_state;

    //! nullptr if not currently initialized.
    std::atomic<r1::arena_fixed_size*> my_arena;
    static_assert(sizeof(std::atomic<r1::arena_fixed_size*>) == sizeof(r1::arena_fixed_size*),
        "To preserve backward compatibility we need the equal size of an atomic pointer and a pointer");
    //! Concurrency level for deferred initialization
    int my_max_concurrency;

    //! Reserved slots for external threads
    unsigned my_num_reserved_slots;

    //! Arena priority
    priority my_priority;

    //! The NUMA node index to which the arena will be attached
    numa_node_id my_numa_id;

    //! The core type index to which arena will be attached
    core_type_id my_core_type;

    //! Number of threads per core
    int my_max_threads_per_core;


    task_arena_fixed_size_base(int max_concurrency, unsigned reserved_for_masters, priority a_priority)
        : my_initialization_state(do_once_state::uninitialized)
        , my_arena(nullptr)
        , my_max_concurrency(max_concurrency)
        , my_num_reserved_slots(reserved_for_masters)
        , my_priority(a_priority)
        , my_numa_id(automatic)
        , my_core_type(automatic)
        , my_max_threads_per_core(automatic)
    {}

public:
    //! Typedef for number of threads that is automatic.
    static const int automatic = -1;
    static const int not_initialized = -2;
};


class task_arena_fixed_size : public task_arena_fixed_size_base {

    void mark_initialized() {
        __TBB_ASSERT( my_arena.load(std::memory_order_relaxed), "task_arena_fixed_size initialization is incomplete" );
        my_initialization_state.store(do_once_state::initialized, std::memory_order_release);
    }

    template<typename R, typename F>
    R execute_impl(F& f) {
        initialize();
        tbb::detail::d1::task_arena_function<F, R> func(f);
        //r1::execute(*this, func);
        std::cout << "Implement Arena Fixed size execute\n";
        return func.consume_result();
    }
public:
    //! Creates task_arena_fixed_size with certain concurrency limits
    /** Sets up settings only, real construction is deferred till the first method invocation
     *  @arg max_concurrency specifies total number of slots in arena where threads work
     *  @arg reserved_for_masters specifies number of slots to be used by external threads only.
     *       Value of 1 is default and reflects behavior of implicit arenas.
     **/
    task_arena_fixed_size(int max_concurrency_ = automatic, unsigned reserved_for_masters = 1,
               priority a_priority = priority::normal)
        : task_arena_fixed_size_base(max_concurrency_, reserved_for_masters, a_priority)
    {}


    //! Forces allocation of the resources for the task_arena_fixed_size as specified in constructor arguments
    void initialize() {
        //tbb::detail::atomic_do_once([this]{ r1::initialize(*this); }, my_initialization_state);
        std::cout << "Fix initialize \n";
    }

    //! Overrides concurrency level and forces initialization of internal representation
    void initialize(int max_concurrency_, unsigned reserved_for_masters = 1,
                    priority a_priority = priority::normal)
    {
        __TBB_ASSERT(!my_arena.load(std::memory_order_relaxed), "Impossible to modify settings of an already initialized task_arena_fixed_size");
        if( !is_active() ) {
            my_max_concurrency = max_concurrency_;
            my_num_reserved_slots = reserved_for_masters;
            my_priority = a_priority;
            r1::initialize(*this);
            mark_initialized();
        }
    }



    //! Removes the reference to the internal arena representation.
    //! Not thread safe wrt concurrent invocations of other methods.
    void terminate() {
        if( is_active() ) {
            r1::terminate(*this);
            my_initialization_state.store(do_once_state::uninitialized, std::memory_order_relaxed);
        }
    }

    //! Removes the reference to the internal arena representation, and destroys the external object.
    //! Not thread safe wrt concurrent invocations of other methods.
    ~task_arena_fixed_size() {
        terminate();
    }

    //! Returns true if the arena is active (initialized); false otherwise.
    //! The name was chosen to match a task_scheduler_init method with the same semantics.
    bool is_active() const {
        return my_initialization_state.load(std::memory_order_acquire) == do_once_state::initialized;
    }

    //! Enqueues a task into the arena to process a functor, and immediately returns.
    //! Does not require the calling thread to join the arena

 
    //! Joins the arena and executes a mutable functor, then returns
    //! If not possible to join, wraps the functor into a task, enqueues it and waits for task completion
    //! Can decrement the arena demand for workers, causing a worker to leave and free a slot to the calling thread
    //! Since C++11, the method returns the value returned by functor (prior to C++11 it returns void).
    template<typename F>
    auto execute(F&& f) -> decltype(f()) {
        return execute_impl<decltype(f())>(f);
    }

#if __TBB_EXTRA_DEBUG
    //! Returns my_num_reserved_slots
    int debug_reserved_slots() const {
        // Handle special cases inside the library
        return my_num_reserved_slots;
    }

    //! Returns my_max_concurrency
    int debug_max_concurrency() const {
        // Handle special cases inside the library
        return my_max_concurrency;
    }

    //! Wait for all work in the arena to be completed
    //! Even submitted by other application threads
    //! Joins arena if/when possible (in the same way as execute())
    void debug_wait_until_empty() {
        initialize();
        r1::wait(*this);
    }
#endif //__TBB_EXTRA_DEBUG

    //! Returns the maximal number of threads that can work inside the arena
    int max_concurrency() const {
        // Handle special cases inside the library
        return (my_max_concurrency > 1) ? my_max_concurrency : 0/*r1::max_concurrency(this)  Implement fixed size max concurrency*/;
    }

};



//! Returns the index, aka slot number, of the calling thread in its current arena
inline int current_thread_index() {
    //slot_id idx = r1::execution_slot(nullptr);
    //slot_id idx = tbb::detail::d1::execution_slot(nullptr);
    slot_id idx = 1;
    return idx == slot_id(-1) ? task_arena_fixed_size_base::not_initialized : int(idx);
}


//! Returns the maximal number of threads that can work inside the arena
inline int max_concurrency() {
    return tbb::detail::r1::max_concurrency(nullptr);
}

} //namespace d1
} // namespace prototype
} // namespace tbb
#endif /* __TBB_task_arena_fixed_size_H */
