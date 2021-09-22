.. _reference:

|short_name| API Reference
==========================

For oneTBB API Reference, refer to `oneAPI Specification <https://spec.oneapi.com/>`_. The current supported
version of oneAPI Specification is 1.0.

Specification extensions
************************

|full_name| implements the `oneTBB specification <https://spec.oneapi.com/versions/latest/elements/oneTBB/source/nested-index.html>`_.
This document provides additional details or restrictions where necessary.
It also describes features that are not included in the oneTBB specification.

.. toctree::
    :titlesonly:

    info_namespace
    parallel_for_each_semantics

Preview features
****************

A preview feature is a component of oneTBB introduced to receive early feedback from
users.

The key properties of a preview feature are:

- It is off by default and must be explicitly enabled.
- It is intended to have a high quality implementation.
- There is no guarantee of future existence or compatibility.
- It may have limited or no support in tools such as correctness analyzers, profilers and debuggers.


.. caution::
    A preview feature is subject to change in future. It might be removed or significantly
    altered in future releases. Changes to a preview feature do NOT require
    usual deprecation and removal process. Therefore, using preview features in production code
    is strongly discouraged.

.. toctree::
    :titlesonly:

    type_specified_message_keys
    blocking_terminate
    scalable_memory_pools
    helpers_for_expressing_graphs
    concurrent_lru_cache_cls
    constraints_extensions
    info_namespace_extensions
    task_group_extensions
    task_arena_extensions 
    this_task_arena_extensions
    mutex_cls
    rw_mutex_cls
    heterogeneous_extensions_chmap
    custom_mutex_chmap
