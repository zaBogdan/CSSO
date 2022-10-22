#include <windows.h>
#include <stdio.h>

#include "Utils.h"
#define MEMORY_NAME "cssow2basicsync"
#define EXE_PATH "\\\\Mac\\Home\\Documents\\Work\\UAIC\\CSSO\\Lab2\\Sources\\x64\\Debug"

bool create_process(const char* path)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	CHECK(CreateProcess(path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi),
	false, "[CreateProcess] Failed to create process for exe. Error: %d", GetLastError());

	// here you can add a callback

	printf("[INFO] Starting to wait for process to finish\n");
	WaitForSingleObject(pi.hProcess, INFINITE);
	printf("[INFO] Starting to wait for process finished to wait\n");
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
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
	//printf("%s\n", buffer);
	printf("Memory mapping data is:\n%s\n", buffer->data);

	UnmapViewOfFile((void*)buffer);
	CloseHandle(h);
	return true;
}

int main()
{
	HANDLE memory_mapping;
	CHECK(create_memory_mapping(MEMORY_NAME, memory_mapping), false, "[Main] Failed to create memory mapping");
	CHECK(create_process(EXE_PATH "\\P1.exe"), false, "[Main] Failed to start and wait for process to finish");
	CHECK(read_from_memory_mapping(MEMORY_NAME), false, "[Main] Failed to read from memory mapping");
	CHECK(create_process(EXE_PATH "\\P2.exe"), false, "[Main] Failed to start and wait for process to finish");
	CloseHandle(memory_mapping);
	return 0;
}