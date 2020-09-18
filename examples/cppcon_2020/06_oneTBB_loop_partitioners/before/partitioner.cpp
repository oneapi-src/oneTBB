//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include "../common/partitioner_test.h"
#include <iostream>

int main() {
  // STEP 3: Try different grainsize values, maybe 10, 100 and 1000
  // STEP 4: Pass a grainsize of 1, but change the partitioner type to auto_partitioner or static_partitioner
  auto r = run_test<tbb::simple_partitioner>(/* title in chart */ "simple_partitioner", /* grainsize */ 1);
  std::cout << "Wallclock time == " << r.first << "\n"
            << "Number of body executions == " << r.second << "\n";
}
