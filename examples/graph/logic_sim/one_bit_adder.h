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

#ifndef __TBBexample_graph_logicsim_oba_H
#define __TBBexample_graph_logicsim_oba_H 1

namespace P {
    //input ports 
    const int CI = 0;
    const int A0 = 1;
    const int B0 = 2;
    const int A1 = 3;
    const int B1 = 4;
    const int A2 = 5;
    const int B2 = 6;
    const int A3 = 7;
    const int B3 = 8;
    
    //output_ports
    const int S0 = 0;
    const int S1 = 1;
    const int S2 = 2;
    const int S3 = 3;

#if USE_TWO_BIT_FULL_ADDER
    const int CO = 2;
#else   
    const int CO = 4;
#endif
}

#include "basics.h"

class one_bit_adder : public composite_node< tuple< signal_t, signal_t, signal_t >, tuple< signal_t, signal_t > > {
    broadcast_node<signal_t> A_port;
    broadcast_node<signal_t> B_port;
    broadcast_node<signal_t> CI_port;
    xor_gate<2> FirstXOR;
    xor_gate<2> SecondXOR;
    and_gate<2> FirstAND;
    and_gate<2> SecondAND;
    or_gate<2> FirstOR;
    graph& my_graph;
    typedef composite_node< tuple< signal_t, signal_t, signal_t >, tuple< signal_t, signal_t > > base_type;

public:
    one_bit_adder(graph& g) : base_type(g), my_graph(g), A_port(g), B_port(g), CI_port(g), FirstXOR(g), 
                              SecondXOR(g), FirstAND(g), SecondAND(g), FirstOR(g) {
        make_connections();
        set_up_composite();
    }
    one_bit_adder(const one_bit_adder& src) : 
        base_type(src.my_graph), my_graph(src.my_graph), A_port(src.my_graph), B_port(src.my_graph), 
        CI_port(src.my_graph), FirstXOR(src.my_graph), SecondXOR(src.my_graph), 
        FirstAND(src.my_graph), SecondAND(src.my_graph), FirstOR(src.my_graph) 
    {
        make_connections();
        set_up_composite();
    }

    ~one_bit_adder() {}

private:
    void make_connections() {
  
        make_edge(A_port, input_port<0>(FirstXOR));
        make_edge(A_port, input_port<0>(FirstAND));
        make_edge(B_port, input_port<1>(FirstXOR));
        make_edge(B_port, input_port<1>(FirstAND));
        make_edge(CI_port, input_port<1>(SecondXOR));
        make_edge(CI_port, input_port<1>(SecondAND));
        make_edge(FirstXOR, input_port<0>(SecondXOR));
        make_edge(FirstXOR, input_port<0>(SecondAND));
        make_edge(SecondAND, input_port<0>(FirstOR));
        make_edge(FirstAND, input_port<1>(FirstOR));
    }

    void set_up_composite() {
        base_type::input_ports_type input_tuple(CI_port, A_port, B_port);
        base_type::output_ports_type output_tuple(output_port<0>(SecondXOR), output_port<0>(FirstOR));
        base_type::set_external_ports( input_tuple, output_tuple);
        base_type::add_visible_nodes(A_port, B_port, CI_port, FirstXOR, SecondXOR, FirstAND, SecondAND, FirstOR );
    }
};

#endif /* __TBBexample_graph_logicsim_oba_H */
