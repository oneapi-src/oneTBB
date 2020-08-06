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

#define __TBB_EXTRA_DEBUG 1
#include "common/spin_barrier.h"
#include "common/utils.h"
#include "common/utils_report.h"

#include "tbb/task_arena.h"
#include "tbb/task_scheduler_observer.h"
#include "tbb/enumerable_thread_specific.h"
#include "tbb/parallel_for.h"
#include "tbb/global_control.h"

#include <stdexcept>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <thread>

//#include "harness_fp.h"

//! \file test_task_arena.cpp
//! \brief Test for [scheduler.task_arena scheduler.task_scheduler_observer] specification

//--------------------------------------------------//
// Test that task_arena::initialize and task_arena::terminate work when doing nothing else.
/* maxthread is treated as the biggest possible concurrency level. */
void InitializeAndTerminate( int maxthread ) {
    for( int i=0; i<200; ++i ) {
        switch( i&3 ) {
            // Arena is created inactive, initialization is always explicit. Lazy initialization is covered by other test functions.
            // Explicit initialization can either keep the original values or change those.
            // Arena termination can be explicit or implicit (in the destructor).
            // TODO: extend with concurrency level checks if such a method is added.
            default: {
                tbb::task_arena arena( std::rand() % maxthread + 1 );
                CHECK_MESSAGE(!arena.is_active(), "arena should not be active until initialized");
                arena.initialize();
                CHECK(arena.is_active());
                arena.terminate();
                CHECK_MESSAGE(!arena.is_active(), "arena should not be active; it was terminated");
                break;
            }
            case 0: {
                tbb::task_arena arena( 1 );
                CHECK_MESSAGE(!arena.is_active(), "arena should not be active until initialized");
                arena.initialize( std::rand() % maxthread + 1 ); // change the parameters
                CHECK(arena.is_active());
                break;
            }
            case 1: {
                tbb::task_arena arena( tbb::task_arena::automatic );
                CHECK(!arena.is_active());
                arena.initialize();
                CHECK(arena.is_active());
                break;
            }
            case 2: {
                tbb::task_arena arena;
                CHECK_MESSAGE(!arena.is_active(), "arena should not be active until initialized");
                arena.initialize( std::rand() % maxthread + 1 );
                CHECK(arena.is_active());
                arena.terminate();
                CHECK_MESSAGE(!arena.is_active(), "arena should not be active; it was terminated");
                break;
            }
        }
    }
}

//--------------------------------------------------//

// Definitions used in more than one test
typedef tbb::blocked_range<int> Range;

// slot_id value: -1 is reserved by current_slot(), -2 is set in on_scheduler_exit() below
static tbb::enumerable_thread_specific<int> local_id, old_id, slot_id(-3);

void ResetTLS() {
    local_id.clear();
    old_id.clear();
    slot_id.clear();
}

class ArenaObserver : public tbb::task_scheduler_observer {
    int myId;               // unique observer/arena id within a test
    int myMaxConcurrency;   // concurrency of the associated arena
    int myNumReservedSlots; // reserved slots in the associated arena
    void on_scheduler_entry( bool is_worker ) override {
        int current_index = tbb::this_task_arena::current_thread_index();
        CHECK(current_index < (myMaxConcurrency > 1 ? myMaxConcurrency : 2));
        if (is_worker) {
            CHECK(current_index >= myNumReservedSlots);
        }
        CHECK_MESSAGE(!old_id.local(), "double call to on_scheduler_entry");
        old_id.local() = local_id.local();
        CHECK_MESSAGE(old_id.local() != myId, "double entry to the same arena");
        local_id.local() = myId;
        slot_id.local() = current_index;
    }
    void on_scheduler_exit( bool /*is_worker*/ ) override {
        CHECK_MESSAGE(local_id.local() == myId, "nesting of arenas is broken");
        CHECK(slot_id.local() == tbb::this_task_arena::current_thread_index());
        slot_id.local() = -2;
        local_id.local() = old_id.local();
        old_id.local() = 0;
    }
public:
    ArenaObserver(tbb::task_arena &a, int maxConcurrency, int numReservedSlots, int id)
        : tbb::task_scheduler_observer(a)
        , myId(id)
        , myMaxConcurrency(maxConcurrency)
        , myNumReservedSlots(numReservedSlots) {
        CHECK(myId);
        observe(true);
    }
    ~ArenaObserver () {
        CHECK_MESSAGE(!old_id.local(), "inconsistent observer state");
    }
};

struct IndexTrackingBody { // Must be used together with ArenaObserver
    void operator() ( const Range& ) const {
        CHECK(slot_id.local() == tbb::this_task_arena::current_thread_index());
        for ( volatile int i = 0; i < 50000; ++i )
            ;
    }
};

struct AsynchronousWork {
    utils::SpinBarrier &my_barrier;
    bool my_is_blocking;
    AsynchronousWork(utils::SpinBarrier &a_barrier, bool blocking = true)
    : my_barrier(a_barrier), my_is_blocking(blocking) {}
    void operator()() const {
        CHECK_MESSAGE(local_id.local() != 0, "not in explicit arena");
        tbb::parallel_for(Range(0,500), IndexTrackingBody(), tbb::simple_partitioner());
        if(my_is_blocking) my_barrier.wait(); // must be asynchronous to master thread
        else my_barrier.signalNoWait();
    }
};

//-----------------------------------------------------------------------------------------//

// Test that task_arenas might be created and used from multiple application threads.
// Also tests arena observers. The parameter p is the index of an app thread running this test.
void TestConcurrentArenasFunc(int idx) {
    // A regression test for observer activation order:
    // check that arena observer can be activated before local observer
    struct LocalObserver : public tbb::task_scheduler_observer {
        LocalObserver() : tbb::task_scheduler_observer() { observe(true); }
    };
    tbb::task_arena a1;
    a1.initialize(1,0);
    ArenaObserver o1(a1, 1, 0, idx*2+1); // the last argument is a "unique" observer/arena id for the test
    CHECK_MESSAGE(o1.is_observing(), "Arena observer has not been activated");
    LocalObserver lo;
    CHECK_MESSAGE(lo.is_observing(), "Local observer has not been activated");
    tbb::task_arena a2(2,1);
    ArenaObserver o2(a2, 2, 1, idx*2+2);
    CHECK_MESSAGE(o2.is_observing(), "Arena observer has not been activated");
    utils::SpinBarrier barrier(2);
    AsynchronousWork work(barrier);
    a1.enqueue(work); // put async work
    barrier.wait();
    a2.enqueue(work); // another work
    a2.execute(work);
    a1.debug_wait_until_empty();
    a2.debug_wait_until_empty();
}

void TestConcurrentArenas(int p) {
    // TODO REVAMP fix with global control
    ResetTLS();
    utils::NativeParallelFor( p, &TestConcurrentArenasFunc );
}
//--------------------------------------------------//
// Test multiple application threads working with a single arena at the same time.
class MultipleMastersPart1 : utils::NoAssign {
    tbb::task_arena &my_a;
    utils::SpinBarrier &my_b1, &my_b2;
public:
    MultipleMastersPart1( tbb::task_arena &a, utils::SpinBarrier &b1, utils::SpinBarrier &b2)
        : my_a(a), my_b1(b1), my_b2(b2) {}
    void operator()(int) const {
        my_a.execute(AsynchronousWork(my_b2, /*blocking=*/false));
        my_b1.wait();
        // A regression test for bugs 1954 & 1971
        my_a.enqueue(AsynchronousWork(my_b2, /*blocking=*/false));
    }
};

