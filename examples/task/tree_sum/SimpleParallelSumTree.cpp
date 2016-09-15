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

class SimpleSumTask: public tbb::task {
    Value* const sum;
    TreeNode* root;
public:
    SimpleSumTask( TreeNode* root_, Value* sum_ ) : root(root_), sum(sum_) {}
    task* execute() {
        if( root->node_count<1000 ) {
            *sum = SerialSumTree(root);
        } else {
            Value x, y;
            int count = 1; 
            tbb::task_list list;
            if( root->left ) {
                ++count;
                list.push_back( *new( allocate_child() ) SimpleSumTask(root->left,&x) );
            }
            if( root->right ) {
                ++count;
                list.push_back( *new( allocate_child() ) SimpleSumTask(root->right,&y) );
            }
            // Argument to set_ref_count is one more than size of the list,
            // because spawn_and_wait_for_all expects an augmented ref_count.
            set_ref_count(count);
            spawn_and_wait_for_all(list);
            *sum = root->value;
            if( root->left ) *sum += x;
            if( root->right ) *sum += y;
        }
        return NULL;
    }
};

Value SimpleParallelSumTree( TreeNode* root ) {
    Value sum;
    SimpleSumTask& a = *new(tbb::task::allocate_root()) SimpleSumTask(root,&sum);
    tbb::task::spawn_root_and_wait(a);
    return sum;
}

