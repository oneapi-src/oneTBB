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

#ifndef WAVEFRONT_TASK_GROUP_H
#define WAVEFRONT_TASK_GROUP_H

#define TBB_PREVIEW_TASK_GROUP_EXTENSIONS 1
#include <tbb/task_group.h>

#include <atomic>
#include <vector>
#include <numeric>
#include <chrono>
#include <iostream>

#include "utils.h"

class cell_computation {
public:
    cell_computation(std::size_t i_ ,std::size_t j_, std::size_t N_, int block_size_,
                     std::vector<double>& A_, std::vector<std::atomic<int>>& counters_, tbb::task_group& tg_)
        : i{i_}, j{j_}, N{N_}, block_size{block_size_}, A{A_}, counters{counters_}, tg(tg_) {}

    tbb::task_handle operator()() const {
        int bypass_east = 0;
        int bypass_south = 0;

        A[i * N + j] = calculate(A[i * N + j], A[(i - 1) * N + j], A[i * N + j - 1], block_size);

        if (try_release_dependant(i, j + 1)) {
            bypass_east = 1;
        }
        if (try_release_dependant(i + 1, j)) {
            if (bypass_east == 0) {
                bypass_south = 1;
            } else {
                tg.run(cell_computation(i + 1, j, N, block_size, A, counters, tg));
            }
        }

        if (bypass_east > 0 || bypass_south > 0) {
            return tg.defer(cell_computation(i + bypass_south, j + bypass_east, N, block_size, A, counters, tg));
        }

        return tbb::task_handle{};
    }
private:
    bool try_release_dependant(std::size_t i, std::size_t j) const {
        if (i < N && j < N && --counters[i * N + j] == 0) {
            return true;
        }
        return false;
    }

    std::size_t i, j;
    std::size_t N;
    int block_size;
    std::vector<double>& A;
    std::vector<std::atomic<int>>& counters;
    tbb::task_group& tg;
};

unsigned long measure_task_group(std::vector<double>& a_ser, std::size_t size,
                                    std::size_t ntrial, std::size_t N, int block_size)
{
    std::vector<unsigned long> times;

    for (std::size_t i = 0; i < ntrial; ++i) {
        auto a_tg = initialize_matrix(size);

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

        tbb::task_group tg;
        tg.run_and_wait(cell_computation(1, 1, N, block_size, a_tg, counters, tg));

        auto t2 = std::chrono::steady_clock::now();

        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        times.push_back(time);

        if (a_ser != a_tg) {
            std::cerr << "Parallel computation with task_group failed!" << std::endl;
        }
    }

    return std::accumulate(times.begin(), times.end(), 0) / times.size();
}

#endif /* WAVEFRONT_TASK_GROUP_H */