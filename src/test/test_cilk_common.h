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

// This file is a common part of test_cilk_interop and test_cilk_dynamic_load tests

int TBB_Fib( int n );

class FibCilkSubtask: public tbb::task {
    int n;
    int& result;
    /*override*/ task* execute() {
        if( n<2 ) {
            result = n;
        } else {
            int x, y;
            x = cilk_spawn TBB_Fib(n-2);
            y = cilk_spawn TBB_Fib(n-1);
            cilk_sync;
            result = x+y;
        }
        return NULL;
    }
public:
    FibCilkSubtask( int& result_, int n_ ) : result(result_), n(n_) {}
};

class FibTask: public tbb::task {
    int n;
    int& result;
    /*override*/ task* execute() {
        if( !g_sandwich && n<2 ) {
            result = n;
        } else {
            int x,y;
            tbb::task_scheduler_init init(P_nested);
            task* self0 = &task::self();
            set_ref_count( 3 );
            if ( g_sandwich ) {
                spawn (*new( allocate_child() ) FibCilkSubtask(x,n-1));
                spawn (*new( allocate_child() ) FibCilkSubtask(y,n-2));
            }
            else {
                spawn (*new( allocate_child() ) FibTask(x,n-1));
                spawn (*new( allocate_child() ) FibTask(y,n-2));
            }
            wait_for_all();
            task* self1 = &task::self();
            ASSERT( self0 == self1, "failed to preserve TBB TLS" );
            result = x+y;
        }
        return NULL;
    }
public:
    FibTask( int& result_, int n_ ) : result(result_), n(n_) {}
};

int TBB_Fib( int n ) {
    if( n<2 ) {
        return n;
    } else {
        int result;
        tbb::task_scheduler_init init(P_nested);
        tbb::task::spawn_root_and_wait(*new( tbb::task::allocate_root()) FibTask(result,n) );
        return result;
    }
}
