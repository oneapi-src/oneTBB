/*
    Copyright (c) 2005-2020 Intel Corporation

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

// Source file for miscellaneous entities that are infrequently referenced by
// an executing program.

#include "tbb/detail/_exception.h"
#include "tbb/detail/_machine.h"

#include "tbb/version.h"

#include "misc.h"
#include "assert_impl.h" // Out-of-line TBB assertion handling routines are instantiated here.

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <cstring>
#include <cstdarg>

#if _WIN32||_WIN64
#include <windows.h>
#endif

#if !_WIN32
#include <unistd.h> // sysconf(_SC_PAGESIZE)
#endif

namespace tbb {
namespace detail {
namespace r1 {

size_t DefaultSystemPageSize() {
#if _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
#else
    return sysconf(_SC_PAGESIZE);
#endif
}

/** The leading "\0" is here so that applying "strings" to the binary delivers a clean result. */
static const char VersionString[] = "\0" TBB_VERSION_STRINGS;

static bool PrintVersionFlag = false;

void PrintVersion() {
    PrintVersionFlag = true;
    std::fputs(VersionString+1,stderr);
}

void PrintExtraVersionInfo( const char* category, const char* format, ... ) {
    if( PrintVersionFlag ) {
        char str[1024]; std::memset(str, 0, 1024);
        va_list args; va_start(args, format);
        // Note: correct vsnprintf definition obtained from tbb_assert_impl.h
        std::vsnprintf( str, 1024-1, format, args);
        va_end(args);
        std::fprintf(stderr, "TBB: %s\t%s\n", category, str );
    }
}

void PrintRMLVersionInfo( void* arg, const char* server_info ) {
    PrintExtraVersionInfo( server_info, (const char *)arg );
}

//! check for transaction support.
#if _MSC_VER
#include <intrin.h> // for __cpuid
#endif
bool cpu_has_speculation() {
#if (__TBB_x86_32 || __TBB_x86_64)
#if (__INTEL_COMPILER || __GNUC__ || _MSC_VER || __SUNPRO_CC)
    bool result = false;
    const int rtm_ebx_mask = 1<<11;
#if _MSC_VER
    int info[4] = {0,0,0,0};
    const int reg_ebx = 1;
    __cpuidex(info, 7, 0);
    result = (info[reg_ebx] & rtm_ebx_mask)!=0;
#elif __GNUC__ || __SUNPRO_CC
    int32_t reg_ebx = 0;
    int32_t reg_eax = 7;
    int32_t reg_ecx = 0;
    __asm__ __volatile__ ( "movl %%ebx, %%esi\n"
                           "cpuid\n"
                           "movl %%ebx, %0\n"
                           "movl %%esi, %%ebx\n"
                           : "=a"(reg_ebx) : "0" (reg_eax), "c" (reg_ecx) : "esi",
#if __TBB_x86_64
                           "ebx",
#endif
                           "edx"
                           );
    result = (reg_ebx & rtm_ebx_mask)!=0 ;
#endif
    return result;
#else
    #error Speculation detection not enabled for compiler
#endif /* __INTEL_COMPILER || __GNUC__ || _MSC_VER */
#else  /* (__TBB_x86_32 || __TBB_x86_64) */
    return false;
#endif /* (__TBB_x86_32 || __TBB_x86_64) */
}

} // namespace r1
} // namespace detail
} // namespace tbb

