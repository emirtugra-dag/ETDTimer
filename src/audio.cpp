#include "audio.h"
#include <windows.h>
#include <mmsystem.h>
#include <thread>

AudioManager& AudioManager::Instance() {
    static AudioManager instance;
    return instance;
}

void AudioManager::PlayAlertSound(const std::string& customPath) {
    StopSound();

    if (!customPath.empty()) {
        std::wstring wpath(customPath.begin(), customPath.end());
        std::wstring cmdOpen = L"open \"" + wpath + L"\" type mpegvideo alias etd_alert";
        if (mciSendStringW(cmdOpen.c_str(), NULL, 0, NULL) == 0) {
            mciSendStringW(L"play etd_alert from 0", NULL, 0, NULL);
            return;
        }
    }

    // High-pitched crisp digital timer alarm beep ("Öt-bırak, öt-bırak!" pattern)
    std::thread([]() {
        for (int cycle = 0; cycle < 2; cycle++) {
            Beep(2600, 110);
            Sleep(70);
            Beep(2600, 110);
            Sleep(70);
            Beep(2600, 140);
            Sleep(250);
        }
    }).detach();
}

void AudioManager::StopSound() {
    mciSendStringW(L"close etd_alert", NULL, 0, NULL);
}
