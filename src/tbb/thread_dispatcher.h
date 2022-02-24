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

#include "clients.h"

namespace tbb {
namespace detail {
namespace r1 {

class thread_dispatcher : no_copy {
    using ticket_list_type = intrusive_list<thread_pool_ticket>;
    using ticket_list_mutex_type = d1::rw_mutex;
public:
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
        // TODO: introduce three state response: alive, not_alive, no_market_arenas
        if (is_ticket_alive(prev)) {
            return ticket_in_need(m_ticket_list, prev);
        }
        return ticket_in_need(m_ticket_list, m_next_ticket);
    }

    void process(thread_data& td) {
        // td.my_last_client can be dead. Don't access it until arena_in_need is called
        thread_pool_ticket* ticket = td.my_last_ticket;
        for (int i = 0; i < 2; ++i) {
            while ((ticket = ticket_in_need(ticket)) ) {
                td.my_last_ticket = ticket;
                ticket->process(td);
            }
            // Workers leave market because there is no arena in need. It can happen earlier than
            // adjust_job_count_estimate() decreases my_slack and RML can put this thread to sleep.
            // It might result in a busy-loop checking for my_slack<0 and calling this method instantly.
            // the yield refines this spinning.
            if ( !i ) {
                yield();
            }
        }
    }

private:
    static constexpr unsigned num_priority_levels = 3;
    ticket_list_mutex_type m_mutex;
    ticket_list_type m_ticket_list[num_priority_levels];

    thread_pool_ticket* m_next_ticket{nullptr};
};

} // namespace r1
} // namespace detail
} // namespace tbb

#endif // _TBB_thread_dispatcher_H