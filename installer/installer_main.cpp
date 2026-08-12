#define INITGUID
#include <windows.h>
#include <shlobj.h>
#include <objbase.h>
#include <shobjidl.h>
#include <string>
#include <vector>

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

// Hover state tracking
static int g_HoverButton = -1;

const wchar_t* g_SetupLegalNoticeTR = 
    L"YASAL BİLDİRİM VE SORUMLULUK REDDİ:\n\n"
    L"Bu yazılım 'olduğu gibi' (As-Is) sunulmaktadır. Geliştirici Emir Tuğra Dağ, uygulamanın kullanımından doğabilecek doğrudan ya da dolaylı hiçbir durum, zarar veya aksaklıktan sorumlu tutulamaz.\n\n"
    L"Geliştiricinin yazılıma güncelleme getirme veya bakım yapma zorunluluğu bulunmamaktadır. Kod tabanı MIT Lisansına tabi olup; projenin adı, logosu ve tüm hakları Emir Tuğra Dağ'a aittir.";

const wchar_t* g_SetupLegalNoticeEN = 
    L"LEGAL DISCLAIMER & NOTICE:\n\n"
    L"This software is provided 'as is', without warranty of any kind. The developer, Emir Tuğra Dağ, shall not be held liable for any claims, damages, or issues arising from the use of this software.\n\n"
    L"The developer assumes no obligation to provide updates or new features. Codebase is under the MIT License; all rights to the project name and logos are reserved by Emir Tuğra Dağ.";

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

void CloseRunningInstances() {
    HWND hwnd = FindWindowW(L"ETDTimerWindowClass", NULL);
    if (hwnd) {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
        for (int i = 0; i < 15; i++) {
            Sleep(100);
            if (!IsWindow(hwnd)) break;
        }
        if (IsWindow(hwnd) && pid != 0) {
            HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
            if (hProc) {
                TerminateProcess(hProc, 0);
                CloseHandle(hProc);
            }
        }
    }
}

