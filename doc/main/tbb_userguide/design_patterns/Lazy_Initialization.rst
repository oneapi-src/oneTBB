.. _Lazy_Initialization:

Lazy Initialization
==================


.. container:: section


   .. rubric:: Problem
      :class: sectiontitle

   Delay the creation of an object (potentially expensive) until it is accessed.
   In parallel programming initialization also must be guarded against race conditions.


.. container:: section


   .. rubric:: Context
      :class: sectiontitle

   The cost of operations that take place during the initialization
   of the object may be considerably high. In that case, the object
   should be initialized only when it is needed. Lazy initialization
   is the common tactic that allows implementing such approach.


.. container:: section


   .. rubric:: Solution
      :class: sectiontitle

   Using ``oneapi::tbb::collaborative_call_once`` with ``oneapi::tbb::collaborative_once_flag``
   helps to implement thread-safe lazy initialization for a user object.


   In addition, ``collaborative_call_once`` allows other thread blocked on
   the same ``collaborative_once_flag`` to join other |short_name|
   parallel constructions called within the initializing function.


.. container:: section


   .. rubric:: Example
      :class: sectiontitle

   The example presented here illustrate implementation of "lazy initialization" for Fibonacci numbers calculation.
   Here is a graphical representation of the Fibonacci recursion tree for N=4.


   |image0|


   As seen in the diagram, some elements are recalculated more than once. These operations are redundant,
   so the "lazy initialized" Fibonacci numbers are relevant here.
   

   The code for the implementation is shown below. Already calculated values are stored in a buffer paired with
   ``collaborative_once_flag`` and won't be recalculated when ``collaborative_call_once`` is invoked
   when initialization has already been done.


   ::


      using FibBuffer = std::vector<std::pair<oneapi::tbb::collaborative_once_flag, std::uint64_t>>;

      std::uint64_t LazyFibHelper(int n, FibBuffer& buffer) {
         // Base case
         if (n <= 1) {
            return n;
         }
         // Calculate nth value only once and store it in the buffer.
         // Other threads won't be blocked on already taken collaborative_once_flag
         // but join parallelism inside functor
         oneapi::tbb::collaborative_call_once(buffer[n].first, [&]() {
            std::uint64_t a, b;
            oneapi::tbb::parallel_invoke([&] { a = LazyFibHelper(n - 2, buffer); },
                                         [&] { b = LazyFibHelper(n - 1, buffer); });
            buffer[n].second = a + b;
         });

         return buffer[n].second;
      }

      std::uint64_t Fib(int n) {
         FibBuffer buffer(n+1);
         return LazyFibHelper(n, buffer);
      }


.. |image0| image:: Images/image008a.jpg
   :width: 740px
   :height: 344px
