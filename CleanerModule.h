#pragma once
#include "Main.h"
#include <string>

class CleanerModule {
public:
    static bool CleanTempFiles();
    static bool CleanBrowserCache(const std::wstring& browser);
    static bool CleanEventLogs();
    static bool CleanWindowsUpdateCache();
    static bool CleanPrefetch();
    static bool FlushDNS();
    static bool EmptyRecycleBin();
    static bool CleanThumbnailCache();
    static bool CleanCBSLogs();
    static bool RebuildIconCache();
    static bool CleanWinSxS();
    static bool CleanProjectTempFiles();
    
private:
    static bool DeleteDirectoryContents(const wchar_t* path);
};
