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
#include "rml_omp.h"
#include "tbb/atomic.h"
#include "tbb/tick_count.h"

#define HARNESS_DEFAULT_MIN_THREADS 4
#include "harness.h"

// dynamic_link initializes its data structures in a static constructor. But
// the initialization order of static constructors in different modules is
// non-deterministic. Thus dynamic_link fails on some systems when the
// application changes its current directory after the library (TBB/OpenMP/...)
// is loaded but before the static constructors in the library are executed.
#define CHDIR_SUPPORT_BROKEN ( ( __GNUC__ == 4 && __GNUC_MINOR__ >= 6 && __GNUC_MINOR__ <= 9 ) || (__linux__ && __clang_major__ == 3 && __clang_minor__ == 5) )

const int OMP_ParallelRegionSize = 16;
int TBB_MaxThread = 4;           // Includes master
int OMP_MaxThread = int(~0u>>1); // Includes master

template<typename Client>
class ClientBase: public Client {
protected:
    typedef typename Client::version_type version_type;
    typedef typename Client::job job;
    typedef typename Client::policy_type policy_type;

private:
    /*override*/version_type version() const {
        return 0;
    }
    /*override*/size_t min_stack_size() const {
        return 1<<20;
    }
    /*override*/job* create_one_job() {
        return new rml::job;
    }
    /*override*/policy_type policy() const {
        return Client::turnaround;
    }
    /*override*/void acknowledge_close_connection() {
        delete this;
    }
    /*override*/void cleanup( job& j ) {delete &j;}

public:
    virtual ~ClientBase() {}
};

#if _WIN32
#include <direct.h>
#define PATH_LEN MAX_PATH+1
#define SLASH '\\'
#define ROOT_DIR "\\"
// ROOT_DIR_REST means how many symbols before first slash in the path
#define ROOT_DIR_REST 2
#else
#include <unistd.h>
#include <limits.h>
#define PATH_LEN PATH_MAX+1
#define SLASH '/'
#define ROOT_DIR "/"
// ROOT_DIR_REST means how many symbols before first slash in the path
#define ROOT_DIR_REST 0
#define _getcwd getcwd
#define _chdir  chdir
#endif

#if !CHDIR_SUPPORT_BROKEN
class ChangeCurrentDir {
    char dir[PATH_LEN+1];
    char *last_slash;
public:
    ChangeCurrentDir() {
        if ( !_getcwd( dir, PATH_LEN ) ) {
            REPORT_FATAL_ERROR("ERROR: Couldn't get current working directory\n");
        }

        last_slash = strrchr( dir, SLASH );
        ASSERT( last_slash, "The current directory doesn't contain slashes" );
        *last_slash = 0;

        if ( _chdir( last_slash-dir == ROOT_DIR_REST ? ROOT_DIR : dir ) ) {
            REPORT_FATAL_ERROR("ERROR: Couldn't change current working directory (%s)\n", dir );
        }
    }

    // Restore current dir
    ~ChangeCurrentDir() {
        *last_slash = SLASH;
        if ( _chdir(dir) ) {
            REPORT_FATAL_ERROR("ERROR: Couldn't change current working directory\n");
        }
    }
};
#endif

//! Represents a TBB or OpenMP run-time that uses RML.
template<typename Factory, typename Client>
class RunTime {
public:
    //! Factory that run-time uses to make servers.
    Factory factory;
    Client* client;
    typename Factory::server_type* server;
#if _WIN32||_WIN64
    ::rml::server::execution_resource_t me;
#endif
    RunTime() {
        factory.open();
    }
    ~RunTime() {
        factory.close();
    }
    //! Create server for this run-time
    void create_connection();

    //! Destroy server for this run-time
    void destroy_connection();
};

class ThreadLevelRecorder {
    tbb::atomic<int> level;
    struct record {
        tbb::tick_count time;
        int nthread;
    };
    tbb::atomic<unsigned> next;
    /** Must be power of two */
    static const unsigned max_record_count = 1<<20;
    record array[max_record_count];
public:
    void change_level( int delta );
    void dump();
};

void ThreadLevelRecorder::change_level( int delta ) {
    int x = level+=delta;
    tbb::tick_count t = tbb::tick_count::now();
    unsigned k = next++;
    if( k<max_record_count ) {
        record& r = array[k];
        r.time = t;
        r.nthread = x;
    } 
}

void ThreadLevelRecorder::dump() {
    FILE* f = fopen("time.txt","w");
    if( !f ) {
        perror("fopen(time.txt)\n");
        exit(1);
    }
    unsigned limit = next;
    if( limit>max_record_count ) {
        // Clip
        limit = next;
    }
    for( unsigned i=0; i<limit; ++i ) {
        fprintf(f,"%f\t%d\n",(array[i].time-array[0].time).seconds(),array[i].nthread);
    }
    fclose(f);
}

