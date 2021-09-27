.. _Get_Started_Guide

Get Started with |full_name|
============================


|full_name| enables you to simplify parallel programming by breaking 
computation into parallel running tasks. oneTBB is available as a stand-alone
product and as part of the |base_tk|.

|short_name| is a runtime-based parallel programming model for C++ code that uses threads.
It consists of a template-based runtime library to help you harness the latent performance
of multi-core processors. Use |short_name| to write scalable applications that:

- Specify logical parallel structure instead of threads
- Emphasize data parallel programming
- Take advantage of concurrent collections and parallel algorithms


Before You Begin
****************

After installing |short_name|, you need to set the environment variables:
  
#. Go to the oneTBB installation directory (``<install_dir>``). By default, ``<install_dir>`` is the following:
     
   * On Linux* OS:
	 
     * For superusers (root): ``/opt/intel/oneapi``
     * For ordinary users (non-root): ``$HOME/intel/oneapi``
     
   * On Windows* OS:

     * ``<Program Files>\Intel\oneAPI``

#. Set the environment variables, using the script in <install_dir>, by running
     
   * On Linux* OS:
	 
     ``vars.{sh|csh} in <install_dir>/tbb/latest/env``
	   
   * On Windows* OS:
	 
     ``vars.bat in <install_dir>/tbb/latest/env``


Example
*******

Below you can find a typical example for a |short_name| algorithm. 
The sample calculates a sum of all integer numbers from 1 to 100. 

.. code:: cpp

   int sum = oneapi::tbb::parallel_reduce(oneapi::tbb::blocked_range<int>(1,101), 0,
      [](oneapi::tbb::blocked_range<int> const& r, int init) -> int {
         for (int v = r.begin(); v != r.end(); v++  ) {
            init += v;
         }
         return init;
      },
      [](int lhs, int rhs) -> int {
         return lhs + rhs;
      }
   );



Find more
*********

See our `documentation <https://oneapi-src.github.io/oneTBB/>`_ to learn more about |short_name|.
