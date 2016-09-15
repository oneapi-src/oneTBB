/*
    Copyright 2005-2016 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks. Threading Building Blocks is free software;
    you can redistribute it and/or modify it under the terms of the GNU General Public License
    version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
    distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See  the GNU General Public License for more details.   You should have received a copy of
    the  GNU General Public License along with Threading Building Blocks; if not, write to the
    Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA

    As a special exception,  you may use this file  as part of a free software library without
    restriction.  Specifically,  if other files instantiate templates  or use macros or inline
    functions from this file, or you compile this file and link it with other files to produce
    an executable,  this file does not by itself cause the resulting executable to be covered
    by the GNU General Public License. This exception does not however invalidate any other
    reasons why the executable file might be covered by the GNU General Public License.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdexcept>

#if _WIN32
#include <io.h>
#ifndef F_OK
#define F_OK 0
#endif
#define access _access
#else
#include <unistd.h>
#endif

const long INPUT_SIZE = 1000000;

//! Generates sample input for square.cpp
void gen_input( const char *fname ) {
    long num = INPUT_SIZE;
    FILE *fptr = fopen(fname, "w");
    if(!fptr) {
        throw std::runtime_error("Could not open file for generating input");
    }

    int a=0;
    int b=1;
    for( long j=0; j<num; ++j ) {
        fprintf(fptr, "%u\n",a);
        b+=a;
        a=(b-a)%10000;
        if (a<0) a=-a;
    }

    if(fptr) {
        fclose(fptr);
    }
}

void generate_if_needed( const char *fname ) {
    if ( access(fname, F_OK) != 0 )
        gen_input(fname);
}
