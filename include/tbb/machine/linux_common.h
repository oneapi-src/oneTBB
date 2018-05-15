/*
    Copyright (c) 2005-2018 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.




*/

#ifndef __TBB_machine_H
#error Do not #include this internal file directly; use public TBB headers instead.
#endif

#include <sched.h>
#define __TBB_Yield()  sched_yield()

#include <unistd.h>
/* Futex definitions */
#include <sys/syscall.h>

#if defined(SYS_futex)

/*
 * Note that this header is included not only on Linux. Some BSD systems may also support futexes,
 * but provide the definitions in other headers. For backward compatibility we also support
 * systems that don't provide any header but support futexes, as indicated by the SYS_futex macro.
 */
#define __TBB_USE_FUTEX 1

#if defined(__linux__)
#include <linux/futex.h>
#elif defined(__OpenBSD__)
#include <sys/futex.h>
#elif defined(__has_include)
#if __has_include(<sys/futex.h>)
#include <sys/futex.h>
#endif
#endif

#include <limits.h>
#include <errno.h>

#if defined(FUTEX_WAIT_PRIVATE)
#define __TBB_FUTEX_WAIT FUTEX_WAIT_PRIVATE
#elif defined(FUTEX_WAIT)
#define __TBB_FUTEX_WAIT FUTEX_WAIT
#else
#define __TBB_FUTEX_WAIT 0
#endif

#if defined(FUTEX_WAKE_PRIVATE)
#define __TBB_FUTEX_WAKE FUTEX_WAKE_PRIVATE
#elif defined(FUTEX_WAKE)
#define __TBB_FUTEX_WAKE FUTEX_WAKE
#else
#define __TBB_FUTEX_WAKE 1
#endif

#ifndef __TBB_ASSERT
#error machine specific headers must be included after tbb_stddef.h
#endif

namespace tbb {

namespace internal {

inline int futex_wait( void *futex, int comparand ) {
    int r = syscall( SYS_futex,futex,__TBB_FUTEX_WAIT,comparand,NULL,NULL,0 );
#if TBB_USE_ASSERT
    int e = errno;
    __TBB_ASSERT( r==0||r==EWOULDBLOCK||(r==-1&&(e==EAGAIN||e==EINTR)), "futex_wait failed." );
#endif /* TBB_USE_ASSERT */
    return r;
}

inline int futex_wakeup_one( void *futex ) {
    int r = ::syscall( SYS_futex,futex,__TBB_FUTEX_WAKE,1,NULL,NULL,0 );
    __TBB_ASSERT( r==0||r==1, "futex_wakeup_one: more than one thread woken up?" );
    return r;
}

inline int futex_wakeup_all( void *futex ) {
    int r = ::syscall( SYS_futex,futex,__TBB_FUTEX_WAKE,INT_MAX,NULL,NULL,0 );
    __TBB_ASSERT( r>=0, "futex_wakeup_all: error in waking up threads" );
    return r;
}

} /* namespace internal */

} /* namespace tbb */

#endif /* SYS_futex */
