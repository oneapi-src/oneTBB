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

#ifndef __TBB_concurrent_monitor_H
#define __TBB_concurrent_monitor_H

#include "oneapi/tbb/spin_mutex.h"
#include "oneapi/tbb/detail/_exception.h"
#include "oneapi/tbb/detail/_aligned_space.h"
#include "oneapi/tbb/detail/_template_helpers.h"
#include "scheduler_common.h"

#include "semaphore.h"

#include <atomic>

namespace tbb {
namespace detail {
namespace r1 {

//! Circular doubly-linked list with sentinel
/** head.next points to the front and head.prev points to the back */
class circular_doubly_linked_list_with_sentinel : no_copy {
public:
    struct base_node {
        base_node* next;
        base_node* prev;
        explicit base_node() : next((base_node*)(uintptr_t)0xcdcdcdcd), prev((base_node*)(uintptr_t)0xcdcdcdcd) {}
    };

    // ctor
    circular_doubly_linked_list_with_sentinel() { clear(); }
    // dtor
    ~circular_doubly_linked_list_with_sentinel() {
        __TBB_ASSERT(head.next == &head && head.prev == &head, "the list is not empty");
    }

    inline std::size_t size() const { return count.load(std::memory_order_relaxed); }
    inline bool empty() const { return size() == 0; }
    inline base_node* front() const { return head.next; }
    inline base_node* last() const { return head.prev; }
    inline const base_node* end() const { return &head; }

    //! add to the back of the list
    inline void add( base_node* n ) {
        count.store(count.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
        n->prev = head.prev;
        n->next = &head;
        head.prev->next = n;
        head.prev = n;
    }

    //! remove node 'n'
    inline void remove( base_node& n ) {
        __TBB_ASSERT(count.load(std::memory_order_relaxed) > 0, "attempt to remove an item from an empty list");
        count.store(count.load( std::memory_order_relaxed ) - 1, std::memory_order_relaxed);
        n.prev->next = n.next;
        n.next->prev = n.prev;
    }

    //! move all elements to 'lst' and initialize the 'this' list
    inline void flush_to( circular_doubly_linked_list_with_sentinel& lst ) {
        const std::size_t l_count = size();
        if (l_count > 0) {
            lst.count.store(l_count, std::memory_order_relaxed);
            lst.head.next = head.next;
            lst.head.prev = head.prev;
            head.next->prev = &lst.head;
            head.prev->next = &lst.head;
            clear();
        }
    }

    void clear() {
        head.next = &head;
        head.prev = &head;
        count.store(0, std::memory_order_relaxed);
    }
private:
    std::atomic<std::size_t> count;
    base_node head;
};

using base_list = circular_doubly_linked_list_with_sentinel;
using base_node = circular_doubly_linked_list_with_sentinel::base_node;

class concurrent_monitor;

class wait_node : public base_node {
public:
    wait_node() = default;

    virtual ~wait_node() = default;

    bool is_ready() {
        return my_ready_flag.load(std::memory_order_acquire) == node_state::ready;
    }

    virtual void init() = 0;
    virtual void wait_impl() = 0;
    virtual void notify_impl() = 0;

protected:
    void wait_notifiers() {
        // We must wait unconditionally, because access to the node might be made until flag set to false.
        spin_wait_until_eq(my_notifiers, 0);
    }

    enum class node_state : unsigned {
        not_ready, // node is not ready for notification
        ready,     // node is ready for notification
        notified   // missed notify, possible only for resumble_node
    };

    friend class concurrent_monitor;
    friend class thread_data;
    std::atomic<node_state> my_ready_flag{node_state::not_ready};
    std::atomic<int> my_notifiers{0};
    std::atomic<bool> my_is_in_list{false};
    bool my_skipped_wakeup_flag{false};
    bool my_is_aborted{false};
    unsigned my_epoch{0};
};

struct extended_context {
    std::uintptr_t uniq_ctx;
    std::uintptr_t arena_ctx;
};

template <typename T>
class node_with_context : public wait_node {
    friend class concurrent_monitor;
    T my_context;
};

template <typename T>
class sleep_node : public node_with_context<T> {
public:
    // Make it virtual due to Intel Compiler warning
    virtual ~sleep_node() {
        if (this->is_ready()) {
            this->wait_notifiers();
            semaphore().~binary_semaphore();
        }
    }

