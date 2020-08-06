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

#if __TBB_CPF_BUILD
#define TBB_PREVIEW_ISOLATED_TASK_GROUP 1
#endif

#include "common/test.h"
#include "common/utils.h"
#include "tbb/detail/_config.h"
#include "tbb/global_control.h"

#include "tbb/task_group.h"

#include "common/concurrency_tracker.h"

#include <atomic>

//! \file test_task_group.cpp
//! \brief Test for [scheduler.task_group scheduler.task_group_status] specification

unsigned g_MaxConcurrency = 4;
using atomic_t = std::atomic<std::uintptr_t>;
unsigned MinThread = 1;
unsigned MaxThread = 4;

//------------------------------------------------------------------------
// Tests for the thread safety of the task_group manipulations
//------------------------------------------------------------------------

#include "common/spin_barrier.h"

enum SharingMode {
    VagabondGroup = 1,
    ParallelWait = 2
};

template<typename task_group_type>
class SharedGroupBodyImpl : utils::NoCopy, utils::NoAfterlife {
    static const std::uintptr_t c_numTasks0 = 4096,
                        c_numTasks1 = 1024;

    const std::uintptr_t m_numThreads;
    const std::uintptr_t m_sharingMode;

    task_group_type *m_taskGroup;
    atomic_t m_tasksSpawned,
             m_threadsReady;
    utils::SpinBarrier m_barrier;

    static atomic_t s_tasksExecuted;

    struct TaskFunctor {
        SharedGroupBodyImpl *m_pOwner;
        void operator () () const {
            if ( m_pOwner->m_sharingMode & ParallelWait ) {
                while ( utils::ConcurrencyTracker::PeakParallelism() < m_pOwner->m_numThreads )
                    std::this_thread::yield();
            }
            ++s_tasksExecuted;
        }
    };

    TaskFunctor m_taskFunctor;

    void Spawn ( std::uintptr_t numTasks ) {
        for ( std::uintptr_t i = 0; i < numTasks; ++i ) {
            ++m_tasksSpawned;
            utils::ConcurrencyTracker ct;
            m_taskGroup->run( m_taskFunctor );
        }
        ++m_threadsReady;
    }

    void DeleteTaskGroup () {
        delete m_taskGroup;
        m_taskGroup = NULL;
    }

    void Wait () {
        while ( m_threadsReady != m_numThreads )
            std::this_thread::yield();
        const std::uintptr_t numSpawned = c_numTasks0 + c_numTasks1 * (m_numThreads - 1);
        CHECK_MESSAGE( m_tasksSpawned == numSpawned, "Wrong number of spawned tasks. The test is broken" );
        INFO("Max spawning parallelism is " << utils::ConcurrencyTracker::PeakParallelism() << "out of " << g_MaxConcurrency);
        if ( m_sharingMode & ParallelWait ) {
            m_barrier.wait( &utils::ConcurrencyTracker::Reset );
            {
                utils::ConcurrencyTracker ct;
                m_taskGroup->wait();
            }
            if ( utils::ConcurrencyTracker::PeakParallelism() == 1 )
                WARN( "Warning: No parallel waiting detected in TestParallelWait" );
            m_barrier.wait();
        }
        else
            m_taskGroup->wait();
        CHECK_MESSAGE( m_tasksSpawned == numSpawned, "No tasks should be spawned after wait starts. The test is broken" );
        CHECK_MESSAGE( s_tasksExecuted == numSpawned, "Not all spawned tasks were executed" );
    }

public:
    SharedGroupBodyImpl ( std::uintptr_t numThreads, std::uintptr_t sharingMode = 0 )
        : m_numThreads(numThreads)
        , m_sharingMode(sharingMode)
        , m_taskGroup(NULL)
        , m_barrier(numThreads)
    {
        CHECK_MESSAGE( m_numThreads > 1, "SharedGroupBody tests require concurrency" );
        if ((m_sharingMode & VagabondGroup) && m_numThreads != 2) {
            CHECK_MESSAGE(false, "In vagabond mode SharedGroupBody must be used with 2 threads only");
        }
        utils::ConcurrencyTracker::Reset();
        s_tasksExecuted = 0;
        m_tasksSpawned = 0;
        m_threadsReady = 0;
        m_taskFunctor.m_pOwner = this;
    }

