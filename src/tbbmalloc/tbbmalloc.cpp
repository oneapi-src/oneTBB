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

#include "TypeDefinitions.h" // Customize.h and proxy.h get included
#include "tbbmalloc_internal_api.h"

#include "../tbb/itt_notify.h" // for __TBB_load_ittnotify()

#include "../tbb/tbb_assert_impl.h" // Out-of-line TBB assertion handling routines are instantiated here.

#undef UNICODE

#if USE_PTHREAD
#include <dlfcn.h> // dlopen
#elif USE_WINTHREAD
#include "tbb/machine/windows_api.h"
#endif

namespace rml {
namespace internal {

/** Caller is responsible for ensuring this routine is called exactly once. */
extern "C" void MallocInitializeITT() {
#if DO_ITT_NOTIFY
    tbb::internal::__TBB_load_ittnotify();
#endif
}

#if TBB_USE_DEBUG
#define DEBUG_SUFFIX "_debug"
#else
#define DEBUG_SUFFIX
#endif /* TBB_USE_DEBUG */

// MALLOCLIB_NAME is the name of the TBB memory allocator library.
#if _WIN32||_WIN64
#define MALLOCLIB_NAME "tbbmalloc" DEBUG_SUFFIX ".dll"
#elif __APPLE__
#define MALLOCLIB_NAME "libtbbmalloc" DEBUG_SUFFIX ".dylib"
#elif __FreeBSD__ || __NetBSD__ || __sun || _AIX || __ANDROID__
#define MALLOCLIB_NAME "libtbbmalloc" DEBUG_SUFFIX ".so"
#elif __linux__
#define MALLOCLIB_NAME "libtbbmalloc" DEBUG_SUFFIX  __TBB_STRING(.so.TBB_COMPATIBLE_INTERFACE_VERSION)
#else
#error Unknown OS
#endif

void init_tbbmalloc() {
#if DO_ITT_NOTIFY
    MallocInitializeITT();
#endif

/* Preventing TBB allocator library from unloading to prevent
   resource leak, as memory is not released on the library unload.
*/
#if USE_WINTHREAD && !__TBB_SOURCE_DIRECTLY_INCLUDED && !__TBB_WIN8UI_SUPPORT
    // Prevent Windows from displaying message boxes if it fails to load library
    UINT prev_mode = SetErrorMode (SEM_FAILCRITICALERRORS);
    HMODULE lib;
    BOOL ret = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
                                 |GET_MODULE_HANDLE_EX_FLAG_PIN,
                                 (LPCTSTR)&scalable_malloc, &lib);
    MALLOC_ASSERT(lib && ret, "Allocator can't find itself.");
    SetErrorMode (prev_mode);
#endif /* USE_PTHREAD && !__TBB_SOURCE_DIRECTLY_INCLUDED */
}

#if !__TBB_SOURCE_DIRECTLY_INCLUDED
#if USE_WINTHREAD
extern "C" BOOL WINAPI DllMain( HINSTANCE /*hInst*/, DWORD callReason, LPVOID )
{

    if (callReason==DLL_THREAD_DETACH)
    {
        __TBB_mallocThreadShutdownNotification();
    }
    else if (callReason==DLL_PROCESS_DETACH)
    {
        __TBB_mallocProcessShutdownNotification();
    }
    return TRUE;
}
#else /* !USE_WINTHREAD */
struct RegisterProcessShutdownNotification {
// Work around non-reentrancy in dlopen() on Android
#if !__TBB_USE_DLOPEN_REENTRANCY_WORKAROUND
    RegisterProcessShutdownNotification() {
        // prevents unloading, POSIX case
        dlopen(MALLOCLIB_NAME, RTLD_NOW);
    }
#endif /* !__ANDROID__ */
    ~RegisterProcessShutdownNotification() {
        __TBB_mallocProcessShutdownNotification();
    }
};

static RegisterProcessShutdownNotification reg;
#endif /* !USE_WINTHREAD */
#endif /* !__TBB_SOURCE_DIRECTLY_INCLUDED */

} } // namespaces

#if __TBB_ipf
/* It was found that on IA-64 architecture inlining of __TBB_machine_lockbyte leads
   to serious performance regression with ICC. So keep it out-of-line.

   This code is copy-pasted from tbb_misc.cpp.
 */
extern "C" intptr_t __TBB_machine_lockbyte( volatile unsigned char& flag ) {
    tbb::internal::atomic_backoff backoff;
    while( !__TBB_TryLockByte(flag) ) backoff.pause();
    return 0;
}
#endif
