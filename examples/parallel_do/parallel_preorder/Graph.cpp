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

#include <cstdlib>
#include "Graph.h"
#include <iostream>

using namespace std;

void Graph::create_random_dag( size_t number_of_nodes ) {
    my_vertex_set.resize(number_of_nodes);
    for( size_t k=0; k<number_of_nodes; ++k ) {
        Cell& c = my_vertex_set[k];
        int op = int((rand()>>8)%5u);
        if( op>int(k) ) op = int(k);
        switch( op ) {
            default:
                c.op = OP_VALUE;
                c.value = Cell::value_type((float)k);
                break;
            case 1:
                c.op = OP_NEGATE;
                break;
            case 2:
                c.op = OP_SUB;
                break;
            case 3: 
                c.op = OP_ADD;
                break;
            case 4: 
                c.op = OP_MUL;
                break;
        }
        for( int j=0; j<ArityOfOp[c.op]; ++j ) {
            Cell& input = my_vertex_set[rand()%k];
            c.input[j] = &input;
        }
    }
}

void Graph::print() {
    for( size_t k=0; k<my_vertex_set.size(); ++k ) {
        std::cout<<"Cell "<<k<<":";
        for( size_t j=0; j<my_vertex_set[k].successor.size(); ++j )
            std::cout<<" "<<int(my_vertex_set[k].successor[j] - &my_vertex_set[0]);
        std::cout<<std::endl;
    }
}

void Graph::get_root_set( vector<Cell*>& root_set ) {
    for( size_t k=0; k<my_vertex_set.size(); ++k ) {
        my_vertex_set[k].successor.clear();
    }
    root_set.clear();
    for( size_t k=0; k<my_vertex_set.size(); ++k ) {
        Cell& c = my_vertex_set[k];
        c.ref_count = ArityOfOp[c.op];
        for( int j=0; j<ArityOfOp[c.op]; ++j ) {
            c.input[j]->successor.push_back(&c);
        }
        if( ArityOfOp[c.op]==0 )
            root_set.push_back(&my_vertex_set[k]);
    }
}

void Cell::update() {
    switch( op ) {
        case OP_VALUE:
            break;
        case OP_NEGATE:
            value = -(input[0]->value);
            break;
        case OP_ADD:
            value = input[0]->value + input[1]->value;
            break;
        case OP_SUB:
            value = input[0]->value - input[1]->value;
            break;
        case OP_MUL:
            value = input[0]->value * input[1]->value;
            break;
    }
}

