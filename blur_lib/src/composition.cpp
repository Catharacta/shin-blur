/*
 * composition.cpp - SetWindowCompositionAttribute implementation
 */

#include "internal.h"
#include <cstdio>

int32_t apply_composition_blur(HWND hwnd, const EffectParams* params,
                               SetWindowCompositionAttributeFunc pFunc) {
    if (!pFunc) {
        set_last_error("SetWindowCompositionAttribute not available");
        return BLUR_API_UNSUPPORTED;
    }
    
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
        accent.GradientColor = 0x01000000;
        accent.AccentFlags = 0;
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