class MultipleMastersPart2 : utils::NoAssign {
    tbb::task_arena &my_a;
    utils::SpinBarrier &my_b;
public:
    MultipleMastersPart2( tbb::task_arena &a, utils::SpinBarrier &b) : my_a(a), my_b(b) {}
    void operator()(int) const {
        my_a.execute(AsynchronousWork(my_b, /*blocking=*/false));
    }
};

class MultipleMastersPart3 : utils::NoAssign {
    tbb::task_arena &my_a;
    utils::SpinBarrier &my_b;
    using wait_context = tbb::detail::d1::wait_context;

    struct Runner : NoAssign {
        wait_context& myWait;
        Runner(wait_context& w) : myWait(w) {}
        void operator()() const {
            for ( volatile int i = 0; i < 10000; ++i )
                ;
            myWait.release();
        }
    };

    struct Waiter : NoAssign {
        wait_context& myWait;
        Waiter(wait_context& w) : myWait(w) {}
        void operator()() const {
            tbb::task_group_context ctx;
            tbb::detail::d1::wait(myWait, ctx);
        }
    };

public:
    MultipleMastersPart3(tbb::task_arena &a, utils::SpinBarrier &b)
        : my_a(a), my_b(b) {}
    void operator()(int) const {
        wait_context wait(0);
        my_b.wait(); // increases chances for task_arena initialization contention
        for( int i=0; i<100; ++i) {
            wait.reserve();
            my_a.enqueue(Runner(wait));
            my_a.execute(Waiter(wait));
        }
        my_b.wait();
    }
};

void TestMultipleMasters(int p) {
    {
        ResetTLS();
        tbb::task_arena a(1,0);
        a.initialize();
        ArenaObserver o(a, 1, 0, 1);
        utils::SpinBarrier barrier1(p), barrier2(2*p+1); // each of p threads will submit two tasks signaling the barrier
        NativeParallelFor( p, MultipleMastersPart1(a, barrier1, barrier2) );
        barrier2.wait();
        a.debug_wait_until_empty();
    } {
        ResetTLS();
        tbb::task_arena a(2,1);
        ArenaObserver o(a, 2, 1, 2);
        utils::SpinBarrier barrier(p+2);
        a.enqueue(AsynchronousWork(barrier, /*blocking=*/true)); // occupy the worker, a regression test for bug 1981
        // TODO: buggy test. A worker threads need time to occupy the slot to prevent a master from taking an enqueue task.
        utils::Sleep(10);
        NativeParallelFor( p, MultipleMastersPart2(a, barrier) );
        barrier.wait();
        a.debug_wait_until_empty();
    } {
        // Regression test for the bug 1981 part 2 (task_arena::execute() with wait_for_all for an enqueued task)
        tbb::task_arena a(p,1);
        utils::SpinBarrier barrier(p+1); // for masters to avoid endless waiting at least in some runs
        // "Oversubscribe" the arena by 1 master thread
        NativeParallelFor( p+1, MultipleMastersPart3(a, barrier) );
        a.debug_wait_until_empty();
    }
}

//--------------------------------------------------//
// TODO: explain what TestArenaEntryConsistency does
#include <sstream>
#include <stdexcept>
#include "tbb/detail/_exception.h"
#include "common/fp_control.h"

struct TestArenaEntryBody : FPModeContext {
    std::atomic<int> &my_stage; // each execute increases it
    std::stringstream my_id;
    bool is_caught, is_expected;
    enum { arenaFPMode = 1 };

    TestArenaEntryBody(std::atomic<int> &s, int idx, int i)  // init thread-specific instance
    :   FPModeContext(idx+i)
    ,   my_stage(s)
    ,   is_caught(false)
#if TBB_USE_EXCEPTION
    ,   is_expected( (idx&(1<<i)) != 0 )
#else
    , is_expected(false)
#endif
    {
        my_id << idx << ':' << i << '@';
    }
    void operator()() { // inside task_arena::execute()
        // synchronize with other stages
        int stage = my_stage++;
        int slot = tbb::this_task_arena::current_thread_index();
        CHECK(slot >= 0);
        CHECK(slot <= 1);
        // wait until the third stage is delegated and then starts on slot 0
        while(my_stage < 2+slot) std::this_thread::yield();
        // deduct its entry type and put it into id, it helps to find source of a problem
        my_id << (stage < 3 ? (tbb::this_task_arena::current_thread_index()?
                              "delegated_to_worker" : stage < 2? "direct" : "delegated_to_master")
                            : stage == 3? "nested_same_ctx" : "nested_alien_ctx");
        AssertFPMode(arenaFPMode);
        if (is_expected) {
            TBB_TEST_THROW(std::logic_error(my_id.str()));
        }
        // no code can be put here since exceptions can be thrown
    }
    void on_exception(const char *e) { // outside arena, in catch block
        is_caught = true;
        CHECK(my_id.str() == e);
        assertFPMode();
    }
    void after_execute() { // outside arena and catch block
        CHECK(is_caught == is_expected);
        assertFPMode();
    }
};

class ForEachArenaEntryBody : utils::NoAssign {
    tbb::task_arena &my_a; // expected task_arena(2,1)
    std::atomic<int> &my_stage; // each execute increases it
    int my_idx;

public:
    ForEachArenaEntryBody(tbb::task_arena &a, std::atomic<int> &c)
    : my_a(a), my_stage(c), my_idx(0) {}

    void test(int idx) {
        my_idx = idx;
        my_stage = 0;
        NativeParallelFor(3, *this); // test cross-arena calls
        CHECK(my_stage == 3);
        my_a.execute(*this); // test nested calls
        CHECK(my_stage == 5);
    }

    // task_arena functor for nested tests
    void operator()() const {
        test_arena_entry(3); // in current task group context
        tbb::parallel_for(4, 5, *this); // in different context
    }

    // NativeParallelFor & parallel_for functor
    void operator()(int i) const {
        test_arena_entry(i);
    }

private:
    void test_arena_entry(int i) const {
        GetRoundingMode();
        TestArenaEntryBody scoped_functor(my_stage, my_idx, i);
        GetRoundingMode();
#if TBB_USE_EXCEPTIONS
        try {
            my_a.execute(scoped_functor);
        } catch(std::logic_error &e) {
            scoped_functor.on_exception(e.what());
        } catch(...) { CHECK_MESSAGE(false, "Unexpected exception type"); }
#else
        my_a.execute(scoped_functor);
#endif
        scoped_functor.after_execute();
    }
};

