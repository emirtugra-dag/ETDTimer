#define INITGUID
#include <windows.h>
#include <shlobj.h>
#include <objbase.h>
#include <shobjidl.h>
#include <string>
#include <vector>
#include <fstream>

enum SetupStep {
    STEP_LANG_SELECT = 0,
    STEP_LICENSE,
    STEP_FOLDER,
    STEP_OPTIONS,
    STEP_INSTALLING,
    STEP_FINISH
};

enum SetupLang {
    SETUP_TR = 0,
    SETUP_EN = 1
};

static SetupLang g_SetupLang = SETUP_TR;
static SetupStep g_CurrentStep = STEP_LANG_SELECT;
static wchar_t g_InstallDir[MAX_PATH] = {0};
static bool g_CreateDesktopShortcut = true;
static bool g_StartWithWindows = false;
static bool g_LaunchAfterSetup = true;

const wchar_t* g_SetupLegalNoticeTR = 
    L"ETDTimer Kurulum Sihirbazına Hoş Geldiniz.\n\n"
    L"Yasal Uyarı / Sorumluluk Reddi:\n"
    L"Proje yapımcısının, Emir Tuğra Dağ, uygulamadaki herhangi bir şeyi düzeltme, uygulamaya yeni özellik getirme veya güncelleme gibi bir sorumluluğu yok. Proje olduğu gibi sunulmakta ve olası iyi veya kötü hiç bir olayda geliştirici Emir Tuğra Dağ sorumlu olamaz. Kod tabanları MIT lisansına tabi olup projenin adı ve logolarının hakları Emir Tuğra Dağ'da saklıdır ve izinsiz kullanılamaz.\n\n"
    L"Devam etmek için İleri'ye tıklayın.";

const wchar_t* g_SetupLegalNoticeEN = 
    L"Welcome to the ETDTimer Setup Wizard.\n\n"
    L"Legal Disclaimer:\n"
    L"The project creator, Emir Tuğra Dağ, has no responsibility to fix anything in the application, bring new features, or issue updates. The project is provided 'as is' and developer Emir Tuğra Dağ cannot be held responsible for any outcome, good or bad. Codebases are subject to the MIT license; all rights to the project name and logos are reserved by Emir Tuğra Dağ and cannot be used without permission.\n\n"
    L"Click Next to continue.";

// Helper to create a Windows shortcut (.lnk)
bool CreateShortcut(const wchar_t* targetPath, const wchar_t* shortcutPath, const wchar_t* description) {
    HRESULT hr = CoInitialize(NULL);
    bool success = false;
    IShellLinkW* psl = NULL;

    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl))) {
        psl->SetPath(targetPath);
        psl->SetDescription(description);

        IPersistFile* ppf = NULL;
        if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf))) {
            if (SUCCEEDED(ppf->Save(shortcutPath, TRUE))) {
                success = true;
            }
            ppf->Release();
        }
        psl->Release();
    }
    CoUninitialize();
    return success;
}

// Copy file helper
bool CopyFileHelper(const std::wstring& src, const std::wstring& dest) {
    return CopyFileW(src.c_str(), dest.c_str(), FALSE) != 0;
}

