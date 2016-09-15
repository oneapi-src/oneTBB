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

// common Windows parts
#include "winvideo.h"

// and another headers
#include <cassert>
#include <stdio.h>
#include <dxsdkver.h>
#if _DXSDK_PRODUCT_MAJOR < 9
#error DXSDK Version 9 and above required.
#endif
#include <d2d1.h>
#include <d2d1helper.h>
#pragma comment(lib, "d2d1.lib")

ID2D1Factory *m_pD2DFactory;
ID2D1HwndRenderTarget *m_pRenderTarget;
ID2D1Bitmap *m_pBitmap;
D2D1_SIZE_U bitmapSize;

HANDLE g_hVSync;

#include <DXErr.h>
#pragma comment(lib, "DxErr.lib")

//! Create a dialog box and tell the user what went wrong
bool DisplayError(LPSTR lpstrErr, HRESULT hres)
{
    if(hres != S_OK){
        static bool InError = false;
        int retval = 0;
        if (!InError)
        {
            InError = true;
            const char *message = hres?DXGetErrorString(hres):0;
            retval = MessageBoxA(g_hAppWnd, lpstrErr, hres?message:"Error!", MB_OK|MB_ICONERROR);
            InError = false;
        }
    }
    return false;
}

void DrawBitmap()
{
    HRESULT hr = S_OK;
    if (m_pRenderTarget) {
        m_pRenderTarget->BeginDraw();
        if (m_pBitmap) 
            hr = m_pBitmap->CopyFromMemory(NULL,(BYTE*)g_pImg, 4*g_sizex);
        DisplayError( "DrawBitmap error", hr );
        m_pRenderTarget->DrawBitmap(m_pBitmap);
        m_pRenderTarget->EndDraw();
    }
    return;
}

inline void mouse(int k, LPARAM lParam)
{
    int x = (int)LOWORD(lParam);
    int y = (int)HIWORD(lParam);
    RECT rc;
    GetClientRect(g_hAppWnd, &rc);
    g_video->on_mouse( x*g_sizex/(rc.right - rc.left), y*g_sizey/(rc.bottom - rc.top), k );
}

//! Win event processing function
LRESULT CALLBACK InternalWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg)
    {
        case WM_MOVE:
            // Check to make sure our window exists before we tell it to repaint.
            // This will fail the first time (while the window is being created).
            if (hwnd) {
                InvalidateRect(hwnd, NULL, FALSE);
                UpdateWindow(hwnd);
            }
            return 0L;

        case WM_SIZE:
        case WM_PAINT:
            if( g_video->running && g_video->updating ) {
                DrawBitmap();
                Sleep(0);
            }
            break;
        // Process all mouse and keyboard events
        case WM_LBUTTONDOWN:    mouse( 1, lParam ); break;
        case WM_LBUTTONUP:      mouse(-1, lParam ); break;
        case WM_RBUTTONDOWN:    mouse( 2, lParam ); break;
        case WM_RBUTTONUP:      mouse(-2, lParam ); break;
        case WM_MBUTTONDOWN:    mouse( 3, lParam ); break;
        case WM_MBUTTONUP:      mouse(-3, lParam ); break;
        case WM_CHAR:           g_video->on_key( (int)wParam); break;

        // some useless stuff
        case WM_ERASEBKGND:     return 1;  // keeps erase-background events from happening, reduces chop
        case WM_DISPLAYCHANGE:  return 0;

        // Now, shut down the window...
        case WM_DESTROY:        PostQuitMessage(0); return 0;
    }
    // call user defined proc, if exists
    return g_pUserProc? g_pUserProc(hwnd, iMsg, wParam, lParam) : DefWindowProc(hwnd, iMsg, wParam, lParam);
}

bool video::init_window(int sizex, int sizey)
{
    assert(win_hInstance != 0);
    g_sizex = sizex; g_sizey = sizey;
    if (!WinInit(win_hInstance, win_iCmdShow, gWndClass, title, false)) {
        DisplayError("Unable to initialize the program's window.");
        return false;
    }
    ShowWindow(g_hAppWnd, SW_SHOW);
    g_pImg = new unsigned int[sizex*sizey];

    HRESULT hr = S_OK;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
    // Create a Direct2D render target.
    if (SUCCEEDED(hr) && !m_pRenderTarget){
        RECT rc;
        GetClientRect(g_hAppWnd, &rc);

        bitmapSize = D2D1::SizeU(
            rc.right - rc.left,
            rc.bottom - rc.top
            );

        hr = m_pD2DFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(g_hAppWnd, bitmapSize),
            &m_pRenderTarget
            );
        if (SUCCEEDED(hr) && !m_pBitmap){
            D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(
                DXGI_FORMAT_B8G8R8A8_UNORM,
                D2D1_ALPHA_MODE_IGNORE
                );
            D2D1_BITMAP_PROPERTIES bitmapProperties;
            bitmapProperties.pixelFormat = pixelFormat;
            m_pRenderTarget->GetDpi( &bitmapProperties.dpiX, &bitmapProperties.dpiY );
            m_pRenderTarget->CreateBitmap(bitmapSize,bitmapProperties,&m_pBitmap);
            m_pRenderTarget->DrawBitmap(m_pBitmap);
        }
    }

    running = true;
    return true;
}

void video::terminate()
{
    if (m_pBitmap) m_pBitmap->Release();
    if (m_pRenderTarget) m_pRenderTarget->Release(); 
    if (m_pD2DFactory) m_pD2DFactory->Release();
    g_video = 0; running = false;
    if(g_pImg) { delete[] g_pImg; g_pImg = 0; }
}

//////////// drawing area constructor & destructor /////////////

drawing_area::drawing_area(int x, int y, int sizex, int sizey)
: start_x(x), start_y(y), size_x(sizex), size_y(sizey), pixel_depth(24),
    base_index(y*g_sizex + x), max_index(g_sizex*g_sizey), index_stride(g_sizex), ptr32(g_pImg)
{
    assert(x < g_sizex); assert(y < g_sizey);
    assert(x+sizex <= g_sizex); assert(y+sizey <= g_sizey);

    index = base_index; // current index
}

void drawing_area::update()
{
    if(g_video->updating) {
        RECT r;
        r.left = start_x; r.right  = start_x + size_x;
        r.top  = start_y; r.bottom = start_y + size_y;
        InvalidateRect(g_hAppWnd, &r, false);
    }
}
