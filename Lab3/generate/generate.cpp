#include "DirectoryCTX.h"
#include "..\\management\\Utils.h"

#include <string>
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <cstring>
#include <sstream>

#define SINGLE_RUN true
#define MEMORY_NAME "cssohw3management"
#define REPORTS_DIR "C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Reports\\Daily"

bool write_to_logs(const char* data, int data_size)
{
#ifdef SINGLE_RUN
	HANDLE hMutex = OpenMutex(SYNCHRONIZE, false, "Global\\guard_logs");
	CHECK(hMutex != INVALID_HANDLE_VALUE, false, "[WriteToLogs] Failed to open mutex!");
	WaitForSingleObject(hMutex, INFINITE);

	HANDLE h = CreateFile("C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Reports\\logs.txt", FILE_APPEND_DATA, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK(h != INVALID_HANDLE_VALUE, false, "[WriteToLogs] Failed to open log file. Error: %d", GetLastError());
	DWORD bytesWritten;

	CHECK(WriteFile(h, data, data_size, &bytesWritten, NULL), false, "[WriteToLogs] Failed to write to log file!");
	CloseHandle(h);
	ReleaseMutex(hMutex);
#endif
	return true;
}

bool read_from_file(const char* path, std::string& data)
{
	HANDLE h = CreateFile(path, GENERIC_READ, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK(h != INVALID_HANDLE_VALUE, false, "[ReadFromFile] Failed to read a file %s. Error: %d", path, GetLastError());

	DWORD file_size = 0, actual_size = 0;
	CHECK((file_size = GetFileSize(h, NULL)) != INVALID_FILE_SIZE, false, "[ReadFromFile] Failed to get file size! Error: %d", GetLastError());

	if (file_size != 0)
	{
		char* buff = new char[file_size + 1];
		CHECK(ReadFile(h, buff, file_size + 1, &actual_size, NULL), false, "[ReadFromFile] Failed to read from file. Error: %d", GetLastError());

		data = buff;

		delete buff;
	}

	CloseHandle(h);

	return true;
}

void* log_data_callback(WIN32_FIND_DATA fdd, void* ctx) {
	if (!strcmp(fdd.cFileName, ".") || !strcmp(fdd.cFileName, "..")) {
		return (void*)true;
	}
	DirectoryCTX* newCTX = (DirectoryCTX*)ctx;

	std::string file_name(fdd.cFileName);
	std::string file_path = REPORTS_DIR;
	file_path += "\\";
	file_path += file_name;

	std::string data;
	CHECK(read_from_file(file_path.c_str(), data), (void*)false, "Failed to read from file %s!", file_path.c_str());
	if (data.length() == 0)
	{
		data = "0";
	}
	printf("Data is: '%s' done\n", file_path.c_str());
	unsigned long long nr = std::stoi(data);
	if (file_name.find("_income") != std::string::npos)
	{
		newCTX->income += nr;
	} else {
		newCTX->payments += nr;
	}

	return (void*)true;
}

bool list_directory(const char* path, void* ctx, void* (*callback)(WIN32_FIND_DATA, void*)) {
	HANDLE h;
	WIN32_FIND_DATA ffd;
	h = FindFirstFile(path, &ffd);
	CHECK(h != INVALID_HANDLE_VALUE, false, "[List Directory] Failed to get handle. Error: %d", GetLastError());

	do {
		CHECK((bool)((*callback)(ffd, ctx)), false, "[List Directory] Callback function failed. Exiting...");
	} while (FindNextFile(h, &ffd) != 0);

	FindClose(h);
	return GetLastError();
}

bool read_from_memory_mapping(const char* name, MMData& ctx)
{
	HANDLE h = OpenFileMapping(FILE_MAP_READ, FALSE, name);
	CHECK(h != NULL, false, "[ReadFromMemoryMapping] Failed to open memory handler. Error: %d", GetLastError());
	LARGE_INTEGER mmSize;
	mmSize.QuadPart = 0;

	MMData* buffer = (MMData*)MapViewOfFile(h, FILE_MAP_READ, mmSize.HighPart, mmSize.LowPart, 0);
	printf("Memory mapping data is:\n%lld\n", buffer->data);

	memcpy(&ctx, buffer, sizeof(MMData));

	UnmapViewOfFile((void*)buffer);
	CloseHandle(h);
	return true;
}

int main()
{
	DirectoryCTX ctx;
	CHECK(list_directory(REPORTS_DIR "\\*", (void*)&ctx, log_data_callback), FAIL_EXIT_CODE, "[Main] Failed to handle work for directory!");

	std::string buff = "S-au facut incasari de " + std::to_string(ctx.income) + "\n";
	write_to_logs(buff.c_str(), buff.length());
	buff = "S-au facut plati de " + std::to_string(ctx.payments) + "\n";
	write_to_logs(buff.c_str(), buff.length());
	long long profit = ctx.income - ctx.payments;

	buff = "S-a realizat un profi de " + std::to_string(profit) + "\n";
	write_to_logs(buff.c_str(), buff.length());

	MMData data;
	CHECK(read_from_memory_mapping(MEMORY_NAME, data), false, "Failed to read from memory mapping");
	
	if (data.data == profit)
	{
		buff = "Report generat cu success!";
	}
	else {
		buff = "Ai o greseala la generarea raportului!";
	}
	write_to_logs(buff.c_str(), buff.length());
	return 0;
}