    void Run ( std::uintptr_t idx ) {
        AssertLive();
        if ( idx == 0 ) {
            if (m_taskGroup || m_tasksSpawned) {
                CHECK_MESSAGE(false, "SharedGroupBody must be reset before reuse");
            }
#if _MSC_VER && !defined(__INTEL_COMPILER)
#pragma warning(push)
#pragma warning(disable:4316) // object allocated on the heap may not be aligned
#endif
#if __TBB_GCC_VERSION >= 70000 && !defined(__clang__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waligned-new=none"
#endif
            m_taskGroup = new task_group_type;
#if __TBB_GCC_VERSION >= 70000 && !defined(__clang__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic pop
#endif
#if _MSC_VER && !defined(__INTEL_COMPILER)
#pragma warning(pop)
#endif
            Spawn( c_numTasks0 );
            Wait();
            if ( m_sharingMode & VagabondGroup )
                m_barrier.wait();
            else
                DeleteTaskGroup();
        }
        else {
            while ( m_tasksSpawned == 0 )
                std::this_thread::yield();
            CHECK_MESSAGE ( m_taskGroup, "Task group is not initialized");
            Spawn (c_numTasks1);
            if ( m_sharingMode & ParallelWait )
                Wait();
            if ( m_sharingMode & VagabondGroup ) {
                CHECK_MESSAGE ( idx == 1, "In vagabond mode SharedGroupBody must be used with 2 threads only" );
                m_barrier.wait();
                DeleteTaskGroup();
            }
        }
        AssertLive();
    }
};

template<typename task_group_type>
atomic_t SharedGroupBodyImpl<task_group_type>::s_tasksExecuted;

template<typename task_group_type>
class  SharedGroupBody : utils::NoAssign, utils::NoAfterlife {
    bool m_bOwner;
    SharedGroupBodyImpl<task_group_type> *m_pImpl;
public:
    SharedGroupBody ( std::uintptr_t numThreads, std::uintptr_t sharingMode = 0 )
        : utils::NoAssign()
        , utils::NoAfterlife()
        , m_bOwner(true)
        , m_pImpl( new SharedGroupBodyImpl<task_group_type>(numThreads, sharingMode) )
    {}
    SharedGroupBody ( const SharedGroupBody& src )
        : utils::NoAssign()
        , utils::NoAfterlife()
        , m_bOwner(false)
        , m_pImpl(src.m_pImpl)
    {}
    ~SharedGroupBody () {
        if ( m_bOwner )
            delete m_pImpl;
    }
    void operator() ( std::uintptr_t idx ) const {
        // Wrap the functior into additional task group to enforce bounding.
        task_group_type tg;
        tg.run_and_wait([&] { m_pImpl->Run(idx); });
    }
};

template<typename task_group_type>
class RunAndWaitSyncronizationTestBody : utils::NoAssign {
    utils::SpinBarrier& m_barrier;
    std::atomic<bool>& m_completed;
    task_group_type& m_tg;
public:
    RunAndWaitSyncronizationTestBody(utils::SpinBarrier& barrier, std::atomic<bool>& completed, task_group_type& tg)
        : m_barrier(barrier), m_completed(completed), m_tg(tg) {}

    void operator()() const {
        m_barrier.wait();
        for (volatile int i = 0; i < 100000; ++i) {}
        m_completed = true;
    }

    void operator()(int id) const {
        if (id == 0) {
            m_tg.run_and_wait(*this);
        } else {
            m_barrier.wait();
            m_tg.wait();
            CHECK_MESSAGE(m_completed, "A concurrent waiter has left the wait method earlier than work has finished");
        }
    }
};

template<typename task_group_type>
void TestParallelSpawn () {
    NativeParallelFor( g_MaxConcurrency, SharedGroupBody<task_group_type>(g_MaxConcurrency) );
}

template<typename task_group_type>
void TestParallelWait () {
    NativeParallelFor( g_MaxConcurrency, SharedGroupBody<task_group_type>(g_MaxConcurrency, ParallelWait) );

    utils::SpinBarrier barrier(g_MaxConcurrency);
    std::atomic<bool> completed;
    completed = false;
    task_group_type tg;
    RunAndWaitSyncronizationTestBody<task_group_type> b(barrier, completed, tg);
    NativeParallelFor( g_MaxConcurrency, b );
}

