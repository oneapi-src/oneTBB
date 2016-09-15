// Copyright (c) 2005-2016 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//
//
//

var WshShell = WScript.CreateObject("WScript.Shell");

var tmpExec;

WScript.Echo("#define __TBB_VERSION_STRINGS(N) \\");

//Getting BUILD_HOST
WScript.echo( "#N \": BUILD_HOST\\t\\t" + 
              WshShell.ExpandEnvironmentStrings("%COMPUTERNAME%") +
              "\" ENDL \\" );

//Getting BUILD_OS
tmpExec = WshShell.Exec("cmd /c ver");
while ( tmpExec.Status == 0 ) {
    WScript.Sleep(100);
}
tmpExec.StdOut.ReadLine();

WScript.echo( "#N \": BUILD_OS\\t\\t" + 
              tmpExec.StdOut.ReadLine() +
              "\" ENDL \\" );

if ( WScript.Arguments(0).toLowerCase().match("gcc") ) {
    tmpExec = WshShell.Exec(WScript.Arguments(0) + " --version");
    WScript.echo( "#N \": BUILD_GCC\\t\\t" + 
                  tmpExec.StdOut.ReadLine() + 
                  "\" ENDL \\" );

} else if ( WScript.Arguments(0).toLowerCase().match("clang") ) {
    tmpExec = WshShell.Exec(WScript.Arguments(0) + " --version");
    WScript.echo( "#N \": BUILD_CLANG\\t" + 
                  tmpExec.StdOut.ReadLine() + 
                  "\" ENDL \\" );

} else { // MS / Intel compilers
    //Getting BUILD_CL
    tmpExec = WshShell.Exec("cmd /c echo #define 0 0>empty.cpp");
    tmpExec = WshShell.Exec("cl -c empty.cpp ");
    while ( tmpExec.Status == 0 ) {
        WScript.Sleep(100);
    }
    var clVersion = tmpExec.StdErr.ReadLine();
    WScript.echo( "#N \": BUILD_CL\\t\\t" + 
                  clVersion +
                  "\" ENDL \\" );

    //Getting BUILD_COMPILER
    if ( WScript.Arguments(0).toLowerCase().match("icl") ) {
        tmpExec = WshShell.Exec("icl -c empty.cpp ");
        while ( tmpExec.Status == 0 ) {
            WScript.Sleep(100);
        }
        WScript.echo( "#N \": BUILD_COMPILER\\t" + 
                      tmpExec.StdErr.ReadLine() + 
                      "\" ENDL \\" );
    } else {
        WScript.echo( "#N \": BUILD_COMPILER\\t\\t" + 
                      clVersion +
                      "\" ENDL \\" );
    }
    tmpExec = WshShell.Exec("cmd /c del /F /Q empty.obj empty.cpp");
}

//Getting BUILD_TARGET
WScript.echo( "#N \": BUILD_TARGET\\t" + 
              WScript.Arguments(1) + 
              "\" ENDL \\" );

//Getting BUILD_COMMAND
WScript.echo( "#N \": BUILD_COMMAND\\t" + WScript.Arguments(2) + "\" ENDL" );

//Getting __TBB_DATETIME and __TBB_VERSION_YMD
var date = new Date();
WScript.echo( "#define __TBB_DATETIME \"" + date.toUTCString() + "\"" );
WScript.echo( "#define __TBB_VERSION_YMD " + date.getUTCFullYear() + ", " + 
              (date.getUTCMonth() > 8 ? (date.getUTCMonth()+1):("0"+(date.getUTCMonth()+1))) + 
              (date.getUTCDate() > 9 ? date.getUTCDate():("0"+date.getUTCDate())) );


/*

Original strings

#define __TBB_VERSION_STRINGS \
"TBB: BUILD_HOST\t\tvpolin-mobl1 (ia32)" ENDL \
"TBB: BUILD_OS\t\tMicrosoft Windows XP [Version 5.1.2600]" ENDL \
"TBB: BUILD_CL\t\tMicrosoft (R) 32-bit C/C++ Optimizing Compiler Version 13.10.3077 for 80x86" ENDL \
"TBB: BUILD_COMPILER\tIntel(R) C++ Compiler for 32-bit applications, Version 9.1 Build 20070109Z Package ID: W_CC_C_9.1.034 " ENDL \
"TBB: BUILD_TARGET\t" ENDL \
"TBB: BUILD_COMMAND\t" ENDL \

#define __TBB_DATETIME "Mon Jun 4 10:16:07 UTC 2007"
#define __TBB_VERSION_YMD 2007, 0604



# The script must be run from two directory levels below this level.
x='"TBB: '
y='" ENDL \'
echo "#define __TBB_VERSION_STRINGS \\"
echo $x "BUILD_HOST\t\t"`hostname`" ("`../../arch.exe`")"$y
echo $x "BUILD_OS\t\t"`../../win_version.bat|grep -i 'Version'`$y
echo >empty.cpp
echo $x "BUILD_CL\t\t"`cl -c empty.cpp 2>&1 | grep -i Version`$y
echo $x "BUILD_COMPILER\t"`icl -c empty.cpp 2>&1 | grep -i Version`$y
echo $x "BUILD_TARGET\t"$TBB_ARCH$y
echo $x "BUILD_COMMAND\t"$*$y
echo ""
# A workaround for MKS 8.6 where `date -u` crashes.
date -u > date.tmp
echo "#define __TBB_DATETIME \""`cat date.tmp`"\""
echo "#define __TBB_VERSION_YMD "`date '+%Y, %m%d'`
rm empty.cpp
rm empty.obj
rm date.tmp
*/
