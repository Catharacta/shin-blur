/*
 * benchmark.cpp - Performance benchmark for blur_lib
 * 
 * Measures apply/clear latency to verify SLO compliance:
 * - apply: 95th percentile < 300ms
 * - clear: 95th percentile < 200ms
 */

#include "blur_lib.h"
#include <windows.h>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <chrono>

using namespace std::chrono;

/* Test window */
static HWND g_testWindow = NULL;

LRESULT CALLBACK TestWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool CreateTestWindow() {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = TestWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"BlurBenchmarkWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    g_testWindow = CreateWindowW(
        L"BlurBenchmarkWindow", L"Benchmark Test Window",
        WS_OVERLAPPEDWINDOW,
        100, 100, 400, 300,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (!g_testWindow) {
        printf("Failed to create test window\n");
        return false;
    }

    ShowWindow(g_testWindow, SW_SHOW);
    UpdateWindow(g_testWindow);
    
    /* Process messages to ensure window is fully created */
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return true;
}

void DestroyTestWindow() {
    if (g_testWindow) {
        DestroyWindow(g_testWindow);
        g_testWindow = NULL;
    }
}

double CalculatePercentile(std::vector<double>& data, double percentile) {
    if (data.empty()) return 0.0;
    
    std::sort(data.begin(), data.end());
    size_t index = (size_t)((percentile / 100.0) * (data.size() - 1));
    return data[index];
}

void RunBenchmark(int iterations) {
    printf("\n=== Blur Library Benchmark ===\n");
    printf("Iterations: %d\n\n", iterations);

    uint32_t caps = 0;
    int32_t result = blur_init(&caps);
    if (result != BLUR_SUCCESS) {
        printf("Failed to initialize blur_lib\n");
        return;
    }

    printf("Capabilities: 0x%04X\n\n", caps);

    if (!CreateTestWindow()) {
        blur_shutdown();
        return;
    }

    std::vector<double> applyTimes;
    std::vector<double> clearTimes;

    /* Warm-up */
    printf("Warming up...\n");
    for (int i = 0; i < 5; i++) {
        blur_apply_to_window((uintptr_t)g_testWindow, nullptr, 0);
        blur_clear_from_window((uintptr_t)g_testWindow, 0);
    }

    /* Benchmark */
    printf("Running benchmark...\n");
    for (int i = 0; i < iterations; i++) {
        /* Measure apply */
        auto start = high_resolution_clock::now();
        result = blur_apply_to_window((uintptr_t)g_testWindow, nullptr, 0);
        auto end = high_resolution_clock::now();
        
        if (result == BLUR_SUCCESS) {
            double ms = duration<double, std::milli>(end - start).count();
            applyTimes.push_back(ms);
        }

        /* Small delay */
        Sleep(10);

        /* Measure clear */
        start = high_resolution_clock::now();
        result = blur_clear_from_window((uintptr_t)g_testWindow, 0);
        end = high_resolution_clock::now();
        
        if (result == BLUR_SUCCESS) {
            double ms = duration<double, std::milli>(end - start).count();
            clearTimes.push_back(ms);
        }

        /* Small delay between iterations */
        Sleep(10);

        if ((i + 1) % 10 == 0) {
            printf("  Progress: %d/%d\n", i + 1, iterations);
        }
    }

    /* Calculate statistics */
    printf("\n=== Results ===\n\n");

    if (!applyTimes.empty()) {
        double applyP50 = CalculatePercentile(applyTimes, 50);
        double applyP95 = CalculatePercentile(applyTimes, 95);
        double applyP99 = CalculatePercentile(applyTimes, 99);
        double applyMin = *std::min_element(applyTimes.begin(), applyTimes.end());
        double applyMax = *std::max_element(applyTimes.begin(), applyTimes.end());

        printf("apply_blur_to_window:\n");
        printf("  Min:  %.2f ms\n", applyMin);
        printf("  P50:  %.2f ms\n", applyP50);
        printf("  P95:  %.2f ms  (target: < 300ms) %s\n", applyP95, applyP95 < 300 ? "PASS" : "FAIL");
        printf("  P99:  %.2f ms\n", applyP99);
        printf("  Max:  %.2f ms\n", applyMax);
        printf("\n");
    }

    if (!clearTimes.empty()) {
        double clearP50 = CalculatePercentile(clearTimes, 50);
        double clearP95 = CalculatePercentile(clearTimes, 95);
        double clearP99 = CalculatePercentile(clearTimes, 99);
        double clearMin = *std::min_element(clearTimes.begin(), clearTimes.end());
        double clearMax = *std::max_element(clearTimes.begin(), clearTimes.end());

        printf("clear_blur_from_window:\n");
        printf("  Min:  %.2f ms\n", clearMin);
        printf("  P50:  %.2f ms\n", clearP50);
        printf("  P95:  %.2f ms  (target: < 200ms) %s\n", clearP95, clearP95 < 200 ? "PASS" : "FAIL");
        printf("  P99:  %.2f ms\n", clearP99);
        printf("  Max:  %.2f ms\n", clearMax);
    }

    DestroyTestWindow();
    blur_shutdown();
}

int main(int argc, char* argv[]) {
    int iterations = 100;
    
    if (argc > 1) {
        iterations = atoi(argv[1]);
        if (iterations < 1) iterations = 100;
    }

    RunBenchmark(iterations);

    printf("\nBenchmark complete.\n");
    return 0;
}
