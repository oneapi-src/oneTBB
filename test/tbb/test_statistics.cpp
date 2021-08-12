/*
    Copyright (c) 2005-2021 Intel Corporation

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

#define TBB_PREVIEW_WAITING_FOR_WORKERS 1

#include <cstdint>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/global_control.h>
#include "common/test.h"


//! \file test_statistics.cpp
//! \brief Test for [internal] functionality

//! \brief \ref error_guessing
TEST_CASE("Simple") {
    auto values = std::vector<std::size_t>(1000000, 0u);
    tbb::task_scheduler_handle handler = tbb::task_scheduler_handle::get();

    tbb::parallel_for(tbb::blocked_range<std::size_t>(0u, values.size()),
                       [&](tbb::blocked_range<std::size_t> r) {
        for (std::size_t i = r.begin(); i != r.end(); ++i) {
            values[i] = i;
        }
    });
    tbb::finalize(handler);
}

