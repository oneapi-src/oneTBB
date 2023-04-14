<!--
******************************************************************************
* 
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/-->

# System Requirements <!-- omit in toc -->
This document provides details about hardware, operating system, and software prerequisites for the oneAPI Threading Building Blocks (oneTBB). 

## Table of Contents <!-- omit in toc -->
- [Supported Hardware](#supported-hardware)
- [Software](#software)
  - [Supported Operating Systems](#supported-operating-systems)
  - [Community-Supported Platforms](#community-supported-platforms)
  - [Supported Compilers](#supported-compilers)


## Supported Hardware
- Intel(R) Celeron(R) processor family
- Intel(R) Core* processor family
- Intel(R) Xeon(R) processor family
- Intel(R) Xeon Phi* processor family
- Intel(R) Atom* processor family
- Non-Intel(R) processors compatible with the processors listed above


## Software

### Supported Operating Systems
- Systems with Microsoft* Windows* operating systems:
  - Microsoft* Windows* 10
  - Microsoft* Windows* 11
  - Microsoft* Windows* Server 2019
  - Microsoft* Windows* Server 2022
- Systems with Linux* operating systems:
  - Oracle Linux* 8
  - Amazon* Linux* 2
  - Debian* 9, 10, 11
  - Fedora* 36, 37
  - Red Hat* Enterprise Linux* 8, 9
  - SuSE* Linux* Enterprise Server 15
  - Ubuntu* 20.04, 22.04
- Systems with macOS* operating systems:
  - macOS* 12.x, 13.x
- Systems with Android* operating systems:
  - Android* 9

### Community-Supported Platforms
- MinGW*
- FreeBSD*
- Microsoft* Windows* on ARM*/ARM64*
- macOS* on ARM64*

### Supported Compilers
- Intel* oneAPI DPC++/C++ Compiler
- Intel* C++ Compiler 19.0 and 19.1 version
- Microsoft* Visual C++ 14.2 (Microsoft* Visual Studio* 2019, Windows* OS only)
- Microsoft* Visual C++ 14.3 (Microsoft* Visual Studio* 2022, Windows* OS only)
- For each supported Linux* operating system, the standard gcc version provided with that operating system is supported:
  - GNU Compilers (gcc) 4.8.5 - 11.2.1
  - GNU C Library (glibc) version 2.17 - 2.34
  - Clang* 6.0.0 - 13.0.0
