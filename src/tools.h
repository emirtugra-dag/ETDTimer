#ifndef TOOLS_H
#define TOOLS_H

#include <string>
#include <vector>
#include <windows.h>
#include <gdiplus.h>

enum ToolType {
    TOOL_CLOCK = 0,
    TOOL_STOPWATCH,
    TOOL_TIMER,
    TOOL_POMODORO
};

struct LapRecord {
    int lapIndex;
    DWORD timeMs;
    std::wstring formattedTime;
};

class ToolCard {
public:
    ToolCard(ToolType type);
    virtual ~ToolCard() = default;

    ToolType GetType() const { return m_type; }
    int GetId() const { return m_id; }
    bool IsHidden() const { return m_hidden; }
    void SetHidden(bool h) { m_hidden = h; }

    virtual void Update(DWORD deltaMs) = 0;
    virtual int GetHeight() const { return m_height; }
    virtual void SetWidth(int w) { m_width = w; }
    
    // UI Interaction helpers
    virtual bool OnLButtonDown(int x, int y) { return false; }
    virtual bool OnMouseMove(int x, int y) { return false; }
    virtual bool OnLButtonUp(int x, int y) { return false; }
    virtual void OnCharInput(wchar_t ch) {}

    // Serialization helper
    virtual std::string SerializeState() const { return ""; }

protected:
    ToolType m_type;
    int m_id;
    bool m_hidden = false;
    int m_width = 340;
    int m_height = 120;
    static int s_nextId;
};

// ----------------------------------------------------
// Stopwatch Tool
// ----------------------------------------------------
class StopwatchTool : public ToolCard {
public:
    StopwatchTool();
    void Update(DWORD deltaMs) override;
    bool OnLButtonDown(int x, int y) override;

    bool IsRunning() const { return m_running; }
    DWORD GetElapsedMs() const { return m_elapsedMs; }
    const std::vector<LapRecord>& GetLaps() const { return m_laps; }
    int GetScrollOffset() const { return m_scrollOffset; }
    void ScrollLaps(int delta) { m_scrollOffset = std::max(0, m_scrollOffset + delta); }

private:
    bool m_running = false;
    DWORD m_elapsedMs = 0;
    std::vector<LapRecord> m_laps;
    int m_scrollOffset = 0;
};

// ----------------------------------------------------
// Timer Tool (Duration & Target Time Modes)
// ----------------------------------------------------
enum TimerMode {
    TIMER_MODE_DURATION = 0,
    TIMER_MODE_TARGET_TIME = 1
};

class TimerTool : public ToolCard {
public:
    TimerTool();
    void Update(DWORD deltaMs) override;
    bool OnLButtonDown(int x, int y) override;
    void OnCharInput(wchar_t ch) override;

    TimerMode GetMode() const { return m_mode; }
    bool IsRunning() const { return m_running; }
    int GetRemainingSec() const { return m_remainingSec; }
    int GetInitialSec() const { return m_initialSec; }

    int inputHours = 0;
    int inputMins = 15;
    int activeInputIndex = -1;

private:
    TimerMode m_mode = TIMER_MODE_DURATION;
    bool m_running = false;
    int m_remainingSec = 900; // 15 mins default
    int m_initialSec = 900;
    bool m_finished = false;
};

// ----------------------------------------------------
// Pomodoro Tool
// ----------------------------------------------------
enum PomodoroState {
    POMO_IDLE = 0,
    POMO_WORK,
    POMO_BREAK,
    POMO_FINISHED
};

class PomodoroTool : public ToolCard {
public:
    PomodoroTool();
    void Update(DWORD deltaMs) override;
    bool OnLButtonDown(int x, int y) override;
    void OnCharInput(wchar_t ch) override;

    void ComputePlan();

    PomodoroState GetState() const { return m_state; }
    int GetCurrentSession() const { return m_currentSession; }
    int GetTotalSessions() const { return m_totalWorkSessions; }
    int GetRemainingSec() const { return m_remainingSec; }

    std::wstring workMinStr = L"25";
    std::wstring breakMinStr = L"5";
    std::wstring targetHoursStr = L"4";
    std::wstring numBreaksStr = L"4";
    int activeInputIndex = -1; // 0: work, 1: break, 2: hours, 3: num breaks

private:
    PomodoroState m_state = POMO_IDLE;
    int m_workSec = 25 * 60;
    int m_breakSec = 5 * 60;
    int m_totalWorkSessions = 8;
    int m_currentSession = 1;
    int m_remainingSec = 25 * 60;
    bool m_running = false;
};

#endif // TOOLS_H
