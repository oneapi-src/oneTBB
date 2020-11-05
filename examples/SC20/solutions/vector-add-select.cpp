//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <CL/sycl.hpp>
#define SIZE 1024

using namespace cl::sycl;

int main() {
  // Initialize container.
  std::array<int, SIZE> a, b, c;
  for (int i = 0; i<SIZE; ++i) {
    a[i] = i + 1;
    b[i] = -i;
    c[i] = 0;
  }
  {
    std::cout << "\nAvailable devices:\n";
    // Report available devices.
    auto platforms = platform::get_platforms();
    for (auto &platform : platforms) {
      std::cout << "Platform: "
        << platform.get_info<info::platform::name>() << std::endl;
      auto devices = platform.get_devices();
      for (auto &device : devices ) {
        std::cout << " Device: "
          << device.get_info<info::device::name>()
          << std::endl;
      }
    }
    
    range<1> r{SIZE};
    gpu_selector selector;
      
    queue q(selector);
    
    // Print out the device information.
    std::cout << "\nRunning on device: "
              << q.get_device().get_info<info::device::name>() << "\n";
      
    buffer<int, 1> a_buf(a.data(), r);
    buffer<int, 1> b_buf(b.data(), r);
    buffer<int, 1> c_buf(c.data(), r);
    q.submit([&](auto &h) {
      accessor a(a_buf, h, read_only);
      accessor b(b_buf, h, read_only);
      accessor c(c_buf, h, write_only);
      h.parallel_for(r,[=](id<1> idx) {
        c[idx] = a[idx] + b[idx];
      });
    }).wait();
  }
  std::cout << "DONE.\n";
  return 0;
}
