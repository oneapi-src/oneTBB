
# Prevent double invocation.
if (RISCV_TOOLCHAIN_INCLUDED)
    return()
endif()
set(RISCV_TOOLCHAIN_INCLUDED TRUE)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR riscv)

# User can use -DCMAKE_FIND_ROOT_PATH to specific toolchain path
set(CMAKE_C_COMPILER ${CMAKE_FIND_ROOT_PATH}/bin/riscv64-unknown-linux-gnu-clang)
set(CMAKE_CXX_COMPILER ${CMAKE_FIND_ROOT_PATH}/bin/riscv64-unknown-linux-gnu-clang++)
set(CMAKE_LINKER ${CMAKE_FIND_ROOT_PATH}/bin/riscv64-unknown-linux-gnu-ld)

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Most linux on riscv64 support rv64imafd_zba_zbb extensions
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=rv64imafd_zba_zbb -mabi=lp64d " CACHE INTERNAL "")
