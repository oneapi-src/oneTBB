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

#define TBB_PREVIEW_WAITING_FOR_WORKERS 1
#include "tbb/task_scheduler_init.h"
#include "tbb/blocked_range.h"
#include "tbb/cache_aligned_allocator.h"
#include "tbb/parallel_for.h"

#define HARNESS_DEFAULT_MIN_THREADS (tbb::task_scheduler_init::default_num_threads())
#define HARNESS_DEFAULT_MAX_THREADS (4*tbb::task_scheduler_init::default_num_threads())
#if __bg__
// CNK does not support fork()
#define HARNESS_SKIP_TEST 1
#endif
#include "harness.h"

#if _WIN32||_WIN64
#include "tbb/concurrent_hash_map.h"

HANDLE getCurrentThreadHandle()
{
    HANDLE hProc = GetCurrentProcess(), hThr = INVALID_HANDLE_VALUE;
#if TBB_USE_ASSERT
    BOOL res =
#endif
    DuplicateHandle( hProc, GetCurrentThread(), hProc, &hThr, 0, FALSE, DUPLICATE_SAME_ACCESS );
    __TBB_ASSERT( res, "Retrieving current thread handle failed" );
    return hThr;
}

bool threadTerminated(HANDLE h)
{
    DWORD ret = WaitForSingleObjectEx(h, 0, FALSE);
    return WAIT_OBJECT_0 == ret;
}

struct Data {
    HANDLE h;
};

typedef tbb::concurrent_hash_map<DWORD, Data> TidTableType;

static TidTableType tidTable;

#else

#if __sun || __SUNPRO_CC
#define _POSIX_PTHREAD_SEMANTICS 1 // to get standard-conforming sigwait(2)
#endif
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sched.h>

#include "tbb/tick_count.h"

static void SigHandler(int) { }

#endif // _WIN32||_WIN64

class AllocTask {
public:
    void operator() (const tbb::blocked_range<int> &r) const {
#if _WIN32||_WIN64
        HANDLE h = getCurrentThreadHandle();
        DWORD tid = GetCurrentThreadId();
        {
            TidTableType::accessor acc;
            if (tidTable.insert(acc, tid)) {
                acc->second.h = h;
            }
        }
#endif
        for (int y = r.begin(); y != r.end(); ++y) {
            void *p = tbb::internal::NFS_Allocate(1, 7000, NULL);
            tbb::internal::NFS_Free(p);
        }
    }
    AllocTask() {}
};

/* Regression test against data race between termination of workers
   and setting blocking terination mode in main thread. */
class RunWorkersBody : NoAssign {
    bool wait_workers;
public:
    RunWorkersBody(bool waitWorkers) : wait_workers(waitWorkers) {}
    void operator()(const int /*threadID*/) const {
        tbb::task_scheduler_init sch(MaxThread, 0, wait_workers);
        tbb::parallel_for(tbb::blocked_range<int>(0, 10000, 1), AllocTask(),
                          tbb::simple_partitioner());
    }
};

void TestBlockNonblock()
{
    for (int i=0; i<100; i++) {
        REMARK("\rIteration %d ", i);
        NativeParallelFor(4, RunWorkersBody(/*wait_workers=*/false));
        RunWorkersBody(/*wait_workers=*/true)(0);
    }
}

class RunInNativeThread : NoAssign {
    bool create_tsi;
public:
    RunInNativeThread(bool create_tsi_) : create_tsi(create_tsi_) {}
    void operator()(const int /*threadID*/) const {
        // nested TSI or auto-initialized TSI can be terminated when
        // wait_workers is true (deferred TSI means auto-initialization)
        tbb::task_scheduler_init tsi(create_tsi? 2 :
                                     tbb::task_scheduler_init::deferred);
        tbb::parallel_for(tbb::blocked_range<int>(0, 10000, 1), AllocTask(),
                              tbb::simple_partitioner());
    }
};

