/*
 * blur_lib.cpp - Main library implementation
 */

#include "blur_lib.h"
#include "internal.h"
#include <mutex>
#include <atomic>

/* Global state */
static std::atomic<bool> g_initialized{false};
static std::mutex g_mutex;
static uint32_t g_capabilities = 0;

/* Function pointers loaded dynamically */
static SetWindowCompositionAttributeFunc g_pSetWindowCompositionAttribute = nullptr;

int32_t BLUR_CALL blur_init(uint32_t* capabilities) {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    if (g_initialized.load()) {
        if (capabilities) {
            *capabilities = g_capabilities;
        }
        return BLUR_SUCCESS;
    }
    
    /* Initialize logging */
    log_init();
    LOG_INFO("Initializing blur_lib...");
    
    /* Detect capabilities */
    g_capabilities = 0;
    
    /* Try to load SetWindowCompositionAttribute from user32.dll */
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        g_pSetWindowCompositionAttribute = (SetWindowCompositionAttributeFunc)
            GetProcAddress(hUser32, "SetWindowCompositionAttribute");
        
        if (g_pSetWindowCompositionAttribute) {
            g_capabilities |= BLUR_CAP_SETWINDOWCOMPOSITION;
            LOG_INFO("SetWindowCompositionAttribute available");
        }
    }
    
    /* Check DWM availability */
    if (is_dwm_blur_available()) {
        g_capabilities |= BLUR_CAP_DWM_BLUR;
        LOG_INFO("DWM blur-behind available");
    }
    
    /* Color and animation control depend on SetWindowCompositionAttribute */
    if (g_capabilities & BLUR_CAP_SETWINDOWCOMPOSITION) {
        g_capabilities |= BLUR_CAP_COLOR_CONTROL;
        g_capabilities |= BLUR_CAP_ANIMATION_CONTROL;
    }
    
    /* Initialize window state tracker */
    init_window_tracker();

    /* Check Direct2D availability */
    // We can just set the cap for now since we link to it
    g_capabilities |= BLUR_CAP_D2D_BLUR;
    LOG_INFO("Direct2D blur capability enabled");

    
    g_initialized.store(true);
    
    if (capabilities) {
        *capabilities = g_capabilities;
    }
    
    LOG_INFO("blur_lib initialized with capabilities: 0x%08X", g_capabilities);
    return BLUR_SUCCESS;
}

void BLUR_CALL blur_shutdown(void) {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    if (!g_initialized.load()) {
        return;
    }
    
    LOG_INFO("Shutting down blur_lib...");
    
    /* Restore all windows */
    blur_restore_all();
    
    /* Cleanup */
    cleanup_window_tracker();
    log_shutdown();
    
    g_pSetWindowCompositionAttribute = nullptr;
    g_capabilities = 0;
    g_initialized.store(false);
}

