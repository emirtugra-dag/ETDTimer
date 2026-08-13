#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <vector>
#include <windows.h>

enum Language {
    LANG_TR = 0,
    LANG_EN = 1
};

enum Theme {
    THEME_DARK = 0,
    THEME_LIGHT = 1
};

struct AppSettings {
    Language lang = LANG_TR;
    Theme theme = THEME_DARK;
    bool showSecondsInClock = false;
    bool showSecondsInTimer = false;
    bool forceAlwaysOnTop = false;
    bool autoStart = false;
    bool restorePreviousTools = true;
    bool clockOnlyMode = false;
    int uiScale = 100; // 80, 100, 120
    std::string customSoundPath = "";
    std::string lastOpenTools = ""; // comma-separated list of tool types
};

class SettingsManager {
public:
    static SettingsManager& Instance();

    void Load();
    void Save();

    AppSettings& Get() { return m_settings; }
    std::wstring GetConfigFilePath();

    void SetAutoStart(bool enable);
    bool IsAutoStartEnabled();

    // Localization helper
    const wchar_t* Text(const char* key);

private:
    SettingsManager() = default;
    AppSettings m_settings;
};

// Legal disclaimer notice constant
extern const wchar_t* g_LegalDisclaimerTR;
extern const wchar_t* g_LegalDisclaimerEN;

#endif // SETTINGS_H
