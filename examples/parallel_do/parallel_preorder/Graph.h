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

#include "Matrix.h"
#include "tbb/atomic.h"
#include <vector>

enum OpKind {
    // Use Cell's value
    OP_VALUE,
    // Unary negation
    OP_NEGATE,
    // Addition
    OP_ADD,
    // Subtraction
    OP_SUB,
    // Multiplication
    OP_MUL
};

static const int ArityOfOp[] = {0,1,2,2,2};

class Cell {
public:
    //! Operation for this cell
    OpKind op;

    //! Inputs to this cell
    Cell* input[2];
   
    //! Type of value stored in a Cell
    typedef Matrix value_type;

    //! Value associated with this Cell
    value_type value;

    //! Set of cells that use this Cell as an input
    std::vector<Cell*> successor;

    //! Reference count of number of inputs that are not yet updated.
    tbb::atomic<int> ref_count;

    //! Update the Cell's value.
    void update();

    //! Default construtor
    Cell() {}
};

//! A directed graph where the vertices are Cells.
class Graph {
    std::vector<Cell> my_vertex_set;
public:
    //! Create a random acyclic directed graph
    void create_random_dag( size_t number_of_nodes );

    //! Print the graph
    void print();

    //! Get set of cells that have no inputs.
    void get_root_set( std::vector<Cell*>& root_set );
};

