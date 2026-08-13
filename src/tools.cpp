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
// ----------------------------------------------------
// Stopwatch Implementation
// ----------------------------------------------------
StopwatchTool::StopwatchTool() : ToolCard(TOOL_STOPWATCH) {
    m_height = 160;
}

DWORD StopwatchTool::GetElapsedMs() const {
    if (m_running) {
        return (DWORD)(m_accumulatedMs + (GetTickCount64() - m_lastStartTick));
    }
    return (DWORD)m_accumulatedMs;
}

void StopwatchTool::Update(DWORD deltaMs) {
    // Tick tracking is calculated in GetElapsedMs() via GetTickCount64()!
}

bool StopwatchTool::OnLButtonDown(int x, int y) {
    // Start/Pause button: x inside [15, 110], y inside [75, 105]
    if (x >= 15 && x <= 110 && y >= 75 && y <= 105) {
        if (!m_running) {
            m_lastStartTick = GetTickCount64();
            m_running = true;
        } else {
            m_accumulatedMs += (GetTickCount64() - m_lastStartTick);
            m_running = false;
        }
        return true;
    }
    // Lap button: x inside [120, 215], y inside [75, 105]
    if (x >= 120 && x <= 215 && y >= 75 && y <= 105) {
        DWORD curMs = GetElapsedMs();
        if (curMs > 0) {
            LapRecord rec;
            rec.lapIndex = (int)m_laps.size() + 1;
            rec.timeMs = curMs;
            
            DWORD mins = (curMs / 60000) % 60;
            DWORD secs = (curMs / 1000) % 60;
            DWORD ms = (curMs % 1000) / 10;
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
        m_accumulatedMs = 0;
        m_lastStartTick = 0;
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

int TimerTool::GetRemainingSec() const {
    if (m_running && !m_finished) {
        DWORD elapsedSec = (DWORD)((GetTickCount64() - m_lastStartTick) / 1000);
        int rem = m_startRemainingSec - (int)elapsedSec;
        if (rem <= 0) return 0;
        return rem;
    }
    return m_remainingSec;
}

void TimerTool::Update(DWORD deltaMs) {
    if (!m_running || m_finished) return;

    if (GetRemainingSec() <= 0) {
        m_remainingSec = 0;
        m_running = false;
        m_finished = true;
        AudioManager::Instance().PlayAlertSound(SettingsManager::Instance().Get().customSoundPath);
    }
}

bool TimerTool::OnLButtonDown(int x, int y) {
    // Mode toggle tabs: Mode 0 [15, 175], Mode 1 [185, 345], y [30, 56]
    if (y >= 30 && y <= 56) {
        if (x >= 15 && x <= 175) {
            m_mode = TIMER_MODE_DURATION;
            activeInputIndex = -1;
            return true;
        }
        if (x >= 185 && x <= 345) {
            m_mode = TIMER_MODE_TARGET_TIME;
            activeInputIndex = -1;
            return true;
        }
    }

    if (!m_running) {
        if (m_mode == TIMER_MODE_DURATION) {
            // 3 Clean Input Boxes (Saat, Dakika, Saniye): y [92, 122]
            if (x >= 15 && x <= 115 && y >= 92 && y <= 122) { activeInputIndex = 0; return true; } // Hours box
            if (x >= 130 && x <= 230 && y >= 92 && y <= 122) { activeInputIndex = 1; return true; } // Mins box
            if (x >= 245 && x <= 345 && y >= 92 && y <= 122) { activeInputIndex = 2; return true; } // Secs box
        } else {
            // 2 Clean Input Boxes (Hedef Saat, Hedef Dakika): y [92, 122]
            if (x >= 15 && x <= 175 && y >= 92 && y <= 122) { activeInputIndex = 0; return true; } // Target Hours box
            if (x >= 185 && x <= 345 && y >= 92 && y <= 122) { activeInputIndex = 1; return true; } // Target Mins box
        }

        activeInputIndex = -1;

        // Start/Pause Button (stopped state): x [15, 215], y [132, 164]
        if (x >= 15 && x <= 215 && y >= 132 && y <= 164) {
            if (m_mode == TIMER_MODE_DURATION) {
                inputHours = _wtoi(hoursStr.c_str());
                inputMins = _wtoi(minsStr.c_str());
                inputSecs = _wtoi(secsStr.c_str());
                m_remainingSec = inputHours * 3600 + inputMins * 60 + inputSecs;
                if (m_remainingSec <= 0) m_remainingSec = 60;
                m_initialSec = m_remainingSec;
            } else {
                int tHrs = _wtoi(targetHoursStr.c_str());
                int tMins = _wtoi(targetMinsStr.c_str());
                SYSTEMTIME st;
                GetLocalTime(&st);
                int targetSec = tHrs * 3600 + tMins * 60;
                int currentSec = st.wHour * 3600 + st.wMinute * 60 + st.wSecond;
                int diff = targetSec - currentSec;
                if (diff <= 0) diff += 86400; // Next day
                m_remainingSec = diff;
                m_initialSec = diff;
            }
            m_finished = false;
            m_startRemainingSec = m_remainingSec;
            m_lastStartTick = GetTickCount64();
            m_running = true;
            return true;
        }

        // Reset Button (stopped state): x [225, 345], y [132, 164]
        if (x >= 225 && x <= 345 && y >= 132 && y <= 164) {
            m_running = false;
            m_finished = false;
            if (m_mode == TIMER_MODE_DURATION) {
                inputHours = _wtoi(hoursStr.c_str());
                inputMins = _wtoi(minsStr.c_str());
                inputSecs = _wtoi(secsStr.c_str());
                m_remainingSec = inputHours * 3600 + inputMins * 60 + inputSecs;
            }
            return true;
        }
    } else {
        // Start/Pause (Pause) Button (running state): x [15, 215], y [92, 125]
        if (x >= 15 && x <= 215 && y >= 92 && y <= 125) {
            m_remainingSec = GetRemainingSec();
            m_running = false;
            return true;
        }

        // Reset Button (running state): x [225, 345], y [92, 125]
        if (x >= 225 && x <= 345 && y >= 92 && y <= 125) {
            m_running = false;
            m_finished = false;
            return true;
        }
    }

    return false;
}

void TimerTool::OnCharInput(wchar_t ch) {
    std::wstring* target = nullptr;
    if (m_mode == TIMER_MODE_DURATION) {
        if (activeInputIndex == 0) target = &hoursStr;
        else if (activeInputIndex == 1) target = &minsStr;
        else if (activeInputIndex == 2) target = &secsStr;
    } else {
        if (activeInputIndex == 0) target = &targetHoursStr;
        else if (activeInputIndex == 1) target = &targetMinsStr;
    }

    if (target) {
        if (ch == VK_BACK) {
            if (!target->empty()) target->pop_back();
        } else if (ch >= L'0' && ch <= L'9') {
            if (target->length() < 2) *target += ch;
        }
        if (m_mode == TIMER_MODE_DURATION) {
            inputHours = _wtoi(hoursStr.c_str());
            inputMins = _wtoi(minsStr.c_str());
            inputSecs = _wtoi(secsStr.c_str());
        }
    }
}

void TimerTool::SyncInputStrings() {
    wchar_t hb[16], mb[16], sb[16];
    swprintf_s(hb, 16, L"%d", inputHours); hoursStr = hb;
    swprintf_s(mb, 16, L"%d", inputMins); minsStr = mb;
    swprintf_s(sb, 16, L"%d", inputSecs); secsStr = sb;
}

// ----------------------------------------------------
// Pomodoro Implementation
// ----------------------------------------------------
PomodoroTool::PomodoroTool() : ToolCard(TOOL_POMODORO) {
    m_height = 200;
    ComputePlan();
}

int PomodoroTool::GetRemainingSec() const {
    if (m_running && m_state != POMO_IDLE && m_state != POMO_FINISHED) {
        DWORD elapsedSec = (DWORD)((GetTickCount64() - m_lastStartTick) / 1000);
        int rem = m_startRemainingSec - (int)elapsedSec;
        if (rem <= 0) return 0;
        return rem;
    }
    return m_remainingSec;
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

    if (GetRemainingSec() <= 0) {
        AudioManager::Instance().PlayAlertSound(SettingsManager::Instance().Get().customSoundPath);

        if (m_state == POMO_WORK) {
            if (m_currentSession < m_totalWorkSessions) {
                m_state = POMO_BREAK;
                m_remainingSec = m_breakSec;
                m_startRemainingSec = m_breakSec;
                m_lastStartTick = GetTickCount64();
            } else {
                m_state = POMO_FINISHED;
                m_running = false;
                m_remainingSec = 0;
            }
        } else if (m_state == POMO_BREAK) {
            m_currentSession++;
            m_state = POMO_WORK;
            m_remainingSec = m_workSec;
            m_startRemainingSec = m_workSec;
            m_lastStartTick = GetTickCount64();
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
        m_startRemainingSec = m_workSec;
        m_lastStartTick = GetTickCount64();
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
