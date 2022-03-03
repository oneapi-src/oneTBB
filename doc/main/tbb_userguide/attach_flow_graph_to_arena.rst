.. _attach_flow_graph_to_arena:

Attach Flow Graph to an Arbitrary Task Arena
======================


|short_name| ``task_arena`` interface provides mechanisms to guide tasks execution within
the arena by setting the preferred computation units or restricting part of computation units.
In some case, you may want to use mechanisms within a flow graph.

During its construction a ``graph`` object attaches to the arena, in which the constructing
thread occupies a slot. Whenever a task is spawned on behalf of the graph, it is spawned
in the arena of the graph it is attached to, disregarding the arena of the thread
which is caused a task to be spawned.

The example shows how to set the most performant core type as preferable for graph execution:


::


       std::vector<tbb::core_type_id> core_types = tbb::info::core_types();
       tbb::task_arena arena(
           tbb::task_arena::constraints{}.set_core_type(core_types.back())
       );
       arena.execute( [&]() {
           graph g;
           function_node< int > f( g, unlimited, []( int i ) {
               /*the most performant core type is defined as preferred.*/
           } );
           f.try_put(1);
           g.wait_for_all();
       } );


A ``graph`` object can be reattached to a different ``task_arena`` by calling
the ``graph::reset()`` function. This reinitializes the ``graph`` and reattaches
it to the task arena instance, inside which the ``graph::reset()`` method is executed.

The example shows how reattach existing graph to an arena with the most performant core type
as preferable for work execution:


::


       graph g;
       function_node< int > f( g, unlimited, []( int i ) {
           /*the most performant core type is defined as preferred.*/
       } );
       std::vector<tbb::core_type_id> core_types = tbb::info::core_types();
       tbb::task_arena arena(
           tbb::task_arena::constraints{}.set_core_type(core_types.back())
       );
       arena.execute( [&]() {
           g.reset();
       } );
       f.try_put(1);
       g.wait_for_all();

See the following topics to learn more.

.. toctree::
   :maxdepth: 4

   ../tbb_userguide/Guiding_Task_Scheduler_Execution
   ../tbb_userguide/work_isolation

