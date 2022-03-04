/*
    Copyright (c) 2020-2022 Intel Corporation

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

#ifndef _TBB_thread_dispatcher_H
#define _TBB_thread_dispatcher_H

#include "oneapi/tbb/detail/_config.h"
#include "oneapi/tbb/detail/_utils.h"
#include "oneapi/tbb/rw_mutex.h"
#include "arena.h"
#include "governor.h"
#include "thread_data.h"
#include "rml_tbb.h"
#include "clients.h"

class threading_control;

namespace tbb {
namespace detail {
namespace r1 {

class thread_dispatcher : no_copy, rml::tbb_client {
    using ticket_list_type = intrusive_list<thread_pool_ticket>;
    using ticket_list_mutex_type = d1::rw_mutex;
public:
    thread_dispatcher(threading_control& tc, unsigned hard_limit, std::size_t stack_size)
        : my_threading_control(tc)
        , my_num_workers_hard_limit(hard_limit)
        , my_stack_size(stack_size)
    {
        my_server = governor::create_rml_server( *this );
        __TBB_ASSERT( my_server, "Failed to create RML server" );
    }

    ~thread_dispatcher() {
        poison_pointer(my_server);
    }

    thread_pool_ticket* select_next_client(thread_pool_ticket* hint) {
        unsigned next_client_priority_level = num_priority_levels;
        if (hint) {
            next_client_priority_level = hint->priority_level();
        }

        for (unsigned idx = 0; idx < next_client_priority_level; ++idx) {
            if (!m_ticket_list[idx].empty()) {
                return &*m_ticket_list[idx].begin();
            }
        }

        return hint;
    }

    void insert_ticket(thread_pool_ticket& ticket) {
        __TBB_ASSERT(ticket.priority_level() < num_priority_levels, nullptr);
        ticket_list_mutex_type::scoped_lock lock(m_mutex);
        m_ticket_list[ticket.priority_level()].push_front(ticket);

        __TBB_ASSERT(!m_next_ticket || m_next_ticket->priority_level() < num_priority_levels, nullptr);
        m_next_ticket = select_next_client(m_next_ticket);
    }

    void remove_ticket(thread_pool_ticket& ticket) {
        __TBB_ASSERT(ticket.priority_level() < num_priority_levels, nullptr);
        ticket_list_mutex_type::scoped_lock lock(m_mutex);
        m_ticket_list[ticket.priority_level()].remove(ticket);

        if (m_next_ticket == &ticket) {
            m_next_ticket = nullptr;
        }
        m_next_ticket = select_next_client(m_next_ticket);
    }

    bool is_ticket_in_list(ticket_list_type& tickets, thread_pool_ticket* ticket) {
        __TBB_ASSERT(ticket, "Expected non-null pointer to ticket.");
        for (ticket_list_type::iterator it = tickets.begin(); it != tickets.end(); ++it) {
            if (ticket == &*it) {
                return true;
            }
        }
        return false;
    }

    bool is_ticket_alive(thread_pool_ticket* ticket) {
        if (!ticket) {
            return false;
        }

        // Still cannot access internals of the arena since the object itself might be destroyed.
        for (unsigned idx = 0; idx < num_priority_levels; ++idx) {
            if (is_ticket_in_list(m_ticket_list[idx], ticket)) {
                return true;
            }
        }
        return false;
    }

    thread_pool_ticket* ticket_in_need(ticket_list_type* tickets, thread_pool_ticket* hint) {
        // TODO: make sure arena with higher priority returned only if there are available slots in it.
        hint = select_next_client(hint);
        if (!hint) {
            return nullptr;
        }

        ticket_list_type::iterator it = hint;
        unsigned curr_priority_level = hint->priority_level();
        __TBB_ASSERT(it != tickets[curr_priority_level].end(), nullptr);
        do {
            thread_pool_ticket& t = *it;
            if (++it == tickets[curr_priority_level].end()) {
                do {
                    ++curr_priority_level %= num_priority_levels;
                } while (tickets[curr_priority_level].empty());
                it = tickets[curr_priority_level].begin();
            }
            if (t.try_join()) {
                return &t;
            }
        } while (it != hint);
        return nullptr;
    }


    thread_pool_ticket* ticket_in_need(thread_pool_ticket* prev) {
        ticket_list_mutex_type::scoped_lock lock(m_mutex, /*is_writer=*/false);
        if (is_ticket_alive(prev)) {
            return ticket_in_need(m_ticket_list, prev);
        }
        return ticket_in_need(m_ticket_list, m_next_ticket);
    }

    void process(job& j) override {
        thread_data& td = static_cast<thread_data&>(j);
        // td.my_last_client can be dead. Don't access it until arena_in_need is called
        thread_pool_ticket* ticket = td.my_last_ticket;
        for (int i = 0; i < 2; ++i) {
            while ((ticket = ticket_in_need(ticket)) ) {
                td.my_last_ticket = ticket;
                ticket->process(td);
            }
            // Workers leave thread_dispatcher because there is no arena in need. It can happen earlier than
            // adjust_job_count_estimate() decreases my_slack and RML can put this thread to sleep.
            // It might result in a busy-loop checking for my_slack<0 and calling this method instantly.
            // the yield refines this spinning.
            if ( !i ) {
                yield();
            }
        }
    }

    void adjust_job_count_estimate(int delta) {
        if (delta != 0) {
            my_server->adjust_job_count_estimate(delta);
        }
    }

    void cleanup(job& j) override;

    void acknowledge_close_connection() override;

    ::rml::job* create_one_job() override;


    //! Used when RML asks for join mode during workers termination.
    bool must_join_workers () const { return my_join_workers; }

    //! Returns the requested stack size of worker threads.
    std::size_t worker_stack_size() const { return my_stack_size; }

    version_type version () const override { return 0; }

    unsigned max_job_count () const override { return my_num_workers_hard_limit; }

    std::size_t min_stack_size () const override { return worker_stack_size(); }

private:
    friend class threading_control;
    static constexpr unsigned num_priority_levels = 3;
    ticket_list_mutex_type m_mutex;
    ticket_list_type m_ticket_list[num_priority_levels];

    thread_pool_ticket* m_next_ticket{nullptr};

    //! Shutdown mode
    bool my_join_workers{false};

    threading_control& my_threading_control;

    //! Maximal number of workers allowed for use by the underlying resource manager
    /** It can't be changed after market creation. **/
    unsigned my_num_workers_hard_limit{0};

    //! Stack size of worker threads
    std::size_t my_stack_size{0};

    //! First unused index of worker
    /** Used to assign indices to the new workers coming from RML **/
    std::atomic<unsigned> my_first_unused_worker_idx{0};

    //! Pointer to the RML server object that services this TBB instance.
    rml::tbb_server* my_server{nullptr};
};

} // namespace r1
} // namespace detail
} // namespace tbb

#endif // _TBB_thread_dispatcher_H