void TestArenaEntryConsistency() {
    tbb::task_arena a(2, 1);
    std::atomic<int> c;
    ForEachArenaEntryBody body(a, c);

    FPModeContext fp_scope(TestArenaEntryBody::arenaFPMode);
    a.initialize(); // capture FP settings to arena
    fp_scope.setNextFPMode();

    for (int i = 0; i < 100; i++) // not less than 32 = 2^5 of entry types
        body.test(i);
}
//--------------------------------------------------
// Test that the requested degree of concurrency for task_arena is achieved in various conditions
class TestArenaConcurrencyBody : utils::NoAssign {
    tbb::task_arena &my_a;
    int my_max_concurrency;
    int my_reserved_slots;
    utils::SpinBarrier *my_barrier;
    utils::SpinBarrier *my_worker_barrier;
public:
    TestArenaConcurrencyBody( tbb::task_arena &a, int max_concurrency, int reserved_slots, utils::SpinBarrier *b = NULL, utils::SpinBarrier *wb = NULL )
    : my_a(a), my_max_concurrency(max_concurrency), my_reserved_slots(reserved_slots), my_barrier(b), my_worker_barrier(wb) {}
    // NativeParallelFor's functor
    void operator()( int ) const {
        CHECK_MESSAGE( local_id.local() == 0, "TLS was not cleaned?" );
        local_id.local() = 1;
        my_a.execute( *this );
    }
    // Arena's functor
    void operator()() const {
        int idx = tbb::this_task_arena::current_thread_index();
        CHECK( idx < (my_max_concurrency > 1 ? my_max_concurrency : 2) );
        CHECK( my_a.max_concurrency() == tbb::this_task_arena::max_concurrency() );
        int max_arena_concurrency = tbb::this_task_arena::max_concurrency();
        CHECK( max_arena_concurrency == my_max_concurrency );
        if ( my_worker_barrier ) {
            if ( local_id.local() == 1 ) {
                // Master thread in a reserved slot
                CHECK_MESSAGE( idx < my_reserved_slots, "Masters are supposed to use only reserved slots in this test" );
            } else {
                // Worker thread
                CHECK( idx >= my_reserved_slots );
                my_worker_barrier->wait();
            }
        } else if ( my_barrier )
            CHECK_MESSAGE( local_id.local() == 1, "Workers are not supposed to enter the arena in this test" );
        if ( my_barrier ) my_barrier->wait();
        else utils::Sleep( 10 );
    }
};

void TestArenaConcurrency( int p, int reserved = 0, int step = 1) {
    for (; reserved <= p; reserved += step) {
        tbb::task_arena a( p, reserved );
        { // Check concurrency with worker & reserved master threads.
            ResetTLS();
            utils::SpinBarrier b( p );
            utils::SpinBarrier wb( p-reserved );
            TestArenaConcurrencyBody test( a, p, reserved, &b, &wb );
            for ( int i = reserved; i < p; ++i )
                a.enqueue( test );
            if ( reserved==1 )
                test( 0 ); // calls execute()
            else
                utils::NativeParallelFor( reserved, test );
            a.debug_wait_until_empty();
        } { // Check if multiple masters alone can achieve maximum concurrency.
            ResetTLS();
            utils::SpinBarrier b( p );
            utils::NativeParallelFor( p, TestArenaConcurrencyBody( a, p, reserved, &b ) );
            a.debug_wait_until_empty();
        } { // Check oversubscription by masters.
#if !_WIN32 || !_WIN64
            // Some C++ implementations allocate 8MB stacks for std::thread on 32 bit platforms
            // that makes impossible to create more than ~500 threads.
            if ( !(sizeof(std::size_t) == 4 && p > 200) )
#endif
#if TBB_TEST_LOW_WORKLOAD
            if ( p <= 16 )
#endif
            {
                ResetTLS();
                utils::NativeParallelFor(2 * p, TestArenaConcurrencyBody(a, p, reserved));
                a.debug_wait_until_empty();
            }
        }
    }
}

void TestConcurrentFunctionality(int min_thread_num = 1, int max_thread_num = 3) {
    InitializeAndTerminate(max_thread_num);
    for (int p = min_thread_num; p <= max_thread_num; ++p) {
        TestConcurrentArenas(p);
        TestMultipleMasters(p);
        TestArenaConcurrency(p);
    }
}

//--------------------------------------------------//
// Test creation/initialization of a task_arena that references an existing arena (aka attach).
// This part of the test uses the knowledge of task_arena internals

struct TaskArenaValidator {
    int my_slot_at_construction;
    const tbb::task_arena& my_arena;
    TaskArenaValidator( const tbb::task_arena& other )
        : my_slot_at_construction(tbb::this_task_arena::current_thread_index())
        , my_arena(other)
    {}
    // Inspect the internal state
    int concurrency() { return my_arena.debug_max_concurrency(); }
    int reserved_for_masters() { return my_arena.debug_reserved_slots(); }

    // This method should be called in task_arena::execute() for a captured arena
    // by the same thread that created the validator.
    void operator()() {
        CHECK_MESSAGE( tbb::this_task_arena::current_thread_index()==my_slot_at_construction,
                "Current thread index has changed since the validator construction" );
    }
};

void ValidateAttachedArena( tbb::task_arena& arena, bool expect_activated,
                            int expect_concurrency, int expect_masters ) {
    CHECK_MESSAGE( arena.is_active()==expect_activated, "Unexpected activation state" );
    if( arena.is_active() ) {
        TaskArenaValidator validator( arena );
        CHECK_MESSAGE( validator.concurrency()==expect_concurrency, "Unexpected arena size" );
        CHECK_MESSAGE( validator.reserved_for_masters()==expect_masters, "Unexpected # of reserved slots" );
        if ( tbb::this_task_arena::current_thread_index() != tbb::task_arena::not_initialized ) {
            CHECK(tbb::this_task_arena::current_thread_index() >= 0);
            // for threads already in arena, check that the thread index remains the same
            arena.execute( validator );
        } else { // not_initialized
            // Test the deprecated method
            CHECK(tbb::this_task_arena::current_thread_index() == -1);
        }

        // Ideally, there should be a check for having the same internal arena object,
        // but that object is not easily accessible for implicit arenas.
    }
}

struct TestAttachBody : utils::NoAssign {
    static thread_local int my_idx; // safe to modify and use within the NativeParallelFor functor
    const int maxthread;
    TestAttachBody( int max_thr ) : maxthread(max_thr) {}

    // The functor body for NativeParallelFor
    void operator()( int idx ) const {
        my_idx = idx;

        int default_threads = tbb::this_task_arena::max_concurrency();

        tbb::task_arena arena = tbb::task_arena( tbb::task_arena::attach() );
        ValidateAttachedArena( arena, false, -1, -1 ); // Nothing yet to attach to

        arena.terminate();
        ValidateAttachedArena( arena, false, -1, -1 );

        // attach to an auto-initialized arena
        tbb::parallel_for(0, 1, [](int) {});

        tbb::task_arena arena2 = tbb::task_arena( tbb::task_arena::attach() );
        ValidateAttachedArena( arena2, true, default_threads, 1 );

        // attach to another task_arena
        arena.initialize( maxthread, std::min(maxthread,idx) );
        arena.execute( *this );
    }

    // The functor body for task_arena::execute above
    void operator()() const {
        tbb::task_arena arena2 = tbb::task_arena( tbb::task_arena::attach() );
        ValidateAttachedArena( arena2, true, maxthread, std::min(maxthread,my_idx) );
    }

