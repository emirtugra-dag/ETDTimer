#include "audio.h"
#include <windows.h>
#include <mmsystem.h>
#include <fstream>

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

    // Fallback: Default Windows Asterisk or Notification sound
    PlaySoundW(L"SystemAsterisk", NULL, SND_ALIAS | SND_ASYNC);
}

void AudioManager::StopSound() {
    mciSendStringW(L"close etd_alert", NULL, 0, NULL);
}
