.. _Task_Scheduler_Bypass:

Task Scheduler Bypass
=====================

Scheduler bypass is an optimization where you directly specify the next task to run. 
According to the rules of execution in  :doc:`How Task Scheduling Works <How_Does_Task_Scheduler_Works>`, 
spawning of the new task causes the executing thread to do the following:

 -  Push a new task onto the thread's deque.
 -  Continue to execute the current task until it is completed.
 -  Pop the task from the thread's deque, unless it is stolen by another thread.

Steps 1 and 3 introduce unnecessary deque operations or, even worse, allow stealing that can hurt 
locality without adding significant parallelism. These problems can be avoided by returning next task to execute 
instead of spawning it. When using the method shown in :doc:`How Task Scheduling Works <How_Does_Task_Scheduler_Works>`,
the returned task becomes the next task executed by the thread. Furthermore, this approach almost guarantees that 
the task will be executed by the current thread, and not by any other thread.