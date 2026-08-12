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
    // Button area checks relative to card bounds
    // [Start/Pause] [Lap] [Reset]
    // Start/Pause button: x inside [20, 100], y inside [50, 80]
    if (x >= 20 && x <= 100 && y >= 50 && y <= 80) {
        m_running = !m_running;
        return true;
    }
    // Lap button: x inside [110, 190], y inside [50, 80]
    if (x >= 110 && x <= 190 && y >= 50 && y <= 80) {
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
    // Reset button: x inside [200, 280], y inside [50, 80]
    if (x >= 200 && x <= 280 && y >= 50 && y <= 80) {
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
    m_height = 150;
}

void TimerTool::Update(DWORD deltaMs) {
    if (!m_running || m_finished) return;

    static DWORD accumulator = 0;
    accumulator += deltaMs;
    if (accumulator >= 1000) {
        int secPassed = accumulator / 1000;
        accumulator %= 1000;

        if (m_mode == TIMER_MODE_DURATION) {
            m_remainingSec -= secPassed;
            if (m_remainingSec <= 0) {
                m_remainingSec = 0;
                m_running = false;
                m_finished = true;
                AudioManager::Instance().PlayAlertSound(SettingsManager::Instance().Get().customSoundPath);
            }
        } else { // TARGET TIME MODE
            SYSTEMTIME st;
            GetLocalTime(&st);
            int nowSec = st.wHour * 3600 + st.wMinute * 60 + st.wSecond;

            int targetH = 17, targetM = 30;
            swscanf_s(targetTimeInputStr.c_str(), L"%d:%d", &targetH, &targetM);
            int targetSec = targetH * 3600 + targetM * 60;

            int diff = targetSec - nowSec;
            if (diff < 0) diff += 86400; // Next day fallback

            m_remainingSec = diff;
            if (m_remainingSec <= 0) {
                m_remainingSec = 0;
                m_running = false;
                m_finished = true;
                AudioManager::Instance().PlayAlertSound(SettingsManager::Instance().Get().customSoundPath);
            }
        }
    }
}

bool TimerTool::OnLButtonDown(int x, int y) {
    // Mode toggle tabs: Mode 0 [20, 140], Mode 1 [150, 270], y [40, 65]
    if (y >= 40 && y <= 65) {
        if (x >= 20 && x <= 140) {
            m_mode = TIMER_MODE_DURATION;
            return true;
        }
        if (x >= 150 && x <= 270) {
            m_mode = TIMER_MODE_TARGET_TIME;
            return true;
        }
    }

    // Input Box Click: x [20, 120], y [75, 100]
    if (x >= 20 && x <= 120 && y >= 75 && y <= 100) {
        activeInputIndex = 0;
        return true;
    } else {
        activeInputIndex = -1;
    }

    // Start/Pause Button: x [140, 210], y [75, 105]
    if (x >= 140 && x <= 210 && y >= 75 && y <= 105) {
        if (!m_running) {
            if (m_mode == TIMER_MODE_DURATION) {
                int mins = _wtoi(durationInputStr.c_str());
                if (mins <= 0) mins = 1;
                m_remainingSec = mins * 60;
                m_initialSec = m_remainingSec;
            }
            m_finished = false;
            m_running = true;
        } else {
            m_running = false;
        }
        return true;
    }

    // Reset Button: x [220, 290], y [75, 105]
    if (x >= 220 && x <= 290 && y >= 75 && y <= 105) {
        m_running = false;
        m_finished = false;
        if (m_mode == TIMER_MODE_DURATION) {
            int mins = _wtoi(durationInputStr.c_str());
            m_remainingSec = mins * 60;
        }
        return true;
    }

    return false;
}

void TimerTool::OnCharInput(wchar_t ch) {
    if (activeInputIndex == 0) {
        if (ch == VK_BACK) {
            if (m_mode == TIMER_MODE_DURATION && !durationInputStr.empty()) durationInputStr.pop_back();
            if (m_mode == TIMER_MODE_TARGET_TIME && !targetTimeInputStr.empty()) targetTimeInputStr.pop_back();
        } else if ((ch >= L'0' && ch <= L'9') || (m_mode == TIMER_MODE_TARGET_TIME && ch == L':')) {
            if (m_mode == TIMER_MODE_DURATION && durationInputStr.length() < 4) durationInputStr += ch;
            if (m_mode == TIMER_MODE_TARGET_TIME && targetTimeInputStr.length() < 5) targetTimeInputStr += ch;
        }
    }
}

// ----------------------------------------------------
// Pomodoro Implementation
// ----------------------------------------------------
PomodoroTool::PomodoroTool() : ToolCard(TOOL_POMODORO) {
    m_height = 190;
    ComputePlan();
}

void PomodoroTool::ComputePlan() {
    int wMin = _wtoi(workMinStr.c_str());
    int bMin = _wtoi(breakMinStr.c_str());
    int numB = _wtoi(numBreaksStr.c_str());

    if (wMin <= 0) wMin = 25;
    if (bMin <= 0) bMin = 5;
    if (numB < 0) numB = 4;

    m_workSec = wMin * 60;
    m_breakSec = bMin * 60;
    m_totalWorkSessions = numB + 1;
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
    // Inputs:
    // Work min box: x [15, 75], y [40, 65]
    if (x >= 15 && x <= 75 && y >= 40 && y <= 65) { activeInputIndex = 0; return true; }
    // Break min box: x [85, 145], y [40, 65]
    if (x >= 85 && x <= 145 && y >= 40 && y <= 65) { activeInputIndex = 1; return true; }
    // Target Hours box: x [155, 215], y [40, 65]
    if (x >= 155 && x <= 215 && y >= 40 && y <= 65) { activeInputIndex = 2; return true; }
    // Num Breaks box: x [225, 295], y [40, 65]
    if (x >= 225 && x <= 295 && y >= 40 && y <= 65) { activeInputIndex = 3; return true; }

    activeInputIndex = -1;

    // Calculate & Start Button: x [20, 180], y [140, 175]
    if (x >= 20 && x <= 180 && y >= 140 && y <= 175) {
        ComputePlan();
        m_state = POMO_WORK;
        m_running = true;
        return true;
    }

    // Reset Button: x [190, 280], y [140, 175]
    if (x >= 190 && x <= 280 && y >= 140 && y <= 175) {
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
    else if (activeInputIndex == 3) target = &numBreaksStr;

    if (target) {
        if (ch == VK_BACK) {
            if (!target->empty()) target->pop_back();
        } else if (ch >= L'0' && ch <= L'9') {
            if (target->length() < 3) *target += ch;
        }
    }
}
