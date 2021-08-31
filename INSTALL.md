# Installation from Sources

Required Software:
* DPC++/C++ Compiler
* Microsoft* Visual Studio* (Windows* OS only)
* GNU Compilers (gcc) 4.8.5–10
* GNU C Library (glibc) version 2.17–2.32
* Clang* 4.2–11.0

The project uses CMake build configuration. Make sure that you have CMake version 3.1 or higher. 

Before installing the library, learn how to [configure, build, and test](cmake\README.md) the project. 

NOTE: Be careful about installation. Avoid commands like `make install` unless you fully understand the consequences.

The following installation components are supported:

`runtime` - oneTBB runtime package (core shared libraries and .dll files on Windows).
`devel` - oneTBB development package (header files, CMake integration files, library symbolic links, and .lib files on Windows).
`tbb4py` - oneTBB Module for Python.

To install the specific components after configuration and build, run:

```bash
cmake -DCOMPONENT=<component> [-DBUILD_TYPE=<build-type>] -P cmake_install.cmake
```

Simple packaging using CPack is supported.
The following commands allow to create a simple portable package that includes header files, libraries, and integration files for CMake:

```bash
cmake <options> ..
cpack
```
