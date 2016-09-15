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

// Header guard and namespace names follow TBB conventions.

#ifndef __TBB_rml_tbb_H
#define __TBB_rml_tbb_H

#include "tbb/tbb_config.h"
#include "rml_base.h"

namespace tbb {
namespace internal {
namespace rml {

class tbb_client;

//------------------------------------------------------------------------
// Classes instantiated by the server
//------------------------------------------------------------------------

//! Represents a set of tbb worker threads provided by the server.
class tbb_server: public ::rml::server {
public:
    //! Inform server of adjustments in the number of workers that the client can profitably use.
    virtual void adjust_job_count_estimate( int delta ) = 0;

#if _WIN32||_WIN64
    //! Inform server of a tbb master thread.
    virtual void register_master( execution_resource_t& v ) = 0;

    //! Inform server that the tbb master thread is done with its work.
    virtual void unregister_master( execution_resource_t v ) = 0;
#endif /* _WIN32||_WIN64 */
};

//------------------------------------------------------------------------
// Classes instantiated by the client
//------------------------------------------------------------------------

class tbb_client: public ::rml::client {
public:
    //! Defined by TBB to steal a task and execute it.  
    /** Called by server when it wants an execution context to do some TBB work.
        The method should return when it is okay for the thread to yield indefinitely. */
    virtual void process( job& ) RML_PURE(void)
};

/** Client must ensure that instance is zero-inited, typically by being a file-scope object. */
class tbb_factory: public ::rml::factory {

    //! Pointer to routine that creates an RML server.
    status_type (*my_make_server_routine)( tbb_factory&, tbb_server*&, tbb_client& );

    //! Pointer to routine that calls callback function with server version info.
    void (*my_call_with_server_info_routine)( ::rml::server_info_callback_t cb, void* arg );

public:
    typedef ::rml::versioned_object::version_type version_type;
    typedef tbb_client client_type;
    typedef tbb_server server_type;

    //! Open factory.
    /** Dynamically links against RML library. 
        Returns st_success, st_incompatible, or st_not_found. */
    status_type open();

    //! Factory method to be called by client to create a server object.
    /** Factory must be open. 
        Returns st_success, or st_incompatible . */
    status_type make_server( server_type*&, client_type& );

    //! Close factory
    void close();

    //! Call the callback with the server build info
    void call_with_server_info( ::rml::server_info_callback_t cb, void* arg ) const;
};

} // namespace rml
} // namespace internal
} // namespace tbb

#endif /*__TBB_rml_tbb_H */
