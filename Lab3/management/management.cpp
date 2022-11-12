#include "Utils.h"

#include <string>
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <cstring>

#define MEMORY_NAME "cssohw3management"
#define EXE_PATH "\\\\Mac\\Home\\Documents\\Work\\UAIC\\CSSO\\Lab3\\Sources\\x64\\Debug"


bool create_mutex(HANDLE& h, const char* name)
{
    h = CreateMutex(NULL, false, name);
    CHECK(h != NULL, false, "[CreateMutex] Failed to create mutex. Error: %d", GetLastError());

    return true;
}

bool create_semaphore(HANDLE& h, const char* name)
{
    h = CreateSemaphore(NULL, 1, 1, name);
    CHECK(h != NULL, false, "[CreateSemaphore] Failed to create semaphore. Error: %d", GetLastError());

    return true;
}

bool create_event(HANDLE& h, const char* name)
{
    h = CreateEvent(NULL, true, false, name);

    CHECK(h != NULL, false, "[CreateEvent] Failed to create event. Error: %d", GetLastError());
    return true;
}

bool create_directory(const char* path)
{
    if (!CreateDirectory(path, NULL)) {
        CHECK(!(GetLastError() == ERROR_PATH_NOT_FOUND), false, "[Create Directory] Failed to find `%s` path!", path);
        WARNING(GetLastError() == ERROR_ALREADY_EXISTS, "[Create Directory] Directory `%s` already exists.", path);
    }
    return true;
}

bool read_from_file(const char* path, std::string& data)
{
    HANDLE h = CreateFile(path, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    CHECK(h != INVALID_HANDLE_VALUE, false, "[ReadFromFile] Failed to read from file '%s'. Error: %d", path, GetLastError());

    DWORD file_size = 0, actual_size = 0;
    CHECK((file_size = GetFileSize(h, NULL)) != 0, false, "[ReadFromFile] Failed to get file size! Error: %d", GetLastError());

    char* buff = new char[file_size + 1];
    CHECK(ReadFile(h, buff, file_size + 1, &actual_size, NULL), false, "[ReadFromFile] Failed to read from file. Error: %d", GetLastError());

    data = buff;

    delete buff;
    CloseHandle(h);

    return true;
}

bool create_memory_mapping(const char* name, HANDLE& h)
{
    LARGE_INTEGER mmSize;
    mmSize.QuadPart = 1000;
    h = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, mmSize.HighPart, mmSize.LowPart, TEXT(name));
    CHECK(h != NULL, false, "[CreateMemoryMapping] Failed to create memory mapping. Error: %d", GetLastError());
    return true;
}

bool read_from_memory_mapping(const char* name)
{
    HANDLE h = OpenFileMapping(FILE_MAP_READ, FALSE, name);
    CHECK(h != NULL, false, "[ReadFromMemoryMapping] Failed to open memory handler. Error: %d", GetLastError());
    LARGE_INTEGER mmSize;
    mmSize.QuadPart = 0;

    MMData* buffer = (MMData*)MapViewOfFile(h, FILE_MAP_READ, mmSize.HighPart, mmSize.LowPart, 0);
    printf("Memory mapping data is:\n%lld\n", buffer->data);

    UnmapViewOfFile((void*)buffer);
    CloseHandle(h);
    return true;
}

bool write_to_memory_mapping(const char* name, int data)
{
    HANDLE h = OpenFileMapping(FILE_MAP_WRITE, FALSE, name);
    CHECK(h != NULL, false, "[WriteToMemoryMapping] Failed to open memory handler. Error: %d", GetLastError());

    LARGE_INTEGER mmSize;
    mmSize.QuadPart = 0;

    MMData* buffer = (MMData*)MapViewOfFile(h, FILE_MAP_WRITE, mmSize.HighPart, mmSize.LowPart, 0);

    CHECK(buffer != NULL, false, "[WriteToMemoryMapping] Failed to read from handler. Error: %d", GetLastError());
    MMData* actual_data = new MMData();
    actual_data->data = 0;

    memcpy(buffer, actual_data, sizeof(MMData));
    UnmapViewOfFile(buffer);
    CloseHandle(h);
    delete actual_data;
    return true;
}