    // The functor body for tbb::parallel_for
    void operator()( const Range& r ) const {
        for( int i = r.begin(); i<r.end(); ++i ) {
            tbb::task_arena arena2 = tbb::task_arena( tbb::task_arena::attach() );
            ValidateAttachedArena( arena2, true, tbb::this_task_arena::max_concurrency(), 1 );
        }
    }
};

thread_local int TestAttachBody::my_idx;

void TestAttach( int maxthread ) {
    // Externally concurrent, but no concurrency within a thread
    utils::NativeParallelFor( std::max(maxthread,4), TestAttachBody( maxthread ) );
    // Concurrent within the current arena; may also serve as a stress test
    tbb::parallel_for( Range(0,10000*maxthread), TestAttachBody( maxthread ) );
}

//--------------------------------------------------//

// Test that task_arena::enqueue does not tolerate a non-const functor.
// TODO: can it be reworked as SFINAE-based compile-time check?
struct TestFunctor {
    void operator()() { CHECK_MESSAGE( false, "Non-const operator called" ); }
    void operator()() const { /* library requires this overload only */ }
};

void TestConstantFunctorRequirement() {
    tbb::task_arena a;
    TestFunctor tf;
    a.enqueue( tf );
}

//--------------------------------------------------//

#include "tbb/parallel_reduce.h"
#include "tbb/parallel_invoke.h"

// Test this_task_arena::isolate
namespace TestIsolatedExecuteNS {
    template <typename NestedPartitioner>
    class NestedParFor : utils::NoAssign {
    public:
        NestedParFor() {}
        void operator()() const {
            NestedPartitioner p;
            tbb::parallel_for( 0, 10, utils::DummyBody( 10 ), p );
        }
    };

    template <typename NestedPartitioner>
    class ParForBody : utils::NoAssign {
        bool myOuterIsolation;
        tbb::enumerable_thread_specific<int> &myEts;
        std::atomic<bool> &myIsStolen;
    public:
        ParForBody( bool outer_isolation, tbb::enumerable_thread_specific<int> &ets, std::atomic<bool> &is_stolen )
            : myOuterIsolation( outer_isolation ), myEts( ets ), myIsStolen( is_stolen ) {}
        void operator()( int ) const {
            int &e = myEts.local();
            if ( e++ > 0 ) myIsStolen = true;
            if ( myOuterIsolation )
                NestedParFor<NestedPartitioner>()();
            else
                tbb::this_task_arena::isolate( NestedParFor<NestedPartitioner>() );
            --e;
        }
    };

    template <typename OuterPartitioner, typename NestedPartitioner>
    class OuterParFor : utils::NoAssign {
        bool myOuterIsolation;
        std::atomic<bool> &myIsStolen;
    public:
        OuterParFor( bool outer_isolation, std::atomic<bool> &is_stolen ) : myOuterIsolation( outer_isolation ), myIsStolen( is_stolen ) {}
        void operator()() const {
            tbb::enumerable_thread_specific<int> ets( 0 );
            OuterPartitioner p;
            tbb::parallel_for( 0, 1000, ParForBody<NestedPartitioner>( myOuterIsolation, ets, myIsStolen ), p );
        }
    };

    template <typename OuterPartitioner, typename NestedPartitioner>
    void TwoLoopsTest( bool outer_isolation ) {
        std::atomic<bool> is_stolen;
        is_stolen = false;
        const int max_repeats = 100;
        if ( outer_isolation ) {
            for ( int i = 0; i <= max_repeats; ++i ) {
                tbb::this_task_arena::isolate( OuterParFor<OuterPartitioner, NestedPartitioner>( outer_isolation, is_stolen ) );
                if ( is_stolen ) break;
            }
            // TODO: was ASSERT_WARNING
            if (!is_stolen) {
                REPORT("Warning: isolate() should not block stealing on nested levels without isolation\n");
            }
        } else {
            for ( int i = 0; i <= max_repeats; ++i ) {
                OuterParFor<OuterPartitioner, NestedPartitioner>( outer_isolation, is_stolen )();
            }
            REQUIRE_MESSAGE( !is_stolen, "isolate() on nested levels should prevent stealing from outer leves" );
        }
    }

    void TwoLoopsTest( bool outer_isolation ) {
        TwoLoopsTest<tbb::simple_partitioner, tbb::simple_partitioner>( outer_isolation );
        TwoLoopsTest<tbb::simple_partitioner, tbb::affinity_partitioner>( outer_isolation );
        TwoLoopsTest<tbb::affinity_partitioner, tbb::simple_partitioner>( outer_isolation );
        TwoLoopsTest<tbb::affinity_partitioner, tbb::affinity_partitioner>( outer_isolation );
    }

    void TwoLoopsTest() {
        TwoLoopsTest( true );
        TwoLoopsTest( false );
    }
    //--------------------------------------------------//
    class HeavyMixTestBody : utils::NoAssign {
        tbb::enumerable_thread_specific<utils::FastRandom<>>& myRandom;
        tbb::enumerable_thread_specific<int>& myIsolatedLevel;
        int myNestedLevel;

        template <typename Partitioner, typename Body>
        static void RunTwoBodies( utils::FastRandom<>& rnd, const Body &body, Partitioner& p, tbb::task_group_context* ctx = NULL ) {
            if ( rnd.get() % 2 ) {
                if  (ctx )
                    tbb::parallel_for( 0, 2, body, p, *ctx );
                else
                    tbb::parallel_for( 0, 2, body, p );
            } else {
                tbb::parallel_invoke( body, body );
            }
        }

        template <typename Partitioner>
        class IsolatedBody : utils::NoAssign {
            const HeavyMixTestBody &myHeavyMixTestBody;
            Partitioner &myPartitioner;
        public:
            IsolatedBody( const HeavyMixTestBody &body, Partitioner &partitioner )
                : myHeavyMixTestBody( body ), myPartitioner( partitioner ) {}
            void operator()() const {
                RunTwoBodies( myHeavyMixTestBody.myRandom.local(),
                    HeavyMixTestBody( myHeavyMixTestBody.myRandom, myHeavyMixTestBody.myIsolatedLevel,
                        myHeavyMixTestBody.myNestedLevel + 1 ),
                    myPartitioner );
            }
        };

        template <typename Partitioner>
        void RunNextLevel( utils::FastRandom<>& rnd, int &isolated_level ) const {
            Partitioner p;
            switch ( rnd.get() % 2 ) {
                case 0: {
                    // No features
                    tbb::task_group_context ctx;
                    RunTwoBodies( rnd, HeavyMixTestBody(myRandom, myIsolatedLevel, myNestedLevel + 1), p, &ctx );
                    break;
                }
                case 1: {
                    // Isolation
                    int previous_isolation = isolated_level;
                    isolated_level = myNestedLevel;
                    tbb::this_task_arena::isolate( IsolatedBody<Partitioner>( *this, p ) );
                    isolated_level = previous_isolation;
                    break;
                }
            }
        }
    public:
        HeavyMixTestBody( tbb::enumerable_thread_specific<utils::FastRandom<>>& random,
            tbb::enumerable_thread_specific<int>& isolated_level, int nested_level )
            : myRandom( random ), myIsolatedLevel( isolated_level )
            , myNestedLevel( nested_level ) {}
        void operator()() const {
            int &isolated_level = myIsolatedLevel.local();
            REQUIRE_MESSAGE( myNestedLevel > isolated_level, "The outer-level task should not be stolen on isolated level" );
            if ( myNestedLevel == 20 )
                return;
            utils::FastRandom<>& rnd = myRandom.local();
            if ( rnd.get() % 2 == 1 ) {
                RunNextLevel<tbb::auto_partitioner>( rnd, isolated_level );
            } else {
                RunNextLevel<tbb::affinity_partitioner>( rnd, isolated_level );
            }
        }
        void operator()(int) const {
            this->operator()();
        }
    };

