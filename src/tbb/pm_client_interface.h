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

#ifndef _TBB_pm_client_interface_H
#define _TBB_pm_client_interface_H

#include "oneapi/tbb/detail/_config.h"

namespace tbb {
namespace detail {
namespace r1 {

class pm_client_interface {
public:
    virtual ~pm_client_interface() {}
    virtual void register_thread() = 0;
    virtual void unregister_thread() = 0;
};


} // namespace r1
} // namespace detail
} // namespace tbb



#endif // _TBB_pm_client_interface_H
