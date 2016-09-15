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

// rpolygon.h
//
#ifndef _RPOLYGON_H_
#define _RPOLYGON_H_
#include <vector>
#include <iostream>
#include "pover_video.h"

#include "tbb/scalable_allocator.h"
#include "tbb/concurrent_vector.h"
#include "tbb/enumerable_thread_specific.h"

using namespace std;

using namespace tbb;

class RPolygon;
typedef scalable_allocator<RPolygon> RPolygon_allocator;
DEFINE RPolygon_allocator rAlloc;

enum MallocBehavior {
    UseMalloc,
    UseScalableAllocator
};

DEFINE MallocBehavior gMBehavior INIT(UseScalableAllocator);

class RPolygon {
public:
    RPolygon() {m_XMin = m_YMin = m_XMax = m_YMax = 0;
        m_r = m_g = m_b = 0;
    }
    RPolygon(int xMin, int yMin, int xMax, int yMax, int r=-1, int g=-1, int b=-1) : m_XMin(xMin), m_YMin(yMin), m_XMax(xMax), m_YMax(yMax) {
        if( r >= 0) {
            m_r=(colorcomp_t)r; m_g=(colorcomp_t)g; m_b=(colorcomp_t)b;
                        if(gDoDraw) drawPoly();
        }
    }

    void set_nodraw(int xMin, int yMin, int xMax, int yMax) {m_XMin=xMin; m_YMin=yMin; m_XMax=xMax; m_YMax=yMax;}

    RPolygon &intersect(RPolygon &otherPoly);
    void set(int xMin, int yMin, int xMax, int yMax) {
         set_nodraw(xMin,yMin,xMax,yMax);
         if(gDoDraw) {
            drawPoly();
         }
    }
    void get(int *xMin, int *yMin, int *xMax, int *yMax) const {*xMin=m_XMin;*yMin=m_YMin;*xMax=m_XMax;*yMax=m_YMax;}
    int xmax() const { return m_XMax; }
    int xmin() const { return m_XMin; }
    int ymax() const { return m_YMax; }
    int ymin() const { return m_YMin; }
    void setColor(colorcomp_t newr, colorcomp_t newg, colorcomp_t newb) {m_r = newr; m_g=newg; m_b=newb;}
    void getColor(int *myr, int *myg, int *myb) {*myr=m_r; *myg=m_g; *myb=m_b;}
    color_t myColor() {return gVideo->get_color(m_r, m_g, m_b);}
    void drawPoly() {
        if(gVideo->running) {
            if(g_next_frame()) {    // Shouldn't call next_frame each time
                drawing_area ldrawing(
                    gDrawXOffset+m_XMin*gPolyXBoxSize,         //x
                    gDrawYOffset+m_YMin*gPolyYBoxSize,         //y
                    (m_XMax-m_XMin+1)*gPolyXBoxSize,           //sizex
                    (m_YMax-m_YMin+1)*gPolyYBoxSize);          //sizey
                for(int y=0; y<ldrawing.size_y; y++) {
                    ldrawing.set_pos(0,y);
                    color_t my_color = myColor();
                    for(int x=0;x < ldrawing.size_x; x++) {
                         ldrawing.put_pixel(my_color);
                    }
                }
            }
        }
    }

    int  area() {return ((m_XMax-m_XMin+1)*(m_YMax-m_YMin+1));}
    void print(int i) { cout << "RPolygon " << i << " (" << m_XMin << ", " << m_YMin << ")-(" << m_XMax << ", " << m_YMax << ") " << endl; fflush(stdout);}
private:
    int m_XMin;
    int m_YMin;
    int m_XMax;
    int m_YMax;
    colorcomp_t m_r;
    colorcomp_t m_g;
    colorcomp_t m_b;
};

#if _MAIN_C_
bool operator<(const RPolygon& a, const RPolygon& b) {
    if(a.ymin() > b.ymin()) return false;
    if(a.ymin() < b.ymin()) return true;
    return a.xmin() < b.xmin();
}
#else
extern bool operator<(const RPolygon& a, const RPolygon& b);
#endif

extern ostream& operator<<(ostream& s, const RPolygon &p);

class RPolygon_flagged {
    RPolygon *myPoly;
    bool is_duplicate;
public:
    RPolygon_flagged() {myPoly = NULL; is_duplicate = false;}
    RPolygon_flagged(RPolygon* _p, bool _is_duplicate) : myPoly(_p), is_duplicate(_is_duplicate) { }
    bool isDuplicate() {return is_duplicate;}
    void setDuplicate(bool newValue) {is_duplicate = newValue;}
    RPolygon *p() {return myPoly;}
    void setp(RPolygon *newp) {myPoly = newp;}
};

typedef class vector<RPolygon, RPolygon_allocator> Polygon_map_t;
typedef class concurrent_vector<RPolygon, RPolygon_allocator> concurrent_Polygon_map_t;
typedef class enumerable_thread_specific<Polygon_map_t> ETS_Polygon_map_t;
typedef class vector<RPolygon_flagged, scalable_allocator<RPolygon_flagged> > Flagged_map_t; // we'll make shallow copies

inline bool PolygonsOverlap(RPolygon *p1, RPolygon *p2, int &xl, int &yl, int &xh, int &yh) {
    int xl1, yl1, xh1, yh1, xl2, yl2, xh2, yh2;
#if _DEBUG
     rt_sleep(1);   // slow down the process so we can see it.
#endif
    p1->get(&xl1, &yl1, &xh1, &yh1);
    p2->get(&xl2, &yl2, &xh2, &yh2);
    if(xl1 > xh2) return false;
    if(xh1 < xl2) return false;
    if(yl1 > yh2) return false;
    if(yh1 < yl2) return false;
    xl = (xl1 < xl2) ? xl2 : xl1;
    xh = (xh1 < xh2) ? xh1 : xh2;
    yl = (yl1 < yl2) ? yl2 : yl1;
    yh = (yh1 < yh2) ? yh1 : yh2;
    return true;
}

#endif // _RPOLYGON_H_
