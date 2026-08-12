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

    void RenderHeader(Gdiplus::Graphics& g, int width, int height, bool toolsCollapsed);
    void RenderToolCard(Gdiplus::Graphics& g, ToolCard* card, int x, int y, int width, int height);
    void RenderToolMenu(Gdiplus::Graphics& g, int x, int y, int width, int height);
    void RenderSettingsModal(Gdiplus::Graphics& g, int width, int height);

private:
    ULONG_PTR m_gdiplusToken = 0;
    
    // Theme brushes & pens
    Gdiplus::SolidBrush* m_bgBrush = nullptr;
    Gdiplus::SolidBrush* m_cardBrush = nullptr;
    Gdiplus::SolidBrush* m_textBrush = nullptr;
    Gdiplus::SolidBrush* m_textDimBrush = nullptr;
    Gdiplus::SolidBrush* m_accentBrush = nullptr;
    Gdiplus::SolidBrush* m_buttonBrush = nullptr;
    Gdiplus::Pen* m_borderPen = nullptr;

    void UpdateThemeBrushes();
};

#endif // UI_RENDERER_H
