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

#include "tbb/parallel_do.h"
#include <vector>
#include <algorithm>
#include "Graph.h"


class Body {
public:
    Body() {};

    //------------------------------------------------------------------------
    // Following signatures are required by parallel_do
    //------------------------------------------------------------------------
    typedef Cell* argument_type;

    void operator()( Cell* c, tbb::parallel_do_feeder<Cell*>& feeder ) const {
        c->update();
        // Restore ref_count in preparation for subsequent traversal.
        c->ref_count = ArityOfOp[c->op];
        for( size_t k=0; k<c->successor.size(); ++k ) {
            Cell* successor = c->successor[k];
            // ref_count is used for inter-task synchronization.
            // Correctness checking tools might not take this into account, and report
            // data races between different tasks, that are actually synchronized.
            if( 0 == --(successor->ref_count) ) {
                feeder.add( successor );
            }
        }
    }
};

void ParallelPreorderTraversal( const std::vector<Cell*>& root_set ) {
    tbb::parallel_do(root_set.begin(), root_set.end(),Body());
}


