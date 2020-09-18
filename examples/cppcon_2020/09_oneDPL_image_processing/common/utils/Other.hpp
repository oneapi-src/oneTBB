//==============================================================
// Copyright (c) 2019-2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#ifndef _GAMMA_UTILS_OTHER_HPP
#define _GAMMA_UTILS_OTHER_HPP

#include <chrono>

// function for time measuring
inline double get_time_in_sec() {
  namespace ch = std::chrono;
  return ch::time_point_cast<ch::milliseconds>(ch::steady_clock::now())
             .time_since_epoch()
             .count() *
         1.e-3;
}

// function to check correctness
template <typename It>
bool check(It begin1, It end1, It begin2) {
  for (; begin1 != end1; ++begin1, ++begin2) {
    if (*begin1 != *begin2) return false;
  }

  return true;
}

#endif  // _GAMMA_UTILS_OTHER_HPP
