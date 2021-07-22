#define TBB_PREVIEW_WAITING_FOR_WORKERS 1
#define __TBB_STATISTICS 1

#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/global_control.h>
#include <iostream>

#include "common/test.h"


//! \file test_statistics.cpp
//! \brief Test for [internal] functionality
TEST_CASE("Simple") {
    auto values = std::vector<double>(1000000, 0);
    tbb::task_scheduler_handle handler = tbb::task_scheduler_handle::get();

    tbb::parallel_for(tbb::blocked_range<int>(0, values.size()),
                       [&](tbb::blocked_range<int> r) {
        for (int i = r.begin(); i < r.end(); ++i) {
            values[i] = i;
        }
    });
    tbb::finalize(handler);
    std::cout << std::flush;
}