    binary_semaphore& semaphore() { return *sema.begin(); }

    void init() override {
        new (sema.begin()) binary_semaphore;
        this->my_ready_flag.store(node_state::ready, std::memory_order_relaxed);
    }

    virtual void wait_impl() override {
        __TBB_ASSERT(this->my_ready_flag.load(std::memory_order_relaxed) == node_state::ready,
            "Use of commit_wait() without prior prepare_wait()");
        semaphore().P();
        __TBB_ASSERT(!this->my_is_in_list.load(std::memory_order_relaxed), "Still in the queue?");
        if (this->my_is_aborted)
            throw_exception(exception_id::user_abort);
    }

    void notify_impl() override {
        semaphore().V();
    }

private:
    using node_state = wait_node::node_state;
    tbb::detail::aligned_space<binary_semaphore> sema;
};

#if __TBB_RESUMABLE_TASKS
class resume_node : public node_with_context<extended_context> {
public:
    resume_node(execution_data_ext& ed_ext, task_dispatcher& target)
        : my_curr_dispatcher(*ed_ext.task_disp), my_target_dispatcher(target)
        , my_suspend_point(my_curr_dispatcher.get_suspend_point())
    {}

    // Make it virtual due to Intel Compiler warning
    virtual ~resume_node() {
        this->wait_notifiers();
    }

    void init() override {}

    virtual void wait_impl() override {
        __TBB_ASSERT(this->my_ready_flag.load(std::memory_order_relaxed) != node_state::ready,
            "Ready flag should be set only on a new stack.");
        my_curr_dispatcher.resume(my_target_dispatcher);
        __TBB_ASSERT(!this->my_is_in_list.load(std::memory_order_relaxed), "Still in the queue?");
        if (this->my_is_aborted)
            throw_exception(exception_id::user_abort);
    }

    void notify_impl() override {
        resume(my_suspend_point);
    }

private:
    using node_state = wait_node::node_state;
    friend class thread_data;
    task_dispatcher& my_curr_dispatcher;
    task_dispatcher& my_target_dispatcher;
    suspend_point_type* my_suspend_point;
};
#endif // __TBB_RESUMABLE_TASKS

//! concurrent_monitor
/** fine-grained concurrent_monitor implementation */
class concurrent_monitor : no_copy {
public:
    /** per-thread descriptor for concurrent_monitor */
    using thread_context = sleep_node<std::uintptr_t>;
    using extended_thread_context = sleep_node<extended_context>;
#if __TBB_RESUMABLE_TASKS
    using resume_context = resume_node;
#endif

    //! ctor
    concurrent_monitor() : epoch{} {}

    //! dtor
    ~concurrent_monitor() ;

    //! prepare wait by inserting 'thr' into the wait queue
    template <typename T>
    void prepare_wait( node_with_context<T>& node, T ctx = T{} ) {
        if (!node.is_ready()) {
            node.init();
        }
        // this is good place to pump previous skipped wakeup
        else if (node.my_skipped_wakeup_flag) {
            node.my_skipped_wakeup_flag = false;
            node.wait_impl();
        }

        node.my_context = ctx;
        node.my_is_in_list.store(true, std::memory_order_relaxed);

        {
            tbb::spin_mutex::scoped_lock l(mutex_ec);
            node.my_epoch = epoch.load(std::memory_order_relaxed);
            waitset_ec.add((base_list::base_node*)&node);
        }
        atomic_fence(std::memory_order_seq_cst);
    }

    //! Commit wait if event count has not changed; otherwise, cancel wait.
    /** Returns true if committed, false if canceled. */
    template <typename T>
    inline bool commit_wait( node_with_context<T>& node ) {
        const bool do_it = node.my_epoch == epoch.load(std::memory_order_relaxed);
        // this check is just an optimization
        if (do_it) {
           node.wait_impl();
        } else {
            cancel_wait( node );
        }
        return do_it;
    }

