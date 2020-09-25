/*
    Copyright (c) 2005-2020 Intel Corporation

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

#include <tbb/spin_mutex.h>
#include <tbb/spin_rw_mutex.h>
#include <tbb/queuing_mutex.h>
#include <tbb/queuing_rw_mutex.h>
#include <tbb/parallel_for.h>
#include <tbb/detail/_utils.h>
#include <tbb/detail/_machine.h>

#include <atomic>

//! \file test_mutex.cpp
//! \brief Test for [mutex.spin_mutex mutex.spin_rw_mutex mutex.queuing_mutex mutex.queuing_rw_mutexmutex.speculative_spin_mutex mutex.speculative_spin_rw_mutex] specifications

template<typename M>
struct Counter {
    typedef M mutex_type;
    M mutex;
    volatile long value;
};

#if __TBB_TSX_INTRINSICS_PRESENT

inline static bool IsInsideTx() {
    return _xtest() != 0;
}

bool have_TSX() {
    bool result = false;
    const int rtm_ebx_mask = 1<<11;
#if _MSC_VER
    int info[4] = {0,0,0,0};
    const int reg_ebx = 1;
    __cpuidex(info, 7, 0);
    result = (info[reg_ebx] & rtm_ebx_mask)!=0;
#elif __GNUC__ || __SUNPRO_CC
    int32_t reg_ebx = 0;
    int32_t reg_eax = 7;
    int32_t reg_ecx = 0;
    __asm__ __volatile__ ( "movl %%ebx, %%esi\n"
                           "cpuid\n"
                           "movl %%ebx, %0\n"
                           "movl %%esi, %%ebx\n"
                           : "=a"(reg_ebx) : "0" (reg_eax), "c" (reg_ecx) : "esi",
#if __TBB_x86_64
                           "ebx",
#endif
                           "edx"
                           );
    result = (reg_ebx & rtm_ebx_mask)!=0 ;
#endif
    return result;
}

//! Function object for use with parallel_for.h to see if a transaction is actually attempted.
std::atomic<std::size_t> n_transactions_attempted;
template<typename C>
struct AddOne_CheckTransaction {

    AddOne_CheckTransaction& operator=(const AddOne_CheckTransaction&) = delete;
    AddOne_CheckTransaction(const AddOne_CheckTransaction&) = default;
    AddOne_CheckTransaction() = default;

    C& counter;
    /** Increments counter once for each iteration in the iteration space. */
    void operator()(tbb::blocked_range<size_t>& range) const {
        for (std::size_t i = range.begin(); i != range.end(); ++i) {
            bool transaction_attempted = false;
            {
              typename C::mutex_type::scoped_lock lock(counter.mutex);
              if (IsInsideTx()) transaction_attempted = true;
              counter.value = counter.value + 1;
            }
            if(transaction_attempted) ++n_transactions_attempted;
            tbb::detail::machine_pause(static_cast<int32_t>(i));
        }
    }
    AddOne_CheckTransaction(C& counter_) : counter(counter_) {}
};

/* TestTransaction() checks if a speculative mutex actually uses transactions. */
template<typename M>
void TestTransaction(const char* name)
{
    Counter<M> counter;
    constexpr int n = 550;

    n_transactions_attempted = 0;
    for(int i = 0; i < 5 && n_transactions_attempted.load(std::memory_order_relaxed) == 0; ++i) {
        counter.value = 0;
        tbb::parallel_for(tbb::blocked_range<std::size_t>(0, n, 2), AddOne_CheckTransaction<Counter<M>>(counter));
        REQUIRE(counter.value == n);
    }
    REQUIRE_MESSAGE(n_transactions_attempted.load(std::memory_order_relaxed), "ERROR for " << name << ": transactions were never attempted");
}


//! \brief \ref error_guessing
TEST_CASE("Transaction test") {
    if(have_TSX()) {
        TestTransaction<tbb::speculative_spin_mutex>("Speculative Spin Mutex");
        TestTransaction<tbb::speculative_spin_rw_mutex>("Speculative Spin RW Mutex");
    }
}
#endif /* __TBB_TSX_TESTING_ENABLED_FOR_THIS_COMPILER */

namespace test_with_native_threads {

template <typename M>
struct Counter {
    using mutex_type = M;

    M mutex;
    volatile long value;

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
    volatile long value[N];

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

    void operator()( int ) const {
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

//! \brief \ref error_guessing
TEST_CASE("test upgrade/downgrade with spin_rw_mutex") {
    test_rwm_upgrade_downgrade<tbb::spin_rw_mutex>();
}

//! \brief \ref error_guessing
TEST_CASE("test upgrade/downgrade with queueing_rw_mutex") {
    test_rwm_upgrade_downgrade<tbb::queuing_rw_mutex>();
}

//! \brief \ref error_guessing
TEST_CASE("test upgrade/downgrade with speculative_spin_rw_mutex") {
    test_rwm_upgrade_downgrade<tbb::speculative_spin_rw_mutex>();
}

//! \brief \ref error_guessing
TEST_CASE("test spin_mutex with native threads") {
    test_with_native_threads::test<tbb::spin_mutex>();
}

//! \brief \ref error_guessing
TEST_CASE("test queuing_mutex with native threads") {
    test_with_native_threads::test<tbb::queuing_mutex>();
}

//! \brief \ref error_guessing
TEST_CASE("test spin_rw_mutex with native threads") {
    test_with_native_threads::test<tbb::spin_rw_mutex>();
    test_with_native_threads::test_rw<tbb::spin_rw_mutex>();
}

//! \brief \ref error_guessing
TEST_CASE("test queuing_rw_mutex with native threads") {
    test_with_native_threads::test<tbb::queuing_rw_mutex>();
    test_with_native_threads::test_rw<tbb::queuing_rw_mutex>();
}
