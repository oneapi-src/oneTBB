/*
    Copyright (c) 2005-2021 Intel Corporation

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

#include "common/test.h"
#include "common/utils.h"
#include "common/utils_concurrency_limit.h"
#include "common/config.h"
#include "common/rwm_upgrade_downgrade.h"

#include <atomic>

namespace test_with_native_threads {

template <typename M>
struct Counter {
    using mutex_type = M;

    M mutex;
    long value;

    void flog_once( std::size_t mode ) {
        // Increments counter once for each iteration in the iteration space
        if (mode & 1) {
            // Try implicit acquire and explicit release
            typename mutex_type::scoped_lock lock(mutex);
            value += 1;
            lock.release();
        } else {
            // Try explicit acquire and implicit release
            typename mutex_type::scoped_lock lock;
            lock.acquire(mutex);
            value += 1;
        }
    }
}; // struct Counter

template <typename M, long N>
struct Invariant {
    using mutex_type = M;

    M mutex;
    long value[N];

    Invariant() {
        for (long k = 0; k < N; ++k) {
            value[k] = 0;
        }
    }

    void update() {
        for (long k = 0; k < N; ++k) {
            ++value[k];
        }
    }

    bool value_is( long expected_value ) const {
        long tmp;

        for (long k = 0; k < N; ++k) {
            if ((tmp = value[k]) != expected_value) {
                return false;
            }
        }
        return true;
    }

    bool is_okay() {
        return value_is(value[0]);
    }

    void flog_once( std::size_t mode ) {
        // Every 8th access is a write access
        bool write = (mode % 8) == 7;
        bool okay = true;
        bool lock_kept = true;

        if ((mode / 8) & 1) {
            // Try implicit acquire and explicit release
            typename mutex_type::scoped_lock lock(mutex, write);
            if (write) {
                long my_value = value[0];
                update();
                if (mode % 16 == 7) {
                    lock_kept = lock.downgrade_to_reader();
                    if (!lock_kept) {
                        my_value = value[0] - 1;
                    }
                    okay = value_is(my_value + 1);
                }
            } else {
                okay = is_okay();
                if (mode % 8 == 3) {
                    long my_value = value[0];
                    lock_kept = lock.upgrade_to_writer();
                    if (!lock_kept) {
                        my_value = value[0];
                    }
                    update();
                    okay = value_is(my_value + 1);
                }
            }
            lock.release();
        } else {
            // Try explicit acquire and implicit release
            typename mutex_type::scoped_lock lock;
            lock.acquire(mutex, write);
            if (write) {
                long my_value = value[0];
                update();
                if (mode % 16 == 7) {
                    lock_kept = lock.downgrade_to_reader();
                    if (!lock_kept) {
                        my_value = value[0] - 1;
                    }
                    okay = value_is(my_value + 1);
                }
            } else {
                okay = is_okay();
                if (mode % 8 == 3) {
                    long my_value = value[0];
                    lock_kept = lock.upgrade_to_writer();
                    if (!lock_kept) {
                        my_value = value[0];
                    }
                    update();
                    okay = value_is(my_value + 1);
                }
            }
        }
        REQUIRE(okay);
    }
}; // struct Invariant

static std::atomic<std::size_t> Order;

template <typename State, long TestSize>
struct Work : utils::NoAssign {
    static constexpr std::size_t chunk = 100;
    State& state;

    Work( State& st ) : state(st){ Order = 0; }

    void operator()(std::size_t) const {
        std::size_t step;
        while( (step = Order.fetch_add(chunk, std::memory_order_acquire)) < TestSize ) {
            for (std::size_t i = 0; i < chunk && step < TestSize; ++i, ++step) {
                state.flog_once(step);
            }
        }
    }
}; // struct Work

constexpr std::size_t TEST_SIZE = 100000;

template <typename M>
void test_basic( std::size_t nthread ) {
    Counter<M> counter;
    counter.value = 0;
    Order = 0;
    utils::NativeParallelFor(nthread, Work<Counter<M>, TEST_SIZE>(counter));

    REQUIRE(counter.value == TEST_SIZE);
}

template <typename M>
void test_rw_basic( std::size_t nthread ) {
    Invariant<M, 8> invariant;
    Order = 0;
    // use the macro because of a gcc 4.6 issue
    utils::NativeParallelFor(nthread, Work<Invariant<M, 8>, TEST_SIZE>(invariant));
    // There is either a writer or a reader upgraded to a writer for each 4th iteration
    long expected_value = TEST_SIZE / 4;
    REQUIRE(invariant.value_is(expected_value));
}

template <typename M>
void test() {
    for (std::size_t p : utils::concurrency_range()) {
        test_basic<M>(p);
    }
}

template <typename M>
void test_rw() {
    for (std::size_t p : utils::concurrency_range()) {
        test_rw_basic<M>(p);
    }
}

} // namespace test_with_native_threads
