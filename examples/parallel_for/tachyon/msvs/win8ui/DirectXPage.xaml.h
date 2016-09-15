/*
    Copyright 2005-2016 Intel Corporation.  All Rights Reserved.

    The source code contained or described herein and all documents related
    to the source code ("Material") are owned by Intel Corporation or its
    suppliers or licensors.  Title to the Material remains with Intel
    Corporation or its suppliers and licensors.  The Material is protected
    by worldwide copyright laws and treaty provisions.  No part of the
    Material may be used, copied, reproduced, modified, published, uploaded,
    posted, transmitted, distributed, or disclosed in any way without
    Intel's prior express written permission.

    No license under any patent, copyright, trade secret or other
    intellectual property right is granted to or conferred upon you by
    disclosure or delivery of the Materials, either expressly, by
    implication, inducement, estoppel or otherwise.  Any license under such
    intellectual property rights must be express and approved by Intel in
    writing.
*/

#pragma once

#include "DirectXPage.g.h"
#include "tbbTachyonRenderer.h"

namespace tbbTachyon
{
    [Windows::Foundation::Metadata::WebHostHidden]
    public ref class DirectXPage sealed
    {
    public:
        DirectXPage();

    private:
        ~DirectXPage();
        void OnRendering(Object^ sender, Object^ args);

        Windows::Foundation::EventRegistrationToken m_eventToken;

        tbbTachyonRenderer^ m_renderer;
        bool m_renderNeeded;
        int m_number_of_threads;

        void ThreadsSliderValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
        void ThreadsApply_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void Exit_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    };
}
