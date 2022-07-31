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

#ifndef __TBB_test_common_test_invoke_H
#define __TBB_test_common_test_invoke_H

#include "oneapi/tbb/flow_graph.h"

#if __TBB_CPP17_INVOKE_PRESENT
namespace test_invoke {

template <std::size_t I>
struct SmartObjectImpl {
    SmartObjectImpl() {}
    SmartObjectImpl(const SmartObjectImpl&) = default;
 
    using subobject_type = SmartObjectImpl<I - 1>;

    subobject_type get_subobject() const {
        return subobject;
    }

private:
    using multinode_of_two = oneapi::tbb::flow::multifunction_node<SmartObjectImpl, std::tuple<subobject_type, subobject_type>>;
public:

    void send_subobject(typename multinode_of_two::output_ports_type& ports) const {
        std::get<0>(ports).try_put(subobject);
        std::get<1>(ports).try_put(subobject);
    }

    subobject_type subobject;
};

template <>
struct SmartObjectImpl<0> {
    SmartObjectImpl() = default;
    SmartObjectImpl(std::vector<std::size_t>&) {}
};

using SmartObject = SmartObjectImpl<9>;

} // namespace test_invoke

#endif // __TBB_CPP17_INVOKE_PRESENT
#endif // __TBB_test_common_test_invoke_H