// Tests non-stack-bound task group (the group that is allocated by one thread and destroyed by the other)
template<typename task_group_type>
void TestVagabondGroup () {
    NativeParallelFor( 2, SharedGroupBody<task_group_type>(2, VagabondGroup) );
}

#include "common/memory_usage.h"

template<typename task_group_type>
void TestThreadSafety() {
    auto tests = [] {
        for (int trail = 0; trail < 10; ++trail) {
            TestParallelSpawn<task_group_type>();
            TestParallelWait<task_group_type>();
            TestVagabondGroup<task_group_type>();
        }
    };

    // Test and warm up allocator.
    tests();

    // Ensure that cosumption is stabilized.
    std::size_t initial = utils::GetMemoryUsage();
    for (int trail = 0; trail < 20; ++trail) {
        tests();
        std::size_t current = utils::GetMemoryUsage();
        if (current <= initial) {
            return;
        }
        initial = current;
    }
    CHECK_MESSAGE(false, "Memory leak is detected");
}
//------------------------------------------------------------------------
// Common requisites of the Fibonacci tests
//------------------------------------------------------------------------

const std::uintptr_t N = 20;
const std::uintptr_t F = 6765;

atomic_t g_Sum;

#define FIB_TEST_PROLOGUE() \
    const unsigned numRepeats = g_MaxConcurrency * (TBB_USE_DEBUG ? 4 : 16);    \
    utils::ConcurrencyTracker::Reset()

#define FIB_TEST_EPILOGUE(sum) \
    CHECK( sum == numRepeats * F );


// Fibonacci tasks specified as functors
template<class task_group_type>
class FibTaskBase : utils::NoAssign, utils::NoAfterlife {
protected:
    std::uintptr_t* m_pRes;
    mutable std::uintptr_t m_Num;
    virtual void impl() const = 0;
public:
    FibTaskBase( std::uintptr_t* y, std::uintptr_t n ) : m_pRes(y), m_Num(n) {}
    void operator()() const {
        utils::ConcurrencyTracker ct;
        AssertLive();
        if( m_Num < 2 ) {
            *m_pRes = m_Num;
        } else {
            impl();
        }
    }
    virtual ~FibTaskBase() {}
};

template<class task_group_type>
class FibTaskAsymmetricTreeWithFunctor : public FibTaskBase<task_group_type> {
public:
    FibTaskAsymmetricTreeWithFunctor( std::uintptr_t* y, std::uintptr_t n ) : FibTaskBase<task_group_type>(y, n) {}
    virtual void impl() const override {
        std::uintptr_t x = ~0u;
        task_group_type tg;
        tg.run( FibTaskAsymmetricTreeWithFunctor(&x, this->m_Num-1) );
        this->m_Num -= 2; tg.run_and_wait( *this );
        *(this->m_pRes) += x;
    }
};

template<class task_group_type>
class FibTaskSymmetricTreeWithFunctor : public FibTaskBase<task_group_type> {
public:
    FibTaskSymmetricTreeWithFunctor( std::uintptr_t* y, std::uintptr_t n ) : FibTaskBase<task_group_type>(y, n) {}
    virtual void impl() const override {
        std::uintptr_t x = ~0u,
               y = ~0u;
        task_group_type tg;
        tg.run( FibTaskSymmetricTreeWithFunctor(&x, this->m_Num-1) );
        tg.run( FibTaskSymmetricTreeWithFunctor(&y, this->m_Num-2) );
        tg.wait();
        *(this->m_pRes) = x + y;
    }
};

// Helper functions
template<class fib_task>
std::uintptr_t RunFibTask(std::uintptr_t n) {
    std::uintptr_t res = ~0u;
    fib_task(&res, n)();
    return res;
}

template<typename fib_task>
void RunFibTest() {
    FIB_TEST_PROLOGUE();
    std::uintptr_t sum = 0;
    for( unsigned i = 0; i < numRepeats; ++i )
        sum += RunFibTask<fib_task>(N);
    FIB_TEST_EPILOGUE(sum);
}

template<typename fib_task>
void FibFunctionNoArgs() {
    g_Sum += RunFibTask<fib_task>(N);
}

template<typename task_group_type>
void TestFibWithLambdas() {
    FIB_TEST_PROLOGUE();
    atomic_t sum;
    sum = 0;
    task_group_type tg;
    for( unsigned i = 0; i < numRepeats; ++i )
        tg.run( [&](){sum += RunFibTask<FibTaskSymmetricTreeWithFunctor<task_group_type> >(N);} );
    tg.wait();
    FIB_TEST_EPILOGUE(sum);
}

