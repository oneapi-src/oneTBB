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

#include "seismic_video.h"
#include "universe.h"
#include "tbb/task_scheduler_init.h"

const char * const SeismicVideo::titles[2] = {"Seismic Simulation: Serial", "Seismic Simulation: Parallel"};
void SeismicVideo::on_mouse(int x, int y, int key) {
    if(key == 1){
        u_.TryPutNewPulseSource(x,y);
    }
}

void SeismicVideo::on_key(int key) {
    key &= 0xff;
    if(char(key) == ' ') initIsParallel = !initIsParallel;
    else if(char(key) == 'p') initIsParallel = true;
    else if(char(key) == 's') initIsParallel = false;
    else if(char(key) == 'e') updating = true;
    else if(char(key) == 'd') updating = false;
    else if(key == 27) running = false;
    title = titles[initIsParallel?1:0];
}

void SeismicVideo::on_process() {
    tbb::task_scheduler_init Init(threadsHigh);
    for( int frames = 0; numberOfFrames_==0 || frames<numberOfFrames_; ++frames ) {
        if( initIsParallel )
            u_.ParallelUpdateUniverse();
        else
            u_.SerialUpdateUniverse();
        if( !next_frame() ) break;
    }
}

#ifdef _WINDOWS
#include "msvs/resource.h"
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
SeismicVideo * gVideo = NULL;
#endif

SeismicVideo::SeismicVideo(    Universe &u, int number_of_frames, int threads_high, bool init_is_parallel)
    :numberOfFrames_(number_of_frames),initIsParallel(init_is_parallel),u_(u),threadsHigh(threads_high)
{
    title = titles[initIsParallel?1:0];
#ifdef _WINDOWS
    gVideo = this;
    LoadStringA(video::win_hInstance, IDC_SEISMICSIMULATION, szWindowClass, MAX_LOADSTRING);
    memset(&wcex, 0, sizeof(wcex));
    wcex.lpfnWndProc    = (WNDPROC)WndProc;
    wcex.hIcon          = LoadIcon(video::win_hInstance, MAKEINTRESOURCE(IDI_SEISMICSIMULATION));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = LPCTSTR(IDC_SEISMICSIMULATION);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(video::win_hInstance, MAKEINTRESOURCE(IDI_SMALL));
    win_set_class(wcex); // ascii convention here
    win_load_accelerators(IDC_SEISMICSIMULATION);
#endif

}





#ifdef _WINDOWS
//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG: return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
    return FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId;
    switch (message) {
    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(video::win_hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, (DLGPROC)About);
            break;
        case IDM_EXIT:
            PostQuitMessage(0);
            break;
        case ID_FILE_PARALLEL:
            gVideo->on_key('p');
            break;
        case ID_FILE_SERIAL:
            gVideo->on_key('s');
            break;
        case ID_FILE_ENABLEGUI:
            gVideo->on_key('e');
            break;
        case ID_FILE_DISABLEGUI:
            gVideo->on_key('d');
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

#endif
