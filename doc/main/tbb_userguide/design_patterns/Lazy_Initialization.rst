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
   parallel constructions called within the intializing function.


.. container:: section


   .. rubric:: Example
      :class: sectiontitle

   The example presented here illustrate implementation "lazy initialization" for segment tree
   that stores information about the sum of each vector subinterval. Here is a graphical
   representation of the segment tree for vector of size 4.
   |image0|


.. |image0| image:: Images/image008a.jpg
   :width: 344px
   :height: 191px
