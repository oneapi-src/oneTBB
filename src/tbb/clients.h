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

#ifndef _TBB_clients_H
#define _TBB_clients_H

#include "oneapi/tbb/detail/_intrusive_list_node.h"

namespace tbb {
namespace detail {
namespace r1 {

class arena;

using mask_type = void*;

class thread_pool_ticket : public d1::intrusive_list_node /* Need for list in thread pool */ {
public:
    void apply_mask(mask_type mask) {}
    // Interface of communication with thread pool
    bool try_join() {}
    void process() {}
private:
    arena& m_arena;
    mask_type m_mask;
};

// TODO resource_manager_client and thread_pool_client
class permit_manager_client {
public:
    permit_manager_client(arena& a) : m_arena(a) {}
    // Interface of communication with resource manager
    virtual void update_allotment() = 0;
protected:
    arena& m_arena;
    thread_pool_ticket m_ticket;
};


} // namespace r1
} // namespace detail
} // namespace tbb



#endif // _TBB_clients_H
