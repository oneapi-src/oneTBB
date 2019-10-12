/*
    Copyright (C) 2005-2011, Intel Corporation

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

#include "tbb/concurrent_vector.h"
#include "tbb/spin_rw_mutex.h"
#include "tbb/spin_mutex.h"
#include "tbb/queuing_rw_mutex.h"
#include "tbb/queuing_mutex.h"
#include "tbb/mutex.h"
#include "tbb/blocked_range.h"
#include "proto/guarded.h"

// Test functions definitions
template <typename Mutextype> 
void SimpleTypeSerialTest();

template <typename Mutextype> 
void StdStringSerialTest();

void ParallelAssigmentTest(int number_of_threads);
template <typename Mutextype> int square(tbb::guarded<int, Mutextype> n);

#include <stdio.h>
#include <string>
#include "harness.h"
#include "harness_assert.h"
int main( int argc, char* argv[] ) {

    MinThread = 1;
    ParseCommandLine(argc,argv);
    if( MinThread<1 ) {
        printf("number of threads must be positive\n");
        exit(1);
    }
    // Testing with simple type
    SimpleTypeSerialTest<tbb::spin_mutex>();
    SimpleTypeSerialTest<tbb::spin_rw_mutex>();
    SimpleTypeSerialTest<tbb::queuing_mutex>();
    SimpleTypeSerialTest<tbb::queuing_rw_mutex>();
    SimpleTypeSerialTest<tbb::mutex>();

    // Testing with std::string
    StdStringSerialTest<tbb::spin_mutex>();
    StdStringSerialTest<tbb::spin_rw_mutex>();
    StdStringSerialTest<tbb::queuing_mutex>();
    StdStringSerialTest<tbb::queuing_rw_mutex>();
    StdStringSerialTest<tbb::mutex>();

    // Parallel test
    for( int p=MinThread; p<=MaxThread; ++p ) {
        ParallelAssigmentTest(p);
    }

    printf ("done\n");
    return 0;
}

//! The function tests tbb::guarded<T> with "int" type and different types of mutexes
template <typename Mutextype> void SimpleTypeSerialTest() {
    tbb::concurrent_vector <tbb::guarded<int, Mutextype> > intvect;
    intvect.grow_by (10);

    typedef typename tbb::guarded<int, Mutextype>::accessor int_accessor;
    typedef typename tbb::guarded<int, Mutextype>::const_accessor int_const_accessor;

    // Initializing the vector
    for  ( int i =0; i < 10; i++) {
        int_accessor a(intvect[i]);
        *a = i;
    }

    {// Testing operator =, operator + with accossors
        int_accessor a( intvect[ 0 ] );
        int_const_accessor a1( intvect[ 1 ] );
        int_const_accessor a2( intvect[ 2 ] );
        *a = *a1 + *a2;
        // The result should be 3
        ASSERT (*a == 3, "operator + provides wrong result");
    }

    // Testing explicit acquire \ release
    {
        int_accessor a( intvect[ 1 ] );
        int_const_accessor a1( intvect[ 2 ] );
        a.release();
        a1.release();
        a.acquire();
        a1.acquire();
    }

    // Testing try_acquire
    {
        int_accessor a( intvect[ 1 ] );
        int_const_accessor a1( intvect[ 2 ] );
        a.release();
        a1.release();
        ASSERT (a.try_acquire(), "try_acquire returned wrong value");
        ASSERT (a1.try_acquire(), "try_acquire returned wrong value");
    }


    // Testing tbb::guarded<T> changing using acquire method.
    {
        int_accessor a( intvect[ 0 ] );
        int_const_accessor a1( intvect[ 1 ] );
        a.release();
        a1.release();
        a.acquire( intvect[ 3 ] );
        a1.acquire( intvect[ 4 ] );
        ASSERT (*a == 3, "Acquire(tbb::guarded<T>) method works incorrectly");
        ASSERT (*a1 == 4, "Acquire(tbb::guarded<T>) method works incorrectly");
    }

    // Testing copy constructor for guarded<T>
    int n = 2;
    tbb::guarded<int, Mutextype> g_n(n);
    ASSERT (square<Mutextype>(g_n) == 4, "Error during testing copy constructor for guarded<T>");

    // Testing operator = for guarded<T>
    tbb::guarded<int, Mutextype> n1 (1);
    tbb::guarded<int, Mutextype> n2 (2);
    n1 = n2;
    int_const_accessor c_n1(n1);
    ASSERT(*c_n1 == 2, "operator = for guarded<T> works incorrectly");
}

//! The function tests tbb::guarded<T> with "std::string" type and different types of mutexes
template <typename Mutextype> void StdStringSerialTest() {
    // Testing with std::string
    tbb::concurrent_vector <tbb::guarded<std::string, Mutextype> > strvect;
    strvect.grow_by (10);

    typedef typename tbb::guarded<std::string, Mutextype>::accessor string_accessor;
    typedef typename tbb::guarded<std::string, Mutextype>::const_accessor string_const_accessor;

    // Initializing the vector
    for  ( char i = 0; i < 10; i++) {
        string_accessor a(strvect[i]);
        *a = 'a' + i;
    }

    {// Testing operator =, operator +, operator != for a
        string_accessor a( strvect[ 0 ] );
        string_const_accessor a1( strvect[ 1 ] );
        string_const_accessor a2( strvect[ 2 ] );
        *a = *a1 + *a2;
        // The result should be bc
        ASSERT(*a == "bc", "operator + provides incorrect result for std::string type");
    }

    // Testing -> operator
    {
        std::string str1("abcd");
        std::string str2("efgh");
        tbb::guarded<std::string, Mutextype> g_str1(str1);
        tbb::guarded<std::string, Mutextype> g_str2(str2);
        string_accessor a( g_str1);
        string_const_accessor a_c( g_str2 );

        ASSERT (a->compare(std::string("abcd")) == 0, "Operator -> for accessor works incorrectly" );
        ASSERT (a_c->compare(std::string("efgh")) == 0, "Operator -> for accessor works incorrectly" );
    }
}

//! This class 
tbb::guarded<int, tbb::spin_mutex> parallel_sum;
class intloop {
public:
    intloop() {}
    void operator () ( const tbb::blocked_range<int>& range) const {
        for ( int i = range.begin(); i != range.end(); ++i ) {
            tbb::guarded<int, tbb::spin_mutex>::accessor a(parallel_sum);
            *a = *a + 1;
        }
    }
};

//! Performs an addition from 0 to 1000 in a loop
void ParallelAssigmentTest(int number_of_threads) {
    {
        tbb::guarded<int, tbb::spin_mutex>::accessor a(parallel_sum);
        *a = 0;
    }

    intloop loop;
    NativeParallelFor(tbb::blocked_range<int>(0, 1000, 1000 / number_of_threads), loop);

    tbb::guarded<int, tbb::spin_mutex>::const_accessor c_a(parallel_sum);
    ASSERT(*c_a == 1000, "tbb::guarded doesn't work properly in parallel mode");
}

template <typename Mutextype> int square(tbb::guarded<int, Mutextype> n) {
    typedef typename tbb::guarded<int, Mutextype>::accessor int_accessor;
    int_accessor a(n);
    return (*a) * (*a);
}
