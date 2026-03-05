#include "SchedulerModule.h"
#include "Logger.h"
#include <taskschd.h>

#pragma comment(lib, "taskschd.lib")

bool SchedulerModule::CreateCleanupTask(const std::wstring& taskName, int frequency, const std::vector<int>& operations) {
    Logger::Log(L"Создание задания в планировщике: " + taskName, LOG_INFO);
    
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) return false;
    
    ITaskService* pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
                          IID_ITaskService, (void**)&pService);
    
    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }
    
    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    
    if (SUCCEEDED(hr)) {
        ITaskFolder* pRootFolder = NULL;
        hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
        
        if (SUCCEEDED(hr)) {
            // Удаляем существующее задание если есть
            pRootFolder->DeleteTask(_bstr_t(taskName.c_str()), 0);
            
            ITaskDefinition* pTask = NULL;
            hr = pService->NewTask(0, &pTask);
            
            if (SUCCEEDED(hr)) {
                // Регистрационная информация
                IRegistrationInfo* pRegInfo = NULL;
                hr = pTask->get_RegistrationInfo(&pRegInfo);
                if (SUCCEEDED(hr)) {
                    pRegInfo->put_Author(_bstr_t(L"System Cleaner Pro"));
                    pRegInfo->put_Description(_bstr_t(L"Автоматическая очистка системы"));
                    pRegInfo->Release();
                }
                
                // Принципал (запуск с правами администратора)
                IPrincipal* pPrincipal = NULL;
                hr = pTask->get_Principal(&pPrincipal);
                if (SUCCEEDED(hr)) {
                    pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
                    pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
                    pPrincipal->Release();
                }
                
                // Настройки
                ITaskSettings* pSettings = NULL;
                hr = pTask->get_Settings(&pSettings);
                if (SUCCEEDED(hr)) {
                    pSettings->put_AllowDemandStart(VARIANT_TRUE);
                    pSettings->put_StartWhenAvailable(VARIANT_TRUE);
                    pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
                    pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
                    pSettings->put_ExecutionTimeLimit(_bstr_t(L"PT2H")); // 2 часа
                    pSettings->put_Priority(7); // Нормальный приоритет
                    pSettings->Release();
                }
                
                // Триггер (расписание)
                ITriggerCollection* pTriggers = NULL;
                hr = pTask->get_Triggers(&pTriggers);
                if (SUCCEEDED(hr)) {
                    ITrigger* pTrigger = NULL;
                    hr = pTriggers->Create(TASK_TRIGGER_TIME, &pTrigger);
                    
                    if (SUCCEEDED(hr)) {
                        ITimeTrigger* pTimeTrigger = NULL;
                        hr = pTrigger->QueryInterface(IID_ITimeTrigger, (void**)&pTimeTrigger);
                        
                        if (SUCCEEDED(hr)) {
                            // Установка времени запуска
                            SYSTEMTIME st;
                            GetLocalTime(&st);
                            
                            if (frequency == 1) { // Ежедневно
                                st.wHour = 3; // 3 часа ночи
                                st.wMinute = 0;
                                st.wSecond = 0;
                            } else if (frequency == 2) { // Еженедельно
                                // Воскресенье в 3 часа
                            } else { // Ежемесячно
                                // 1 число каждого месяца
                            }
                            
                            FILETIME ft;
                            SystemTimeToFileTime(&st, &ft);
                            
                            pTimeTrigger->put_StartBoundary(_bstr_t(GetTaskTimeString(st).c_str()));
                            pTimeTrigger->Release();
                        }
                        pTrigger->Release();
                    }
                    pTriggers->Release();
                }
                
                // Действие (запуск программы)
                IActionCollection* pActions = NULL;
                hr = pTask->get_Actions(&pActions);
                if (SUCCEEDED(hr)) {
                    IAction* pAction = NULL;
                    hr = pActions->Create(TASK_ACTION_EXEC, &pAction);
                    
                    if (SUCCEEDED(hr)) {
                        IExecAction* pExecAction = NULL;
                        hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
                        
                        if (SUCCEEDED(hr)) {
                            wchar_t exePath[MAX_PATH];
                            GetModuleFileName(NULL, exePath, MAX_PATH);
                            
                            pExecAction->put_Path(_bstr_t(exePath));
                            pExecAction->put_Arguments(_bstr_t(L"-silent -cleanup"));
                            pExecAction->Release();
                        }
                        pAction->Release();
                    }
                    pActions->Release();
                }
                
                // Регистрация задания
                IRegisteredTask* pRegisteredTask = NULL;
                hr = pRootFolder->RegisterTaskDefinition(_bstr_t(taskName.c_str()),
                                                          pTask,
                                                          TASK_CREATE_OR_UPDATE,
                                                          _variant_t(),
                                                          _variant_t(),
                                                          TASK_LOGON_INTERACTIVE_TOKEN,
                                                          _variant_t(),
                                                          &pRegisteredTask);
                
                if (SUCCEEDED(hr)) {
                    pRegisteredTask->Release();
                    Logger::Log(L"Задание создано: " + taskName, LOG_SUCCESS);
                }
                
                pTask->Release();
            }
            
            pRootFolder->Release();
        }
        
        pService->Release();
    }
    
    CoUninitialize();
    
    return SUCCEEDED(hr);
}

