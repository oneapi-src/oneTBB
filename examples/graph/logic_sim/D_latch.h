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

#ifndef __TBBexample_graph_logicsim_dlatch_H
#define __TBBexample_graph_logicsim_dlatch_H 1

#include "basics.h"

class D_latch : public composite_node< tuple< signal_t, signal_t >, tuple< signal_t, signal_t > > {
    broadcast_node<signal_t> D_port;
    broadcast_node<signal_t> E_port;
    not_gate a_not;
    and_gate<2> first_and;
    and_gate<2> second_and;
    nor_gate<2> first_nor;
    nor_gate<2> second_nor;
    graph& my_graph;
    typedef composite_node< tuple< signal_t, signal_t >, tuple< signal_t, signal_t > > base_type;

 public:
    D_latch(graph& g) : base_type(g), my_graph(g), D_port(g), E_port(g), a_not(g), first_and(g), second_and(g), 
                        first_nor(g), second_nor(g) 
    {
        make_edge(D_port, input_port<0>(a_not));
        make_edge(D_port, input_port<1>(second_and));
        make_edge(E_port, input_port<1>(first_and));
        make_edge(E_port, input_port<0>(second_and));
        make_edge(a_not, input_port<0>(first_and));
        make_edge(first_and, input_port<0>(first_nor));
        make_edge(second_and, input_port<1>(second_nor));
        make_edge(first_nor, input_port<0>(second_nor));
        make_edge(second_nor, input_port<1>(first_nor));
 
        base_type::input_ports_type input_tuple(D_port, E_port);
        base_type::output_ports_type output_tuple(output_port<0>(first_nor), output_port<0>(second_nor)); 

        base_type::set_external_ports(input_tuple, output_tuple); 
        base_type::add_visible_nodes(D_port, E_port, a_not, first_and, second_and, first_nor, second_nor);
    }
    ~D_latch() {}
};

#endif /* __TBBexample_graph_logicsim_dlatch_H */
