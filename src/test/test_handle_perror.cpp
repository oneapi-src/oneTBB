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

// Program for basic correctness of handle_perror, which is internal
// to the TBB shared library.

#include <cerrno>

#if !TBB_USE_EXCEPTIONS && _MSC_VER
    // Suppress "C++ exception handler used, but unwind semantics are not enabled" warning in STL headers
    #pragma warning (push)
    #pragma warning (disable: 4530)
#endif

#include <stdexcept>

#if !TBB_USE_EXCEPTIONS && _MSC_VER
    #pragma warning (pop)
#endif

#include "../tbb/tbb_misc.h"
#include "harness.h"

#if TBB_USE_EXCEPTIONS

static void TestHandlePerror() {
    bool caught = false;
    try {
        tbb::internal::handle_perror( EAGAIN, "apple" );
    } catch( std::runtime_error& e ) {
#if TBB_USE_EXCEPTIONS
        REMARK("caught runtime_exception('%s')\n",e.what());
        ASSERT( memcmp(e.what(),"apple: ",7)==0, NULL );
        ASSERT( strlen(strstr(e.what(), strerror(EAGAIN))), "bad error message?" ); 
#endif /* TBB_USE_EXCEPTIONS */
        caught = true;
    }
    ASSERT( caught, NULL );
}

int TestMain () {
    TestHandlePerror();
    return Harness::Done;
}

#else /* !TBB_USE_EXCEPTIONS */

int TestMain () {
    return Harness::Skipped;
}

#endif /* TBB_USE_EXCEPTIONS */
