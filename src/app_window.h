#ifndef APP_WINDOW_H
#define APP_WINDOW_H

#include <windows.h>
#include <vector>
#include <memory>
#include "tools.h"
#include "ui_renderer.h"

class AppWindow {
public:
    AppWindow();
    ~AppWindow();

    bool Create();
    void RunMessageLoop();

    HWND GetHWND() const { return m_hwnd; }

    bool AddTool(ToolType type);
    void RemoveTool(int cardId);
    void ToggleToolsCollapse();

private:
    void InitTrayIcon();
    void RemoveTrayIcon();
    void ShowTrayContextMenu();

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void RecalculateLayout();
    void UpdateWindowRegion();
    void UpdateFullscreenDetection();
    void SaveState();
    void RestoreState();

    HWND m_hwnd = NULL;
    NOTIFYICONDATAW m_nid = {0};
    UIRenderer m_renderer;

    std::vector<std::unique_ptr<ToolCard>> m_tools;
    bool m_toolsCollapsed = false;
    bool m_menuOpen = false;
    bool m_settingsOpen = false;
    bool m_dragging = false;
    POINT m_dragStartOffset = {0, 0};

    bool m_isFullscreenActive = false;

    int m_windowWidth = 360;
    int m_windowHeight = 50; // Initial clock header height
    int m_logicalWidth = 360;
    int m_logicalHeight = 50;
};

#endif // APP_WINDOW_H