int32_t BLUR_CALL blur_apply_to_window(
    uintptr_t window_handle,
    const EffectParams* params,
    uint32_t timeout_ms
) {
    if (!g_initialized.load()) {
        set_last_error("Library not initialized");
        return BLUR_NOT_INITIALIZED;
    }
    
    HWND hwnd = (HWND)window_handle;
    
    /* Validate window handle */
    if (!IsWindow(hwnd)) {
        set_last_error("Invalid window handle");
        return BLUR_INVALID_HANDLE;
    }
    
    /* Check if already applied - if so, clear first to allow re-apply with new params */
    if (is_blur_applied(hwnd)) {
        LOG_DEBUG("Blur already applied, clearing first to update params");
        untrack_window(hwnd);
    }
    
    /* Use default params if not provided */
    EffectParams default_params = {};
    default_params.struct_version = 1;
    default_params.intensity = 1.0f;
    default_params.color_argb = 0;
    default_params.animate = 0;
    default_params.animation_ms = 0;
    
    const EffectParams* effective_params = params ? params : &default_params;
    
    /* Validate params */
    if (effective_params->struct_version != 1) {
        set_last_error("Invalid struct_version");
        return BLUR_INVALID_PARAMS;
    }
    
    if (effective_params->intensity < 0.0f || effective_params->intensity > 1.0f) {
        set_last_error("Intensity must be between 0.0 and 1.0");
        return BLUR_INVALID_PARAMS;
    }
    
    LOG_DEBUG("Applying blur to window 0x%p", hwnd);
    
    int32_t result = BLUR_API_UNSUPPORTED;
    
    /* Try Direct2D blur first (User preferred for Acrylic) */
    if (g_capabilities & BLUR_CAP_D2D_BLUR) {
        result = apply_d2d_blur(hwnd, effective_params);
        if (result == BLUR_SUCCESS) {
            track_window(hwnd, effective_params);
            LOG_INFO("Blur applied via Direct2D");
            return BLUR_SUCCESS;
        }
    }

    /* Try SetWindowCompositionAttribute (Modern Win32) */
    if (g_capabilities & BLUR_CAP_SETWINDOWCOMPOSITION) {
        result = apply_composition_blur(hwnd, effective_params, g_pSetWindowCompositionAttribute);
        if (result == BLUR_SUCCESS) {
            track_window(hwnd, effective_params);
            LOG_INFO("Blur applied via SetWindowCompositionAttribute");
            return BLUR_SUCCESS;
        }
    }
    
    /* Fallback to DWM blur-behind (Legacy Win7/8) */
    if (g_capabilities & BLUR_CAP_DWM_BLUR) {
        result = apply_dwm_blur(hwnd, effective_params);
        if (result == BLUR_SUCCESS) {
            track_window(hwnd, effective_params);
            LOG_INFO("Blur applied via DWM blur-behind");
            return BLUR_SUCCESS;
        }
    }

    
    set_last_error("No blur method available or all methods failed");
    return result;
}

int32_t BLUR_CALL blur_clear_from_window(
    uintptr_t window_handle,
    uint32_t timeout_ms
) {
    if (!g_initialized.load()) {
        set_last_error("Library not initialized");
        return BLUR_NOT_INITIALIZED;
    }
    
    HWND hwnd = (HWND)window_handle;
    
    /* If window is already closed, just remove tracking and succeed */
    if (!IsWindow(hwnd)) {
        untrack_window(hwnd);
        return BLUR_SUCCESS;
    }
    
    /* Check if blur is applied */
    if (!is_blur_applied(hwnd)) {
        /* No blur to clear - succeed silently */
        return BLUR_SUCCESS;
    }
    
    LOG_DEBUG("Clearing blur from window 0x%p", hwnd);
    
    int32_t result = BLUR_SUCCESS;
    
    /* Try to restore via Direct2D */
    if (g_capabilities & BLUR_CAP_D2D_BLUR) {
        result = clear_d2d_blur(hwnd);
    } 
    
    /* Fallback to other clearing methods */
    if (g_capabilities & BLUR_CAP_SETWINDOWCOMPOSITION) {
        result = clear_composition_blur(hwnd, g_pSetWindowCompositionAttribute);
    } else if (g_capabilities & BLUR_CAP_DWM_BLUR) {
        result = clear_dwm_blur(hwnd);
    }

    
    untrack_window(hwnd);
    
    if (result == BLUR_SUCCESS) {
        LOG_INFO("Blur cleared from window");
    }
    
    return result;
}

int32_t BLUR_CALL blur_restore_all(void) {
    if (!g_initialized.load()) {
        set_last_error("Library not initialized");
        return BLUR_NOT_INITIALIZED;
    }
    
    LOG_INFO("Restoring all windows...");
    return restore_all_tracked_windows(g_pSetWindowCompositionAttribute);
}

int32_t BLUR_CALL blur_get_version(char** out_utf8) {
    if (!out_utf8) {
        return BLUR_INVALID_PARAMS;
    }
    
    const char* version = "1.0.0";
    *out_utf8 = alloc_string(version);
    return *out_utf8 ? BLUR_SUCCESS : BLUR_OUT_OF_MEMORY;
}
