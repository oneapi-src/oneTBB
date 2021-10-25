.. _parallel_sort_rvalue_ranges:

parallel_sort rvalue ranges support
=================================================

.. contents::
    :local:
    :depth: 1

Description
***********

|full_name| implementation extends the `tbb::parallel_sort specification <https://spec.oneapi.io/versions/latest/elements/oneTBB/source/algorithms/functions/parallel_sort_func.html>`_
with an overloads that takes the container by a forwarding reference.


API
***

Header
------

.. code:: cpp

    #include <oneapi/tbb/flow_graph.h>

Syntax
------

.. code:: cpp

    namespace oneapi {
        namespace tbb {

            template<typename Container>
            void parallel_sort( Container&& c );
            template<typename Container>
            void parallel_sort( Container&& c, const Compare& comp );

        } // namespace tbb
    } // namespace oneapi

Functions
---------

.. cpp:function:: template<typename Container> void parallel_sort( Container&& c );

    Equivalent to ``parallel_sort( std::begin(c), std::end(c), comp )``, where `comp` uses `operator<` to determine relative orderings.

.. cpp:function:: template<typename Container> void parallel_sort( Container&& c, const Compare& comp );

    Equivalent to ``parallel_sort( std::begin(c), std::end(c), comp )``.
