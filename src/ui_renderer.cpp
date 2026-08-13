#include "ui_renderer.h"
#include "settings.h"
#include <cwchar>
#include <cstdio>

using namespace Gdiplus;

UIRenderer::UIRenderer() {
    InitGDIPlus();
    UpdateThemeBrushes();
}

UIRenderer::~UIRenderer() {
    delete m_bgBrush;
    delete m_cardBrush;
    delete m_textBrush;
    delete m_textDimBrush;
    delete m_accentBrush;
    delete m_buttonBrush;
    delete m_borderPen;
    ShutdownGDIPlus();
}

void UIRenderer::InitGDIPlus() {
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
}

void UIRenderer::ShutdownGDIPlus() {
    if (m_gdiplusToken) {
        GdiplusShutdown(m_gdiplusToken);
        m_gdiplusToken = 0;
    }
}

void UIRenderer::UpdateThemeBrushes() {
    delete m_bgBrush;
    delete m_cardBrush;
    delete m_textBrush;
    delete m_textDimBrush;
    delete m_accentBrush;
    delete m_buttonBrush;
    delete m_buttonHoverBrush;
    delete m_borderPen;
    delete m_accentPen;

    bool dark = (SettingsManager::Instance().Get().theme == THEME_DARK);

    if (dark) {
        m_bgBrush = new SolidBrush(Color(240, 20, 20, 24));
        m_cardBrush = new SolidBrush(Color(255, 32, 32, 38));
        m_textBrush = new SolidBrush(Color(255, 240, 240, 245));
        m_textDimBrush = new SolidBrush(Color(255, 150, 150, 160));
        m_accentBrush = new SolidBrush(Color(255, 0, 173, 181));
        m_buttonBrush = new SolidBrush(Color(255, 45, 45, 55));
        m_buttonHoverBrush = new SolidBrush(Color(255, 65, 65, 80));
        m_borderPen = new Pen(Color(255, 55, 55, 65), 1.0f);
        m_accentPen = new Pen(Color(255, 0, 173, 181), 1.5f);
    } else {
        m_bgBrush = new SolidBrush(Color(240, 245, 246, 248));
        m_cardBrush = new SolidBrush(Color(255, 255, 255, 255));
        m_textBrush = new SolidBrush(Color(255, 20, 24, 33));
        m_textDimBrush = new SolidBrush(Color(255, 100, 105, 115));
        m_accentBrush = new SolidBrush(Color(255, 0, 120, 212));
        m_buttonBrush = new SolidBrush(Color(255, 230, 235, 242));
        m_buttonHoverBrush = new SolidBrush(Color(255, 210, 220, 235));
        m_borderPen = new Pen(Color(255, 210, 215, 225), 1.0f);
        m_accentPen = new Pen(Color(255, 0, 120, 212), 1.5f);
    }
}

void UIRenderer::RenderBackground(Graphics& g, int width, int height) {
    UpdateThemeBrushes();
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    RectF bgRect(0, 0, (REAL)width, (REAL)height);
    g.FillRectangle(m_bgBrush, bgRect);
}

void UIRenderer::RenderEmptyStateCard(Graphics& g, int x, int y, int width, int height) {
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    RectF cardRect((REAL)x, (REAL)y, (REAL)width, (REAL)height);
    g.FillRectangle(m_cardBrush, cardRect);
    g.DrawRectangle(m_borderPen, cardRect);

    Font titleFont(L"Segoe UI", 9, FontStyleBold, UnitPoint);
    Font textFont(L"Segoe UI", 8, FontStyleRegular, UnitPoint);

    g.DrawString(SettingsManager::Instance().Text("ADD_TOOL_HINT"), -1, &titleFont, PointF((REAL)x + 10, (REAL)y + 20), m_textBrush);
    g.DrawString(SettingsManager::Instance().Text("CHOOSE_FROM_MENU"), -1, &textFont, PointF((REAL)x + 10, (REAL)y + 55), m_textDimBrush);
}

