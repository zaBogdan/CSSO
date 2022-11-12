#include "..\\management\\Utils.h"
#include "..\\pay\\DirectoryCTX.h"

#include <string>
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <cstring>
#include <sstream>

#define SINGLE_RUN true
#define MEMORY_NAME "cssohw3management"
#define PAYMENTS_DIR "C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Input\\income\\"

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

bool write_to_normal_file(const char* path, const char* data, int input_size) {
	HANDLE h = CreateFile(path, GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK(h != INVALID_HANDLE_VALUE, false, "[Create File] Failed to create a file. Error: %d", GetLastError());

	DWORD bytesWritten;

	CHECK(WriteFile(h, data, input_size + 1, &bytesWritten, NULL), false, "[Create File] Failed to write to file!. Error: %d", GetLastError());
	CloseHandle(h);
	return true;
}

bool read_from_file(const char* path, std::string& data)
{
	HANDLE h = CreateFile(path, GENERIC_READ, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK(h != INVALID_HANDLE_VALUE, false, "[ReadFromFile] Failed to create a file %s. Error: %d",path, GetLastError());

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

bool update_counts(std::string path, unsigned long long total_size_per_day, bool add = true)
{
	std::string old_value;
	CHECK(read_from_file(path.c_str(), old_value), false, "Failed to read from income.txt");

	if (old_value.length() == 0)
	{
		old_value = "0";
	}

	long long sum = total_size_per_day;
	std::string new_value;
	try {
		new_value = std::to_string(std::stoi(old_value) + sum);

	}
	catch(...) {
		printf("----------------------------------------------------------- %s\n", old_value);
	}

	CHECK(write_to_normal_file(path.c_str(), new_value.c_str(), new_value.length()),
		false,
		"Failed to write to income.txt");
	return true;
}

bool update_summary(unsigned long long total_size_per_day)
{
#ifdef SINGLE_RUN
	HANDLE hSemaphore = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, false, "Global\\guard_summary");
	CHECK(hSemaphore != NULL, false, "[UpdateSymmary] Failed to open semaphore!");
	WaitForSingleObject(hSemaphore, INFINITE);
#endif // SINGLE_RUN
	update_counts("C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Reports\\Summary\\summary.txt", total_size_per_day);

#ifdef SINGLE_RUN
	ReleaseSemaphore(hSemaphore, 1, NULL);
#endif
	return true;
}

void* log_data_callback(WIN32_FIND_DATA fdd, void* ctx) {
	DirectoryStatsCTX* newCTX = (DirectoryStatsCTX*)ctx;
	if (!strcmp(fdd.cFileName, ".") || !strcmp(fdd.cFileName, "..")) {
		return (void*)true;
	}

	std::string file_path = PAYMENTS_DIR, file_content;
	file_path += fdd.cFileName;
	printf("Reading file %s done\n", file_path.c_str());
	CHECK(read_from_file(file_path.c_str(), file_content), (void*)false, "Failed to read from file!");

	unsigned long long total_size_per_day = 0;

	std::stringstream ss(file_content);
	std::string word;

	while (!ss.eof()) {
		std::getline(ss, word, '\n');
		if (word.length() == 0 || ss.eof()) {
			continue;
		}
		word.pop_back();
		std::string buff = "Am facut o incasare in valoare de " + word + " in data de " + fdd.cFileName + "\n";
		total_size_per_day += std::stoi(word);
		write_to_logs(buff.c_str(), buff.length());
	}

	std::string day_payments = "C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Reports\\Daily\\";
	day_payments += fdd.cFileName;
	day_payments += "_income.txt";
	std::string total_amount = std::to_string(total_size_per_day);

	CHECK(write_to_normal_file(day_payments.c_str(), total_amount.c_str(), total_amount.length()), (void*)false, "Failed to write total amount per day");
	CHECK(update_counts("C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Reports\\Summary\\income.txt", total_size_per_day), (void*)false, "Failed to update income.txt");
	CHECK(update_summary(total_size_per_day), (void*)false, "Failed to update summary.txt");

	newCTX->total_data += total_size_per_day;
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

bool write_to_memory_mapping(const char* name, int data)
{
	HANDLE h = OpenFileMapping(FILE_MAP_WRITE, FALSE, name);
	CHECK(h != NULL, false, "[WriteToMemoryMapping] Failed to open memory handler. Error: %d", GetLastError());

	LARGE_INTEGER mmSize;
	mmSize.QuadPart = 0;

	MMData* buffer = (MMData*)MapViewOfFile(h, FILE_MAP_WRITE, mmSize.HighPart, mmSize.LowPart, 0);

	CHECK(buffer != NULL, false, "[WriteToMemoryMapping] Failed to read from handler. Error: %d", GetLastError());
	MMData* actual_data = new MMData();
	actual_data->data = data;

	memcpy(buffer, actual_data, sizeof(MMData));
	UnmapViewOfFile(buffer);
	CloseHandle(h);
	delete actual_data;
	return true;
}

bool update_memory_mapping(unsigned long long total)
{
#ifdef SINGLE_RUN
	HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, false, "Global\\guard_memory");

	CHECK(hEvent != INVALID_HANDLE_VALUE, false, "Failed to get event handler!");
	WaitForSingleObject(hEvent, INFINITE);
	MMData data;
	CHECK(read_from_memory_mapping(MEMORY_NAME, data), false, "Failed to read from memory mapping");
	data.data -= total;
	CHECK(write_to_memory_mapping(MEMORY_NAME, data.data), false, "Failed to update value from memory mapping");
#endif
	return true;
}

int main()
{
	DirectoryStatsCTX ctx;
	ctx.total_data = 0;
	CHECK(list_directory(PAYMENTS_DIR "\\*", (void*)&ctx, log_data_callback), FAIL_EXIT_CODE, "[Main] Failed to handle work for directory!");

	printf("[INCOME] Total data: %d\n", ctx.total_data);
	CHECK(update_memory_mapping(ctx.total_data), FAIL_EXIT_CODE, "[Main] Failed to write to memory mapping!");
	return 0;
}