/*
    Copyright 2005-2015 Intel Corporation.  All Rights Reserved.

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

#define HARNESS_DEFAULT_MIN_THREADS 0
#define HARNESS_DEFAULT_MAX_THREADS 4

#define __TBB_EXTRA_DEBUG 1 // for concurrent_hash_map
#include "tbb/combinable.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/tick_count.h"
#include "tbb/tbb_allocator.h"
#include "tbb/tbb_thread.h"

#if !TBB_USE_EXCEPTIONS && _MSC_VER
    // Suppress "C++ exception handler used, but unwind semantics are not enabled" warning in STL headers
    #pragma warning (push)
    #pragma warning (disable: 4530)
#endif

#include <cstring>
#include <vector>
#include <utility>

#if !TBB_USE_EXCEPTIONS && _MSC_VER
    #pragma warning (pop)
#endif

#include "harness_assert.h"
#include "harness.h"

#if __TBB_GCC_WARNING_SUPPRESSION_PRESENT
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif

static tbb::atomic<int> construction_counter;
static tbb::atomic<int> destruction_counter;

const int REPETITIONS = 10;
const int N = 100000;
const double EXPECTED_SUM = (REPETITIONS + 1) * N;

//
// A minimal class
// Define: default and copy constructor, and allow implicit operator&
// also operator=
//

class minimal {
private:
    int my_value;
public:
    minimal(int val=0) : my_value(val) { ++construction_counter; }
    minimal( const minimal &m ) : my_value(m.my_value) { ++construction_counter; }
    minimal& operator=(const minimal& other) { my_value = other.my_value; return *this; }
    minimal& operator+=(const minimal& other) { my_value += other.my_value; return *this; }
    operator int() const { return my_value; }
    ~minimal() { ++destruction_counter; }
    void set_value( const int i ) { my_value = i; }
    int value( ) const { return my_value; }
};

//// functors for initialization and combine

// Addition
template <typename T>
struct FunctorAddFinit {
    T operator()() { return 0; }
};

template <typename T>
struct FunctorAddFinit7 {
    T operator()() { return 7; }
};

template <typename T>
struct FunctorAddCombine {
    T operator()(T left, T right ) const {
        return left + right;
    }
};

template <typename T>
struct FunctorAddCombineRef {
    T operator()(const T& left, const T& right ) const {
        return left + right;
    }
};

template <typename T>
T my_finit( ) { return 0; }

template <typename T>
T my_combine( T left, T right) { return left + right; }

template <typename T>
T my_combine_ref( const T &left, const T &right) { return left + right; }

template <typename T>
class CombineEachHelper {
public:
    CombineEachHelper(T& _result) : my_result(_result) {}
    void operator()(const T& new_bit) { my_result +=  new_bit; }
    CombineEachHelper& operator=(const CombineEachHelper& other) { 
        my_result =  other; 
        return *this; 
    }
private:
    T& my_result;
};

template <typename T>
class CombineEachHelperCnt {
public:
    CombineEachHelperCnt(T& _result, int& _nbuckets) : my_result(_result), nBuckets(_nbuckets) {}
    void operator()(const T& new_bit) { my_result +=  new_bit; ++nBuckets; }
    CombineEachHelperCnt& operator=(const CombineEachHelperCnt& other) { 
        my_result =  other.my_result; 
        nBuckets = other.nBuckets;
        return *this; 
    }
private:
    T& my_result;
    int& nBuckets;
};

template <typename T>
class CombineEachVectorHelper {
public:
    typedef std::vector<T, tbb::tbb_allocator<T> > ContainerType;
    CombineEachVectorHelper(T& _result) : my_result(_result) { }
    void operator()(const ContainerType& new_bit) { 
        for(typename ContainerType::const_iterator ci = new_bit.begin(); ci != new_bit.end(); ++ci) {
            my_result +=  *ci;
        }
    }
    CombineEachVectorHelper& operator=(const CombineEachVectorHelper& other) { my_result=other.my_result; return *this;}
private:
    T& my_result;
};



//// end functors

template< typename T >
void run_serial_scalar_tests(const char *test_name) {
    tbb::tick_count t0;
    T sum = 0;

    REMARK("Testing serial %s... ", test_name);
    for (int t = -1; t < REPETITIONS; ++t) {
        if (Verbose && t == 0) t0 = tbb::tick_count::now(); 
        for (int i = 0; i < N; ++i) {
            sum += 1; 
        }
    }
 
    double ResultValue = sum;
    ASSERT( EXPECTED_SUM == ResultValue, NULL);
    REMARK("done\nserial %s, 0, %g, %g\n", test_name, ResultValue, ( tbb::tick_count::now() - t0).seconds());
}


template <typename T>
class ParallelScalarBody: NoAssign {
    
    tbb::combinable<T> &sums;
 
public:

    ParallelScalarBody ( tbb::combinable<T> &_sums ) : sums(_sums) { }

    void operator()( const tbb::blocked_range<int> &r ) const {
        for (int i = r.begin(); i != r.end(); ++i) { 
            bool was_there;
            T& my_local = sums.local(was_there);
            if(!was_there) my_local = 0;
             my_local +=  1 ;
        }
    }
   
};

// parallel body with no test for first access.
template <typename T>
class ParallelScalarBodyNoInit: NoAssign {
    
    tbb::combinable<T> &sums;
 
public:

    ParallelScalarBodyNoInit ( tbb::combinable<T> &_sums ) : sums(_sums) { }

    void operator()( const tbb::blocked_range<int> &r ) const {
        for (int i = r.begin(); i != r.end(); ++i) { 
             sums.local() +=  1 ;
        }
    }
   
};

template< typename T >
void RunParallelScalarTests(const char *test_name) {

    tbb::task_scheduler_init init(tbb::task_scheduler_init::deferred);

    for (int p = MinThread; p <= MaxThread; ++p) { 


        if (p == 0) continue;

        REMARK("Testing parallel %s on %d thread(s)... ", test_name, p); 
        init.initialize(p);

        tbb::tick_count t0;

        T assign_sum(0);

        T combine_sum(0);

        T combine_ref_sum(0);

        T combine_each_sum(0);

        T combine_finit_sum(0);

        for (int t = -1; t < REPETITIONS; ++t) {
            if (Verbose && t == 0) t0 = tbb::tick_count::now(); 

            tbb::combinable<T> sums;
            FunctorAddFinit<T> my_finit_decl;
            tbb::combinable<T> finit_combinable(my_finit_decl);
                                    

            tbb::parallel_for( tbb::blocked_range<int>( 0, N, 10000 ), ParallelScalarBodyNoInit<T>( finit_combinable ) );
            tbb::parallel_for( tbb::blocked_range<int>( 0, N, 10000 ), ParallelScalarBody<T>( sums ) );

            // Use combine
            combine_sum +=  sums.combine(my_combine<T>);
            combine_ref_sum +=  sums.combine(my_combine_ref<T>);

            CombineEachHelper<T> my_helper(combine_each_sum);
            sums.combine_each(my_helper);
           
            // test assignment
            tbb::combinable<T> assigned;
            assigned = sums;

            assign_sum +=  assigned.combine(my_combine<T>);

            combine_finit_sum += finit_combinable.combine(my_combine<T>);
        }

        ASSERT( EXPECTED_SUM == combine_sum, NULL);
        ASSERT( EXPECTED_SUM == combine_ref_sum, NULL);
        ASSERT( EXPECTED_SUM == assign_sum, NULL);
        ASSERT( EXPECTED_SUM == combine_finit_sum, NULL);

        REMARK("done\nparallel %s, %d, %g, %g\n", test_name, p, static_cast<double>(combine_sum), 
                                                      ( tbb::tick_count::now() - t0).seconds());
        init.terminate();
    }
}


template <typename T>
class ParallelVectorForBody: NoAssign {
    
    tbb::combinable< std::vector<T, tbb::tbb_allocator<T> > > &locals;
 
public:

    ParallelVectorForBody ( tbb::combinable< std::vector<T, tbb::tbb_allocator<T> > > &_locals ) : locals(_locals) { }

    void operator()( const tbb::blocked_range<int> &r ) const {
        T one = 1;

        for (int i = r.begin(); i < r.end(); ++i) {
            locals.local().push_back( one );
        }
    }
   
};

template< typename T >
void RunParallelVectorTests(const char *test_name) {
    tbb::tick_count t0;
    tbb::task_scheduler_init init(tbb::task_scheduler_init::deferred);
    typedef std::vector<T, tbb::tbb_allocator<T> > ContainerType;

    for (int p = MinThread; p <= MaxThread; ++p) { 

        if (p == 0) continue;
        REMARK("Testing parallel %s on %d thread(s)... ", test_name, p);
        init.initialize(p);

        T sum = 0;
        T sum2 = 0;
        T sum3 = 0;

        for (int t = -1; t < REPETITIONS; ++t) {
            if (Verbose && t == 0) t0 = tbb::tick_count::now(); 
            typedef typename tbb::combinable< ContainerType > CombinableType;
            CombinableType vs;

            tbb::parallel_for ( tbb::blocked_range<int> (0, N, 10000), ParallelVectorForBody<T>( vs ) );

            // copy construct
            CombinableType vs2(vs); // this causes an assertion failure, related to allocators...

            // assign
            CombinableType vs3;
            vs3 = vs;

            CombineEachVectorHelper<T> MyCombineEach(sum);
            vs.combine_each(MyCombineEach);

            CombineEachVectorHelper<T> MyCombineEach2(sum2);
            vs2.combine_each(MyCombineEach2);

            CombineEachVectorHelper<T> MyCombineEach3(sum3);
            vs2.combine_each(MyCombineEach3);
            // combine_each sums all elements of each vector into the result.
        }

        double ResultValue = sum;
        ASSERT( EXPECTED_SUM == ResultValue, NULL);
        ResultValue = sum2;
        ASSERT( EXPECTED_SUM == ResultValue, NULL);
        ResultValue = sum3;
        ASSERT( EXPECTED_SUM == ResultValue, NULL);
        REMARK("done\nparallel %s, %d, %g, %g\n", test_name, p, ResultValue, ( tbb::tick_count::now() - t0).seconds());
        init.terminate();
    }
}

#include "harness_barrier.h"

Harness::SpinBarrier sBarrier;

struct Body : NoAssign {
    tbb::combinable<int>* locals;
    const int nthread;
    const int nIters;
    Body( int nthread_, int niters_ ) : nthread(nthread_), nIters(niters_) { sBarrier.initialize(nthread_); }


    void operator()(int thread_id ) const {
        bool existed;
        sBarrier.wait();
        for(int i = 0; i < nIters; ++i ) {
            existed = thread_id & 1;
            int oldval = locals->local(existed);
            ASSERT(existed == (i > 0), "Error on first reference");
            ASSERT(!existed || (oldval == thread_id), "Error on fetched value");
            existed = thread_id & 1;
            locals->local(existed) = thread_id;
            ASSERT(existed, "Error on assignment");
        }
    }
};

void
TestLocalAllocations( int nthread ) {
    ASSERT(nthread > 0, "nthread must be positive");
#define NITERATIONS 1000
    Body myBody(nthread, NITERATIONS);
    tbb::combinable<int> myCombinable;
    myBody.locals = &myCombinable;

    NativeParallelFor( nthread, myBody );

    int mySum = 0;
    int mySlots = 0;
    CombineEachHelperCnt<int> myCountCombine(mySum, mySlots);
    myCombinable.combine_each(myCountCombine);

    ASSERT(nthread == mySlots, "Incorrect number of slots");
    ASSERT(mySum == (nthread - 1) * nthread / 2, "Incorrect values in result");
}


void 
RunParallelTests() {
    RunParallelScalarTests<int>("int");
    RunParallelScalarTests<double>("double");
    RunParallelScalarTests<minimal>("minimal");
    RunParallelVectorTests<int>("std::vector<int, tbb::tbb_allocator<int> >");
    RunParallelVectorTests<double>("std::vector<double, tbb::tbb_allocator<double> >");
}

template <typename T>
void
RunAssignmentAndCopyConstructorTest(const char *test_name) {
    REMARK("Testing assignment and copy construction for %s\n", test_name);

    // test creation with finit function (combine returns finit return value if no threads have created locals)
    FunctorAddFinit7<T> my_finit7_decl;
    tbb::combinable<T> create2(my_finit7_decl);
    ASSERT(7 == create2.combine(my_combine<T>), NULL);

    // test copy construction with function initializer
    tbb::combinable<T> copy2(create2);
    ASSERT(7 == copy2.combine(my_combine<T>), NULL);

    // test copy assignment with function initializer
    FunctorAddFinit<T> my_finit_decl;
    tbb::combinable<T> assign2(my_finit_decl);
    assign2 = create2;
    ASSERT(7 == assign2.combine(my_combine<T>), NULL);
}

void
RunAssignmentAndCopyConstructorTests() {
    REMARK("Running assignment and copy constructor tests\n");
    RunAssignmentAndCopyConstructorTest<int>("int");
    RunAssignmentAndCopyConstructorTest<double>("double");
    RunAssignmentAndCopyConstructorTest<minimal>("minimal");
}

int TestMain () {
    if (MaxThread > 0) {
        RunParallelTests();
    }
    RunAssignmentAndCopyConstructorTests();
    for(int i = 1 <= MinThread ? MinThread : 1; i <= MaxThread; ++i) {
        REMARK("Testing local() allocation with nthreads=%d\n", i);
        for(int j = 0; j < 100; ++j) {
            TestLocalAllocations(i);
        }
    }
    return Harness::Done;
}

