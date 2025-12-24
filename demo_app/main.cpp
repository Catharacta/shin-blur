/*
 * Blur Demo Application
 * 
 * A simple Windows GUI app to test blur_lib functionality.
 * - Click on a window to select it
 * - Apply/Clear blur effects
 * - Adjust intensity and color
 */

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "blur_lib.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' \
    name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
    processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

/* Control IDs */
#define ID_BTN_PICK       101
#define ID_BTN_APPLY      102
#define ID_BTN_CLEAR      103
#define ID_BTN_RESTORE    104
#define ID_SLIDER_INTENSITY 105
#define ID_EDIT_COLOR     106
#define ID_STATIC_HWND    107
#define ID_STATIC_CAPS    108
#define ID_STATIC_STATUS  109
#define ID_LIST_LOG       110
#define ID_BTN_CREATE_TEST 111

/* Global state */
static HWND g_hMainWnd = NULL;
static HWND g_hTargetWnd = NULL;
static HWND g_hListLog = NULL;
static HWND g_hStaticHwnd = NULL;
static HWND g_hStaticCaps = NULL;
static HWND g_hStaticStatus = NULL;
static HWND g_hSliderIntensity = NULL;
static HWND g_hEditColor = NULL;
static uint32_t g_capabilities = 0;
static bool g_picking = false;

/* Forward declarations */
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PickerProc(int, WPARAM, LPARAM);
LRESULT CALLBACK BackgroundWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TestWndProc(HWND, UINT, WPARAM, LPARAM);
void AddLogMessage(const char* format, ...);
void UpdateStatus(const char* status);
void UpdateTargetInfo();
void CreateTestWindows();
void DestroyTestWindows();
HHOOK g_hMouseHook = NULL;
HWND g_hBackgroundWnd = NULL;
HWND g_hTestWnd = NULL;