bool create_process(PROCESS_INFORMATION& pi, const char* path)
{
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    CHECK(CreateProcess(path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi),
        false, "[CreateProcess] Failed to create process for exe. Error: %d", GetLastError());

    return true;
}

bool close_proccess_handle(PROCESS_INFORMATION& pi)
{
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}

bool launch_pay_and_income()
{
    HANDLE hMutexLog, hSemaphoreSummary, hEventMapping;
    CHECK(create_mutex(hMutexLog, "Global\\guard_logs"), false, "Failed to create mutex for log file!");
    CHECK(create_semaphore(hSemaphoreSummary, "Global\\guard_summary"), false, "Failed to create semaphore for summary file!");
    CHECK(create_event(hEventMapping, "Global\\guard_memory"), false, "Failed to create event for memory mapping!");


    printf("[INFO] Launching pay.exe and income.exe...\n");
    PROCESS_INFORMATION pay_pi, income_pi;
    CHECK(create_process(pay_pi, EXE_PATH "\\pay.exe"), false, "[Main] Failed to start proccess pay.exe");
    CHECK(create_process(income_pi, EXE_PATH "\\income.exe"), false, "[Main] Failed to start proccess income.exe");

    // i will set timeout to 120 seconds because on parallels it seems to run more slowly
    WaitForSingleObject(pay_pi.hProcess, 120 * 1000);
    WaitForSingleObject(income_pi.hProcess, 120 * 1000);
    printf("[INFO] Proccesses pay.exe and income.exe finished execution!\n");

    CHECK(close_proccess_handle(pay_pi), false, "");
    CHECK(close_proccess_handle(income_pi), false, "");

    CloseHandle(hMutexLog);
    CloseHandle(hSemaphoreSummary);
    CloseHandle(hEventMapping);
    return true;
}

int main()
{
    HANDLE memory_mapping;

    CHECK(create_directory("C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Reports"), FAIL_EXIT_CODE, "[Main] Failed to create directory");
    CHECK(create_directory("C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Reports\\Daily"), FAIL_EXIT_CODE, "[Main] Failed to create directory");
    CHECK(create_directory("C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Reports\\Summary"), FAIL_EXIT_CODE, "[Main] Failed to create directory");

    CHECK(create_memory_mapping(MEMORY_NAME, memory_mapping), FAIL_EXIT_CODE, "[Main] Failed to create memory mapping");
    CHECK(write_to_memory_mapping(MEMORY_NAME, 0), FAIL_EXIT_CODE, "[Main] Failed to write 0 to memory mapping");

    // launch both pay.exe and income.exe
    CHECK(launch_pay_and_income(), FAIL_EXIT_CODE, "");

    // launch generate.exe
    PROCESS_INFORMATION generate_pi;
    printf("[INFO] Launching generate.exe...\n");
    CHECK(create_process(generate_pi, EXE_PATH "\\generate.exe"), FAIL_EXIT_CODE, "[Main] Failed to start proccess generate.exe");
    WaitForSingleObject(generate_pi.hProcess, 120 * 1000);
    printf("[INFO] Proccesses generate.exe finished execution!\n");
    CHECK(close_proccess_handle(generate_pi), false, "");


    // read results
    std::string income, payments, summary;
    CHECK(read_from_file("C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Reports\\Summary\\income.txt", income), FAIL_EXIT_CODE, "[Main] Failed to read from file!");
    CHECK(read_from_file("C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Reports\\Summary\\payments.txt", payments), FAIL_EXIT_CODE, "[Main] Failed to read from file!");
    CHECK(read_from_file("C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Reports\\Summary\\summary.txt", summary), FAIL_EXIT_CODE, "[Main] Failed to read from file!");
    
    printf("Data from summary files:\nIncome: %s\nPayments: %s\nSummary: %s\n", income.c_str(), payments.c_str(), summary.c_str());

    CHECK(read_from_memory_mapping(MEMORY_NAME), FAIL_EXIT_CODE, "[Main] Failed to read from memory mapping");
    CloseHandle(memory_mapping);
}