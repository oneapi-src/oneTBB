/*
    Copyright 2005-2016 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks. Threading Building Blocks is free software;
    you can redistribute it and/or modify it under the terms of the GNU General Public License
    version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
    distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See  the GNU General Public License for more details.   You should have received a copy of
    the  GNU General Public License along with Threading Building Blocks; if not, write to the
    Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA

    As a special exception,  you may use this file  as part of a free software library without
    restriction.  Specifically,  if other files instantiate templates  or use macros or inline
    functions from this file, or you compile this file and link it with other files to produce
    an executable,  this file does not by itself cause the resulting executable to be covered
    by the GNU General Public License. This exception does not however invalidate any other
    reasons why the executable file might be covered by the GNU General Public License.
*/

#ifndef __TBB_harness_defs_H
#define __TBB_harness_defs_H

#include "tbb/tbb_config.h"
#if __FreeBSD__
#include <sys/param.h>  // for __FreeBSD_version
#endif

#if __TBB_TEST_PIC && !__PIC__
#define __TBB_TEST_SKIP_PIC_MODE 1
#else
#define __TBB_TEST_SKIP_PIC_MODE 0
#endif

// no need to test gcc builtins mode on ICC
#define __TBB_TEST_SKIP_GCC_BUILTINS_MODE ( __TBB_TEST_BUILTINS && (!__TBB_GCC_BUILTIN_ATOMICS_PRESENT || __INTEL_COMPILER) )

#define __TBB_TEST_SKIP_ICC_BUILTINS_MODE ( __TBB_TEST_BUILTINS && !__TBB_ICC_BUILTIN_ATOMICS_PRESENT )

#ifndef TBB_USE_GCC_BUILTINS
  //Force TBB to use GCC intrinsics port, but not on ICC, as no need
  #define TBB_USE_GCC_BUILTINS         ( __TBB_TEST_BUILTINS && __TBB_GCC_BUILTIN_ATOMICS_PRESENT && !__INTEL_COMPILER )
#endif

#ifndef TBB_USE_ICC_BUILTINS
  //Force TBB to use ICC c++11 style intrinsics port
  #define TBB_USE_ICC_BUILTINS         ( __TBB_TEST_BUILTINS && __TBB_ICC_BUILTIN_ATOMICS_PRESENT )
#endif

//ICC has a bug in assumptions of the modifications made via atomic pointer
#define __TBB_ICC_BUILTIN_ATOMICS_POINTER_ALIASING_BROKEN (TBB_USE_ICC_BUILTINS &&  __INTEL_COMPILER < 1400 && __INTEL_COMPILER > 1200)

#if (_WIN32 && !__TBB_WIN8UI_SUPPORT) || (__linux__ && !__ANDROID__ && !__bg__) || __FreeBSD_version >= 701000
#define __TBB_TEST_SKIP_AFFINITY 0
#else
#define __TBB_TEST_SKIP_AFFINITY 1
#endif

#if __INTEL_COMPILER
  #define __TBB_CPP11_REFERENCE_WRAPPER_PRESENT ( _TBB_CPP0X && __INTEL_COMPILER >= 1200 && \
    ( _MSC_VER >= 1600 || __TBB_GCC_VERSION >= 40400 || ( __clang__ && __cplusplus >= 201103L ) ) )
  #define __TBB_RANGE_BASED_FOR_PRESENT ( _TBB_CPP0X && __INTEL_COMPILER >= 1300 )
  #define __TBB_SCOPED_ENUM_PRESENT ( _TBB_CPP0X && __INTEL_COMPILER > 1100 )
#elif __clang__
  #define __TBB_CPP11_REFERENCE_WRAPPER_PRESENT ( _TBB_CPP0X && __cplusplus >= 201103L && (__TBB_GCC_VERSION >= 40400 || _LIBCPP_VERSION) )
  #define __TBB_RANGE_BASED_FOR_PRESENT ( _TBB_CPP0X && __has_feature(__cxx_range_for) )
  #define __TBB_SCOPED_ENUM_PRESENT ( _TBB_CPP0X && __has_feature(cxx_strong_enums) )
#elif __GNUC__
  #define __TBB_CPP11_REFERENCE_WRAPPER_PRESENT ( _TBB_CPP0X && __TBB_GCC_VERSION >= 40400 )
  #define __TBB_RANGE_BASED_FOR_PRESENT ( _TBB_CPP0X && __TBB_GCC_VERSION >= 40500 )
  #define __TBB_SCOPED_ENUM_PRESENT ( _TBB_CPP0X && __TBB_GCC_VERSION >= 40400 )