    struct RandomInitializer {
        utils::FastRandom<> operator()() {
            return utils::FastRandom<>( tbb::this_task_arena::current_thread_index() );
        }
    };

    void HeavyMixTest() {
        std::size_t num_threads = tbb::this_task_arena::max_concurrency() < 3 ? 3 : tbb::this_task_arena::max_concurrency();
        tbb::global_control ctl(tbb::global_control::max_allowed_parallelism, num_threads);

        RandomInitializer init_random;
        tbb::enumerable_thread_specific<utils::FastRandom<>> random( init_random );
        tbb::enumerable_thread_specific<int> isolated_level( 0 );
        for ( int i = 0; i < 5; ++i ) {
            HeavyMixTestBody b( random, isolated_level, 1 );
            b( 0 );
        }
    }

    //--------------------------------------------------//
#if TBB_USE_EXCEPTIONS
    struct MyException {};
    struct IsolatedBodyThrowsException {
        void operator()() const {
#if _MSC_VER && !__INTEL_COMPILER
            // Workaround an unreachable code warning in task_arena_function.
            volatile bool workaround = true;
            if (workaround)
#endif
            {
                throw MyException();
            }
        }
    };
    struct ExceptionTestBody : utils::NoAssign {
        tbb::enumerable_thread_specific<int>& myEts;
        std::atomic<bool>& myIsStolen;
        ExceptionTestBody( tbb::enumerable_thread_specific<int>& ets, std::atomic<bool>& is_stolen )
            : myEts( ets ), myIsStolen( is_stolen ) {}
        void operator()( int i ) const {
            try {
                tbb::this_task_arena::isolate( IsolatedBodyThrowsException() );
                REQUIRE_MESSAGE( false, "The exception has been lost" );
            }
            catch ( MyException ) {}
            catch ( ... ) {
                REQUIRE_MESSAGE( false, "Unexpected exception" );
            }
            // Check that nested algorithms can steal outer-level tasks
            int &e = myEts.local();
            if ( e++ > 0 ) myIsStolen = true;
            // work imbalance increases chances for stealing
            tbb::parallel_for( 0, 10+i, utils::DummyBody( 100 ) );
            --e;
        }
    };

#endif /* TBB_USE_EXCEPTIONS */
    void ExceptionTest() {
#if TBB_USE_EXCEPTIONS
        tbb::enumerable_thread_specific<int> ets;
        std::atomic<bool> is_stolen;
        is_stolen = false;
        for ( int i = 0; i<10; ++i ) {
            tbb::parallel_for( 0, 1000, ExceptionTestBody( ets, is_stolen ) );
            if ( is_stolen ) break;
        }
        REQUIRE_MESSAGE( is_stolen, "isolate should not affect non-isolated work" );
#endif /* TBB_USE_EXCEPTIONS */
    }

    struct NonConstBody {
        unsigned int state;
        void operator()() {
            state ^= ~0u;
        }
    };

    void TestNonConstBody() {
        NonConstBody body;
        body.state = 0x6c97d5ed;
        tbb::this_task_arena::isolate(body);
        REQUIRE_MESSAGE(body.state == 0x93682a12, "The wrong state");
    }

    // TODO: Consider tbb::task_group instead of explicit task API.
    class TestEnqueueTask : public tbb::detail::d1::task {
        using wait_context = tbb::detail::d1::wait_context;

        tbb::enumerable_thread_specific<bool>& executed;
        std::atomic<int>& completed;

    public:
        wait_context& waiter;
        tbb::task_arena& arena;
        static const int N = 100;

        TestEnqueueTask(tbb::enumerable_thread_specific<bool>& exe, std::atomic<int>& c, wait_context& w, tbb::task_arena& a)
            : executed(exe), completed(c), waiter(w), arena(a) {}

        tbb::detail::d1::task* execute(tbb::detail::d1::execution_data&) override {
            for (int i = 0; i < N; ++i) {
                arena.enqueue([&]() {
                    executed.local() = true;
                    ++completed;
                    for (volatile int j = 0; j < 100; j++) std::this_thread::yield();
                    waiter.release(1);
                });
            }
            return nullptr;
        }
        tbb::detail::d1::task* cancel(tbb::detail::d1::execution_data&) override { return nullptr; }
    };

    class TestEnqueueIsolateBody : utils::NoCopy {
        tbb::enumerable_thread_specific<bool>& executed;
        std::atomic<int>& completed;
        tbb::task_arena& arena;
    public:
        static const int N = 100;

        TestEnqueueIsolateBody(tbb::enumerable_thread_specific<bool>& exe, std::atomic<int>& c, tbb::task_arena& a)
            : executed(exe), completed(c), arena(a) {}
        void operator()() {
            tbb::task_group_context ctx;
            tbb::detail::d1::wait_context waiter(N);

            TestEnqueueTask root(executed, completed, waiter, arena);
            tbb::detail::d1::execute_and_wait(root, ctx, waiter, ctx);
        }
    };

    void TestEnqueue() {
        tbb::enumerable_thread_specific<bool> executed(false);
        std::atomic<int> completed;
        tbb::task_arena arena = tbb::task_arena(tbb::task_arena::attach());

        // Check that the main thread can process enqueued tasks.
        completed = 0;
        TestEnqueueIsolateBody b1(executed, completed, arena);
        b1();

        if (!executed.local()) {
            REPORT("Warning: No one enqueued task has executed by the main thread.\n");
        }

        executed.local() = false;
        completed = 0;
        const int N = 100;
        // Create enqueued tasks out of isolation.

        tbb::task_group_context ctx;
        tbb::detail::d1::wait_context waiter(N);
        for (int i = 0; i < N; ++i) {
            arena.enqueue([&]() {
                executed.local() = true;
                ++completed;
                std::this_thread::yield();
                waiter.release(1);
            });
        }
        TestEnqueueIsolateBody b2(executed, completed, arena);
        tbb::this_task_arena::isolate(b2);
        REQUIRE_MESSAGE(executed.local() == false, "An enqueued task was executed within isolate.");

        tbb::detail::d1::wait(waiter, ctx);
        // while (completed < TestEnqueueTask::N + N) std::this_thread::yield();
    }
}

