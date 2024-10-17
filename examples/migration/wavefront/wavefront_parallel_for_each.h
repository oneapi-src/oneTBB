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

#ifndef WAVEFRONT_PARALLEL_FOR_EACH
#define WAVEFRONT_PARALLEL_FOR_EACH


#include <tbb/parallel_for_each.h>

#include <atomic>
#include <vector>
#include <numeric>
#include <chrono>
#include <iostream>

#include "utils.h"


unsigned long measure_parallel_for_each(std::vector<double>& a_ser, std::size_t size,
                                    std::size_t ntrial, std::size_t N, int block_size)
{
    std::vector<unsigned long> times;

    for (std::size_t i = 0; i < ntrial; ++i) {
        auto a_p_for_each = initialize_matrix(size);

        auto t1 = std::chrono::steady_clock::now();

        std::vector<std::atomic<int>> counters(size);

        for (int k = 0; k < N; ++k) {
            for (int l = 0; l < N; ++l) {
                if (k == 1 || l == 1) {
                    counters[k * N + l].store(1, std::memory_order_relaxed);
                } else {
                    counters[k * N + l].store(2, std::memory_order_relaxed);
                }
            }

        }
        counters[N + 1].store(0, std::memory_order_relaxed);

        using block = std::pair<size_t,size_t>;
        block origin(1,1);
        oneapi::tbb::parallel_for_each(&origin, &origin + 1, [&] (const block& b, oneapi::tbb::feeder<block>& feeder) {
            int i = b.first;
            int j = b.second;

            a_p_for_each[i * N + j] = calculate(a_p_for_each[i * N + j],
                                                a_p_for_each[(i - 1) * N + j],
                                                a_p_for_each[i * N + j - 1], block_size);

            if (j + 1 < N && --counters[i * N + j + 1] == 0) {
                feeder.add(block(i, j + 1));
            }
            if (i + 1 < N && --counters[(i + 1) * N + j] == 0) {
                feeder.add(block(i + 1, j));
            }
        });

        auto t2 = std::chrono::steady_clock::now();

        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        times.push_back(time);

        if (a_ser != a_p_for_each) {
            std::cerr << "Parallel computation with task_group failed!" << std::endl;
        }
    }

    return std::accumulate(times.begin(), times.end(), 0) / times.size();
}

#endif /* WAVEFRONT_PARALLEL_FOR_EACH */
