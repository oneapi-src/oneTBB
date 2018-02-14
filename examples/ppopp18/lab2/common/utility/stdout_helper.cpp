/*
  Copyright (c) 2018 Intel Corporation

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

/**
 * stdout_helper.cpp
 * This file defines some of the methods of the stdout_helper class defined in
 * stdout_helper.h.
 */

#include "stdout_helper.h"

// constructor
// input: absolute path for placing the output-redirection files
// sets some default values

stdout_helper::stdout_helper(const char* dirPath):
gotErr(0),
stdout_fname(dirPath), stderr_fname(dirPath),
out_fptr(NULL), err_fptr(NULL),
outStr(NULL), errStr(NULL) 
{
    stdout_fname.append("/stdout.txt");
    stderr_fname.append("/stderr.txt");

    // try to redirect stdout to dirPath/stdout.txt 
    // The freopen function opens the file name in the specified mode
    // and associates the stream (stdout here) with this file. It also
    // closes stdout, if open.

    out_fptr = freopen(stdout_fname.c_str(), "w+", stdout);

    if(!out_fptr) {
        gotErr = 1;
        errMsg = std::string("File open error\n");
    }
    else {  // successfully opened file for stdout redirection
        // try redirecting stderr to dirPath/stderr
        err_fptr = freopen(stderr_fname.c_str(), "w+", stderr);
        if(!err_fptr) {
            gotErr = 1;
            errMsg = std::string("File open error\n");
        }
    }
}


// Read the contents of stdout and stderr from the files.
// Read the stdout file only if the stderr file is empty.
const char *stdout_helper::getProgramOutput() {
    const char *retStr = read_file(err_fptr, &errStr);
    if(!errStr) {
        retStr = read_file(out_fptr, &outStr);
    }
    return retStr;
}


// For the file pointed to by fptr, determine its file size, allocate
// rdbuff based on file size, and read the contents of the file into
// the buffer.
const char *stdout_helper::read_file(FILE *fptr, char **rdbuff) 
{
    fseek(fptr, 0, SEEK_END);
    long fileSize = ftell(fptr);
    if(-1 == fileSize) {
        return "filesize failure\n";
    }
    if(fileSize > 0) {
        *rdbuff = new char[fileSize];
        if(!(*rdbuff)) {
            return "heap allocation error\n";
        }
        rewind(fptr);
        fread(*rdbuff, sizeof(char), fileSize, fptr);
        return *rdbuff;
    }

    return "<no error or output>\n";
}
