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

// Header that includes TSX-specific test functions

#if __TBB_TSX_AVAILABLE 
#define __TBB_TSX_TESTING_ENABLED_FOR_THIS_COMPILER (__INTEL_COMPILER || __GNUC__ || _MSC_VER || __SUNPRO_CC)
#if __TBB_TSX_TESTING_ENABLED_FOR_THIS_COMPILER

#include "harness_defs.h"

inline static bool IsInsideTx()
{
    return __TBB_machine_is_in_transaction() != 0;
}

#if _MSC_VER
#include <intrin.h> // for __cpuid
#endif
// TODO: consider reusing tbb_misc.cpp:cpu_has_speculation() instead of code duplication.
bool have_TSX() {
    bool result = false;
    const int hle_ebx_mask = 1<<4;
    const int rtm_ebx_mask = 1<<11;
#if _MSC_VER
    int info[4] = {0,0,0,0};
    const int reg_ebx = 1;
    int old_ecx = 0;
    __cpuidex(info, 7, old_ecx);
    result = (info[reg_ebx] & hle_ebx_mask)!=0;
    if( result ) ASSERT( (info[reg_ebx] & rtm_ebx_mask)!=0, NULL );
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
    result = (reg_ebx & hle_ebx_mask)!=0 ;
    if( result ) ASSERT( (reg_ebx & rtm_ebx_mask)!=0, NULL );
#endif
    return result;
}

#endif /* __TBB_TSX_TESTING_ENABLED_FOR_THIS_COMPILER */
#endif /* __TBB_TSX_AVAILABLE */
