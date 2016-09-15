/*
    Copyright 2005-2015 Intel Corporation.  All Rights Reserved.

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

#ifndef __TBB_function_replacement_H
#define __TBB_function_replacement_H

#include <stddef.h> //for ptrdiff_t
typedef enum {
    FRR_OK,     /* Succeeded in replacing the function */
    FRR_NODLL,  /* The requested DLL was not found */
    FRR_NOFUNC, /* The requested function was not found */
    FRR_FAILED, /* The function replacement request failed */
} FRR_TYPE;

typedef enum {
    FRR_FAIL,     /* Required function */
    FRR_IGNORE,   /* optional function */
} FRR_ON_ERROR;

typedef void (*FUNCPTR)();

#ifndef UNICODE
#define ReplaceFunction ReplaceFunctionA
#else
#define ReplaceFunction ReplaceFunctionW
#endif //UNICODE

FRR_TYPE ReplaceFunctionA(const char *dllName, const char *funcName, FUNCPTR newFunc, const char ** opcodes, FUNCPTR* origFunc=NULL);
FRR_TYPE ReplaceFunctionW(const wchar_t *dllName, const char *funcName, FUNCPTR newFunc, const char ** opcodes, FUNCPTR* origFunc=NULL);

// Utilities to convert between ADDRESS and LPVOID
union Int2Ptr {
    UINT_PTR uip;
    LPVOID lpv;
};

inline UINT_PTR Ptr2Addrint(LPVOID ptr);
inline LPVOID Addrint2Ptr(UINT_PTR ptr);

// Use this value as the maximum size the trampoline region
const unsigned MAX_PROBE_SIZE = 32;

// The size of a jump relative instruction "e9 00 00 00 00"
const unsigned SIZE_OF_RELJUMP = 5;

// The size of jump RIP relative indirect "ff 25 00 00 00 00"
const unsigned SIZE_OF_INDJUMP = 6;

// The size of address we put in the location (in Intel64)
const unsigned SIZE_OF_ADDRESS = 8;

// The max distance covered in 32 bits: 2^31 - 1 - C
// where C should not be smaller than the size of a probe.
// The latter is important to correctly handle "backward" jumps.
const __int64 MAX_DISTANCE = (((__int64)1 << 31) - 1) - MAX_PROBE_SIZE;

// The maximum number of distinct buffers in memory
const ptrdiff_t MAX_NUM_BUFFERS = 256;

#endif //__TBB_function_replacement_H
