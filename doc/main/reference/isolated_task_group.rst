.. _isolated_task_group:

isolated_task_group
===================

A template class of ``task_group`` with implicit isolation.

.. note::
    To enable this feature, define the ``TBB_PREVIEW_ISOLATED_TASK_GROUP`` macro to 1.


Header
------

.. code:: cpp

    #include <oneapi/tbb/task_group.h>


Description
-----------

An ``isolated_task_group`` is a ``task_group`` in which spawned tasks have the same isolation level. 
Therefore, threads calling ``isolated_task_group`` members are restricted to process tasks scheduled only within this group. 
This functionality can be useful when implementing lazy initialization patterns.

.. code:: cpp

   namespace oneapi{
       namespace tbb {
           class isolated_task_group {
           public:
               isolated_task_group();
               ~isolated_task_group();
               // Same task_group members
            };
        }
   }


Example
-------
The following example demonstrates the lazy initialization approach. 
That is, only one thread calls the initialize function.
Other threads join parallel constructions created by this function.

.. code:: cpp

   #define TBB_PREVIEW_ISOLATED_TASK_GROUP 1

   #include "oneapi/tbb/task_group.h"
   #include "oneapi/tbb/parallel_for.h"

   #include <atomic>
   #include <iostream>
   #include <cassert>

   class parallel_once {
       enum class init_state : int { none, in_process, done };

       oneapi::tbb::isolated_task_group    my_itg;
       std::atomic<init_state>     my_state{ init_state::none };
   public:
       template<class Callable>
       void call(Callable f) {
           // Check if the initialize function is already completed
           init_state s = my_state.load(std::memory_order_acquire);
           switch (s) {
           case init_state::none:
               // The initialization is not done
               // Decide if the current thread has to call the initialize function.
               if (my_state.compare_exchange_strong(s, init_state::in_process)) {
                   // The current thread has to start the initialize function
                   my_itg.run_and_wait([this, f] {
                       // The function is called only by one thread
                       // Other threads can join a nested parallel loop
                       f();
                       my_state.store(init_state::done, std::memory_order_release);
                   });
                   break;
               }
               // [[fallthrough]]
           case init_state::in_process:
               // Another thread has started the initialize function, join the initialization work
               // The while loop is required because run_and_wait might be not called yet
               do {
                   my_itg.wait();
               } while (my_state != init_state::done);
               break;
           case init_state::done:
               // The initialization is done.
               break;
           }
           assert(my_state == init_state::done);
       } 
   };

   void initialize() {
       const int K = 10;
       oneapi::tbb::parallel_for(0, K, [](int i) {
           std::cout << i << std::endl;
       });
   }

   int main() {
       const int N = 1000;

       parallel_once po;
       oneapi::tbb::parallel_for(0, N, [&po](int) {
           // The initialize function is called once while other threads can join nested parallel loop.
           po.call(initialize);
       });

       return 0;
   }


