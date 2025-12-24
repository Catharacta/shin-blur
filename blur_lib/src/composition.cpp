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
    
    /* Step 2: Apply accent policy with user-specified color */
    ACCENT_POLICY accent = {};
    
    /* Use ACRYLICBLURBEHIND which supports custom color */
    accent.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
    
    /* Apply user-specified color (ARGB format) */
    /* The alpha channel controls the tint opacity */
    if (params->color_argb != 0) {
        accent.GradientColor = params->color_argb;
    } else {
        /* Default: semi-transparent based on intensity */
        /* Alpha = intensity * 200 (0-200 range for visibility) */
        uint8_t alpha = (uint8_t)(params->intensity * 200);
        accent.GradientColor = (alpha << 24) | 0x000000; /* Black with variable alpha */
    }
    accent.AccentFlags = 2; /* Enable gradient color */
    
    LOG_DEBUG("Applying blur with GradientColor=0x%08X (intensity=%.2f)", 
              accent.GradientColor, params->intensity);
    
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
    
    /* Step 1: Reset accent policy */
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
    
    /* Step 2: Reset DWM frame extension */
    MARGINS margins = {0, 0, 0, 0};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
    
    LOG_DEBUG("Cleared composition blur and reset DWM frame");
    return BLUR_SUCCESS;
}
