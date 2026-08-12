#include "settings.h"
#include <shlobj.h>
#include <map>
#include <fstream>
#include <sstream>

const wchar_t* g_LegalDisclaimerTR = 
    L"Yasal Uyarı / Sorumluluk Reddi:\n\n"
    L"Proje yapımcısının, Emir Tuğra Dağ, uygulamadaki herhangi bir şeyi düzeltme, uygulamaya yeni özellik getirme veya güncelleme gibi bir sorumluluğu yok. Proje olduğu gibi sunulmakta ve olası iyi veya kötü hiç bir olayda geliştirici Emir Tuğra Dağ sorumlu olamaz. Kod tabanları MIT lisansına tabi olup projenin adı ve logolarının hakları Emir Tuğra Dağ'da saklıdır ve izinsiz kullanılamaz.";

const wchar_t* g_LegalDisclaimerEN = 
    L"Legal Disclaimer:\n\n"
    L"The project creator, Emir Tuğra Dağ, has no responsibility to fix anything in the application, bring new features, or issue updates. The project is provided 'as is' and developer Emir Tuğra Dağ cannot be held responsible for any outcome, good or bad. Codebases are subject to the MIT license; all rights to the project name and logos are reserved by Emir Tuğra Dağ and cannot be used without permission.";

static std::map<std::string, std::pair<std::wstring, std::wstring>> g_Dictionary = {
    {"APP_TITLE", {L"ETDTimer", L"ETDTimer"}},
    {"CLOCK", {L"Saat", L"Clock"}},
    {"STOPWATCH", {L"Kronometre", L"Stopwatch"}},
    {"TIMER", {L"Sayaç", L"Timer"}},
    {"POMODORO", {L"Pomodoro", L"Pomodoro"}},
    {"SETTINGS", {L"Ayarlar", L"Settings"}},
    {"ABOUT", {L"Hakkında", L"About"}},
    {"START", {L"Başlat", L"Start"}},
    {"PAUSE", {L"Durdur", L"Pause"}},
    {"RESET", {L"Sıfırla", L"Reset"}},
    {"LAP", {L"Tur Ekleyin", L"Add Lap"}},
    {"HIDE_TOOLS", {L"Araçları Gizle/Göster", L"Hide/Show Tools"}},
    {"ADD_TOOL", {L"Araç Ekle", L"Add Tool"}},
    {"THEME", {L"Tema", L"Theme"}},
    {"THEME_DARK", {L"Koyu Tema", L"Dark Theme"}},
    {"THEME_LIGHT", {L"Açık Tema", L"Light Theme"}},
    {"LANGUAGE", {L"Dil / Language", L"Language / Dil"}},
    {"SHOW_SECONDS_CLOCK", {L"Saat Görünümünde Saniye Göster", L"Show Seconds in Clock"}},
    {"SHOW_SECONDS_TIMER", {L"Sayaç Görünümünde Saniye Göster", L"Show Seconds in Timers"}},
    {"FORCE_TOPMOST", {L"Zorla En Üstte (Oyunları da Kapla)", L"Force Always on Top (Override Games)"}},
    {"AUTOSTART", {L"Sistem Başlangıcında Çalıştır", L"Start with Windows"}},
    {"RESTORE_TOOLS", {L"Önceki Araçları Açılışta Yükle", L"Restore Previous Tools on Launch"}},
    {"UI_SCALE", {L"Arayüz Boyutu", L"UI Scale"}},
    {"CUSTOM_SOUND", {L"Özel Alarm Sesi (.mp3 / .wav)", L"Custom Alarm Sound (.mp3 / .wav)"}},
    {"BROWSE", {L"Gözat...", L"Browse..."}},
    {"DEFAULT_SOUND", {L"Varsayılan Ses", L"Default Sound"}},
    {"WORK_MIN", {L"Çalışma (dk)", L"Work (min)"}},
    {"BREAK_MIN", {L"Dinlenme (dk)", L"Break (min)"}},
    {"TARGET_HOURS", {L"Toplam Hedef (saat)", L"Total Target (hrs)"}},
    {"NUM_BREAKS", {L"Mola Sayısı", L"Number of Breaks"}},
    {"COMPUTE_POMODORO", {L"Planı Hesapla ve Başlat", L"Calculate Plan & Start"}},
    {"WORK_PHASE", {L"Çalışma Periyodu", L"Work Phase"}},
    {"BREAK_PHASE", {L"Dinlenme Mola Periyodu", L"Break Phase"}},
    {"FINISHED", {L"Tamamlandı!", L"Finished!"}},
    {"MODE_DURATION", {L"Süre Sayımı", L"Duration Countdown"}},
    {"MODE_TARGET_TIME", {L"Hedef Saate Sayım", L"Target Time Countdown"}},
    {"TARGET_TIME_LABEL", {L"Hedef Saat (SS:DK)", L"Target Time (HH:MM)"}},
    {"DURATION_MIN_LABEL", {L"Süre (Dakika)", L"Duration (Minutes)"}},
    {"CLOSE", {L"Kapat", L"Close"}},
    {"SAVE", {L"Kaydet", L"Save"}},
    {"LEGAL_NOTICE", {L"Yasal Bildirim", L"Legal Disclaimer"}},
    {"MAX_TOOL_LIMIT", {L"En fazla 3 adet aynı araçtan ve toplam 10 araç açılabilir!", L"Max 3 instances per tool & max 10 total tools allowed!"}},
    {"TURKISH", {L"Türkçe", L"Turkish"}},
    {"ENGLISH", {L"English", L"English"}}
};