/* Log callback */
void BLUR_CALL LogCallback(int32_t level, const char* msg, void* user_data) {
    const char* prefix = "";
    switch (level) {
        case BLUR_LOG_ERROR: prefix = "[ERROR] "; break;
        case BLUR_LOG_WARN:  prefix = "[WARN] "; break;
        case BLUR_LOG_INFO:  prefix = "[INFO] "; break;
        case BLUR_LOG_DEBUG: prefix = "[DEBUG] "; break;
    }
    AddLogMessage("%s%s", prefix, msg);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    /* Initialize common controls */
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    /* Register window class */
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"BlurDemoClass";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    /* Create main window */
    g_hMainWnd = CreateWindowExW(
        0, L"BlurDemoClass", L"Blur Library Demo",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 600,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hMainWnd) {
        MessageBoxW(NULL, L"Failed to create window", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    /* Message loop */
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

void CreateControls(HWND hWnd) {
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
    int y = 10;

    /* Capabilities label */
    CreateWindowW(L"STATIC", L"Capabilities:", WS_CHILD | WS_VISIBLE,
        10, y, 100, 20, hWnd, NULL, hInst, NULL);
    g_hStaticCaps = CreateWindowW(L"STATIC", L"Initializing...",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        110, y, 370, 40, hWnd, (HMENU)ID_STATIC_CAPS, hInst, NULL);
    y += 50;

    /* Target window info */
    CreateWindowW(L"STATIC", L"Target HWND:", WS_CHILD | WS_VISIBLE,
        10, y, 100, 20, hWnd, NULL, hInst, NULL);
    g_hStaticHwnd = CreateWindowW(L"STATIC", L"(none)",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        110, y, 300, 20, hWnd, (HMENU)ID_STATIC_HWND, hInst, NULL);
    y += 30;

    /* Pick button */
    CreateWindowW(L"BUTTON", L"Pick Window (Click)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, y, 150, 30, hWnd, (HMENU)ID_BTN_PICK, hInst, NULL);
    CreateWindowW(L"BUTTON", L"Create Test Windows",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        170, y, 160, 30, hWnd, (HMENU)ID_BTN_CREATE_TEST, hInst, NULL);
    y += 40;

    /* Intensity slider */
    CreateWindowW(L"STATIC", L"Intensity:", WS_CHILD | WS_VISIBLE,
        10, y, 80, 20, hWnd, NULL, hInst, NULL);
    g_hSliderIntensity = CreateWindowW(TRACKBAR_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS,
        100, y, 300, 30, hWnd, (HMENU)ID_SLIDER_INTENSITY, hInst, NULL);
    SendMessage(g_hSliderIntensity, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    SendMessage(g_hSliderIntensity, TBM_SETPOS, TRUE, 100);
    y += 40;

    /* Color input */
    CreateWindowW(L"STATIC", L"Color (ARGB):", WS_CHILD | WS_VISIBLE,
        10, y, 90, 20, hWnd, NULL, hInst, NULL);
    g_hEditColor = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"80000000",
        WS_CHILD | WS_VISIBLE | ES_LEFT,
        100, y, 120, 24, hWnd, (HMENU)ID_EDIT_COLOR, hInst, NULL);
    y += 35;

    /* Action buttons */
    CreateWindowW(L"BUTTON", L"Apply Blur",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, y, 100, 30, hWnd, (HMENU)ID_BTN_APPLY, hInst, NULL);
    CreateWindowW(L"BUTTON", L"Clear Blur",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        120, y, 100, 30, hWnd, (HMENU)ID_BTN_CLEAR, hInst, NULL);
    CreateWindowW(L"BUTTON", L"Restore All",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        230, y, 100, 30, hWnd, (HMENU)ID_BTN_RESTORE, hInst, NULL);
    y += 45;

    /* Status */
    CreateWindowW(L"STATIC", L"Status:", WS_CHILD | WS_VISIBLE,
        10, y, 50, 20, hWnd, NULL, hInst, NULL);
    g_hStaticStatus = CreateWindowW(L"STATIC", L"Ready",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        70, y, 400, 20, hWnd, (HMENU)ID_STATIC_STATUS, hInst, NULL);
    y += 30;

    /* Log listbox */
    CreateWindowW(L"STATIC", L"Log:", WS_CHILD | WS_VISIBLE,
        10, y, 50, 20, hWnd, NULL, hInst, NULL);
    y += 25;
    g_hListLog = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT,
        10, y, 460, 200, hWnd, (HMENU)ID_LIST_LOG, hInst, NULL);
}

void InitializeLibrary() {
    /* Set up logging */
    blur_set_log_level(BLUR_LOG_DEBUG);
    blur_set_log_callback(LogCallback, NULL);

    /* Initialize */
    int32_t result = blur_init(&g_capabilities);
    
    if (result == BLUR_SUCCESS) {
        char caps[256];
        snprintf(caps, sizeof(caps), "0x%04X - %s%s%s%s",
            g_capabilities,
            (g_capabilities & BLUR_CAP_SETWINDOWCOMPOSITION) ? "Composition " : "",
            (g_capabilities & BLUR_CAP_DWM_BLUR) ? "DWM " : "",
            (g_capabilities & BLUR_CAP_COLOR_CONTROL) ? "Color " : "",
            (g_capabilities & BLUR_CAP_ANIMATION_CONTROL) ? "Animation" : ""
        );
        SetWindowTextA(g_hStaticCaps, caps);
        AddLogMessage("Library initialized successfully");
    } else {
        SetWindowTextA(g_hStaticCaps, "FAILED");
        AddLogMessage("Failed to initialize library: %d", result);
    }
}

LRESULT CALLBACK PickerProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && g_picking) {
        MOUSEHOOKSTRUCT* pMouse = (MOUSEHOOKSTRUCT*)lParam;
        
        if (wParam == WM_LBUTTONDOWN) {
            POINT pt = pMouse->pt;
            HWND hwnd = WindowFromPoint(pt);
            
            /* Get top-level window */
            HWND hwndRoot = GetAncestor(hwnd, GA_ROOT);
            if (hwndRoot && hwndRoot != g_hMainWnd) {
                g_hTargetWnd = hwndRoot;
                UpdateTargetInfo();
                AddLogMessage("Selected window: 0x%p", g_hTargetWnd);
            }
            
            g_picking = false;
            ReleaseCapture();
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            UnhookWindowsHookEx(g_hMouseHook);
            g_hMouseHook = NULL;
            UpdateStatus("Window selected");
            return 1; /* Consume the click */
        }
    }
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

void StartPicking() {
    g_picking = true;
    SetCursor(LoadCursor(NULL, IDC_CROSS));
    UpdateStatus("Click on a window to select...");
    
    g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, PickerProc, NULL, 0);
}

void UpdateTargetInfo() {
    if (g_hTargetWnd && IsWindow(g_hTargetWnd)) {
        char title[256];
        char info[512];
        GetWindowTextA(g_hTargetWnd, title, sizeof(title));
        snprintf(info, sizeof(info), "0x%p - %.200s", g_hTargetWnd, title);
        SetWindowTextA(g_hStaticHwnd, info);
    } else {
        SetWindowTextA(g_hStaticHwnd, "(none)");
    }
}

void ApplyBlur() {
    if (!g_hTargetWnd || !IsWindow(g_hTargetWnd)) {
        UpdateStatus("No target window selected");
        return;
    }

    /* Get intensity */
    int intensity = (int)SendMessage(g_hSliderIntensity, TBM_GETPOS, 0, 0);
    
    /* Get color */
    char colorText[32];
    GetWindowTextA(g_hEditColor, colorText, sizeof(colorText));
    uint32_t color = (uint32_t)strtoul(colorText, NULL, 16);

    /* Build params */
    EffectParams params = {};
    params.struct_version = 1;
    params.intensity = intensity / 100.0f;
    params.color_argb = color;
    params.animate = 0;

    AddLogMessage("Applying blur: intensity=%.2f, color=0x%08X", params.intensity, params.color_argb);

    int32_t result = blur_apply_to_window((uintptr_t)g_hTargetWnd, &params, 0);
    
    if (result == BLUR_SUCCESS) {
        UpdateStatus("Blur applied successfully");
    } else {
        char* error = NULL;
        blur_get_last_error(&error);
        UpdateStatus(error ? error : "Apply failed");
        if (error) blur_free_string(error);
    }
}

void ClearBlur() {
    if (!g_hTargetWnd) {
        UpdateStatus("No target window selected");
        return;
    }

    AddLogMessage("Clearing blur from window 0x%p", g_hTargetWnd);

    int32_t result = blur_clear_from_window((uintptr_t)g_hTargetWnd, 0);
    
    if (result == BLUR_SUCCESS) {
        UpdateStatus("Blur cleared");
    } else {
        char* error = NULL;
        blur_get_last_error(&error);
        UpdateStatus(error ? error : "Clear failed");
        if (error) blur_free_string(error);
    }
}

void RestoreAll() {
    AddLogMessage("Restoring all windows...");
    int32_t result = blur_restore_all();
    UpdateStatus(result == BLUR_SUCCESS ? "All restored" : "Restore failed");
}

void AddLogMessage(const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    SendMessageA(g_hListLog, LB_ADDSTRING, 0, (LPARAM)buffer);
    int count = (int)SendMessage(g_hListLog, LB_GETCOUNT, 0, 0);
    SendMessage(g_hListLog, LB_SETTOPINDEX, count - 1, 0);
}

void UpdateStatus(const char* status) {
    SetWindowTextA(g_hStaticStatus, status);
}

/* Background window proc - draws colorful pattern */
LRESULT CALLBACK BackgroundWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);
            
            /* Draw colorful checkerboard pattern */
            int cellSize = 40;
            COLORREF colors[] = {
                RGB(255, 0, 0),     /* Red */
                RGB(0, 255, 0),     /* Green */
                RGB(0, 0, 255),     /* Blue */
                RGB(255, 255, 0),   /* Yellow */
                RGB(255, 0, 255),   /* Magenta */
                RGB(0, 255, 255),   /* Cyan */
                RGB(255, 128, 0),   /* Orange */
                RGB(128, 0, 255),   /* Purple */
            };
            int numColors = sizeof(colors) / sizeof(colors[0]);
            
            for (int y = 0; y < rc.bottom; y += cellSize) {
                for (int x = 0; x < rc.right; x += cellSize) {
                    int colorIdx = ((x / cellSize) + (y / cellSize)) % numColors;
                    HBRUSH hBrush = CreateSolidBrush(colors[colorIdx]);
                    RECT cellRc = {x, y, x + cellSize, y + cellSize};
                    FillRect(hdc, &cellRc, hBrush);
                    DeleteObject(hBrush);
                }
            }
            
            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            g_hBackgroundWnd = NULL;
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

