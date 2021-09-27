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

## Installation Steps
1. Clone the sources from GitHub\* as follows:
```
git clone https://github.com/oneapi-src/oneTBB.git
```
2. Set the environment variables for one of the supported C/C++ compilers. For example:
- **Intel(R) oneAPI DPC++/C++ Compiler 2021.1 (Windows\*)**:
```
 call "C:\Program Files (x86)\Intel\oneAPI\compiler\latest\env\vars.bat" intel64
```
3. Choose a version to install, it is recommended to use the newest one:
```
git checkout <version>
```
4. Build oneTBB via command-line. 
```
cmake --build . <options>
```
Binaries are placed to ```./<compiler-id>_<compiler-ver>_cxx<stdver>_<build-type>```, for example, ```./gnu_4.8_cxx11_release``` .

5. To install the specific components after configuration and build, run:

```bash
cmake -DCOMPONENT=<component> [-DBUILD_TYPE=<build-type>] -P cmake_install.cmake
```

The following installation components are supported:

`runtime` - oneTBB runtime package (core shared libraries and .dll files on Windows).
`devel` - oneTBB development package (header files, CMake integration files, library symbolic links, and .lib files on Windows).
`tbb4py` - oneTBB Module for Python.

6. Simple packaging using CPack is supported.
The following commands allow you to create a simple portable package that includes header files, libraries, and integration files for CMake:

```bash
cmake <options> ..
cpack
```
7. To run the tests and get the results, use:
```ctest```
