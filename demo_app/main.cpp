/*
 * Blur Demo Application - UI Polished Version
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    SetProcessDPIAware();
    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wc = { sizeof(wc), 0, WndProc, 0, 0, hInstance, LoadIcon(NULL, IDI_APPLICATION), LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1), NULL, L"BlurDemoClass", NULL };
    RegisterClassExW(&wc);

    /* Increased window size to 600x750 */
    g_hMainWnd = CreateWindowExW(0, L"BlurDemoClass", L"Blur Library Demo", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 600, 750, NULL, NULL, hInstance, NULL);
    if (!g_hMainWnd) return 1;

    ShowWindow(g_hMainWnd, nCmdShow); UpdateWindow(g_hMainWnd);

    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return (int)msg.wParam;
}

void CreateControls(HWND hWnd) {
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
    int y = 15;
    int labelW = 120;
    int controlX = 140;
    int controlW = 420;

    CreateWindowW(L"STATIC", L"Capabilities:", WS_CHILD | WS_VISIBLE, 15, y, labelW, 25, hWnd, NULL, hInst, NULL);
    g_hStaticCaps = CreateWindowW(L"STATIC", L"Initializing...", WS_CHILD | WS_VISIBLE | SS_LEFT, controlX, y, controlW, 45, hWnd, (HMENU)ID_STATIC_CAPS, hInst, NULL);
    y += 55;

    CreateWindowW(L"STATIC", L"Target HWND:", WS_CHILD | WS_VISIBLE, 15, y, labelW, 25, hWnd, NULL, hInst, NULL);
    g_hStaticHwnd = CreateWindowW(L"STATIC", L"(none)", WS_CHILD | WS_VISIBLE | SS_LEFT, controlX, y, controlW, 25, hWnd, (HMENU)ID_STATIC_HWND, hInst, NULL);
    y += 40;

    CreateWindowW(L"BUTTON", L"Pick Window (Click)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 15, y, 180, 40, hWnd, (HMENU)ID_BTN_PICK, hInst, NULL);
    CreateWindowW(L"BUTTON", L"Create Test Windows", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 210, y, 200, 40, hWnd, (HMENU)ID_BTN_CREATE_TEST, hInst, NULL);
    y += 55;

    CreateWindowW(L"STATIC", L"Intensity:", WS_CHILD | WS_VISIBLE, 15, y, labelW, 25, hWnd, NULL, hInst, NULL);
    g_hSliderIntensity = CreateWindowW(TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS, controlX, y, controlW, 40, hWnd, (HMENU)ID_SLIDER_INTENSITY, hInst, NULL);
    SendMessage(g_hSliderIntensity, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    SendMessage(g_hSliderIntensity, TBM_SETPOS, TRUE, 100);
    y += 55;

    CreateWindowW(L"STATIC", L"Color (ARGB):", WS_CHILD | WS_VISIBLE, 15, y, labelW, 25, hWnd, NULL, hInst, NULL);
    g_hEditColor = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"80000000", WS_CHILD | WS_VISIBLE | ES_LEFT, controlX, y, 150, 30, hWnd, (HMENU)ID_EDIT_COLOR, hInst, NULL);
    y += 50;

    CreateWindowW(L"BUTTON", L"Apply Blur", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 15, y, 120, 40, hWnd, (HMENU)ID_BTN_APPLY, hInst, NULL);
    CreateWindowW(L"BUTTON", L"Clear Blur", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 145, y, 120, 40, hWnd, (HMENU)ID_BTN_CLEAR, hInst, NULL);
    CreateWindowW(L"BUTTON", L"Restore All", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 275, y, 120, 40, hWnd, (HMENU)ID_BTN_RESTORE, hInst, NULL);
    y += 60;

    CreateWindowW(L"STATIC", L"Status:", WS_CHILD | WS_VISIBLE, 15, y, 70, 25, hWnd, NULL, hInst, NULL);
    g_hStaticStatus = CreateWindowW(L"STATIC", L"Ready", WS_CHILD | WS_VISIBLE | SS_LEFT, 95, y, 460, 25, hWnd, (HMENU)ID_STATIC_STATUS, hInst, NULL);
    y += 40;

    CreateWindowW(L"STATIC", L"Log:", WS_CHILD | WS_VISIBLE, 15, y, 70, 25, hWnd, NULL, hInst, NULL);
    y += 30;
    g_hListLog = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT, 15, y, 550, 250, hWnd, (HMENU)ID_LIST_LOG, hInst, NULL);
}

