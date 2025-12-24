/*
 * blur_lib.h - Windows 11 Blur Library Public API
 * 
 * Copyright (c) 2024
 * License: MIT
 */

#ifndef BLUR_LIB_H
#define BLUR_LIB_H

#include <stdint.h>

#ifdef _WIN32
    #define BLUR_CALL __stdcall
    #ifdef BLUR_LIB_EXPORTS
        #define BLUR_API __declspec(dllexport)
    #else
        #define BLUR_API __declspec(dllimport)
    #endif
#else
    #define BLUR_CALL
    #define BLUR_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Capability Bits (returned by blur_init)
 * ============================================================================ */
#define BLUR_CAP_SETWINDOWCOMPOSITION  0x0001  /* SetWindowCompositionAttribute available */
#define BLUR_CAP_DWM_BLUR              0x0002  /* DwmEnableBlurBehindWindow available */
#define BLUR_CAP_OVERLAY_FALLBACK      0x0004  /* Overlay fallback available */
#define BLUR_CAP_COLOR_CONTROL         0x0008  /* Color tint control supported */
#define BLUR_CAP_ANIMATION_CONTROL     0x0010  /* Animation control supported */

/* ============================================================================
 * Error Codes
 * ============================================================================ */
#define BLUR_SUCCESS            0   /* Operation completed successfully */
#define BLUR_NOT_INITIALIZED    1   /* Library not initialized */
#define BLUR_INVALID_HANDLE     2   /* Invalid window handle */
#define BLUR_PERMISSION_DENIED  3   /* Permission denied */
#define BLUR_API_UNSUPPORTED    4   /* API not supported on this system */
#define BLUR_TIMEOUT            5   /* Operation timed out */
#define BLUR_OUT_OF_MEMORY      6   /* Memory allocation failed */
#define BLUR_INTERNAL_ERROR     7   /* Internal error occurred */
#define BLUR_INVALID_PARAMS     8   /* Invalid parameters provided */
#define BLUR_ALREADY_APPLIED    9   /* Blur already applied to window */

/* ============================================================================
 * EffectParams Structure (Version 1)
 * ============================================================================ */
#pragma pack(push, 1)
typedef struct EffectParams_V1 {
    uint32_t struct_version;      /* Must be 1 */
    float    intensity;           /* 0.0 to 1.0 */
    uint32_t color_argb;          /* 0xAARRGGBB (0 = no override) */
    uint8_t  animate;             /* 0 = no animation, 1 = animate */
    uint32_t animation_ms;        /* Animation duration in milliseconds */
    uint32_t reserved_flags;      /* Reserved for future use */
    uint8_t  reserved_padding[4]; /* Alignment padding */
} EffectParams_V1;
#pragma pack(pop)

typedef EffectParams_V1 EffectParams;

/* ============================================================================
 * Log Levels
 * ============================================================================ */
#define BLUR_LOG_ERROR   0
#define BLUR_LOG_WARN    1
#define BLUR_LOG_INFO    2
#define BLUR_LOG_DEBUG   3

/* Log callback function type */
typedef void (BLUR_CALL *BlurLogCallback)(int32_t level, const char* utf8_msg, void* user_data);

/* ============================================================================
 * Core API Functions
 * ============================================================================ */

/**
 * Initialize the blur library.
 * Must be called before any other API functions.
 * 
 * @param capabilities Output pointer to receive capability bits
 * @return BLUR_SUCCESS on success, error code otherwise
 */
BLUR_API int32_t BLUR_CALL blur_init(uint32_t* capabilities);

/**
 * Shutdown the blur library and release all resources.
 * Optionally clears all applied blur effects.
 */
BLUR_API void BLUR_CALL blur_shutdown(void);

/**
 * Apply blur effect to a window.
 * 
 * @param window_handle HWND of the target window
 * @param params Effect parameters (NULL for defaults)
 * @param timeout_ms Maximum time to wait (0 = default SLO)
 * @return BLUR_SUCCESS on success, error code otherwise
 */
BLUR_API int32_t BLUR_CALL blur_apply_to_window(
    uintptr_t window_handle,
    const EffectParams* params,
    uint32_t timeout_ms
);

/**
 * Remove blur effect from a window.
 * 
 * @param window_handle HWND of the target window
 * @param timeout_ms Maximum time to wait (0 = default SLO)
 * @return BLUR_SUCCESS on success, error code otherwise
 */
BLUR_API int32_t BLUR_CALL blur_clear_from_window(
    uintptr_t window_handle,
    uint32_t timeout_ms
);

/* ============================================================================
 * Auxiliary API Functions
 * ============================================================================ */

/**
 * Restore all windows to their original state.
 * 
 * @return BLUR_SUCCESS on success, error code otherwise
 */
BLUR_API int32_t BLUR_CALL blur_restore_all(void);

/**
 * Get list of windows with blur currently applied.
 * 
 * @param out_json_utf8 Output pointer to receive JSON string (free with blur_free_string)
 * @return BLUR_SUCCESS on success, error code otherwise
 */
BLUR_API int32_t BLUR_CALL blur_get_blurred_list(char** out_json_utf8);

/**
 * Get the last error message.
 * 
 * @param out_utf8 Output pointer to receive error message (free with blur_free_string)
 * @return BLUR_SUCCESS on success, error code otherwise
 */
BLUR_API int32_t BLUR_CALL blur_get_last_error(char** out_utf8);

/**
 * Get the library version string.
 * 
 * @param out_utf8 Output pointer to receive version string (free with blur_free_string)
 * @return BLUR_SUCCESS on success, error code otherwise
 */
BLUR_API int32_t BLUR_CALL blur_get_version(char** out_utf8);

/**
 * Free a string allocated by the library.
 * 
 * @param ptr Pointer to the string to free
 */
BLUR_API void BLUR_CALL blur_free_string(char* ptr);

/* ============================================================================
 * Logging API Functions
 * ============================================================================ */

/**
 * Set the log level.
 * 
 * @param level Log level (BLUR_LOG_ERROR, BLUR_LOG_WARN, BLUR_LOG_INFO, BLUR_LOG_DEBUG)
 */
BLUR_API void BLUR_CALL blur_set_log_level(int32_t level);

/**
 * Set a callback function for log messages.
 * 
 * @param callback Callback function (NULL to disable)
 * @param user_data User data passed to callback
 */
BLUR_API void BLUR_CALL blur_set_log_callback(BlurLogCallback callback, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* BLUR_LIB_H */
