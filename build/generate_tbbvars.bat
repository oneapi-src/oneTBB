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
setlocal
for %%D in ("%tbb_root%") do set actual_root=%%~fD
set fslash_root=%actual_root:\=/%
set bin_dir=%CD%
set fslash_bin_dir=%bin_dir:\=/%
set _INCLUDE=INCLUDE& set _LIB=LIB
if not x%UNIXMODE%==x set _INCLUDE=CPATH& set _LIB=LIBRARY_PATH

if exist tbbvars.bat goto skipbat
echo Generating local tbbvars.bat
echo @echo off>tbbvars.bat
echo SET TBBROOT=%actual_root%>>tbbvars.bat
echo SET TBB_ARCH_PLATFORM=%arch%\%runtime%>>tbbvars.bat
echo SET TBB_TARGET_ARCH=%arch%>>tbbvars.bat
echo SET %_INCLUDE%=%%TBBROOT%%\include;%%%_INCLUDE%%%>>tbbvars.bat
echo SET %_LIB%=%bin_dir%;%%%_LIB%%%>>tbbvars.bat
echo SET PATH=%bin_dir%;%%PATH%%>>tbbvars.bat
if not x%UNIXMODE%==x echo SET LD_LIBRARY_PATH=%bin_dir%;%%LD_LIBRARY_PATH%%>>tbbvars.bat
:skipbat

if exist tbbvars.sh goto skipsh
echo Generating local tbbvars.sh
echo #!/bin/sh>tbbvars.sh
echo export TBBROOT="%fslash_root%">>tbbvars.sh
echo export TBB_ARCH_PLATFORM="%arch%\%runtime%">>tbbvars.sh
echo export TBB_TARGET_ARCH="%arch%">>tbbvars.sh
echo export %_INCLUDE%="${TBBROOT}/include;$%_INCLUDE%">>tbbvars.sh
echo export %_LIB%="%fslash_bin_dir%;$%_LIB%">>tbbvars.sh
echo export PATH="%fslash_bin_dir%;$PATH">>tbbvars.sh
if not x%UNIXMODE%==x echo export LD_LIBRARY_PATH="%fslash_bin_dir%;$LD_LIBRARY_PATH">>tbbvars.sh
:skipsh

if exist tbbvars.csh goto skipcsh
echo Generating local tbbvars.csh
echo #!/bin/csh>tbbvars.csh
echo setenv TBBROOT "%actual_root%">>tbbvars.csh
echo setenv TBB_ARCH_PLATFORM "%arch%\%runtime%">>tbbvars.csh
echo setenv TBB_TARGET_ARCH "%arch%">>tbbvars.csh
echo setenv %_INCLUDE% "${TBBROOT}\include;$%_INCLUDE%">>tbbvars.csh
echo setenv %_LIB% "%bin_dir%;$%_LIB%">>tbbvars.csh
echo setenv PATH "%bin_dir%;$PATH">>tbbvars.csh
if not x%UNIXMODE%==x echo setenv LD_LIBRARY_PATH "%bin_dir%;$LD_LIBRARY_PATH">>tbbvars.csh
:skipcsh

REM Workaround for copying Android* specific libgnustl_shared.so library to work folder
if not x%LIB_GNU_STL_ANDROID%==x copy /Y "%LIB_GNU_STL_ANDROID%"\libgnustl_shared.so

endlocal
exit
