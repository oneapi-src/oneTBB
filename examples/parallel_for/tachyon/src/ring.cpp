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
 * ring.cpp - This file contains the functions for dealing with rings.
 */
 
#include "machine.h"
#include "types.h"
#include "macros.h"
#include "vector.h"
#include "intersect.h"
#include "util.h"

#define RING_PRIVATE
#include "ring.h"

static object_methods ring_methods = {
  (void (*)(void *, void *))(ring_intersect),
  (void (*)(void *, void *, void *, void *))(ring_normal),
  ring_bbox, 
  free 
};

object * newring(void * tex, vector ctr, vector norm, flt inrad, flt outrad) {
  ring * r;
  
  r=(ring *) rt_getmem(sizeof(ring));
  memset(r, 0, sizeof(ring));
  r->methods = &ring_methods;

  r->tex = (texture *)tex;
  r->ctr = ctr;
  r->norm = norm;
  r->inrad = inrad;
  r->outrad= outrad;

  return (object *) r;
}

static int ring_bbox(void * obj, vector * min, vector * max) {
  ring * r = (ring *) obj;

  min->x = r->ctr.x - r->outrad;
  min->y = r->ctr.y - r->outrad;
  min->z = r->ctr.z - r->outrad;
  max->x = r->ctr.x + r->outrad;
  max->y = r->ctr.y + r->outrad;
  max->z = r->ctr.z + r->outrad;

  return 1;
}

static void ring_intersect(ring * rng, ray * ry) {
  flt d;
  flt t,td;
  vector hit, pnt;
  
  d = -VDot(&(rng->ctr), &(rng->norm));
   
  t=-(d+VDot(&(rng->norm), &(ry->o)));
  td=VDot(&(rng->norm),&(ry->d)); 
  if (td != 0.0) {
    t= t / td;
    if (t>=0.0) {
      hit=Raypnt(ry, t);
      VSUB(hit, rng->ctr, pnt);
      VDOT(td, pnt, pnt);
      td=sqrt(td);
      if ((td > rng->inrad) && (td < rng->outrad)) 
        add_intersection(t,(object *) rng, ry);
    }
  }
}

static void ring_normal(ring * rng, vector  * pnt, ray * incident, vector * N) {
  *N=rng->norm;
  VNorm(N);
  if (VDot(N, &(incident->d)) > 0.0)  {
    N->x=-N->x;
    N->y=-N->y;
    N->z=-N->z;
  } 
}

