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
#include "rml_omp.h"

typedef __kmp::rml::omp_server MyServer;
typedef __kmp::rml::omp_factory MyFactory;

// Forward declaration for the function used in test_server.h
void DoClientSpecificVerification( MyServer& , int );

#define HARNESS_DEFAULT_MIN_THREADS 0
#include "test_server.h"
#include "tbb/tbb_misc.h"

static bool StrictTeam;

class MyTeam {
    MyTeam& operator=( const MyTeam& ) ;
public:
    struct info_type {
        rml::job* job;
        bool ran;
        info_type() : job(NULL), ran(false) {}
    };
    MyTeam( MyServer& /* server */, size_t max_thread_ ) :
        max_thread(max_thread_)
    {
        self_ptr = this;
        info = new info_type[max_thread];
    }
    ~MyTeam() {
        delete[] info;
    }
    const size_t max_thread;
    size_t n_thread;
    tbb::atomic<int> barrier;
    /** Indexed with 1-origin index */
    info_type* info;
    int iteration;
    MyTeam* self_ptr;
};

class MyClient: public ClientBase<__kmp::rml::omp_client> {
public:
    MyServer* server;
    /*override*/void process( job& j, void* cookie, size_type index ) {
        MyTeam& t = *static_cast<MyTeam*>(cookie);
        ASSERT( t.self_ptr==&t, "trashed cookie" );
        ASSERT( index<t.max_thread, NULL );
        ASSERT( !t.info[index].ran, "duplicate index?" );
        t.info[index].job = &j;
        t.info[index].ran = true;
        do_process(j);
        if( index==1 && nesting.level<nesting.limit ) {
            DoOneConnection<MyFactory,MyClient> doc(MaxThread,Nesting(nesting.level+1,nesting.limit),0,false);
            doc(0);
        }
#if _WIN32||_WIN64
        // test activate/deactivate
        if( t.n_thread>1 && t.n_thread%2==0 ) {
            if( nesting.level==0 ) {
                if( index&1 ) {
                    size_type target = index-1;
                    ASSERT(  target<t.max_thread, NULL );
                    // wait until t.info[target].job is defined
                    tbb::internal::spin_wait_until_eq( t.info[target].ran, true );
                    server->try_increase_load( 1, true );
                    server->reactivate( t.info[target].job );
                } else {
                    server->deactivate( &j );
                }
            }
        }
#endif /* _WIN32||_WIN64 */
        ++t.barrier;
    }
    static const bool is_omp = true;
    bool is_strict() const {return StrictTeam;}
};

void FireUpJobs( MyServer& server, MyClient& client, int max_thread, int n_extra, Checker* checker ) {
    ASSERT( max_thread>=0, NULL );
#if _WIN32||_WIN64
    ::rml::server::execution_resource_t me;
    server.register_master( me );
#endif /* _WIN32||_WIN64 */
    client.server = &server;
    MyTeam team(server,size_t(max_thread));
    MyServer::size_type n_thread = 0;
    for( int iteration=0; iteration<4; ++iteration ) {
        for( size_t i=0; i<team.max_thread; ++i )
            team.info[i].ran = false;
        switch( iteration ) {
            default:
                n_thread = int(max_thread);
                break;
            case 1:
                // No change in number of threads
                break;
            case 2:
                // Decrease number of threads.
                n_thread = int(max_thread)/2;
                break;
            // Case 3 is same code as the default, but has effect of increasing the number of threads.
        }
        team.barrier = 0;
        REMARK("client %d: server.run with n_thread=%d\n", client.client_id(), int(n_thread) );
        server.independent_thread_number_changed( n_extra );
        if( checker ) {
            // Give RML time to respond to change in number of threads.
            Harness::Sleep(1);
        }
        int n_delivered = server.try_increase_load( n_thread, StrictTeam );
        ASSERT( !StrictTeam || n_delivered==int(n_thread), "server failed to satisfy strict request" );
        if( n_delivered<0 ) {
            REMARK( "client %d: oversubscription occurred (by %d)\n", client.client_id(), -n_delivered );
            server.independent_thread_number_changed( -n_extra );
            n_delivered = 0;
        } else {
            team.n_thread = n_delivered;
            ::rml::job* job_array[JobArraySize];
            job_array[n_delivered] = (::rml::job*)intptr_t(-1);
            server.get_threads( n_delivered, &team, job_array );
            __TBB_ASSERT( job_array[n_delivered]== (::rml::job*)intptr_t(-1), NULL );
            for( int i=0; i<n_delivered; ++i ) {
                MyJob* j = static_cast<MyJob*>(job_array[i]);
                int s = j->state;
                ASSERT( s==MyJob::idle||s==MyJob::busy, NULL );
            }
            server.independent_thread_number_changed( -n_extra );
            REMARK("client %d: team size is %d\n", client.client_id(), n_delivered);
            if( checker ) {
                checker->check_number_of_threads_delivered( n_delivered, n_thread, n_extra );
            }
            // Protocol requires that master wait until workers have called "done_processing"
            while( team.barrier!=n_delivered ) {
                ASSERT( team.barrier>=0, NULL );
                ASSERT( team.barrier<=n_delivered, NULL );
                __TBB_Yield();
            }
            REMARK("client %d: team completed\n", client.client_id() );
            for( int i=0; i<n_delivered; ++i ) {
                ASSERT( team.info[i].ran, "thread on team allegedly delivered, but did not run?" );
            }
        }
        for( MyServer::size_type i=n_delivered; i<MyServer::size_type(max_thread); ++i ) {
            ASSERT( !team.info[i].ran, "thread on team ran with illegal index" );
        }
    }
#if _WIN32||_WIN64
    server.unregister_master( me );
#endif
}

void DoClientSpecificVerification( MyServer& server, int /*n_thread*/ )
{
    ASSERT( server.current_balance()==int(tbb::internal::AvailableHwConcurrency())-1, NULL );
}

int TestMain () {
#if _MSC_VER == 1600 && RML_USE_WCRM
    REPORT("Known issue: RML resets the process mask when Concurrency Runtime is used.\n");
    // AvailableHwConcurrency reads process mask when the first call. That's why it should
    // be called before RML initialization.
    tbb::internal::AvailableHwConcurrency();
#endif

    StrictTeam = true;
    VerifyInitialization<MyFactory,MyClient>( MaxThread );
    SimpleTest<MyFactory,MyClient>();

    StrictTeam = false;
    VerifyInitialization<MyFactory,MyClient>( MaxThread );
    SimpleTest<MyFactory,MyClient>();

    return Harness::Done;
}
#endif /* __TBB_WIN8UI_SUPPORT || __TBB_MIC_OFFLOAD */
