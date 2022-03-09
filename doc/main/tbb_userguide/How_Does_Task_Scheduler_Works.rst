.. _How_Does_Task_Scheduler_Works.rst:

How Does Task Scheduler Works
=============================

The scheduler runs tasks in a way that tends to minimize both memory 
demands and cross-thread communication. The intuition is that a balance 
must be reached between depth-first and breadth-first executions. 
Assuming that the tree is finite, depth-first is better for sequential 
execution for the following reasons:

- **Strike when the cache is hot**. The deepest tasks are the most recently created tasks and therefore are the hottest in the cache.
  The deepest tasks are the most recently created tasks and therefore are the hottest in the cache. 
  Also, if they can complete, then tasks depending on it can continue executing, and though not the hottest in cache, 
  it is still warmer than the older tasks above it.
 
- **Minimize space**. Executing the shallowest task leads to breadth-first unfolding of the tree. It creates an exponential
  Executing the shallowest task leads to breadth-first unfolding of the tree. It creates an exponential
  number of nodes that co-exist simultaneously. In contrast, depth-first execution creates the same number 
  of nodes, but only a linear number can exist at the same time, because it creates a stack of other ready 
  tasks.
  
Each thread has its own deque[8] of tasks that are ready to run. When a 
thread spawns a task, it pushes it onto the bottom of its deque.

When a thread participates in the evaluation of tasks, it constantly executes 
a task obtained by the first rule below that applies:

- Get the task returned by the previous one. This rule does not apply 
  if the task does not return anything.

- Take a task from the bottom of its own deque. This rule does not apply 
  if the deque is empty.

- Steal a task from the top of another randomly chosen deque. If the 
  selected deque is empty, the thread tries again to execute this rule until it succeeds.

Rule 1 is described in :doc:`Task Scheduler Bypass <Task_Scheduler_Bypass>`. 
The overall effect of rule 2 is to execute the *youngest* task spawned by the thread, 
which causes the depth-first execution until the thread runs out of work. 
Then rule 3 applies. It steals the *oldest* task spawned by another thread, 
which causes temporary breadth-first execution that converts potential parallelism 
into actual parallelism.
