/*
    Copyright (c) 2005-2016 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.




*/

// TODO: revise by comparing with mac_ppc.h

#if !defined(__TBB_machine_H) || defined(__TBB_machine_xbox360_ppc_H)
#error Do not #include this internal file directly; use public TBB headers instead.
#endif

#define __TBB_machine_xbox360_ppc_H

#define NONET
#define NOD3D
#include "xtl.h"
#include "ppcintrinsics.h"

#if _MSC_VER >= 1300
extern "C" void _MemoryBarrier();
#pragma intrinsic(_MemoryBarrier)
#define __TBB_control_consistency_helper() __isync()
#define __TBB_acquire_consistency_helper() _MemoryBarrier()
#define __TBB_release_consistency_helper() _MemoryBarrier()
#endif

#define __TBB_full_memory_fence() __sync()

#define __TBB_WORDSIZE 4
#define __TBB_ENDIANNESS __TBB_ENDIAN_BIG

//todo: define __TBB_USE_FENCED_ATOMICS and define acquire/release primitives to maximize performance

inline __int32 __TBB_machine_cmpswp4(volatile void *ptr, __int32 value, __int32 comparand ) {
 __sync();
 __int32 result = InterlockedCompareExchange((volatile LONG*)ptr, value, comparand);
 __isync();
 return result;
}

inline __int64 __TBB_machine_cmpswp8(volatile void *ptr, __int64 value, __int64 comparand )
{
 __sync();
 __int64 result = InterlockedCompareExchange64((volatile LONG64*)ptr, value, comparand);
 __isync();
 return result;
}

#define __TBB_USE_GENERIC_PART_WORD_CAS                     1
#define __TBB_USE_GENERIC_FETCH_ADD                         1
#define __TBB_USE_GENERIC_FETCH_STORE                       1
#define __TBB_USE_GENERIC_HALF_FENCED_LOAD_STORE            1
#define __TBB_USE_GENERIC_RELAXED_LOAD_STORE                1
#define __TBB_USE_GENERIC_DWORD_LOAD_STORE                  1
#define __TBB_USE_GENERIC_SEQUENTIAL_CONSISTENCY_LOAD_STORE 1

#pragma optimize( "", off )
inline void __TBB_machine_pause (__int32 delay )
{
 for (__int32 i=0; i<delay; i++) {;};
}
#pragma optimize( "", on )

#define __TBB_Yield()  Sleep(0)
#define __TBB_Pause(V) __TBB_machine_pause(V)

// This port uses only 2 hardware threads for TBB on XBOX 360.
// Others are left to sound etc.
// Change the following mask to allow TBB use more HW threads.
static const int __TBB_XBOX360_HARDWARE_THREAD_MASK = 0x0C;

static inline int __TBB_XBOX360_DetectNumberOfWorkers()
{
     char a[__TBB_XBOX360_HARDWARE_THREAD_MASK];  //compile time assert - at least one bit should be set always
     a[0]=0;

     return ((__TBB_XBOX360_HARDWARE_THREAD_MASK >> 0) & 1) +
            ((__TBB_XBOX360_HARDWARE_THREAD_MASK >> 1) & 1) +
            ((__TBB_XBOX360_HARDWARE_THREAD_MASK >> 2) & 1) +
            ((__TBB_XBOX360_HARDWARE_THREAD_MASK >> 3) & 1) +
            ((__TBB_XBOX360_HARDWARE_THREAD_MASK >> 4) & 1) +
            ((__TBB_XBOX360_HARDWARE_THREAD_MASK >> 5) & 1) + 1;  // +1 accommodates for the master thread
}

static inline int __TBB_XBOX360_GetHardwareThreadIndex(int workerThreadIndex)
{
    workerThreadIndex %= __TBB_XBOX360_DetectNumberOfWorkers()-1;
    int m = __TBB_XBOX360_HARDWARE_THREAD_MASK;
    int index = 0;
    int skipcount = workerThreadIndex;
    while (true)
    {
        if ((m & 1)!=0)
        {
            if (skipcount==0) break;
            skipcount--;
        }
        m >>= 1;
       index++;
    }
    return index;
}

#define __TBB_HardwareConcurrency() __TBB_XBOX360_DetectNumberOfWorkers()
