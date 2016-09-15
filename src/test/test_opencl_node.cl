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

static size_t my_strlen( __constant char* str ) {
    size_t len = 0;
    if ( str ) while ( *str++ ) ++len;
    return len;
}

static void my_strcpy(__global char *dest, __constant char *src, size_t n) {
    while ( n-- ) *dest++ = *src++;
}

static void set_error_msg( __global char *error_msg, size_t error_msg_size, __constant char *msg ) {
    const size_t msg_len = my_strlen(msg);
    const size_t len  = msg_len < error_msg_size ? msg_len : error_msg_size-1;
    my_strcpy( error_msg, msg, len );
    error_msg[len] = 0;
}

__kernel
void TestArgumentPassing( 
    __global int *b1, __global int *b2, int stride_x, int stride_y, int stride_z, int dim, __global char *error_msg, int error_msg_size ) {
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int z = get_global_id(2);
    
    if ( dim < 2 ) {
        if ( y != 0 ) {
            set_error_msg( error_msg, (size_t)error_msg_size, "Y dimension does not equal 0" );
            return;
        }
        if ( stride_y != 0 ) {
            set_error_msg( error_msg, (size_t)error_msg_size, "stride_y does not equal 0" );
            return;
        }
    }
    
    if ( dim < 3 ) {
        if ( z != 0 ) {
            set_error_msg( error_msg, (size_t)error_msg_size, "Z dimension does not equal 0" );
            return;
        }
        if ( stride_z != 0 ) {
            set_error_msg( error_msg, (size_t)error_msg_size, "stride_z does not equal 0" );
            return;
        }
    }
    
    const int index = x*stride_x+y*stride_y+z*stride_z;
    b2[index] = b1[index];
    
    set_error_msg( error_msg, (size_t)error_msg_size, "Done" );
}

__kernel
void Sum(
       __global float *b1, __global float *b2 )
{
    const int index = get_global_id(0);
    b2[index] += b1[index];
}

__kernel
void Mul(
       __global int *b1, __global int *b2 )
{
    const int index = get_global_id(0);
    b1[index] *= b2[index];
}

__kernel
void Sqr(
    __global float *b2, __global float *b3   )
{
    const int index = get_global_id(0);
    b3[index] = b2[index]*b2[index];
}

__kernel
void DiamondDependencyTestFill(
    __global short *b, short v )
{
    const int index = get_global_id(0);
    b[index] = v;
}

__kernel
void DiamondDependencyTestSquare(
    __global short *b1, __global int *b2 )
{
    const int index = get_global_id(0);
    b2[index] = b1[index]*b1[index];
}

__kernel
void DiamondDependencyTestCube(
    __global short *b1, __global int *b2 )
{
    const int index = get_global_id(0);
    b2[index] = b1[index]*b1[index]*b1[index];
}

__kernel
void DiamondDependencyTestDivision(
    __global short *b, __global int *b1, __global int *b2 )
{
    const int index = get_global_id(0);
    b[index] *= b2[index]/b1[index];
}

__kernel
void LoopTestIter( __global long *b1, __global long *b2 ) {
    const int index = get_global_id(0);
    b1[index] += b2[index]++;
}

__kernel
void ConcurrencyTestIter( __global char *b1, __global short *b2 ) {
    const int index = get_global_id(0);
    b2[index] += b1[index];
}

__kernel
void BroadcastTest( __global int *b1, __global int *b2 ) {
    const int index = get_global_id(0);
    b2[index] = b1[index];
}

#if __IMAGE_SUPPORT__
const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;

__kernel
void Image2dTest (
    __read_only image2d_t src,
    __write_only image2d_t dst,
    char type)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    int2 coord = (int2)(x, y);
    switch ( type ) { 
    case 'f':
        write_imagef(dst, coord, read_imagef( src, sampler, coord )) ;
        break;
    case 'i':
        write_imagei(dst, coord, read_imagei( src, sampler, coord )) ;
        break;
    case 'u':
        write_imageui(dst, coord, read_imageui( src, sampler, coord )) ;
        break;
    }

}

__kernel
void Image2dTestDepth (
#if __OPENCL_VERSION__ >= 200
    __read_only image2d_depth_t src,
    __write_only image2d_depth_t dst,
#else
    __read_only image2d_t src,
    __write_only image2d_t dst,
#endif
    char type )
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    int2 coord = (int2)(x, y);
    write_imagef(dst, coord, read_imagef( src, sampler, coord )) ;
}
#endif /* __IMAGE_SUPPORT__ */
