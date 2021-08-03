.. _Task_Scheduler_Bypass:

Task Scheduler Bypass
=====================

Scheduler bypass is an optimization where you directly specify the next task to run. 
By the execution rules in  :doc:`How Task Scheduling Works <How_Does_Task_Scheduler_Works>`, 
spawning of the new task causes the executing thread to do the following:

 -  Push new task onto the thread's deque.
 -  Continue executing current task till completion.
 -  Pop the task from the thread's deque, unless it is stolen by another thread.

Steps 1 and 3 introduce unnecessary deque operations, or worse yet, permit stealing that can hurt 
locality without adding significant parallelism. These problems can be avoided by returning next task to execute 
instead of spawning it. When using the method shown in :doc:`How Task Scheduling Works <How_Does_Task_Scheduler_Works>`,
returned task becomes the next task executed by the thread. Furthermore, this approach (almost) guarantees that 
the task will be executed by the current thread, not some other thread.