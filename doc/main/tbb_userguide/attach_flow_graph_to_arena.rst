.. _attach_flow_graph_to_arena:

Attach Flow Graph to arbitrary Task Arena
======================


During construction a ``graph`` object captures a reference to the arena of the thread that
constructed the object. Whenever a task is spawned to execute work in the graph, the tasks
are spawned in this arena, not in the arena of the that caused the task to be spawned.
All tasks are spawned into a single arena, the arena of the thread that constructed the graph
object.


::


       task_arena a{2};
       a.execute( [&]() {
           graph g;
           function_node< string > f( g, unlimited, []( const string& str ) {
               int current_concurrency = tbb::this_task_arena::max_concurrency();
               cout << str << " : " << current_concurrency << endl;
           } );
           f.try_put("arena");
           g.wait_for_all();
       } );


Used by a ``graph`` the ``task_arena`` can be changed by calling the ``graph::reset()`` function.
This reinitializes the ``graph``, including recapturing the task arena.


::


       graph g;
       task_arena arena{2};
       function_node< string > f( g, unlimited, []( const string& str ) {
           int current_concurrency = tbb::this_task_arena::max_concurrency();
           cout << str << " : " << current_concurrency << endl;
       } );
       arena.execute( [&]() {
           g.reset();
           f.try_put("arena");
           g.wait_for_all();
       } );


Since the main thread constructs the ``graph`` object, the ``graph`` will use the default arena,
which we initialize with eight slots. After calling ``reset`` function to reinitialize
the graph and nodes executes in arena ``arena``.


So performance tuning optimizations which is described in sections below can be applied for
flow graphs as well.

.. toctree::
   :maxdepth: 4

   ../tbb_userguide/work_isolation
   ../tbb_userguide/Guiding_Task_Scheduler_Execution

