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
    // Mode toggle tabs: Mode 0 [15, 160], Mode 1 [170, 325], y [30, 56]
    if (y >= 30 && y <= 56) {
        if (x >= 15 && x <= 160) {
            m_mode = TIMER_MODE_DURATION;
            return true;
        }
        if (x >= 170 && x <= 325) {
            m_mode = TIMER_MODE_TARGET_TIME;
            return true;
        }
    }

    // Input Box Click: x [15, 125], y [105, 135]
    if (x >= 15 && x <= 125 && y >= 105 && y <= 135) {
        activeInputIndex = 0;
        return true;
    } else {
        activeInputIndex = -1;
    }

    // Start/Pause Button: x [135, 225], y [105, 135]
    if (x >= 135 && x <= 225 && y >= 105 && y <= 135) {
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

    // Reset Button: x [235, 325], y [105, 135]
    if (x >= 235 && x <= 325 && y >= 105 && y <= 135) {
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
    m_height = 200;
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
    // Work min box: x [15, 85], y [95, 120]
    if (x >= 15 && x <= 85 && y >= 95 && y <= 120) { activeInputIndex = 0; return true; }
    // Break min box: x [95, 165], y [95, 120]
    if (x >= 95 && x <= 165 && y >= 95 && y <= 120) { activeInputIndex = 1; return true; }
    // Target Hours box: x [175, 245], y [95, 120]
    if (x >= 175 && x <= 245 && y >= 95 && y <= 120) { activeInputIndex = 2; return true; }
    // Num Breaks box: x [255, 325], y [95, 120]
    if (x >= 255 && x <= 325 && y >= 95 && y <= 120) { activeInputIndex = 3; return true; }

    activeInputIndex = -1;

    // Calculate & Start Button: x [15, 210], y [155, 188]
    if (x >= 15 && x <= 210 && y >= 155 && y <= 188) {
        ComputePlan();
        m_state = POMO_WORK;
        m_running = true;
        return true;
    }

    // Reset Button: x [220, 325], y [155, 188]
    if (x >= 220 && x <= 325 && y >= 155 && y <= 188) {
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