template<typename task_group_type>
void TestFibWithFunctor() {
    RunFibTest<FibTaskAsymmetricTreeWithFunctor<task_group_type> >();
    RunFibTest< FibTaskSymmetricTreeWithFunctor<task_group_type> >();
}

template<typename task_group_type>
void TestFibWithFunctionPtr() {
    FIB_TEST_PROLOGUE();
    g_Sum = 0;
    task_group_type tg;
    for( unsigned i = 0; i < numRepeats; ++i )
        tg.run( &FibFunctionNoArgs<FibTaskSymmetricTreeWithFunctor<task_group_type> > );
    tg.wait();
    FIB_TEST_EPILOGUE(g_Sum);
}

template<typename task_group_type>
void RunFibonacciTests() {
    TestFibWithLambdas<task_group_type>();
    TestFibWithFunctor<task_group_type>();
    TestFibWithFunctionPtr<task_group_type>();
}

class test_exception : public std::exception
{
    const char* m_strDescription;
public:
    test_exception ( const char* descr ) : m_strDescription(descr) {}

    const char* what() const throw() override { return m_strDescription; }
};

#if TBB_USE_CAPTURED_EXCEPTION
    #include "tbb/tbb_exception.h"
    typedef tbb::captured_exception TestException;
#else
    typedef test_exception TestException;
#endif

#include <string.h>

#define NUM_CHORES      512
#define NUM_GROUPS      64
#define SKIP_CHORES     (NUM_CHORES/4)
#define SKIP_GROUPS     (NUM_GROUPS/4)
#define EXCEPTION_DESCR1 "Test exception 1"
#define EXCEPTION_DESCR2 "Test exception 2"

atomic_t g_ExceptionCount;
atomic_t g_TaskCount;
unsigned g_ExecutedAtCancellation;
bool g_Rethrow;
bool g_Throw;

class ThrowingTask : utils::NoAssign, utils::NoAfterlife {
    atomic_t &m_TaskCount;
public:
    ThrowingTask( atomic_t& counter ) : m_TaskCount(counter) {}
    void operator() () const {
        utils::ConcurrencyTracker ct;
        AssertLive();
        if ( g_Throw ) {
            if ( ++m_TaskCount == SKIP_CHORES )
                TBB_TEST_THROW(test_exception(EXCEPTION_DESCR1));
            std::this_thread::yield();
        }
        else {
            ++g_TaskCount;
            while( !tbb::is_current_task_group_canceling() )
                std::this_thread::yield();
        }
    }
};

inline void ResetGlobals ( bool bThrow, bool bRethrow ) {
    g_Throw = bThrow;
    g_Rethrow = bRethrow;
    g_ExceptionCount = 0;
    g_TaskCount = 0;
    utils::ConcurrencyTracker::Reset();
}

template<typename task_group_type>
void LaunchChildrenWithFunctor () {
    atomic_t count;
    count = 0;
    task_group_type g;
    for( unsigned i = 0; i < NUM_CHORES; ++i )
        g.run( ThrowingTask(count) );
#if TBB_USE_EXCEPTIONS
    tbb::task_group_status status = tbb::not_complete;
    bool exceptionCaught = false;
    try {
        status = g.wait();
    }
    catch ( TestException& e ) {
        CHECK_MESSAGE( e.what(), "Empty what() string" );
        CHECK_MESSAGE( strcmp(e.what(), EXCEPTION_DESCR1) == 0, "Unknown exception" );
        exceptionCaught = true;
        ++g_ExceptionCount;
    } catch( ... ) { CHECK_MESSAGE( false, "Unknown exception" ); }
    if (g_Throw && !exceptionCaught && status != tbb::canceled)
        CHECK_MESSAGE( false, "No exception in the child task group" );
    if ( g_Rethrow && g_ExceptionCount > SKIP_GROUPS ) {
        throw test_exception(EXCEPTION_DESCR2);
    }
#else
    g.wait();
#endif
}

