# Installation from Sources

---
**NOTE:** The project uses CMake build configuration. Make sure that you have CMake version 3.1 or higher.

---


## Required Software
To create configuration files for a build system, you should have one of the following CMake generators:
* Command-Line Build Tool Generators. For example, Borland Makefiles* or Ninja*.
* IDE Build Tool Generators. For example, Visual Studio* 16 2019.
* Extra Generators. For example, CodeBlocks* or Eclipse* CDT4.


## Installation Steps

The following instruction is an example of how you can install oneTBB. 
See [Build system description](cmake/README.md) to fully understand how to configure, build, test, and install the project. 

---
**CAUTION:** Be careful about installation. Avoid commands like `make install` that make this directory pre-pended onto all install directories.
By default, this directive is `/usr/local` on UNIX* and `c:/Program Files/${PROJECT_NAME}` on Windows*.

To avoid this, use:
```
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
```

---


1. Clone the sources from GitHub\* as follows:
```
git clone https://github.com/oneapi-src/oneTBB.git
```

2. To configure the system, run:
```
cmake <options> <repo_root>
```

3. Build oneTBB via command-line:
```
cmake --build . <options>
```

4. If needed, run the tests and get the results by using: ```ctest```

5. To install the specific components, run:

```bash
cmake -DCOMPONENT=<component> [-DBUILD_TYPE=<build-type>] -P cmake_install.cmake
```

  The following installation components are supported:

  * `runtime` - oneTBB runtime package (core shared libraries and .dll files on Windows).
  * `devel` - oneTBB development package (header files, CMake integration files, library symbolic links, and .lib files on Windows).
  * `tbb4py` - oneTBB Module for Python.

6. Simple packaging using CPack is supported.
The following commands allow you to create a simple portable package that includes header files, libraries, and integration files for CMake:

```bash
cmake <options> ..
cpack
```

