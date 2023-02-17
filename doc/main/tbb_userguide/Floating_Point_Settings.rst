.. _Floating_Point_Settings:

Floating-point Settings
=======================

To propagate CPU-specific settings for floating-point computations to tasks executed by the task scheduler, you can use one of the following two methods:

* When a ``task_arena`` or a task scheduler for a given application thread is initialized, they capture the current floating-point settings of the thread. 
* The ``task_group_context`` class has a method to capture the current floating-point settings. 

By default, worker threads use floating-point settings obtained during the initialization of the implicit arena of the application thread or the explicit ``task_arena``. 
These settings are applied to all parallel computations within the ``task_arena`` or started by the application thread until that thread terminates its task scheduler instance. 
If the thread later re-initializes the task scheduler, new settings are captured.

For better control over floating point behavior, a thread may capture the current settings in a task group context. Do it at context creation with a special flag passed to the constructor:

::
    
    task_group_context ctx( task_group_context::isolated,
                        task_group_context::default_traits | task_group_context::fp_settings );


Or call the ``capture_fp_settings`` method:

::
    
     task_group_context ctx;
    ctx.capture_fp_settings();


You can then pass the task group context to most parallel algorithms, including ``flow::graph``, to ensure that all tasks related to this algorithm use the specified floating-point settings. 
It is possible to execute the parallel algorithms with different floating-point settings captured to separate contexts, even at the same time.

Floating-point settings captured to a task group context prevail over the settings captured during task scheduler initialization. It means, if a context is passed to a parallel algorithm, the floating-point settings captured to the context are used. 
Otherwise, if floating-point settings are not captured to the context, or a context is not explicitly specified, the settings captured during task scheduler initialization are used.

In a nested call to a parallel algorithm that does not use the context of a task group with explicitly captured floating-point settings, the outer-level settings are used. 
If none of the outer-level contexts capture floating-point settings, the settings captured during task scheduler initialization are used.

It guarantees that: 

* Floating-point settings are applied to all tasks executed by the task scheduler, if they are captured: 

  * To a task group context. 
  * During task scheduler initialization. 

* A call to the oneTBB parallel algorithm does not noticeably change the settings of the calling floating-point stream. Even if the algorithm is executed with different settings.

.. note:: 
    The guarantees above apply only to the following conditions:
    
    * A user code inside a task should: 
      
      * Not change the floating-point settings.
      * Revert any modifications. 
      * Restore previous settings before the end of the task.

    * oneTBB task scheduler observers are not used to set or modify floating point settings.

    Otherwise, the stated guarantees are not valid and the behavior related to floating-point settings is undefined.

