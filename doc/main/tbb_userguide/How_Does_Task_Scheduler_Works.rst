.. _How_Does_Task_Scheduler_Works.rst:

How Does Task Scheduler Works
=============================


While the task scheduler is not bound to any particular type of the  parallelism, 
it was designed to works efficiently for fork-join parallelism with lots of forks 
(this type of parallelism is typical to parallel algorithms like parallel_for).

Lets consider mapping of fork-join parallelsm on the task scheduler in more details. 

The scheduler runs tasks in a way that tries to achievel several targets simulteniously : 
 - utilize as more threads as possible, to acieve actual parallelism
 - preserve data locality, to make single thread execution more efficient  
 - minimize both memory demands and cross-thread communication, to reduce overhead 

To achieve this a balance between depth-first and breadth-first execution strategies 
must be reached. Assuming that the task graph is finite, depth-first is better for 
sequential execution for the following reasons:

- **Strike when the cache is hot**. The deepest tasks are the most recently created tasks and therefore are the hottest in the cache.
  Also, if they can complete, then tasks depending on it can continue executing, and though not the hottest in cache, 
  they are still warmer than the older tasks above.
 
- **Minimize space**. Executing the shallowest task leads to breadth-first unfolding of the graph. It creates an exponential
  number of nodes that co-exist simultaneously. In contrast, depth-first execution creates the same number 
  of nodes, but only a linear number can exist at the same time, because it creates a stack of other ready 
  tasks.
  
Each thread has its own deque[8] of tasks that are ready to run. When a 
thread spawns a task, it pushes it onto the bottom of its deque.

When a thread participates in the evaluation of tasks, it constantly executes 
a task obtained by the first rule that applies from the roughly equivalent ruleset below:

- Get the task returned by the previous one, if any.

- Take a task from the bottom of its own deque, if any.

- Steal a task from the top of another randomly chosen deque. If the 
  selected deque is empty, the thread tries again to execute this rule until it succeeds.

Rule 1 is described in :doc:`Task Scheduler Bypass <Task_Scheduler_Bypass>`. 
The overall effect of rule 2 is to execute the *youngest* task spawned by the thread, 
which causes the depth-first execution until the thread runs out of work. 
Then rule 3 applies. It steals the *oldest* task spawned by another thread, 
which causes temporary breadth-first execution that converts potential parallelism 
into actual parallelism.
