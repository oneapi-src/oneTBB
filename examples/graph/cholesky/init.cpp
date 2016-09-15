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

#include <cstdio>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <mkl_cblas.h>

static void posdef_gen( double * A, int n )
{
    /* Allocate memory for the matrix and its transpose */
    double *L = (double *)calloc( sizeof( double ), n*n );
    assert( L );

    double *LT = (double *)calloc( sizeof( double ), n*n) ;
    assert( LT );

    memset( A, 0, sizeof( double )*n*n );

    /* Generate a conditioned matrix and fill it with random numbers */
    for ( int j = 0; j < n; ++j ) {
        for ( int k = 0; k < j; ++k ) {
            // The initial value has to be between [0,1].
            L[k*n+j] = ( ( (j*k) / ((double)(j+1)) / ((double)(k+2)) * 2.0) - 1.0 ) / ((double)n);
        }

        L[j*n+j] = 1;
    }

    /* Compute transpose of the matrix */
    for ( int i = 0; i < n; ++i ) {
        for ( int j = 0; j < n; ++j ) {
            LT[j*n+i] = L[i*n+j];
        }
    }
    cblas_dgemm( CblasColMajor, CblasNoTrans, CblasNoTrans, n, n, n, 1, L, n, LT, n, 0, A, n );

    free( L );
    free( LT );
}

// Read the matrix from the input file
void matrix_init( double * &A, int &n, const char *fname ) {
    if( fname ) {
        int i;
        int j;
        FILE *fp;

        fp = fopen( fname, "r" );
        if ( fp == NULL ) {
            fprintf( stderr, "\nFile does not exist\n" );
            exit( 0 );
        }
        if ( fscanf( fp, "%d", &n ) <= 0 ) {
            fprintf( stderr,"\nCouldn't read n from %s\n", fname );
            exit( 1 );
        }
        A = (double *)calloc( sizeof( double ), n*n );
        for ( i = 0; i < n; ++i ) {
            for ( j = 0; j <= i; ++j ) {
                if( fscanf( fp, "%lf ", &A[i*n+j] ) <= 0) {
                    fprintf( stderr,"\nMatrix size incorrect %i %i\n", i, j );
                    exit( 1 );
                }
                if ( i != j ) {
                    A[j*n+i] = A[i*n+j];
                }
            }
        }
        fclose( fp );
    } else {
        A = (double *)calloc( sizeof( double ), n*n );
        posdef_gen( A, n );
    }
}

// write matrix to file
void matrix_write ( double *A, int n, const char *fname, bool is_triangular = false )
{
    if( fname ) {
        int i = 0;
        int j = 0;
        FILE *fp = NULL;

        fp = fopen( fname, "w" );
        if ( fp == NULL ) {
            fprintf( stderr, "\nCould not open file %s for writing.\n", fname );
            exit( 0 );
        }
        fprintf( fp, "%d\n", n );
        for ( i = 0; i < n; ++i) {
            for ( j = 0; j <= i; ++j ) {
                fprintf( fp, "%lf ", A[j*n+i] );
            }
            if ( !is_triangular ) {
                for ( ; j < n; ++j ) {
                    fprintf( fp, "%lf ", A[i*n+j] );
                }
            } else {
                for ( ; j < n; ++j ) {
                    fprintf( fp, "%lf ", 0.0 );
                }
            } 
            fprintf( fp, "\n" );
        }
        if ( is_triangular ) {
            fprintf( fp, "\n" );
            for ( i = 0; i < n; ++i ) {
                for ( j = 0; j < i; ++j ) {
                    fprintf( fp, "%lf ", 0.0 );
                }
                for ( ; j < n; ++j ) {
                    fprintf( fp, "%lf ", A[i*n+j] );
                }
                fprintf( fp, "\n" );
            }
        }
        fclose( fp );
    }
}
