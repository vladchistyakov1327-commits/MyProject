SYSTEM CLEANER PRO v1.0
========================

 СТРУКТУРА ПРОЕКТА:
- Main.cpp/h - главный файл и определения
- AdminChecker.cpp/h - проверка прав администратора
- DiagnosticsModule.cpp/h - диагностика системы
- CleanerModule.cpp/h - очистка системы
- ProgramsModule.cpp/h - управление программами
- ServicesModule.cpp/h - управление службами
- NetworkModule.cpp/h - сетевые настройки
- SecurityModule.cpp/h - безопасность
- SchedulerModule.cpp/h - планировщик задач
- RegistryUtils.cpp/h - работа с реестром
- Logger.cpp/h - логирование
- Resource.h - определения ресурсов
- SystemCleanerPro.rc - файл ресурсов

 КОМПИЛЯЦИЯ:

Вариант 1: Visual Studio 2022
- Откройте SystemCleanerPro.sln
- Выберите Release/x64
- Build  Build Solution

Вариант 2: Командная строка
- Запустите "Developer Command Prompt for VS 2022"
- Перейдите в папку проекта
- Запустите compile.bat

Вариант 3: CMake
- cmake .
- cmake --build . --config Release

 ПРИМЕЧАНИЯ:
- Программа требует прав администратора
- Совместимость: Windows 10/11 (64-bit)
- Все операции логируются
- Создаются точки восстановления

 ПОДДЕРЖКА:
При проблемах с компиляцией проверьте:
1. Все ли файлы на месте
2. Установлен ли Windows SDK
3. Запущен ли скрипт fix_systemcleaner.ps1
