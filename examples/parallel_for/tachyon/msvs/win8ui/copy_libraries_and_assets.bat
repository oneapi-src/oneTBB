@echo on
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
:: Getting parameters
:: Architecture
if ("%1") == ("") goto error0
:: Release/Debug
if ("%2") == ("") goto error0
:: Output directory
if (%3) == ("") goto error0
set arch=%1
if ("%2") == ("debug") set postfix=_debug
set output_dir="%3"
if ("%4") == ("") set dat_file="%output_dir%\..\..\dat\balls.dat"

:: Actually we can set install root by ourselves
if ("%TBBROOT%") == ("") set TBBROOT=%~dp0..\..\..\..\..\

:: ordered from oldest to newest, so we end with newest available version
if ("%VS110COMNTOOLS%") NEQ ("") set vc_dir=vc11_ui
if ("%VS120COMNTOOLS%") NEQ ("") set vc_dir=vc12_ui
echo Using %vc_dir% libraries

if exist "%TBBROOT%\bin\%arch%\%vc_dir%\tbb%postfix%.dll" set interim_path=bin\%arch%
if exist "%TBBROOT%..\redist\%arch%\tbb\%vc_dir%\tbb%postfix%.dll" set interim_path=..\redist\%arch%\tbb
if exist "%TBBROOT%\lib\%arch%\%vc_dir%\tbb%postfix%.lib" set interim_lib_path=lib\%arch%
if ("%interim_path%") == ("") goto error1
if ("%interim_lib_path%") == ("") goto error1

:: We know everything we wanted and there are no errors
:: Copying binaries

copy "%TBBROOT%\%interim_path%\%vc_dir%\tbb%postfix%.dll" "%output_dir%"
copy "%TBBROOT%\%interim_path%\%vc_dir%\tbb%postfix%.pdb" "%output_dir%"
copy "%TBBROOT%\%interim_path%\%vc_dir%\tbbmalloc%postfix%.dll" "%output_dir%"
copy "%TBBROOT%\%interim_path%\%vc_dir%\tbbmalloc%postfix%.pdb" "%output_dir%"
copy "%TBBROOT%\%interim_lib_path%\%vc_dir%\tbb%postfix%.lib" "%output_dir%"

:: Copying DAT-file
echo Using DAT-file %dat_file% 
if exist %dat_file% copy %dat_file% "%output_dir%\Assets\balls.dat"

goto end
:error0
echo Custom build script usage: %0 [ia32 or intel64] [release or debug] [output dir] [dat-file]
exit /B 1
:error1
echo Could not determine path to Intel TBB libraries
exit /B 1

:end
exit /B 0
