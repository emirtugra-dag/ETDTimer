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
    delete m_borderPen;

    bool dark = (SettingsManager::Instance().Get().theme == THEME_DARK);

    if (dark) {
        m_bgBrush = new SolidBrush(Color(240, 20, 20, 24));
        m_cardBrush = new SolidBrush(Color(255, 32, 32, 38));
        m_textBrush = new SolidBrush(Color(255, 240, 240, 245));
        m_textDimBrush = new SolidBrush(Color(255, 150, 150, 160));
        m_accentBrush = new SolidBrush(Color(255, 0, 173, 181));
        m_buttonBrush = new SolidBrush(Color(255, 45, 45, 55));
        m_borderPen = new Pen(Color(255, 55, 55, 65), 1.0f);
    } else {
        m_bgBrush = new SolidBrush(Color(240, 245, 246, 248));
        m_cardBrush = new SolidBrush(Color(255, 255, 255, 255));
        m_textBrush = new SolidBrush(Color(255, 20, 24, 33));
        m_textDimBrush = new SolidBrush(Color(255, 100, 105, 115));
        m_accentBrush = new SolidBrush(Color(255, 0, 120, 212));
        m_buttonBrush = new SolidBrush(Color(255, 230, 235, 242));
        m_borderPen = new Pen(Color(255, 210, 215, 225), 1.0f);
    }
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
    g.DrawString(timeBuf, -1, &clockFont, PointF(15, 10), m_textBrush);

    // Render Top Control Buttons
    Font btnFont(L"Segoe UI Symbol", 13, FontStyleRegular, UnitPoint);
    
    // ☰ Burger Menu Button [x: width - 110]
    RectF burgerBtn((REAL)width - 110, 10, 28, 28);
    g.FillRectangle(m_buttonBrush, burgerBtn);
    g.DrawRectangle(m_borderPen, burgerBtn);
    g.DrawString(L"☰", -1, &btnFont, PointF((REAL)width - 105, 12), m_textBrush);

    // 👁 Eye (Hide/Show) Button [x: width - 75]
    RectF eyeBtn((REAL)width - 75, 10, 28, 28);
    g.FillRectangle(toolsCollapsed ? m_accentBrush : m_buttonBrush, eyeBtn);
    g.DrawRectangle(m_borderPen, eyeBtn);
    g.DrawString(L"👁", -1, &btnFont, PointF((REAL)width - 71, 12), toolsCollapsed ? m_cardBrush : m_textBrush);

    // ⚙ Settings Gear Button [x: width - 40]
    RectF gearBtn((REAL)width - 40, 10, 28, 28);
    g.FillRectangle(m_buttonBrush, gearBtn);
    g.DrawRectangle(m_borderPen, gearBtn);
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
    g.DrawString(L"✕", -1, &bodyFont, PointF((REAL)x + width - 26, (REAL)y + 7), m_textDimBrush);

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

        g.FillRectangle(m_buttonBrush, btn1); g.DrawRectangle(m_borderPen, btn1);
        g.FillRectangle(m_buttonBrush, btn2); g.DrawRectangle(m_borderPen, btn2);
        g.FillRectangle(m_buttonBrush, btn3); g.DrawRectangle(m_borderPen, btn3);

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

    } else if (card->GetType() == TOOL_TIMER) {
        TimerTool* tm = (TimerTool*)card;

        // Tabs: Duration vs Target Time
        RectF tab1((REAL)x + 15, (REAL)y + 30, 145, 26);
        RectF tab2((REAL)x + 170, (REAL)y + 30, 155, 26);
        g.FillRectangle(tm->GetMode() == TIMER_MODE_DURATION ? m_accentBrush : m_buttonBrush, tab1);
        g.FillRectangle(tm->GetMode() == TIMER_MODE_TARGET_TIME ? m_accentBrush : m_buttonBrush, tab2);
        g.DrawString(SettingsManager::Instance().Text("MODE_DURATION"), -1, &smallFont, PointF((REAL)x + 22, (REAL)y + 34), tm->GetMode() == TIMER_MODE_DURATION ? m_cardBrush : m_textBrush);
        g.DrawString(SettingsManager::Instance().Text("MODE_TARGET_TIME"), -1, &smallFont, PointF((REAL)x + 176, (REAL)y + 34), tm->GetMode() == TIMER_MODE_TARGET_TIME ? m_cardBrush : m_textBrush);

        // Digit countdown
        int remSec = tm->GetRemainingSec();
        int hrs = remSec / 3600;
        int mins = (remSec % 3600) / 60;
        int secs = remSec % 60;
        wchar_t buf[64];
        if (SettingsManager::Instance().Get().showSecondsInTimer) {
            if (hrs > 0) swprintf_s(buf, 64, L"%02d:%02d:%02d", hrs, mins, secs);
            else swprintf_s(buf, 64, L"%02d:%02d", mins, secs);
        } else {
            if (hrs > 0) swprintf_s(buf, 64, L"%02d sa %02d dk", hrs, mins);
            else swprintf_s(buf, 64, L"%02d dk", mins);
        }

        g.DrawString(buf, -1, &digitFont, PointF((REAL)x + 15, (REAL)y + 58), m_textBrush);

        if (!tm->IsRunning()) {
            // Quick Presets Row: [+1dk] [+5dk] [+15dk] [+1sa]
            RectF p1((REAL)x + 15, (REAL)y + 95, 70, 26);
            RectF p2((REAL)x + 95, (REAL)y + 95, 70, 26);
            RectF p3((REAL)x + 175, (REAL)y + 95, 70, 26);
            RectF p4((REAL)x + 255, (REAL)y + 95, 70, 26);
            g.FillRectangle(m_buttonBrush, p1); g.DrawRectangle(m_borderPen, p1);
            g.FillRectangle(m_buttonBrush, p2); g.DrawRectangle(m_borderPen, p2);
            g.FillRectangle(m_buttonBrush, p3); g.DrawRectangle(m_borderPen, p3);
            g.FillRectangle(m_buttonBrush, p4); g.DrawRectangle(m_borderPen, p4);

            g.DrawString(L"+1dk", -1, &smallFont, PointF((REAL)x + 32, (REAL)y + 99), m_textBrush);
            g.DrawString(L"+5dk", -1, &smallFont, PointF((REAL)x + 112, (REAL)y + 99), m_textBrush);
            g.DrawString(L"+15dk", -1, &smallFont, PointF((REAL)x + 188, (REAL)y + 99), m_textBrush);
            g.DrawString(L"+1sa", -1, &smallFont, PointF((REAL)x + 272, (REAL)y + 99), m_textBrush);

            // Start/Pause & Reset Buttons (Stopped state)
            RectF startBtn((REAL)x + 15, (REAL)y + 135, 190, 32);
            RectF resetBtn((REAL)x + 215, (REAL)y + 135, 110, 32);
            g.FillRectangle(m_buttonBrush, startBtn); g.DrawRectangle(m_borderPen, startBtn);
            g.FillRectangle(m_buttonBrush, resetBtn); g.DrawRectangle(m_borderPen, resetBtn);

            const wchar_t* stTxt = SettingsManager::Instance().Text("START");
            g.DrawString(stTxt, -1, &smallFont, PointF((REAL)x + 85, (REAL)y + 142), m_textBrush);
            g.DrawString(SettingsManager::Instance().Text("RESET"), -1, &smallFont, PointF((REAL)x + 245, (REAL)y + 142), m_textBrush);
        } else {
            // Start/Pause & Reset Buttons (Running state - compact height)
            RectF startBtn((REAL)x + 15, (REAL)y + 92, 190, 32);
            RectF resetBtn((REAL)x + 215, (REAL)y + 92, 110, 32);
            g.FillRectangle(m_buttonBrush, startBtn); g.DrawRectangle(m_borderPen, startBtn);
            g.FillRectangle(m_buttonBrush, resetBtn); g.DrawRectangle(m_borderPen, resetBtn);

            const wchar_t* stTxt = SettingsManager::Instance().Text("PAUSE");
            g.DrawString(stTxt, -1, &smallFont, PointF((REAL)x + 85, (REAL)y + 99), m_textBrush);
            g.DrawString(SettingsManager::Instance().Text("RESET"), -1, &smallFont, PointF((REAL)x + 245, (REAL)y + 99), m_textBrush);
        }

    } else if (card->GetType() == TOOL_POMODORO) {
        PomodoroTool* pm = (PomodoroTool*)card;

        // State & countdown
        wchar_t statusBuf[128];
        if (pm->GetState() == POMO_IDLE) {
            swprintf_s(statusBuf, 128, L"%s", SettingsManager::Instance().Text("POMODORO"));
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

        // 4 Input parameter boxes: Work min, Break min, Target hrs, Num breaks
        RectF box1((REAL)x + 15, (REAL)y + 95, 68, 26);
        RectF box2((REAL)x + 95, (REAL)y + 95, 68, 26);
        RectF box3((REAL)x + 175, (REAL)y + 95, 68, 26);
        RectF box4((REAL)x + 255, (REAL)y + 95, 68, 26);

        g.FillRectangle(m_buttonBrush, box1); g.DrawRectangle(m_borderPen, box1);
        g.FillRectangle(m_buttonBrush, box2); g.DrawRectangle(m_borderPen, box2);
        g.FillRectangle(m_buttonBrush, box3); g.DrawRectangle(m_borderPen, box3);
        g.FillRectangle(m_buttonBrush, box4); g.DrawRectangle(m_borderPen, box4);

        g.DrawString(pm->workMinStr.c_str(), -1, &smallFont, PointF((REAL)x + 22, (REAL)y + 98), m_textBrush);
        g.DrawString(pm->breakMinStr.c_str(), -1, &smallFont, PointF((REAL)x + 102, (REAL)y + 98), m_textBrush);
        g.DrawString(pm->targetHoursStr.c_str(), -1, &smallFont, PointF((REAL)x + 182, (REAL)y + 98), m_textBrush);
        g.DrawString(pm->numBreaksStr.c_str(), -1, &smallFont, PointF((REAL)x + 262, (REAL)y + 98), m_textBrush);

        Font tinyFont(L"Segoe UI", 8, FontStyleRegular, UnitPoint);
        g.DrawString(L"Çalışma", -1, &tinyFont, PointF((REAL)x + 15, (REAL)y + 125), m_textDimBrush);
        g.DrawString(L"Mola", -1, &tinyFont, PointF((REAL)x + 95, (REAL)y + 125), m_textDimBrush);
        g.DrawString(L"Hedef (sa)", -1, &tinyFont, PointF((REAL)x + 175, (REAL)y + 125), m_textDimBrush);
        g.DrawString(L"Mola Sayısı", -1, &tinyFont, PointF((REAL)x + 255, (REAL)y + 125), m_textDimBrush);

        // Buttons: Calculate & Start, Reset
        RectF startBtn((REAL)x + 15, (REAL)y + 155, 195, 30);
        RectF resetBtn((REAL)x + 220, (REAL)y + 155, 105, 30);
        g.FillRectangle(m_buttonBrush, startBtn); g.DrawRectangle(m_borderPen, startBtn);
        g.FillRectangle(m_buttonBrush, resetBtn); g.DrawRectangle(m_borderPen, resetBtn);

        g.DrawString(SettingsManager::Instance().Text("COMPUTE_POMODORO"), -1, &smallFont, PointF((REAL)x + 25, (REAL)y + 161), m_textBrush);
        g.DrawString(SettingsManager::Instance().Text("RESET"), -1, &smallFont, PointF((REAL)x + 245, (REAL)y + 161), m_textBrush);
    }
}

