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

#include <wrl/client.h>
#include <d3d11_1.h>
#include <d2d1_1.h>
#include <d2d1effects.h>
#include <dwrite_1.h>
#include <wincodec.h>
#include "App.xaml.h"
#include <agile.h>

#pragma warning (disable: 4449)

// Helper utilities to make DirectX APIs work with exceptions
namespace DX
{
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            // Set a breakpoint on this line to catch DirectX API errors
            throw Platform::Exception::CreateException(hr);
        }
    }
}

// Helper class that initializes DirectX APIs
ref class DirectXBase abstract
{
internal:
    DirectXBase();

public:
    virtual void Initialize(Windows::UI::Core::CoreWindow^ window, Windows::UI::Xaml::Controls::SwapChainBackgroundPanel^ panel, float dpi);
    virtual void CreateDeviceIndependentResources();
    virtual void CreateDeviceResources();
    virtual void SetDpi(float dpi);
    virtual void CreateWindowSizeDependentResources();
    virtual void UpdateForWindowSizeChange();
    virtual void Render() = 0;
    virtual void Present();
    virtual float ConvertDipsToPixels(float dips);

protected private:

    Platform::Agile<Windows::UI::Core::CoreWindow>         m_window;
    Windows::UI::Xaml::Controls::SwapChainBackgroundPanel^ m_panel;

    // Direct2D Objects
    Microsoft::WRL::ComPtr<ID2D1Factory1>                  m_d2dFactory;
    Microsoft::WRL::ComPtr<ID2D1Device>                    m_d2dDevice;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext>             m_d2dContext;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1>                   m_d2dTargetBitmap;

    // DirectWrite & Windows Imaging Component Objects
    Microsoft::WRL::ComPtr<IDWriteFactory1>                m_dwriteFactory;
    Microsoft::WRL::ComPtr<IWICImagingFactory2>            m_wicFactory;

    // Direct3D Objects
    Microsoft::WRL::ComPtr<ID3D11Device1>                  m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1>           m_d3dContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain1>                m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>         m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>         m_depthStencilView;

    D3D_FEATURE_LEVEL                                      m_featureLevel;
    Windows::Foundation::Size                              m_renderTargetSize;
    Windows::Foundation::Rect                              m_windowBounds;
    float                                                  m_dpi;
};

#pragma warning (default: 4449)
