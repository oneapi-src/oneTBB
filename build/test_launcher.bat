@echo off
REM
REM Copyright 2005-2016 Intel Corporation.  All Rights Reserved.
REM
REM This file is part of Threading Building Blocks. Threading Building Blocks is free software;
REM you can redistribute it and/or modify it under the terms of the GNU General Public License
REM version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
REM distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
REM implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
REM See  the GNU General Public License for more details.   You should have received a copy of
REM the  GNU General Public License along with Threading Building Blocks; if not, write to the
REM Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA
REM
REM As a special exception,  you may use this file  as part of a free software library without
REM restriction.  Specifically,  if other files instantiate templates  or use macros or inline
REM functions from this file, or you compile this file and link it with other files to produce
REM an executable,  this file does not by itself cause the resulting executable to be covered
REM by the GNU General Public License. This exception does not however invalidate any other
REM reasons why the executable file might be covered by the GNU General Public License.
REM

set cmd_line=
if DEFINED run_prefix set cmd_line=%run_prefix%
:while
if NOT "%1"=="" (
    REM Verbose mode
    if "%1"=="-v" (
        set verbose=yes
        GOTO continue
    )
    REM Silent mode of 'make' requires additional support for associating
    REM of test output with the test name. Verbose mode is the simplest way
    if "%1"=="-q" (
        set verbose=yes
        GOTO continue
    )
    REM Run in stress mode
    if "%1"=="-s" (
        echo Doing stress testing. Press Ctrl-C to terminate
        set stress=yes
        GOTO continue
    )
    REM Repeat execution specified number of times
    if "%1"=="-r" (
        set repeat=%2
        SHIFT
        GOTO continue
    )
    REM no LD_PRELOAD under Windows
    REM but run the test to check "#pragma comment" construction
    if "%1"=="-l" (
        REM The command line may specify -l with empty dll name,
        REM e.g. "test_launcher.bat -l  app.exe". If the dll name is
        REM empty then %2 contains the application name and the SHIFT
        REM operation is not necessary.
        if exist "%3" SHIFT
        GOTO continue
    )
    REM no need to setup up stack size under Windows
    if "%1"=="-u" GOTO continue
    set cmd_line=%cmd_line% %1
:continue
    SHIFT
    GOTO while
)
set cmd_line=%cmd_line:./=.\%
if DEFINED verbose echo Running %cmd_line%
if DEFINED stress set cmd_line=%cmd_line% ^& IF NOT ERRORLEVEL 1 GOTO stress
:stress
if DEFINED repeat (
    for /L %%i in (1,1,%repeat%) do echo %%i of %repeat%: & %cmd_line%
) else (
    %cmd_line%
)
