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

#ifndef _TBB_permit_manager_H
#define _TBB_permit_manager_H

#include "oneapi/tbb/detail/_utils.h"

namespace tbb {
namespace detail {
namespace r1 {

class arena;
class permit_manager_client;
class thread_pool_ticket;

using constraits_type = void*;

class permit_manager : no_copy {
public:
    virtual ~permit_manager() {}
    virtual permit_manager_client* create_client(arena& a, constraits_type* constraits) = 0;
    virtual void destroy_client(permit_manager_client& c) = 0;

    // ??? Update demand ???
    // virtual void update_resource_request(unsigned min, unsigned max, client&) = 0;
    // virtual void release_resource_request(client&) = 0;

    virtual void request_demand(unsigned min, unsigned max, permit_manager_client&) = 0;
    virtual void release_demand(permit_manager_client&) = 0;
};


class thread_pool : no_copy {
public:
    void wake_up(thread_pool_ticket& ticket, unsigned request) {
        suppress_unused_warning(ticket, request);
    }
private:
    // Intrusive_list ticket_list;
};

} // namespace r1
} // namespace detail
} // namespace tbb

#endif // _TBB_permit_manager_H
