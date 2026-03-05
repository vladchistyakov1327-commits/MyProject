#pragma once
#include "Main.h"
#include <vector>
#include <string>

class ProgramsModule {
public:
    static std::vector<ProgramInfo> GetInstalledPrograms();
    static bool IsConflictProgram(const std::wstring& programName);
    static bool UninstallProgram(const ProgramInfo& program);
    static void AnalyzeConflicts();
    static std::vector<ProgramInfo> FindDuplicates();
};
