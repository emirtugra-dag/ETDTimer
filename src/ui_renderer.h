#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include "tools.h"
#include "settings.h"

class UIRenderer {
public:
    UIRenderer();
    ~UIRenderer();

    void InitGDIPlus();
    void ShutdownGDIPlus();

    void SetMousePos(int x, int y) { m_mouseX = x; m_mouseY = y; }
    void SetBlinkState(bool show) { m_blinkOn = show; }

    void RenderBackground(Gdiplus::Graphics& g, int width, int height);
    void RenderHeader(Gdiplus::Graphics& g, int width, int height, bool toolsCollapsed);
    void RenderToolCard(Gdiplus::Graphics& g, ToolCard* card, int x, int y, int width, int height);
    void RenderEmptyStateCard(Gdiplus::Graphics& g, int x, int y, int width, int height);
    void RenderToolMenu(Gdiplus::Graphics& g, int x, int y, int width, int height);
    void RenderSettingsModal(Gdiplus::Graphics& g, int width, int height);

private:
    ULONG_PTR m_gdiplusToken = 0;
    int m_mouseX = -1;
    int m_mouseY = -1;
    bool m_blinkOn = true;
    
    // Theme brushes & pens
    Gdiplus::SolidBrush* m_bgBrush = nullptr;
    Gdiplus::SolidBrush* m_cardBrush = nullptr;
    Gdiplus::SolidBrush* m_textBrush = nullptr;
    Gdiplus::SolidBrush* m_textDimBrush = nullptr;
    Gdiplus::SolidBrush* m_accentBrush = nullptr;
    Gdiplus::SolidBrush* m_buttonBrush = nullptr;
    Gdiplus::SolidBrush* m_buttonHoverBrush = nullptr;
    Gdiplus::Pen* m_borderPen = nullptr;
    Gdiplus::Pen* m_accentPen = nullptr;

    void UpdateThemeBrushes();
};

#endif // UI_RENDERER_H
