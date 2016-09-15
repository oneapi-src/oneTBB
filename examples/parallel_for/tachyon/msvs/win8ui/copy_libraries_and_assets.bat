@echo on
REM
REM Copyright 2005-2016 Intel Corporation.  All Rights Reserved.
REM
REM The source code contained or described herein and all documents related
REM to the source code ("Material") are owned by Intel Corporation or its
REM suppliers or licensors.  Title to the Material remains with Intel
REM Corporation or its suppliers and licensors.  The Material is protected
REM by worldwide copyright laws and treaty provisions.  No part of the
REM Material may be used, copied, reproduced, modified, published, uploaded,
REM posted, transmitted, distributed, or disclosed in any way without
REM Intel's prior express written permission.
REM
REM No license under any patent, copyright, trade secret or other
REM intellectual property right is granted to or conferred upon you by
REM disclosure or delivery of the Materials, either expressly, by
REM implication, inducement, estoppel or otherwise.  Any license under such
REM intellectual property rights must be express and approved by Intel in
REM writing.
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
