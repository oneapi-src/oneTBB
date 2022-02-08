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


   .. rubric:: Forces
      :class: sectiontitle

    Sample text


.. container:: section


   .. rubric:: Solution
      :class: sectiontitle


.. container:: section


   .. rubric:: Example
      :class: sectiontitle

   Sample text
