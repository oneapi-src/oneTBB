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

#include "rml_omp.h"
#include "omp_dynamic_link.h"
#include <assert.h>

namespace __kmp {
namespace rml {

#define MAKE_SERVER(x) DLD(__KMP_make_rml_server,x)
#define GET_INFO(x) DLD(__KMP_call_with_my_server_info,x)
#define SERVER omp_server 
#define CLIENT omp_client
#define FACTORY omp_factory

#if __TBB_WEAK_SYMBOLS_PRESENT
    #pragma weak __KMP_make_rml_server
    #pragma weak __KMP_call_with_my_server_info
    extern "C" {
        omp_factory::status_type __KMP_make_rml_server( omp_factory& f, omp_server*& server, omp_client& client );
        void __KMP_call_with_my_server_info( ::rml::server_info_callback_t cb, void* arg );
    }
#endif /* __TBB_WEAK_SYMBOLS_PRESENT */

#include "rml_factory.h"

} // rml
} // __kmp
