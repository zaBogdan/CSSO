#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <string>
#include <cstring>
#include <securitybaseapi.h>
#include <aclapi.h>
#include <tchar.h>
#include <sddl.h>
#include <vector>
#include <algorithm>
#include "../Utils.h"

bool write_to_logs(std::string& data)
{
    HANDLE h = CreateFile("C:\\Facultate\\CSSO\\Laboratoare\\Week5\\LOGS\\P3.log", FILE_APPEND_DATA, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
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

bool get_sid_by_type(PSID& everyone_sid, WELL_KNOWN_SID_TYPE sid_type)
{
    DWORD cbSid = SECURITY_MAX_SID_SIZE;
    PSID bSID = (SID*)LocalAlloc(LMEM_FIXED, cbSid);
    CHECK(CreateWellKnownSid(sid_type, NULL, bSID, &cbSid), false, "[SetPermission] Failed to get everyone SID. Error: %d", GetLastError());
    
    everyone_sid = bSID;
    return true;
}

bool get_admin_and_everyone_sid(const char* path, std::vector<std::string>& sids)
{
    std::string logs, adminSid, everyoneSid, data;
    PSID pEveryoneSID, pAdministratorSid;
    CHECK(get_sid_by_type(pEveryoneSID, WinWorldSid), false, "[DumpSidToFile] Failed to get everyone sid");
    CHECK(get_sid_by_type(pAdministratorSid, WinBuiltinAdministratorsSid), false, "[DumpSidToFile] Failed to get everyone sid");

    LPSTR userSid;
    ConvertSidToStringSid(pEveryoneSID, &userSid);
    everyoneSid = userSid;
    ConvertSidToStringSid(pAdministratorSid, &userSid);
    adminSid = userSid;

    data = everyoneSid + "\n" + adminSid + "\n";
    sids.push_back(everyoneSid);
    sids.push_back(adminSid);

    if (create_file(path, data))
    {
        logs = "Successfully dumped admin and everyone sid";
    }
    else 
    {
        logs = "Failed to dump admin and everyone sid";
    }
    write_to_logs(logs);
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

bool create_reg_key(HKEY origin, const char* sub_key, std::vector<std::string>& data)
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

    std::string name = "Everyone";
    bool first = true;

    for (auto& d : data)
    {
        if (!first)
        {
            name = "Administrator";
        }
        get_last_chars(d, 6);

        DWORD val = std::atoi(d.c_str());
        error = RegSetValueEx(new_key, name.c_str(), NULL, REG_DWORD, (const BYTE*)&val, sizeof(val));
        if (error != ERROR_SUCCESS)
        {
            logs = "Failed to write REG_DWORD value. Error: " + std::to_string(error);
        }
        else {
            logs = "Successfully created REG_DWORD";
        }
        write_to_logs(logs);
        first = !first;
    }


    RegCloseKey(new_key);
    return true;
}

int main()
{
    std::vector<std::string> group_sids;
    CHECK(get_admin_and_everyone_sid("C:\\Facultate\\CSSO\\Laboratoare\\Week5\\RESTRICTED_WRITE\\P3.txt", group_sids), 17, "Failed to dump sid to file");
    CHECK(create_reg_key(HKEY_CURRENT_USER, "Software\\CSSO\\Tema5\\P3", group_sids), 17, "Failed to create registry subkey");

    return 0;
}