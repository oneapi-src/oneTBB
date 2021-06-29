.. _task_group_extensions:

task_group extensions
=====================

.. note::
    To enable these extensions, set the ``TBB_PREVIEW_TASK_GROUP_EXTENSIONS`` macro to 1.

.. contents::
    :local:
    :depth: 1

Description
***********

|full_name| implementation extends the `tbb::task_group specification <https://spec.oneapi.com/versions/latest/elements/oneTBB/source/task_scheduler/task_group/task_group_cls.html>`_ with the following members:

  - Constructor that takes a custom ``tbb::task_group_context`` object as an argument.
  - Methods to create and run deferred tasks with ``task_handle``. 
  - Requirements for a user-provided function object.
   

API
***

Header
------

.. code:: cpp

    #include <oneapi/tbb/task_group.h>

Synopsis
--------

.. code:: cpp

    namespace oneapi {
        namespace tbb {
   
           class task_group {
           public:
               task_group(task_group_context& context);
               
               template<typename F>
               task_handle defer(F&& f);
                   
               void run(task_handle&& h);

               //only F return type requirements have changed              
               template<typename F>
               void run(F&& f);
           }; 

        } // namespace tbb
    } // namespace oneapi

Member Functions
----------------

.. cpp:function:: task_group(task_group_context& context)

Constructs an empty ``task_group``, which tasks are associated with the ``context``.

.. cpp:function:: template<typename F> task_handle  defer(F&& f)

Creates a deferred task to compute ``f()`` and returns ``task_handle`` pointing to it.

The task is not scheduled for execution until explicitly requested. For example, with the ``task_group::run`` method.
However, the task is still added into the ``task_group``, thus the ``task_group::wait`` method waits until the ``task_handle`` is either scheduled or destroyed.

The ``F`` type must meet the `Function Objects` requirements described in the [function.objects] section of the ISO C++ Standard.

As an optimization hint, ``F`` might return a ``task_handle``, which task object can be executed next.

.. note::
   The ``task_handle`` returned by the function must be created with ``*this`` ``task_group``. It means, with the one for which run method is called, otherwise it is an undefined behavior. 

**Returns:** ``task_handle`` object pointing to task to compute ``f()``.

.. cpp:function:: void run(task_handle&& h)
   
Schedules the task object pointed by the ``h`` for execution.

.. note::
   The failure to satisfy the following conditions leads to undefined behavior:
      * ``h`` is not empty.
      * ``*this`` is the same ``task_group`` that ``h`` is created with.    

.. cpp:function:: template<typename F> void  run(F&& f)

As an optimization hint, ``F`` might return a ``task_handle``, which task object can be executed next.

.. note::
   The ``task_handle`` returned by the function must be created with ``*this`` ``task_group``. It means, with the one for which run method is called, otherwise it is an undefined behavior. 
    
               
.. rubric:: See also

* `oneapi::tbb::task_group specification <https://spec.oneapi.com/versions/latest/elements/oneTBB/source/task_scheduler/task_group/task_group_cls.html>`_
* `oneapi::tbb::task_group_context specification <https://spec.oneapi.com/versions/latest/elements/oneTBB/source/task_scheduler/scheduling_controls/task_group_context_cls.html>`_
* :doc:`oneapi::tbb::task_handle class <task_group_extensions/task_handle>`