std::wstring SchedulerModule::GetTaskTimeString(const SYSTEMTIME& st) {
    wchar_t buffer[64];
    swprintf(buffer, 64, L"%04d-%02d-%02dT%02d:%02d:%02d",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return buffer;
}

std::vector<ScheduledTask> SchedulerModule::GetScheduledTasks() {
    std::vector<ScheduledTask> tasks;
    
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) return tasks;
    
    ITaskService* pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
                          IID_ITaskService, (void**)&pService);
    
    if (SUCCEEDED(hr)) {
        hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        
        if (SUCCEEDED(hr)) {
            ITaskFolder* pRootFolder = NULL;
            hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
            
            if (SUCCEEDED(hr)) {
                IRegisteredTaskCollection* pTaskCollection = NULL;
                hr = pRootFolder->GetTasks(TASK_ENUM_HIDDEN, &pTaskCollection);
                
                if (SUCCEEDED(hr)) {
                    LONG taskCount = 0;
                    pTaskCollection->get_Count(&taskCount);
                    
                    for (LONG i = 0; i < taskCount; i++) {
                        IRegisteredTask* pTask = NULL;
                        hr = pTaskCollection->get_Item(_variant_t(i + 1), &pTask);
                        
                        if (SUCCEEDED(hr)) {
                            ScheduledTask task;
                            
                            BSTR taskName;
                            pTask->get_Name(&taskName);
                            task.name = taskName;
                            SysFreeString(taskName);
                            
                            ITaskDefinition* pDef = NULL;
                            pTask->get_Definition(&pDef);
                            
                            if (pDef) {
                                IRegistrationInfo* pReg = NULL;
                                pDef->get_RegistrationInfo(&pReg);
                                
                                if (pReg) {
                                    BSTR desc;
                                    pReg->get_Description(&desc);
                                    if (desc) {
                                        task.description = desc;
                                        SysFreeString(desc);
                                    }
                                    pReg->Release();
                                }
                                
                                pDef->Release();
                            }
                            
                            TASK_STATE state;
                            pTask->get_State(&state);
                            
                            switch (state) {
                                case TASK_STATE_READY: task.state = L"Готово"; break;
                                case TASK_STATE_RUNNING: task.state = L"Выполняется"; break;
                                case TASK_STATE_DISABLED: task.state = L"Отключено"; break;
                                default: task.state = L"Неизвестно";
                            }
                            
                            tasks.push_back(task);
                            pTask->Release();
                        }
                    }
                    
                    pTaskCollection->Release();
                }
                
                pRootFolder->Release();
            }
        }
        
        pService->Release();
    }
    
    CoUninitialize();
    return tasks;
}

bool SchedulerModule::DeleteTask(const std::wstring& taskName) {
    Logger::Log(L"Удаление задания: " + taskName, LOG_INFO);
    
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) return false;
    
    ITaskService* pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
                          IID_ITaskService, (void**)&pService);
    
    if (SUCCEEDED(hr)) {
        hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        
        if (SUCCEEDED(hr)) {
            ITaskFolder* pRootFolder = NULL;
            hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
            
            if (SUCCEEDED(hr)) {
                hr = pRootFolder->DeleteTask(_bstr_t(taskName.c_str()), 0);
                pRootFolder->Release();
            }
        }
        
        pService->Release();
    }
    
    CoUninitialize();
    
    if (SUCCEEDED(hr)) {
        Logger::Log(L"Задание удалено: " + taskName, LOG_SUCCESS);
    } else {
        Logger::Log(L"Ошибка удаления задания: " + taskName, LOG_ERROR);
    }
    
    return SUCCEEDED(hr);
}