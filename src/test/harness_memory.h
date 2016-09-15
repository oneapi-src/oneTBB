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

// Declarations for simple estimate of the memory being used by a program.
// Not yet implemented for OS X*.
// This header is an optional part of the test harness.
// It assumes that "harness_assert.h" has already been included.

#if __linux__ || __sun
#include <sys/resource.h>
#include <unistd.h>

#elif __APPLE__ && !__ARM_ARCH
#include <unistd.h>
#include <mach/mach.h>
#include <AvailabilityMacros.h>
#if MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_10_6 || __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_8_0
#include <mach/shared_region.h>
#else
#include <mach/shared_memory_server.h>
#endif
#if SHARED_TEXT_REGION_SIZE || SHARED_DATA_REGION_SIZE
const size_t shared_size = SHARED_TEXT_REGION_SIZE+SHARED_DATA_REGION_SIZE;
#else
const size_t shared_size = 0;
#endif

#elif _WIN32 && !_XBOX && !__TBB_WIN8UI_SUPPORT
#include <windows.h>
#include <psapi.h>
#if _MSC_VER
#pragma comment(lib, "psapi")
#endif

#endif /* OS selection */

//! Return estimate of number of bytes of memory that this program is currently using.
/* Returns 0 if not implemented on platform. */
size_t GetMemoryUsage() {
#if _XBOX || __TBB_WIN8UI_SUPPORT
    return 0;
#elif _WIN32
    PROCESS_MEMORY_COUNTERS mem;
    bool status = GetProcessMemoryInfo(GetCurrentProcess(), &mem, sizeof(mem))!=0;
    ASSERT(status, NULL);
    return mem.PagefileUsage;
#elif __linux__
    FILE* statsfile = fopen("/proc/self/statm","r");
    size_t pagesize = getpagesize();
    ASSERT(statsfile, NULL);
    long total_mem;
    int n = fscanf(statsfile,"%lu",&total_mem);
    if( n!=1 ) {
        REPORT("Warning: memory usage statistics wasn't obtained\n");
        return 0;
    }
    fclose(statsfile);
    return total_mem*pagesize;
#elif __APPLE__ && !__ARM_ARCH
    kern_return_t status;
    task_basic_info info;
    mach_msg_type_number_t msg_type = TASK_BASIC_INFO_COUNT;
    status = task_info(mach_task_self(), TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &msg_type);
    ASSERT(status==KERN_SUCCESS, NULL);
    return info.virtual_size - shared_size;
#else
    return 0;
#endif
}

//! Use approximately a specified amount of stack space.
/** Recursion is used here instead of alloca because some implementations of alloca do not use the stack. */
void UseStackSpace( size_t amount, char* top=0 ) {
    char x[1000];
    memset( x, -1, sizeof(x) );
    if( !top )
        top = x;
    ASSERT( x<=top, "test assumes that stacks grow downwards" );
    if( size_t(top-x)<amount )
        UseStackSpace( amount, top );
}
