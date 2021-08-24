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
#include "common/concepts_common.h"
#include <tbb/null_rw_mutex.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <random>
#include <utility>

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

template <typename RWMutexType>
void TestIsWriter(const char* mutex_name) {
    using scoped_lock = typename RWMutexType::scoped_lock;

    RWMutexType rw_mutex;
    std::string error_message_writer = std::string(mutex_name) + "::scoped_lock is not acquired for write, is_writer should return false";
    std::string error_message_not_writer = std::string(mutex_name) + "::scoped_lock is acquired for write, is_writer should return true";
    // Test is_writer after construction
    {
        scoped_lock lock(rw_mutex, /*writer = */false);
        CHECK_MESSAGE(!lock.is_writer(), error_message_writer);
    }
    {
        scoped_lock lock(rw_mutex, /*writer = */true);
        CHECK_MESSAGE(lock.is_writer(), error_message_not_writer);
    }
    // Test is_writer after acquire
    {
        scoped_lock lock;
        lock.acquire(rw_mutex, /*writer = */false);
        CHECK_MESSAGE(!lock.is_writer(), error_message_writer);
    }
    {
        scoped_lock lock;
        lock.acquire(rw_mutex, /*writer = */true);
        CHECK_MESSAGE(lock.is_writer(), error_message_not_writer);
    }
    // Test is_writer on upgrade/downgrade
    {
        scoped_lock lock(rw_mutex, /*writer = */false);
        lock.upgrade_to_writer();
        CHECK_MESSAGE(lock.is_writer(), error_message_not_writer);
        lock.downgrade_to_reader();
        CHECK_MESSAGE(!lock.is_writer(), error_message_writer);
    }
}

template <>
void TestIsWriter<oneapi::tbb::null_rw_mutex>( const char* ) {
    using scoped_lock = typename oneapi::tbb::null_rw_mutex::scoped_lock;

    oneapi::tbb::null_rw_mutex nrw_mutex;
    scoped_lock l(nrw_mutex);
    CHECK(l.is_writer());
}

namespace test_move_constructor_details {

static constexpr std::size_t buckets_number = 10;
static constexpr std::size_t bucket_capacity = 42;
static constexpr std::size_t total_elements = buckets_number * bucket_capacity;

template <class Mutex>
class hash_set_facade {
    unsigned values[total_elements];
    Mutex mutexes[buckets_number];

    std::mt19937 random;

    struct element {
        unsigned & value;
        typename Mutex::scoped_lock lock;
    };

public:
    hash_set_facade() noexcept {
        std::for_each(values, values + total_elements, [this] (unsigned & value) {
            value = random();
        });
    }

    using iterator = unsigned *;

    iterator begin() noexcept {
        return values;
    }

    iterator end() noexcept {
        return values + total_elements;
    }

    element operator[](std::size_t required_index) {
        {
            auto in_range = [] (std::size_t a, std::size_t b, std::size_t c) {
                return a <= b && b <= c;
            };
            CHECK(in_range(0, required_index, total_elements));
        }
        std::size_t bucket_index = required_index / bucket_capacity;
        Mutex::scoped_lock lock(mutexes[bucket_index]);
        return element {values[required_index], std::move(lock)};
    }

    unsigned get_pure_value(std::size_t required_index) {
        {
            auto in_range = [] (std::size_t a, std::size_t b, std::size_t c) {
                return a <= b && b <= c;
            };
            CHECK(in_range(0, required_index, total_elements));
        }
        return values[required_index];
    }
};

}

template <class Mutex>
void test_scoped_lock_move_constructor_basic() {
    Mutex mutex;
    Mutex::scoped_lock global_lock(mutex);

    int mutable_data = 42;

    std::thread thread1([&mutex, &global_lock, &mutable_data]() {
        auto thread1_lock = std::move(global_lock);
        
        std::thread thread2([&thread1_lock, &mutable_data]() {
            auto thread2_lock = std::move(thread1_lock);

            mutable_data = 12;
        });
        thread2.join();
    });
    thread1.join();

    REQUIRE(mutable_data == 12);
}

template <class Mutex>
void test_scoped_lock_move_constructor_return_value() {
    using namespace test_move_constructor_details;

    hash_set_facade<Mutex> hash_set;

    utils::NativeParallelFor(total_elements, [&hash_set](std::size_t required_index) {
        auto access = hash_set[required_index];
        REQUIRE(access.value == hash_set.get_pure_value(required_index));
        access.value = 0;
    });

    REQUIRE(std::all_of(hash_set.begin(), hash_set.end(), [](unsigned value) {
        return (value == 0);
    }));
}

template <class Mutex>
void test_scoped_lock_move_assignment_operator_basic() {
    Mutex mutex1;
    Mutex mutex2;

    Mutex::scoped_lock lock1(mutex1);
    Mutex::scoped_lock lock2(mutex2);

    lock1 = std::move(lock2);
    lock2 = std::move(lock1);

    std::thread t1([&mutex1]() {
        Mutex::scoped_lock local_lock;
        REQUIRE(local_lock.try_acquire(mutex1));
    });

    std::atomic<bool> try_lock_reached(false);
    
    std::thread t2([&mutex2, &try_lock_reached]() {
        Mutex::scoped_lock local_lock;
       
        REQUIRE(!local_lock.try_acquire(mutex2));

        try_lock_reached.store(true);

        local_lock.acquire(mutex2);
    });

    tbb::detail::d0::spin_wait_while_eq(try_lock_reached, false);

    lock2.release();
    
    t1.join();
    t2.join();
}

template <class Mutex>
void test_scoped_lock_move_assignment_operator_complex() {
    constexpr std::size_t mutable_data_number = 200;

    std::array<int, mutable_data_number> mutable_data;
    mutable_data.fill(0);

    std::array<Mutex, mutable_data_number> mutexes;

    std::array<Mutex::scoped_lock, mutable_data_number> main_locks;
    
    std::transform(main_locks.begin(), main_locks.end(), mutexes.begin(), main_locks.begin(),
        [](Mutex::scoped_lock & lock, Mutex & mutex) -> Mutex::scoped_lock {
            lock.acquire(mutex);
            return std::move(lock);
        }
    );

    std::vector<std::thread> aside_threads;
    aside_threads.reserve(mutable_data_number);

    for (std::size_t i = 0; i < mutable_data_number; ++i) {
        aside_threads.emplace_back([i, &mutable_data, &mutexes]() {
            Mutex::scoped_lock local_lock(mutexes.at(i));
            REQUIRE(mutable_data.at(i) == 1);
            mutable_data.at(i) = 2;
        });
    }

    for (std::size_t i = 0; i < mutable_data_number; ++i) {
        main_locks.front() = std::move(main_locks.at(i));

        REQUIRE(mutable_data.at(i) == 0);
        mutable_data.at(i) = 1;
    }

    main_locks.front().release();

    std::for_each(aside_threads.begin(), aside_threads.end(), [](std::thread & thread) {
        thread.join();
    });

    REQUIRE(std::all_of(mutable_data.begin(), mutable_data.end(), [](int value) {
        return (value == 2); 
    }));
}
