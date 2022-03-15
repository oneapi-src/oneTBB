.. _Intro_gsg:


|full-name| is a runtime-based parallel programming model for C++ code that uses threads. 
It consists of a template-based runtime library to help you harness the latent performance of multi-core processors.

oneTBB enables you to simplify parallel programming by breaking computation into parallel running tasks. Within a single process, 
parallelism is carried out through threads, an operating system mechanism that allows the same or different sets of instructions 
to be executed simultaneously.


.. image:: Images/how-oneTBB-works.png


Use oneTBB to write scalable applications that:

* Specify logical parallel structure instead of threads
* Emphasize data-parallel programming
* Take advantage of concurrent collections and parallel algorithms

oneTBB supports nested parallelism as well. It means that you can use the library without being worried about overloading a computer. 
