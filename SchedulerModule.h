#pragma once
#include <string>
#include <vector>

struct ScheduledTask {
    std::wstring name;
    std::wstring description;
    std::wstring state;
};

class SchedulerModule {
public:
    static bool CreateCleanupTask(const std::wstring& taskName, int frequency, const std::vector<int>& operations);
    static std::wstring GetTaskTimeString(const SYSTEMTIME& st);
    static std::vector<ScheduledTask> GetScheduledTasks();
    static bool DeleteTask(const std::wstring& taskName);
};