void ExecuteInstallation(HWND hwnd) {
    CreateDirectoryW(g_InstallDir, NULL);

    wchar_t currentExePath[MAX_PATH];
    GetModuleFileNameW(NULL, currentExePath, MAX_PATH);
    std::wstring currentDir = currentExePath;
    size_t pos = currentDir.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        currentDir = currentDir.substr(0, pos);
    }

    std::wstring targetExe = std::wstring(g_InstallDir) + L"\\ETDTimer.exe";
    std::wstring srcExe = currentDir + L"\\ETDTimer.exe";

    if (!CopyFileHelper(srcExe, targetExe)) {
        // Fallback if installer is running from same directory
        CopyFileW(currentExePath, targetExe.c_str(), FALSE);
    }

    // Copy PNG logo if available
    std::wstring srcLogo = currentDir + L"\\etdtimer.png";
    std::wstring destLogo = std::wstring(g_InstallDir) + L"\\etdtimer.png";
    CopyFileW(srcLogo.c_str(), destLogo.c_str(), FALSE);

    // Create Desktop Shortcut
    if (g_CreateDesktopShortcut) {
        wchar_t desktopPath[MAX_PATH];
        SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath);
        std::wstring shortcutFile = std::wstring(desktopPath) + L"\\ETDTimer.lnk";
        CreateShortcut(targetExe.c_str(), shortcutFile.c_str(), L"ETDTimer Floating Clock & Tools");
    }

    // Autostart Registry
    if (g_StartWithWindows) {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
            std::wstring qExe = L"\"" + targetExe + L"\"";
            RegSetValueExW(hKey, L"ETDTimer", 0, REG_SZ, (BYTE*)qExe.c_str(), (DWORD)((qExe.length() + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
        }
    }

    // Add/Remove Programs Registry Entry
    HKEY hUnKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ETDTimer", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hUnKey, NULL) == ERROR_SUCCESS) {
        std::wstring dispName = L"ETDTimer";
        std::wstring publisher = L"Emir Tuğra Dağ";
        RegSetValueExW(hUnKey, L"DisplayName", 0, REG_SZ, (BYTE*)dispName.c_str(), (DWORD)((dispName.length() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hUnKey, L"Publisher", 0, REG_SZ, (BYTE*)publisher.c_str(), (DWORD)((publisher.length() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hUnKey, L"InstallLocation", 0, REG_SZ, (BYTE*)g_InstallDir, (DWORD)((wcslen(g_InstallDir) + 1) * sizeof(wchar_t)));
        RegSetValueExW(hUnKey, L"DisplayIcon", 0, REG_SZ, (BYTE*)targetExe.c_str(), (DWORD)((targetExe.length() + 1) * sizeof(wchar_t)));
        RegCloseKey(hUnKey);
    }
}

LRESULT CALLBACK SetupWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        wchar_t localPath[MAX_PATH];
        SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localPath);
        swprintf_s(g_InstallDir, MAX_PATH, L"%s\\Programs\\ETDTimer", localPath);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);

        // Header Background
        HBRUSH headerBrush = CreateSolidBrush(RGB(20, 20, 25));
        RECT headerRect = {0, 0, rect.right, 60};
        FillRect(hdc, &headerRect, headerBrush);
        DeleteObject(headerBrush);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 173, 181));
        HFONT hFontTitle = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFontTitle);

        TextOutW(hdc, 20, 15, L"ETDTimer Setup", 14);

        SelectObject(hdc, hOldFont);
        DeleteObject(hFontTitle);

        // Body Content
        SetTextColor(hdc, RGB(30, 30, 30));
        HFONT hFontBody = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");
        SelectObject(hdc, hFontBody);

        if (g_CurrentStep == STEP_LANG_SELECT) {
            TextOutW(hdc, 30, 90, L"Lütfen Kurulum Dilini Seçin / Please Select Installation Language:", 66);
            TextOutW(hdc, 50, 130, L"1. Türkçe [Tıklayın]", 20);
            TextOutW(hdc, 50, 170, L"2. English [Click Here]", 23);
        } else if (g_CurrentStep == STEP_LICENSE) {
            RECT txtRect = {30, 80, rect.right - 30, 260};
            const wchar_t* notice = (g_SetupLang == SETUP_TR) ? g_SetupLegalNoticeTR : g_SetupLegalNoticeEN;
            DrawTextW(hdc, notice, -1, &txtRect, DT_WORDBREAK);
        } else if (g_CurrentStep == STEP_FOLDER) {
            TextOutW(hdc, 30, 90, (g_SetupLang == SETUP_TR) ? L"Kurulum Dizinini Seçin:" : L"Select Installation Directory:", 28);
            TextOutW(hdc, 30, 130, g_InstallDir, (int)wcslen(g_InstallDir));
        } else if (g_CurrentStep == STEP_OPTIONS) {
            TextOutW(hdc, 30, 90, (g_SetupLang == SETUP_TR) ? L"Kurulum Seçenekleri:" : L"Installation Options:", 22);
            TextOutW(hdc, 50, 130, (g_SetupLang == SETUP_TR) ? L"[✓] Masaüstü Kısayolu Oluştur" : L"[✓] Create Desktop Shortcut", 28);
            TextOutW(hdc, 50, 170, (g_SetupLang == SETUP_TR) ? L"[✓] Windows ile Otomatik Başlat" : L"[✓] Start Automatically with Windows", 35);
        } else if (g_CurrentStep == STEP_INSTALLING) {
            TextOutW(hdc, 30, 120, (g_SetupLang == SETUP_TR) ? L"Dosyalar kopyalanıyor ve kuruluyor..." : L"Copying files and installing...", 37);
        } else if (g_CurrentStep == STEP_FINISH) {
            TextOutW(hdc, 30, 100, (g_SetupLang == SETUP_TR) ? L"Kurulum Başarıyla Tamamlandı!" : L"Installation Successfully Completed!", 36);
            TextOutW(hdc, 30, 140, (g_SetupLang == SETUP_TR) ? L"[✓] ETDTimer uygulamasını şimdi başlat" : L"[✓] Launch ETDTimer application now", 35);
        }

        SelectObject(hdc, hOldFont);
        DeleteObject(hFontBody);

        // Footer buttons bar
        HBRUSH btnBrush = CreateSolidBrush(RGB(240, 240, 245));
        RECT footerRect = {0, rect.bottom - 50, rect.right, rect.bottom};
        FillRect(hdc, &footerRect, btnBrush);
        DeleteObject(btnBrush);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        if (g_CurrentStep == STEP_LANG_SELECT) {
            if (y >= 120 && y <= 150) {
                g_SetupLang = SETUP_TR;
                g_CurrentStep = STEP_LICENSE;
            } else if (y >= 160 && y <= 190) {
                g_SetupLang = SETUP_EN;
                g_CurrentStep = STEP_LICENSE;
            }
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }

        // Footer button click area [y >= 300]
        if (y >= 300) {
            if (g_CurrentStep == STEP_LICENSE) {
                g_CurrentStep = STEP_FOLDER;
            } else if (g_CurrentStep == STEP_FOLDER) {
                g_CurrentStep = STEP_OPTIONS;
            } else if (g_CurrentStep == STEP_OPTIONS) {
                g_CurrentStep = STEP_INSTALLING;
                InvalidateRect(hwnd, NULL, TRUE);
                UpdateWindow(hwnd);
                
                ExecuteInstallation(hwnd);
                
                g_CurrentStep = STEP_FINISH;
            } else if (g_CurrentStep == STEP_FINISH) {
                if (g_LaunchAfterSetup) {
                    std::wstring targetExe = std::wstring(g_InstallDir) + L"\\ETDTimer.exe";
                    ShellExecuteW(NULL, L"open", targetExe.c_str(), NULL, NULL, SW_SHOWNORMAL);
                }
                PostQuitMessage(0);
            }
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }

        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = SetupWindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"ETDTimerSetupClass";

    RegisterClassExW(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int width = 480, height = 360;

    HWND hwnd = CreateWindowExW(
        0, L"ETDTimerSetupClass", L"ETDTimer Setup Wizard",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
        (screenW - width) / 2, (screenH - height) / 2, width, height,
        NULL, NULL, hInstance, NULL
    );

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