// Tests for cancellation and exception handling behavior
template<typename task_group_type>
void TestManualCancellationWithFunctor () {
    ResetGlobals( false, false );
    task_group_type tg;
    for( unsigned i = 0; i < NUM_GROUPS; ++i )
        // TBB version does not require taking function address
        tg.run( &LaunchChildrenWithFunctor<task_group_type> );
    CHECK_MESSAGE ( !tbb::is_current_task_group_canceling(), "Unexpected cancellation" );
    CHECK_MESSAGE ( !tg.is_canceling(), "Unexpected cancellation" );
    while ( g_MaxConcurrency > 1 && g_TaskCount == 0 )
        std::this_thread::yield();
    tg.cancel();
    g_ExecutedAtCancellation = int(g_TaskCount);
    CHECK_MESSAGE ( tg.is_canceling(), "No cancellation reported" );
    tg.wait();
    CHECK_MESSAGE( g_TaskCount <= NUM_GROUPS * NUM_CHORES, "Too many tasks reported. The test is broken" );
    CHECK_MESSAGE( g_TaskCount < NUM_GROUPS * NUM_CHORES, "No tasks were cancelled. Cancellation model changed?" );
    CHECK_MESSAGE( g_TaskCount <= g_ExecutedAtCancellation + utils::ConcurrencyTracker::PeakParallelism(), "Too many tasks survived cancellation" );
}

#if TBB_USE_EXCEPTION
template<typename task_group_type>
void TestExceptionHandling1 () {
    ResetGlobals( true, false );
    task_group_type tg;
    for( unsigned i = 0; i < NUM_GROUPS; ++i )
        // TBB version does not require taking function address
        tg.run( &LaunchChildrenWithFunctor<task_group_type> );
    try {
        tg.wait();
    } catch ( ... ) {
        CHECK_MESSAGE( false, "Unexpected exception" );
    }
    CHECK_MESSAGE( g_ExceptionCount <= NUM_GROUPS, "Too many exceptions from the child groups. The test is broken" );
    CHECK_MESSAGE( g_ExceptionCount == NUM_GROUPS, "Not all child groups threw the exception" );
}

template<typename task_group_type>
void TestExceptionHandling2 () {
    ResetGlobals( true, true );
    task_group_type tg;
    bool exceptionCaught = false;
    for( unsigned i = 0; i < NUM_GROUPS; ++i ) {
        // TBB version does not require taking function address
        tg.run( &LaunchChildrenWithFunctor<task_group_type> );
    }
    try {
        tg.wait();
    } catch ( TestException& e ) {
        CHECK_MESSAGE( e.what(), "Empty what() string" );
        CHECK_MESSAGE( strcmp(e.what(), EXCEPTION_DESCR2) == 0, "Unknown exception" );
        CHECK_MESSAGE ( !tg.is_canceling(), "wait() has not reset cancellation state" );
        exceptionCaught = true;
    } catch( ... ) { CHECK_MESSAGE( false, "Unknown exception" ); }
    CHECK_MESSAGE( exceptionCaught, "No exception thrown from the root task group" );
    CHECK_MESSAGE( g_ExceptionCount >= SKIP_GROUPS, "Too few exceptions from the child groups. The test is broken" );
    CHECK_MESSAGE( g_ExceptionCount <= NUM_GROUPS - SKIP_GROUPS, "Too many exceptions from the child groups. The test is broken" );
    CHECK_MESSAGE( g_ExceptionCount < NUM_GROUPS - SKIP_GROUPS, "None of the child groups was cancelled" );
}

template<typename task_group_type>
class LaunchChildrenDriver {
public:
    void Launch(task_group_type& tg) {
        ResetGlobals(false, false);
        for (unsigned i = 0; i < NUM_GROUPS; ++i) {
            tg.run(LaunchChildrenWithFunctor<task_group_type>);
        }
        CHECK_MESSAGE(!tbb::is_current_task_group_canceling(), "Unexpected cancellation");
        CHECK_MESSAGE(!tg.is_canceling(), "Unexpected cancellation");
        while (g_MaxConcurrency > 1 && g_TaskCount == 0)
            std::this_thread::yield();
    }

    void Finish() {
        CHECK_MESSAGE(g_TaskCount <= NUM_GROUPS * NUM_CHORES, "Too many tasks reported. The test is broken");
        CHECK_MESSAGE(g_TaskCount < NUM_GROUPS * NUM_CHORES, "No tasks were cancelled. Cancellation model changed?");
        CHECK_MESSAGE(g_TaskCount <= g_ExecutedAtCancellation + g_MaxConcurrency, "Too many tasks survived cancellation");
    }
}; // LaunchChildrenWithTaskHandleDriver

