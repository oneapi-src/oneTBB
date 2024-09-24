/*
    Copyright (c) 2024 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

// this pseudo-code was used in the book "Today's TBB" (2015)
// it serves no other purpose other than to be here to verify compilation,
// and provide consist code coloring for the book


while (auto img = getImage()) {
  auto x = f1(img);
  auto y = f2(x);
  auto z = f3(x);
  f4(y,z);
}


namespace oneapi {
namespace tbb {
namespace flow {

  class graph : no_copy {
  public:

    //! Constructs a graph with isolated task_group_context
    graph();
    
    //! Constructs a graph with use_this_context as context
    explicit graph(task_group_context& use_this_context);
    
    //! Destroys the graph.
    ~graph ();
    
    //! Used to register that an external entity may still 
    //! interact with the graph.
    void reserve_wait() override;
    
    //! Deregisters an external entity that may have 
    //! interacted with the graph.
    void release_wait() override;
    
    //! Wait until graph is idle and the number of 
    //! release_wait calls equals
    //! to the number of reserve_wait calls.
    void wait_for_all();
    
    // thread-unsafe state reset.
    void reset(reset_flags f = rf_reset_protocol);
    
    //! cancels execution of the associated task_group_context
    void cancel();
    
    //! return status of graph execution
    bool is_cancelled() { return cancelled; }
    bool exception_thrown () { return caught_exception; }
    
  }; // class graph
} } }


namespace oneapi {
namespace tbb {
namespace flow {
  
  template < typename Input,
	     typename Output = continue_msg,
	     typename Policy = /*implementation-defined*/ >
  class function_node : public graph_node,
			public receiver<Input>,
			public sender<Output> {
  public:
    //! Constructor
    template<typename Body>
    function_node( graph &g, size_t concurrency, Body body,
		   Policy /*unspecified*/ = Policy(),
		   node_priority_t priority = no_priority );
    template<typename Body>
    function_node( graph &g, size_t concurrency, Body body,
		   node_priority_t priority = no_priority );
    ~fuction_node();
    
    //! Copy constructor
    function_node( const function_node &src );
    
    //! Explicitly pass a mesage to the node
    bool try_put( const Input &v );
    
    bool try_get( Output &v );
  };

} } }


namespace oneapi {
namespace tbb {
namespace flow {
  using tag_value = /*implementation-specific*/;
  
  // JoinNodePolicies:
  struct reserving;
  struct queueing;
  template<typename K,
	   class KHash=tbb_hash_compare<K> > struct key_matching;
  using tag_matching = key_matching<tag_value>;
  
  template<typename OutputTuple,
	   class JoinPolicy = /*implementation-defined*/>
  class join_node {
  public:
    using input_ports_type = /*implementation-defined*/;
    
    explicit join_node( graph &g );
    join_node( const join_node &src );
    
    input_ports_type &input_ports( );
    
    bool try_get( OutputTuple &v );
    
    template<typename Output Tuple, typename K,
	     class KHash=tbb_hash_compare<K> >
    class join_node< OutputTuple, key_matching<K, KHash> > {
    public:
      using input_ports_type = /*implementation-defined*/;
      
      explicit join_node( graph &g );
      join_node( const join_node &src );
      
      template<typename ... TagBodies>
      join_node( graph &g, TagBodies ... );
      
      input_ports_type &input_ports( );
      
      bool try_get( OutputTuple &v );
    };
} } }



tbb::flow::join_node<std::tuple<int, std::string, double>, 
                     tbb::flow::queueing> j(g);


make_edge(predecessor_node, successor_node);



make_edge(output_port<0>(predecessor_node),
input_port<1>(successor_node));


  
make_edge(my_node, tbb: :flow: : input_port<0>(my_join_node));
make_edge(my_other_node, tbb: : flow: : input_port<1>(my_join_node));


  
make_edge(my_join_node, my_final_node);



my_first_node.try_put(10);



