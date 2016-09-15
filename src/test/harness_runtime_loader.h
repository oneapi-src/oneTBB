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

#ifndef harness_runtime_loader_H
#define harness_runtime_loader_H

#if HARNESS_USE_RUNTIME_LOADER
    #if TEST_USES_TBB
        #define TBB_PREVIEW_RUNTIME_LOADER 1
        #include "tbb/runtime_loader.h"
        static char const * _path[] = { ".", NULL };
        // declaration must be placed before 1st TBB call
        static tbb::runtime_loader _runtime_loader( _path );
    #else // TEST_USES_TBB
        // if TBB library is not used, no need to test Runtime Loader
        #define HARNESS_SKIP_TEST 1
    #endif // TEST_USES_TBB
#endif // HARNESS_USE_RUNTIME_LOADER

#endif /* harness_runtime_loader_H */
