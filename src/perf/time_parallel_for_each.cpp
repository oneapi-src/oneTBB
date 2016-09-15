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

#include <vector>
#include <list>
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <string>

#include "tbb/parallel_for_each.h"
#include "tbb/tick_count.h"

template <typename Type>
void foo( Type &f ) {
    f += 1.0f;
}

template <typename Container>
void test( std::string testName, const int N, const int numRepeats ) {
    typedef typename Container::value_type Type;
    Container v;

    for ( int i = 0; i < N; ++i ) {
        v.push_back( static_cast<Type>(std::rand()) );
    }

    std::vector<double> times;
    times.reserve( numRepeats );

    for ( int i = 0; i < numRepeats; ++i ) {
        tbb::tick_count t0 = tbb::tick_count::now();
        tbb::parallel_for_each( v.begin(), v.end(), foo<Type> );
        tbb::tick_count t1 = tbb::tick_count::now();
        times.push_back( (t1 - t0).seconds()*1e+3 );
    }

    std::sort( times.begin(), times.end() );
    std::cout << "Test " << testName << std::endl
        << "min " << times[times.size() / 20] << " ms " << std::endl
        << "med " << times[times.size() / 2] << " ms " << std::endl
        << "max " << times[times.size() - times.size() / 20 - 1] << " ms " << std::endl;
}

int main( int argc, char* argv[] ) {
    const int N = argc > 1 ? std::atoi( argv[1] ) : 10 * 1000;
    const int numRepeats = argc > 2 ? std::atoi( argv[2] ) : 10;

    test< std::vector<float> >( "std::vector<float>", N, numRepeats );
    test< std::list<float> >( "std::list<float>", N / 100, numRepeats );

    return 0;
}