template<typename task_group_type, bool Throw>
void TestMissingWait () {
    bool exception_occurred = false,
         unexpected_exception = false;
    LaunchChildrenDriver<task_group_type> driver;
    try {
        task_group_type tg;
        driver.Launch( tg );
        if ( Throw )
            throw int(); // Initiate stack unwinding
    }
    catch ( const tbb::missing_wait& e ) {
        CHECK_MESSAGE( e.what(), "Error message is absent" );
        exception_occurred = true;
        unexpected_exception = Throw;
    }
    catch ( int ) {
        exception_occurred = true;
        unexpected_exception = !Throw;
    }
    catch ( ... ) {
        exception_occurred = unexpected_exception = true;
    }
    CHECK( exception_occurred );
    CHECK( !unexpected_exception );
    driver.Finish();
}
#endif

template<typename task_group_type>
void RunCancellationAndExceptionHandlingTests() {
    TestManualCancellationWithFunctor<task_group_type>();
#if TBB_USE_EXCEPTION
    TestExceptionHandling1<task_group_type>();
    TestExceptionHandling2<task_group_type>();
    TestMissingWait<task_group_type, true>();
    TestMissingWait<task_group_type, false>();
#endif
}

void EmptyFunction () {}

struct TestFunctor {
    void operator()() { CHECK_MESSAGE( false, "Non-const operator called" ); }
    void operator()() const { /* library requires this overload only */ }
};

template<typename task_group_type>
void TestConstantFunctorRequirement() {
    task_group_type g;
    TestFunctor tf;
    g.run( tf ); g.wait();
    g.run_and_wait( tf );
}

//------------------------------------------------------------------------
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
        MovePreferableFunctor(MovePreferableFunctor&& other) : utils::Movable(std::move(other)) {};
        MovePreferableFunctor(const MovePreferableFunctor& other) : utils::Movable(other) {};
    };

    struct NoMoveNoCopyFunctor : utils::NoCopy, TestFunctor {
        NoMoveNoCopyFunctor() : utils::NoCopy() {};
        // mv ctor is not allowed as cp ctor from parent utils::NoCopy
    private:
        NoMoveNoCopyFunctor(NoMoveNoCopyFunctor&&);
    };

     template<typename task_group_type>
    void TestBareFunctors() {
        task_group_type tg;
        MovePreferableFunctor mpf;
        // run_and_wait() doesn't have any copies or moves of arguments inside the impl
        tg.run_and_wait( NoMoveNoCopyFunctor() );

        tg.run( MoveOnlyFunctor() );
        tg.wait();

        tg.run( mpf );
        tg.wait();
        CHECK_MESSAGE(mpf.alive, "object was moved when was passed by lval");
        mpf.Reset();

        tg.run( std::move(mpf) );
        tg.wait();
        CHECK_MESSAGE(!mpf.alive, "object was copied when was passed by rval");
        mpf.Reset();
    }
}

template<typename task_group_type>
void TestMoveSemantics() {
    TestMoveSemanticsNS::TestBareFunctors<task_group_type>();
}
//------------------------------------------------------------------------

// TODO: TBB_REVAMP_TODO - enable when ETS is available
#if TBBTEST_USE_TBB && TBB_PREVIEW_ISOLATED_TASK_GROUP
namespace TestIsolationNS {
    class DummyFunctor {
    public:
        DummyFunctor() {}
        void operator()() const {
            for ( volatile int j = 0; j < 10; ++j ) {}
        }
    };

    template<typename task_group_type>
    class ParForBody {
        task_group_type& m_tg;
        std::atomic<bool>& m_preserved;
        tbb::enumerable_thread_specific<int>& m_ets;
    public:
        ParForBody(
            task_group_type& tg,
            std::atomic<bool>& preserved,
            tbb::enumerable_thread_specific<int>& ets
        ) : m_tg(tg), m_preserved(preserved), m_ets(ets) {}

        void operator()(int) const {
            if (++m_ets.local() > 1) m_preserved = false;

            for (int i = 0; i < 1000; ++i)
                m_tg.run(DummyFunctor());
            m_tg.wait();
            m_tg.run_and_wait(DummyFunctor());

            --m_ets.local();
        }
    };

