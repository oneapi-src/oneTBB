.. _How_Does_Task_Scheduler_Works.rst:

How Does Task Scheduler Works
=============================

The scheduler runs tasks in a way that tends to minimize both memory 
demands and cross-thread communication. The intuition is that a balance 
must be reached between depth-first and breadth-first execution. 
Assuming that the tree is finite, depth-first is best for sequential 
execution for the following reasons:

- Strike when the cache is hot
  The deepest tasks are the most recently created tasks, and therefore are hottest in cache. 
  Also, if they can complete, then tasks depending on it can continue executing, and though not the hottest in cache, 
  it is still warmer than the older tasks above it.
 
- Minimize space
  Executing the shallowest task leads to breadth-first unfolding of the tree. This creates an exponential
  number of nodes that coexist simultaneously. In contrast, depth-first execution creates the same number 
  of nodes, but only a linear number have to exist at the same time, because it stacks the other ready 
  tasks.
  
Each thread has its own deque[8] of tasks that are ready to run. When a 
thread spawns a task, it pushes it onto the bottom of its deque.

When a thread participates in tasks evaluation, it continually executes 
a task obtained by the first rule below that applies:

- Get the task returned by the previous task. This rule does not apply 
  if the task does not return anything.

- Pop a task from the bottom of its own deque. This rule does not apply 
  if the deque is empty.

- Steal a task from the top of another randomly chosen deque. If the 
  chosen deque is empty, the thread tries this rule again until it succeeds.

Rule 1 is discussed in :doc:`Task Scheduler Bypass <Task_Scheduler_Bypass>`. 
The overall effect of rule 2 is to execute the *youngest* task spawned by the thread, 
which causes depth-first execution until the thread runs out of work. 
Then rule 3 applies. It steals the *oldest* task spawned by another thread, 
which causes temporary breadth-first execution that converts potential parallelism 
into actual parallelism.