SettingsManager& SettingsManager::Instance() {
    static SettingsManager instance;
    return instance;
}

std::wstring SettingsManager::GetConfigFilePath() {
    wchar_t appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        std::wstring dir = std::wstring(appDataPath) + L"\\ETDTimer";
        CreateDirectoryW(dir.c_str(), NULL);
        return dir + L"\\config.ini";
    }
    return L"config.ini";
}

void SettingsManager::Load() {
    std::wstring cfgFile = GetConfigFilePath();

    m_settings.lang = (Language)GetPrivateProfileIntW(L"Settings", L"Language", LANG_TR, cfgFile.c_str());
    m_settings.theme = (Theme)GetPrivateProfileIntW(L"Settings", L"Theme", THEME_DARK, cfgFile.c_str());
    m_settings.showSecondsInClock = GetPrivateProfileIntW(L"Settings", L"ShowSecondsClock", 0, cfgFile.c_str()) != 0;
    m_settings.showSecondsInTimer = GetPrivateProfileIntW(L"Settings", L"ShowSecondsTimer", 0, cfgFile.c_str()) != 0;
    m_settings.forceAlwaysOnTop = GetPrivateProfileIntW(L"Settings", L"ForceAlwaysOnTop", 0, cfgFile.c_str()) != 0;
    m_settings.restorePreviousTools = GetPrivateProfileIntW(L"Settings", L"RestorePreviousTools", 1, cfgFile.c_str()) != 0;
    m_settings.uiScale = GetPrivateProfileIntW(L"Settings", L"UIScale", 100, cfgFile.c_str());

    wchar_t soundBuf[MAX_PATH] = {0};
    GetPrivateProfileStringW(L"Settings", L"CustomSound", L"", soundBuf, MAX_PATH, cfgFile.c_str());
    char soundMB[MAX_PATH] = {0};
    WideCharToMultiByte(CP_UTF8, 0, soundBuf, -1, soundMB, MAX_PATH, NULL, NULL);
    m_settings.customSoundPath = soundMB;

    wchar_t toolsBuf[1024] = {0};
    GetPrivateProfileStringW(L"Settings", L"LastOpenTools", L"", toolsBuf, 1024, cfgFile.c_str());
    char toolsMB[1024] = {0};
    WideCharToMultiByte(CP_UTF8, 0, toolsBuf, -1, toolsMB, 1024, NULL, NULL);
    m_settings.lastOpenTools = toolsMB;

    m_settings.autoStart = IsAutoStartEnabled();
}

