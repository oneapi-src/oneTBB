# Installation from Sources

---
**NOTE:** The project uses CMake build configuration. Make sure that you have CMake version 3.1 or higher.

---


## Installation and packaging

---
**CAUTION**

Be careful about installing prefix. It defaults to `/usr/local` on UNIX* and `c:/Program Files/${PROJECT_NAME}` on Windows* OS.
You can define custom `CMAKE_INSTALL_PREFIX` during configuration:

```
cmake -DCMAKE_INSTALL_PREFIX=/my/install/prefix ..
```

---


The following install components are supported:
- `runtime` - oneTBB runtime package (core shared libraries and `.dll` files on Windows* OS).
- `devel` - oneTBB development package (header files, CMake integration files, library symbolic links, and `.lib` files on Windows* OS).
- `tbb4py` - [oneTBB Module for Python](#onetbb-python-module-support).

To install specific components after configuration and build, run:

```bash
cmake -DCOMPONENT=<component> [-DBUILD_TYPE=<build-type>] -P cmake_install.cmake
```

Simple packaging using CPack is supported.
The following commands allow you to create a simple portable package that includes header files, libraries, and integration files for CMake:

```bash
cmake <options> ..
cpack
```
