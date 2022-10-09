#include "Utils.h"

#include <windows.h>
#include <stdio.h>
#include <string>
#include <cstring>

#define MAX_BUFFER 4000
using namespace std;

void* filter_files_callback(WIN32_FIND_DATA fdd,void* ctx) {
	DirectoryStatsCTX* data = (DirectoryStatsCTX*)ctx;
	
	for (int idx = 0; idx < data->file_count; ++idx) {
		if (!strcmp(fdd.cFileName, data->used_files[idx])) {
			char* file_buffer = new char[150];
			int written_bytes = snprintf(file_buffer, 150, "%s\\%s %d\n", data->path, fdd.cFileName, fdd.nFileSizeLow);
			data->buffer += file_buffer;
			delete file_buffer;
			break;
		}
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

bool dump_from_hive(const char* file_name, HKEY hive_key)
{
	char* file_data = new char[100];
	char* file_path = new char[200];

	DWORD lpcSubKeys, lpcMaxSubKeyLen;
	FILETIME lpftLastWriteTime;
	SYSTEMTIME sys_time;

	CHECK(RegQueryInfoKey(hive_key, NULL, NULL, NULL, &lpcSubKeys, &lpcMaxSubKeyLen, NULL, NULL, NULL, NULL, NULL, &lpftLastWriteTime) == ERROR_SUCCESS, false, "Failed to get info about registry. Error code: %d", GetLastError());

	CHECK(FileTimeToSystemTime(&lpftLastWriteTime, &sys_time), false, "Failed to convert to system time!");

	int file_size = sprintf_s(file_data, 100, "%d\n%d\n%d/%d/%d %d:%d:%d\n", lpcSubKeys, lpcMaxSubKeyLen, sys_time.wYear, sys_time.wMonth, sys_time.wDay, sys_time.wHour, sys_time.wMinute, sys_time.wSecond);

	CHECK(file_size >= 0, false, "Failed to format data properly");


	CHECK(sprintf_s(file_path, 200, "C:\\Facultate\\CSSO\\Laboratoare\\Week1\\Rezultate\\%s", file_name) >= 0, false, "");
	CHECK(create_file(file_path, file_data, file_size), false, "[Dump From Hive] Can't dump to file!");

	delete file_data;
	delete file_path;
	return true;
}

bool dump_directory_stats(const char* path,std::string& written_path, int& file_size)
{
	std::string buffer;
	DirectoryStatsCTX ctx = { 3, path, { "HKLM.txt", "HKCC.txt", "HKCU.txt" }, buffer };
	std::string search_path = path;
	written_path = search_path + "\\sumar.txt";
	search_path += "\\*";

	CHECK(list_directory(search_path.c_str(), (void*)&ctx, filter_files_callback), false, "[Get directory stats] Failed to list directory!");
	file_size = (int)buffer.size();

	CHECK(create_file(written_path.c_str(), buffer.c_str(), file_size), false, "[Get directory stats] Can't dump to file!");

	return true;
}

bool dump_data_from_registers(const char* path, HKEY hive_key, unsigned int count)
{
	int response = -1;
	DWORD lpcSubKeys, lpcMaxSubKeyLen, lpcValues, lpcMaxValueNameLen, lpcMaxValueLen;
	CHECK(RegQueryInfoKey(hive_key, NULL, NULL, NULL, &lpcSubKeys, &lpcMaxSubKeyLen, NULL, &lpcValues, &lpcMaxValueNameLen, &lpcMaxValueLen, NULL, NULL) == ERROR_SUCCESS, false, "Failed to get info about registry. Error code: %d", GetLastError());
	WARNING(lpcSubKeys <= count, "[DumpDataFromRegisters] We don't have enough keys! We will count only to: %d", lpcSubKeys); 

	for (unsigned int keyIdx = 0; keyIdx < count; ++keyIdx) {
		char* keyName = new char[255];
		DWORD keySize;
		
		CHECK(RegEnumKey(hive_key, keyIdx, keyName, 255) == ERROR_SUCCESS, false, "[DumpDataFromRegisters] Failed to get key at idx: %d. Error: %d", keyIdx, GetLastError());
		
		// handling default cases, because the files would be harder to create
		if (strcmp(keyName, "*") == 0 || strcmp(keyName, ".") == 0)
		{
			++count;
			RegCloseKey(hive_key);
			delete keyName;
			continue;
		}

		HKEY key_at_idx;
		CHECK((response = RegOpenKeyEx(hive_key, keyName,0, KEY_QUERY_VALUE, &key_at_idx)) == ERROR_SUCCESS, false, "[DumpDataFromRegisters] Failed to open key %s. Error: %d", keyName, response);


		BYTE reg_buffer[200];
		DWORD size = 200;
		DWORD type;
		CHECK((response = RegQueryValueEx(key_at_idx, "", NULL, &type, reg_buffer, &size)) == ERROR_SUCCESS, false, "[DumpDataFromRegisters] Failed to get data from registry. Error: %d", response);
		
		std::string path_extended = path;
		path_extended += "\\";
		path_extended += keyName;

		CHECK(create_file(path_extended.c_str(), reinterpret_cast<const char*>(reg_buffer), size), false, "[Get directory stats] Can't dump to file!");

		
		RegCloseKey(key_at_idx);
		delete keyName;
	}
	return true;
}

bool write_a_new_key_with_data(HKEY origin, const char* sub_key, const char *buffer, int bufferSize, int file_size)
{
	int error = -1;
	HKEY new_key;
	DWORD disposition;
	
	CHECK((error = RegCreateKeyEx(origin, sub_key, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &new_key, &disposition)) == ERROR_SUCCESS, false, "[WriteANewRegistryKey] Failed to create new registry. Error: %d", error);
	WARNING(disposition == REG_OPENED_EXISTING_KEY, "[WriteANewKeyWithData] The key already exists. Overriding current data.");

	CHECK((error = RegSetValueEx(new_key, "summary_path", NULL, REG_SZ, reinterpret_cast<const BYTE*>(buffer), bufferSize)) == ERROR_SUCCESS, false, "[WriteANewRegistryKey] Failed to write data. Error: %d", error);
	CHECK((error = RegSetValueEx(new_key, "summary_size", NULL, REG_DWORD, (const BYTE*)&file_size, sizeof(file_size)) == ERROR_SUCCESS), false, "[WriteANewRegistryKey] Failed to write data. Error: %d", error);
	
	RegCloseKey(new_key);
	return true;
}

int main()
{
	// 1
	CHECK(create_directory("C:\\Facultate\\CSSO\\Laboratoare\\Week1"), 1, "");
	CHECK(create_directory("C:\\Facultate\\CSSO\\Laboratoare\\Week1\\Extensii"), 1, "");
	CHECK(create_directory("C:\\Facultate\\CSSO\\Laboratoare\\Week1\\Rezultate"), 1, "");

	// 2
	CHECK(dump_from_hive("HKLM.txt", HKEY_LOCAL_MACHINE), 1, "Failed to save data from hive HKEY_LOCAL_MACHINE");
	CHECK(dump_from_hive("HKCC.txt", HKEY_CURRENT_CONFIG), 1, "Failed to save data from hive HKEY_CURRENT_CONFIG");
	CHECK(dump_from_hive("HKCU.txt", HKEY_CURRENT_USER), 1, "Failed to save data from hive HKEY_CURRENT_USER");

	// 3
	std::string result_path;
	int buffer_size = 0;
	CHECK(dump_directory_stats("C:\\Facultate\\CSSO\\Laboratoare\\Week1\\Rezultate", result_path, buffer_size), 1, "Failed to handle sumar.txt");

	// 4
	CHECK(dump_data_from_registers("C:\\Facultate\\CSSO\\Laboratoare\\Week1\\Extensii", HKEY_CLASSES_ROOT, 3), 1, "Failed to read data from registry");
	
	// 5
	CHECK(write_a_new_key_with_data(HKEY_CURRENT_USER, "Software\\CSSO\\Week1", result_path.c_str(), (int)result_path.size(), buffer_size), 1, "Failed to create a new key with speicified data");
	return 0;
}