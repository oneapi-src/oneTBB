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

// Header that sets HAVE_m128/HAVE_m256 if vector types (__m128/__m256) are available

//! Class for testing safety of using vector types.
/** Uses circuitous logic forces compiler to put __m128/__m256 objects on stack while
    executing various methods, and thus tempt it to use aligned loads and stores
    on the stack. */
//  Do not create file-scope objects of the class, because MinGW (as of May 2010)
//  did not always provide proper stack alignment in destructors of such objects.

#if (_MSC_VER>=1600)
//TODO: handle /arch:AVX in the right way.
#pragma warning (push)
#pragma warning (disable: 4752)
#endif

template<typename __Mvec>
class ClassWithVectorType {
    static const int n = 16;
    static const int F = sizeof(__Mvec)/sizeof(float);
    __Mvec field[n];
    void init( int start );
public:
    ClassWithVectorType() {init(-n);}
    ClassWithVectorType( int i ) {init(i);}
    void operator=( const ClassWithVectorType& src ) {
        __Mvec stack[n];
        for( int i=0; i<n; ++i )
            stack[i^5] = src.field[i];
        for( int i=0; i<n; ++i )
            field[i^5] = stack[i];
    }
    ~ClassWithVectorType() {init(-2*n);}
    friend bool operator==( const ClassWithVectorType& x, const ClassWithVectorType& y ) {
        for( int i=0; i<F*n; ++i )
            if( ((const float*)x.field)[i]!=((const float*)y.field)[i] )
                return false;
        return true;
    }
    friend bool operator!=( const ClassWithVectorType& x, const ClassWithVectorType& y ) {
        return !(x==y);
    }
};

template<typename __Mvec>
void ClassWithVectorType<__Mvec>::init( int start ) {
    __Mvec stack[n];
    for( int i=0; i<n; ++i ) {
        // Declaring value as a one-element array instead of a scalar quites
        // gratuitous warnings about possible use of "value" before it was set.
        __Mvec value[1];
        for( int j=0; j<F; ++j )
            ((float*)value)[j] = float(n*start+F*i+j);
        stack[i^5] = value[0];
    }
    for( int i=0; i<n; ++i )
        field[i^5] = stack[i];
}

#if (__AVX__ || (_MSC_VER>=1600 && _M_X64)) && !defined(__sun)
#include <immintrin.h>
#define HAVE_m256 1
typedef ClassWithVectorType<__m256> ClassWithAVX;
#if _MSC_VER
#include <intrin.h> // for __cpuid
#endif
bool have_AVX() {
    bool result = false;
    const int avx_mask = 1<<28;
#if _MSC_VER || __INTEL_COMPILER
    int info[4] = {0,0,0,0};
    const int ECX = 2;
    __cpuid(info, 1);
    result = (info[ECX] & avx_mask)!=0;
#elif __GNUC__
    int ECX;
    __asm__( "cpuid"
             : "=c"(ECX)
             : "a" (1)
             : "ebx", "edx" );
    result = (ECX & avx_mask);
#endif
    return result;
}
#endif /* __AVX__ etc */

#if (__SSE__ || _M_IX86_FP || _M_X64) && !defined(__sun)
#include <xmmintrin.h>
#define HAVE_m128 1
typedef ClassWithVectorType<__m128> ClassWithSSE;
#endif

#if (_MSC_VER>=1600)
#pragma warning (pop)
#endif
