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

#include "common.h"
#include "tbb/task.h"

class OptimizedSumTask: public tbb::task {
    Value* const sum;
    TreeNode* root;
    bool is_continuation;
    Value x, y;
public:
    OptimizedSumTask( TreeNode* root_, Value* sum_ ) : root(root_), sum(sum_), is_continuation(false) {
    }
    tbb::task* execute() {
        tbb::task* next = NULL;
        if( !is_continuation ) {
            if( root->node_count<1000 ) {
                *sum = SerialSumTree(root);
            } else {
                // Create tasks before spawning any of them.
                tbb::task* a = NULL;
                tbb::task* b = NULL;
                if( root->left )
                    a = new( allocate_child() ) OptimizedSumTask(root->left,&x);
                if( root->right )
                    b = new( allocate_child() ) OptimizedSumTask(root->right,&y);
                recycle_as_continuation();
                is_continuation = true;
                set_ref_count( (a!=NULL)+(b!=NULL) );
                if( a ) {
                    if( b ) spawn(*b);
                } else 
                    a = b;
                next = a;
            }
        } else {
            *sum = root->value;
            if( root->left ) *sum += x;
            if( root->right ) *sum += y;
        } 
        return next;
    }
};

Value OptimizedParallelSumTree( TreeNode* root ) {
    Value sum;
    OptimizedSumTask& a = *new(tbb::task::allocate_root()) OptimizedSumTask(root,&sum);
    tbb::task::spawn_root_and_wait(a);
    return sum;
}

