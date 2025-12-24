/*
 * composition.cpp - SetWindowCompositionAttribute implementation
 */

#include "internal.h"
#include <cstdio>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

int32_t apply_composition_blur(HWND hwnd, const EffectParams* params,
                               SetWindowCompositionAttributeFunc pFunc) {
    if (!pFunc) {
        set_last_error("SetWindowCompositionAttribute not available");
        return BLUR_API_UNSUPPORTED;
    }
    
    /* Step 1: Extend frame into client area for transparency */
    MARGINS margins = {-1, -1, -1, -1};
    HRESULT hr = DwmExtendFrameIntoClientArea(hwnd, &margins);
    if (FAILED(hr)) {
        LOG_WARN("DwmExtendFrameIntoClientArea failed: 0x%08X", hr);
    } else {
        LOG_DEBUG("Extended frame into client area");
    }
    
    /* Step 2: Try Windows 11 Mica/Acrylic (Build 22000+) */
    #ifndef DWMWA_SYSTEMBACKDROP_TYPE
    #define DWMWA_SYSTEMBACKDROP_TYPE 38
    #endif
    const int DWMSBT_TRANSIENTWINDOW = 3; /* Acrylic */
    const int DWMSBT_MAINWINDOW = 2;      /* Mica */
    
    int backdropType = (params->intensity >= 0.5f) ? DWMSBT_TRANSIENTWINDOW : DWMSBT_MAINWINDOW;
    hr = DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdropType, sizeof(backdropType));
    if (SUCCEEDED(hr)) {
        LOG_DEBUG("Applied Windows 11 backdrop type: %d", backdropType);
        /* Windows 11 backdrop applied, but still apply composition for color tint */
    }
    
    /* Step 2: Apply accent policy */
    ACCENT_POLICY accent = {};
    
    /* Choose accent state based on intensity */
    if (params->intensity >= 0.5f) {
        /* Use acrylic blur for higher intensity */
        accent.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
    } else {
        /* Use regular blur for lower intensity */
        accent.AccentState = ACCENT_ENABLE_BLURBEHIND;
    }
    
    /* Apply color if specified */
    if (params->color_argb != 0) {
        accent.GradientColor = params->color_argb;
        accent.AccentFlags = 2; /* Flag to enable gradient color */
    } else {
        /* Default semi-transparent dark tint */
        accent.GradientColor = 0x99000000;
        accent.AccentFlags = 2;
    }
    
    WINDOWCOMPOSITIONATTRIBDATA data = {};
    data.Attrib = WCA_ACCENT_POLICY;
    data.pvData = &accent;
    data.cbData = sizeof(accent);
    
    if (!pFunc(hwnd, &data)) {
        DWORD error = GetLastError();
        char msg[256];
        snprintf(msg, sizeof(msg), "SetWindowCompositionAttribute failed with error %lu", error);
        set_last_error(msg);
        return BLUR_INTERNAL_ERROR;
    }
    
    LOG_DEBUG("Applied composition blur: AccentState=%d, GradientColor=0x%08X", 
              accent.AccentState, accent.GradientColor);
    
    return BLUR_SUCCESS;
}

int32_t clear_composition_blur(HWND hwnd, SetWindowCompositionAttributeFunc pFunc) {
    if (!pFunc) {
        set_last_error("SetWindowCompositionAttribute not available");
        return BLUR_API_UNSUPPORTED;
    }
    
    ACCENT_POLICY accent = {};
    accent.AccentState = ACCENT_DISABLED;
    accent.GradientColor = 0;
    accent.AccentFlags = 0;
    accent.AnimationId = 0;
    
    WINDOWCOMPOSITIONATTRIBDATA data = {};
    data.Attrib = WCA_ACCENT_POLICY;
    data.pvData = &accent;
    data.cbData = sizeof(accent);
    
    if (!pFunc(hwnd, &data)) {
        DWORD error = GetLastError();
        char msg[256];
        snprintf(msg, sizeof(msg), "SetWindowCompositionAttribute (clear) failed with error %lu", error);
        set_last_error(msg);
        return BLUR_INTERNAL_ERROR;
    }
    
    LOG_DEBUG("Cleared composition blur");
    return BLUR_SUCCESS;
}