void TestTasksInThread()
{
    tbb::task_scheduler_init sch(2, 0, /*wait_workers=*/true);
    tbb::parallel_for(tbb::blocked_range<int>(0, 10000, 1), AllocTask(),
                      tbb::simple_partitioner());
    for (int i=0; i<2; i++)
        NativeParallelFor(2, RunInNativeThread(/*create_tsi=*/1==i));
}

int TestMain()
{
    using namespace Harness;

    TestBlockNonblock();
    TestTasksInThread();

    bool child = false;
#if _WIN32||_WIN64
    DWORD masterTid = GetCurrentThreadId();
#else
    struct sigaction sa;
    sigset_t sig_set;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = SigHandler;
    if (sigaction(SIGCHLD, &sa, NULL))
        ASSERT(0, "sigaction failed");
    if (sigaction(SIGALRM, &sa, NULL))
        ASSERT(0, "sigaction failed");
    // block SIGCHLD and SIGALRM, the mask is inherited by worker threads
    sigemptyset(&sig_set);
    sigaddset(&sig_set, SIGCHLD);
    sigaddset(&sig_set, SIGALRM);
    if (pthread_sigmask(SIG_BLOCK, &sig_set, NULL))
        ASSERT(0, "pthread_sigmask failed");
#endif
    for (int threads=MinThread; threads<=MaxThread; threads+=MinThread) {
        for (int i=0; i<20; i++) {
            if (!child)
                REMARK("\rThreads %d %d ", threads, i);
            {
                tbb::task_scheduler_init sch(threads, 0, /*wait_workers=*/true);
            }
            tbb::task_scheduler_init sch(threads, 0, /*wait_workers=*/true);

            tbb::parallel_for(tbb::blocked_range<int>(0, 10000, 1), AllocTask(),
                              tbb::simple_partitioner());
            sch.terminate();

#if _WIN32||_WIN64
            // check that there is no alive threads after terminate()
            for (TidTableType::const_iterator it = tidTable.begin();
                 it != tidTable.end(); ++it) {
                if (masterTid != it->first) {
                    ASSERT(threadTerminated(it->second.h), NULL);
                }
            }
            tidTable.clear();
#else // _WIN32||_WIN64
            if (child)
                exit(0);
            else {
                pid_t pid = fork();
                if (!pid) {
                    i = -1;
                    child = true;
                } else {
                    int sig;
                    pid_t w_ret = 0;
                    // wait for SIGCHLD up to timeout
                    alarm(30);
                    if (0 != sigwait(&sig_set, &sig))
                        ASSERT(0, "sigwait failed");
                    alarm(0);
                    w_ret = waitpid(pid, NULL, WNOHANG);
                    ASSERT(w_ret>=0, "waitpid failed");
                    if (!w_ret) {
                        ASSERT(!kill(pid, SIGKILL), NULL);
                        w_ret = waitpid(pid, NULL, 0);
                        ASSERT(w_ret!=-1, "waitpid failed");

                        ASSERT(0, "Hang after fork");
                    }
                    // clean pending signals (if any occurs since sigwait)
                    sigset_t p_mask;
                    for (;;) {
                        sigemptyset(&p_mask);
                        sigpending(&p_mask);
                        if (sigismember(&p_mask, SIGALRM)
                            || sigismember(&p_mask, SIGCHLD)) {
                            if (0 != sigwait(&p_mask, &sig))
                                ASSERT(0, "sigwait failed");
                        } else
                            break;
                    }
                }
            }
#endif // _WIN32||_WIN64
        }
    }
    REMARK("\n");
#if TBB_USE_EXCEPTIONS
    REMARK("Testing exceptions\n");
    try {
        {
            tbb::task_scheduler_init schBlock(2, 0, /*wait_workers=*/true);
            tbb::task_scheduler_init schBlock1(2, 0, /*wait_workers=*/true);
        }
        ASSERT(0, "Nesting of blocking schedulers is impossible.");
    } catch (...) {}
#endif

    return Harness::Done;
}

