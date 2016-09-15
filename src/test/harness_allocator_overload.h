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

#ifndef tbb_test_harness_allocator_overload_H
#define tbb_test_harness_allocator_overload_H

#include "../tbbmalloc/proxy.h" // for MALLOC_UNIXLIKE_OVERLOAD_ENABLED, MALLOC_ZONE_OVERLOAD_ENABLED
#include "tbb/tbb_config.h" // for __TBB_WIN8UI_SUPPORT

// Skip configurations with unsupported system malloc overload:
// skip unsupported MSVCs, WIN8UI and MINGW (it doesn't define _MSC_VER),
// no support for MSVC 2015 in debug for now,
// don't use defined(_MSC_VER), because result of using defined() in macro expansion is undefined
#define MALLOC_WINDOWS_OVERLOAD_ENABLED ((_WIN32||_WIN64) && !__TBB_WIN8UI_SUPPORT && _MSC_VER >= 1500 && !(_MSC_VER == 1900 && _DEBUG))

// Skip configurations with unsupported system malloc overload:
// * overload via linking with -lmalloc_proxy is broken in offload,
// as the library is loaded too late in that mode,
// * LD_PRELOAD mechanism is broken in offload
#define HARNESS_SKIP_TEST ((!MALLOC_WINDOWS_OVERLOAD_ENABLED && !MALLOC_UNIXLIKE_OVERLOAD_ENABLED && !MALLOC_ZONE_OVERLOAD_ENABLED) || __TBB_MIC_OFFLOAD)

#endif // tbb_test_harness_allocator_overload_H
