/*
 * window_tracker.cpp - Window state tracking
 */

#include "internal.h"
#include <unordered_map>
#include <mutex>
#include <sstream>

struct WindowState {
    EffectParams params;
    bool using_composition;  /* true = SetWindowCompositionAttribute, false = DWM */
};

static std::mutex g_tracker_mutex;
static std::unordered_map<HWND, WindowState> g_tracked_windows;

void init_window_tracker(void) {
    std::lock_guard<std::mutex> lock(g_tracker_mutex);
    g_tracked_windows.clear();
}

void cleanup_window_tracker(void) {
    std::lock_guard<std::mutex> lock(g_tracker_mutex);
    g_tracked_windows.clear();
}

void track_window(HWND hwnd, const EffectParams* params) {
    std::lock_guard<std::mutex> lock(g_tracker_mutex);
    
    WindowState state = {};
    if (params) {
        state.params = *params;
    }
    state.using_composition = true;  /* Default to composition */
    
    g_tracked_windows[hwnd] = state;
    LOG_DEBUG("Tracking window 0x%p, total tracked: %zu", hwnd, g_tracked_windows.size());
}

void untrack_window(HWND hwnd) {
    std::lock_guard<std::mutex> lock(g_tracker_mutex);
    g_tracked_windows.erase(hwnd);
    LOG_DEBUG("Untracked window 0x%p, total tracked: %zu", hwnd, g_tracked_windows.size());
}

bool is_blur_applied(HWND hwnd) {
    std::lock_guard<std::mutex> lock(g_tracker_mutex);
    return g_tracked_windows.find(hwnd) != g_tracked_windows.end();
}

int32_t restore_all_tracked_windows(SetWindowCompositionAttributeFunc pFunc) {
    std::lock_guard<std::mutex> lock(g_tracker_mutex);
    
    int32_t result = BLUR_SUCCESS;
    
    for (const auto& pair : g_tracked_windows) {
        HWND hwnd = pair.first;
        
        /* Skip if window is already destroyed */
        if (!IsWindow(hwnd)) {
            continue;
        }
        
        if (pair.second.using_composition && pFunc) {
            if (clear_composition_blur(hwnd, pFunc) != BLUR_SUCCESS) {
                result = BLUR_INTERNAL_ERROR;
            }
        } else {
            if (clear_dwm_blur(hwnd) != BLUR_SUCCESS) {
                result = BLUR_INTERNAL_ERROR;
            }
        }
    }
    
    g_tracked_windows.clear();
    return result;
}

std::string get_tracked_windows_json() {
    std::lock_guard<std::mutex> lock(g_tracker_mutex);
    
    std::ostringstream oss;
    oss << "[";
    
    bool first = true;
    for (const auto& pair : g_tracked_windows) {
        if (!first) {
            oss << ",";
        }
        first = false;
        
        oss << "{\"hwnd\":" << (uintptr_t)pair.first 
            << ",\"intensity\":" << pair.second.params.intensity 
            << "}";
    }
    
    oss << "]";
    return oss.str();
}
