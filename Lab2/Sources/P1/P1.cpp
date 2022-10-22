#include "Utils.h"

#include <string>
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <cstring>

bool create_directory(const char* path)
{
    if (!CreateDirectory(path, NULL)) {
        CHECK(!(GetLastError() == ERROR_PATH_NOT_FOUND), false, "[Create Directory] Failed to find `%s` path!", path);
        WARNING(GetLastError() == ERROR_ALREADY_EXISTS, "[Create Directory] Directory `%s` already exists.", path);
    }
    return true;
}

bool create_file(const char* path, const char* data, int input_size) {
    HANDLE h = CreateFile(path, GENERIC_READ | GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    CHECK(h != INVALID_HANDLE_VALUE, false, "[Create File] Failed to create a file. Error: %d", GetLastError());

    DWORD bytesWritten;

    CHECK(WriteFile(h, data, input_size, &bytesWritten, NULL), false, "[Create File] Failed to write to file!. Error: %d", GetLastError());
    CloseHandle(h);
    return true;
}

bool enable_privilege(const char* attribute)
{
    HANDLE tokenHandle;
    CHECK(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &tokenHandle), false, "[EnablePrivilege] Failed to obtain token. Error: %d", GetLastError());


    LUID privilege;
    CHECK(LookupPrivilegeValue(NULL, attribute, &privilege), false, "[EnablePrivilege] Failed to aquire LUID of privilege. Error: %d", GetLastError());

    TOKEN_PRIVILEGES tokenData;

    tokenData.PrivilegeCount = 1;
    tokenData.Privileges[0] = { privilege, SE_PRIVILEGE_ENABLED };

    CHECK(AdjustTokenPrivileges(tokenHandle, FALSE, &tokenData, sizeof(TOKEN_PRIVILEGES), NULL, NULL), false, "[EnablePrivilege] Failed to ajust token. Error: %d", GetLastError());

    CHECK(GetLastError() == ERROR_NOT_ALL_ASSIGNED, false, "[EnablePrivilege] The token doesn't have the specified privileges.");

    return true;
}

bool enumerate(int flag, int pid, Counters& counter, void* (*callback)(HANDLE, Counters&))
{
    HANDLE h;

    CHECK((h = CreateToolhelp32Snapshot(flag, pid)) != INVALID_HANDLE_VALUE, false, "[Enumerate] Failed to obtain handler. Error: %d", GetLastError());

    WARNING((bool)((*callback)(h, counter)) != true, "[Enumerate] Callback function failed");
    CloseHandle(h);
    return true;
}

void* module_handler(HANDLE h, Counters& counter)
{
    MODULEENTRY32 module;

    module.dwSize = sizeof(MODULEENTRY32);

    CHECK(Module32First(h, &module), (void*)false, "[ModuleHandler] Failed to get first module. Error: %d", GetLastError());
    std::string module_data("");
    do {
        ++counter.module;
        char* data = new char[250];

        sprintf_s(data, 250, "Module ID: %d | Parent PID: %d | szModule: %s | SzExePath: %s\n", module.th32ModuleID, module.th32ProcessID, module.szModule, module.szExePath);

        module_data += std::string(data);
        delete data;
    } while (Module32Next(h, &module));

    CHECK(create_file("C:\\Facultate\\CSSO\\Laboratoare\\Week2\\module_process.txt", module_data.c_str(), module_data.length()), (void*)false, "[ModuleHandler] Failed to create file.");

    return (void*)true;
}

void* thread_handler(HANDLE h, Counters& counter)
{
    THREADENTRY32 thread;

    thread.dwSize = sizeof(THREADENTRY32);

    CHECK(Thread32First(h, &thread), (void*)false, "[ThreadHandler] Failed to get first thread. Error: %d", GetLastError());
    std::string thread_data("");
    do {
        ++counter.thread;
        char* data = new char[250];

        sprintf_s(data, 250, "Thread ID: %d | Owner PID: %d\n", thread.th32ThreadID, thread.th32OwnerProcessID);

        thread_data += std::string(data);
        delete data;
    } while (Thread32Next(h, &thread));

    CHECK(create_file("C:\\Facultate\\CSSO\\Laboratoare\\Week2\\fire.txt", thread_data.c_str(), thread_data.length()), (void*)false, "[ThreadHandler] Failed to create file.");

    return (void*)true;
}

void* proccess_handler(HANDLE h, Counters& counter)
{
    PROCESSENTRY32 proc;
    int current_pid = GetCurrentProcessId();
    proc.dwSize = sizeof(PROCESSENTRY32);
    CHECK(Process32First(h, &proc), (void*)false, "[ProccessHandler] Failed to get first proccess. Error: %d", GetLastError());
    std::string thread_data("");
    do {
        ++counter.proccess;
        char* data = new char[250];
        sprintf_s(data, 250, "Parent Proccess: %d | PID: %d | SzExe: %s\n", proc.th32ParentProcessID, proc.th32ProcessID, proc.szExeFile);
        thread_data += std::string(data);
        delete data;

        if (proc.th32ProcessID == current_pid) {
            CHECK(enumerate(TH32CS_SNAPMODULE, current_pid, counter, module_handler), (void*)false, "[ProcessHandler] Failed to dump module data for current proccess.");
        }
    } while (Process32Next(h, &proc));

    CHECK(create_file("C:\\Facultate\\CSSO\\Laboratoare\\Week2\\procese.txt", thread_data.c_str(), thread_data.length()), (void*)false, "[ProcessHandler] Failed to create file.");
    return (void*)true;
}

bool write_to_memory_mapping(const char* name, const char* data)
{
    HANDLE h = OpenFileMapping(FILE_MAP_WRITE, FALSE, name);
    CHECK(h != NULL, false, "[WriteToMemoryMapping] Failed to open memory handler. Error: %d", GetLastError());

    LARGE_INTEGER mmSize;
    mmSize.QuadPart = 0;

    MMData* buffer = (MMData*)MapViewOfFile(h, FILE_MAP_WRITE, mmSize.HighPart, mmSize.LowPart, 0);

    CHECK(buffer != NULL, false, "[WriteToMemoryMapping] Failed to read from handler. Error: %d", GetLastError());
    MMData* actual_data = new MMData();
    memcpy(actual_data->data, data, sizeof(actual_data->data));

    memcpy(buffer, actual_data, sizeof(MMData));
    UnmapViewOfFile(buffer);
    CloseHandle(h);
    delete actual_data;
    return true;
}

int main()
{
    Counters counters;

    CHECK(enable_privilege(SE_SYSTEM_PROFILE_NAME), FAIL_EXIT_CODE, "Failed to enable privilege!");
    CHECK(create_directory("C:\\Facultate\\CSSO\\Laboratoare\\Week2"), FAIL_EXIT_CODE, "Failed to create directory");
    CHECK(enumerate(TH32CS_SNAPPROCESS, 0, counters, proccess_handler), FAIL_EXIT_CODE, "Failed to list and dump all proccesses");
    CHECK(enumerate(TH32CS_SNAPTHREAD, 0, counters, thread_handler), FAIL_EXIT_CODE, "Failed to list and dump all threads");
    char* data = new char[100];
    counters.dump(data);
    CHECK(write_to_memory_mapping("cssow2basicsync",data), FAIL_EXIT_CODE, "Failed to write to memory!");
    delete data;

    //printf("Nr of proccesses: %d", proccess_counter);
    return 0;
}