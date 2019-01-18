
#tbb_system_name must be configured before this file is included
if (tbb_system_name STREQUAL "Linux")
   set(TBB_SHARED_LIB_DIR "lib")  # Directory for shared libs
   set(TBB_X32_SUBDIR "ia32")     # 32bit architecture subdir
   set(TBB_X64_SUBDIR "intel64")  # 64bit architecture subdir
   set(TBB_LIB_PREFIX "lib")      # file prefix
   set(TBB_LIB_EXT "so.2")        # file extension
elseif (tbb_system_name STREQUAL "Windows")
   set(TBB_SHARED_LIB_DIR "bin")
   set(TBB_X32_SUBDIR "ia32")
   set(TBB_X64_SUBDIR "intel64")
   set(TBB_LIB_PREFIX "")
   set(TBB_LIB_EXT "dll")
elseif (tbb_system_name STREQUAL "Darwin")
   set(TBB_SHARED_LIB_DIR "lib")
   set(TBB_X32_SUBDIR ".")
   set(TBB_X64_SUBDIR ".")
   set(TBB_LIB_PREFIX "lib")
   set(TBB_LIB_EXT "dylib")
elseif (tbb_system_name STREQUAL "Android")
   set(TBB_SHARED_LIB_DIR "lib")
   set(TBB_X32_SUBDIR ".")
   set(TBB_X64_SUBDIR "x86_64")
   set(TBB_LIB_PREFIX "lib")
   set(TBB_LIB_EXT "so")
else()
   message(FATAL_ERROR "Unsupported OS name: ${tbb_system_name}")
endif()
set(TBB_STATICLIB_EXT "a")

set(make_tool_name make)
if (${tbb_system_name} MATCHES "Windows")
   set(make_tool_name gmake)
elseif (CMAKE_SYSTEM_NAME MATCHES "Android")
   set(make_tool_name ndk-build)
endif()