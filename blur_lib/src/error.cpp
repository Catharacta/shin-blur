/*
 * error.cpp - Error handling and string utilities
 */

#include "internal.h"
#include <string>
#include <mutex>

static std::mutex g_error_mutex;
static std::string g_last_error;

void set_last_error(const char* message) {
    std::lock_guard<std::mutex> lock(g_error_mutex);
    g_last_error = message ? message : "";
    LOG_ERROR("%s", message);
}

char* alloc_string(const char* src) {
    if (!src) {
        return nullptr;
    }
    
    size_t len = strlen(src) + 1;
    char* dst = (char*)malloc(len);
    
    if (dst) {
        memcpy(dst, src, len);
    }
    
    return dst;
}

int32_t BLUR_CALL blur_get_last_error(char** out_utf8) {
    if (!out_utf8) {
        return BLUR_INVALID_PARAMS;
    }
    
    std::lock_guard<std::mutex> lock(g_error_mutex);
    
    if (g_last_error.empty()) {
        *out_utf8 = alloc_string("No error");
    } else {
        *out_utf8 = alloc_string(g_last_error.c_str());
    }
    
    return *out_utf8 ? BLUR_SUCCESS : BLUR_OUT_OF_MEMORY;
}

void BLUR_CALL blur_free_string(char* ptr) {
    if (ptr) {
        free(ptr);
    }
}

int32_t BLUR_CALL blur_get_blurred_list(char** out_json_utf8) {
    if (!out_json_utf8) {
        return BLUR_INVALID_PARAMS;
    }
    
    /* Get list from window tracker */
    extern std::string get_tracked_windows_json();
    std::string json = get_tracked_windows_json();
    
    *out_json_utf8 = alloc_string(json.c_str());
    return *out_json_utf8 ? BLUR_SUCCESS : BLUR_OUT_OF_MEMORY;
}
