/*
    Copyright 2005-2016 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks. Threading Building Blocks is free software;
    you can redistribute it and/or modify it under the terms of the GNU General Public License
    version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
    distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See  the GNU General Public License for more details.   You should have received a copy of
    the  GNU General Public License along with Threading Building Blocks; if not, write to the
    Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA

    As a special exception,  you may use this file  as part of a free software library without
    restriction.  Specifically,  if other files instantiate templates  or use macros or inline
    functions from this file, or you compile this file and link it with other files to produce
    an executable,  this file does not by itself cause the resulting executable to be covered
    by the GNU General Public License. This exception does not however invalidate any other
    reasons why the executable file might be covered by the GNU General Public License.
*/

// undefine __TBB_CPF_BUILD to simulate user's setup
#undef __TBB_CPF_BUILD

#define TBB_PREVIEW_LOCAL_OBSERVER 1
#define __TBB_EXTRA_DEBUG 1

#if !TBB_USE_EXCEPTIONS && _MSC_VER
    // Suppress "C++ exception handler used, but unwind semantics are not enabled" warning in STL headers
    #pragma warning (push)
    #pragma warning (disable: 4530)
#endif

#include <stdexcept>

#if !TBB_USE_EXCEPTIONS && _MSC_VER
    #pragma warning (pop)
#endif

#include <cstdlib>
#include <cstdio>

#include "harness_fp.h"

#include "tbb/task_arena.h"
#include "tbb/task_scheduler_observer.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/enumerable_thread_specific.h"

#include "harness_assert.h"
#include "harness.h"
#include "harness_barrier.h"

#if _MSC_VER
// plays around __TBB_NO_IMPLICIT_LINKAGE. __TBB_LIB_NAME should be defined (in makefiles)
    #pragma comment(lib, __TBB_STRING(__TBB_LIB_NAME))
#endif

