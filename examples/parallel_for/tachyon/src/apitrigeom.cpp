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

/*
    The original source for this example is
    Copyright (c) 1994-2008 John E. Stone
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. The name of the author may not be used to endorse or promote products
       derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
    OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/

/*
 * apitrigeom.cpp - This file contains code for generating triangle tesselated
 *                geometry, for use with OpenGL, XGL, etc.
 */

#include "machine.h"
#include "types.h"
#include "api.h"
#include "macros.h"
#include "vector.h"

#define MyVNorm(a)		VNorm ((vector *) a)
#define MyVCross(a,b,c)		VCross ((vector *) a, (vector *) b, (vector *) c)
#define MyVAddS(x,a,b,c)	VAddS ((flt) x, (vector *) a, (vector *) b, (vector *) c)

#define CYLFACETS 36
#define RINGFACETS 36
#define SPHEREFACETS 25

void rt_tri_fcylinder(void * tex, vector ctr, vector axis, apiflt rad) {
  vector x, y, z, tmp;
  double u, v, u2, v2;
  int j;
  vector p1, p2, p3, p4;
  vector n1, n2;

  z = axis;
  MyVNorm(&z);
  tmp.x = z.y - 2.1111111;
  tmp.y = -z.z + 3.14159267;
  tmp.z = z.x - 3.915292342341;
  MyVNorm(&z);
  MyVNorm(&tmp);
  MyVCross(&z, &tmp, &x);
  MyVNorm(&x);
  MyVCross(&x, &z, &y);
  MyVNorm(&y);

  for (j=0; j<CYLFACETS; j++) {
     u = rad * sin((6.28 * j) / (CYLFACETS - 1.0));
     v = rad * cos((6.28 * j) / (CYLFACETS - 1.0));
    u2 = rad * sin((6.28 * (j + 1.0)) / (CYLFACETS - 1.0));
    v2 = rad * cos((6.28 * (j + 1.0)) / (CYLFACETS - 1.0));

    p1.x = p1.y = p1.z = 0.0;
    p4 = p3 = p2 = p1;

    MyVAddS(u, &x, &p1, &p1);
    MyVAddS(v, &y, &p1, &p1);
    n1 = p1;
    MyVNorm(&n1);
    MyVAddS(1.0, &ctr, &p1, &p1);
  

    MyVAddS(u2, &x, &p2, &p2);
    MyVAddS(v2, &y, &p2, &p2);
    n2 = p2;
    MyVNorm(&n2);
    MyVAddS(1.0, &ctr, &p2, &p2);

    MyVAddS(1.0, &axis, &p1, &p3);
    MyVAddS(1.0, &axis, &p2, &p4);

    rt_stri(tex, p1, p2, p3, n1, n2, n1);
    rt_stri(tex, p3, p2, p4, n1, n2, n2);
  }
}

void rt_tri_cylinder(void * tex, vector ctr, vector axis, apiflt rad) {
  rt_fcylinder(tex, ctr, axis, rad);
}

void rt_tri_ring(void * tex, vector ctr, vector norm, apiflt a, apiflt b) {
  vector x, y, z, tmp;
  double u, v, u2, v2;
  int j;
  vector p1, p2, p3, p4;
  vector n1, n2;

  z = norm;
  MyVNorm(&z);
  tmp.x = z.y - 2.1111111;
  tmp.y = -z.z + 3.14159267;
  tmp.z = z.x - 3.915292342341;
  MyVNorm(&z);
  MyVNorm(&tmp);
  MyVCross(&z, &tmp, &x);
  MyVNorm(&x);
  MyVCross(&x, &z, &y);
  MyVNorm(&y);

  for (j=0; j<RINGFACETS; j++) {
     u = sin((6.28 * j) / (RINGFACETS - 1.0));
     v = cos((6.28 * j) / (RINGFACETS - 1.0));
    u2 = sin((6.28 * (j + 1.0)) / (RINGFACETS - 1.0));
    v2 = cos((6.28 * (j + 1.0)) / (RINGFACETS - 1.0));

    p1.x = p1.y = p1.z = 0.0;
    p4 = p3 = p2 = p1;

    MyVAddS(u, &x, &p1, &p1);
    MyVAddS(v, &y, &p1, &p1);
    n1 = p1;
    MyVNorm(&n1);
    MyVAddS(a, &n1, &ctr, &p1);
    MyVAddS(b, &n1, &ctr, &p3);

    MyVAddS(u2, &x, &p2, &p2);
    MyVAddS(v2, &y, &p2, &p2);
    n2 = p2;
    MyVNorm(&n2);
    MyVAddS(a, &n2, &ctr, &p2);
    MyVAddS(b, &n2, &ctr, &p4);

    rt_stri(tex, p1, p2, p3, norm, norm, norm);
    rt_stri(tex, p3, p2, p4, norm, norm, norm);

  }
} 

void rt_tri_box(void * tex, vector min, vector max) {
  /* -XY face */
  rt_tri(tex, rt_vector(min.x, min.y, min.z),
              rt_vector(min.x, max.y, min.z), 
              rt_vector(max.x, max.y, min.z));
  rt_tri(tex, rt_vector(min.x, min.y, min.z),
              rt_vector(max.x, max.y, min.z), 
              rt_vector(max.x, min.y, min.z));

  /* +XY face */
  rt_tri(tex, rt_vector(min.x, min.y, max.z),
              rt_vector(max.x, max.y, max.z),
              rt_vector(min.x, max.y, max.z)); 
  rt_tri(tex, rt_vector(min.x, min.y, max.z),
              rt_vector(max.x, min.y, max.z),
              rt_vector(max.x, max.y, max.z)); 

  /* -YZ face */
  rt_tri(tex, rt_vector(min.x, min.y, min.z),
              rt_vector(min.x, max.y, max.z),
              rt_vector(min.x, min.y, max.z)); 
  rt_tri(tex, rt_vector(min.x, min.y, min.z),
              rt_vector(min.x, max.y, min.z),
              rt_vector(min.x, max.y, max.z)); 

  /* +YZ face */
  rt_tri(tex, rt_vector(max.x, min.y, min.z),
              rt_vector(max.x, min.y, max.z),
              rt_vector(max.x, max.y, max.z));
  rt_tri(tex, rt_vector(max.x, min.y, min.z),
              rt_vector(max.x, max.y, max.z),
              rt_vector(max.x, max.y, min.z));

  /* -XZ face */
  rt_tri(tex, rt_vector(min.x, min.y, min.z),
              rt_vector(min.x, min.y, max.z), 
              rt_vector(max.x, min.y, max.z));
  rt_tri(tex, rt_vector(min.x, min.y, min.z),
              rt_vector(max.x, min.y, max.z), 
              rt_vector(max.x, min.y, min.z));

  /* +XZ face */
  rt_tri(tex, rt_vector(min.x, max.y, min.z),
              rt_vector(max.x, max.y, max.z),
              rt_vector(min.x, max.y, max.z)); 
  rt_tri(tex, rt_vector(min.x, max.y, min.z),
              rt_vector(max.x, max.y, min.z),
              rt_vector(max.x, max.y, max.z)); 
}

void rt_tri_sphere(void * tex, vector ctr, apiflt rad) {
}

void rt_tri_plane(void * tex, vector ctr, vector norm) {
  rt_tri_ring(tex, ctr, norm, 0.0, 10000.0);
} 

