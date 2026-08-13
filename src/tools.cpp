#include "tools.h"
#include "audio.h"
#include "settings.h"
#include <cwchar>
#include <cstdio>
#include <algorithm>

int ToolCard::s_nextId = 1;

ToolCard::ToolCard(ToolType type) : m_type(type) {
    m_id = s_nextId++;
}

// ----------------------------------------------------
// Stopwatch Implementation
// ----------------------------------------------------
StopwatchTool::StopwatchTool() : ToolCard(TOOL_STOPWATCH) {
    m_height = 160;
}

void StopwatchTool::Update(DWORD deltaMs) {
    if (m_running) {
        m_elapsedMs += deltaMs;
    }
}

bool StopwatchTool::OnLButtonDown(int x, int y) {
    // Start/Pause button: x inside [15, 110], y inside [75, 105]
    if (x >= 15 && x <= 110 && y >= 75 && y <= 105) {
        m_running = !m_running;
        return true;
    }
    // Lap button: x inside [120, 215], y inside [75, 105]
    if (x >= 120 && x <= 215 && y >= 75 && y <= 105) {
        if (m_elapsedMs > 0) {
            LapRecord rec;
            rec.lapIndex = (int)m_laps.size() + 1;
            rec.timeMs = m_elapsedMs;
            
            DWORD mins = (m_elapsedMs / 60000) % 60;
            DWORD secs = (m_elapsedMs / 1000) % 60;
            DWORD ms = (m_elapsedMs % 1000) / 10;
            wchar_t buf[64];
            swprintf_s(buf, 64, L"#%d  %02d:%02d.%02d", rec.lapIndex, mins, secs, ms);
            rec.formattedTime = buf;
            m_laps.push_back(rec);

            if (m_laps.size() > 2) {
                m_height = std::min(280, 160 + (int)(m_laps.size() - 2) * 24);
            }
        }
        return true;
    }
    // Reset button: x inside [225, 320], y inside [75, 105]
    if (x >= 225 && x <= 320 && y >= 75 && y <= 105) {
        m_running = false;
        m_elapsedMs = 0;
        m_laps.clear();
        m_height = 160;
        return true;
    }

    return false;
}

// ----------------------------------------------------
// Timer Implementation
// ----------------------------------------------------
TimerTool::TimerTool() : ToolCard(TOOL_TIMER) {
    m_height = 180;
    inputHours = 0;
    inputMins = 15;
    m_remainingSec = 900;
    m_initialSec = 900;
}

void TimerTool::Update(DWORD deltaMs) {
    if (!m_running || m_finished) return;

    static DWORD timerAcc = 0;
    timerAcc += deltaMs;
    if (timerAcc >= 1000) {
        int secPassed = timerAcc / 1000;
        timerAcc %= 1000;

        m_remainingSec -= secPassed;
        if (m_remainingSec <= 0) {
            m_remainingSec = 0;
            m_running = false;
            m_finished = true;
            AudioManager::Instance().PlayAlertSound(SettingsManager::Instance().Get().customSoundPath);
        }
    }
}

bool TimerTool::OnLButtonDown(int x, int y) {
    // Mode toggle tabs: Mode 0 [15, 175], Mode 1 [185, 345], y [30, 56]
    if (y >= 30 && y <= 56) {
        if (x >= 15 && x <= 175) {
            m_mode = TIMER_MODE_DURATION;
            return true;
        }
        if (x >= 185 && x <= 345) {
            m_mode = TIMER_MODE_TARGET_TIME;
            return true;
        }
    }

    if (!m_running) {
        // Direct Stepper Row: y [95, 122]
        if (y >= 95 && y <= 122) {
            // Hours - / +
            if (x >= 15 && x <= 40) { if (inputHours > 0) inputHours--; return true; }
            if (x >= 85 && x <= 110) { inputHours++; return true; }
            // Mins - / +
            if (x >= 130 && x <= 155) { if (inputMins > 0) inputMins--; return true; }
            if (x >= 200 && x <= 225) { inputMins = (inputMins + 1) % 60; return true; }
            // Secs - / +
            if (x >= 245 && x <= 270) { if (inputSecs > 0) inputSecs--; return true; }
            if (x >= 315 && x <= 340) { inputSecs = (inputSecs + 5) % 60; return true; }
        }

        // Quick Presets Row: y [128, 154]
        if (y >= 128 && y <= 154) {
            if (x >= 15 && x <= 75) { inputMins += 1; if (inputMins >= 60) { inputHours += inputMins / 60; inputMins %= 60; } return true; } // +1dk
            if (x >= 82 && x <= 142) { inputMins += 5; if (inputMins >= 60) { inputHours += inputMins / 60; inputMins %= 60; } return true; } // +5dk
            if (x >= 149 && x <= 209) { inputMins += 15; if (inputMins >= 60) { inputHours += inputMins / 60; inputMins %= 60; } return true; } // +15dk
            if (x >= 216 && x <= 276) { inputHours += 1; return true; } // +1sa
            if (x >= 283 && x <= 345) { inputHours = 0; inputMins = 0; inputSecs = 0; return true; } // Temizle
        }

        // Start/Pause Button (stopped state): x [15, 215], y [162, 192]
        if (x >= 15 && x <= 215 && y >= 162 && y <= 192) {
            if (m_mode == TIMER_MODE_DURATION) {
                m_remainingSec = inputHours * 3600 + inputMins * 60 + inputSecs;
                if (m_remainingSec <= 0) m_remainingSec = 60;
                m_initialSec = m_remainingSec;
            } else {
                SYSTEMTIME st;
                GetLocalTime(&st);
                int targetSec = inputHours * 3600 + inputMins * 60 + inputSecs;
                int currentSec = st.wHour * 3600 + st.wMinute * 60 + st.wSecond;
                int diff = targetSec - currentSec;
                if (diff <= 0) diff += 86400; // Next day
                m_remainingSec = diff;
                m_initialSec = diff;
            }
            m_finished = false;
            m_running = true;
            return true;
        }

        // Reset Button (stopped state): x [225, 345], y [162, 192]
        if (x >= 225 && x <= 345 && y >= 162 && y <= 192) {
            m_running = false;
            m_finished = false;
            if (m_mode == TIMER_MODE_DURATION) {
                m_remainingSec = inputHours * 3600 + inputMins * 60 + inputSecs;
            }
            return true;
        }
    } else {
        // Start/Pause (Pause) Button (running state): x [15, 215], y [92, 125]
        if (x >= 15 && x <= 215 && y >= 92 && y <= 125) {
            m_running = false;
            return true;
        }

        // Reset Button (running state): x [225, 345], y [92, 125]
        if (x >= 225 && x <= 345 && y >= 92 && y <= 125) {
            m_running = false;
            m_finished = false;
            if (m_mode == TIMER_MODE_DURATION) {
                m_remainingSec = inputHours * 3600 + inputMins * 60 + inputSecs;
            }
            return true;
        }
    }

    return false;
}

