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

#include "pover_global.h"  // for declaration of DEFINE and INIT

DEFINE Polygon_map_t *gPolymap1 INIT(0);
DEFINE Polygon_map_t *gPolymap2 INIT(0);
DEFINE Polygon_map_t *gResultMap INIT(0);

extern void Usage(int argc, char **argv);

extern bool ParseCmdLine(int argc, char **argv );

extern bool GenerateMap(Polygon_map_t **newMap, int xSize, int ySize, int gNPolygons, colorcomp_t maxR, colorcomp_t maxG, colorcomp_t maxB);

extern bool PolygonsOverlap(RPolygon *p1, RPolygon *p2, int &xl, int &yl, int &xh, int &yh);

extern void CheckPolygonMap(Polygon_map_t *checkMap);

extern bool CompOnePolygon(RPolygon *p1, RPolygon *p2);

extern bool PolygonsEqual(RPolygon *p1, RPolygon *p2);

extern bool ComparePolygonMaps(Polygon_map_t *map1, Polygon_map_t *map2);

extern void SetRandomSeed(int newSeed);

extern int NextRan(int n);