void InitializeLibrary() {
    blur_set_log_level(BLUR_LOG_DEBUG); blur_set_log_callback(LogCallback, NULL);
    if (blur_init(&g_capabilities) == BLUR_SUCCESS) {
        char caps[256]; snprintf(caps, 256, "0x%04X - %s%s%s%s%s", g_capabilities, (g_capabilities&BLUR_CAP_SETWINDOWCOMPOSITION)?"Comp ":"", (g_capabilities&BLUR_CAP_DWM_BLUR)?"DWM ":"", (g_capabilities&BLUR_CAP_COLOR_CONTROL)?"Col ":"", (g_capabilities&BLUR_CAP_ANIMATION_CONTROL)?"Anim ":"", (g_capabilities&BLUR_CAP_D2D_BLUR)?"D2D":"");
        SetWindowTextA(g_hStaticCaps, caps); AddLogMessage("Library initialized");
    }
}

LRESULT CALLBACK PickerProc(int n, WPARAM w, LPARAM l) {
    if (n >= 0 && g_picking && w == WM_LBUTTONDOWN) {
        HWND hr = GetAncestor(WindowFromPoint(((MOUSEHOOKSTRUCT*)l)->pt), GA_ROOT);
        if (hr && hr != g_hMainWnd) { g_hTargetWnd = hr; UpdateTargetInfo(); AddLogMessage("Selected: 0x%p", hr); }
        g_picking = false; ReleaseCapture(); UnhookWindowsHookEx(g_hMouseHook); g_hMouseHook = NULL; UpdateStatus("Window selected"); return 1;
    }
    return CallNextHookEx(g_hMouseHook, n, w, l);
}

void StartPicking() { g_picking = true; SetCursor(LoadCursor(NULL, IDC_CROSS)); UpdateStatus("Click window..."); g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, PickerProc, NULL, 0); }
void UpdateTargetInfo() {
    if (g_hTargetWnd && IsWindow(g_hTargetWnd)) { wchar_t t[256]; char i[512]; GetWindowTextW(g_hTargetWnd, t, 256); snprintf(i, 512, "0x%p - %ls", g_hTargetWnd, t); SetWindowTextA(g_hStaticHwnd, i); }
    else SetWindowTextA(g_hStaticHwnd, "(none)");
}
void ApplyBlur() {
    if (!g_hTargetWnd) return; EffectParams p = { 1, (int)SendMessage(g_hSliderIntensity, TBM_GETPOS, 0, 0)/100.0f };
    char ct[32]; GetWindowTextA(g_hEditColor, ct, 32); p.color_argb = (uint32_t)strtoul(ct, NULL, 16);
    if (blur_apply_to_window((uintptr_t)g_hTargetWnd, &p, 0) == BLUR_SUCCESS) UpdateStatus("Applied");
}
void ClearBlur() { if (g_hTargetWnd && blur_clear_from_window((uintptr_t)g_hTargetWnd, 0) == BLUR_SUCCESS) UpdateStatus("Cleared"); }
void RestoreAll() { blur_restore_all(); UpdateStatus("Restored All"); }
void AddLogMessage(const char* f, ...) { char b[512]; va_list a; va_start(a, f); vsnprintf(b, 512, f, a); va_end(a); SendMessageA(g_hListLog, LB_ADDSTRING, 0, (LPARAM)b); SendMessage(g_hListLog, LB_SETTOPINDEX, (int)SendMessage(g_hListLog, LB_GETCOUNT, 0, 0)-1, 0); }
void UpdateStatus(const char* s) { SetWindowTextA(g_hStaticStatus, s); }

