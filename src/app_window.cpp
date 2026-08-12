#include "app_window.h"
#include "settings.h"
#include <sstream>

#define WM_TRAYICON (WM_USER + 1)
#define IDM_TRAY_SHOW 2001
#define IDM_TRAY_SETTINGS 2002
#define IDM_TRAY_EXIT 2003

AppWindow::AppWindow() {}

AppWindow::~AppWindow() {
    SaveState();
    RemoveTrayIcon();
}

bool AppWindow::Create() {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW;
    wc.lpfnWndProc = AppWindow::WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszClassName = L"ETDTimerWindowClass";

    RegisterClassExW(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int posX = screenW - 370;
    int posY = 50;

    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"ETDTimerWindowClass",
        L"ETDTimer",
        WS_POPUP | WS_VISIBLE,
        posX, posY, m_windowWidth, m_windowHeight,
        NULL, NULL, GetModuleHandle(NULL), this
    );

    if (!m_hwnd) return false;

    InitTrayIcon();

    SetTimer(m_hwnd, 1, 100, NULL);  // 100ms render & tool update timer
    SetTimer(m_hwnd, 2, 1000, NULL); // 1000ms fullscreen detection timer

    RestoreState();
    RecalculateLayout();

    return true;
}

void AppWindow::InitTrayIcon() {
    memset(&m_nid, 0, sizeof(m_nid));
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_hwnd;
    m_nid.uID = 1001;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = (HICON)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(101), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    wcscpy_s(m_nid.szTip, 128, L"ETDTimer");
    Shell_NotifyIconW(NIM_ADD, &m_nid);
}

void AppWindow::RemoveTrayIcon() {
    Shell_NotifyIconW(NIM_DELETE, &m_nid);
}

void AppWindow::ShowTrayContextMenu() {
    HMENU hMenu = CreatePopupMenu();
    if (hMenu) {
        InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, IDM_TRAY_SHOW, SettingsManager::Instance().Text("SHOW_HIDE"));
        InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_STRING, IDM_TRAY_SETTINGS, SettingsManager::Instance().Text("SETTINGS"));
        InsertMenuW(hMenu, 2, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
        InsertMenuW(hMenu, 3, MF_BYPOSITION | MF_STRING, IDM_TRAY_EXIT, SettingsManager::Instance().Text("EXIT"));

        POINT pt;
        GetCursorPos(&pt);
        SetForegroundWindow(m_hwnd);
        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hwnd, NULL);
        DestroyMenu(hMenu);
    }
}

