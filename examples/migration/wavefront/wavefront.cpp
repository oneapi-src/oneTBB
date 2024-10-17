/*
    Copyright (c) 2024 Intel Corporation

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

#include "wavefront_task_group.h"
#include "wavefront_parallel_for_each.h"
#include "common/utility/utility.hpp"

#include <iostream>
#include <atomic>
#include <vector>

unsigned long measure_single_thread(std::vector<double>& a_ser, std::size_t size,
                                    std::size_t ntrial, std::size_t N, int block_size)
{
    std::vector<unsigned long> times;

    for (std::size_t i = 0; i < ntrial; ++i) {
        a_ser = initialize_matrix(size);

        auto t1 = std::chrono::steady_clock::now();
        for (std::size_t i = 1; i < N; ++i) {
            for (std::size_t j = 1; j < N; ++j) {
                a_ser[i * N + j] = calculate(a_ser[i * N + j], a_ser[(i - 1) * N + j],
                                             a_ser[i * N + j - 1], block_size);
            }
        }
        auto t2 = std::chrono::steady_clock::now();

        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        times.push_back(time);
    }

    return std::accumulate(times.begin(), times.end(), 0) / times.size();
}

int main (int argc, char **argv) {
    std::size_t N = 1000;
    std::size_t size = N * N;
    std::size_t ntrial = 20;
    int block_size = 50;


    std::vector<double> a_ser_ref;
    auto avg_serial_time = measure_single_thread(a_ser_ref, size, ntrial, N, block_size);
    std::cout << "Wavefront Serial, time = "
              << avg_serial_time
              << " ms" << std::endl;

    auto avg_task_group_time = measure_task_group(a_ser_ref, size, ntrial, N, block_size);
    std::cout << "Wavefront task_group, time = "
            << avg_task_group_time
            << " ms" << std::endl;

    auto avg_p_for_each_time = measure_parallel_for_each(a_ser_ref, size, ntrial, N, block_size);
    std::cout << "Wavefront parallel_for_each, time = "
            << avg_p_for_each_time
            << " ms" << std::endl;

//   std::cout<<"Serial Time = " << t_ser <<" msec\n";
//   std::cout<<"Thrds = " << nth << "; Parallel Time = " << t_par << " msec\n";
//   std::cout<<"Speedup = " << t_ser/t_par << '\n';

  return 0;
}