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

// This file is compiled with C++, but linked with a program written in C.
// The intent is to find dependencies on the C++ run-time.

#include <stdlib.h>
#include "harness_defs.h"
#define RML_PURE_VIRTUAL_HANDLER abort

#if _MSC_VER==1500 && !defined(__INTEL_COMPILER)
// VS2008/VC9 seems to have an issue; 
#pragma warning( push )
#pragma warning( disable: 4100 ) 
#elif __TBB_MSVC_UNREACHABLE_CODE_IGNORED
// VS2012-2013 issues "warning C4702: unreachable code" for the code which really
// shouldn't be reached according to the test logic: rml::client has the
// implementation for the "pure" virtual methods to be aborted if they are
// called.
#pragma warning( push )
#pragma warning( disable: 4702 )
#endif
#include "rml_omp.h"
#if ( _MSC_VER==1500 && !defined(__INTEL_COMPILER)) || __TBB_MSVC_UNREACHABLE_CODE_IGNORED
#pragma warning( pop )
#endif

rml::versioned_object::version_type Version;

class MyClient: public __kmp::rml::omp_client {
public:
    /*override*/rml::versioned_object::version_type version() const {return 0;}
    /*override*/size_type max_job_count() const {return 1024;}
    /*override*/size_t min_stack_size() const {return 1<<20;}
    /*override*/rml::job* create_one_job() {return NULL;}
    /*override*/void acknowledge_close_connection() {}
    /*override*/void cleanup(job&) {}
    /*override*/policy_type policy() const {return throughput;}
    /*override*/void process( job&, void*, __kmp::rml::omp_client::size_type ) {}
   
};

//! Never actually set, because point of test is to find linkage issues.
__kmp::rml::omp_server* MyServerPtr;

#define HARNESS_NO_PARSE_COMMAND_LINE 1
#define HARNESS_CUSTOM_MAIN 1
#include "harness.h"

extern "C" void Cplusplus() {
    MyClient client;
    Version = client.version();
    REPORT("done\n");
}
