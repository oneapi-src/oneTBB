//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>

double calc_pi(int num_intervals) {
  double dx = 1.0 / num_intervals;
  double sum = 0.0;
  for (int i = 0; i < num_intervals; ++i) {
    double x = (i+0.5)*dx;
    double h = std::sqrt(1-x*x);
    sum += h*dx;
  }
  double pi = 4 * sum;
  return pi;
}

int main() {
  const int num_intervals = std::numeric_limits<int>::max();
  double serial_time = 0.0;
  {
    auto st0 = std::chrono::high_resolution_clock::now();
    double pi = calc_pi(num_intervals);
    serial_time = 1e-9*(std::chrono::high_resolution_clock::now() - st0).count();
    std::cout << "serial pi == " << pi << std::endl;
  }

  std::cout << "serial_time == " << serial_time << " seconds" << std::endl;
  return 0;
}
