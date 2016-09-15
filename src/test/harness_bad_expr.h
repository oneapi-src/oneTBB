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

// Declarations for checking __TBB_ASSERT checks inside TBB.
// This header is an optional part of the test harness.
// It assumes that "harness.h" has already been included.

#define TRY_BAD_EXPR_ENABLED (TBB_USE_ASSERT && TBB_USE_EXCEPTIONS && !__TBB_THROW_ACROSS_MODULE_BOUNDARY_BROKEN)

#if TRY_BAD_EXPR_ENABLED

//! Check that expression x raises assertion failure with message containing given substring.
/** Assumes that tbb::set_assertion_handler( AssertionFailureHandler ) was called earlier. */
#define TRY_BAD_EXPR(x,substr)          \
    {                                   \
        const char* message = NULL;     \
        bool okay = false;              \
        try {                           \
            x;                          \
        } catch( AssertionFailure a ) { \
            okay = true;                \
            message = a.message;        \
        }                               \
        CheckAssertionFailure(__LINE__,#x,okay,message,substr); \
    }

//! Exception object that holds a message.
struct AssertionFailure {
    const char* message;
    AssertionFailure( const char* filename, int line, const char* expression, const char* comment );
};

AssertionFailure::AssertionFailure( const char* filename, int line, const char* expression, const char* comment ) :
    message(comment)
{
    ASSERT(filename,"missing filename");
    ASSERT(0<line,"line number must be positive");
    // All of our current files have fewer than 4000 lines.
    ASSERT(line<5000,"dubiously high line number");
    ASSERT(expression,"missing expression");
}

void AssertionFailureHandler( const char* filename, int line, const char* expression, const char* comment ) {
    throw AssertionFailure(filename,line,expression,comment);
}

void CheckAssertionFailure( int line, const char* expression, bool okay, const char* message, const char* substr ) {
    if( !okay ) {
        REPORT("Line %d, %s failed to fail\n", line, expression );
        abort();
    } else if( !message ) {
        REPORT("Line %d, %s failed without a message\n", line, expression );
        abort();
    } else if( strstr(message,substr)==0 ) {
        REPORT("Line %d, %s failed with message '%s' missing substring '%s'\n", __LINE__, expression, message, substr );
        abort();
    }
}

#endif /* TRY_BAD_EXPR_ENABLED */
