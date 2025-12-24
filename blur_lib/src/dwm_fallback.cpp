/*
 * dwm_fallback.cpp - DWM blur-behind fallback implementation
 */

#include "internal.h"
#include <cstdio>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

bool is_dwm_blur_available(void) {
    BOOL dwmEnabled = FALSE;
    HRESULT hr = DwmIsCompositionEnabled(&dwmEnabled);
    
    if (SUCCEEDED(hr) && dwmEnabled) {
        LOG_DEBUG("DWM composition is enabled");
        return true;
    }
    
    LOG_DEBUG("DWM composition is not available");
    return false;
}

int32_t apply_dwm_blur(HWND hwnd, const EffectParams* params) {
    /* Note: DwmEnableBlurBehindWindow is deprecated in Windows 8+
     * but may still work on some configurations */
    
    DWM_BLURBEHIND bb = {};
    bb.dwFlags = DWM_BB_ENABLE;
    bb.fEnable = TRUE;
    bb.hRgnBlur = NULL; /* Blur entire window */
    
    HRESULT hr = DwmEnableBlurBehindWindow(hwnd, &bb);
    
    if (FAILED(hr)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "DwmEnableBlurBehindWindow failed with HRESULT 0x%08lX", hr);
        set_last_error(msg);
        return BLUR_INTERNAL_ERROR;
    }
    
    /* Try to extend frame into client area for better effect */
    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);
    
    LOG_DEBUG("Applied DWM blur-behind");
    return BLUR_SUCCESS;
}

int32_t clear_dwm_blur(HWND hwnd) {
    DWM_BLURBEHIND bb = {};
    bb.dwFlags = DWM_BB_ENABLE;
    bb.fEnable = FALSE;
    bb.hRgnBlur = NULL;
    
    HRESULT hr = DwmEnableBlurBehindWindow(hwnd, &bb);
    
    if (FAILED(hr)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "DwmEnableBlurBehindWindow (clear) failed with HRESULT 0x%08lX", hr);
        set_last_error(msg);
        return BLUR_INTERNAL_ERROR;
    }
    
    /* Reset frame margins */
    MARGINS margins = { 0, 0, 0, 0 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);
    
    LOG_DEBUG("Cleared DWM blur-behind");
    return BLUR_SUCCESS;
}
