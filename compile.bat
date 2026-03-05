@echo off
echo Compiling SystemCleanerPro...
echo.

REM Проверка наличия MSVC
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo MSVC compiler not found!
    echo Please open "Developer Command Prompt for VS 2022"
    pause
    exit /b 1
)

REM Компиляция
cl /EHsc /FeSystemCleanerPro.exe /I. ^
    Main.cpp AdminChecker.cpp DiagnosticsModule.cpp ^
    CleanerModule.cpp ProgramsModule.cpp ServicesModule.cpp ^
    NetworkModule.cpp SecurityModule.cpp SchedulerModule.cpp ^
    RegistryUtils.cpp Logger.cpp ^
    /link user32.lib kernel32.lib gdi32.lib comctl32.lib ^
    shell32.lib advapi32.lib iphlpapi.lib ws2_32.lib ^
    crypt32.lib taskschd.lib wininet.lib ^
    /SUBSYSTEM:WINDOWS

if %errorlevel% equ 0 (
    echo.
    echo  Compilation successful!
    echo File: SystemCleanerPro.exe
) else (
    echo.
    echo  Compilation failed!
)

pause
