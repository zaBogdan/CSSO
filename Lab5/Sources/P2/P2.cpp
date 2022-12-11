#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <string>
#include <cstring>
#include <securitybaseapi.h>
#include <aclapi.h>
#include <tchar.h>
#include <sddl.h>
#include "../Utils.h"
#define EXE_PATH "\\\\Mac\\Home\\Documents\\Work\\UAIC\\CSSO\\Lab5\\x64\\Debug"

bool write_to_logs(std::string& data)
{
    HANDLE h = CreateFile("C:\\Facultate\\CSSO\\Laboratoare\\Week5\\LOGS\\P2.log", FILE_APPEND_DATA, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    CHECK(h != INVALID_HANDLE_VALUE, false, "[WriteToLogs] Failed to open log file. Error: %d", GetLastError());
    DWORD bytesWritten;

    data += '\n';

    CHECK(WriteFile(h, data.c_str(), data.size(), &bytesWritten, NULL), false, "[WriteToLogs] Failed to write to log file!");
    CloseHandle(h);
    return true;
}

bool create_file(const char* path, std::string data) {
    HANDLE h = CreateFile(path, GENERIC_READ | GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    CHECK(h != INVALID_HANDLE_VALUE, false, "[Create File] Failed to create a file. Error: %d", GetLastError());

    DWORD bytesWritten;

    CHECK(WriteFile(h, data.c_str(), data.size(), &bytesWritten, NULL), false, "[Create File] Failed to write to file!. Error: %d", GetLastError());
    CloseHandle(h);
    return true;
}

bool get_current_user_sid(PSID& current_sid)
{
    DWORD userBuffSize = 256;
    char chCurrentUser[256];
    CHECK(GetUserName(chCurrentUser, &userBuffSize), false, "[GetCurrentUserSid] Failed to get current username. Error: %d", GetLastError());


    DWORD cbSid = SECURITY_MAX_SID_SIZE, domainNameSize = 256;

    PSID bSID = (SID*)LocalAlloc(LMEM_FIXED, cbSid);
    SID_NAME_USE user_sid_name;
    char szDomainName[256];
    CHECK(LookupAccountName(NULL, chCurrentUser, bSID, &cbSid, szDomainName, &domainNameSize, &user_sid_name), false, "[GetCurrentUserSid] Failed to get current user sid. Error: %d", GetLastError());

    current_sid = bSID;

    return true;
}

bool dump_sid_to_file(const char* path, std::string* copyUserSID = nullptr)
{
    std::string logs;
    PSID pCurrentUserSID;
    CHECK(get_current_user_sid(pCurrentUserSID), false, "[DumpSidToFile] Failed to get user sid");

    LPSTR userSid;
    ConvertSidToStringSid(pCurrentUserSID, &userSid);
    printf("User sid is: %s\n", userSid);
    std::string user_sid = userSid;
    if (!create_file(path, user_sid)) {
        logs = "Failed to write user pid to ";
        logs += path;
    }
    else {
        logs = "Successfully written the user sid";
    }
    write_to_logs(logs);


    if (copyUserSID != nullptr)
    {
        copyUserSID->assign(userSid);
    }

    return true;
}
void get_last_chars(std::string& str, int count)
{
    std::string from = "-";
    std::string to = "";
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    printf("Chars: %s\n", str.c_str());
    str = str.substr(1, max(0, str.size() - 6));
}

bool create_reg_key(HKEY origin, const char* sub_key, std::string& data)
{
    std::string logs;
    int error = -1;
    HKEY new_key;
    DWORD disposition;
    error = RegCreateKeyEx(origin, sub_key, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &new_key, &disposition);
    if (error != ERROR_SUCCESS)
    {
        logs = "Failed to create registry subkey. Error: " + std::to_string(error);
    }
    else {
        logs = "Successfully create registry subkey.";
    }
    write_to_logs(logs);

    WARNING(disposition == REG_OPENED_EXISTING_KEY, "[WriteANewKeyWithData] The key already exists. Overriding current data.");
    get_last_chars(data, 6);
    DWORD val = std::atoi(data.c_str());
    error = RegSetValueEx(new_key, "sid", NULL, REG_DWORD, (const BYTE*)&val, sizeof(val));
    if (error != ERROR_SUCCESS)
    {
        logs = "Failed to write REG_DWORD value. Error: " + std::to_string(error);
    }
    else {
        logs = "Successfully created REG_DWORD";
    }
    write_to_logs(logs);

    RegCloseKey(new_key);
    return true;
}

bool create_process(const char* path)
{
    std::string logs;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    if (!CreateProcess(path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        logs += "Failed to create process P3 exe.";
    }
    else {
        logs += "Successfully started process P3";
    }

    write_to_logs(logs);
    // here you can add a callback

    printf("[INFO] Starting to wait for process to finish\n");
    WaitForSingleObject(pi.hProcess, INFINITE);
    printf("[INFO] Starting to wait for process finished to wait\n");
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}


int main()
{
    std::string userSid, truncatedSid;
    CHECK(dump_sid_to_file("C:\\Facultate\\CSSO\\Laboratoare\\Week5\\RESTRICTED_WRITE\\P2.txt", &userSid), 17, "Failed to dump sid to file");
    truncatedSid = userSid.substr(userSid.size() - 6);
    CHECK(create_reg_key(HKEY_CURRENT_USER, "Software\\CSSO\\Tema5\\P2", truncatedSid), 17, "Failed to create registry subkey");
    CHECK(create_process(EXE_PATH "\\P3.exe"), 17, "Failed to start P3 process");
    return 0;
}