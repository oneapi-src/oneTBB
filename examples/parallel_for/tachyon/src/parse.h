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
 * parse.h - this file contains defines for model file reading.
 *
 *  $Id: parse.h,v 1.2 2007-02-22 17:54:16 Exp $
 */

#define PARSENOERR       0
#define PARSEBADFILE     1
#define PARSEBADSUBFILE  2
#define PARSEBADSYNTAX   4
#define PARSEEOF         8
#define PARSEALLOCERR    16
 
unsigned int readmodel(char *, SceneHandle);

#ifdef PARSE_INTERNAL
#define NUMTEXS 32768
#define TEXNAMELEN 24

typedef struct {
   double rx1; double rx2; double rx3;
   double ry1; double ry2; double ry3;
   double rz1; double rz2; double rz3;
} RotMat;

typedef struct {
        char name[TEXNAMELEN];
        void * tex;
} texentry;

#ifdef _ERRCODE_DEFINED
#define errcode errcode_t
#endif//_ERRCODE_DEFINED
typedef unsigned int errcode;

static errcode add_texture(void * tex, char name[TEXNAMELEN]);
static errcode GetString(FILE *, const char *);
static errcode GetScenedefs(FILE *, SceneHandle);
static errcode GetColor(FILE *, color *);
static errcode GetVector(FILE *, vector *);
static errcode GetTexDef(FILE *);
static errcode GetTexAlias(FILE *);
static errcode GetTexture(FILE *, void **);
void * GetTexBody(FILE *);
static errcode GetBackGnd(FILE *);
static errcode GetCylinder(FILE *);
static errcode GetFCylinder(FILE *);
static errcode GetPolyCylinder(FILE *);
static errcode GetSphere(FILE *);
static errcode GetPlane(FILE *);
static errcode GetRing(FILE *);
static errcode GetBox(FILE *);
static errcode GetVol(FILE *);
static errcode GetTri(FILE *);
static errcode GetSTri(FILE *);
static errcode GetLight(FILE *);
static errcode GetLandScape(FILE *);
static errcode GetTPolyFile(FILE *);
static errcode GetMGFFile(FILE *, SceneHandle);
static errcode GetObject(FILE *, SceneHandle);

#endif
