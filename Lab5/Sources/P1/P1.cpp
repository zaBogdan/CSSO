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

bool create_directory(const char* path)
{
	if (!CreateDirectory(path, NULL)) {
		CHECK(!(GetLastError() == ERROR_PATH_NOT_FOUND), false, "[Create Directory] Failed to find `%s` path!", path);
		WARNING(GetLastError() == ERROR_ALREADY_EXISTS, "[Create Directory] Directory `%s` already exists.", path);
	}
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

bool get_everyone_sid(PSID& everyone_sid)
{
    DWORD cbSid = SECURITY_MAX_SID_SIZE;
    PSID bSID = (SID*)LocalAlloc(LMEM_FIXED, cbSid);
    CHECK(CreateWellKnownSid(WinWorldSid, NULL, bSID, &cbSid), false, "[SetPermission] Failed to get everyone SID. Error: %d", GetLastError());

    everyone_sid = bSID;
    return true;
}

bool create_security_descriptor(PSECURITY_DESCRIPTOR &childSD, EXPLICIT_ACCESS ea[], int explicit_access_size)
{
    int error = 0;
    PACL pACL = NULL;

    CHECK((error = SetEntriesInAcl(explicit_access_size, ea, NULL, &pACL)) == ERROR_SUCCESS, false, "[SetPermission] Failed to create ACL Entry. Error: %d", error);

    PSECURITY_DESCRIPTOR sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
    CHECK(sd != NULL, false, "[SetPermission] Failed to allocate SecurityDescriptor. Error: %d", GetLastError());

    CHECK(InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION), false, "[SetPermission] Failed to initialize SecurityDescriptor. Error: %d", GetLastError())

    CHECK(SetSecurityDescriptorDacl(sd, TRUE, pACL, FALSE) != ERROR_SUCCESS, false, "[SetPermission] Failed to set the DACL. Error: %d", GetLastError());
    childSD = sd;
    return true;
}

bool set_file_sd(const char* path)
{
    PSECURITY_DESCRIPTOR sd = NULL;

    PSID pEveryoneSID = NULL, pCurrentUser = NULL;
    CHECK(get_current_user_sid(pCurrentUser), false, "[SetFileSD] Failed to get current user sid");
    CHECK(get_everyone_sid(pEveryoneSID), false, "[SetFileSD] Failed to get everyone sid");
    
    EXPLICIT_ACCESS ea[2];
    ZeroMemory(&ea, sizeof(ea));
    ea[0].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[0].Trustee.ptstrName = (LPSTR)pCurrentUser;

    ea[1].grfAccessPermissions = GENERIC_READ;
    ea[1].grfAccessMode = SET_ACCESS;
    ea[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[1].Trustee.ptstrName = (LPSTR)pEveryoneSID;
    
    CHECK(create_security_descriptor(sd, ea, 2), false, "[SetFileSD] Failed to create Security descriptor");

    
    CHECK(SetFileSecurity(path, DACL_SECURITY_INFORMATION, sd), false, "[SetFileSD] Failed to set file SD. Error: %d", GetLastError());
    return true;
}

bool create_reg_key(HKEY origin, const char* sub_key)
{
    int error = -1;
    HKEY new_key;
    DWORD disposition;

    PSECURITY_DESCRIPTOR sd = NULL;

    PSID pEveryoneSID = NULL, pCurrentUser = NULL;
    CHECK(get_current_user_sid(pCurrentUser), false, "[SetFileSD] Failed to get current user sid");
    CHECK(get_everyone_sid(pEveryoneSID), false, "[SetFileSD] Failed to get everyone sid");

    EXPLICIT_ACCESS ea[2];
    ZeroMemory(&ea, sizeof(ea));
    ea[0].grfAccessPermissions = KEY_READ | KEY_WRITE;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[0].Trustee.ptstrName = (LPSTR)pCurrentUser;

    ea[1].grfAccessPermissions = KEY_READ;
    ea[1].grfAccessMode = SET_ACCESS;
    ea[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[1].Trustee.ptstrName = (LPSTR)pEveryoneSID;

    CHECK(create_security_descriptor(sd, ea, 2), false, "[SetFileSD] Failed to create Security descriptor");

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = false;
    sa.lpSecurityDescriptor = sd;

    CHECK((error = RegCreateKeyEx(origin, sub_key, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, &sa, &new_key, &disposition)) == ERROR_SUCCESS, false, "[WriteANewRegistryKey] Failed to create new registry. Error: %d", error);
    WARNING(disposition == REG_OPENED_EXISTING_KEY, "[WriteANewKeyWithData] The key already exists. Overriding current data.");
    
    RegCloseKey(new_key);
    return true;
}

int main()
{
    CHECK(create_directory("C:\\Facultate\\CSSO\\Laboratoare\\Week5\\RESTRICTED_WRITE"), 1, "Failed to create directory");
    CHECK(set_file_sd("C:\\Facultate\\CSSO\\Laboratoare\\Week5\\RESTRICTED_WRITE"), 1, "Failed to set permission to directory");

    CHECK(create_directory("C:\\Facultate\\CSSO\\Laboratoare\\Week5\\LOGS"), 1, "Failed to create directory");
    CHECK(set_file_sd("C:\\Facultate\\CSSO\\Laboratoare\\Week5\\LOGS"), 1, "Failed to set permission to directory");

    CHECK(create_reg_key(HKEY_CURRENT_USER, "Software\\CSSO\\Tema5"), 1, "Failed to create registry key");
    return 0;
}