void TestIsolatedExecute() {
    // At least 3 threads (owner + 2 thieves) are required to reproduce a situation when the owner steals outer
    // level task on a nested level. If we have only one thief then it will execute outer level tasks first and
    // the owner will not have a possibility to steal outer level tasks.
    int platform_max_thread = tbb::this_task_arena::max_concurrency();
    int num_threads = utils::min( platform_max_thread, 3 );
    {
        // Too many threads require too many work to reproduce the stealing from outer level.
        tbb::global_control ctl(tbb::global_control::max_allowed_parallelism, utils::max(num_threads, 7));
        TestIsolatedExecuteNS::TwoLoopsTest();
        TestIsolatedExecuteNS::HeavyMixTest();
        TestIsolatedExecuteNS::ExceptionTest();
    }
    tbb::global_control ctl(tbb::global_control::max_allowed_parallelism, num_threads);
    TestIsolatedExecuteNS::HeavyMixTest();
    TestIsolatedExecuteNS::TestNonConstBody();
    TestIsolatedExecuteNS::TestEnqueue();
}

//-----------------------------------------------------------------------------------------//

class TestDelegatedSpawnWaitBody : utils::NoAssign {
    tbb::task_arena &my_a;
    utils::SpinBarrier &my_b1, &my_b2;
public:
    TestDelegatedSpawnWaitBody( tbb::task_arena &a, utils::SpinBarrier &b1, utils::SpinBarrier &b2)
        : my_a(a), my_b1(b1), my_b2(b2) {}
    // NativeParallelFor's functor
    void operator()(int idx) const {
        if ( idx==0 ) { // thread 0 works in the arena, thread 1 waits for it (to prevent test hang)
            for (int i = 0; i < 2; ++i) {
                my_a.enqueue([this] { my_b1.wait(); }); // tasks to sync with workers
            }
            tbb::task_group tg;
            my_b1.wait(); // sync with the workers
            for( int i=0; i<100000; ++i) {
                my_a.execute([&tg] { tg.run([] {}); });
            }
            my_a.execute([&tg] {tg.wait(); });
        }

        my_b2.wait(); // sync both threads
    }
};

void TestDelegatedSpawnWait() {
    // Regression test for a bug with missed wakeup notification from a delegated task
    tbb::task_arena a(2,0);
    a.initialize();
    utils::SpinBarrier barrier1(3), barrier2(2);
    utils::NativeParallelFor( 2, TestDelegatedSpawnWaitBody(a, barrier1, barrier2) );
    a.debug_wait_until_empty();
}

//-----------------------------------------------------------------------------------------//

class TestMultipleWaitsArenaWait : utils::NoAssign {
    using wait_context = tbb::detail::d1::wait_context;
public:
    TestMultipleWaitsArenaWait( int idx, int bunch_size, int num_tasks, std::vector<wait_context*>& waiters, std::atomic<int>& processed, tbb::task_group_context& tgc )
        : my_idx( idx ), my_bunch_size( bunch_size ), my_num_tasks(num_tasks), my_waiters( waiters ), my_processed( processed ), my_context(tgc) {}
    void operator()() const {
        ++my_processed;
        // Wait for all tasks
        if ( my_idx < my_num_tasks ) {
            tbb::detail::d1::wait(*my_waiters[my_idx], my_context);
        }
        // Signal waiting tasks
        if ( my_idx >= my_bunch_size ) {
            my_waiters[my_idx-my_bunch_size]->release();
        }
    }
private:
    int my_idx;
    int my_bunch_size;
    int my_num_tasks;
    std::vector<wait_context*>& my_waiters;
    std::atomic<int>& my_processed;
    tbb::task_group_context& my_context;
};

class TestMultipleWaitsThreadBody : utils::NoAssign {
    using wait_context = tbb::detail::d1::wait_context;
public:
    TestMultipleWaitsThreadBody( int bunch_size, int num_tasks, tbb::task_arena& a, std::vector<wait_context*>& waiters, std::atomic<int>& processed, tbb::task_group_context& tgc )
        : my_bunch_size( bunch_size ), my_num_tasks( num_tasks ), my_arena( a ), my_waiters( waiters ), my_processed( processed ), my_context(tgc) {}
    void operator()( int idx ) const {
        my_arena.execute( TestMultipleWaitsArenaWait( idx, my_bunch_size, my_num_tasks, my_waiters, my_processed, my_context ) );
        --my_processed;
    }
private:
    int my_bunch_size;
    int my_num_tasks;
    tbb::task_arena& my_arena;
    std::vector<wait_context*>& my_waiters;
    std::atomic<int>& my_processed;
    tbb::task_group_context& my_context;
};

void TestMultipleWaits( int num_threads, int num_bunches, int bunch_size ) {
    tbb::task_arena a( num_threads );
    const int num_tasks = (num_bunches-1)*bunch_size;

    tbb::task_group_context tgc;
    std::vector<tbb::detail::d1::wait_context*> waiters(num_tasks);
    for (auto& w : waiters) w = new tbb::detail::d1::wait_context(0);

    std::atomic<int> processed(0);
    for ( int repeats = 0; repeats<10; ++repeats ) {
        int idx = 0;
        for ( int bunch = 0; bunch < num_bunches-1; ++bunch ) {
            // Sync with the previous bunch of waiters to prevent "false" nested dependicies (when a nested task waits for an outer task).
            while ( processed < bunch*bunch_size ) std::this_thread::yield();
            // Run the bunch of threads/waiters that depend on the next bunch of threads/waiters.
            for ( int i = 0; i<bunch_size; ++i ) {
                waiters[idx]->reserve();
                std::thread( TestMultipleWaitsThreadBody( bunch_size, num_tasks, a, waiters, processed, tgc ), idx++ ).detach();
            }
        }
        // No sync because the threads of the last bunch do not call wait_for_all.
        // Run the last bunch of threads.
        for ( int i = 0; i<bunch_size; ++i )
            std::thread( TestMultipleWaitsThreadBody( bunch_size, num_tasks, a, waiters, processed, tgc ), idx++ ).detach();
        while ( processed ) std::this_thread::yield();
    }
    for (auto w : waiters) delete w;
}

void TestMultipleWaits() {
    // Limit the number of threads to prevent heavy oversubscription.
#if TBB_TEST_LOW_WORKLOAD
    const int max_threads = std::min( 4, tbb::this_task_arena::max_concurrency() );
#else
    const int max_threads = std::min( 16, tbb::this_task_arena::max_concurrency() );
#endif

    utils::FastRandom<> rnd(1234);
    for ( int threads = 1; threads <= max_threads; threads += utils::max( threads/2, 1 ) ) {
        for ( int i = 0; i<3; ++i ) {
            const int num_bunches = 3 + rnd.get()%3;
            const int bunch_size = max_threads + rnd.get()%max_threads;
            TestMultipleWaits( threads, num_bunches, bunch_size );
        }
    }
}

//--------------------------------------------------//

void TestSmallStackSize() {
    tbb::global_control gc(tbb::global_control::thread_stack_size,
            tbb::global_control::active_value(tbb::global_control::thread_stack_size) / 2 );
    // The test produces the warning (not a error) if fails. So the test is run many times
    // to make the log annoying (to force to consider it as an error).
    for (int i = 0; i < 100; ++i) {
        tbb::task_arena a;
        a.initialize();
    }
}

//--------------------------------------------------//

namespace TestMoveSemanticsNS {
    struct TestFunctor {
        void operator()() const {};
    };