LRESULT CALLBACK BackgroundWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) { PAINTSTRUCT ps; HDC hdc = BeginPaint(h, &ps); RECT rc; GetClientRect(h, &rc);
        int cs = 40; COLORREF cls[] = { RGB(255,0,0), RGB(0,255,0), RGB(0,0,255), RGB(255,255,0), RGB(255,0,255), RGB(0,255,255), RGB(255,128,0), RGB(128,0,255) };
        for (int y = 0; y < rc.bottom; y += cs) for (int x = 0; x < rc.right; x += cs) {
            HBRUSH br = CreateSolidBrush(cls[((x/cs)+(y/cs))%8]); RECT cr = {x, y, x+cs, y+cs}; FillRect(hdc, &cr, br); DeleteObject(br);
        }
        EndPaint(h, &ps); return 0;
    }
    if (m == WM_NCHITTEST) return HTCAPTION;
    return DefWindowProc(h, m, w, l);
}

LRESULT CALLBACK TestWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) { PAINTSTRUCT ps; HDC hdc = BeginPaint(h, &ps); RECT rc; GetClientRect(h, &rc);
        SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, RGB(255,255,255));
        const char* t = "Blur Test Window\n\nDrag anywhere to move!"; DrawTextA(hdc, t, -1, &rc, DT_CENTER|DT_VCENTER|DT_WORDBREAK);
        EndPaint(h, &ps); return 0;
    }
    if (m == WM_NCHITTEST) return HTCAPTION;
    return DefWindowProc(h, m, w, l);
}

void DestroyTestWindows() { if (g_hTestWnd) DestroyWindow(g_hTestWnd); if (g_hBackgroundWnd) DestroyWindow(g_hBackgroundWnd); g_hTestWnd = g_hBackgroundWnd = NULL; }

void CreateTestWindows() {
    DestroyTestWindows(); HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(g_hMainWnd, GWLP_HINSTANCE);
    WNDCLASSEXW wbg = { sizeof(wbg), 0, BackgroundWndProc, 0, 0, hInst, LoadIcon(NULL, IDI_APPLICATION), LoadCursor(NULL, IDC_ARROW), NULL, NULL, L"BlurBg", NULL };
    RegisterClassExW(&wbg);
    WNDCLASSEXW wts = { sizeof(wts), 0, TestWndProc, 0, 0, hInst, LoadIcon(NULL, IDI_APPLICATION), LoadCursor(NULL, IDC_ARROW), (HBRUSH)GetStockObject(BLACK_BRUSH), NULL, L"BlurTs", NULL };
    RegisterClassExW(&wts);

    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN), ww = 600, wh = 450, px = (sw - ww) / 2, py = (sh - wh) / 2;
    g_hBackgroundWnd = CreateWindowExW(0, L"BlurBg", L"Bg", WS_POPUP | WS_VISIBLE, px, py, ww, wh, NULL, NULL, hInst, NULL);
    g_hTestWnd = CreateWindowExW(WS_EX_TOPMOST, L"BlurTs", L"Blur", WS_POPUP | WS_VISIBLE, px + 50, py + 50, ww - 100, wh - 100, NULL, NULL, hInst, NULL);
    
    if (g_hTestWnd) {
        g_hTargetWnd = g_hTestWnd; UpdateTargetInfo();
        EffectParams p = { 1, 1.0f, 0x60000000 }; blur_apply_to_window((uintptr_t)g_hTestWnd, &p, 0);
    }
}

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
        case WM_CREATE: CreateControls(h); InitializeLibrary(); break;
        case WM_COMMAND: switch (LOWORD(w)) { case ID_BTN_PICK: StartPicking(); break; case ID_BTN_APPLY: ApplyBlur(); break; case ID_BTN_CLEAR: ClearBlur(); break; case ID_BTN_RESTORE: RestoreAll(); break; case ID_BTN_CREATE_TEST: CreateTestWindows(); break; } break;
        case WM_DESTROY: DestroyTestWindows(); blur_shutdown(); PostQuitMessage(0); break;
    }
    return DefWindowProc(h, m, w, l);
}