/* Test window proc - transparent window for blur */
LRESULT CALLBACK TestWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);
            
            /* Draw semi-transparent background with text */
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));
            
            const char* text = "Blur Test Window\n\nLook at the pattern behind!";
            DrawTextA(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_WORDBREAK);
            
            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            g_hTestWnd = NULL;
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void DestroyTestWindows() {
    if (g_hTestWnd && IsWindow(g_hTestWnd)) {
        DestroyWindow(g_hTestWnd);
        g_hTestWnd = NULL;
    }
    if (g_hBackgroundWnd && IsWindow(g_hBackgroundWnd)) {
        DestroyWindow(g_hBackgroundWnd);
        g_hBackgroundWnd = NULL;
    }
}

void CreateTestWindows() {
    DestroyTestWindows();
    
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(g_hMainWnd, GWLP_HINSTANCE);
    
    /* Register background window class */
    static bool bgClassRegistered = false;
    if (!bgClassRegistered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = BackgroundWndProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = L"BlurBackgroundClass";
        RegisterClassExW(&wc);
        bgClassRegistered = true;
    }
    
    /* Register test window class */
    static bool testClassRegistered = false;
    if (!testClassRegistered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = TestWndProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszClassName = L"BlurTestClass";
        RegisterClassExW(&wc);
        testClassRegistered = true;
    }
    
    /* Get screen center */
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 400;
    int winH = 300;
    int posX = (screenW - winW) / 2;
    int posY = (screenH - winH) / 2;
    
    /* Create background window */
    g_hBackgroundWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        L"BlurBackgroundClass",
        L"Background (Colorful Pattern)",
        WS_POPUP | WS_VISIBLE | WS_CAPTION,
        posX, posY, winW, winH,
        NULL, NULL, hInst, NULL
    );
    
    /* Create test window on top of background */
    g_hTestWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        L"BlurTestClass",
        L"Blur Test Window",
        WS_POPUP | WS_VISIBLE | WS_CAPTION,
        posX + 50, posY + 50, winW - 100, winH - 100,
        NULL, NULL, hInst, NULL
    );
    
    if (g_hTestWnd) {
        /* Set as target */
        g_hTargetWnd = g_hTestWnd;
        UpdateTargetInfo();
        
        /* Apply blur with default params */
        EffectParams params = {};
        params.struct_version = 1;
        params.intensity = 1.0f;
        params.color_argb = 0x60000000;
        
        int32_t result = blur_apply_to_window((uintptr_t)g_hTestWnd, &params, 0);
        if (result == BLUR_SUCCESS) {
            AddLogMessage("Test windows created and blur applied");
            UpdateStatus("Look at the background pattern through the blur window!");
        } else {
            AddLogMessage("Test windows created, blur failed");
            UpdateStatus("Blur failed - check log");
        }
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            CreateControls(hWnd);
            InitializeLibrary();
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_BTN_PICK:
                    StartPicking();
                    break;
                case ID_BTN_APPLY:
                    ApplyBlur();
                    break;
                case ID_BTN_CLEAR:
                    ClearBlur();
                    break;
                case ID_BTN_RESTORE:
                    RestoreAll();
                    break;
                case ID_BTN_CREATE_TEST:
                    CreateTestWindows();
                    break;
            }
            return 0;

        case WM_DESTROY:
            DestroyTestWindows();
            blur_shutdown();
            if (g_hMouseHook) {
                UnhookWindowsHookEx(g_hMouseHook);
            }
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
