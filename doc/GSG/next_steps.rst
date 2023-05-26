.. _next_steps:

Next Steps
===========

After installing oneTBB, complete the following steps to start working with the library.

Set the Environment Variables
*****************************

After installing |short_name|, set the environment variables:
  
#. Go to the oneTBB installation directory. 

#. Set the environment variables using the script in ``<install_dir>`` by running:
     
   * On Linux* OS: ``vars.{sh|csh} in <install_dir>/tbb/latest/env``
   * On Windows* OS: ``vars.bat in <install_dir>/tbb/latest/env``


Build and Run a Sample 
**********************

.. tabs::

   .. group-tab:: Windows* OS

      #. Create a new C++ project using your IDE. In this example, Microsoft* Visual Studio* Code is used. 
      #. Create an ``example.cpp`` file in the project. 
      #. Copy and paste the code below. It is a typical example of a |short_name| algorithm. The sample calculates a sum of all integer numbers from 1 to 100. 
   
         .. code:: 

            #include <oneapi/tbb.h>
            
            int main (){
                int sum = oneapi::tbb::parallel_reduce(
                    oneapi::tbb::blocked_range<int>(1,101), 0,
                    [](oneapi::tbb::blocked_range<int> const& r, int init) -> int {
                        for (int v = r.begin(); v != r.end(); v++) {
                            init += v;
                        }
                        return init;
                    },
                    [](int lhs, int rhs) -> int {
                        return lhs + rhs;
                    }
                );
      
                printf("Sum: %d\n", sum);
                return 0;
            }
      
      #. Open the ``tasks.json`` file in the ``.vscode`` directory and paste the following lines to the args array:

         * ``-Ipath/to/oneTBB/include`` to add oneTBB include directory. 
         * ``path/to/oneTBB/`` to add oneTBB. 

         For example:

         .. code-block::

                { 
                   "tasks": [
                        {
                           "label": "build & run",
                           "type": "cppbuild",
                           "group": {
                           "args": [ 
                               "/IC:\\Program Files (x86)\\Intel\\oneAPI\\tbb\\2021.9.0\\include",
                               "C:\\Program Files (x86)\\Intel\\oneAPI\\tbb\\2021.9.0\\lib\\ia32\\vc14\\tbb12.lib"
                           

      #. Build the project. 
      #. Run the example. 
      #. If oneTBB is configured correctly, the output displays ``Sum: 5050``.  

   .. group-tab:: Linux* OS

      #. Create an ``example.cpp`` file in the project. 
      #. Copy and paste the code below. It is a typical example of a |short_name| algorithm. The sample calculates a sum of all integer numbers from 1 to 100. 
         
         .. code:: 

            #include <oneapi/tbb.h>

            int main(){
                int sum = oneapi::tbb::parallel_reduce(
                    oneapi::tbb::blocked_range<int>(1,101), 0,
                    [](oneapi::tbb::blocked_range<int> const& r, int init) -> int {
                        for (int v = r.begin(); v != r.end(); v++) {
                            init += v;
                        }
                        return init;
                    },
                    [](int lhs, int rhs) -> int {
                        return lhs + rhs;
                    }
                );
      
                printf("Sum: %d\n", sum);
                return 0;
            }

      #. Compile the code using oneTBB. For example, 

         .. code-block:: 

                g++ -std=c++11 example.cpp -o example -ltbb

      
      #. Run the executable:

         .. code-block:: 

                ./example
      
      #. If oneTBB is configured correctly, the output displays ``Sum: 5050``.  


