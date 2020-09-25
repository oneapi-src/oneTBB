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

#ifndef __TBB_test_common_spin_barrier_H
#define __TBB_test_common_spin_barrier_H

// Do not replace this include by doctest.h
// This headers includes inside benchmark.h and used for benchmarking based on Celero
// Using of both DocTest and Celero frameworks caused unexpected compilation errors.
#include "utils_assert.h"

#include "tbb/detail/_machine.h"
#include "tbb/detail/_utils.h"
#include "tbb/tick_count.h"

#include <atomic>
#include <thread>

namespace utils {

// Spin WHILE the value of the variable is equal to a given value
/* T and U should be comparable types. */
class TimedWaitWhileEq {
    //! Assignment not allowed
    void operator=( const TimedWaitWhileEq& );
    double &my_limit;
public:
    TimedWaitWhileEq(double &n_seconds) : my_limit(n_seconds) {}
    TimedWaitWhileEq(const TimedWaitWhileEq &src) : my_limit(src.my_limit) {}
    template<typename T, typename U>
    void operator()( const std::atomic<T>& location, U value ) const {
        tbb::tick_count start = tbb::tick_count::now();
        double time_passed;
        do {
            time_passed = (tbb::tick_count::now() - start).seconds();
            if( time_passed < 0.0001 ) {
                tbb::detail::machine_pause(10);
            } else {
                std::this_thread::yield();
            }
        } while (time_passed < my_limit && location == value);
        my_limit -= time_passed;
    }
};

//! Spin WHILE the condition is true.
/** T and U should be comparable types. */
template <typename T, typename C>
void SpinWaitWhileCondition(const std::atomic<T>& location, C comp) {
    while (comp(location.load(std::memory_order_acquire))) {
        std::this_thread::yield();
    }
}

//! Spin WHILE the value of the variable is equal to a given value
/** T and U should be comparable types. */
template <typename T, typename U>
void SpinWaitWhileEq(const std::atomic<T>& location, const U value) {
    SpinWaitWhileCondition(location, [&value](T t) { return t == value; });
}

//! Spin UNTIL the value of the variable is equal to a given value
/** T and U should be comparable types. */
template<typename T, typename U>
void SpinWaitUntilEq(const std::atomic<T>& location, const U value) {
    SpinWaitWhileCondition(location, [&value](T t) { return t != value; });
}

class WaitWhileEq {
public:
    //! Assignment not allowed
    void operator=( const WaitWhileEq& ) = delete;

    template<typename T, typename U>
    void operator()( const std::atomic<T>& location, U value ) const {
        SpinWaitWhileEq(location, value);
    }
};

class SpinBarrier {
public:
    using size_type = std::size_t;
private:
    std::atomic<size_type> myNumThreads;
    std::atomic<size_type> myNumThreadsFinished; // reached the barrier in this epoch
    // the number of times the barrier was opened
    std::atomic<size_type> myEpoch;
    // a throwaway barrier can be used only once, then wait() becomes a no-op
    bool myThrowaway;

    struct DummyCallback {
        void operator() () const {}
        template<typename T, typename U>
        void operator()( const T&, U) const {}
    };

public:
    SpinBarrier( const SpinBarrier& ) = delete;    // no copy ctor
    SpinBarrier& operator=( const SpinBarrier& ) = delete; // no assignment

    SpinBarrier( size_type nthreads = 0, bool throwaway = false ) {
        initialize(nthreads, throwaway);
    }

    void initialize( size_type nthreads, bool throwaway = false ) {
        myNumThreads = nthreads;
        myNumThreadsFinished = 0;
        myEpoch = 0;
        myThrowaway = throwaway;
    }

    // Returns whether this thread was the last to reach the barrier.
    // onWaitCallback is called by a thread for waiting;
    // onOpenBarrierCallback is called by the last thread before unblocking other threads.
    template<typename WaitEq, typename Callback>
    bool customWait( const WaitEq& onWaitCallback, const Callback& onOpenBarrierCallback ) {
        if (myThrowaway && myEpoch) {
            return false;
        }

        size_type epoch = myEpoch;
        size_type numThreads = myNumThreads; // read it before the increment
        int threadsLeft = static_cast<int>(numThreads - myNumThreadsFinished++ - 1);
        ASSERT(threadsLeft >= 0,"Broken barrier");
        if (threadsLeft > 0) {
            /* this thread is not the last; wait until the epoch changes & return false */
            onWaitCallback(myEpoch, epoch);
            return false;
        }
        /* This thread is the last one at the barrier in this epoch */
        onOpenBarrierCallback();
        /* reset the barrier, increment the epoch, and return true */
        myNumThreadsFinished -= numThreads;
        threadsLeft = static_cast<int>(myNumThreadsFinished);
        ASSERT(threadsLeft == 0,"Broken barrier");
        /* wakes up threads waiting to exit in this epoch */
        epoch -= myEpoch++;
        ASSERT(epoch == 0,"Broken barrier");
        return true;
    }

    bool timedWaitNoError(double n_seconds) {
        customWait(TimedWaitWhileEq(n_seconds), DummyCallback());
        return n_seconds >= 0.0001;
    }

    bool timedWait(double n_seconds, const char *msg="Time is out while waiting on a barrier") {
        bool is_last = customWait(TimedWaitWhileEq(n_seconds), DummyCallback());
        ASSERT( n_seconds >= 0, msg); // TODO: refactor to avoid passing msg here and rising assertion
        return is_last;
    }

    // onOpenBarrierCallback is called by the last thread before unblocking other threads.
    template<typename Callback>
    bool wait( const Callback& onOpenBarrierCallback ) {
        return customWait(WaitWhileEq(), onOpenBarrierCallback);
    }

    bool wait() {
        return wait(DummyCallback());
    }

    // Signal to the barrier, rather a semaphore functionality.
    bool signalNoWait() {
        return customWait(DummyCallback(), DummyCallback());
    }
};

} // utils

#endif // __TBB_test_common_spin_barrier_H
