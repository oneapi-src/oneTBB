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

#include <tbb/tbb_config.h>
#if __TBB_WIN8UI_SUPPORT || __TBB_MIC_OFFLOAD
#include "harness.h"
int TestMain () {
    return Harness::Skipped;
}
#else
#include "rml_tbb.h"

typedef tbb::internal::rml::tbb_server MyServer;
typedef tbb::internal::rml::tbb_factory MyFactory;

// Forward declaration of the function used in test_server.h
void DoClientSpecificVerification( MyServer&, int );

#define HARNESS_DEFAULT_MIN_THREADS 0
#include "test_server.h"

tbb::atomic<int> n_available_hw_threads;

class MyClient: public ClientBase<tbb::internal::rml::tbb_client> {
    tbb::atomic<int> counter;
    tbb::atomic<int> gate;
    /*override*/void process( job& j ) {
        do_process(j);
        //wait until the gate is open.
        while( gate==0 )
            Harness::Sleep(1);

        __TBB_ASSERT( nesting.limit<=2, NULL );
        if( nesting.level>=nesting.limit )
            return;

        size_type max_outstanding_connections = max_job_count(); // if nesting.level==0
        if( nesting.level==1 )
            max_outstanding_connections *= (1+max_outstanding_connections);

        if( default_concurrency()<=max_outstanding_connections+2 )
            // i.e., if it is not guaranteed that at least two connections may be made without depleting the_balance
            return;

        // at this point, ( nesting.level<nesting.limit ) && ( my_server->default_concurrency()-max_outstanding_connections>2 ) 
        for( ;; ) {
            while( n_available_hw_threads<=1 )
                Harness::Sleep(1);

            int n = --n_available_hw_threads;
            if( n>0 ) break;
            // else I lost
            ++n_available_hw_threads;
        }
        
        DoOneConnection<MyFactory,MyClient> doc(max_job_count(),Nesting(nesting.level+1,nesting.limit),0,false);
        doc(0);

        ++n_available_hw_threads;
    }
public:
    MyClient() {counter=1;}
    static const bool is_omp = false;
    bool is_strict() const {return false;}
    void open_the_gate() { gate = 1; }
    void close_the_gate() { gate = 0; }
};

void FireUpJobs( MyServer& server, MyClient& client, int n_thread, int n_extra, Checker* checker ) {
    REMARK("client %d: calling adjust_job_count_estimate(%d)\n", client.client_id(),n_thread);
    // Exercise independent_thread_number_changed, even for zero values.
    server.independent_thread_number_changed( n_extra );
#if _WIN32||_WIN64
    ::rml::server::execution_resource_t me;
    server.register_master( me );
#endif /* _WIN32||_WIN64 */
    // Experiments indicate that when oversubscribing, the main thread should wait a little
    // while for the RML worker threads to do some work. 
    if( checker ) {
        // Give RML time to respond to change in number of threads.
        Harness::Sleep(1);
        for( int k=0; k<n_thread; ++k )
            client.job_array[k].processing_count = 0;
    }
    //close the gate to keep worker threads from returning to RML until a snapshot is taken
    client.close_the_gate();
    server.adjust_job_count_estimate( n_thread );
    int n_used = 0;
    if( checker ) {
        Harness::Sleep(100);
        for( int k=0; k<n_thread; ++k )
            if( client.job_array[k].processing_count )
                ++n_used;
    }
    // open the gate
    client.open_the_gate();
    // Logic further below presumes that jobs never starve, so undo previous call
    // to independent_thread_number_changed before waiting on those jobs.
    server.independent_thread_number_changed( -n_extra );
    REMARK("client %d: wait for each job to be processed at least once\n",client.client_id());
    // Calculate the number of jobs that are expected to get threads.
    int expected = n_thread;
    // Wait for expected number of jobs to be processed.
#if RML_USE_WCRM
    int default_concurrency = server.default_concurrency();
    if( N_TestConnections>0 ) {
        if( default_concurrency+1>=8 && n_thread<=3 && N_TestConnections<=3 && (default_concurrency/int(N_TestConnections)-1)>=n_thread ) {
#endif /* RML_USE_WCRM */
            for(;;) {
                int n = 0;
                for( int k=0; k<n_thread; ++k ) 
                    if( client.job_array[k].processing_count!=0 ) 
                        ++n;
                    if( n>=expected ) break;
                server.yield();
            }
#if RML_USE_WCRM
        } else if( n_thread>0 ) {
            for( int m=0; m<20; ++m ) {
                int n = 0;
                for( int k=0; k<n_thread; ++k ) 
                    if( client.job_array[k].processing_count!=0 ) 
                        ++n;
                if( n>=expected ) break;
                Harness::Sleep(1);
            }
        }
    }
#endif /* RML_USE_WCRM */
    server.adjust_job_count_estimate(-n_thread);
#if _WIN32||_WIN64
    server.unregister_master( me );
#endif
    // Give RML some time to respond
    if( checker ) {
        Harness::Sleep(1);
        checker->check_number_of_threads_delivered( n_used, n_thread, n_extra );
    }
}

void DoClientSpecificVerification( MyServer&, int n_thread )
{
    MyClient* client = new MyClient;
    client->initialize( n_thread, Nesting(), ClientStackSize[0] );
    MyFactory factory;
    memset( &factory, 0, sizeof(factory) );
    MyFactory::status_type status = factory.open();
    ASSERT( status!=MyFactory::st_not_found, "could not find RML library" );
    ASSERT( status!=MyFactory::st_incompatible, NULL );
    ASSERT( status==MyFactory::st_success, NULL );
    MyFactory::server_type* server; 
    status = factory.make_server( server, *client );
    ASSERT( status==MyFactory::st_success, NULL );
    client->set_server( server );
    client->expect_close_connection = true;
    server->request_close_connection();
    // Client deletes itself when it sees call to acknowledge_close_connection from server.
    factory.close();
}

void Initialize()
{
    MyClient* client = new MyClient;
    client->initialize( 1, Nesting(), ClientStackSize[0] );
    MyFactory factory;
    memset( &factory, 0, sizeof(factory) );
    factory.open();
    MyFactory::server_type* server; 
    factory.make_server( server, *client );
    client->set_server( server );
    n_available_hw_threads = server->default_concurrency();
    client->expect_close_connection = true;
    server->request_close_connection();
    // Client deletes itself when it sees call to acknowledge_close_connection from server.
    factory.close();
}

int TestMain () {
    VerifyInitialization<MyFactory,MyClient>( MaxThread );
    if ( default_concurrency<1 ) {
         REPORT("The test is not intended to run on 1 thread\n");
         return Harness::Skipped;
    }
    Initialize();
    SimpleTest<MyFactory,MyClient>();
    return Harness::Done;
}
#endif /* __TBB_WIN8UI_SUPPORT || __TBB_MIC_OFFLOAD */