#elif _MSC_VER
  #define __TBB_CPP11_REFERENCE_WRAPPER_PRESENT ( _MSC_VER >= 1600 )
  #define __TBB_RANGE_BASED_FOR_PRESENT ( _MSC_VER >= 1700 )
  #define __TBB_SCOPED_ENUM_PRESENT ( _MSC_VER >= 1700 )
#endif

//Due to libc++ limitations in C++03 mode, do not pass rvalues to std::make_shared()
#define __TBB_CPP11_SMART_POINTERS_PRESENT ( _MSC_VER >= 1600 || _TBB_CPP0X && __TBB_GCC_VERSION >= 40400 || _LIBCPP_VERSION)
#define __TBB_LAMBDAS_PRESENT __TBB_CPP11_LAMBDAS_PRESENT // TODO: replace the old macro in tests
#define __TBB_TEST_SKIP_LAMBDA (__TBB_ICC_13_0_CPP11_STDLIB_SUPPORT_BROKEN || !__TBB_CPP11_LAMBDAS_PRESENT)

#if __GNUC__ && __ANDROID__
  /** Android GCC does not support _thread keyword **/
  #define __TBB_THREAD_LOCAL_VARIABLES_PRESENT 0
#else
  #define __TBB_THREAD_LOCAL_VARIABLES_PRESENT 1
#endif

//MSVC 2013 is unable to properly resolve call to overloaded operator= with std::initializer_list argument for std::pair list elements
#define __TBB_CPP11_INIT_LIST_ASSIGN_OP_RESOLUTION_BROKEN (_MSC_VER <= 1800 && _MSC_VER && !__INTEL_COMPILER)
//MSVC 2013 is unable to manage lifetime of temporary objects passed to a std::initializer_list constructor properly
#define __TBB_CPP11_INIT_LIST_TEMP_OBJS_LIFETIME_BROKEN (_MSC_FULL_VER < 180030501 && _MSC_VER && !__INTEL_COMPILER)
//Implementation of C++11 std::placeholders in libstdc++ coming with gcc prior to 4.5 reveals bug in Intel Compiler 13 causing "multiple definition" link errors.
#define __TBB_CPP11_STD_PLACEHOLDERS_LINKAGE_BROKEN ((__INTEL_COMPILER == 1300 || __INTEL_COMPILER == 1310 )&& __GXX_EXPERIMENTAL_CXX0X__ && __TBB_GCC_VERSION < 40500)
// Intel(R) Compiler have an issue when a scoped enum with a specified underlying type has negative values.
#define __TBB_ICC_SCOPED_ENUM_WITH_UNDERLYING_TYPE_NEGATIVE_VALUE_BROKEN ( _MSC_VER && !__TBB_DEBUG && __INTEL_COMPILER && __INTEL_COMPILER <= 1500 )
// Intel(R) Compiler have an issue with __atomic_load_explicit from a scoped enum with a specified underlying type.
#define __TBB_ICC_SCOPED_ENUM_WITH_UNDERLYING_TYPE_ATOMIC_LOAD_BROKEN ( TBB_USE_ICC_BUILTINS && !__TBB_DEBUG && __INTEL_COMPILER && __INTEL_COMPILER <= 1500 )
//Unable to use constexpr member functions to initialize compile time constants
#define __TBB_CONSTEXPR_MEMBER_FUNCTION_BROKEN (__INTEL_COMPILER == 1500)
// MSVC 2015 does not do compile-time initialization of static variables with constexpr constructors in debug mode
#define __TBB_STATIC_CONSTEXPR_INIT_BROKEN (_MSC_VER==1900 && !__INTEL_COMPILER && _DEBUG)

//some compilers do not generate implicitly move constructor and assignment operator, as this feature (r-value reference 3.0) was added later
#if __INTEL_COMPILER
  #define __TBB_CPP11_IMPLICIT_MOVE_MEMBERS_GENERATION_BROKEN  (__TBB_CPP11_RVALUE_REF_PRESENT && (__INTEL_COMPILER < 1400 || __INTEL_COMPILER == 1600 && __INTEL_COMPILER_UPDATE <= 1))
#elif __clang__
  #define __TBB_CPP11_IMPLICIT_MOVE_MEMBERS_GENERATION_BROKEN !__has_feature(cxx_implicit_moves)
#else
  #define __TBB_CPP11_IMPLICIT_MOVE_MEMBERS_GENERATION_BROKEN (__TBB_CPP11_RVALUE_REF_PRESENT && _MSC_VER && _MSC_VER <=1800)
#endif /* __INTEL_COMPILER */

#if __GNUC__ && __ANDROID__
  #define __TBB_EXCEPTION_TYPE_INFO_BROKEN ( __TBB_GCC_VERSION < 40600 )
