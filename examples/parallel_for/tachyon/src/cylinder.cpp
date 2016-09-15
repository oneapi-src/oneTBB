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
 * cylinder.cpp - This file contains the functions for dealing with cylinders.
 */
 
#include "machine.h"
#include "types.h"
#include "macros.h"
#include "vector.h"
#include "intersect.h"
#include "util.h"

#define CYLINDER_PRIVATE 
#include "cylinder.h"

static object_methods cylinder_methods = {
  (void (*)(void *, void *))(cylinder_intersect),
  (void (*)(void *, void *, void *, void *))(cylinder_normal),
  cylinder_bbox, 
  free 
};

static object_methods fcylinder_methods = {
  (void (*)(void *, void *))(fcylinder_intersect),
  (void (*)(void *, void *, void *, void *))(cylinder_normal),
  fcylinder_bbox, 
  free 
};


object * newcylinder(void * tex, vector ctr, vector axis, flt rad) {
  cylinder * c;
  
  c=(cylinder *) rt_getmem(sizeof(cylinder));
  memset(c, 0, sizeof(cylinder));
  c->methods = &cylinder_methods;

  c->tex=(texture *) tex;
  c->ctr=ctr;
  c->axis=axis;
  c->rad=rad;
  return (object *) c;
}

static int cylinder_bbox(void * obj, vector * min, vector * max) {
  return 0; /* infinite / unbounded object */
}

static void cylinder_intersect(cylinder * cyl, ray * ry) {
  vector rc, n, D, O;  
  flt t, s, tin, tout, ln, d; 

  rc.x = ry->o.x - cyl->ctr.x;
  rc.y = ry->o.y - cyl->ctr.y;
  rc.z = ry->o.z - cyl->ctr.z; 

  VCross(&ry->d, &cyl->axis, &n);

  VDOT(ln, n, n);
  ln=sqrt(ln);    /* finish length calculation */

  if (ln == 0.0) {  /* ray is parallel to the cylinder.. */
    VDOT(d, rc, cyl->axis);         
    D.x = rc.x - d * cyl->axis.x; 
    D.y = rc.y - d * cyl->axis.y;
    D.z = rc.z - d * cyl->axis.z;
    VDOT(d, D, D);
    d = sqrt(d);
    tin = -FHUGE;
    tout = FHUGE;
    /* if (d <= cyl->rad) then ray is inside cylinder.. else outside */
  }

  VNorm(&n);
  VDOT(d, rc, n);
  d = fabs(d); 

  if (d <= cyl->rad) {  /* ray intersects cylinder.. */
    VCross(&rc, &cyl->axis, &O);
    VDOT(t, O, n);
    t = - t / ln;
    VCross(&n, &cyl->axis, &O); 
    VNorm(&O);
    VDOT(s, ry->d, O);
    s = fabs(sqrt(cyl->rad*cyl->rad - d*d) / s);
    tin = t - s;
    add_intersection(tin, (object *) cyl, ry); 
    tout = t + s;
    add_intersection(tout, (object *) cyl, ry);
  }
}

static void cylinder_normal(cylinder * cyl, vector * pnt, ray * incident, vector * N) {
  vector a,b,c;
  flt t;

  VSub((vector *) pnt, &(cyl->ctr), &a);

  c=cyl->axis;

  VNorm(&c);
 
  VDOT(t, a, c);

  b.x = c.x * t + cyl->ctr.x; 
  b.y = c.y * t + cyl->ctr.y;
  b.z = c.z * t + cyl->ctr.z;

  VSub(pnt, &b, N); 
  VNorm(N);

  if (VDot(N, &(incident->d)) > 0.0)  { /* make cylinder double sided */
    N->x=-N->x;
    N->y=-N->y;
    N->z=-N->z;
  } 
}

object * newfcylinder(void * tex, vector ctr, vector axis, flt rad) {
  cylinder * c;
  
  c=(cylinder *) rt_getmem(sizeof(cylinder));
  memset(c, 0, sizeof(cylinder));
  c->methods = &fcylinder_methods;

  c->tex=(texture *) tex;
  c->ctr=ctr;
  c->axis=axis;
  c->rad=rad;

  return (object *) c;
}

static int fcylinder_bbox(void * obj, vector * min, vector * max) {
  cylinder * c = (cylinder *) obj;
  vector mintmp, maxtmp;

  mintmp.x = c->ctr.x;
  mintmp.y = c->ctr.y;
  mintmp.z = c->ctr.z;
  maxtmp.x = c->ctr.x + c->axis.x;
  maxtmp.y = c->ctr.y + c->axis.y;
  maxtmp.z = c->ctr.z + c->axis.z;

  min->x = MYMIN(mintmp.x, maxtmp.x);
  min->y = MYMIN(mintmp.y, maxtmp.y);
  min->z = MYMIN(mintmp.z, maxtmp.z);
  min->x -= c->rad;
  min->y -= c->rad;
  min->z -= c->rad;

  max->x = MYMAX(mintmp.x, maxtmp.x);
  max->y = MYMAX(mintmp.y, maxtmp.y);
  max->z = MYMAX(mintmp.z, maxtmp.z);
  max->x += c->rad;
  max->y += c->rad;
  max->z += c->rad;

  return 1;
}


static void fcylinder_intersect(cylinder * cyl, ray * ry) {
  vector rc, n, O, hit, tmp2, ctmp4;
  flt t, s, tin, tout, ln, d, tmp, tmp3;
 
  rc.x = ry->o.x - cyl->ctr.x;  
  rc.y = ry->o.y - cyl->ctr.y;
  rc.z = ry->o.z - cyl->ctr.z;
 
  VCross(&ry->d, &cyl->axis, &n);
 
  VDOT(ln, n, n);
  ln=sqrt(ln);    /* finish length calculation */
 
  if (ln == 0.0) {  /* ray is parallel to the cylinder.. */
    return;       /* in this case, we want to miss or go through the "hole" */
  }
 
  VNorm(&n);
  VDOT(d, rc, n);
  d = fabs(d);
 
  if (d <= cyl->rad) {  /* ray intersects cylinder.. */
    VCross(&rc, &cyl->axis, &O);
    VDOT(t, O, n);
    t = - t / ln;
    VCross(&n, &cyl->axis, &O);
    VNorm(&O);
    VDOT(s, ry->d, O);
    s = fabs(sqrt(cyl->rad*cyl->rad - d*d) / s);
    tin = t - s;

    RAYPNT(hit, (*ry), tin); 

    ctmp4=cyl->axis;
    VNorm(&ctmp4);

    tmp2.x = hit.x - cyl->ctr.x;   
    tmp2.y = hit.y - cyl->ctr.y;   
    tmp2.z = hit.z - cyl->ctr.z;   

    VDOT(tmp,  tmp2, ctmp4);
    VDOT(tmp3, cyl->axis, cyl->axis);

    if ((tmp > 0.0) && (tmp < sqrt(tmp3))) 
      add_intersection(tin, (object *) cyl, ry);
    tout = t + s;

    RAYPNT(hit, (*ry), tout); 

    tmp2.x = hit.x - cyl->ctr.x;   
    tmp2.y = hit.y - cyl->ctr.y;   
    tmp2.z = hit.z - cyl->ctr.z;   

    VDOT(tmp,  tmp2, ctmp4); 
    VDOT(tmp3, cyl->axis, cyl->axis);

    if ((tmp > 0.0) && (tmp < sqrt(tmp3))) 
      add_intersection(tout, (object *) cyl, ry);
  }
}

