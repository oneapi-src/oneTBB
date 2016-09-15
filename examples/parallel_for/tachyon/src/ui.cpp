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
 * ui.cpp - Contains functions for dealing with user interfaces
 */

#include "machine.h"
#include "types.h"
#include "macros.h"
#include "util.h"
#include "ui.h"

static void (* rt_static_ui_message) (int, const char *) = NULL;
static void (* rt_static_ui_progress) (int) = NULL;
static int (* rt_static_ui_checkaction) (void) = NULL;

extern bool silent_mode;

void set_rt_ui_message(void (* func) (int, const char *)) {
  rt_static_ui_message = func;
}

void set_rt_ui_progress(void (* func) (int)) {
  rt_static_ui_progress = func;
}

void rt_ui_message(int level, const char * msg) {
  if (rt_static_ui_message == NULL) {
    if ( !silent_mode ) {
      fprintf(stderr, "%s\n", msg);
      fflush (stderr);
    }
  } else {
    rt_static_ui_message(level, msg);
  }
}

void rt_ui_progress(int percent) {
  if (rt_static_ui_progress != NULL)
    rt_static_ui_progress(percent);
  else {
    if ( !silent_mode ) {
      fprintf(stderr, "\r %3d%% Complete            \r", percent);
      fflush(stderr);
    }
  }
}

int rt_ui_checkaction(void) {
  if (rt_static_ui_checkaction != NULL) 
    return rt_static_ui_checkaction();
  else
    return 0;
}