    template<typename task_group_type>
    void CheckIsolation(bool isolation_is_expected) {
        task_group_type tg;
        std::atomic<bool> isolation_is_preserved;
        isolation_is_preserved = true;
        tbb::enumerable_thread_specific<int> ets(0);

        tbb::parallel_for(0, 100, ParForBody<task_group_type>(tg, isolation_is_preserved, ets));

        ASSERT(
            isolation_is_expected == isolation_is_preserved,
            "Actual and expected isolation-related behaviours are different"
        );
    }

    // Should be called only when > 1 thread is used, because otherwise isolation is guaranteed to take place
    void TestIsolation() {
        CheckIsolation<tbb::task_group>(false);
        CheckIsolation<tbb::isolated_task_group>(true);
    }
}
#endif

//! Test for thread safety for the task_group
//! \brief \ref error_guessing \ref resource_usage
TEST_CASE("Thread safety test for the task group") {
    for (unsigned p=MinThread; p <= MaxThread; ++p) {
        if (p < 2) {
            continue;
        }
        tbb::global_control limit(tbb::global_control::max_allowed_parallelism, p);
        g_MaxConcurrency = p;
        TestThreadSafety<tbb::task_group>();
    }
}

//! Fibonacci test for task group
//! \brief \ref interface \ref requirement
TEST_CASE("Fibonacci test for the task group") {
    for (unsigned p=MinThread; p <= MaxThread; ++p) {
        tbb::global_control limit(tbb::global_control::max_allowed_parallelism, p);
        g_MaxConcurrency = p;
        RunFibonacciTests<tbb::task_group>();
    }
}

//! Cancellation and exception test for the task group
//! \brief \ref interface \ref requirement
TEST_CASE("Cancellation and exception test for the task group") {
    for (unsigned p=MinThread; p <= MaxThread; ++p) {
        tbb::global_control limit(tbb::global_control::max_allowed_parallelism, p);
        g_MaxConcurrency = p;
        RunCancellationAndExceptionHandlingTests<tbb::task_group>();
    }
}

//! Constant functor test for the task group
//! \brief \ref interface \ref negative
TEST_CASE("Constant functor test for the task group") {
    for (unsigned p=MinThread; p <= MaxThread; ++p) {
        tbb::global_control limit(tbb::global_control::max_allowed_parallelism, p);
        g_MaxConcurrency = p;
        TestConstantFunctorRequirement<tbb::task_group>();
    }
}

//! Move semantics test for the task group
//! \brief \ref interface \ref requirement
TEST_CASE("Move semantics test for the task group") {
    for (unsigned p=MinThread; p <= MaxThread; ++p) {
        tbb::global_control limit(tbb::global_control::max_allowed_parallelism, p);
        g_MaxConcurrency = p;
        TestMoveSemantics<tbb::task_group>();
    }
}

#if TBB_PREVIEW_ISOLATED_TASK_GROUP
//! Test for thread safety for the isolated_task_group
//! \brief \ref error_guessing
TEST_CASE("Thread safety test for the isolated task group") {
    for (unsigned p=MinThread; p <= MaxThread; ++p) {
        if (p < 2) {
            continue;
        }
        tbb::global_control limit(tbb::global_control::max_allowed_parallelism, p);
        g_MaxConcurrency = p;
        TestThreadSafety<tbb::isolated_task_group>();
    }
}

//! Cancellation and exception test for the isolated task group
//! \brief \ref interface \ref requirement
TEST_CASE("Fibonacci test for the isolated task group") {
    for (unsigned p=MinThread; p <= MaxThread; ++p) {
        tbb::global_control limit(tbb::global_control::max_allowed_parallelism, p);
        g_MaxConcurrency = p;
        RunFibonacciTests<tbb::isolated_task_group>();
    }
}

//! Cancellation and exception test for the isolated task group
//! \brief \ref interface \ref requirement
TEST_CASE("Cancellation and exception test for the isolated task group") {
    for (unsigned p=MinThread; p <= MaxThread; ++p) {
        tbb::global_control limit(tbb::global_control::max_allowed_parallelism, p);
        g_MaxConcurrency = p;
        RunCancellationAndExceptionHandlingTests<tbb::isolated_task_group>();
    }
}

//! Constant functor test for the isolated task group.
//! \brief \ref interface \ref negative
TEST_CASE("Constant functor test for the isolated task group") {
    for (unsigned p=MinThread; p <= MaxThread; ++p) {
        tbb::global_control limit(tbb::global_control::max_allowed_parallelism, p);
        g_MaxConcurrency = p;
        TestConstantFunctorRequirement<tbb::isolated_task_group>();
    }
}