    struct MoveOnlyFunctor : utils::MoveOnly, TestFunctor {
        MoveOnlyFunctor() : utils::MoveOnly() {};
        MoveOnlyFunctor(MoveOnlyFunctor&& other) : utils::MoveOnly(std::move(other)) {};
    };

    struct MovePreferableFunctor : utils::Movable, TestFunctor {
        MovePreferableFunctor() : utils::Movable() {};
        MovePreferableFunctor(MovePreferableFunctor&& other) : utils::Movable( std::move(other) ) {};
        MovePreferableFunctor(const MovePreferableFunctor& other) : utils::Movable(other) {};
    };

    struct NoMoveNoCopyFunctor : utils::NoCopy, TestFunctor {
        NoMoveNoCopyFunctor() : utils::NoCopy() {};
        // mv ctor is not allowed as cp ctor from parent NoCopy
    private:
        NoMoveNoCopyFunctor(NoMoveNoCopyFunctor&&);
    };

    void TestFunctors() {
        tbb::task_arena ta;
        MovePreferableFunctor mpf;
        // execute() doesn't have any copies or moves of arguments inside the impl
        ta.execute( NoMoveNoCopyFunctor() );

        ta.enqueue( MoveOnlyFunctor() );
        ta.enqueue( mpf );
        REQUIRE_MESSAGE(mpf.alive, "object was moved when was passed by lval");
        mpf.Reset();
        ta.enqueue( std::move(mpf) );
        REQUIRE_MESSAGE(!mpf.alive, "object was copied when was passed by rval");
        mpf.Reset();
    }
}

void TestMoveSemantics() {
    TestMoveSemanticsNS::TestFunctors();
}

//--------------------------------------------------//

#include <vector>

#include "common/state_trackable.h"

namespace TestReturnValueNS {
    struct noDefaultTag {};
    class ReturnType : public StateTrackable<> {
        static const int SIZE = 42;
        std::vector<int> data;
    public:
        ReturnType(noDefaultTag) : StateTrackable<>(0) {}
        // Define copy constructor to test that it is never called
        ReturnType(const ReturnType& r) : StateTrackable<>(r), data(r.data) {}
        ReturnType(ReturnType&& r) : StateTrackable<>(std::move(r)), data(std::move(r.data)) {}

        void fill() {
            for (int i = 0; i < SIZE; ++i)
                data.push_back(i);
        }
        void check() {
            REQUIRE(data.size() == unsigned(SIZE));
            for (int i = 0; i < SIZE; ++i)
                REQUIRE(data[i] == i);
            StateTrackableCounters::counters_type& cnts = StateTrackableCounters::counters;
            REQUIRE(cnts[StateTrackableBase::DefaultInitialized] == 0);
            REQUIRE(cnts[StateTrackableBase::DirectInitialized] == 1);
            std::size_t copied = cnts[StateTrackableBase::CopyInitialized];
            std::size_t moved = cnts[StateTrackableBase::MoveInitialized];
            REQUIRE(cnts[StateTrackableBase::Destroyed] == copied + moved);
            // The number of copies/moves should not exceed 3: function return, store to an internal storage,
            // acquire internal storage.
            REQUIRE((copied == 0 && moved <=3));
        }
    };

    template <typename R>
    R function() {
        noDefaultTag tag;
        R r(tag);
        r.fill();
        return r;
    }

    template <>
    void function<void>() {}

    template <typename R>
    struct Functor {
        R operator()() const {
            return function<R>();
        }
    };

    tbb::task_arena& arena() {
        static tbb::task_arena a;
        return a;
    }

    template <typename F>
    void TestExecute(F &f) {
        StateTrackableCounters::reset();
        ReturnType r = arena().execute(f);
        r.check();
    }

    template <typename F>
    void TestExecute(const F &f) {
        StateTrackableCounters::reset();
        ReturnType r = arena().execute(f);
        r.check();
    }
    template <typename F>
    void TestIsolate(F &f) {
        StateTrackableCounters::reset();
        ReturnType r = tbb::this_task_arena::isolate(f);
        r.check();
    }

    template <typename F>
    void TestIsolate(const F &f) {
        StateTrackableCounters::reset();
        ReturnType r = tbb::this_task_arena::isolate(f);
        r.check();
    }

    void Test() {
        TestExecute(Functor<ReturnType>());
        Functor<ReturnType> f1;
        TestExecute(f1);
        TestExecute(function<ReturnType>);

        arena().execute(Functor<void>());
        Functor<void> f2;
        arena().execute(f2);
        arena().execute(function<void>);
        TestIsolate(Functor<ReturnType>());
        TestIsolate(f1);
        TestIsolate(function<ReturnType>);
        tbb::this_task_arena::isolate(Functor<void>());
        tbb::this_task_arena::isolate(f2);
        tbb::this_task_arena::isolate(function<void>);
    }
}

void TestReturnValue() {
    TestReturnValueNS::Test();
}

//--------------------------------------------------//

// MyObserver checks if threads join to the same arena
struct MyObserver: public tbb::task_scheduler_observer {
    tbb::enumerable_thread_specific<tbb::task_arena*>& my_tls;
    tbb::task_arena& my_arena;
    std::atomic<int>& my_failure_counter;
    std::atomic<int>& my_counter;

    MyObserver(tbb::task_arena& a,
        tbb::enumerable_thread_specific<tbb::task_arena*>& tls,
        std::atomic<int>& failure_counter,
        std::atomic<int>& counter)
        : tbb::task_scheduler_observer(a), my_tls(tls), my_arena(a),
        my_failure_counter(failure_counter), my_counter(counter) {
        observe(true);
    }
    void on_scheduler_entry(bool worker) override {
        if (worker) {
            ++my_counter;
            tbb::task_arena*& cur_arena = my_tls.local();
            if (cur_arena != 0 && cur_arena != &my_arena) {
                ++my_failure_counter;
            }
            cur_arena = &my_arena;
        }
    }
};

struct MyLoopBody {
    utils::SpinBarrier& m_barrier;
    MyLoopBody(utils::SpinBarrier& b):m_barrier(b) { }
    void operator()(int) const {
        m_barrier.wait();
    }
};

struct TaskForArenaExecute {
    utils::SpinBarrier& m_barrier;
    TaskForArenaExecute(utils::SpinBarrier& b):m_barrier(b) { }
    void operator()() const {
         tbb::parallel_for(0, tbb::this_task_arena::max_concurrency(),
             MyLoopBody(m_barrier), tbb::simple_partitioner()
         );
    }
};

struct ExecuteParallelFor {
    int n_per_thread;
    int n_repetitions;
    std::vector<tbb::task_arena>& arenas;
    utils::SpinBarrier& arena_barrier;
    utils::SpinBarrier& master_barrier;
    ExecuteParallelFor(const int n_per_thread_, const int n_repetitions_,
        std::vector<tbb::task_arena>& arenas_,
        utils::SpinBarrier& arena_barrier_, utils::SpinBarrier& master_barrier_)
            : n_per_thread(n_per_thread_), n_repetitions(n_repetitions_), arenas(arenas_),
              arena_barrier(arena_barrier_), master_barrier(master_barrier_){ }
    void operator()(int i) const {
        for (int j = 0; j < n_repetitions; ++j) {
            arenas[i].execute(TaskForArenaExecute(arena_barrier));
            for(volatile int k = 0; k < n_per_thread; ++k){/* waiting until workers fall asleep */}
            master_barrier.wait();
        }
    }
};

