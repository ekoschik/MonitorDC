
#include "stdafx.h"
#include <windows.h>

const SIZE defSize = { 600, 400 };
const COLORREF rgbBackground = RGB(0, 0, 255);
UINT GetDpiForWindow(HWND hwnd);
UINT SetPMAware();

void DrawOnMonitorDC(HWND hwnd)
{
    MONITORINFOEX mi;
    mi.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi);
    HDC hdc = CreateDC(mi.szDevice, NULL, NULL, NULL);
    if (hdc == NULL) {
        printf("Failed to get monitor DC!\n");
    }

    SelectObject(hdc, GetStockObject(BLACK_PEN));
    SelectObject(hdc, GetStockObject(DKGRAY_BRUSH));
    
    RECT rc;
    GetWindowRect(hwnd, &rc);
    if (!Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom)) {
        printf("Rectangle() failed! hdc %X\n", (UINT)hdc);
    } else {
        printf("rect(%i, %i, %i, %i)\n",
            rc.left, rc.top, rc.right, rc.bottom);
    }

    DeleteDC(hdc);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        int dpi = GetDpiForWindow(hwnd);
        SetWindowPos(hwnd, NULL, 0, 0,
            MulDiv(defSize.cx, dpi, 96),
            MulDiv(defSize.cy, dpi, 96),
            SWP_NOMOVE | SWP_SHOWWINDOW);
        break;
    }

    case WM_DPICHANGED:
    {
        RECT* prc = (RECT*)lParam;
        SetWindowPos(hwnd, nullptr,
            prc->left,
            prc->top,
            prc->right - prc->left,
            prc->bottom - prc->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        break;
    }

    case WM_LBUTTONDOWN:
        DrawOnMonitorDC(hwnd);
        break;

    case WM_PAINT:
    {
        static HBRUSH hbr = CreateSolidBrush(rgbBackground);

        PAINTSTRUCT ps;
        RECT rc;
        HDC hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, hbr);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

int main()
{
    SetPMAware();

    LPCWSTR szClass = L"class";
    LPCWSTR szTitle = L"title";
    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW /*| CS_DBLCLKS*/;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = szClass;

    if (!RegisterClassExW(&wcex)) {
        printf("RegisterWindow failed, bailing...\n");
        return 1;
    }

    HWND hwnd = CreateWindow(
        szClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd)
    {
        printf("CreateWindow failed, bailing...\n");
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

UINT SetPMAware()
{
#define DPI_AWARENESS_CONTEXT DWORD
#define DPI_AWARENESS_CONTEXT_UNAWARE              ((DPI_AWARENESS_CONTEXT)-1)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
    typedef DPI_AWARENESS_CONTEXT(WINAPI *fnSTDAC)(DPI_AWARENESS_CONTEXT);
    static fnSTDAC pfn = nullptr;
    if (pfn == nullptr) {
        static HMODULE hModUser32 = GetModuleHandle(_T("user32.dll"));
        pfn = (fnSTDAC)GetProcAddress(hModUser32, "SetThreadDpiAwarenessContext");
        if (pfn == nullptr) {
            printf("ERROR! Can't find SetThreadDpiAwarenessContext!\n");
            return 0;
        }
    }
    return pfn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
}

UINT GetDpiForWindow(HWND hwnd)
{
    typedef UINT(WINAPI *fnGetDpiForWindow)(HWND);
    static fnGetDpiForWindow pfn = nullptr;
    if (!pfn) {
        static HMODULE hModUser32 = GetModuleHandle(_T("user32.dll"));
        pfn = (fnGetDpiForWindow)GetProcAddress(hModUser32, "GetDpiForWindow");
        if (!pfn) {
            printf("Can't find GetDpiForWindow.\n");
            return 96;
        }
    }
    return pfn(hwnd);
}
