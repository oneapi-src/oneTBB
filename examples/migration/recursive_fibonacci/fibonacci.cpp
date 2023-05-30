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

#include "task_emulation_layer.h"

#include <iostream>
#include <numeric>
#include <utility>

int cutoff;

long serial_fib(int n) {
    return n < 2 ? n : serial_fib(n - 1) + serial_fib(n - 2);
}

struct fib_continuation : task_emulation::base_task {
    fib_continuation(int& s) : sum(s) {}

    void execute() override {
        sum = x + y;
    }

    int x{ 0 }, y{ 0 };
    int& sum;
};

struct fib_computation : task_emulation::base_task {
    fib_computation(int n, int* x) : n(n), x(x) {}

    void execute() override {
        if (n < cutoff) {
            *x = serial_fib(n);
        }
        else {
            // Continuation passing
            auto& c = *this->create_continuation<fib_continuation>(/* children_counter = */ 2, *x);
            task_emulation::run_task(c.create_child_of_continuation<fib_computation>(n - 1, &c.x));

            // Recycling
            this->recycle_as_child_of_continuation(c);
            n = n - 2;
            x = &c.y;

            // Bypass is not supported by task_emulation and next_task executed directly.
            // Howewer, the old-TBB bypass behavior can be achieved with
            // `return task_group::defer()` (check Migration Guide).
            // Consider submit another task if recursion call is not acceptable
            // i.e. instead of Recycling + Direct Body call
            // submit task_emulation::run_task(c.create_child_of_continuation<fib_computation>(n - 2, &c.y));
            this->operator()();
        }
    }

    int n;
    int* x;
};

int fibonacci(int n) {
    int sum{};
    tbb::task_group tg;
    tg.run_and_wait(
        task_emulation::create_root_task<fib_computation>(/* for root task = */ tg, n, &sum));
    return sum;
}

template <typename F>
std::pair</* result */ unsigned long, /* time */ unsigned long> measure(F&& f,
                                                                        int number,
                                                                        unsigned long ntrial) {
    std::vector<unsigned long> times;

    unsigned long result;
    for (unsigned long i = 0; i < ntrial; ++i) {
        auto t1 = std::chrono::steady_clock::now();
        result = f(number);
        auto t2 = std::chrono::steady_clock::now();

        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        times.push_back(time);
    }

    return std::make_pair(
        result,
        static_cast<unsigned long>(std::accumulate(times.begin(), times.end(), 0) / times.size()));
}

int main(int argc, char* argv[]) {
    int numbers = argc > 1 ? strtol(argv[1], nullptr, 0) : 50;
    cutoff = argc > 2 ? strtol(argv[2], nullptr, 0) : 16;
    unsigned long ntrial = argc > 3 ? (unsigned long)strtoul(argv[3], nullptr, 0) : 20;

    auto res = measure(fibonacci, numbers, ntrial);
    std::cout << "Fibonacci N = " << res.first << " Avg time = " << res.second << " ms"
              << std::endl;
}
