/*
    Copyright 2005-2015 Intel Corporation.  All Rights Reserved.

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

// Just the assertion portion of the harness.
// This is useful for writing portions of tests that include
// the minimal number of necessary header files.
//
// The full "harness.h" must be included later.

#ifndef harness_assert_H
#define harness_assert_H

void ReportError( const char* filename, int line, const char* expression, const char* message); 
void ReportWarning( const char* filename, int line, const char* expression, const char* message); 

#define ASSERT_CUSTOM(p,message,file,line)  ((p)?(void)0:ReportError(file,line,#p,message))
#define ASSERT(p,message)                   ASSERT_CUSTOM(p,message,__FILE__,__LINE__)
#define ASSERT_WARNING(p,message)           ((p)?(void)0:ReportWarning(__FILE__,__LINE__,#p,message))

//! Compile-time error if x and y have different types
template<typename T>
void AssertSameType( const T& /*x*/, const T& /*y*/ ) {}

#endif /* harness_assert_H */