    //! Cancel the wait. Removes the thread from the wait queue if not removed yet.
    void cancel_wait( wait_node& node ) ;

    //! Notify one thread about the event
    void notify_one() {
        atomic_fence(std::memory_order_seq_cst);
        notify_one_relaxed();
    }

    //! Notify one thread about the event. Relaxed version.
    void notify_one_relaxed();

    //! Notify all waiting threads of the event
    void notify_all() {
        atomic_fence(std::memory_order_seq_cst);
        notify_all_relaxed();
    }

    // ! Notify all waiting threads of the event; Relaxed version
    void notify_all_relaxed();

    //! Notify waiting threads of the event that satisfies the given predicate
    template<typename P> 
    void notify( const P& predicate ) {
        atomic_fence(std::memory_order_seq_cst);
        notify_relaxed( predicate );
    }

    //! Notify waiting threads of the event that satisfies the given predicate; Relaxed version
    template<typename P>
    void notify_relaxed( const P& predicate );

    //! Abort any sleeping threads at the time of the call
    void abort_all() {
        atomic_fence( std::memory_order_seq_cst );
        abort_all_relaxed();
    }

    //! Abort any sleeping threads at the time of the call; Relaxed version
    void abort_all_relaxed();

private:
    tbb::spin_mutex mutex_ec;
    base_list waitset_ec;
    std::atomic<unsigned> epoch;

    wait_node* to_wait_node( base_node* node ) { return static_cast<wait_node*>(node); }
};

template<typename P>
void concurrent_monitor::notify_relaxed( const P& predicate ) {
    if (waitset_ec.empty()) {
        return;
    }

    base_list temp;
    base_node* nxt;
    const base_node* end = waitset_ec.end();
    {
        tbb::spin_mutex::scoped_lock l(mutex_ec);
        epoch.store(epoch.load( std::memory_order_relaxed ) + 1, std::memory_order_relaxed);
        for (base_node* n = waitset_ec.last(); n != end; n = nxt) {
            nxt = n->prev;
            using context_type = argument_type_of<P>;
            using state = wait_node::node_state;
            auto* node = static_cast<node_with_context<context_type>*>(n);
            state expected = state::not_ready;
            if (predicate(node->my_context) && (node->is_ready() ||
#if defined(__INTEL_COMPILER) && __INTEL_COMPILER <= 1910
            !((std::atomic<unsigned>&)node->my_ready_flag).compare_exchange_strong((unsigned&)expected, (unsigned)state::notified)))
#else
            !node->my_ready_flag.compare_exchange_strong(expected, state::notified)))
#endif
            {
                waitset_ec.remove(*n);
                ++node->my_notifiers;
                node->my_is_in_list.store(false, std::memory_order_relaxed);
                temp.add(n);
            }
        }
    }

    end = temp.end();
    for (base_node* n=temp.front(); n != end; n = nxt) {
        nxt = n->next;
        to_wait_node(n)->notify_impl();
        --to_wait_node(n)->my_notifiers;
    }
#if TBB_USE_ASSERT
    temp.clear();
#endif
}

// Additional possible methods that are not required right now
//! Wait for a condition to be satisfied with waiting-on my_context
// template<typename WaitUntil, typename my_context>
// void concurrent_monitor::wait( WaitUntil until, my_context on )
// {
//     bool slept = false;
//     thread_context thr_ctx;
//     prepare_wait( thr_ctx, on() );
//     while( !until() ) {
//         if( (slept = commit_wait( thr_ctx ) )==true )
//             if( until() ) break;
//         slept = false;
//         prepare_wait( thr_ctx, on() );
//     }
//     if( !slept )
//         cancel_wait( thr_ctx );
// }

} // namespace r1
} // namespace detail
} // namespace tbb

#endif /* __TBB_concurrent_monitor_H */
