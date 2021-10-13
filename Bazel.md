# Bazel build support

The main build system of oneTBB is CMake.
[Bazel](https://bazel.build/) support is community-based.
The maintainers of oneTBB do not use Bazel internally.
The Bazel configuration may not include recommended compiler and/or linker flags used in the official CMake configuration.

The Bazel build of oneTBB currently only aims for a subset of oneTBB that suffices restricted use cases of the usage of oneTBB.
Pull requests to improve the Bazel build experience are welcomed.

The standard approach of how Bazel handles third-party libraries is static linking. 
Even this is not recommended by the oneTBB maintainers this is chosen since this is considered as a best practice within the Bazel ecosystem.

## Using oneTBB as a dependency

The following minimal example shows how to use oneTBB as a dependency within a Bazel project.

The following file structure is assumed:

```
example
├── .bazelrc
├── BUILD.bazel
├── main.cpp
└── WORKSPACE.bazel
```

_WORKSPACE.bazel_:
```python
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "oneTBB",
    branch = "master",
    remote = "https://github.com/oneapi-src/oneTBB/",
)
```

In the *WORKSPACE* file, the oneTBB GitHub repository is fetched. 

_BUILD.bazel_:

```python
cc_binary(
    name = "Demo",
    srcs = ["main.cpp"],
    deps = ["@oneTBB//:tbb"],
)
```

The *BUILD* file defines a binary named `Demo` that has a dependency to oneTBB.

_main.cpp_:

```c++
#include "oneapi/tbb/version.h"

#include <iostream>

int main() {
    std::cout << "Hello from oneTBB "
              << TBB_VERSION_MAJOR << "."
              << TBB_VERSION_MINOR << "."
              << TBB_VERSION_PATCH
              << "!" << std::endl;

    return 0;
}
```

The expected output of this program is the current version number of oneTBB.

Switch to the folder where you have created the above-mentioned files and run the binary via `bazel run //:Demo`.

## Build oneTBB using Bazel

Run `bazel build //...` in the root directory of oneTBB.

## Compiler support

The Bazel build makes use of the compiler flag `-mwaitpkg` in non-Windows builds.
This flag is supported by GCC 9.3, Clang 12, and newer versions of those tools.
If you want to use the Bazel build in combination with earlier versions of GCC you should remove this flag since it will lead to errors during compilation.