#elif _MSC_VER
  #define __TBB_EXCEPTION_TYPE_INFO_BROKEN ( _MSC_VER < 1400 )
#else
  #define __TBB_EXCEPTION_TYPE_INFO_BROKEN 0
#endif

//! a function ptr cannot be converted to const T& template argument without explicit cast
#define __TBB_FUNC_PTR_AS_TEMPL_PARAM_BROKEN ( ((__linux__ || __APPLE__) && __INTEL_COMPILER && __INTEL_COMPILER < 1100) || __SUNPRO_CC )
#define __TBB_UNQUALIFIED_CALL_OF_DTOR_BROKEN (__GNUC__==3 && __GNUC_MINOR__<=3)

#define __TBB_CAS_8_CODEGEN_BROKEN (__TBB_x86_32 && __PIC__ && __TBB_GCC_VERSION == 40102 && !__INTEL_COMPILER)

#define __TBB_THROW_FROM_DTOR_BROKEN (__clang__ &&  (__apple_build_version__ &&  __apple_build_version__ < 5000279 || __TBB_CLANG_VERSION && __TBB_CLANG_VERSION < 50000))

//std::uncaught_exception is broken on some version of stdlibc++ (it returns true with no active exception)
#define __TBB_STD_UNCAUGHT_EXCEPTION_BROKEN (__linux__ && (__TBB_GCC_VERSION == 40407 || __TBB_GCC_VERSION >= 40800 && __TBB_GCC_VERSION < 50300))

#if __TBB_LIBSTDCPP_EXCEPTION_HEADERS_BROKEN
  #define _EXCEPTION_PTR_H /* prevents exception_ptr.h inclusion */
  #define _GLIBCXX_NESTED_EXCEPTION_H /* prevents nested_exception.h inclusion */
#endif

// TODO: Investigate the cases that require this macro.
#define __TBB_COMPLICATED_ADL_BROKEN ( __TBB_GCC_VERSION && __TBB_GCC_VERSION < 40400 )

// Intel Compiler fails to compile the comparison of tuples in some cases
#if __INTEL_COMPILER && __INTEL_COMPILER < 1600 || __INTEL_COMPILER == 1600 && __INTEL_COMPILER_UPDATE <= 1
  #define __TBB_TUPLE_COMPARISON_COMPILATION_BROKEN (__TBB_GCC_VERSION >= 40800 && __TBB_GCC_VERSION <= 50101 || __MIC__)
#endif

// Intel Compiler fails to compile std::reference in some cases
#if __INTEL_COMPILER && __INTEL_COMPILER < 1600 || __INTEL_COMPILER == 1600 && __INTEL_COMPILER_UPDATE <= 1
  #define __TBB_REFERENCE_WRAPPER_COMPILATION_BROKEN (__TBB_GCC_VERSION >= 40800 && __TBB_GCC_VERSION <= 50101 || __MIC__)
#endif

// The tuple-based tests with more inputs take a long time to compile.  If changes
// are made to the tuple implementation or any switch that controls it, or if testing
// with a new platform implementation of std::tuple, the test should be compiled with
// MAX_TUPLE_TEST_SIZE >= 10 (or the largest number of elements supported) to ensure
// all tuple sizes are tested.  Expect a very long compile time.
#ifndef MAX_TUPLE_TEST_SIZE
    #if TBB_USE_DEBUG
        #define MAX_TUPLE_TEST_SIZE 3
    #else
        #define MAX_TUPLE_TEST_SIZE 5
    #endif
#else
    #if _MSC_VER
// test sizes <= 8 don't get "decorated name length exceeded" errors. (disable : 4503)
        #if MAX_TUPLE_TEST_SIZE > 8
            #undef MAX_TUPLE_TEST_SIZE
            #define MAX_TUPLE_TEST_SIZE 8
        #endif
    #endif
    #if MAX_TUPLE_TEST_SIZE > __TBB_VARIADIC_MAX
        #undef MAX_TUPLE_TEST_SIZE
        #define MAX_TUPLE_TEST_SIZE __TBB_VARIADIC_MAX
    #endif
#endif

#ifndef TBB_PREVIEW_FLOW_GRAPH_FEATURES
    #if __TBB_CPF_BUILD
        #define TBB_PREVIEW_FLOW_GRAPH_FEATURES 1
    #endif
#endif

namespace Harness {
    //! Utility template function to prevent "unused" warnings by various compilers.
    template<typename T> void suppress_unused_warning( const T& ) {}

    //TODO: unify with one in tbb::internal
    //! Utility helper structure to ease overload resolution
    template<int > struct int_to_type {};
}

#endif /* __TBB_harness_defs_H */
