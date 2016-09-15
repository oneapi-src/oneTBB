/*
    Copyright 2005-2015 Intel Corporation.  All Rights Reserved.

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

#include <wrl.h>
#include "DirectXBase.h"

ref class tbbTachyonRenderer sealed : public DirectXBase
{
public:
    tbbTachyonRenderer();
    virtual void CreateDeviceIndependentResources() override;
    virtual void CreateDeviceResources() override;
    virtual void CreateWindowSizeDependentResources() override;
    virtual void Render() override;
    void Update(float timeTotal, float timeDelta);

    void UpdateView(Windows::Foundation::Point deltaViewPosition);

private:
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_Brush;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_opacityBitmap;
    Microsoft::WRL::ComPtr<IDWriteTextLayout> m_textLayout;
    DWRITE_TEXT_METRICS m_textMetrics;
    bool m_renderNeeded;
    ~tbbTachyonRenderer();
};
