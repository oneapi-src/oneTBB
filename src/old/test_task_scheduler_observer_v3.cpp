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

//TODO: when removing TBB_PREVIEW_LOCAL_OBSERVER, change the header or defines here
#include "tbb/task_scheduler_observer.h"

typedef uintptr_t FlagType;
const int MaxFlagIndex = sizeof(FlagType)*8-1;

class MyObserver: public tbb::task_scheduler_observer {
    FlagType flags;
    /*override*/ void on_scheduler_entry( bool is_worker );
    /*override*/ void on_scheduler_exit( bool is_worker );
public:
    MyObserver( FlagType flags_ ) : flags(flags_) {
        observe(true);
    }
};

#include "harness_assert.h"
#include "tbb/atomic.h"

tbb::atomic<int> EntryCount;
tbb::atomic<int> ExitCount;

struct State {
    FlagType MyFlags;
    bool IsMaster;
    State() : MyFlags(), IsMaster() {}
};

#include "../tbb/tls.h"
tbb::internal::tls<State*> LocalState;

void MyObserver::on_scheduler_entry( bool is_worker ) {
    State& state = *LocalState;
    ASSERT( is_worker==!state.IsMaster, NULL );
    ++EntryCount;
    state.MyFlags |= flags;
}

void MyObserver::on_scheduler_exit( bool is_worker ) {
    State& state = *LocalState;
    ASSERT( is_worker==!state.IsMaster, NULL );
    ++ExitCount;
    state.MyFlags &= ~flags;
}

#include "tbb/task.h"

class FibTask: public tbb::task {
    const int n;
    FlagType flags;
public:
    FibTask( int n_, FlagType flags_ ) : n(n_), flags(flags_) {}
    /*override*/ tbb::task* execute() {
        ASSERT( !(~LocalState->MyFlags & flags), NULL );
        if( n>=2 ) {
            set_ref_count(3);
            spawn(*new( allocate_child() ) FibTask(n-1,flags));
            spawn_and_wait_for_all(*new( allocate_child() ) FibTask(n-2,flags));
        }
        return NULL;
    }
};

void DoFib( FlagType flags ) {
    tbb::task* t = new( tbb::task::allocate_root() ) FibTask(10,flags);
    tbb::task::spawn_root_and_wait(*t);
}

#include "tbb/task_scheduler_init.h"
#include "harness.h"

class DoTest {
    int nthread;
public:
    DoTest( int n ) : nthread(n) {}
    void operator()( int i ) const {
        LocalState->IsMaster = true;
        if( i==0 ) {   
            tbb::task_scheduler_init init(nthread);
            DoFib(0);
        } else {
            FlagType f = i<=MaxFlagIndex? 1<<i : 0;
            MyObserver w(f);
            tbb::task_scheduler_init init(nthread);
            DoFib(f);
        }
    }
};

void TestObserver( int p, int q ) {
    NativeParallelFor( p, DoTest(q) );
}

int TestMain () {
    for( int p=MinThread; p<=MaxThread; ++p ) 
        for( int q=MinThread; q<=MaxThread; ++q ) 
            TestObserver(p,q);
    ASSERT( EntryCount>0, "on_scheduler_entry not exercised" );
    ASSERT( ExitCount>0, "on_scheduler_exit not exercised" );
    return Harness::Done;
}
