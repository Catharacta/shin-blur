/*
 * test_blur.cpp - Basic tests for blur_lib
 */

#include "blur_lib.h"
#include <cstdio>
#include <cassert>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("FAIL: %s\n", msg); \
            return 1; \
        } \
        printf("PASS: %s\n", msg); \
    } while (0)

int test_init_shutdown() {
    uint32_t caps = 0;
    
    /* Test init */
    int32_t result = blur_init(&caps);
    TEST_ASSERT(result == BLUR_SUCCESS, "blur_init should succeed");
    TEST_ASSERT(caps != 0, "Capabilities should be non-zero on Windows 11");
    
    printf("  Capabilities: 0x%08X\n", caps);
    
    if (caps & BLUR_CAP_SETWINDOWCOMPOSITION) {
        printf("  - SetWindowCompositionAttribute: available\n");
    }
    if (caps & BLUR_CAP_DWM_BLUR) {
        printf("  - DWM blur-behind: available\n");
    }
    
    /* Test double init */
    result = blur_init(&caps);
    TEST_ASSERT(result == BLUR_SUCCESS, "Double init should succeed");
    
    /* Test version */
    char* version = nullptr;
    result = blur_get_version(&version);
    TEST_ASSERT(result == BLUR_SUCCESS, "blur_get_version should succeed");
    TEST_ASSERT(version != nullptr, "Version string should not be null");
    printf("  Version: %s\n", version);
    blur_free_string(version);
    
    /* Test shutdown */
    blur_shutdown();
    
    return 0;
}

int test_invalid_handle() {
    uint32_t caps = 0;
    blur_init(&caps);
    
    /* Test with invalid handle */
    int32_t result = blur_apply_to_window(0, nullptr, 0);
    TEST_ASSERT(result == BLUR_INVALID_HANDLE, "Invalid handle should return BLUR_INVALID_HANDLE");
    
    result = blur_apply_to_window((uintptr_t)0xDEADBEEF, nullptr, 0);
    TEST_ASSERT(result == BLUR_INVALID_HANDLE, "Invalid handle should return BLUR_INVALID_HANDLE");
    
    blur_shutdown();
    return 0;
}

int test_not_initialized() {
    /* Ensure not initialized */
    blur_shutdown();
    
    int32_t result = blur_apply_to_window(0, nullptr, 0);
    TEST_ASSERT(result == BLUR_NOT_INITIALIZED, "Should return NOT_INITIALIZED when not initialized");
    
    result = blur_clear_from_window(0, 0);
    TEST_ASSERT(result == BLUR_NOT_INITIALIZED, "Should return NOT_INITIALIZED when not initialized");
    
    return 0;
}

int test_null_params() {
    char** null_ptr = nullptr;
    
    int32_t result = blur_get_last_error(null_ptr);
    TEST_ASSERT(result == BLUR_INVALID_PARAMS, "Null output param should return INVALID_PARAMS");
    
    result = blur_get_version(null_ptr);
    TEST_ASSERT(result == BLUR_INVALID_PARAMS, "Null output param should return INVALID_PARAMS");
    
    return 0;
}

int main() {
    printf("=== blur_lib Test Suite ===\n\n");
    
    int failures = 0;
    
    printf("Test: init_shutdown\n");
    failures += test_init_shutdown();
    printf("\n");
    
    printf("Test: invalid_handle\n");
    failures += test_invalid_handle();
    printf("\n");
    
    printf("Test: not_initialized\n");
    failures += test_not_initialized();
    printf("\n");
    
    printf("Test: null_params\n");
    failures += test_null_params();
    printf("\n");
    
    printf("=== Results: %d failures ===\n", failures);
    
    return failures;
}