ThreadLevelRecorder TotalThreadLevel;

class TBB_Client: public ClientBase<tbb::internal::rml::tbb_client> {
    /*override*/void process( job& j );
    /*override*/size_type max_job_count() const {
        return TBB_MaxThread-1;
    }
};

class OMP_Client: public ClientBase<__kmp::rml::omp_client> {
    /*override*/void process( job&, void* cookie, omp_client::size_type );
    /*override*/size_type max_job_count() const {
        return OMP_MaxThread-1;
    }
};

#if !CHDIR_SUPPORT_BROKEN
// A global instance of ChangeCurrentDir should be declared before TBB_RunTime and OMP_RunTime
// since we want to change current directory before opening factory
ChangeCurrentDir Changer;
#endif
RunTime<tbb::internal::rml::tbb_factory, TBB_Client> TBB_RunTime;
RunTime<__kmp::rml::omp_factory, OMP_Client> OMP_RunTime;

template<typename Factory, typename Client>
void RunTime<Factory,Client>::create_connection() {
    client = new Client;
    typename Factory::status_type status = factory.make_server( server, *client );
    ASSERT( status==Factory::st_success, NULL );
#if _WIN32||_WIN64
    server->register_master( me );
#endif /* _WIN32||_WIN64 */
}

template<typename Factory, typename Client>
void RunTime<Factory,Client>::destroy_connection() {
#if _WIN32||_WIN64
    server->unregister_master( me );
#endif /* _WIN32||_WIN64 */
    server->request_close_connection();
    server = NULL;
}

class OMP_Team {
public:
    OMP_Team( __kmp::rml::omp_server& ) {}
    tbb::atomic<unsigned> barrier;
};

tbb::atomic<int> AvailWork;
tbb::atomic<int> CompletionCount;
 
void OMPWork() {
    tbb::atomic<int> x;
    for( x=0; x<2000000; ++x ) {
        continue;
    }
}

void TBBWork() {
    if( AvailWork>=0 ) {
        int k = --AvailWork;
        if( k==-1 ) {
            TBB_RunTime.server->adjust_job_count_estimate(-(TBB_MaxThread-1));
            ++CompletionCount;
        } else if( k>=0 ) {
            for( int k=0; k<4; ++k ) {
                OMP_Team team( *OMP_RunTime.server );
                int n = OMP_RunTime.server->try_increase_load( OMP_ParallelRegionSize-1, /*strict=*/false );
                team.barrier = 0;
                ::rml::job* array[OMP_ParallelRegionSize-1];
                if( n>0)
                    OMP_RunTime.server->get_threads( n, &team, array );
                // Master does work inside parallel region too.
                OMPWork();
                // Master waits for workers to finish
                if( n>0 )
                    while( team.barrier!=unsigned(n) ) {
                        __TBB_Yield();
                    } 
            }
            ++CompletionCount;
        }
    }
}

/*override*/void TBB_Client::process( job& ) {
    TotalThreadLevel.change_level(1);
    TBBWork();
    TotalThreadLevel.change_level(-1);
}  

/*override*/void OMP_Client::process( job& /* j */, void* cookie, omp_client::size_type ) {
    TotalThreadLevel.change_level(1);
    ASSERT( OMP_RunTime.server, NULL );
    OMPWork();
    ASSERT( OMP_RunTime.server, NULL );
    static_cast<OMP_Team*>(cookie)->barrier+=1;
    TotalThreadLevel.change_level(-1);
}

void TBBOutSideOpenMPInside() {
    TotalThreadLevel.change_level(1);
    CompletionCount = 0;
    int tbbtasks = 32;
    AvailWork = tbbtasks;
    TBB_RunTime.server->adjust_job_count_estimate(TBB_MaxThread-1);
    while( CompletionCount!=tbbtasks+1 ) {
        TBBWork();
    }
    TotalThreadLevel.change_level(-1);
}

int TestMain () {
#if CHDIR_SUPPORT_BROKEN
    REPORT("Known issue: dynamic_link does not support current directory changing before its initialization.\n");
#endif
    for( int TBB_MaxThread=MinThread; TBB_MaxThread<=MaxThread; ++TBB_MaxThread ) {
        REMARK("Testing with TBB_MaxThread=%d\n", TBB_MaxThread);
        TBB_RunTime.create_connection();
        OMP_RunTime.create_connection();
        TBBOutSideOpenMPInside();
        OMP_RunTime.destroy_connection();
        TBB_RunTime.destroy_connection();
    }
    TotalThreadLevel.dump();
    return Harness::Done;
}
#endif /* __TBB_WIN8UI_SUPPORT || __TBB_MIC_OFFLOAD */
