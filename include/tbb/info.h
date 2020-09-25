/*
    Copyright (c) 2019-2020 Intel Corporation

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

#ifndef __TBB_info_H
#define __TBB_info_H

#include "detail/_config.h"

#if __TBB_NUMA_SUPPORT
#include <vector>

namespace tbb {
namespace detail {

namespace r1 {
unsigned __TBB_EXPORTED_FUNC numa_node_count();
void __TBB_EXPORTED_FUNC fill_numa_indices(int* index_array);
int __TBB_EXPORTED_FUNC numa_default_concurrency(int node_id);
} // namespace r1

namespace d1{

using numa_node_id = int;

inline std::vector<numa_node_id> numa_nodes() {
    std::vector<numa_node_id> node_indices(r1::numa_node_count());
    r1::fill_numa_indices(&node_indices.front());
    return node_indices;
}

inline int default_concurrency(numa_node_id id = -1) {
    return r1::numa_default_concurrency(id);
}

} // namespace d1
} // namespace detail

inline namespace v1 {
using detail::d1::numa_node_id;
namespace info {
using detail::d1::numa_nodes;
using detail::d1::default_concurrency;
} // namespace info
} // namespace v1

} // namespace tbb

#endif /*__TBB_NUMA_SUPPORT*/

#endif /*__TBB_info_H*/
