/*
    Copyright (c) 2023 Intel Corporation

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

#ifndef SINGLE_TASK_HEADER
#define SINGLE_TASK_HEADER

#include "task_emulation_layer.h"

#include <iostream>
#include <numeric>
#include <utility>

extern int cutoff;
extern bool testing_enabled;

long serial_fib_1(int n) {
    return n < 2 ? n : serial_fib_1(n - 1) + serial_fib_1(n - 2);
}

struct single_fib_task : task_emulation::base_task {
    enum class state {
        compute,
        sum
    };

    single_fib_task(int n, int* x) : n(n), x(x), s(state::compute)
    {}

    task_emulation::base_task* execute() override {
        task_emulation::base_task* bypass = nullptr;
        switch (s) {
            case state::compute : {
                bypass = compute_impl();
                break;
            }
            case state::sum : {
                *x = x_l + x_r;

                if (testing_enabled) {
                    if (n == cutoff && num_recycles > 0) {
                        --num_recycles;
                        bypass = compute_impl();
                    }
                }

                break;
            }
        }
        return bypass;
    }

    task_emulation::base_task* compute_impl() {
        task_emulation::base_task* bypass = nullptr;
        if (n < cutoff) {
            *x = serial_fib_1(n);
        }
        else {
            bypass = this->allocate_child_and_increment<single_fib_task>(n - 2, &x_r);
            task_emulation::run_task(this->allocate_child_and_increment<single_fib_task>(n - 1, &x_l));

            // Recycling
            this->s = state::sum;
            this->recycle_as_continuation();
        }
        return bypass;
    }


    int n;
    int* x;
    state s;

    int x_l{ 0 }, x_r{ 0 };
    int num_recycles{5};
};

int fibonacci_single_task(int n) {
    int sum{};
    tbb::task_group tg;
    task_emulation::run_and_wait(tg, task_emulation::allocate_root_task<single_fib_task>(/* for root task = */ tg, n, &sum));
    return sum;
}

#endif // SINGLE_TASK_HEADER
