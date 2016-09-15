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


#include "pch.h"
#include "DirectXPage.xaml.h"
#include "tbb/tbb.h"

using namespace tbbTachyon;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Input;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Graphics::Display;

#include "src/tachyon_video.h"
extern int volatile global_number_of_threads;
extern volatile bool global_isCancelled;

#pragma intrinsic(_BitScanReverse)
static int log2( unsigned int x ) {
    DWORD i;
    _BitScanReverse(&i,(DWORD)x);
    return (int)i;
}

const unsigned interval_step_power = 1;
const unsigned num_interval_steps = 1 << (interval_step_power+1);

DirectXPage::DirectXPage() :
    m_renderNeeded(true)
{
    InitializeComponent();

    m_renderer = ref new tbbTachyonRenderer();

    m_renderer->Initialize(
        Window::Current->CoreWindow,
        this,
        DisplayProperties::LogicalDpi
        );

    m_eventToken = CompositionTarget::Rendering::add(ref new EventHandler<Object^>(this, &DirectXPage::OnRendering));

    int num_threads = 2*tbb::task_scheduler_init::default_num_threads();
    // The thread slider has geometric sequence with several intermidiate steps for each interval between 2^N and 2^(N+1).
    // The nearest (from below) the power of 2.
    int i_base = log2(num_threads);
    int base = 1 << i_base;
    // The step size for the current interval.
    int step = base / num_interval_steps;
    // The number of steps inside the interval.
    int i_step = (num_threads-base)/step;

    ThreadsSlider->Maximum = (i_base-interval_step_power)*num_interval_steps + i_step;
    global_number_of_threads = m_number_of_threads = tbb::task_scheduler_init::automatic;
}

DirectXPage::~DirectXPage()
{
}

void DirectXPage::OnRendering(Platform::Object^ sender, Platform::Object^ args)
{
    if (m_renderNeeded){
        m_renderer->Render();
        m_renderer->Present();
        m_renderNeeded = true;
    }
}

void tbbTachyon::DirectXPage::ThreadsApply_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if (global_number_of_threads != m_number_of_threads){
        global_number_of_threads = m_number_of_threads;
        global_isCancelled = true;
        video->running = false;
        m_renderNeeded = true;
        ThreadsApply->Visibility=Windows::UI::Xaml::Visibility::Collapsed;
    }
}

void tbbTachyon::DirectXPage::Exit_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    m_renderNeeded = false;
    Application::Current->Exit();
}

void tbbTachyon::DirectXPage::ThreadsSliderValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
    int pos = (int) e->NewValue;

    // The nearest (from below) the power of 2.
    int base = pos<num_interval_steps ? 0 : 1 << (pos/num_interval_steps+interval_step_power);
    // The step size for the current interval.
    int step = max(1,base/num_interval_steps);
    m_number_of_threads = base + (pos%num_interval_steps)*step;

    if (m_number_of_threads == 0) m_number_of_threads = tbb::task_scheduler_init::automatic;

    NumberOfThreadsTextBlock->Text="Number Of Threads: " + (m_number_of_threads == tbb::task_scheduler_init::automatic? "Auto": m_number_of_threads.ToString());
    if (global_number_of_threads != m_number_of_threads){
        ThreadsApply->Visibility=Windows::UI::Xaml::Visibility::Visible;
    }else{
        ThreadsApply->Visibility=Windows::UI::Xaml::Visibility::Collapsed;
    }
}
