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
 * stdout_helper.h
 *
 * This file defines the class stdout_helper, which redirects stdout
 * and stderr to files named stdout.txt and stderr.txt.  In order to
 * run the examples on an Android device, this redirection is
 * necessary to capture the output from the examples.  Otherwise,
 * Android applications redirect stdout and stderr to /dev/null, by
 * default.
 */

#include <cstdio>
#include <cstring>
#include <string>

class stdout_helper {
private:
    std::string errMsg;                      // error message, if any
    int gotErr;                              // flag to indicate error
    std::string stdout_fname, stderr_fname;  // file names for stdout and stderr
    FILE *out_fptr, *err_fptr;               // file pointers for stdout and stderr
    char *outStr, *errStr;                   // text from stdout and stderr

    const char *read_file(FILE *, char **);

public:

    stdout_helper(const char* dirPath);

    ~stdout_helper() {
        if(out_fptr) {
            fclose(out_fptr);
        }
        if(err_fptr) {
            fclose(err_fptr);
        }
        if(outStr) {
            delete [] outStr;
        }
        if(errStr) {
            delete [] errStr;
        }
    }

    const char* getErrMsg(void) {
        return errMsg.c_str();
    }

    int inError(void) {
        return gotErr;
    }

    const char *getProgramOutput();
};