void SettingsManager::Save() {
    std::wstring cfgFile = GetConfigFilePath();

    WritePrivateProfileStringW(L"Settings", L"Language", std::to_wstring((int)m_settings.lang).c_str(), cfgFile.c_str());
    WritePrivateProfileStringW(L"Settings", L"Theme", std::to_wstring((int)m_settings.theme).c_str(), cfgFile.c_str());
    WritePrivateProfileStringW(L"Settings", L"ShowSecondsClock", std::to_wstring(m_settings.showSecondsInClock ? 1 : 0).c_str(), cfgFile.c_str());
    WritePrivateProfileStringW(L"Settings", L"ShowSecondsTimer", std::to_wstring(m_settings.showSecondsInTimer ? 1 : 0).c_str(), cfgFile.c_str());
    WritePrivateProfileStringW(L"Settings", L"ForceAlwaysOnTop", std::to_wstring(m_settings.forceAlwaysOnTop ? 1 : 0).c_str(), cfgFile.c_str());
    WritePrivateProfileStringW(L"Settings", L"RestorePreviousTools", std::to_wstring(m_settings.restorePreviousTools ? 1 : 0).c_str(), cfgFile.c_str());
    WritePrivateProfileStringW(L"Settings", L"UIScale", std::to_wstring(m_settings.uiScale).c_str(), cfgFile.c_str());

    wchar_t soundW[MAX_PATH] = {0};
    MultiByteToWideChar(CP_UTF8, 0, m_settings.customSoundPath.c_str(), -1, soundW, MAX_PATH);
    WritePrivateProfileStringW(L"Settings", L"CustomSound", soundW, cfgFile.c_str());

    wchar_t toolsW[1024] = {0};
    MultiByteToWideChar(CP_UTF8, 0, m_settings.lastOpenTools.c_str(), -1, toolsW, 1024);
    WritePrivateProfileStringW(L"Settings", L"LastOpenTools", toolsW, cfgFile.c_str());

    SetAutoStart(m_settings.autoStart);
}

void SettingsManager::SetAutoStart(bool enable) {
    HKEY hKey;
    LONG lRes = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_ALL_ACCESS, &hKey);
    if (lRes == ERROR_SUCCESS) {
        if (enable) {
            wchar_t exePath[MAX_PATH];
            GetModuleFileNameW(NULL, exePath, MAX_PATH);
            std::wstring quotedPath = L"\"" + std::wstring(exePath) + L"\"";
            RegSetValueExW(hKey, L"ETDTimer", 0, REG_SZ, (BYTE*)quotedPath.c_str(), (DWORD)((quotedPath.length() + 1) * sizeof(wchar_t)));
        } else {
            RegDeleteValueW(hKey, L"ETDTimer");
        }
        RegCloseKey(hKey);
    }
}

bool SettingsManager::IsAutoStartEnabled() {
    HKEY hKey;
    LONG lRes = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey);
    bool enabled = false;
    if (lRes == ERROR_SUCCESS) {
        wchar_t value[MAX_PATH];
        DWORD size = sizeof(value);
        if (RegQueryValueExW(hKey, L"ETDTimer", NULL, NULL, (BYTE*)value, &size) == ERROR_SUCCESS) {
            enabled = true;
        }
        RegCloseKey(hKey);
    }
    return enabled;
}

const wchar_t* SettingsManager::Text(const char* key) {
    auto it = g_Dictionary.find(key);
    if (it != g_Dictionary.end()) {
        return (m_settings.lang == LANG_TR) ? it->second.first.c_str() : it->second.second.c_str();
    }
    return L"";
}
