/*
 * logging.cpp - Logging implementation
 */

#include "internal.h"
#include <mutex>
#include <cstdio>
#include <cstdarg>

static std::mutex g_log_mutex;
static int32_t g_log_level = BLUR_LOG_WARN;
static BlurLogCallback g_log_callback = nullptr;
static void* g_log_user_data = nullptr;

void log_init(void) {
    /* Default initialization */
    g_log_level = BLUR_LOG_WARN;
    g_log_callback = nullptr;
    g_log_user_data = nullptr;
}

void log_shutdown(void) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    g_log_callback = nullptr;
    g_log_user_data = nullptr;
}

void log_message(int32_t level, const char* format, ...) {
    if (level > g_log_level) {
        return;
    }
    
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    std::lock_guard<std::mutex> lock(g_log_mutex);
    
    if (g_log_callback) {
        g_log_callback(level, buffer, g_log_user_data);
    } else {
        /* Default: output to debug console on Windows */
        const char* level_str = "UNKNOWN";
        switch (level) {
            case BLUR_LOG_ERROR: level_str = "ERROR"; break;
            case BLUR_LOG_WARN:  level_str = "WARN";  break;
            case BLUR_LOG_INFO:  level_str = "INFO";  break;
            case BLUR_LOG_DEBUG: level_str = "DEBUG"; break;
        }
        
        char output[1100];
        snprintf(output, sizeof(output), "[blur_lib][%s] %s\n", level_str, buffer);
        OutputDebugStringA(output);
    }
}

void BLUR_CALL blur_set_log_level(int32_t level) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    
    if (level >= BLUR_LOG_ERROR && level <= BLUR_LOG_DEBUG) {
        g_log_level = level;
    }
}

void BLUR_CALL blur_set_log_callback(BlurLogCallback callback, void* user_data) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    g_log_callback = callback;
    g_log_user_data = user_data;
}
