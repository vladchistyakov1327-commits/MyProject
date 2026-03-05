#include "CleanerModule.h"
#include "Logger.h"
#include <shlobj.h>

bool CleanerModule::CleanTempFiles() {
    Logger::Log(L"Очистка временных файлов", LOG_INFO);
    
    bool success = true;
    
    // Windows Temp
    wchar_t tempPath[MAX_PATH];
    GetTempPath(MAX_PATH, tempPath);
    if (!DeleteDirectoryContents(tempPath)) success = false;
    
    // User Temp
    wchar_t userTempPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, userTempPath))) {
        std::wstring path = std::wstring(userTempPath) + L"\\Temp";
        if (!DeleteDirectoryContents(path.c_str())) success = false;
    }
    
    // Windows Temp (системная)
    if (!DeleteDirectoryContents(L"C:\\Windows\\Temp")) success = false;
    
    // Internet Explorer Cache (старые версии)
    wchar_t ieCachePath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_INTERNET_CACHE, NULL, 0, ieCachePath))) {
        DeleteDirectoryContents(ieCachePath);
    }
    
    // Recent Documents
    wchar_t recentPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_RECENT, NULL, 0, recentPath))) {
        DeleteDirectoryContents(recentPath);
    }
    
    return success;
}

bool CleanerModule::CleanBrowserCache(const std::wstring& browser) {
    Logger::Log(L"Очистка кэша браузера: " + browser, LOG_INFO);
    
    wchar_t appDataPath[MAX_PATH];
    wchar_t localAppDataPath[MAX_PATH];
    
    SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, appDataPath);
    SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppDataPath);
    
    std::wstring cachePath;
    std::wstring cookiesPath;
    
    if (browser == L"Chrome") {
        cachePath = std::wstring(localAppDataPath) + L"\\Google\\Chrome\\User Data\\Default\\Cache";
        cookiesPath = std::wstring(localAppDataPath) + L"\\Google\\Chrome\\User Data\\Default\\Cookies";
    } else if (browser == L"Edge") {
        cachePath = std::wstring(localAppDataPath) + L"\\Microsoft\\Edge\\User Data\\Default\\Cache";
        cookiesPath = std::wstring(localAppDataPath) + L"\\Microsoft\\Edge\\User Data\\Default\\Cookies";
    } else if (browser == L"Firefox") {
        // Поиск профиля Firefox
        std::wstring profilesPath = std::wstring(appDataPath) + L"\\Mozilla\\Firefox\\Profiles";
        WIN32_FIND_DATA findData;
        HANDLE hFind = FindFirstFile((profilesPath + L"\\*").c_str(), &findData);
        
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
                    wcscmp(findData.cFileName, L".") != 0 &&
                    wcscmp(findData.cFileName, L"..") != 0) {
                    
                    std::wstring profilePath = profilesPath + L"\\" + findData.cFileName;
                    
                    // Кэш Firefox
                    DeleteDirectoryContents((profilePath + L"\\cache2").c_str());
                    DeleteDirectoryContents((profilePath + L"\\thumbnails").c_str());
                    
                    // Cookies
                    std::wstring cookiesFile = profilePath + L"\\cookies.sqlite";
                    DeleteFile(cookiesFile.c_str());
                }
            } while (FindNextFile(hFind, &findData));
            FindClose(hFind);
        }
    } else if (browser == L"Opera") {
        cachePath = std::wstring(localAppDataPath) + L"\\Opera Software\\Opera Stable\\Cache";
    } else if (browser == L"Yandex") {
        cachePath = std::wstring(localAppDataPath) + L"\\Yandex\\YandexBrowser\\User Data\\Default\\Cache";
    }
    
    if (!cachePath.empty()) {
        DeleteDirectoryContents(cachePath.c_str());
    }
    
    if (!cookiesPath.empty()) {
        DeleteFile(cookiesPath.c_str());
    }
    
    return true;
}