void TimerTool::OnCharInput(wchar_t ch) {}

// ----------------------------------------------------
// Pomodoro Implementation
// ----------------------------------------------------
PomodoroTool::PomodoroTool() : ToolCard(TOOL_POMODORO) {
    m_height = 200;
    ComputePlan();
}

void PomodoroTool::ComputePlan() {
    int wMin = _wtoi(workMinStr.c_str());
    int bMin = _wtoi(breakMinStr.c_str());
    int tHrs = _wtoi(targetHoursStr.c_str());

    if (wMin <= 0) wMin = 25;
    if (bMin <= 0) bMin = 5;
    if (tHrs <= 0) tHrs = 4;

    m_workSec = wMin * 60;
    m_breakSec = bMin * 60;

    int targetMins = tHrs * 60;
    int cycleMins = wMin + bMin;
    m_totalWorkSessions = std::max(1, targetMins / cycleMins);
    m_currentSession = 1;
    m_remainingSec = m_workSec;
    m_state = POMO_IDLE;
    m_running = false;
}

void PomodoroTool::Update(DWORD deltaMs) {
    if (!m_running || m_state == POMO_IDLE || m_state == POMO_FINISHED) return;

    static DWORD pomAcc = 0;
    pomAcc += deltaMs;
    if (pomAcc >= 1000) {
        int secPassed = pomAcc / 1000;
        pomAcc %= 1000;

        m_remainingSec -= secPassed;
        if (m_remainingSec <= 0) {
            AudioManager::Instance().PlayAlertSound(SettingsManager::Instance().Get().customSoundPath);

            if (m_state == POMO_WORK) {
                if (m_currentSession < m_totalWorkSessions) {
                    m_state = POMO_BREAK;
                    m_remainingSec = m_breakSec;
                } else {
                    m_state = POMO_FINISHED;
                    m_running = false;
                    m_remainingSec = 0;
                }
            } else if (m_state == POMO_BREAK) {
                m_currentSession++;
                m_state = POMO_WORK;
                m_remainingSec = m_workSec;
            }
        }
    }
}

bool PomodoroTool::OnLButtonDown(int x, int y) {
    // 3 Input Parameter Boxes:
    // Work min box: x [15, 115], y [95, 121]
    if (x >= 15 && x <= 115 && y >= 95 && y <= 121) { activeInputIndex = 0; return true; }
    // Break min box: x [130, 230], y [95, 121]
    if (x >= 130 && x <= 230 && y >= 95 && y <= 121) { activeInputIndex = 1; return true; }
    // Target Hours box: x [245, 345], y [95, 121]
    if (x >= 245 && x <= 345 && y >= 95 && y <= 121) { activeInputIndex = 2; return true; }

    activeInputIndex = -1;

    // Calculate & Start Button: x [15, 220], y [155, 188]
    if (x >= 15 && x <= 220 && y >= 155 && y <= 188) {
        ComputePlan();
        m_state = POMO_WORK;
        m_running = true;
        return true;
    }

    // Reset Button: x [230, 345], y [155, 188]
    if (x >= 230 && x <= 345 && y >= 155 && y <= 188) {
        ComputePlan();
        return true;
    }

    return false;
}

void PomodoroTool::OnCharInput(wchar_t ch) {
    std::wstring* target = nullptr;
    if (activeInputIndex == 0) target = &workMinStr;
    else if (activeInputIndex == 1) target = &breakMinStr;
    else if (activeInputIndex == 2) target = &targetHoursStr;

    if (target) {
        if (ch == VK_BACK) {
            if (!target->empty()) target->pop_back();
        } else if (ch >= L'0' && ch <= L'9') {
            if (target->length() < 3) *target += ch;
        }
    }
}