void UIRenderer::RenderToolMenu(Graphics& g, int x, int y, int width, int height) {
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    RectF menuRect((REAL)x, (REAL)y, (REAL)width, (REAL)height);
    g.FillRectangle(m_cardBrush, menuRect);
    g.DrawRectangle(m_borderPen, menuRect);

    Font itemFont(L"Segoe UI", 10, FontStyleRegular, UnitPoint);
    Font exitFont(L"Segoe UI", 9, FontStyleBold, UnitPoint);

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

    int modalW = 320;
    int modalH = 390;
    int modalX = (width - modalW) / 2;
    int modalY = (height - modalH) / 2;

    RectF modalRect((REAL)modalX, (REAL)modalY, (REAL)modalW, (REAL)modalH);
    g.FillRectangle(m_cardBrush, modalRect);
    g.DrawRectangle(m_borderPen, modalRect);

    Font titleFont(L"Segoe UI", 12, FontStyleBold, UnitPoint);
    Font bodyFont(L"Segoe UI", 9, FontStyleRegular, UnitPoint);

    g.DrawString(SettingsManager::Instance().Text("SETTINGS"), -1, &titleFont, PointF((REAL)modalX + 15, (REAL)modalY + 12), m_accentBrush);

    // Close X
    g.DrawString(L"✕", -1, &titleFont, PointF((REAL)modalX + modalW - 25, (REAL)modalY + 10), m_textDimBrush);

    int curY = modalY + 42;
    AppSettings& s = SettingsManager::Instance().Get();

    // 1. Language Toggle
    g.DrawString(SettingsManager::Instance().Text("LANGUAGE"), -1, &bodyFont, PointF((REAL)modalX + 15, (REAL)curY), m_textBrush);
    g.DrawString(s.lang == LANG_TR ? L"[Türkçe]" : L"[English]", -1, &bodyFont, PointF((REAL)modalX + 210, (REAL)curY), m_accentBrush);
    curY += 28;

    // 2. Theme Toggle
    g.DrawString(SettingsManager::Instance().Text("THEME"), -1, &bodyFont, PointF((REAL)modalX + 15, (REAL)curY), m_textBrush);
    g.DrawString(s.theme == THEME_DARK ? SettingsManager::Instance().Text("THEME_DARK") : SettingsManager::Instance().Text("THEME_LIGHT"), -1, &bodyFont, PointF((REAL)modalX + 210, (REAL)curY), m_accentBrush);
    curY += 28;

    // 3. UI Scale Toggle
    g.DrawString(SettingsManager::Instance().Text("UI_SCALE"), -1, &bodyFont, PointF((REAL)modalX + 15, (REAL)curY), m_textBrush);
    g.DrawString(s.uiScale == 120 ? SettingsManager::Instance().Text("SCALE_120") : (s.uiScale == 85 ? SettingsManager::Instance().Text("SCALE_85") : SettingsManager::Instance().Text("SCALE_100")), -1, &bodyFont, PointF((REAL)modalX + 210, (REAL)curY), m_accentBrush);
    curY += 28;

    // 4. Show Seconds in Clock
    g.DrawString(SettingsManager::Instance().Text("SHOW_SECONDS_CLOCK"), -1, &bodyFont, PointF((REAL)modalX + 15, (REAL)curY), m_textBrush);
    g.DrawString(s.showSecondsInClock ? L"[✓]" : L"[  ]", -1, &bodyFont, PointF((REAL)modalX + 275, (REAL)curY), m_accentBrush);
    curY += 28;

    // 5. Show Seconds in Timer
    g.DrawString(SettingsManager::Instance().Text("SHOW_SECONDS_TIMER"), -1, &bodyFont, PointF((REAL)modalX + 15, (REAL)curY), m_textBrush);
    g.DrawString(s.showSecondsInTimer ? L"[✓]" : L"[  ]", -1, &bodyFont, PointF((REAL)modalX + 275, (REAL)curY), m_accentBrush);
    curY += 28;

    // 6. Force Always on Top
    g.DrawString(SettingsManager::Instance().Text("FORCE_TOPMOST"), -1, &bodyFont, PointF((REAL)modalX + 15, (REAL)curY), m_textBrush);
    g.DrawString(s.forceAlwaysOnTop ? L"[✓]" : L"[  ]", -1, &bodyFont, PointF((REAL)modalX + 275, (REAL)curY), m_accentBrush);
    curY += 28;

    // 7. Autostart
    g.DrawString(SettingsManager::Instance().Text("AUTOSTART"), -1, &bodyFont, PointF((REAL)modalX + 15, (REAL)curY), m_textBrush);
    g.DrawString(s.autoStart ? L"[✓]" : L"[  ]", -1, &bodyFont, PointF((REAL)modalX + 275, (REAL)curY), m_accentBrush);
    curY += 28;

    // 8. Restore Previous Tools
    g.DrawString(SettingsManager::Instance().Text("RESTORE_TOOLS"), -1, &bodyFont, PointF((REAL)modalX + 15, (REAL)curY), m_textBrush);
    g.DrawString(s.restorePreviousTools ? L"[✓]" : L"[  ]", -1, &bodyFont, PointF((REAL)modalX + 275, (REAL)curY), m_accentBrush);
    curY += 28;

    // 9. Legal Disclaimer Info Button
    RectF legalBtn((REAL)modalX + 15, (REAL)curY + 5, 290, 28);
    g.FillRectangle(m_buttonBrush, legalBtn); g.DrawRectangle(m_borderPen, legalBtn);
    g.DrawString(SettingsManager::Instance().Text("LEGAL_NOTICE"), -1, &bodyFont, PointF((REAL)modalX + 100, (REAL)curY + 10), m_textBrush);
}
