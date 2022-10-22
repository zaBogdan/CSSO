#include <windows.h>
#include <string>
#include <tchar.h>
#include <strsafe.h>

#include "Utils.h"

#define MAX_THREADS 3
#define BUF_SIZE 455
#define MAX_FILE_CONTENT 100000

bool read_from_file(const char* path, std::string& data)
{
    HANDLE h = CreateFile(path, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    CHECK(h != INVALID_HANDLE_VALUE, false, "[ReadFromFile] Failed to create a file. Error: %d", GetLastError());

    DWORD file_size = 0, actual_size = 0;
    CHECK((file_size = GetFileSize(h, NULL)) != 0, false, "[ReadFromFile] Failed to get file size! Error: %d", GetLastError());

    char* buff = new char[file_size + 1];
    CHECK(ReadFile(h, buff, file_size + 1, &actual_size, NULL), false, "[ReadFromFile] Failed to read from file. Error: %d", GetLastError());
    
    data = buff;

    delete buff;
    CloseHandle(h);
    
    return true;
}

int find_newlines(std::string& data)
{
    const char* to_find = "\n";
    int counter = 0;
    size_t pos = data.find(to_find, 0);
    while (pos != std::string::npos)
    {
        ++counter;
        pos = data.find(to_find, pos + 1);
    }
    return counter;
}

DWORD WINAPI get_new_lines_from_file(void* data)
{
    ThreadData* td = (ThreadData*)data;
    HANDLE std_out_handler;
    TCHAR msgBuf[BUF_SIZE];
    size_t str_len = -1;
    DWORD chars;
    std::string fileData;
    CHECK(read_from_file(td->fileName, fileData), false, "[GetNewLinesFromFile] Failed to read file. Exiting...");
    int nlCounter = find_newlines(fileData);

    std_out_handler = GetStdHandle(STD_OUTPUT_HANDLE);
    CHECK(std_out_handler != ERROR_SUCCESS, 0, "[GetNewLinesFromFile] Failed to get handle for stdout. Error: %d", GetLastError());

    StringCchPrintf(msgBuf, BUF_SIZE, TEXT("File = %s | Number of new lines: %d\n"), td->fileName, nlCounter);
    StringCchLength(msgBuf, BUF_SIZE, &str_len);
    WriteConsole(std_out_handler, msgBuf, (DWORD)str_len, &chars, NULL);

    return 1;
}

int main()
{
    HANDLE hThreads[MAX_THREADS];
    DWORD threadIDs[MAX_THREADS];
    ThreadData* p_thread_data[MAX_THREADS];

    ThreadData thread_data[MAX_THREADS]{
        ThreadData("C:\\Facultate\\CSSO\\Laboratoare\\Week2\\procese.txt"),
        ThreadData("C:\\Facultate\\CSSO\\Laboratoare\\Week2\\fire.txt"),
        ThreadData("C:\\Facultate\\CSSO\\Laboratoare\\Week2\\module_process.txt"),
    };


    for (int idx = 0; idx < MAX_THREADS; ++idx)
    {
        p_thread_data[idx] = (ThreadData*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ThreadData));
        CHECK(p_thread_data[idx] != NULL, EXIT_CODE_FAIL, "[Main] Failed to allocate memory");

        p_thread_data[idx]->fileName = thread_data[idx].fileName;

        hThreads[idx] = CreateThread(
            NULL,
            0,
            get_new_lines_from_file,
            p_thread_data[idx],
            0,
            &threadIDs[idx]
        );

        CHECK(hThreads[idx] != NULL, EXIT_CODE_FAIL, "[Main] Failed to create thread! Error: %d", GetLastError());
    }
    WaitForMultipleObjects(MAX_THREADS, hThreads, TRUE, INFINITE);

    for (int i = 0; i < MAX_THREADS; i++)
    {
        CloseHandle(hThreads[i]);
        if (p_thread_data[i] != NULL)
        {
            HeapFree(GetProcessHeap(), 0, p_thread_data[i]);
            p_thread_data[i] = NULL;    // Ensure address is not reused.
        }
    }

    return 0;
}

