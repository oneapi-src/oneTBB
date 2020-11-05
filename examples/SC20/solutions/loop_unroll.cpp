//==============================================================
// Copyright (c) Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================
#include <CL/sycl.hpp>
#include <iomanip>
#include <iostream>
#include <vector>

// dpc_common.hpp can be found in the dev-utilities include folder.
// e.g., $ONEAPI_ROOT/dev-utilities//include/dpc_common.hpp
#include "dpc_common.hpp"

// Header locations and some DPC++ extensions changed between beta09 and beta10
// Temporarily modify the code sample to accept either version
#define BETA09 20200827
#if __SYCL_COMPILER_VERSION <= BETA09
  #include <CL/sycl/intel/fpga_extensions.hpp>
  namespace INTEL = sycl::intel;  // Namespace alias for backward compatibility
#else
  #include <CL/sycl/INTEL/fpga_extensions.hpp>
#endif

using namespace sycl;

template <int unroll_factor> class VAdd;

// This function instantiates the vector add kernel, which contains
// a loop that adds up the two summand arrays and stores the result
// into sum. This loop will be unrolled by the specified unroll_factor.
template <int unroll_factor>
void VecAdd(const std::vector<float> &summands1,
            const std::vector<float> &summands2, std::vector<float> &sum,
            size_t array_size) {


#if defined(FPGA_EMULATOR)
  INTEL::fpga_emulator_selector device_selector;
#else
  INTEL::fpga_selector device_selector;
#endif

  try {
    queue q(device_selector, dpc_common::exception_handler,
            property::queue::enable_profiling{});

    buffer buffer_summands1(summands1);
    buffer buffer_summands2(summands2);
    // Use verbose SYCL 1.2 syntax for the output buffer.
    // (This will become unnecessary in a future compiler version.)
    buffer<float, 1> buffer_sum(sum.data(), array_size);

    event e = q.submit([&](handler &h) {
      auto acc_summands1 = buffer_summands1.get_access<access::mode::read>(h);
      auto acc_summands2 = buffer_summands2.get_access<access::mode::read>(h);
      auto acc_sum = buffer_sum.get_access<access::mode::discard_write>(h);

      h.single_task<VAdd<unroll_factor>>([=]()
                                         [[intel::kernel_args_restrict]] {
        // Unroll the loop fully or partially, depending on unroll_factor
        #pragma unroll unroll_factor
        for (size_t i = 0; i < array_size; i++) {
          acc_sum[i] = acc_summands1[i] + acc_summands2[i];
        }
      });
    });

    double start = e.get_profiling_info<info::event_profiling::command_start>();
    double end = e.get_profiling_info<info::event_profiling::command_end>();
    // convert from nanoseconds to ms
    double kernel_time = (double)(end - start) * 1e-6;

    std::cout << "unroll_factor " << unroll_factor
              << " kernel time : " << kernel_time << " ms\n";
    std::cout << "Throughput for kernel with unroll_factor " << unroll_factor
              << ": ";
    std::cout << std::fixed << std::setprecision(3)
              << ((double)array_size / kernel_time) / 1e6f << " GFlops\n";

  } catch (sycl::exception const &e) {
    // Catches exceptions in the host code
    std::cout << "Caught a SYCL host exception:\n" << e.what() << "\n";

    // Most likely the runtime couldn't find FPGA hardware!
    if (e.get_cl_code() == CL_DEVICE_NOT_FOUND) {
      std::cout << "If you are targeting an FPGA, please ensure that your "
                   "system has a correctly configured FPGA board.\n";
      std::cout << "If you are targeting the FPGA emulator, compile with "
                   "-DFPGA_EMULATOR.\n";
    }
    std::terminate();
  }
}

int main(int argc, char *argv[]) {
  size_t array_size = 1 << 26;

  if (argc > 1) {
    std::string option(argv[1]);
    if (option == "-h" || option == "--help") {
      std::cout << "Usage: \n<executable> <data size>\n\nFAILED\n";
      return 1;
    } else {
      array_size = std::stoi(option);
    }
  }

  std::vector<float> summands1(array_size);
  std::vector<float> summands2(array_size);

  std::vector<float> sum_unrollx1(array_size);
  std::vector<float> sum_unrollx2(array_size);
  std::vector<float> sum_unrollx4(array_size);
  std::vector<float> sum_unrollx8(array_size);
  std::vector<float> sum_unrollx16(array_size);

  // Initialize the two summand arrays (arrays to be added to each other) to
  // 1:N and N:1, so that the sum of all elements is N + 1
  for (size_t i = 0; i < array_size; i++) {
    summands1[i] = static_cast<float>(i + 1);
    summands2[i] = static_cast<float>(array_size - i);
  }

  std::cout << "Input Array Size:  " << array_size << "\n";

  // Instantiate VecAdd kernel with different unroll factors: 1, 2, 4, 8, 16
  // The VecAdd kernel contains a loop that adds up the two summand arrays.
  // This loop will be unrolled by the specified unroll factor.
  // The sum array is expected to be identical, regardless of the unroll factor.
  VecAdd<1>(summands1, summands2, sum_unrollx1, array_size);
  VecAdd<2>(summands1, summands2, sum_unrollx2, array_size);
  VecAdd<4>(summands1, summands2, sum_unrollx4, array_size);
  VecAdd<8>(summands1, summands2, sum_unrollx8, array_size);
  VecAdd<16>(summands1, summands2, sum_unrollx16, array_size);

  // Verify that the output data is the same for every unroll factor
  for (size_t i = 0; i < array_size; i++) {
    if (sum_unrollx1[i] != summands1[i] + summands2[i] ||
        sum_unrollx1[i] != sum_unrollx2[i] ||
        sum_unrollx1[i] != sum_unrollx4[i] ||
        sum_unrollx1[i] != sum_unrollx8[i] ||
        sum_unrollx1[i] != sum_unrollx16[i]) {
      std::cout << "FAILED: The results are incorrect\n";
      return 1;
    }
  }
  std::cout << "PASSED: The results are correct\n";
  return 0;
}