//! Move semantics test for the isolated task group.
//! \brief \ref interface \ref requirement
TEST_CASE("Move semantics test for the isolated task group") {
    for (unsigned p=MinThread; p <= MaxThread; ++p) {
        tbb::global_control limit(tbb::global_control::max_allowed_parallelism, p);
        g_MaxConcurrency = p;
        TestMoveSemantics<tbb::isolated_task_group>();
    }
}
#endif /* TBB_PREVIEW_ISOLATED_TASK_GROUP */

void run_deep_stealing(tbb::task_group& tg1, tbb::task_group& tg2, int num_tasks, std::atomic<int>& tasks_executed) {
    for (int i = 0; i < num_tasks; ++i) {
        tg2.run([&tg1, &tasks_executed] {
            volatile char consume_stack[1000]{};
            ++tasks_executed;
            tg1.wait();
            utils::suppress_unused_warning(consume_stack);
        });
    }
}

// TODO: move to the conformance test
//! Test for stack overflow avoidance mechanism.
//! \brief \ref requirement
TEST_CASE("Test for stack overflow avoidance mechanism") {
    if (tbb::this_task_arena::max_concurrency() < 2) {
        return;
    }

    tbb::global_control thread_limit(tbb::global_control::max_allowed_parallelism, 2);
    tbb::task_group tg1;
    tbb::task_group tg2;
    std::atomic<int> tasks_executed{};
    tg1.run_and_wait([&tg1, &tg2, &tasks_executed] {
        run_deep_stealing(tg1, tg2, 10000, tasks_executed);
        while (tasks_executed < 100) {
            // Some stealing is expected to happen.
            std::this_thread::yield();
        }
        CHECK(tasks_executed < 10000);
    });
    tg2.wait();
    CHECK(tasks_executed == 10000);
}

//! Test for stack overflow avoidance mechanism.
//! \brief \ref error_guessing
TEST_CASE("Test for stack overflow avoidance mechanism within arena") {
    if (tbb::this_task_arena::max_concurrency() < 2) {
        return;
    }

    tbb::global_control thread_limit(tbb::global_control::max_allowed_parallelism, 2);
    tbb::task_group tg1;
    tbb::task_group tg2;
    std::atomic<int> tasks_executed{};

    // Determine nested task execution limit.
    int second_thread_executed{};
    tg1.run_and_wait([&tg1, &tg2, &tasks_executed, &second_thread_executed] {
        run_deep_stealing(tg1, tg2, 10000, tasks_executed);
        do {
            second_thread_executed = tasks_executed;
            utils::Sleep(10);
        } while (second_thread_executed < 100 || second_thread_executed != tasks_executed);
        std::cout << "tasks_executed = " << tasks_executed << std::endl;
        CHECK(tasks_executed < 10000);
    });
    tg2.wait();
    CHECK(tasks_executed == 10000);

    tasks_executed = 0;
    tbb::task_arena a(2, 2);
    tg1.run_and_wait([&a, &tg1, &tg2, &tasks_executed, second_thread_executed] {
        run_deep_stealing(tg1, tg2, second_thread_executed-1, tasks_executed);
        while (tasks_executed < second_thread_executed-1) {
            // Wait until the second thread near the limit.
            std::this_thread::yield();
        }
        tg2.run([&a, &tg1, &tasks_executed] {
            a.execute([&tg1, &tasks_executed] {
                volatile char consume_stack[1000]{};
                ++tasks_executed;
                tg1.wait();
                utils::suppress_unused_warning(consume_stack);
            });
        });
        while (tasks_executed < second_thread_executed) {
            // Wait until the second joins the arena.
            std::this_thread::yield();
        }
        a.execute([&tg1, &tg2, &tasks_executed] {
            run_deep_stealing(tg1, tg2, 10000, tasks_executed);
        });
        int currently_executed{};
        do {
            currently_executed = tasks_executed;
            utils::Sleep(10);
        } while (currently_executed != tasks_executed);
        CHECK(tasks_executed < 10000 + second_thread_executed);
    });
    a.execute([&tg2] {
        tg2.wait();
    });
    CHECK(tasks_executed == 10000 + second_thread_executed);
}
