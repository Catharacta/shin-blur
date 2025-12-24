/*
 * internal.h - Internal declarations
 */

#ifndef BLUR_LIB_INTERNAL_H
#define BLUR_LIB_INTERNAL_H

#include "blur_lib.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dwmapi.h>

/* ============================================================================
 * Undocumented Windows structures for SetWindowCompositionAttribute
 * ============================================================================ */

typedef enum _ACCENT_STATE {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
    ACCENT_ENABLE_HOSTBACKDROP = 5,
    ACCENT_INVALID_STATE = 6
} ACCENT_STATE;

typedef struct _ACCENT_POLICY {
    ACCENT_STATE AccentState;
    DWORD AccentFlags;
    DWORD GradientColor;    /* ARGB format */
    DWORD AnimationId;
} ACCENT_POLICY;

typedef enum _WINDOWCOMPOSITIONATTRIB {
    WCA_UNDEFINED = 0,
    WCA_NCRENDERING_ENABLED = 1,
    WCA_NCRENDERING_POLICY = 2,
    WCA_TRANSITIONS_FORCEDISABLED = 3,
    WCA_ALLOW_NCPAINT = 4,
    WCA_CAPTION_BUTTON_BOUNDS = 5,
    WCA_NONCLIENT_RTL_LAYOUT = 6,
    WCA_FORCE_ICONIC_REPRESENTATION = 7,
    WCA_EXTENDED_FRAME_BOUNDS = 8,
    WCA_HAS_ICONIC_BITMAP = 9,
    WCA_THEME_ATTRIBUTES = 10,
    WCA_NCRENDERING_EXILED = 11,
    WCA_NCADORNMENTINFO = 12,
    WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
    WCA_VIDEO_OVERLAY_ACTIVE = 14,
    WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
    WCA_DISALLOW_PEEK = 16,
    WCA_CLOAK = 17,
    WCA_CLOAKED = 18,
    WCA_ACCENT_POLICY = 19,
    WCA_FREEZE_REPRESENTATION = 20,
    WCA_EVER_UNCLOAKED = 21,
    WCA_VISUAL_OWNER = 22,
    WCA_HOLOGRAPHIC = 23,
    WCA_EXCLUDED_FROM_DDA = 24,
    WCA_PASSIVEUPDATEMODE = 25,
    WCA_USEDARKMODECOLORS = 26,
    WCA_CORNER_STYLE = 27,
    WCA_PART_COLOR = 28,
    WCA_DISABLE_MOVESIZE_FEEDBACK = 29,
    WCA_LAST = 30
} WINDOWCOMPOSITIONATTRIB;

typedef struct _WINDOWCOMPOSITIONATTRIBDATA {
    WINDOWCOMPOSITIONATTRIB Attrib;
    PVOID pvData;
    SIZE_T cbData;
} WINDOWCOMPOSITIONATTRIBDATA;

typedef BOOL(WINAPI* SetWindowCompositionAttributeFunc)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

/* ============================================================================
 * Logging functions (logging.cpp)
 * ============================================================================ */
void log_init(void);
void log_shutdown(void);
void log_message(int32_t level, const char* format, ...);

#define LOG_ERROR(fmt, ...) log_message(BLUR_LOG_ERROR, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  log_message(BLUR_LOG_WARN, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  log_message(BLUR_LOG_INFO, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) log_message(BLUR_LOG_DEBUG, fmt, ##__VA_ARGS__)

/* ============================================================================
 * Error handling (error.cpp)
 * ============================================================================ */
void set_last_error(const char* message);
char* alloc_string(const char* src);

/* ============================================================================
 * Window tracking (blur_lib.cpp)
 * ============================================================================ */
void init_window_tracker(void);
void cleanup_window_tracker(void);
void track_window(HWND hwnd, const EffectParams* params);
void untrack_window(HWND hwnd);
bool is_blur_applied(HWND hwnd);
int32_t restore_all_tracked_windows(SetWindowCompositionAttributeFunc pFunc);

/* ============================================================================
 * SetWindowCompositionAttribute implementation (composition.cpp)
 * ============================================================================ */
int32_t apply_composition_blur(HWND hwnd, const EffectParams* params, 
                               SetWindowCompositionAttributeFunc pFunc);
int32_t clear_composition_blur(HWND hwnd, SetWindowCompositionAttributeFunc pFunc);

/* ============================================================================
 * DWM fallback implementation (dwm_fallback.cpp)
 * ============================================================================ */
bool is_dwm_blur_available(void);
int32_t apply_dwm_blur(HWND hwnd, const EffectParams* params);
int32_t clear_dwm_blur(HWND hwnd);

#endif /* BLUR_LIB_INTERNAL_H */