bool CleanerModule::CleanEventLogs() {
    Logger::Log(L"Очистка системных логов", LOG_INFO);
    
    const wchar_t* logs[] = {
        L"Application",
        L"System",
        L"Security",
        L"Setup",
        L"ForwardedEvents",
        L"Microsoft-Windows-PowerShell/Operational",
        L"Windows PowerShell"
    };
    
    for (const wchar_t* log : logs) {
        std::wstring command = L"wevtutil cl " + std::wstring(log) + L" 2>nul";
        _wsystem(command.c_str());
    }
    
    return true;
}

bool CleanerModule::CleanWindowsUpdateCache() {
    Logger::Log(L"Очистка кэша обновлений Windows", LOG_INFO);
    
    // Остановка служб обновлений
    _wsystem(L"net stop wuauserv /y 2>nul");
    _wsystem(L"net stop bits /y 2>nul");
    _wsystem(L"net stop cryptsvc /y 2>nul");
    
    // Удаление содержимого SoftwareDistribution
    DeleteDirectoryContents(L"C:\\Windows\\SoftwareDistribution\\Download");
    DeleteDirectoryContents(L"C:\\Windows\\SoftwareDistribution\\DataStore");
    
    // Удаление каталога CatRoot2
    DeleteDirectoryContents(L"C:\\Windows\\System32\\catroot2");
    
    // Запуск служб обратно
    _wsystem(L"net start cryptsvc");
    _wsystem(L"net start bits");
    _wsystem(L"net start wuauserv");
    
    return true;
}

bool CleanerModule::CleanPrefetch() {
    Logger::Log(L"Очистка Prefetch", LOG_INFO);
    
    wchar_t windowsPath[MAX_PATH];
    GetWindowsDirectory(windowsPath, MAX_PATH);
    std::wstring prefetchPath = std::wstring(windowsPath) + L"\\Prefetch";
    
    // Удаление всех .pf файлов
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile((prefetchPath + L"\\*.pf").c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::wstring filePath = prefetchPath + L"\\" + findData.cFileName;
            DeleteFile(filePath.c_str());
        } while (FindNextFile(hFind, &findData));
        FindClose(hFind);
    }
    
    return true;
}

bool CleanerModule::FlushDNS() {
    Logger::Log(L"Сброс DNS кэша", LOG_INFO);
    
    _wsystem(L"ipconfig /flushdns");
    return true;
}

bool CleanerModule::EmptyRecycleBin() {
    Logger::Log(L"Очистка корзины", LOG_INFO);
    
    SHEmptyRecycleBin(NULL, NULL, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
    return true;
}

bool CleanerModule::CleanThumbnailCache() {
    Logger::Log(L"Очистка кэша превью", LOG_INFO);
    
    wchar_t localAppDataPath[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppDataPath);
    
    std::wstring thumbCachePath = std::wstring(localAppDataPath) + L"\\Microsoft\\Windows\\Explorer";
    
    // Остановка проводника
    _wsystem(L"taskkill /f /im explorer.exe 2>nul");
    
    // Удаление thumbcache
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile((thumbCachePath + L"\\thumbcache_*.db").c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::wstring filePath = thumbCachePath + L"\\" + findData.cFileName;
            DeleteFile(filePath.c_str());
        } while (FindNextFile(hFind, &findData));
        FindClose(hFind);
    }
    
    // Удаление iconcache
    std::wstring iconCachePath = std::wstring(localAppDataPath) + L"\\IconCache.db";
    DeleteFile(iconCachePath.c_str());
    
    // Запуск проводника
    _wsystem(L"start explorer.exe");
    
    return true;
}

bool CleanerModule::CleanCBSLogs() {
    Logger::Log(L"Очистка логов CBS", LOG_INFO);
    
    DeleteDirectoryContents(L"C:\\Windows\\Logs\\CBS");
    DeleteDirectoryContents(L"C:\\Windows\\Logs\\DISM");
    
    // Удаление старых логов CBS
    wchar_t windowsPath[MAX_PATH];
    GetWindowsDirectory(windowsPath, MAX_PATH);
    
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile((std::wstring(windowsPath) + L"\\Logs\\CBS\\*.cab").c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::wstring filePath = std::wstring(windowsPath) + L"\\Logs\\CBS\\" + findData.cFileName;
            DeleteFile(filePath.c_str());
        } while (FindNextFile(hFind, &findData));
        FindClose(hFind);
    }
    
    return true;
}

