/*
 * d2d_blur.cpp - Direct2D Gaussian Blur implementation
 */

#include <initguid.h>
#include "internal.h"
#include <d2d1_1.h>
#include <d2d1effects.h>
#include <d3d11.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <map>
#include <mutex>

using Microsoft::WRL::ComPtr;

struct D2DState {
    HWND targetHwnd;
    UINT_PTR timerId;
    float intensity;
    uint32_t color;
};

static std::map<HWND, D2DState> g_states;
static std::mutex g_mtx;

static ComPtr<ID2D1Factory1> g_d2dFactory;
static ComPtr<ID2D1Device> g_d2dDevice;
static ComPtr<ID2D1DeviceContext> g_d2dContext;
static ComPtr<ID3D11Device> g_d3dDevice;

static HRESULT InitD2D() {
    if (g_d2dFactory) return S_OK;
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), nullptr, &g_d2dFactory);
    if (FAILED(hr)) return hr;
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, &g_d3dDevice, nullptr, nullptr);
    if (FAILED(hr)) return hr;
    ComPtr<IDXGIDevice> dxgi;
    hr = g_d3dDevice.As(&dxgi);
    if (FAILED(hr)) return hr;
    hr = g_d2dFactory->CreateDevice(dxgi.Get(), &g_d2dDevice);
    if (FAILED(hr)) return hr;
    return g_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &g_d2dContext);
}

static void DoBlur(HWND hwnd, float intens, uint32_t col) {
    if (FAILED(InitD2D())) return;

    // Use DWMWA_EXTENDED_FRAME_BOUNDS for accurate visible rect
    RECT rc;
    if (FAILED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rc, sizeof(rc)))) {
        GetWindowRect(hwnd, &rc);
    }
    int w = rc.right - rc.left; int h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) return;

    HDC hdcS = GetDC(NULL); HDC hdcM = CreateCompatibleDC(hdcS);
    BITMAPINFO bmi = {0}; bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = w; bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1; bmi.bmiHeader.biBitCount = 32; bmi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr; HBITMAP hbm = CreateDIBSection(hdcS, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    HGDIOBJ hOld = SelectObject(hdcM, hbm);
    
    // Capture background
    BitBlt(hdcM, 0, 0, w, h, hdcS, rc.left, rc.top, SRCCOPY);

    ComPtr<ID2D1Bitmap1> bIn, bTarget, bStage;
    D2D1_BITMAP_PROPERTIES1 prp = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
    g_d2dContext->CreateBitmap(D2D1::SizeU(w, h), bits, w * 4, &prp, &bIn);
    
    prp.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
    g_d2dContext->CreateBitmap(D2D1::SizeU(w, h), nullptr, 0, &prp, &bTarget);

    g_d2dContext->SetTarget(bTarget.Get());
    g_d2dContext->BeginDraw();
    g_d2dContext->Clear(D2D1::ColorF(0,0,0,0));
    
    ComPtr<ID2D1Effect> blur; g_d2dContext->CreateEffect(CLSID_D2D1GaussianBlur, &blur);
    blur->SetInput(0, bIn.Get()); blur->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, intens * 20.0f);
    g_d2dContext->DrawImage(blur.Get());
    
    if (col != 0) {
        float r = ((col >> 16) & 0xFF) / 255.0f, g = ((col >> 8) & 0xFF) / 255.0f, b = (col & 0xFF) / 255.0f, a = ((col >> 24) & 0xFF) / 255.0f;
        if (a == 0) a = 0.5f;
        ComPtr<ID2D1SolidColorBrush> br; g_d2dContext->CreateSolidColorBrush(D2D1::ColorF(r, g, b, a), &br);
        g_d2dContext->FillRectangle(D2D1::RectF(0, 0, (float)w, (float)h), br.Get());
    }
    
    // DEBUG Border
    ComPtr<ID2D1SolidColorBrush> bdr; g_d2dContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 0.5f), &bdr);
    g_d2dContext->DrawRectangle(D2D1::RectF(1, 1, (float)w - 1, (float)h - 1), bdr.Get(), 1.0f);
    
    g_d2dContext->EndDraw();

    // Staging Copy
    prp.bitmapOptions = D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
    if (SUCCEEDED(g_d2dContext->CreateBitmap(D2D1::SizeU(w, h), nullptr, 0, &prp, &bStage))) {
        bStage->CopyFromBitmap(nullptr, bTarget.Get(), nullptr);
        D2D1_MAPPED_RECT map;
        if (SUCCEEDED(bStage->Map(D2D1_MAP_OPTIONS_READ, &map))) {
            for (int y = 0; y < h; y++) memcpy((BYTE*)bits + y * w * 4, map.bits + y * map.pitch, w * 4);
            bStage->Unmap();
        }
    }

    // UpdateLayeredWindow is the EXCLUSIVE controller of window appearance
    POINT ptD = { rc.left, rc.top }; 
    POINT ptS = { 0, 0 }; 
    SIZE sz = { w, h }; 
    BLENDFUNCTION bl = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    
    UpdateLayeredWindow(hwnd, hdcS, &ptD, &sz, hdcM, &ptS, 0, &bl, ULW_ALPHA);
    
    SelectObject(hdcM, hOld); DeleteObject(hbm); DeleteDC(hdcM); ReleaseDC(NULL, hdcS);
}

static VOID CALLBACK MyTimer(HWND hwnd, UINT msg, UINT_PTR id, DWORD time) {
    float i = 0; uint32_t c = 0;
    { std::lock_guard<std::mutex> l(g_mtx); auto it = g_states.find(hwnd); if (it != g_states.end()) { i = it->second.intensity; c = it->second.color; } else { KillTimer(hwnd, id); return; } }
    DoBlur(hwnd, i, c);
}

int32_t apply_d2d_blur(HWND hwnd, const EffectParams* params) {
    if (FAILED(InitD2D())) return BLUR_INTERNAL_ERROR;
    
    // Ensure WS_EX_LAYERED only. Do NOT call SetLayeredWindowAttributes.
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    
    std::lock_guard<std::mutex> l(g_mtx);
    D2DState& s = g_states[hwnd]; s.targetHwnd = hwnd; s.intensity = params->intensity; s.color = params->color_argb;
    if (s.timerId) KillTimer(hwnd, s.timerId);
    s.timerId = SetTimer(hwnd, (UINT_PTR)hwnd, 100, MyTimer);
    
    DoBlur(hwnd, s.intensity, s.color);
    return BLUR_SUCCESS;
}

int32_t clear_d2d_blur(HWND hwnd) {
    std::lock_guard<std::mutex> l(g_mtx);
    auto it = g_states.find(hwnd);
    if (it != g_states.end()) {
        if (it->second.timerId) KillTimer(hwnd, it->second.timerId);
        g_states.erase(it);
    }
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
    return BLUR_SUCCESS;
}