// if n_threads == -1 then global_control initialized with default value
void TestArenaWorkersMigrationWithNumThreads(int n_threads = 0) {
    if (n_threads == 0) {
        n_threads = tbb::this_task_arena::max_concurrency();
    }
    const int max_n_arenas = 8;
    int n_arenas = 2;
    if(n_threads >= 16)
        n_arenas = max_n_arenas;
    else if (n_threads >= 8)
        n_arenas = 4;
    n_threads = n_arenas * (n_threads / n_arenas);
    const int n_per_thread = 10000000;
    const int n_repetitions = 100;
    const int n_outer_repetitions = 20;
    std::multiset<float> failure_ratio; // for median calculating
    tbb::global_control control(tbb::global_control::max_allowed_parallelism, n_threads - (n_arenas - 1));
    utils::SpinBarrier master_barrier(n_arenas);
    utils::SpinBarrier arena_barrier(n_threads);
    MyObserver* observer[max_n_arenas];
    std::vector<tbb::task_arena> arenas(n_arenas);
    std::atomic<int> failure_counter;
    std::atomic<int> counter;
    tbb::enumerable_thread_specific<tbb::task_arena*> tls;
    for (int i = 0; i < n_arenas; ++i) {
        arenas[i].initialize(n_threads / n_arenas);
        observer[i] = new MyObserver(arenas[i], tls, failure_counter, counter);
    }
    int ii = 0;
    for (; ii < n_outer_repetitions; ++ii) {
        failure_counter = 0;
        counter = 0;
        // Main code
        utils::NativeParallelFor(n_arenas, ExecuteParallelFor(n_per_thread, n_repetitions,
            arenas, arena_barrier, master_barrier));
        // TODO: get rid of check below by setting ratio between n_threads and n_arenas
        failure_ratio.insert((counter != 0 ? float(failure_counter) / counter : 1.0f));
        tls.clear();
        // collect 3 elements in failure_ratio before calculating median
        if (ii > 1) {
            std::multiset<float>::iterator it = failure_ratio.begin();
            std::advance(it, failure_ratio.size() / 2);
            if (*it < 0.02)
                break;
        }
    }
    for (int i = 0; i < n_arenas; ++i) {
        delete observer[i];
    }
    // check if median is so big
    std::multiset<float>::iterator it = failure_ratio.begin();
    std::advance(it, failure_ratio.size() / 2);
    // TODO: decrease constants 0.05 and 0.3 by setting ratio between n_threads and n_arenas
    if (*it > 0.05) {
        REPORT("Warning: So many cases when threads join to different arenas.\n");
        REQUIRE_MESSAGE(*it <= 0.3, "A lot of cases when threads join to different arenas.\n");
    }
}

void TestArenaWorkersMigration() {
    TestArenaWorkersMigrationWithNumThreads(4);
    if (tbb::this_task_arena::max_concurrency() != 4) {
        TestArenaWorkersMigrationWithNumThreads();
    }
}

//--------------------------------------------------//
void TestDefaultCreatedWorkersAmount() {
    int threads = tbb::this_task_arena::max_concurrency();
    utils::NativeParallelFor(1, [threads](int idx) {
        REQUIRE_MESSAGE(idx == 0, "more than 1 thread is going to reset TLS");
        utils::SpinBarrier barrier(threads);
        ResetTLS();
        for (int trail = 0; trail < 10; ++trail) {
            tbb::parallel_for(0, threads, [threads, &barrier](int) {
                REQUIRE_MESSAGE(threads == tbb::this_task_arena::max_concurrency(), "concurrency level is not equal specified threadnum");
                REQUIRE_MESSAGE(tbb::this_task_arena::current_thread_index() < tbb::this_task_arena::max_concurrency(), "amount of created threads is more than specified by default");
                local_id.local() = 1;
                // If there is more threads than expected, 'sleep' gives a chance to join unexpected threads.
                utils::Sleep(1);
                barrier.wait();
                }, tbb::simple_partitioner());
            REQUIRE_MESSAGE(local_id.size() == size_t(threads), "amount of created threads is not equal to default num");
        }
    });
}

void TestAbilityToCreateWorkers(int thread_num) {
    tbb::global_control thread_limit(tbb::global_control::max_allowed_parallelism, thread_num);
    // Checks only some part of reserved-master threads amount:
    // 0 and 1 reserved threads are important cases but it is also needed
    // to collect some statistic data with other amount and to not consume
    // whole test sesion time checking each amount
    TestArenaConcurrency(thread_num - 1, 0, int(thread_num / 2.72));
    TestArenaConcurrency(thread_num, 1, int(thread_num / 3.14));
}

void TestDefaultWorkersLimit() {
    TestDefaultCreatedWorkersAmount();
#if TBB_TEST_LOW_WORKLOAD
    TestAbilityToCreateWorkers(24);
#else
    TestAbilityToCreateWorkers(256);
#endif
}

//--------------------------------------------------//

//! Test for task arena in concurrent cases
//! \brief \ref requirement
TEST_CASE("Test for concurrent functionality") {
    TestConcurrentFunctionality();
}

//! Test for arena entry consistency
//! \brief \ref requirement \ref error_guessing
TEST_CASE("Test for task arena entry consistency") {
    TestArenaEntryConsistency();
}

//! Test for task arena attach functionality
//! \brief \ref requirement \ref interface
TEST_CASE("Test for the attach functionality") {
    TestAttach(4);
}

//! Test for constant functor requirements
//! \brief \ref requirement \ref interface
TEST_CASE("Test for constant functor requirement") {
    TestConstantFunctorRequirement();
}

//! Test for move semantics support
//! \brief \ref requirement \ref interface
TEST_CASE("Move semantics support") {
    TestMoveSemantics();
}

//! Test for different return value types
//! \brief \ref requirement \ref interface
TEST_CASE("Return value test") {
    TestReturnValue();
}

//! Test for delegated task spawn in case of unsuccessful slot attach
//! \brief \ref error_guessing
TEST_CASE("Delegated spawn wait") {
    TestDelegatedSpawnWait();
}

//! Test task arena isolation functionality
//! \brief \ref requirement \ref interface
TEST_CASE("Isolated execute") {
    // Isolation tests cases is valid only for more then 2 threads
    if (tbb::this_task_arena::max_concurrency() > 2) {
        TestIsolatedExecute();
    }
}

//! Test for TBB Workers creation limits
//! \brief \ref requirement
TEST_CASE("Default workers limit") {
    TestDefaultWorkersLimit();
}

//! Test for workers migration between arenas
//! \brief \ref error_guessing \ref stress
TEST_CASE("Arena workers migration") {
    TestArenaWorkersMigration();
}

//! Test for multiple waits, threads should not block each other
//! \brief \ref requirement
TEST_CASE("Multiple waits") {
    TestMultipleWaits();
}

//! Test for small stack size settings and arena initialization
//! \brief \ref error_guessing
TEST_CASE("Small stack size") {
    TestSmallStackSize();
}