bool CleanerModule::RebuildIconCache() {
    Logger::Log(L"Перестроение кэша иконок", LOG_INFO);
    
    wchar_t localAppDataPath[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppDataPath);
    
    std::wstring iconCachePath = std::wstring(localAppDataPath) + L"\\IconCache.db";
    
    // Остановка проводника
    _wsystem(L"taskkill /f /im explorer.exe 2>nul");
    
    // Удаление кэша иконок
    DeleteFile(iconCachePath.c_str());
    
    // Удаление всех thumbcache
    std::wstring thumbCachePath = std::wstring(localAppDataPath) + L"\\Microsoft\\Windows\\Explorer";
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile((thumbCachePath + L"\\thumbcache_*.db").c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::wstring filePath = thumbCachePath + L"\\" + findData.cFileName;
            DeleteFile(filePath.c_str());
        } while (FindNextFile(hFind, &findData));
        FindClose(hFind);
    }
    
    // Запуск проводника
    _wsystem(L"start explorer.exe");
    
    return true;
}

bool CleanerModule::CleanWinSxS() {
    Logger::Log(L"Очистка WinSxS", LOG_INFO);
    
    // Анализ размера WinSxS
    _wsystem(L"DISM /Online /Cleanup-Image /AnalyzeComponentStore");
    
    // Очистка компонентов
    _wsystem(L"DISM /Online /Cleanup-Image /StartComponentCleanup /ResetBase");
    
    // Дополнительная очистка старых версий
    _wsystem(L"DISM /Online /Cleanup-Image /SPSuperseded");
    
    return true;
}

bool CleanerModule::CleanProjectTempFiles() {
    Logger::Log(L"Очистка временных файлов проектов", LOG_INFO);
    
    wchar_t userTempPath[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, userTempPath);
    
    // SolidWorks
    std::wstring swPath = std::wstring(userTempPath) + L"\\Temp\\SW";
    DeleteDirectoryContents(swPath.c_str());
    
    // AutoCAD
    std::wstring acadPath = std::wstring(userTempPath) + L"\\Temp\\acad*";
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile(acadPath.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                std::wstring folderPath = std::wstring(userTempPath) + L"\\Temp\\" + findData.cFileName;
                DeleteDirectoryContents(folderPath.c_str());
                RemoveDirectory(folderPath.c_str());
            }
        } while (FindNextFile(hFind, &findData));
        FindClose(hFind);
    }
    
    // КОМПАС-3D
    std::wstring kompasPath = std::wstring(userTempPath) + L"\\Temp\\kompas*";
    hFind = FindFirstFile(kompasPath.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                std::wstring folderPath = std::wstring(userTempPath) + L"\\Temp\\" + findData.cFileName;
                DeleteDirectoryContents(folderPath.c_str());
                RemoveDirectory(folderPath.c_str());
            }
        } while (FindNextFile(hFind, &findData));
        FindClose(hFind);
    }
    
    // Adobe временные файлы
    std::wstring adobePath = std::wstring(userTempPath) + L"\\Adobe";
    DeleteDirectoryContents(adobePath.c_str());
    
    return true;
}

bool CleanerModule::DeleteDirectoryContents(const wchar_t* path) {
    std::wstring searchPath = std::wstring(path) + L"\\*";
    
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    bool success = true;
    
    do {
        if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
            std::wstring fullPath = std::wstring(path) + L"\\" + findData.cFileName;
            
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                DeleteDirectoryContents(fullPath.c_str());
                if (!RemoveDirectory(fullPath.c_str())) {
                    success = false;
                }
            } else {
                // Сброс атрибутов для удаления
                SetFileAttributes(fullPath.c_str(), FILE_ATTRIBUTE_NORMAL);
                if (!DeleteFile(fullPath.c_str())) {
                    success = false;
                }
            }
        }
    } while (FindNextFile(hFind, &findData));
    
    FindClose(hFind);
    return success;
}