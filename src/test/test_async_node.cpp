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

#ifndef TBB_PREVIEW_FLOW_GRAPH_NODES
    #define TBB_PREVIEW_FLOW_GRAPH_NODES 1
#endif

#include "harness.h"

#if __TBB_PREVIEW_ASYNC_NODE

#include "tbb/concurrent_queue.h"
#include "tbb/flow_graph.h"
#include "tbb/task.h"

#include "tbb/tbb_thread.h"

#include <assert.h>

const size_t NUMBER_OF_SIGNALS = 10;

tbb::atomic<size_t> async_body_exec_count;
tbb::atomic<size_t> async_activity_processed_signnals_count;
tbb::atomic<size_t> end_body_exec_count;

struct signal_type {
    size_t signal;
    tbb::task* prev_task;
};

struct async_result_type {
    size_t signal;
    tbb::tbb_thread::id async_thread_id;
};

typedef tbb::flow::async_node< signal_type, async_result_type > async_node_type;

template< typename Input, typename Output >
class AsyncActivity {
public:
    typedef Input input_type;
    typedef Output output_type;
    typedef typename async_node_type::async_gateway_type async_gateway_type;
    struct work_type {
        input_type input;
        async_gateway_type* callback;
    };

    class ServiceThreadBody {
    public:
        ServiceThreadBody( AsyncActivity* activity ) : my_activity( activity ) {}

        void operator()() {
            my_activity->process();
        }
    private:
        AsyncActivity* my_activity;
    };

    AsyncActivity() {
        my_quit = false;
        tbb::tbb_thread( ServiceThreadBody( this ) ).swap( my_service_thread );
    }

private:
    AsyncActivity( const AsyncActivity& ) {
    }

public:
    ~AsyncActivity() {
        stop();
        my_service_thread.join();
    }

    void submit( input_type input, async_gateway_type& callback ) {
        work_type work = { input, &callback };
        my_work_queue.push( work );
    }

    void process() {
        do {
            work_type work;
            if( my_work_queue.try_pop( work ) ) {
                Harness::Sleep(500);
                
                ++async_activity_processed_signnals_count;
                
                async_result_type async_result;
                async_result.signal = work.input.signal;
                async_result.async_thread_id = tbb::this_tbb_thread::get_id();
                
                work.callback->async_try_put( async_result );
                work.callback->async_commit();
            }
        } while( my_quit == false || !my_work_queue.empty());
    }

    void stop() {
        my_quit = true;
    }

private:
    tbb::concurrent_queue< work_type > my_work_queue;

    tbb::atomic< bool > my_quit;

    tbb::tbb_thread my_service_thread;
};

class StartBody {
public:
    signal_type operator()( size_t input ) {
        //Harness::Sleep(100);

        signal_type output;
        output.signal = input;
        output.prev_task = &tbb::task::self(); 
        return output;
    }
};


template< typename Output >
class AsyncBody {
public:
    typedef AsyncActivity< signal_type, Output > async_activity_type;
    
    AsyncBody( async_activity_type* async_activity ) : my_async_activity( async_activity ) {
    }

    AsyncBody( const AsyncBody& other ) : my_async_activity( other.my_async_activity ) {
    }

    void operator()( signal_type input, async_node_type::async_gateway_type& my_node ) {
        ++async_body_exec_count;
       
        tbb::task* current_task = &tbb::task::self();
        ASSERT(input.prev_task != current_task, "async_node body was not executed in separate task");
        
        input.prev_task = current_task;
        my_async_activity->submit( input, my_node );
        my_node.async_reserve();
    }

private:
    async_activity_type* my_async_activity;
};

/* TODO: once the task_arena submit is possible, we need to check that
 * the StartBody and EndBody run in the same arena. */
class EndBody {
public:
    void operator()( async_result_type input ) {
        ASSERT( input.async_thread_id != tbb::this_tbb_thread::get_id(), "end_node body executed in service thread" );
        ++end_body_exec_count;
    }
};

#endif // __TBB_PREVIEW_ASYNC_NODE 

int TestMain() {
#if __TBB_PREVIEW_ASYNC_NODE
    AsyncActivity< signal_type, async_result_type > async_activity;

    tbb::flow::graph g;

    tbb::flow::function_node< size_t, signal_type > start_node( g, tbb::flow::unlimited, StartBody() );
    
    async_node_type async_node_obj( g, AsyncBody< async_result_type >( &async_activity ) );

    tbb::flow::function_node< async_result_type > end_node(g, tbb::flow::unlimited, EndBody() );

    make_edge( start_node, async_node_obj );
    make_edge( async_node_obj, end_node );

    for (size_t i = 0; i < NUMBER_OF_SIGNALS; ++i) {
        start_node.try_put( i );
    }

    g.wait_for_all();

    ASSERT( async_body_exec_count == NUMBER_OF_SIGNALS, "AsyncBody procesed wrong number of signals" );
    ASSERT( async_activity_processed_signnals_count == NUMBER_OF_SIGNALS, "AsyncActivity processed wrong number of signals" );
    ASSERT( end_body_exec_count == NUMBER_OF_SIGNALS, "EndBody processed wrong number of signals");

    return Harness::Done;
#else
    return Harness::Skipped;
#endif
}
