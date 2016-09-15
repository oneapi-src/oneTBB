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

#define HARNESS_DEFAULT_MIN_THREADS 2
#define HARNESS_DEFAULT_MAX_THREADS 2

#if !TBB_USE_THREADING_TOOLS
    #define TBB_USE_THREADING_TOOLS 1
#endif

#include "harness.h"

#if DO_ITT_NOTIFY

#include "tbb/spin_mutex.h"
#include "tbb/spin_rw_mutex.h"
#include "tbb/queuing_rw_mutex.h"
#include "tbb/queuing_mutex.h"
#include "tbb/mutex.h"
#include "tbb/recursive_mutex.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/task_scheduler_init.h"


#include "../tbb/itt_notify.h"


template<typename M>
class WorkEmulator: NoAssign {
    M& m_mutex;
    static volatile size_t s_anchor;
public:
    void operator()( tbb::blocked_range<size_t>& range ) const {
        for( size_t i=range.begin(); i!=range.end(); ++i ) {
            typename M::scoped_lock lock(m_mutex);
            for ( size_t j = 0; j!=range.end(); ++j )
                s_anchor = (s_anchor - i) / 2 + (s_anchor + j) / 2;
        }
    }
    WorkEmulator( M& mutex ) : m_mutex(mutex) {}
};

template<typename M>
volatile size_t WorkEmulator<M>::s_anchor = 0;


template<class M>
void Test( const char * name ) {
    REMARK("Testing %s\n",name);
    M mtx;
    tbb::profiling::set_name(mtx, name);

    const int n = 10000;
    tbb::parallel_for( tbb::blocked_range<size_t>(0,n,n/100), WorkEmulator<M>(mtx) );
}

    #define TEST_MUTEX(type, name)  Test<tbb::type>( name )

#endif /* !DO_ITT_NOTIFY */

int TestMain () {
#if DO_ITT_NOTIFY
    for( int p=MinThread; p<=MaxThread; ++p ) {
        REMARK( "testing with %d workers\n", p );
        tbb::task_scheduler_init init( p );
        TEST_MUTEX( spin_mutex, "Spin Mutex" );
        TEST_MUTEX( queuing_mutex, "Queuing Mutex" );
        TEST_MUTEX( queuing_rw_mutex, "Queuing RW Mutex" );
        TEST_MUTEX( spin_rw_mutex, "Spin RW Mutex" );
    }
    return Harness::Done;
#else /* !DO_ITT_NOTIFY */
    return Harness::Skipped;
#endif /* !DO_ITT_NOTIFY */
}
