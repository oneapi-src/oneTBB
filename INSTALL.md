# Installation from Sources

---
**NOTE:** The project uses CMake build configuration. Make sure that you have CMake version 3.1 or higher.

---

## Configure, build, and test

### Preparation

To perform out-of-source build, create a build directory and go there:

```bash
mkdir /tmp/my-build
cd /tmp/my-build
```

### Configure

```bash
cmake <options> <repo_root>
```

Some useful options:
- `-G <generator>` - specify particular project generator. See `cmake --help` for details.
- `-DCMAKE_BUILD_TYPE=Debug` - specify for Debug build. It is not applicable for multiconfig generators, e.g. for Visual Studio* generator.

#### TBBBind library configuration

The TBBbind library has three versions: `tbbbind`, `tbbbind_2_0`, and `tbbbind_2_5`. Each of these versions is linked with corresponding HWLOC library version: 
- `tbbbind` links with HWLOC 1.11.x 
- `tbbbind_2_0` links with HWLOC 2.1–2.4
- `tbbbind_2_5` links with HWLOC 2.5 and later

The search for a suitable version of the HWLOC library is enabled by default. If you want to use a specific version of the library, you can specify the path to it manually using the following CMake variables:

 - `CMAKE_HWLOC_<HWLOC_VER>_LIBRARY_PATH` - path to the corresponding HWLOC version shared library on Linux* OS or path to `.lib` file on Windows* OS
 - `CMAKE_HWLOC_<HWLOC_VER>_INCLUDE_PATH` - path to the corresponding HWLOC version including directory


---
**NOTE:** Automatic HWLOC searching requires CMake version 3.6 or higher.

---


Windows* OS requires an additional variable for correct TBBBind library building:
 - `CMAKE_HWLOC_<HWLOC_VER>_DLL_PATH` - path to the corresponding HWLOC version `.dll` file.

`HWLOC_VER` substring used earlier can be replaced with one of the three values:
- `1_11` for the `tbbbind` library configuration
- `2` for the `tbbbind_2_0`
- `2_5` for the `tbbbind_2_5` library configuration

If you specify variables for several TBBBind versions, the building process for all of these versions is performed during single build session.

---
**TIP**

Specify the `TBB_DISABLE_HWLOC_AUTOMATIC_SEARCH` to disable HWLOC libraries automatic search.

---


### Build

```bash
cmake --build . <options>
```

Some useful options:
- `--target <target>` - specific target, "all" is default.
- `--config <Release|Debug>` - build configuration, applicable only for multiconfig generators, e.g. Visual Studio* generator.

The binaries are placed to `./<compiler-id>_<compiler-ver>_cxx<stdver>_<build-type>`. For example, `./gnu_4.8_cxx11_release`.

#### Build for 32-bit

* **Intel(R) Compiler**. Source Intel(R) C++ Compiler with `ia32` and build as usual.
* **MSVC**. Use switch for [generator](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html) (e.g. `-A Win32` for [VS2019](https://cmake.org/cmake/help/latest/generator/Visual%20Studio%2016%202019.html)) during the configuration stage and then build as usual.
* **GCC/Clang**. Specify `-m32` during the configuration. It can be `CXXFLAGS=-m32 cmake ..` or `cmake -DCMAKE_CXX_FLAGS=-m32 ..`
* For any other compiler, which builds for 64-bit by default, specify 32-bit compiler key during the configuration as above.

#### Windows* OS specific builds

---
**NOTE**

Following builds require CMake version 3.15 or higher. 

---

* **Dynamic linkage with C Runtime Library (CRT)**. The default behavior can be explicitly specified by setting `CMAKE_MSVC_RUNTIME_LIBRARY` to `MultiThreadedDLL` or `MultiThreadedDebugDLL`.
```bash
cmake ..  # dynamic linkage is used by default
```
```bash
cmake -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL ..
```
```bash
cmake -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebugDLL -DCMAKE_BUILD_TYPE=Debug ..
```
* **Static linkage with CRT**. Set `CMAKE_MSVC_RUNTIME_LIBRARY` to `MultiThreaded` or `MultiThreadedDebug`.
```bash
cmake -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded ..
```
```bash
cmake -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug -DCMAKE_BUILD_TYPE=Debug ..
```
* **Windows* OS 10 Universal Windows application build**. Set `CMAKE_SYSTEM_NAME` to `WindowsStore` and `CMAKE_SYSTEM_VERSION` to `10.0`.

---
**NOTE**

Set `TBB_NO_APPCONTAINER` to `ON` in order to apply `/APPCONTAINER:NO` option during the compilation (used for testing).
```bash
cmake -DCMAKE_SYSTEM_NAME:STRING=WindowsStore -DCMAKE_SYSTEM_VERSION:STRING=10.0 ..
```

---

* **Universal Windows* OS Driver build**. Set `TBB_WINDOWS_DRIVER` to `ON` and use static linkage with CRT.

```bash
cmake -DTBB_WINDOWS_DRIVER=ON -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded ..
```

### Test

#### Build test
To build a test, use the default target 'all':
```
cmake --build .
```

Or use a specific test target:
```
cmake --build . --target <test> # e.g. test_version
```

#### Run test

You can run a test by using CTest:
```bash
ctest
```

Or by using 'test' target:
```bash
cmake --build . --target test # currently does not work on Windows* OS
```


## Intallation and packaging

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