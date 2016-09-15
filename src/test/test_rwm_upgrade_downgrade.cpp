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

#define HARNESS_DEFAULT_MIN_THREADS 4
#define HARNESS_DEFAULT_MAX_THREADS 4

#include "tbb/queuing_rw_mutex.h"
#include "tbb/spin_rw_mutex.h"
#include "harness.h"

using namespace tbb;

volatile int Count;

template<typename RWMutex>
struct Hammer: NoAssign {
    RWMutex &MutexProtectingCount;
    mutable volatile int dummy;

    Hammer(RWMutex &m): MutexProtectingCount(m) {}
    void operator()( int /*thread_id*/ ) const {
        for( int j=0; j<100000; ++j ) {
            typename RWMutex::scoped_lock lock(MutexProtectingCount,false);
            int c = Count;
            for( int k=0; k<10; ++k ) {
                ++dummy;
            }
            if( lock.upgrade_to_writer() ) {
                // The upgrade succeeded without any intervening writers
                ASSERT( c==Count, "another thread modified Count while I held a read lock" );
            } else {
                c = Count;
            }
            for( int k=0; k<10; ++k ) {
                ++Count;
            }
            lock.downgrade_to_reader();
            for( int k=0; k<10; ++k ) {
                ++dummy;
            }
        }
    }
};

queuing_rw_mutex QRW_mutex;
spin_rw_mutex SRW_mutex;

int TestMain () {
    for( int p=MinThread; p<=MaxThread; ++p ) {
        REMARK("Testing on %d threads", p);
        Count = 0;
        NativeParallelFor( p, Hammer<queuing_rw_mutex>(QRW_mutex) );
        Count = 0;
        NativeParallelFor( p, Hammer<spin_rw_mutex>(SRW_mutex) );
    }
    return Harness::Done;
}
