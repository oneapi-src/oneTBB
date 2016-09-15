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

#include "tbb/task.h"
#include "harness.h"

//! Helper for verifying that old use cases of spawn syntax still work.
tbb::task* GetTaskPtr( int& counter ) {
    ++counter;
    return NULL;
}

class TaskGenerator: public tbb::task {
    int m_ChildCount;
    int m_Depth;

public:
    TaskGenerator( int child_count, int _depth ) : m_ChildCount(child_count), m_Depth(_depth) {}
    ~TaskGenerator( ) { m_ChildCount = m_Depth = -125; }

    /*override*/ tbb::task* execute() {
        ASSERT( m_ChildCount>=0 && m_Depth>=0, NULL );
        if( m_Depth>0 ) {
            recycle_as_safe_continuation();
            set_ref_count( m_ChildCount+1 );
            int k=0;
            for( int j=0; j<m_ChildCount; ++j ) {
                tbb::task& t = *new( allocate_child() ) TaskGenerator(m_ChildCount/2,m_Depth-1);
                GetTaskPtr(k)->spawn(t);
            }
            ASSERT(k==m_ChildCount,NULL);
            --m_Depth;
            __TBB_Yield();
            ASSERT( state()==recycle && ref_count()>0, NULL);
        }
        return NULL;
    }
};