void ExecuteUninstallation() {
    int res = MessageBoxW(NULL, 
        L"ETDTimer bilgisayarınızdan kaldırılacaktır.\nDevam etmek istiyor musunuz?", 
        L"ETDTimer Kaldırma Sihirbazı", 
        MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);

    if (res != IDYES) return;

    // 1. Close active app instances
    CloseRunningInstances();

    // 2. Remove autostart registry entry
    HKEY hRunKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_ALL_ACCESS, &hRunKey) == ERROR_SUCCESS) {
        RegDeleteValueW(hRunKey, L"ETDTimer");
        RegCloseKey(hRunKey);
    }

    // 3. Remove Control Panel Uninstall registry key
    RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ETDTimer");

    // 4. Remove Desktop Shortcut
    wchar_t desktopPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath))) {
        std::wstring shortcutFile = std::wstring(desktopPath) + L"\\ETDTimer.lnk";
        DeleteFileW(shortcutFile.c_str());
    }

    // 5. Ask user if settings should be removed
    int delData = MessageBoxW(NULL, 
        L"Kullanıcı ayarlarınız ve geçmiş verileriniz de silinsin mi?\n(Önceki ayarlarınızı saklamak istiyorsanız 'Hayır'ı seçin)", 
        L"ETDTimer Kaldırma Sihirbazı", 
        MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);

    if (delData == IDYES) {
        wchar_t appDataPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
            std::wstring configDir = std::wstring(appDataPath) + L"\\ETDTimer";
            std::wstring configFile = configDir + L"\\config.ini";
            DeleteFileW(configFile.c_str());
            RemoveDirectoryW(configDir.c_str());
        }
    }

    // 6. Delete install dir files & directory asynchronously via background cmd
    wchar_t currentExe[MAX_PATH];
    GetModuleFileNameW(NULL, currentExe, MAX_PATH);
    std::wstring currentDir = currentExe;
    size_t pos = currentDir.find_last_of(L"\\/");
    if (pos != std::wstring::npos) currentDir = currentDir.substr(0, pos);

    std::wstring cmd = L"cmd.exe /c start /b \"\" cmd /c \"timeout /t 1 /nobreak >nul & rmdir /s /q \"" + currentDir + L"\"\"";

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi;
    if (CreateProcessW(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    MessageBoxW(NULL, L"ETDTimer başarıyla bilgisayarınızdan kaldırıldı.", L"ETDTimer Kaldırıldı", MB_OK | MB_ICONINFORMATION);
}

void ExecuteInstallation(HWND hwnd) {
    // Automatically close any running ETDTimer instance before updating!
    CloseRunningInstances();

    CreateDirectoryW(g_InstallDir, NULL);

    wchar_t currentExePath[MAX_PATH];
    GetModuleFileNameW(NULL, currentExePath, MAX_PATH);
    std::wstring currentDir = currentExePath;
    size_t pos = currentDir.find_last_of(L"\\/");
    if (pos != std::wstring::npos) currentDir = currentDir.substr(0, pos);

    std::wstring targetExe = std::wstring(g_InstallDir) + L"\\ETDTimer.exe";

    // 1. Try to extract embedded payload resource (ID 102, Type 256)
    HMODULE hModule = GetModuleHandleW(NULL);
    HRSRC hRes = FindResourceW(hModule, MAKEINTRESOURCEW(102), MAKEINTRESOURCEW(256));
    bool extracted = false;

    if (hRes) {
        HGLOBAL hMem = LoadResource(hModule, hRes);
        DWORD resSize = SizeofResource(hModule, hRes);
        void* pData = LockResource(hMem);

        if (pData && resSize > 0) {
            HANDLE hFile = CreateFileW(targetExe.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                DWORD written = 0;
                WriteFile(hFile, pData, resSize, &written, NULL);
                CloseHandle(hFile);
                extracted = true;
            }
        }
    }

    // 2. Fallback to copying next to setup if resource is missing
    if (!extracted) {
        std::wstring srcExe = currentDir + L"\\ETDTimer.exe";
        if (GetFileAttributesW(srcExe.c_str()) != INVALID_FILE_ATTRIBUTES) {
            CopyFileW(srcExe.c_str(), targetExe.c_str(), FALSE);
        }
    }

    std::wstring srcLogo = currentDir + L"\\etdtimer.png";
    std::wstring destLogo = std::wstring(g_InstallDir) + L"\\etdtimer.png";
    CopyFileW(srcLogo.c_str(), destLogo.c_str(), FALSE);

    // 3. Create standalone Uninstall.exe in target program directory
    std::wstring targetUninstallExe = std::wstring(g_InstallDir) + L"\\Uninstall.exe";
    CopyFileW(currentExePath, targetUninstallExe.c_str(), FALSE);

    // 4. Desktop Shortcut
    if (g_CreateDesktopShortcut) {
        wchar_t desktopPath[MAX_PATH];
        SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath);
        std::wstring shortcutFile = std::wstring(desktopPath) + L"\\ETDTimer.lnk";
        CreateShortcut(targetExe.c_str(), shortcutFile.c_str(), L"ETDTimer Floating Clock & Tools");
    }

    // 5. Autostart Registry
    if (g_StartWithWindows) {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
            std::wstring qExe = L"\"" + targetExe + L"\"";
            RegSetValueExW(hKey, L"ETDTimer", 0, REG_SZ, (BYTE*)qExe.c_str(), (DWORD)((qExe.length() + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
        }
    }

    // 6. Control Panel Uninstall Registry Registration
    HKEY hUnKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ETDTimer", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hUnKey, NULL) == ERROR_SUCCESS) {
        std::wstring dispName = L"ETDTimer";
        std::wstring publisher = L"Emir Tuğra Dağ";
        std::wstring version = L"1.0.0";
        std::wstring iconPath = targetExe + L",0";
        std::wstring uninstallCmd = L"\"" + targetUninstallExe + L"\" /uninstall";

        RegSetValueExW(hUnKey, L"DisplayName", 0, REG_SZ, (BYTE*)dispName.c_str(), (DWORD)((dispName.length() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hUnKey, L"Publisher", 0, REG_SZ, (BYTE*)publisher.c_str(), (DWORD)((publisher.length() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hUnKey, L"DisplayVersion", 0, REG_SZ, (BYTE*)version.c_str(), (DWORD)((version.length() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hUnKey, L"InstallLocation", 0, REG_SZ, (BYTE*)g_InstallDir, (DWORD)((wcslen(g_InstallDir) + 1) * sizeof(wchar_t)));
        RegSetValueExW(hUnKey, L"DisplayIcon", 0, REG_SZ, (BYTE*)iconPath.c_str(), (DWORD)((iconPath.length() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hUnKey, L"UninstallString", 0, REG_SZ, (BYTE*)uninstallCmd.c_str(), (DWORD)((uninstallCmd.length() + 1) * sizeof(wchar_t)));

        DWORD sizeKB = 2000;
        RegSetValueExW(hUnKey, L"EstimatedSize", 0, REG_DWORD, (BYTE*)&sizeKB, sizeof(DWORD));
        DWORD noModify = 1;
        RegSetValueExW(hUnKey, L"NoModify", 0, REG_DWORD, (BYTE*)&noModify, sizeof(DWORD));
        DWORD noRepair = 1;
        RegSetValueExW(hUnKey, L"NoRepair", 0, REG_DWORD, (BYTE*)&noRepair, sizeof(DWORD));

        RegCloseKey(hUnKey);
    }
}

// Button drawing helper
void DrawStyledButton(HDC hdc, RECT rect, const wchar_t* text, bool isHover, bool isPrimary, bool isSelected = false) {
    COLORREF bgCol = isSelected ? RGB(0, 173, 181) : (isPrimary ? (isHover ? RGB(0, 195, 205) : RGB(0, 173, 181)) : (isHover ? RGB(230, 235, 242) : RGB(245, 247, 250)));
    COLORREF textCol = (isPrimary || isSelected) ? RGB(255, 255, 255) : RGB(30, 35, 45);
    COLORREF borderCol = isSelected ? RGB(0, 150, 160) : (isPrimary ? RGB(0, 150, 160) : RGB(200, 205, 215));

    HBRUSH brush = CreateSolidBrush(bgCol);
    HPEN pen = CreatePen(PS_SOLID, 1, borderCol);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);

    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 6, 6);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textCol);

    HFONT hFont = CreateFontW(15, 0, 0, 0, (isPrimary || isSelected) ? FW_BOLD : FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

    DrawTextW(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
}

LRESULT CALLBACK SetupWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        wchar_t localPath[MAX_PATH];
        SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localPath);
        swprintf_s(g_InstallDir, MAX_PATH, L"%s\\Programs\\ETDTimer", localPath);
        return 0;
    }

    case WM_MOUSEMOVE: {
        TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);

        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        int oldHover = g_HoverButton;
        g_HoverButton = -1;

        RECT rect;
        GetClientRect(hwnd, &rect);
        int footerTop = rect.bottom - 50;
        int footerBottom = rect.bottom - 10;

        if (y >= footerTop && y <= footerBottom) {
            if (x >= 20 && x <= 120) g_HoverButton = 10; // Back / Cancel
            if (x >= 380 && x <= 490) g_HoverButton = 11; // Next / Install / Finish
        } else if (g_CurrentStep == STEP_LANG_SELECT && y >= 140 && y <= 210) {
            if (x >= 40 && x <= 240) g_HoverButton = 1; // TR Button
            if (x >= 270 && x <= 470) g_HoverButton = 2; // EN Button
        }

        if (oldHover != g_HoverButton) {
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MOUSELEAVE:
        if (g_HoverButton != -1) {
            g_HoverButton = -1;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);

        // Double buffer
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

        // Background
        HBRUSH bgBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(memDC, &rect, bgBrush);
        DeleteObject(bgBrush);

        // Header Banner (#1E1E24)
        HBRUSH headerBrush = CreateSolidBrush(RGB(24, 24, 30));
        RECT headerRect = {0, 0, rect.right, 70};
        FillRect(memDC, &headerRect, headerBrush);
        DeleteObject(headerBrush);

        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, RGB(0, 173, 181));
        HFONT hFontTitle = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");
        HFONT oldFont = (HFONT)SelectObject(memDC, hFontTitle);

        TextOutW(memDC, 25, 12, L"ETDTimer Kurulum Sihirbazı", 26);

        SelectObject(memDC, oldFont);
        DeleteObject(hFontTitle);

        SetTextColor(memDC, RGB(160, 165, 175));
        HFONT hFontSub = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");
        SelectObject(memDC, hFontSub);

        const wchar_t* subTitle = L"";
        if (g_CurrentStep == STEP_LANG_SELECT) subTitle = (g_SetupLang == SETUP_TR) ? L"Kurulum Dili Seçimi / Select Setup Language" : L"Select Setup Language / Kurulum Dili Seçimi";
        else if (g_CurrentStep == STEP_LICENSE) subTitle = (g_SetupLang == SETUP_TR) ? L"Yasal Uyarı ve Lisans Sözleşmesi" : L"Legal Disclaimer & License Agreement";
        else if (g_CurrentStep == STEP_FOLDER) subTitle = (g_SetupLang == SETUP_TR) ? L"Kurulum Hedef Dizin Seçimi" : L"Select Destination Folder";
        else if (g_CurrentStep == STEP_OPTIONS) subTitle = (g_SetupLang == SETUP_TR) ? L"Kısayol ve Başlangıç Seçenekleri" : L"Shortcuts & Startup Options";
        else if (g_CurrentStep == STEP_INSTALLING) subTitle = (g_SetupLang == SETUP_TR) ? L"Dosyalar Kopyalanıyor..." : L"Copying Files...";
        else if (g_CurrentStep == STEP_FINISH) subTitle = (g_SetupLang == SETUP_TR) ? L"Kurulum Tamamlandı!" : L"Setup Completed Successfully!";

        TextOutW(memDC, 25, 40, subTitle, (int)wcslen(subTitle));

        SelectObject(memDC, oldFont);
        DeleteObject(hFontSub);

        // Body rendering
        HFONT hFontBody = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");
        HFONT hFontBodyBold = CreateFontW(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");
        SelectObject(memDC, hFontBody);
        SetTextColor(memDC, RGB(40, 45, 55));

        if (g_CurrentStep == STEP_LANG_SELECT) {
            SelectObject(memDC, hFontBodyBold);
            TextOutW(memDC, 30, 95, L"Lütfen Kurulum Dilini Seçiniz / Please Select Installation Language:", 67);
            SelectObject(memDC, hFontBody);

            RECT trBtnRect = {40, 140, 240, 210};
            RECT enBtnRect = {270, 140, 470, 210};

            DrawStyledButton(memDC, trBtnRect, L"🇹🇷  Türkçe", g_HoverButton == 1, false, g_SetupLang == SETUP_TR);
            DrawStyledButton(memDC, enBtnRect, L"🇬🇧  English", g_HoverButton == 2, false, g_SetupLang == SETUP_EN);

        } else if (g_CurrentStep == STEP_LICENSE) {
            RECT txtBoxRect = {30, 90, rect.right - 30, 290};
            HBRUSH boxBg = CreateSolidBrush(RGB(248, 249, 252));
            HPEN boxPen = CreatePen(PS_SOLID, 1, RGB(220, 225, 235));
            SelectObject(memDC, boxBg);
            SelectObject(memDC, boxPen);
            RoundRect(memDC, txtBoxRect.left, txtBoxRect.top, txtBoxRect.right, txtBoxRect.bottom, 6, 6);
            DeleteObject(boxBg);
            DeleteObject(boxPen);

            RECT innerRect = {40, 100, rect.right - 40, 280};
            const wchar_t* notice = (g_SetupLang == SETUP_TR) ? g_SetupLegalNoticeTR : g_SetupLegalNoticeEN;
            DrawTextW(memDC, notice, -1, &innerRect, DT_WORDBREAK);

        } else if (g_CurrentStep == STEP_FOLDER) {
            TextOutW(memDC, 30, 100, (g_SetupLang == SETUP_TR) ? L"Uygulama aşağıdaki dizine kurulacaktır:" : L"Application will be installed to:", 40);

            RECT pathRect = {30, 135, rect.right - 30, 175};
            HBRUSH boxBg = CreateSolidBrush(RGB(248, 249, 252));
            HPEN boxPen = CreatePen(PS_SOLID, 1, RGB(210, 215, 225));
            SelectObject(memDC, boxBg); SelectObject(memDC, boxPen);
            RoundRect(memDC, pathRect.left, pathRect.top, pathRect.right, pathRect.bottom, 6, 6);
            DeleteObject(boxBg); DeleteObject(boxPen);

            RECT pathTextRect = {40, 145, rect.right - 40, 165};
            DrawTextW(memDC, g_InstallDir, -1, &pathTextRect, DT_SINGLELINE | DT_VCENTER);

        } else if (g_CurrentStep == STEP_OPTIONS) {
            SelectObject(memDC, hFontBodyBold);
            TextOutW(memDC, 30, 95, (g_SetupLang == SETUP_TR) ? L"Kurulum Seçenekleri:" : L"Installation Options:", 22);
            SelectObject(memDC, hFontBody);

            const wchar_t* txt1 = (g_SetupLang == SETUP_TR) ? L"Masaüstü Kısayolu Oluştur" : L"Create Desktop Shortcut";
            const wchar_t* txt2 = (g_SetupLang == SETUP_TR) ? L"Windows Başlangıcında Otomatik Çalıştır" : L"Start Automatically with Windows";

            TextOutW(memDC, 65, 140, txt1, (int)wcslen(txt1));
            TextOutW(memDC, 65, 185, txt2, (int)wcslen(txt2));

            // Draw Checkboxes
            RECT chk1 = {35, 138, 55, 158};
            RECT chk2 = {35, 183, 55, 203};

            DrawStyledButton(memDC, chk1, g_CreateDesktopShortcut ? L"✓" : L"", false, g_CreateDesktopShortcut);
            DrawStyledButton(memDC, chk2, g_StartWithWindows ? L"✓" : L"", false, g_StartWithWindows);

        } else if (g_CurrentStep == STEP_INSTALLING) {
            const wchar_t* txtProg = (g_SetupLang == SETUP_TR) ? L"Dosyalar kopyalanıyor ve kayıtlar yapılıyor..." : L"Copying files and writing registry...";
            TextOutW(memDC, 30, 120, txtProg, (int)wcslen(txtProg));

            RECT progRect = {30, 160, rect.right - 30, 185};
            HBRUSH bgProg = CreateSolidBrush(RGB(230, 235, 245));
            FillRect(memDC, &progRect, bgProg);
            DeleteObject(bgProg);

            RECT fillProg = {30, 160, rect.right - 30, 185};
            HBRUSH fillBrush = CreateSolidBrush(RGB(0, 173, 181));
            FillRect(memDC, &fillProg, fillBrush);
            DeleteObject(fillBrush);

        } else if (g_CurrentStep == STEP_FINISH) {
            SelectObject(memDC, hFontBodyBold);
            SetTextColor(memDC, RGB(0, 173, 181));
            const wchar_t* txtDone = (g_SetupLang == SETUP_TR) ? L"Kurulum Başarıyla Tamamlandı!" : L"Installation Completed Successfully!";
            TextOutW(memDC, 30, 100, txtDone, (int)wcslen(txtDone));
            SetTextColor(memDC, RGB(40, 45, 55));
            SelectObject(memDC, hFontBody);

            const wchar_t* txtRun = (g_SetupLang == SETUP_TR) ? L"ETDTimer uygulamasını şimdi başlat" : L"Launch ETDTimer application now";
            TextOutW(memDC, 65, 155, txtRun, (int)wcslen(txtRun));

            RECT chkFinish = {35, 153, 55, 173};
            DrawStyledButton(memDC, chkFinish, g_LaunchAfterSetup ? L"✓" : L"", false, g_LaunchAfterSetup);
        }

        SelectObject(memDC, oldFont);
        DeleteObject(hFontBody);
        DeleteObject(hFontBodyBold);

        // Footer Bar & Action Buttons
        HBRUSH footerBrush = CreateSolidBrush(RGB(245, 247, 250));
        HPEN footerPen = CreatePen(PS_SOLID, 1, RGB(225, 230, 238));
        SelectObject(memDC, footerBrush); SelectObject(memDC, footerPen);
        Rectangle(memDC, -1, rect.bottom - 60, rect.right + 1, rect.bottom + 1);
        DeleteObject(footerBrush); DeleteObject(footerPen);

        // Navigation Buttons
        RECT btnLeftRect = {20, rect.bottom - 45, 120, rect.bottom - 15};
        RECT btnRightRect = {380, rect.bottom - 45, 490, rect.bottom - 15};

        if (g_CurrentStep == STEP_LANG_SELECT) {
            DrawStyledButton(memDC, btnLeftRect, (g_SetupLang == SETUP_TR) ? L"İptal" : L"Cancel", g_HoverButton == 10, false);
            DrawStyledButton(memDC, btnRightRect, (g_SetupLang == SETUP_TR) ? L"İleri >" : L"Next >", g_HoverButton == 11, true);
        } else if (g_CurrentStep == STEP_LICENSE) {
            DrawStyledButton(memDC, btnLeftRect, (g_SetupLang == SETUP_TR) ? L"< Geri" : L"< Back", g_HoverButton == 10, false);
            DrawStyledButton(memDC, btnRightRect, (g_SetupLang == SETUP_TR) ? L"Kabul Et >" : L"Accept >", g_HoverButton == 11, true);
        } else if (g_CurrentStep == STEP_FOLDER) {
            DrawStyledButton(memDC, btnLeftRect, (g_SetupLang == SETUP_TR) ? L"< Geri" : L"< Back", g_HoverButton == 10, false);
            DrawStyledButton(memDC, btnRightRect, (g_SetupLang == SETUP_TR) ? L"İleri >" : L"Next >", g_HoverButton == 11, true);
        } else if (g_CurrentStep == STEP_OPTIONS) {
            DrawStyledButton(memDC, btnLeftRect, (g_SetupLang == SETUP_TR) ? L"< Geri" : L"< Back", g_HoverButton == 10, false);
            DrawStyledButton(memDC, btnRightRect, (g_SetupLang == SETUP_TR) ? L"Kur" : L"Install", g_HoverButton == 11, true);
        } else if (g_CurrentStep == STEP_FINISH) {
            DrawStyledButton(memDC, btnRightRect, (g_SetupLang == SETUP_TR) ? L"Bitir" : L"Finish", g_HoverButton == 11, true);
        }

        BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        // Language selection card click
        if (g_CurrentStep == STEP_LANG_SELECT) {
            if (y >= 140 && y <= 210) {
                if (x >= 40 && x <= 240) {
                    g_SetupLang = SETUP_TR;
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                } else if (x >= 270 && x <= 470) {
                    g_SetupLang = SETUP_EN;
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                }
            }
        }

        // Options Checkbox clicks
        if (g_CurrentStep == STEP_OPTIONS) {
            if (y >= 135 && y <= 160 && x >= 35 && x <= 280) {
                g_CreateDesktopShortcut = !g_CreateDesktopShortcut;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            if (y >= 180 && y <= 205 && x >= 35 && x <= 350) {
                g_StartWithWindows = !g_StartWithWindows;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
        }

        // Finish Checkbox click
        if (g_CurrentStep == STEP_FINISH) {
            if (y >= 150 && y <= 175 && x >= 35 && x <= 320) {
                g_LaunchAfterSetup = !g_LaunchAfterSetup;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
        }

        // Navigation Footer Button clicks (Y inside footer button bounds)
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        int footerTop = clientRect.bottom - 50;
        int footerBottom = clientRect.bottom - 10;

        if (y >= footerTop && y <= footerBottom) {
            // Left Button (Back / Cancel)
            if (x >= 20 && x <= 120) {
                if (g_CurrentStep == STEP_LANG_SELECT) {
                    PostQuitMessage(0);
                } else if (g_CurrentStep == STEP_LICENSE) {
                    g_CurrentStep = STEP_LANG_SELECT;
                } else if (g_CurrentStep == STEP_FOLDER) {
                    g_CurrentStep = STEP_LICENSE;
                } else if (g_CurrentStep == STEP_OPTIONS) {
                    g_CurrentStep = STEP_FOLDER;
                }
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            // Right Button (Next / Accept / Install / Finish)
            if (x >= 380 && x <= 490) {
                if (g_CurrentStep == STEP_LANG_SELECT) {
                    g_CurrentStep = STEP_LICENSE;
                } else if (g_CurrentStep == STEP_LICENSE) {
                    g_CurrentStep = STEP_FOLDER;
                } else if (g_CurrentStep == STEP_FOLDER) {
                    g_CurrentStep = STEP_OPTIONS;
                } else if (g_CurrentStep == STEP_OPTIONS) {
                    g_CurrentStep = STEP_INSTALLING;
                    InvalidateRect(hwnd, NULL, FALSE);
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
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
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
    wchar_t currentExe[MAX_PATH];
    GetModuleFileNameW(NULL, currentExe, MAX_PATH);
    std::wstring exeName = currentExe;
    size_t pos = exeName.find_last_of(L"\\/");
    if (pos != std::wstring::npos) exeName = exeName.substr(pos + 1);
    for (auto& c : exeName) c = towlower(c);

    bool isUninstallMode = (wcsstr(pCmdLine, L"/uninstall") != NULL ||
                            wcsstr(pCmdLine, L"-uninstall") != NULL ||
                            exeName.find(L"uninstall") != std::wstring::npos);

    if (isUninstallMode) {
        ExecuteUninstallation();
        return 0;
    }

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = SetupWindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszClassName = L"ETDTimerSetupClass";

    RegisterClassExW(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int width = 520, height = 410;

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
