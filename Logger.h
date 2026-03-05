#pragma once
#include <fstream>
#include <mutex>
#include <ctime>
#include <iomanip>

enum LogLevel {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_SUCCESS
};

class Logger {
private:
    static std::wofstream logFile;
    static std::mutex logMutex;
    static bool initialized;
    
public:
    static void Initialize(const std::wstring& filename) {
        std::lock_guard<std::mutex> lock(logMutex);
        logFile.open(filename, std::ios::out | std::ios::app);
        initialized = logFile.is_open();
        
        if (initialized) {
            logFile.imbue(std::locale(""));
            Log(L"Логирование инициализировано", LOG_INFO);
        }
    }
    
    static void Log(const std::wstring& message, LogLevel level) {
        std::lock_guard<std::mutex> lock(logMutex);
        
        if (!initialized) return;
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        
        std::wstring levelStr;
        switch (level) {
            case LOG_INFO: levelStr = L"INFO"; break;
            case LOG_WARNING: levelStr = L"WARNING"; break;
            case LOG_ERROR: levelStr = L"ERROR"; break;
            case LOG_SUCCESS: levelStr = L"SUCCESS"; break;
        }
        
        logFile << std::put_time(std::localtime(&time), L"%Y-%m-%d %H:%M:%S") 
                << L" [" << levelStr << L"] " << message << std::endl;
        logFile.flush();
    }
    
    static void Close() {
        std::lock_guard<std::mutex> lock(logMutex);
        if (initialized) {
            logFile.close();
            initialized = false;
        }
    }
};

// Определение статических членов
std::wofstream Logger::logFile;
std::mutex Logger::logMutex;
bool Logger::initialized = false;

// Вспомогательные функции
inline std::wstring GetCurrentDateTime() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::wstringstream ss;
    ss << std::put_time(std::localtime(&time), L"%Y%m%d_%H%M%S");
    return ss.str();
}

inline std::wstring GetCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::wstringstream ss;
    ss << std::put_time(std::localtime(&time), L"%H:%M:%S");
    return ss.str();
}

inline std::wstring FormatBytes(DWORD64 bytes) {
    const wchar_t* suffixes[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    int suffix = 0;
    double size = (double)bytes;
    
    while (size >= 1024 && suffix < 4) {
        size /= 1024;
        suffix++;
    }
    
    wchar_t buffer[64];
    swprintf(buffer, 64, L"%.1f %s", size, suffixes[suffix]);
    return buffer;
}