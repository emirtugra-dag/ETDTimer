#include <windows.h>
#include "app_window.h"
#include "settings.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    // Single Instance Guard
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"Global\\ETDTimerSingleInstanceMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existingHwnd = FindWindowW(L"ETDTimerWindowClass", L"ETDTimer");
        if (existingHwnd) {
            ShowWindow(existingHwnd, SW_SHOWNORMAL);
            SetForegroundWindow(existingHwnd);
        }
        return 0;
    }

    // High-DPI Awareness
    typedef BOOL(WINAPI* PFN_SetProcessDpiAwarenessContext)(HANDLE);
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        PFN_SetProcessDpiAwarenessContext pSetDpi = (PFN_SetProcessDpiAwarenessContext)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (pSetDpi) {
            pSetDpi((HANDLE)-4); // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
        }
    }

    SettingsManager::Instance().Load();

    AppWindow app;
    if (!app.Create()) {
        return 1;
    }

    app.RunMessageLoop();

    if (hMutex) CloseHandle(hMutex);
    return 0;
}