void AppWindow::RunMessageLoop() {
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

LRESULT CALLBACK AppWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    AppWindow* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCTW* pCreate = (CREATESTRUCTW*)lParam;
        pThis = (AppWindow*)pCreate->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hwnd = hwnd;
    } else {
        pThis = (AppWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    }

    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

LRESULT AppWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            ShowTrayContextMenu();
        } else if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK) {
            BOOL vis = IsWindowVisible(m_hwnd);
            ShowWindow(m_hwnd, vis ? SW_HIDE : SW_SHOWNOACTIVATE);
            if (!vis) SetForegroundWindow(m_hwnd);
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_TRAY_SHOW:
            if (IsWindowVisible(m_hwnd)) ShowWindow(m_hwnd, SW_HIDE);
            else { ShowWindow(m_hwnd, SW_SHOWNOACTIVATE); SetForegroundWindow(m_hwnd); }
            break;
        case IDM_TRAY_SETTINGS:
            ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
            SetForegroundWindow(m_hwnd);
            m_settingsOpen = true;
            RecalculateLayout();
            break;
        case IDM_TRAY_EXIT:
            PostQuitMessage(0);
            break;
        }
        return 0;

    case WM_TIMER:
        if (wParam == 1) {
            // Update tools & redraw
            for (auto& tool : m_tools) {
                tool->Update(100);
            }
            InvalidateRect(m_hwnd, NULL, FALSE);
        } else if (wParam == 2) {
            UpdateFullscreenDetection();
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, m_windowWidth, m_windowHeight);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

        Gdiplus::Graphics g(memDC);

        // Clear background
        g.Clear(Gdiplus::Color(0, 0, 0, 0));

        // Render Clock Header
        m_renderer.RenderHeader(g, m_windowWidth, 50, m_toolsCollapsed);

        // Render active tool cards if expanded
        if (!m_toolsCollapsed) {
            int curY = 55;
            int cardW = 340;
            int colX = 0;
            int maxColH = 55;

            RECT workArea;
            SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
            int maxScreenH = workArea.bottom - workArea.top - 100;

            for (size_t i = 0; i < m_tools.size(); i++) {
                int cardH = m_tools[i]->GetHeight();
                
                // Column overflow wrapping logic (Grid mode)
                if (curY + cardH > maxScreenH || (i > 3 && colX == 0)) {
                    colX += 350;
                    curY = 55;
                }

                m_renderer.RenderToolCard(g, m_tools[i].get(), colX, curY, cardW, cardH);
                curY += cardH + 10;
                if (curY > maxColH) maxColH = curY;
            }
        }

        // Render Burger Menu popup
        if (m_menuOpen) {
            m_renderer.RenderToolMenu(g, m_windowWidth - 170, 50, 160, 110);
        }

        // Render Settings Modal overlay
        if (m_settingsOpen) {
            m_renderer.RenderSettingsModal(g, m_windowWidth, m_windowHeight);
        }

        BitBlt(hdc, 0, 0, m_windowWidth, m_windowHeight, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);

        EndPaint(m_hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        // Settings Modal Interaction
        if (m_settingsOpen) {
            int modalW = 310, modalH = 360;
            int modalX = (m_windowWidth - modalW) / 2;
            int modalY = (m_windowHeight - modalH) / 2;

            // Close X
            if (x >= modalX + modalW - 30 && x <= modalX + modalW && y >= modalY && y <= modalY + 30) {
                m_settingsOpen = false;
                SettingsManager::Instance().Save();
                RecalculateLayout();
                return 0;
            }

            int curY = modalY + 45;
            AppSettings& s = SettingsManager::Instance().Get();

            // Language switch
            if (y >= curY && y <= curY + 28) {
                s.lang = (s.lang == LANG_TR) ? LANG_EN : LANG_TR;
                InvalidateRect(m_hwnd, NULL, FALSE);
                return 0;
            }
            curY += 32;

            // Theme switch
            if (y >= curY && y <= curY + 28) {
                s.theme = (s.theme == THEME_DARK) ? THEME_LIGHT : THEME_DARK;
                InvalidateRect(m_hwnd, NULL, FALSE);
                return 0;
            }
            curY += 32;

            // Show Seconds Clock
            if (y >= curY && y <= curY + 28) {
                s.showSecondsInClock = !s.showSecondsInClock;
                InvalidateRect(m_hwnd, NULL, FALSE);
                return 0;
            }
            curY += 32;

            // Show Seconds Timer
            if (y >= curY && y <= curY + 28) {
                s.showSecondsInTimer = !s.showSecondsInTimer;
                InvalidateRect(m_hwnd, NULL, FALSE);
                return 0;
            }
            curY += 32;

            // Force Always on Top
            if (y >= curY && y <= curY + 28) {
                s.forceAlwaysOnTop = !s.forceAlwaysOnTop;
                if (s.forceAlwaysOnTop) {
                    SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
                }
                InvalidateRect(m_hwnd, NULL, FALSE);
                return 0;
            }
            curY += 32;

            // Autostart
            if (y >= curY && y <= curY + 28) {
                s.autoStart = !s.autoStart;
                SettingsManager::Instance().SetAutoStart(s.autoStart);
                InvalidateRect(m_hwnd, NULL, FALSE);
                return 0;
            }
            curY += 32;

            // Restore previous tools
            if (y >= curY && y <= curY + 28) {
                s.restorePreviousTools = !s.restorePreviousTools;
                InvalidateRect(m_hwnd, NULL, FALSE);
                return 0;
            }
            curY += 32;

            // Legal notice popup
            if (y >= curY && y <= curY + 32) {
                const wchar_t* legalText = (s.lang == LANG_TR) ? g_LegalDisclaimerTR : g_LegalDisclaimerEN;
                MessageBoxW(m_hwnd, legalText, SettingsManager::Instance().Text("LEGAL_NOTICE"), MB_OK | MB_ICONINFORMATION);
                return 0;
            }

            return 0;
        }

        // Header controls (y <= 50)
        if (y <= 50) {
            // Burger menu click [x: m_windowWidth - 110 .. m_windowWidth - 80]
            if (x >= m_windowWidth - 110 && x <= m_windowWidth - 80) {
                m_menuOpen = !m_menuOpen;
                RecalculateLayout();
                return 0;
            }
            // Eye button click [x: m_windowWidth - 75 .. m_windowWidth - 45]
            if (x >= m_windowWidth - 75 && x <= m_windowWidth - 45) {
                ToggleToolsCollapse();
                return 0;
            }
            // Gear button click [x: m_windowWidth - 40 .. m_windowWidth - 10]
            if (x >= m_windowWidth - 40 && x <= m_windowWidth - 10) {
                m_settingsOpen = !m_settingsOpen;
                RecalculateLayout();
                return 0;
            }

            // Drag window
            m_dragging = true;
            POINT pt;
            GetCursorPos(&pt);
            RECT rect;
            GetWindowRect(m_hwnd, &rect);
            m_dragStartOffset.x = pt.x - rect.left;
            m_dragStartOffset.y = pt.y - rect.top;
            SetCapture(m_hwnd);
            return 0;
        }

        // Menu selection interaction
        if (m_menuOpen) {
            if (x >= m_windowWidth - 170 && x <= m_windowWidth - 10) {
                if (y >= 45 && y <= 80) AddTool(TOOL_STOPWATCH);
                else if (y >= 80 && y <= 112) AddTool(TOOL_TIMER);
                else if (y >= 112 && y <= 145) AddTool(TOOL_POMODORO);
            }
            m_menuOpen = false;
            RecalculateLayout();
            return 0;
        }

        // Tool cards interaction
        if (!m_toolsCollapsed) {
            int curY = 55;
            int colX = 0;
            RECT workArea;
            SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
            int maxScreenH = workArea.bottom - workArea.top - 100;

            for (size_t i = 0; i < m_tools.size(); i++) {
                int cardH = m_tools[i]->GetHeight();
                if (curY + cardH > maxScreenH || (i > 3 && colX == 0)) {
                    colX += 350;
                    curY = 55;
                }

                if (x >= colX && x <= colX + 340 && y >= curY && y <= curY + cardH) {
                    // Close button (X) click
                    if (x >= colX + 305 && x <= colX + 335 && y >= curY + 5 && y <= curY + 30) {
                        RemoveTool(m_tools[i]->GetId());
                        return 0;
                    }

                    // Card inner interaction
                    m_tools[i]->OnLButtonDown(x - colX, y - curY);
                    InvalidateRect(m_hwnd, NULL, FALSE);
                    return 0;
                }

                curY += cardH + 10;
            }
        }
        return 0;
    }

    case WM_MOUSEMOVE:
        if (m_dragging) {
            POINT pt;
            GetCursorPos(&pt);
            SetWindowPos(m_hwnd, NULL, pt.x - m_dragStartOffset.x, pt.y - m_dragStartOffset.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return 0;

    case WM_LBUTTONUP:
        if (m_dragging) {
            m_dragging = false;
            ReleaseCapture();
        }
        return 0;

    case WM_CHAR: {
        wchar_t ch = (wchar_t)wParam;
        for (auto& tool : m_tools) {
            tool->OnCharInput(ch);
        }
        InvalidateRect(m_hwnd, NULL, FALSE);
        return 0;
    }

    case WM_DESTROY:
        SaveState();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(m_hwnd, uMsg, wParam, lParam);
}

bool AppWindow::AddTool(ToolType type) {
    if (m_tools.size() >= 10) {
        MessageBoxW(m_hwnd, SettingsManager::Instance().Text("MAX_TOOL_LIMIT"), SettingsManager::Instance().Text("APP_TITLE"), MB_OK | MB_ICONWARNING);
        return false;
    }

    int countOfThisType = 0;
    for (const auto& tool : m_tools) {
        if (tool->GetType() == type) countOfThisType++;
    }

    if (countOfThisType >= 3) {
        MessageBoxW(m_hwnd, SettingsManager::Instance().Text("MAX_TOOL_LIMIT"), SettingsManager::Instance().Text("APP_TITLE"), MB_OK | MB_ICONWARNING);
        return false;
    }

    if (type == TOOL_STOPWATCH) m_tools.push_back(std::make_unique<StopwatchTool>());
    else if (type == TOOL_TIMER) m_tools.push_back(std::make_unique<TimerTool>());
    else if (type == TOOL_POMODORO) m_tools.push_back(std::make_unique<PomodoroTool>());

    RecalculateLayout();
    return true;
}

void AppWindow::RemoveTool(int cardId) {
    for (auto it = m_tools.begin(); it != m_tools.end(); ++it) {
        if ((*it)->GetId() == cardId) {
            m_tools.erase(it);
            break;
        }
    }
    RecalculateLayout();
}

void AppWindow::ToggleToolsCollapse() {
    m_toolsCollapsed = !m_toolsCollapsed;
    RecalculateLayout();
}

void AppWindow::RecalculateLayout() {
    if (m_toolsCollapsed || m_tools.empty()) {
        m_windowWidth = 340;
        m_windowHeight = 50;
    } else {
        RECT workArea;
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
        int maxScreenH = workArea.bottom - workArea.top - 100;

        int curY = 55;
        int maxH = 55;
        int numCols = 1;

        for (size_t i = 0; i < m_tools.size(); i++) {
            int cardH = m_tools[i]->GetHeight();
            if (curY + cardH > maxScreenH || (i > 3 && numCols == 1)) {
                numCols++;
                curY = 55;
            }
            curY += cardH + 10;
            if (curY > maxH) maxH = curY;
        }

        m_windowWidth = numCols * 350 - 10;
        m_windowHeight = maxH;
    }

    // Expand window dimensions dynamically if Settings modal or Burger menu is open!
    if (m_settingsOpen) {
        if (m_windowWidth < 340) m_windowWidth = 340;
        if (m_windowHeight < 390) m_windowHeight = 390;
    }
    if (m_menuOpen) {
        if (m_windowWidth < 340) m_windowWidth = 340;
        if (m_windowHeight < 165) m_windowHeight = 165;
    }

    SetWindowPos(m_hwnd, NULL, 0, 0, m_windowWidth, m_windowHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    InvalidateRect(m_hwnd, NULL, FALSE);
}

void AppWindow::UpdateFullscreenDetection() {
    if (SettingsManager::Instance().Get().forceAlwaysOnTop) {
        if (m_isFullscreenActive) {
            m_isFullscreenActive = false;
            ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
        }
        SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
        return;
    }

    HWND fgHwnd = GetForegroundWindow();
    if (!fgHwnd || fgHwnd == m_hwnd) return;

    wchar_t className[256];
    GetClassNameW(fgHwnd, className, 256);
    if (wcscmp(className, L"Progman") == 0 || wcscmp(className, L"WorkerW") == 0 || wcscmp(className, L"Shell_TrayWnd") == 0) {
        if (m_isFullscreenActive) {
            m_isFullscreenActive = false;
            SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        }
        return;
    }

    RECT fgRect;
    GetWindowRect(fgHwnd, &fgRect);

    HMONITOR hMon = MonitorFromWindow(fgHwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(MONITORINFO) };
    GetMonitorInfoW(hMon, &mi);

    // True Fullscreen: The window bounds cover or exceed the ENTIRE physical monitor (including taskbar region)
    bool isFullscreen = (fgRect.left <= mi.rcMonitor.left && 
                         fgRect.top <= mi.rcMonitor.top && 
                         fgRect.right >= mi.rcMonitor.right && 
                         fgRect.bottom >= mi.rcMonitor.bottom);

    if (isFullscreen && !m_isFullscreenActive) {
        m_isFullscreenActive = true;
        SetWindowPos(m_hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        ShowWindow(m_hwnd, SW_HIDE);
    } else if (!isFullscreen && m_isFullscreenActive) {
        m_isFullscreenActive = false;
        ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
        SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    }
}

void AppWindow::SaveState() {
    if (!SettingsManager::Instance().Get().restorePreviousTools) return;

    std::string toolList = "";
    for (const auto& tool : m_tools) {
        if (!toolList.empty()) toolList += ",";
        if (tool->GetType() == TOOL_STOPWATCH) toolList += "STOPWATCH";
        else if (tool->GetType() == TOOL_TIMER) toolList += "TIMER";
        else if (tool->GetType() == TOOL_POMODORO) toolList += "POMODORO";
    }
    SettingsManager::Instance().Get().lastOpenTools = toolList;
    SettingsManager::Instance().Save();
}

void AppWindow::RestoreState() {
    SettingsManager::Instance().Load();
    if (!SettingsManager::Instance().Get().restorePreviousTools) return;

    std::string toolList = SettingsManager::Instance().Get().lastOpenTools;
    if (toolList.empty()) return;

    std::stringstream ss(toolList);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (item == "STOPWATCH") AddTool(TOOL_STOPWATCH);
        else if (item == "TIMER") AddTool(TOOL_TIMER);
        else if (item == "POMODORO") AddTool(TOOL_POMODORO);
    }
}