void UIRenderer::RenderHeader(Graphics& g, int width, int height, bool toolsCollapsed) {
    UpdateThemeBrushes();
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    // Draw main background header card
    RectF rect(0, 0, (REAL)width, (REAL)height);
    g.FillRectangle(m_cardBrush, rect);
    g.DrawRectangle(m_borderPen, rect);

    // Get current local time
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t timeBuf[64];
    if (SettingsManager::Instance().Get().showSecondsInClock) {
        swprintf_s(timeBuf, 64, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
    } else {
        swprintf_s(timeBuf, 64, L"%02d:%02d", st.wHour, st.wMinute);
    }

    // Render App Name / Clock text
    Font clockFont(L"Segoe UI", 20, FontStyleBold, UnitPoint);
    g.DrawString(timeBuf, -1, &clockFont, PointF(15, 6), m_textBrush);

    // Render Top Control Buttons
    Font btnFont(L"Segoe UI Symbol", 13, FontStyleRegular, UnitPoint);
    
    if (SettingsManager::Instance().Get().clockOnlyMode) {
        // Clock Only Mode: Show Clock, Settings Gear (⚙), and Close App (✕) buttons!
        // ⚙ Settings Gear Button [x: width - 74]
        RectF gearBtn((REAL)width - 74, 10, 28, 28);
        bool hoverGear = (m_mouseX >= width - 74 && m_mouseX <= width - 46 && m_mouseY >= 10 && m_mouseY <= 38);
        g.FillRectangle(hoverGear ? m_buttonHoverBrush : m_buttonBrush, gearBtn);
        g.DrawRectangle(hoverGear ? m_accentPen : m_borderPen, gearBtn);
        g.DrawString(L"⚙", -1, &btnFont, PointF((REAL)width - 70, 12), m_textBrush);

        // ✕ Close App Button [x: width - 40]
        RectF closeBtn((REAL)width - 40, 10, 28, 28);
        bool hoverClose = (m_mouseX >= width - 40 && m_mouseX <= width - 12 && m_mouseY >= 10 && m_mouseY <= 38);
        SolidBrush redBrush(Color(255, 235, 87, 87));
        g.FillRectangle(hoverClose ? m_buttonHoverBrush : m_buttonBrush, closeBtn);
        g.DrawRectangle(hoverClose ? m_accentPen : m_borderPen, closeBtn);
        g.DrawString(L"✕", -1, &btnFont, PointF((REAL)width - 35, 12), hoverClose ? &redBrush : m_textBrush);
        return;
    }

    // ☰ Burger Menu Button [x: width - 110]
    RectF burgerBtn((REAL)width - 110, 10, 28, 28);
    bool hoverBurger = (m_mouseX >= width - 110 && m_mouseX <= width - 82 && m_mouseY >= 10 && m_mouseY <= 38);
    g.FillRectangle(hoverBurger ? m_buttonHoverBrush : m_buttonBrush, burgerBtn);
    g.DrawRectangle(hoverBurger ? m_accentPen : m_borderPen, burgerBtn);
    g.DrawString(L"☰", -1, &btnFont, PointF((REAL)width - 105, 12), m_textBrush);

    // 👁 Eye (Hide/Show) Button [x: width - 75]
    RectF eyeBtn((REAL)width - 75, 10, 28, 28);
    bool hoverEye = (m_mouseX >= width - 75 && m_mouseX <= width - 47 && m_mouseY >= 10 && m_mouseY <= 38);
    g.FillRectangle(toolsCollapsed ? m_accentBrush : (hoverEye ? m_buttonHoverBrush : m_buttonBrush), eyeBtn);
    g.DrawRectangle(hoverEye ? m_accentPen : m_borderPen, eyeBtn);
    g.DrawString(L"👁", -1, &btnFont, PointF((REAL)width - 71, 12), toolsCollapsed ? m_cardBrush : m_textBrush);

    // ⚙ Settings Gear Button [x: width - 40]
    RectF gearBtn((REAL)width - 40, 10, 28, 28);
    bool hoverGear = (m_mouseX >= width - 40 && m_mouseX <= width - 12 && m_mouseY >= 10 && m_mouseY <= 38);
    g.FillRectangle(hoverGear ? m_buttonHoverBrush : m_buttonBrush, gearBtn);
    g.DrawRectangle(hoverGear ? m_accentPen : m_borderPen, gearBtn);
    g.DrawString(L"⚙", -1, &btnFont, PointF((REAL)width - 36, 12), m_textBrush);
}

void UIRenderer::RenderToolCard(Graphics& g, ToolCard* card, int x, int y, int width, int height) {
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    RectF cardRect((REAL)x, (REAL)y, (REAL)width, (REAL)height);
    g.FillRectangle(m_cardBrush, cardRect);
    g.DrawRectangle(m_borderPen, cardRect);

    Font titleFont(L"Segoe UI", 11, FontStyleBold, UnitPoint);
    Font bodyFont(L"Segoe UI", 10, FontStyleRegular, UnitPoint);
    Font digitFont(L"Segoe UI", 24, FontStyleBold, UnitPoint);
    Font smallFont(L"Segoe UI", 9, FontStyleRegular, UnitPoint);

    // Card Header Title & Close Button (X)
    const wchar_t* title = L"";
    if (card->GetType() == TOOL_STOPWATCH) title = SettingsManager::Instance().Text("STOPWATCH");
    else if (card->GetType() == TOOL_TIMER) title = SettingsManager::Instance().Text("TIMER");
    else if (card->GetType() == TOOL_POMODORO) title = SettingsManager::Instance().Text("POMODORO");

    g.DrawString(title, -1, &titleFont, PointF((REAL)x + 15, (REAL)y + 8), m_accentBrush);

    // Close button (X)
    RectF closeBtn((REAL)x + width - 30, (REAL)y + 8, 20, 20);
    bool hoverClose = (m_mouseX >= x + width - 30 && m_mouseX <= x + width - 10 && m_mouseY >= y + 8 && m_mouseY <= y + 28);
    SolidBrush closeHoverBrush(Color(255, 235, 87, 87));
    g.DrawString(L"✕", -1, &bodyFont, PointF((REAL)x + width - 26, (REAL)y + 7), hoverClose ? &closeHoverBrush : m_textDimBrush);

    if (card->GetType() == TOOL_STOPWATCH) {
        StopwatchTool* sw = (StopwatchTool*)card;
        DWORD ms = sw->GetElapsedMs();
        DWORD mins = (ms / 60000) % 60;
        DWORD secs = (ms / 1000) % 60;
        DWORD hundredths = (ms % 1000) / 10;
        wchar_t digits[64];
        swprintf_s(digits, 64, L"%02d:%02d.%02d", mins, secs, hundredths);

        g.DrawString(digits, -1, &digitFont, PointF((REAL)x + 15, (REAL)y + 30), m_textBrush);

        // Buttons: Start/Pause, Lap, Reset
        RectF btn1((REAL)x + 15, (REAL)y + 75, 95, 30);
        RectF btn2((REAL)x + 120, (REAL)y + 75, 95, 30);
        RectF btn3((REAL)x + 225, (REAL)y + 75, 95, 30);

        bool h1 = (m_mouseX >= x + 15 && m_mouseX <= x + 110 && m_mouseY >= y + 75 && m_mouseY <= y + 105);
        bool h2 = (m_mouseX >= x + 120 && m_mouseX <= x + 215 && m_mouseY >= y + 75 && m_mouseY <= y + 105);
        bool h3 = (m_mouseX >= x + 225 && m_mouseX <= x + 320 && m_mouseY >= y + 75 && m_mouseY <= y + 105);

        g.FillRectangle(h1 ? m_buttonHoverBrush : m_buttonBrush, btn1); g.DrawRectangle(h1 ? m_accentPen : m_borderPen, btn1);
        g.FillRectangle(h2 ? m_buttonHoverBrush : m_buttonBrush, btn2); g.DrawRectangle(h2 ? m_accentPen : m_borderPen, btn2);
        g.FillRectangle(h3 ? m_buttonHoverBrush : m_buttonBrush, btn3); g.DrawRectangle(h3 ? m_accentPen : m_borderPen, btn3);

        const wchar_t* startPause = sw->IsRunning() ? SettingsManager::Instance().Text("PAUSE") : SettingsManager::Instance().Text("START");
        g.DrawString(startPause, -1, &smallFont, PointF((REAL)x + 25, (REAL)y + 81), m_textBrush);
        g.DrawString(SettingsManager::Instance().Text("LAP"), -1, &smallFont, PointF((REAL)x + 130, (REAL)y + 81), m_textBrush);
        g.DrawString(SettingsManager::Instance().Text("RESET"), -1, &smallFont, PointF((REAL)x + 235, (REAL)y + 81), m_textBrush);

        // Render Lap records
        int lapY = y + 115;
        const auto& laps = sw->GetLaps();
        int maxLapsToDraw = (height - 120) / 22;
        int scrollOff = sw->GetScrollOffset();
        int startIdx = std::min((int)laps.size(), scrollOff);
        int endIdx = std::min((int)laps.size(), startIdx + maxLapsToDraw);

        for (int i = startIdx; i < endIdx; i++) {
            g.DrawString(laps[i].formattedTime.c_str(), -1, &smallFont, PointF((REAL)x + 15, (REAL)lapY), m_textDimBrush);
            lapY += 22;
        }

        // Draw scrollbar track & hint if laps exceed visible area
        if ((int)laps.size() > 4) {
            RectF track((REAL)x + width - 18, (REAL)y + 115, 6, 85);
            g.FillRectangle(m_buttonBrush, track); g.DrawRectangle(m_borderPen, track);
            
            float thumbH = std::max(15.0f, 85.0f * 4.0f / (float)laps.size());
            float thumbY = (REAL)y + 115 + (85.0f - thumbH) * ((float)scrollOff / (float)(laps.size() - 4));
            RectF thumb((REAL)x + width - 18, thumbY, 6, thumbH);
            g.FillRectangle(m_accentBrush, thumb);

            Font tinyFont(L"Segoe UI", 7, FontStyleRegular, UnitPoint);
            g.DrawString(SettingsManager::Instance().Text("SCROLL_HINT"), -1, &tinyFont, PointF((REAL)x + 115, (REAL)y + height - 16), m_textDimBrush);
        }

    } else if (card->GetType() == TOOL_TIMER) {
        TimerTool* tm = (TimerTool*)card;

        // Tabs: Duration vs Target Time
        RectF tab1((REAL)x + 15, (REAL)y + 30, 160, 26);
        RectF tab2((REAL)x + 185, (REAL)y + 30, 160, 26);
        bool ht1 = (m_mouseX >= x + 15 && m_mouseX <= x + 175 && m_mouseY >= y + 30 && m_mouseY <= y + 56);
        bool ht2 = (m_mouseX >= x + 185 && m_mouseX <= x + 345 && m_mouseY >= y + 30 && m_mouseY <= y + 56);

        g.FillRectangle(tm->GetMode() == TIMER_MODE_DURATION ? m_accentBrush : (ht1 ? m_buttonHoverBrush : m_buttonBrush), tab1);
        g.FillRectangle(tm->GetMode() == TIMER_MODE_TARGET_TIME ? m_accentBrush : (ht2 ? m_buttonHoverBrush : m_buttonBrush), tab2);
        g.DrawRectangle(ht1 ? m_accentPen : m_borderPen, tab1);
        g.DrawRectangle(ht2 ? m_accentPen : m_borderPen, tab2);

        g.DrawString(SettingsManager::Instance().Text("MODE_DURATION"), -1, &smallFont, PointF((REAL)x + 30, (REAL)y + 34), tm->GetMode() == TIMER_MODE_DURATION ? m_cardBrush : m_textBrush);
        g.DrawString(SettingsManager::Instance().Text("MODE_TARGET_TIME"), -1, &smallFont, PointF((REAL)x + 195, (REAL)y + 34), tm->GetMode() == TIMER_MODE_TARGET_TIME ? m_cardBrush : m_textBrush);

        // Digit countdown / Display
        int remSec = tm->GetRemainingSec();
        int hrs = remSec / 3600;
        int mins = (remSec % 3600) / 60;
        int secs = remSec % 60;
        wchar_t buf[64];
        if (SettingsManager::Instance().Get().showSecondsInTimer || secs > 0) {
            swprintf_s(buf, 64, L"%02d:%02d:%02d", hrs, mins, secs);
        } else {
            if (hrs > 0) swprintf_s(buf, 64, L"%02d sa %02d dk", hrs, mins);
            else swprintf_s(buf, 64, L"%02d dk", mins);
        }

        g.DrawString(buf, -1, &digitFont, PointF((REAL)x + 15, (REAL)y + 58), m_textBrush);

        if (!tm->IsRunning()) {
            Font boxFont(L"Segoe UI", 9, FontStyleBold, UnitPoint);

            if (tm->GetMode() == TIMER_MODE_DURATION) {
                // 3 Clean Input Boxes (Saat, Dakika, Saniye)
                RectF box1((REAL)x + 15, (REAL)y + 92, 100, 30);
                RectF box2((REAL)x + 130, (REAL)y + 92, 100, 30);
                RectF box3((REAL)x + 245, (REAL)y + 92, 100, 30);

                bool act1 = (tm->activeInputIndex == 0);
                bool act2 = (tm->activeInputIndex == 1);
                bool act3 = (tm->activeInputIndex == 2);

                g.FillRectangle(m_buttonBrush, box1); g.DrawRectangle(act1 ? m_accentPen : m_borderPen, box1);
                g.FillRectangle(m_buttonBrush, box2); g.DrawRectangle(act2 ? m_accentPen : m_borderPen, box2);
                g.FillRectangle(m_buttonBrush, box3); g.DrawRectangle(act3 ? m_accentPen : m_borderPen, box3);

                std::wstring displayH = tm->hoursStr + (act1 && m_blinkOn ? L"|" : L"") + L" sa";
                std::wstring displayM = tm->minsStr + (act2 && m_blinkOn ? L"|" : L"") + L" dk";
                std::wstring displayS = tm->secsStr + (act3 && m_blinkOn ? L"|" : L"") + L" sn";

                g.DrawString(displayH.c_str(), -1, &boxFont, PointF((REAL)x + 25, (REAL)y + 97), m_textBrush);
                g.DrawString(displayM.c_str(), -1, &boxFont, PointF((REAL)x + 140, (REAL)y + 97), m_textBrush);
                g.DrawString(displayS.c_str(), -1, &boxFont, PointF((REAL)x + 255, (REAL)y + 97), m_textBrush);
            } else {
                // 2 Clean Input Boxes (Hedef Saat, Hedef Dakika - e.g. 11 sa 49 dk)
                RectF box1((REAL)x + 15, (REAL)y + 92, 160, 30);
                RectF box2((REAL)x + 185, (REAL)y + 92, 160, 30);

                bool act1 = (tm->activeInputIndex == 0);
                bool act2 = (tm->activeInputIndex == 1);

                g.FillRectangle(m_buttonBrush, box1); g.DrawRectangle(act1 ? m_accentPen : m_borderPen, box1);
                g.FillRectangle(m_buttonBrush, box2); g.DrawRectangle(act2 ? m_accentPen : m_borderPen, box2);

                std::wstring displayTH = L"Hedef Saat: " + tm->targetHoursStr + (act1 && m_blinkOn ? L"|" : L"");
                std::wstring displayTM = L"Hedef Dakika: " + tm->targetMinsStr + (act2 && m_blinkOn ? L"|" : L"");

                g.DrawString(displayTH.c_str(), -1, &boxFont, PointF((REAL)x + 25, (REAL)y + 97), m_textBrush);
                g.DrawString(displayTM.c_str(), -1, &boxFont, PointF((REAL)x + 195, (REAL)y + 97), m_textBrush);
            }

            // Start/Pause & Reset Buttons (Stopped state)
            RectF startBtn((REAL)x + 15, (REAL)y + 132, 200, 30);
            RectF resetBtn((REAL)x + 225, (REAL)y + 132, 120, 30);

            bool hs = (m_mouseX >= x + 15 && m_mouseX <= x + 215 && m_mouseY >= y + 132 && m_mouseY <= y + 164);
            bool hr = (m_mouseX >= x + 225 && m_mouseX <= x + 345 && m_mouseY >= y + 132 && m_mouseY <= y + 164);

            g.FillRectangle(hs ? m_buttonHoverBrush : m_buttonBrush, startBtn); g.DrawRectangle(hs ? m_accentPen : m_borderPen, startBtn);
            g.FillRectangle(hr ? m_buttonHoverBrush : m_buttonBrush, resetBtn); g.DrawRectangle(hr ? m_accentPen : m_borderPen, resetBtn);

            const wchar_t* stTxt = SettingsManager::Instance().Text("START");
            g.DrawString(stTxt, -1, &smallFont, PointF((REAL)x + 90, (REAL)y + 138), m_textBrush);
            g.DrawString(SettingsManager::Instance().Text("RESET"), -1, &smallFont, PointF((REAL)x + 265, (REAL)y + 138), m_textBrush);
        } else {
            // Start/Pause & Reset Buttons (Running state - compact height)
            RectF startBtn((REAL)x + 15, (REAL)y + 92, 200, 32);
            RectF resetBtn((REAL)x + 225, (REAL)y + 92, 120, 32);

            bool hs = (m_mouseX >= x + 15 && m_mouseX <= x + 215 && m_mouseY >= y + 92 && m_mouseY <= y + 124);
            bool hr = (m_mouseX >= x + 225 && m_mouseX <= x + 345 && m_mouseY >= y + 92 && m_mouseY <= y + 124);

            g.FillRectangle(hs ? m_buttonHoverBrush : m_buttonBrush, startBtn); g.DrawRectangle(hs ? m_accentPen : m_borderPen, startBtn);
            g.FillRectangle(hr ? m_buttonHoverBrush : m_buttonBrush, resetBtn); g.DrawRectangle(hr ? m_accentPen : m_borderPen, resetBtn);

            const wchar_t* stTxt = SettingsManager::Instance().Text("PAUSE");
            g.DrawString(stTxt, -1, &smallFont, PointF((REAL)x + 90, (REAL)y + 99), m_textBrush);
            g.DrawString(SettingsManager::Instance().Text("RESET"), -1, &smallFont, PointF((REAL)x + 265, (REAL)y + 99), m_textBrush);
        }

    } else if (card->GetType() == TOOL_POMODORO) {
        PomodoroTool* pm = (PomodoroTool*)card;

        // Subtitle (NO duplicate "Pomodoro" text when idle!)
        wchar_t statusBuf[128];
        if (pm->GetState() == POMO_IDLE) {
            swprintf_s(statusBuf, 128, L"Hazır (Toplam %d Seans Planlandı)", pm->GetTotalSessions());
        } else if (pm->GetState() == POMO_WORK) {
            swprintf_s(statusBuf, 128, L"%s (%d/%d)", SettingsManager::Instance().Text("WORK_PHASE"), pm->GetCurrentSession(), pm->GetTotalSessions());
        } else if (pm->GetState() == POMO_BREAK) {
            swprintf_s(statusBuf, 128, L"%s (%d/%d)", SettingsManager::Instance().Text("BREAK_PHASE"), pm->GetCurrentSession(), pm->GetTotalSessions() - 1);
        } else {
            swprintf_s(statusBuf, 128, L"%s", SettingsManager::Instance().Text("FINISHED"));
        }
        g.DrawString(statusBuf, -1, &bodyFont, PointF((REAL)x + 15, (REAL)y + 30), m_textDimBrush);

        int remSec = pm->GetRemainingSec();
        int mins = remSec / 60;
        int secs = remSec % 60;
        wchar_t timeBuf[64];
        swprintf_s(timeBuf, 64, L"%02d:%02d", mins, secs);
        g.DrawString(timeBuf, -1, &digitFont, PointF((REAL)x + 15, (REAL)y + 50), m_textBrush);

        // 3 Input parameter boxes (Width 100px each!)
        RectF box1((REAL)x + 15, (REAL)y + 95, 100, 26);
        RectF box2((REAL)x + 130, (REAL)y + 95, 100, 26);
        RectF box3((REAL)x + 245, (REAL)y + 95, 100, 26);

        bool hb1 = (m_mouseX >= x + 15 && m_mouseX <= x + 115 && m_mouseY >= y + 95 && m_mouseY <= y + 121);
        bool hb2 = (m_mouseX >= x + 130 && m_mouseX <= x + 230 && m_mouseY >= y + 95 && m_mouseY <= y + 121);
        bool hb3 = (m_mouseX >= x + 245 && m_mouseX <= x + 345 && m_mouseY >= y + 95 && m_mouseY <= y + 121);

        g.FillRectangle((hb1 || pm->activeInputIndex == 0) ? m_buttonHoverBrush : m_buttonBrush, box1);
        g.FillRectangle((hb2 || pm->activeInputIndex == 1) ? m_buttonHoverBrush : m_buttonBrush, box2);
        g.FillRectangle((hb3 || pm->activeInputIndex == 2) ? m_buttonHoverBrush : m_buttonBrush, box3);

        g.DrawRectangle(pm->activeInputIndex == 0 ? m_accentPen : (hb1 ? m_accentPen : m_borderPen), box1);
        g.DrawRectangle(pm->activeInputIndex == 1 ? m_accentPen : (hb2 ? m_accentPen : m_borderPen), box2);
        g.DrawRectangle(pm->activeInputIndex == 2 ? m_accentPen : (hb3 ? m_accentPen : m_borderPen), box3);

        std::wstring s1 = pm->workMinStr + (pm->activeInputIndex == 0 && m_blinkOn ? L"|" : L"");
        std::wstring s2 = pm->breakMinStr + (pm->activeInputIndex == 1 && m_blinkOn ? L"|" : L"");
        std::wstring s3 = pm->targetHoursStr + (pm->activeInputIndex == 2 && m_blinkOn ? L"|" : L"");

        g.DrawString(s1.c_str(), -1, &smallFont, PointF((REAL)x + 22, (REAL)y + 98), m_textBrush);
        g.DrawString(s2.c_str(), -1, &smallFont, PointF((REAL)x + 137, (REAL)y + 98), m_textBrush);
        g.DrawString(s3.c_str(), -1, &smallFont, PointF((REAL)x + 252, (REAL)y + 98), m_textBrush);

        Font tinyFont(L"Segoe UI", 8, FontStyleRegular, UnitPoint);
        g.DrawString(L"Çalışma (dk)", -1, &tinyFont, PointF((REAL)x + 15, (REAL)y + 125), m_textDimBrush);
        g.DrawString(L"Mola (dk)", -1, &tinyFont, PointF((REAL)x + 130, (REAL)y + 125), m_textDimBrush);
        g.DrawString(L"Hedef (saat)", -1, &tinyFont, PointF((REAL)x + 245, (REAL)y + 125), m_textDimBrush);

        // Buttons: Calculate & Start, Reset
        RectF startBtn((REAL)x + 15, (REAL)y + 155, 205, 30);
        RectF resetBtn((REAL)x + 230, (REAL)y + 155, 115, 30);

        bool hs = (m_mouseX >= x + 15 && m_mouseX <= x + 220 && m_mouseY >= y + 155 && m_mouseY <= y + 185);
        bool hr = (m_mouseX >= x + 230 && m_mouseX <= x + 345 && m_mouseY >= y + 155 && m_mouseY <= y + 185);

        g.FillRectangle(hs ? m_buttonHoverBrush : m_buttonBrush, startBtn); g.DrawRectangle(hs ? m_accentPen : m_borderPen, startBtn);
        g.FillRectangle(hr ? m_buttonHoverBrush : m_buttonBrush, resetBtn); g.DrawRectangle(hr ? m_accentPen : m_borderPen, resetBtn);

        g.DrawString(SettingsManager::Instance().Text("COMPUTE_POMODORO"), -1, &smallFont, PointF((REAL)x + 25, (REAL)y + 161), m_textBrush);
        g.DrawString(SettingsManager::Instance().Text("RESET"), -1, &smallFont, PointF((REAL)x + 260, (REAL)y + 161), m_textBrush);
    }
}

void UIRenderer::RenderToolMenu(Graphics& g, int x, int y, int width, int height) {
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    RectF menuRect((REAL)x, (REAL)y, (REAL)width, (REAL)height);
    g.FillRectangle(m_cardBrush, menuRect);
    g.DrawRectangle(m_borderPen, menuRect);

    Font itemFont(L"Segoe UI", 10, FontStyleRegular, UnitPoint);
    Font exitFont(L"Segoe UI", 9, FontStyleBold, UnitPoint);

    bool hm1 = (m_mouseX >= x && m_mouseX <= x + width && m_mouseY >= y + 5 && m_mouseY <= y + 35);
    bool hm2 = (m_mouseX >= x && m_mouseX <= x + width && m_mouseY >= y + 35 && m_mouseY <= y + 68);
    bool hm3 = (m_mouseX >= x && m_mouseX <= x + width && m_mouseY >= y + 68 && m_mouseY <= y + 100);
    bool hm4 = (m_mouseX >= x && m_mouseX <= x + width && m_mouseY >= y + 105 && m_mouseY <= y + 140);

    if (hm1) { RectF r((REAL)x + 4, (REAL)y + 4, (REAL)width - 8, 28); g.FillRectangle(m_buttonHoverBrush, r); }
    if (hm2) { RectF r((REAL)x + 4, (REAL)y + 34, (REAL)width - 8, 28); g.FillRectangle(m_buttonHoverBrush, r); }
    if (hm3) { RectF r((REAL)x + 4, (REAL)y + 64, (REAL)width - 8, 28); g.FillRectangle(m_buttonHoverBrush, r); }
    if (hm4) { RectF r((REAL)x + 4, (REAL)y + 105, (REAL)width - 8, 32); g.FillRectangle(m_buttonHoverBrush, r); }

    // Item 1: Stopwatch
    g.DrawString(SettingsManager::Instance().Text("STOPWATCH"), -1, &itemFont, PointF((REAL)x + 15, (REAL)y + 10), m_textBrush);

    // Item 2: Timer
    g.DrawString(SettingsManager::Instance().Text("TIMER"), -1, &itemFont, PointF((REAL)x + 15, (REAL)y + 40), m_textBrush);

    // Item 3: Pomodoro
    g.DrawString(SettingsManager::Instance().Text("POMODORO"), -1, &itemFont, PointF((REAL)x + 15, (REAL)y + 70), m_textBrush);

    // Separator Line
    g.DrawLine(m_borderPen, x + 10, y + 102, x + width - 10, y + 102);

    // Item 4: Close Application (Exit App)
    SolidBrush exitBrush(Color(255, 235, 87, 87)); // Modern soft red
    g.DrawString(SettingsManager::Instance().Text("EXIT_APP"), -1, &exitFont, PointF((REAL)x + 15, (REAL)y + 112), &exitBrush);
}

void UIRenderer::RenderSettingsModal(Graphics& g, int width, int height) {
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    // Semi-transparent overlay backdrop
    SolidBrush overlayBrush(Color(180, 0, 0, 0));
    g.FillRectangle(&overlayBrush, 0, 0, width, height);

    int modalW = 360;
    int modalH = 370;
    int modalX = (width - modalW) / 2;
    int modalY = (height - modalH) / 2;

    RectF modalRect((REAL)modalX, (REAL)modalY, (REAL)modalW, (REAL)modalH);
    g.FillRectangle(m_cardBrush, modalRect);
    g.DrawRectangle(m_borderPen, modalRect);

    Font titleFont(L"Segoe UI", 11, FontStyleBold, UnitPoint);
    Font bodyFont(L"Segoe UI", 9, FontStyleRegular, UnitPoint);

    g.DrawString(SettingsManager::Instance().Text("SETTINGS"), -1, &titleFont, PointF((REAL)modalX + 15, (REAL)modalY + 10), m_accentBrush);

    // Close X
    bool hoverX = (m_mouseX >= modalX + modalW - 30 && m_mouseX <= modalX + modalW && m_mouseY >= modalY && m_mouseY <= modalY + 30);
    SolidBrush redBrush(Color(255, 235, 87, 87));
    g.DrawString(L"✕", -1, &titleFont, PointF((REAL)modalX + modalW - 25, (REAL)modalY + 8), hoverX ? &redBrush : m_textDimBrush);

    int curY = modalY + 38;
    AppSettings& s = SettingsManager::Instance().Get();

    // 1. Language Toggle
    g.DrawString(SettingsManager::Instance().Text("LANGUAGE"), -1, &bodyFont, PointF((REAL)modalX + 15, (REAL)curY), m_textBrush);
    g.DrawString(s.lang == LANG_TR ? L"[Türkçe]" : L"[English]", -1, &bodyFont, PointF((REAL)modalX + 260, (REAL)curY), m_accentBrush);
    curY += 26;

    // 2. Theme Toggle
    g.DrawString(SettingsManager::Instance().Text("THEME"), -1, &bodyFont, PointF((REAL)modalX + 15, (REAL)curY), m_textBrush);
    g.DrawString(s.theme == THEME_DARK ? SettingsManager::Instance().Text("THEME_DARK") : SettingsManager::Instance().Text("THEME_LIGHT"), -1, &bodyFont, PointF((REAL)modalX + 260, (REAL)curY), m_accentBrush);
    curY += 26;

    // 3. UI Scale Toggle
    g.DrawString(SettingsManager::Instance().Text("UI_SCALE"), -1, &bodyFont, PointF((REAL)modalX + 15, (REAL)curY), m_textBrush);
    g.DrawString(s.uiScale == 120 ? SettingsManager::Instance().Text("SCALE_120") : (s.uiScale == 85 ? SettingsManager::Instance().Text("SCALE_85") : SettingsManager::Instance().Text("SCALE_100")), -1, &bodyFont, PointF((REAL)modalX + 245, (REAL)curY), m_accentBrush);
    curY += 26;

    // 4. Clock Only Mode Toggle (Kompakt Saat Modu)
    g.DrawString(SettingsManager::Instance().Text("CLOCK_ONLY_MODE"), -1, &bodyFont, PointF((REAL)modalX + 15, (REAL)curY), m_textBrush);
    g.DrawString(s.clockOnlyMode ? L"[✓]" : L"[  ]", -1, &bodyFont, PointF((REAL)modalX + 315, (REAL)curY), m_accentBrush);
    curY += 25;

    // 5. Rounded Corners Mode Toggle (Yuvarlatılmış Kenarlar)
    g.DrawString(SettingsManager::Instance().Text("ROUNDED_CORNERS"), -1, &bodyFont, PointF((REAL)modalX + 15, (REAL)curY), m_textBrush);
    g.DrawString(s.roundedCorners ? L"[✓]" : L"[  ]", -1, &bodyFont, PointF((REAL)modalX + 315, (REAL)curY), m_accentBrush);
    curY += 25;

    // 5. Show Seconds in Clock
    g.DrawString(SettingsManager::Instance().Text("SHOW_SECONDS_CLOCK"), -1, &bodyFont, PointF((REAL)modalX + 15, (REAL)curY), m_textBrush);
    g.DrawString(s.showSecondsInClock ? L"[✓]" : L"[  ]", -1, &bodyFont, PointF((REAL)modalX + 315, (REAL)curY), m_accentBrush);
    curY += 26;

    // 6. Show Seconds in Timer
    g.DrawString(SettingsManager::Instance().Text("SHOW_SECONDS_TIMER"), -1, &bodyFont, PointF((REAL)modalX + 15, (REAL)curY), m_textBrush);
    g.DrawString(s.showSecondsInTimer ? L"[✓]" : L"[  ]", -1, &bodyFont, PointF((REAL)modalX + 315, (REAL)curY), m_accentBrush);
    curY += 26;

    // 7. Force Always on Top
    g.DrawString(SettingsManager::Instance().Text("FORCE_TOPMOST"), -1, &bodyFont, PointF((REAL)modalX + 15, (REAL)curY), m_textBrush);
    g.DrawString(s.forceAlwaysOnTop ? L"[✓]" : L"[  ]", -1, &bodyFont, PointF((REAL)modalX + 315, (REAL)curY), m_accentBrush);
    curY += 26;

    // 8. Autostart
    g.DrawString(SettingsManager::Instance().Text("AUTOSTART"), -1, &bodyFont, PointF((REAL)modalX + 15, (REAL)curY), m_textBrush);
    g.DrawString(s.autoStart ? L"[✓]" : L"[  ]", -1, &bodyFont, PointF((REAL)modalX + 315, (REAL)curY), m_accentBrush);
    curY += 26;

    // 9. Restore Previous Tools
    g.DrawString(SettingsManager::Instance().Text("RESTORE_TOOLS"), -1, &bodyFont, PointF((REAL)modalX + 15, (REAL)curY), m_textBrush);
    g.DrawString(s.restorePreviousTools ? L"[✓]" : L"[  ]", -1, &bodyFont, PointF((REAL)modalX + 315, (REAL)curY), m_accentBrush);
    curY += 26;

    // 10. Legal Disclaimer Info Button
    RectF legalBtn((REAL)modalX + 15, (REAL)curY + 8, 330, 26);
    g.FillRectangle(m_buttonBrush, legalBtn); g.DrawRectangle(m_borderPen, legalBtn);
    g.DrawString(SettingsManager::Instance().Text("LEGAL_NOTICE"), -1, &bodyFont, PointF((REAL)modalX + 120, (REAL)curY + 12), m_textBrush);
}
