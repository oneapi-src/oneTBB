set(tbb_32_FROM
    _ZN3tbb6detail2r115create_itt_syncEPvPKcS4_
    _ZN3tbb6detail2r117itt_set_sync_nameEPvPKc
)
set(tbb_32_TO
    _ZN3tbb6detail2r115create_itt_syncEPvPKwS4_
    _ZN3tbb6detail2r117itt_set_sync_nameEPvPKw
)
set(tbb_64_FROM
    ${tbb_32_FROM}
    _ZN3tbb6detail2r115allocate_memoryEm
    _ZN3tbb6detail2r122cache_aligned_allocateEm
    _ZN3tbb6detail2r18allocateERPNS0_2d117small_object_poolEm
    _ZN3tbb6detail2r18allocateERPNS0_2d117small_object_poolEmRKNS2_14execution_dataE
    _ZN3tbb6detail2r110deallocateERNS0_2d117small_object_poolEPvm
    _ZN3tbb6detail2r110deallocateERNS0_2d117small_object_poolEPvmRKNS2_14execution_dataE
    _ZN3tbb6detail2r114notify_waitersEm
    _ZN3tbb6detail2r16submitERNS0_2d14taskERNS2_18task_group_contextEPNS1_5arenaEm
    _ZN3tbb6detail2r120isolate_within_arenaERNS0_2d113delegate_baseEl
    _ZN3tbb6detail2r18finalizeERNS0_2d121task_scheduler_handleEl
    _ZN3tbb6detail2r117parallel_pipelineERNS0_2d118task_group_contextEmRKNS2_11filter_nodeE
    _ZN3tbb6detail2r126allocate_bounded_queue_repEm
    _ZN3tbb6detail2r126wait_bounded_queue_monitorEPNS1_18concurrent_monitorEmlRNS0_2d113delegate_baseE
    _ZN3tbb6detail2r128deallocate_bounded_queue_repEPhm
    _ZN3tbb6detail2r128notify_bounded_queue_monitorEPNS1_18concurrent_monitorEmm
)
set(tbb_64_TO
    ${tbb_32_TO}
    _ZN3tbb6detail2r115allocate_memoryEy
    _ZN3tbb6detail2r122cache_aligned_allocateEy
    _ZN3tbb6detail2r18allocateERPNS0_2d117small_object_poolEy
    _ZN3tbb6detail2r18allocateERPNS0_2d117small_object_poolEyRKNS2_14execution_dataE
    _ZN3tbb6detail2r110deallocateERNS0_2d117small_object_poolEPvy
    _ZN3tbb6detail2r110deallocateERNS0_2d117small_object_poolEPvyRKNS2_14execution_dataE
    _ZN3tbb6detail2r114notify_waitersEy
    _ZN3tbb6detail2r16submitERNS0_2d14taskERNS2_18task_group_contextEPNS1_5arenaEy
    _ZN3tbb6detail2r120isolate_within_arenaERNS0_2d113delegate_baseEx
    _ZN3tbb6detail2r18finalizeERNS0_2d121task_scheduler_handleEx
    _ZN3tbb6detail2r117parallel_pipelineERNS0_2d118task_group_contextEyRKNS2_11filter_nodeE
    _ZN3tbb6detail2r126allocate_bounded_queue_repEy
    _ZN3tbb6detail2r126wait_bounded_queue_monitorEPNS1_18concurrent_monitorEyxRNS0_2d113delegate_baseE
    _ZN3tbb6detail2r128deallocate_bounded_queue_repEPhy
    _ZN3tbb6detail2r128notify_bounded_queue_monitorEPNS1_18concurrent_monitorEyy
)
set(tbbmalloc_32_FROM

)
set(tbbmalloc_32_TO

)
set(tbbmalloc_64_FROM
    ${tbbmalloc_32_FROM}
    _ZN3rml11pool_createElPKNS_13MemPoolPolicyE
    _ZN3rml14pool_create_v1ElPKNS_13MemPoolPolicyEPPNS_10MemoryPoolE
    _ZN3rml11pool_mallocEPNS_10MemoryPoolEm
    _ZN3rml12pool_reallocEPNS_10MemoryPoolEPvm
    _ZN3rml20pool_aligned_reallocEPNS_10MemoryPoolEPvmm
    _ZN3rml19pool_aligned_mallocEPNS_10MemoryPoolEmm
)
set(tbbmalloc_64_TO
    ${tbbmalloc_32_TO}
    _ZN3rml11pool_createExPKNS_13MemPoolPolicyE
    _ZN3rml14pool_create_v1ExPKNS_13MemPoolPolicyEPPNS_10MemoryPoolE
    _ZN3rml11pool_mallocEPNS_10MemoryPoolEy
    _ZN3rml12pool_reallocEPNS_10MemoryPoolEPvy
    _ZN3rml20pool_aligned_reallocEPNS_10MemoryPoolEPvyy
    _ZN3rml19pool_aligned_mallocEPNS_10MemoryPoolEyy
)

set(DEF_START "\; Copyright (c) 2005-2020 Intel Corporation
\;
\; Licensed under the Apache License, Version 2.0 (the \"License\")\;
\; you may not use this file except in compliance with the License.
\; You may obtain a copy of the License at
\;
\;     http://www.apache.org/licenses/LICENSE-2.0
\;
\; Unless required by applicable law or agreed to in writing, software
\; distributed under the License is distributed on an \"AS IS\" BASIS,
\; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
\; See the License for the specific language governing permissions and
\; limitations under the License.

\; This file is organized with a section for each .cpp file.

EXPORTS

")

cmake_policy(SET CMP0007 NEW)

message("Generating mingw def files...")
foreach(component tbb tbbmalloc tbbbind)
    foreach(bitness 32 64)
        message("Generating win${bitness}-mingw-${component}.def")
        file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/../../src/${component}/def/win${bitness}-mingw-${component}.def ${DEF_START})
        file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/../../src/${component}/def/lin${bitness}-${component}.def INPUT)
        foreach(line IN LISTS INPUT)
            if(line MATCHES "(local:)|(}\;)")
                break()
            elseif(line MATCHES "[A-Za-z0-9_]+\;")
                string(REPLACE ";" "" symbol_no_semicolon "${line}")
                foreach(from to IN ZIP_LISTS ${component}_${bitness}_FROM ${component}_${bitness}_TO)
                    if(symbol_no_semicolon STREQUAL ${from})
                        set(symbol_no_semicolon ${to})
                        break()
                    endif()
                endforeach()
                file(APPEND ${CMAKE_CURRENT_SOURCE_DIR}/../../src/${component}/def/win${bitness}-mingw-${component}.def
                    "${symbol_no_semicolon}\n")
            endif()
        endforeach()
    endforeach()
endforeach()