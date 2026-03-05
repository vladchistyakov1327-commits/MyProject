#include "Logger.h"

// Определение статических членов
std::wofstream Logger::logFile;
std::mutex Logger::logMutex;
bool Logger::initialized = false;