//--------------------------------------------------//
// Test that task_arena::initialize and task_arena::terminate work when doing nothing else.
/* maxthread is treated as the biggest possible concurrency level. */
void InitializeAndTerminate( int maxthread ) {
    __TBB_TRY {
        for( int i=0; i<200; ++i ) {
            switch( i&3 ) {
                // Arena is created inactive, initialization is always explicit. Lazy initialization is covered by other test functions.
                // Explicit initialization can either keep the original values or change those.
                // Arena termination can be explicit or implicit (in the destructor).
                // TODO: extend with concurrency level checks if such a method is added.
                default: {
                    tbb::task_arena arena( std::rand() % maxthread + 1 );
                    ASSERT(!arena.is_active(), "arena should not be active until initialized");
                    arena.initialize();
                    ASSERT(arena.is_active(), NULL);
                    arena.terminate();
                    ASSERT(!arena.is_active(), "arena should not be active; it was terminated");
                    break;
                }
                case 0: {
                    tbb::task_arena arena( 1 );
                    ASSERT(!arena.is_active(), "arena should not be active until initialized");
                    arena.initialize( std::rand() % maxthread + 1 ); // change the parameters
                    ASSERT(arena.is_active(), NULL);
                    break;
                }
                case 1: {
                    tbb::task_arena arena( tbb::task_arena::automatic );
                    ASSERT(!arena.is_active(), NULL);
                    arena.initialize();
                    ASSERT(arena.is_active(), NULL);
                    break;
                }
                case 2: {
                    tbb::task_arena arena;
                    ASSERT(!arena.is_active(), "arena should not be active until initialized");
                    arena.initialize( std::rand() % maxthread + 1 );
                    ASSERT(arena.is_active(), NULL);
                    arena.terminate();
                    ASSERT(!arena.is_active(), "arena should not be active; it was terminated");
                    break;
                }
            }
        }
    } __TBB_CATCH( std::runtime_error& error ) {
#if TBB_USE_EXCEPTIONS
        REPORT("ERROR: %s\n", error.what() );
#endif /* TBB_USE_EXCEPTIONS */
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
    /*override*/
    void on_scheduler_entry( bool is_worker ) {
        int current_index = tbb::task_arena::current_thread_index();
        REMARK("a %s #%p is entering arena %d from %d on slot %d\n", is_worker?"worker":"master",
                &local_id.local(), myId, local_id.local(), current_index );
        ASSERT(current_index<(myMaxConcurrency>1?myMaxConcurrency:2), NULL);
        if(is_worker) ASSERT(current_index>=myNumReservedSlots, NULL);

        ASSERT(!old_id.local(), "double call to on_scheduler_entry");
        old_id.local() = local_id.local();
        ASSERT(old_id.local() != myId, "double entry to the same arena");
        local_id.local() = myId;
        slot_id.local() = current_index;
    }
    /*override*/
    void on_scheduler_exit( bool is_worker ) {
        REMARK("a %s #%p is leaving arena %d to %d\n", is_worker?"worker":"master",
                &local_id.local(), myId, old_id.local());
        ASSERT(local_id.local() == myId, "nesting of arenas is broken");
        ASSERT(slot_id.local() == tbb::task_arena::current_thread_index(), NULL);
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
        ASSERT(myId, NULL);
        observe(true);
    }
    ~ArenaObserver () {
        ASSERT(!old_id.local(), "inconsistent observer state");
    }
};

struct IndexTrackingBody { // Must be used together with ArenaObserver
    void operator() ( const Range& ) const {
        ASSERT(slot_id.local() == tbb::task_arena::current_thread_index(), NULL);
        for ( volatile int i = 0; i < 50000; ++i )
            ;
    }
};

struct AsynchronousWork : NoAssign {
    Harness::SpinBarrier &my_barrier;
    bool my_is_blocking;
    AsynchronousWork(Harness::SpinBarrier &a_barrier, bool blocking = true)
    : my_barrier(a_barrier), my_is_blocking(blocking) {}
    void operator()() const {
        ASSERT(local_id.local() != 0, "not in explicit arena");
        tbb::parallel_for(Range(0,500), IndexTrackingBody(), tbb::simple_partitioner(), *tbb::task::self().group());
        if(my_is_blocking) my_barrier.timed_wait(10); // must be asynchronous to master thread
        else my_barrier.signal_nowait();
    }
};

//--------------------------------------------------//
// Test that task_arenas might be created and used from multiple application threads.
// Also tests arena observers. The parameter p is the index of an app thread running this test.
void TestConcurrentArenasFunc(int idx) {
    tbb::task_arena a1;
    a1.initialize(1,0);
    ArenaObserver o1(a1, 1, 0, idx*2+1); // the last argument is a "unique" observer/arena id for the test
    tbb::task_arena a2(2,1);
    ArenaObserver o2(a2, 2, 1, idx*2+2);
    Harness::SpinBarrier barrier(2);
    AsynchronousWork work(barrier);
    a1.enqueue(work); // put async work
    barrier.timed_wait(10);
    a2.enqueue(work); // another work
    a2.execute(work); // my_barrier.timed_wait(10) inside
    a1.debug_wait_until_empty();
    a2.debug_wait_until_empty();
}

void TestConcurrentArenas(int p) {
    ResetTLS();
    NativeParallelFor( p, &TestConcurrentArenasFunc );
}

//--------------------------------------------------//
// Test multiple application threads working with a single arena at the same time.
class MultipleMastersPart1 : NoAssign {
    tbb::task_arena &my_a;
    Harness::SpinBarrier &my_b1, &my_b2;
public:
    MultipleMastersPart1( tbb::task_arena &a, Harness::SpinBarrier &b1, Harness::SpinBarrier &b2)
        : my_a(a), my_b1(b1), my_b2(b2) {}
    void operator()(int) const {
        my_a.execute(AsynchronousWork(my_b2, /*blocking=*/false));
        my_b1.timed_wait(10);
        // A regression test for bugs 1954 & 1971
        my_a.enqueue(AsynchronousWork(my_b2, /*blocking=*/false));
    }
};

class MultipleMastersPart2 : NoAssign {
    tbb::task_arena &my_a;
    Harness::SpinBarrier &my_b;
public:
    MultipleMastersPart2( tbb::task_arena &a, Harness::SpinBarrier &b) : my_a(a), my_b(b) {}
    void operator()(int) const {
        my_a.execute(AsynchronousWork(my_b, /*blocking=*/false));
    }
};

class MultipleMastersPart3 : NoAssign {
    tbb::task_arena &my_a;
    Harness::SpinBarrier &my_b;

    struct Runner : NoAssign {
        tbb::task* const a_task;
        Runner(tbb::task* const t) : a_task(t) {}
        void operator()() const {
            for ( volatile int i = 0; i < 10000; ++i )
                ;
            a_task->decrement_ref_count();
        }
    };

    struct Waiter : NoAssign {
        tbb::task* const a_task;
        Waiter(tbb::task* const t) : a_task(t) {}
        void operator()() const {
            a_task->wait_for_all();
        }
    };

public:
    MultipleMastersPart3(tbb::task_arena &a, Harness::SpinBarrier &b)
        : my_a(a), my_b(b) {}
    void operator()(int idx) const {
        tbb::empty_task* root_task = new(tbb::task::allocate_root()) tbb::empty_task;
        my_b.timed_wait(10); // increases chances for task_arena initialization contention
        for( int i=0; i<100; ++i) {
            root_task->set_ref_count(2);
            my_a.enqueue(Runner(root_task));
            my_a.execute(Waiter(root_task));
        }
        tbb::task::destroy(*root_task);
        REMARK("Master #%d: job completed, wait for others\n", idx);
        my_b.timed_wait(10);
    }
};

class MultipleMastersPart4 : NoAssign {
    tbb::task_arena &my_a;
    Harness::SpinBarrier &my_b;
    tbb::task_group_context *my_ag;

    struct Getter : NoAssign {
        tbb::task_group_context *& my_g;
        Getter(tbb::task_group_context *&a_g) : my_g(a_g) {}
        void operator()() const {
            my_g = tbb::task::self().group();
        }
    };
    struct Checker : NoAssign {
        tbb::task_group_context *my_g;
        Checker(tbb::task_group_context *a_g) : my_g(a_g) {}
        void operator()() const {
            ASSERT(my_g == tbb::task::self().group(), NULL);
            tbb::task *t = new( tbb::task::allocate_root() ) tbb::empty_task;
            ASSERT(my_g == t->group(), NULL);
            tbb::task::destroy(*t);
        }
    };
    struct NestedChecker : NoAssign {
        const MultipleMastersPart4 &my_body;
        NestedChecker(const MultipleMastersPart4 &b) : my_body(b) {}
        void operator()() const {
            tbb::task_group_context *nested_g = tbb::task::self().group();
            ASSERT(my_body.my_ag != nested_g, NULL);
            tbb::task *t = new( tbb::task::allocate_root() ) tbb::empty_task;
            ASSERT(nested_g == t->group(), NULL);
            tbb::task::destroy(*t);
            my_body.my_a.enqueue(Checker(my_body.my_ag));
        }
    };
public:
    MultipleMastersPart4( tbb::task_arena &a, Harness::SpinBarrier &b) : my_a(a), my_b(b) {
        my_a.execute(Getter(my_ag));
    }
    // NativeParallelFor's functor
    void operator()(int) const {
        my_a.execute(*this);
    }
    // Arena's functor
    void operator()() const {
        Checker check(my_ag);
        check();
        tbb::task_arena nested(1,1);
        nested.execute(NestedChecker(*this)); // change arena
        tbb::parallel_for(Range(0,1),*this); // change group context only
        my_b.timed_wait(10);
        my_a.execute(check);
        check();
    }
    // parallel_for's functor
    void operator()(const Range &) const {
        NestedChecker(*this)();
        my_a.execute(Checker(my_ag)); // restore arena context
    }
};

void TestMultipleMasters(int p) {
    {
        REMARK("multiple masters, part 1\n");
        ResetTLS();
        tbb::task_arena a(1,0);
        a.initialize();
        ArenaObserver o(a, 1, 0, 1);
        Harness::SpinBarrier barrier1(p), barrier2(2*p+1); // each of p threads will submit two tasks signaling the barrier
        NativeParallelFor( p, MultipleMastersPart1(a, barrier1, barrier2) );
        barrier2.timed_wait(10);
        a.debug_wait_until_empty();
    } {
        REMARK("multiple masters, part 2\n");
        ResetTLS();
        tbb::task_arena a(2,1);
        ArenaObserver o(a, 2, 1, 2);
        Harness::SpinBarrier barrier(p+2);
        a.enqueue(AsynchronousWork(barrier, /*blocking=*/true)); // occupy the worker, a regression test for bug 1981
        NativeParallelFor( p, MultipleMastersPart2(a, barrier) );
        barrier.timed_wait(10);
        a.debug_wait_until_empty();
    } {
        // Regression test for the bug 1981 part 2 (task_arena::execute() with wait_for_all for an enqueued task)
        REMARK("multiple masters, part 3: wait_for_all() in execute()\n");
        tbb::task_arena a(p,1);
        Harness::SpinBarrier barrier(p+1); // for masters to avoid endless waiting at least in some runs
        // "Oversubscribe" the arena by 1 master thread
        NativeParallelFor( p+1, MultipleMastersPart3(a, barrier) );
        a.debug_wait_until_empty();
    } {
        int c = p%3? (p%2? p : 2) : 3;
        REMARK("multiple masters, part 4: contexts, arena(%d)\n", c);
        ResetTLS();
        tbb::task_arena a(c, 1);
        ArenaObserver o(a, c, 1, c);
        Harness::SpinBarrier barrier(c);
        MultipleMastersPart4 test(a, barrier);
        NativeParallelFor(p, test);
        a.debug_wait_until_empty();
    }
}

//--------------------------------------------------//
// TODO: explain what TestArenaEntryConsistency does
#include <sstream>
#if TBB_USE_EXCEPTIONS
#include <stdexcept>
#include "tbb/tbb_exception.h"
#endif

struct TestArenaEntryBody : FPModeContext {
    tbb::atomic<int> &my_stage; // each execute increases it
    std::stringstream my_id;
    bool is_caught, is_expected;
    enum { arenaFPMode = 1 };

    TestArenaEntryBody(tbb::atomic<int> &s, int idx, int i)  // init thread-specific instance
    :   FPModeContext(idx+i)
    ,   my_stage(s)
    ,   is_caught(false)
    ,   is_expected( (idx&(1<<i)) != 0 && (TBB_USE_EXCEPTIONS) != 0 )
    {
        my_id << idx << ':' << i << '@';
    }
    void operator()() { // inside task_arena::execute()
        // synchronize with other stages
        int stage = my_stage++;
        int slot = tbb::task_arena::current_thread_index();
        ASSERT(slot >= 0 && slot <= 1, "master or the only worker");
        // wait until the third stage is delegated and then starts on slot 0
        while(my_stage < 2+slot) __TBB_Yield();
        // deduct its entry type and put it into id, it helps to find source of a problem
        my_id << (stage < 3 ? (tbb::task_arena::current_thread_index()?
                              "delegated_to_worker" : stage < 2? "direct" : "delegated_to_master")
                            : stage == 3? "nested_same_ctx" : "nested_alien_ctx");
        REMARK("running %s\n", my_id.str().c_str());
        AssertFPMode(arenaFPMode);
        if(is_expected)
            __TBB_THROW(std::logic_error(my_id.str()));
        // no code can be put here since exceptions can be thrown
    }
    void on_exception(const char *e) { // outside arena, in catch block
        is_caught = true;
        REMARK("caught %s\n", e);
        ASSERT(my_id.str() == e, NULL);
        assertFPMode();
    }
    void after_execute() { // outside arena and catch block
        REMARK("completing %s\n", my_id.str().c_str() );
        ASSERT(is_caught == is_expected, NULL);
        assertFPMode();
    }
};

class ForEachArenaEntryBody : NoAssign {
    tbb::task_arena &my_a; // expected task_arena(2,1)
    tbb::atomic<int> &my_stage; // each execute increases it
    int my_idx;

public:
    ForEachArenaEntryBody(tbb::task_arena &a, tbb::atomic<int> &c)
    : my_a(a), my_stage(c), my_idx(0) {}

    void test(int idx) {
        my_idx = idx;
        my_stage = 0;
        NativeParallelFor(3, *this); // test cross-arena calls
        ASSERT(my_stage == 3, NULL);
        my_a.execute(*this); // test nested calls
        ASSERT(my_stage == 5, NULL);
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
        TestArenaEntryBody scoped_functor(my_stage, my_idx, i);
        __TBB_TRY {
            my_a.execute(scoped_functor);
        }
#if TBB_USE_EXCEPTIONS
        catch(tbb::captured_exception &e) {
            scoped_functor.on_exception(e.what());
            ASSERT_WARNING(TBB_USE_CAPTURED_EXCEPTION, "Caught captured_exception while expecting exact one");
        } catch(std::logic_error &e) {
            scoped_functor.on_exception(e.what());
            ASSERT(!TBB_USE_CAPTURED_EXCEPTION, "Caught exception of wrong type");
        } catch(...) { ASSERT(false, "Unexpected exception type"); }
#endif //TBB_USE_EXCEPTIONS
        scoped_functor.after_execute();
    }
};

void TestArenaEntryConsistency() {
    REMARK("test arena entry consistency\n" );

    tbb::task_arena a(2,1);
    tbb::atomic<int> c;
    ForEachArenaEntryBody body(a, c);

    FPModeContext fp_scope(TestArenaEntryBody::arenaFPMode);
    a.initialize(); // capture FP settings to arena
    fp_scope.setNextFPMode();

    for(int i = 0; i < 100; i++) // not less than 32 = 2^5 of entry types
        body.test(i);
}

//--------------------------------------------------
// Test that the requested degree of concurrency for task_arena is achieved in various conditions
class TestArenaConcurrencyBody : NoAssign {
    tbb::task_arena &my_a;
    int my_max_concurrency;
    int my_reserved_slots;
    Harness::SpinBarrier *my_barrier;
    Harness::SpinBarrier *my_worker_barrier;
public:
    TestArenaConcurrencyBody( tbb::task_arena &a, int max_concurrency, int reserved_slots, Harness::SpinBarrier *b = NULL, Harness::SpinBarrier *wb = NULL )
    : my_a(a), my_max_concurrency(max_concurrency), my_reserved_slots(reserved_slots), my_barrier(b), my_worker_barrier(wb) {}
    // NativeParallelFor's functor
    void operator()( int ) const {
        ASSERT( local_id.local() == 0, "TLS was not cleaned?" );
        local_id.local() = 1;
        my_a.execute( *this );
    }
    // Arena's functor
    void operator()() const {
        int idx = tbb::task_arena::current_thread_index();
        ASSERT( idx < (my_max_concurrency > 1 ? my_max_concurrency : 2), NULL );
        if ( my_worker_barrier ) {
            if ( local_id.local() == 1 ) {
                // Master thread in a reserved slot
                ASSERT( idx < my_reserved_slots, "Masters are supposed to use only reserved slots in this test" );
            } else {
                // Worker thread
                ASSERT( idx >= my_reserved_slots, NULL );
                my_worker_barrier->timed_wait( 10 );
            }
        } else if ( my_barrier )
            ASSERT( local_id.local() == 1, "Workers are not supposed to enter the arena in this test" );
        if ( my_barrier ) my_barrier->timed_wait( 10 );
        else Harness::Sleep( 10 );
    }
};

void TestArenaConcurrency( int p ) {
    for ( int reserved = 0; reserved <= p; ++reserved ) {
        REMARK("TestArenaConcurrency: %d slots, %d reserved\n", p, reserved);
        tbb::task_arena a( p, reserved );
        { // Check concurrency with worker & reserved master threads.
            ResetTLS();
            Harness::SpinBarrier b( p );
            Harness::SpinBarrier wb( p-reserved );
            TestArenaConcurrencyBody test( a, p, reserved, &b, &wb );
            for ( int i = reserved; i < p; ++i )
                a.enqueue( test );
            if ( reserved==1 )
                test( 0 ); // calls execute()
            else
                NativeParallelFor( reserved, test );
            a.debug_wait_until_empty();
        } { // Check if multiple masters alone can achieve maximum concurrency.
            ResetTLS();
            Harness::SpinBarrier b( p );
            NativeParallelFor( p, TestArenaConcurrencyBody( a, p, reserved, &b ) );
            a.debug_wait_until_empty();
        } { // Check oversubscription by masters.
            ResetTLS();
            NativeParallelFor( 2*p, TestArenaConcurrencyBody( a, p, reserved ) );
            a.debug_wait_until_empty();
        }
    }
}

//--------------------------------------------------//
// Test creation/initialization of a task_arena that references an existing arena (aka attach).
// This part of the test uses the knowledge of task_arena internals

typedef tbb::interface7::internal::task_arena_base task_arena_internals;

struct TaskArenaValidator : public task_arena_internals {
    int my_slot_at_construction;
    TaskArenaValidator( const task_arena_internals& other )
    : task_arena_internals(other) /*copies the internal state of other*/ {
        my_slot_at_construction = tbb::task_arena::current_thread_index();
    }
    // Inspect the internal state
    int concurrency() { return my_max_concurrency; }
    int reserved_for_masters() { return (int)my_master_slots; }

    // This method should be called in task_arena::execute() for a captured arena
    // by the same thread that created the validator.
    void operator()() {
        ASSERT( tbb::task_arena::current_thread_index()==my_slot_at_construction,
                "Current thread index has changed since the validator construction" );
    }
};

void ValidateAttachedArena( tbb::task_arena& arena, bool expect_activated,
                            int expect_concurrency, int expect_masters ) {
    ASSERT( arena.is_active()==expect_activated, "Unexpected activation state" );
    if( arena.is_active() ) {
        TaskArenaValidator validator( arena );
        ASSERT( validator.concurrency()==expect_concurrency, "Unexpected arena size" );
        ASSERT( validator.reserved_for_masters()==expect_masters, "Unexpected # of reserved slots" );
        if( tbb::task_arena::current_thread_index()!=-1 ) {
            // for threads already in arena, check that the thread index remains the same
            arena.execute( validator );
        }
        // Ideally, there should be a check for having the same internal arena object,
        // but that object is not easily accessible for implicit arenas.
    }
}

struct TestAttachBody : NoAssign {
    mutable int my_idx; // safe to modify and use within the NativeParallelFor functor
    const int maxthread;
    TestAttachBody( int max_thr ) : maxthread(max_thr) {}

    // The functor body for NativeParallelFor
    void operator()( int idx ) const {
        my_idx = idx;
        int default_threads = tbb::task_scheduler_init::default_num_threads();

        tbb::task_arena arena = tbb::task_arena( tbb::task_arena::attach() );
        ValidateAttachedArena( arena, false, -1, -1 ); // Nothing yet to attach to

        { // attach to an arena created via task_scheduler_init
            tbb::task_scheduler_init init( idx+1 );

            tbb::task_arena arena2 = tbb::task_arena( tbb::task_arena::attach() );
            ValidateAttachedArena( arena2, true, idx+1, 1 );

            arena.initialize( tbb::task_arena::attach() );
        }
        ValidateAttachedArena( arena, true, idx+1, 1 );

        arena.terminate();
        ValidateAttachedArena( arena, false, -1, -1 );

        // Check default behavior when attach cannot succeed
        switch (idx%2) {
        case 0:
            { // construct as attached, then initialize
                tbb::task_arena arena2 = tbb::task_arena( tbb::task_arena::attach() );
                ValidateAttachedArena( arena2, false, -1, -1 );
                arena2.initialize(); // must be initialized with default parameters
                ValidateAttachedArena( arena2, true, default_threads, 1 );
            }
            break;
        case 1:
            { // default-construct, then initialize as attached
                tbb::task_arena arena2;
                ValidateAttachedArena( arena2, false, -1, -1 );
                arena2.initialize( tbb::task_arena::attach() ); // must use default parameters
                ValidateAttachedArena( arena2, true, default_threads, 1 );
            }
            break;
        } // switch

        // attach to an auto-initialized arena
        tbb::empty_task& tsk = *new (tbb::task::allocate_root()) tbb::empty_task;
        tbb::task::spawn_root_and_wait(tsk);
        tbb::task_arena arena2 = tbb::task_arena( tbb::task_arena::attach() );
        ValidateAttachedArena( arena2, true, default_threads, 1 );

        // attach to another task_arena
        arena.initialize( maxthread, min(maxthread,idx) );
        arena.execute( *this );
    }

    // The functor body for task_arena::execute above
    void operator()() const {
        tbb::task_arena arena2 = tbb::task_arena( tbb::task_arena::attach() );
        ValidateAttachedArena( arena2, true, maxthread, min(maxthread,my_idx) );
    }

    // The functor body for tbb::parallel_for
    void operator()( const Range& r ) const {
        for( int i = r.begin(); i<r.end(); ++i ) {
            tbb::task_arena arena2 = tbb::task_arena( tbb::task_arena::attach() );
            ValidateAttachedArena( arena2, true, maxthread+1, 1 ); // +1 to match initialization in TestMain
        }
    }
};

void TestAttach( int maxthread ) {
    REMARK( "Testing attached task_arenas\n" );
    // Externally concurrent, but no concurrency within a thread
    NativeParallelFor( max(maxthread,4), TestAttachBody( maxthread ) );
    // Concurrent within the current arena; may also serve as a stress test
    tbb::parallel_for( Range(0,10000*maxthread), TestAttachBody( maxthread ) );
}

//--------------------------------------------------//
// Test that task_arena::enqueue does not tolerate a non-const functor.
// TODO: can it be reworked as SFINAE-based compile-time check?
struct test_functor_t {
    void operator()() { ASSERT( false, "Non-const operator called" ); }
    void operator()() const { /* library requires this overload only */ }
};

void TestConstantFunctorRequirement() {
    tbb::task_arena a;
    test_functor_t tf;
    a.enqueue( tf );
#if __TBB_TASK_PRIORITY
    a.enqueue( tf, tbb::priority_normal );
#endif
}

//--------------------------------------------------//
int TestMain () {
    // The test uses up to MaxThread workers (in arenas with no master thread),
    // so the runtime should be initialized appropriately.
    tbb::task_scheduler_init init_market_p_plus_one(MaxThread+1);
    InitializeAndTerminate(MaxThread);
    for( int p=MinThread; p<=MaxThread; ++p ) {
        REMARK("testing with %d threads\n", p );
        TestConcurrentArenas( p );
        TestMultipleMasters( p );
        TestArenaConcurrency( p );
    }
    TestArenaEntryConsistency();
    TestAttach(MaxThread);
    TestConstantFunctorRequirement();
    return Harness::Done;
}
