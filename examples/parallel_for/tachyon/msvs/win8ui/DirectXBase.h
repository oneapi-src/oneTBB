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
