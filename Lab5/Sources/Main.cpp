#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <string>
#include <cstring>
#include <securitybaseapi.h>
#include <aclapi.h>
#include <tchar.h>
#include <sddl.h>
#include "Utils.h"
#define EXE_PATH "\\\\Mac\\Home\\Documents\\Work\\UAIC\\CSSO\\Lab5\\x64\\Debug"

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

bool get_everyone_sid(PSID& everyone_sid)
{
	PSECURITY_DESCRIPTOR sd = NULL;

	DWORD cbSid = SECURITY_MAX_SID_SIZE;
	PSID bSID = (SID*)LocalAlloc(LMEM_FIXED, cbSid);
	CHECK(CreateWellKnownSid(WinWorldSid, NULL, bSID, &cbSid), false, "[SetPermission] Failed to get everyone SID. Error: %d", GetLastError());

	everyone_sid = bSID;
	return true;
}

bool create_security_descriptor(PSECURITY_DESCRIPTOR& childSD, PACL oldAcl, EXPLICIT_ACCESS ea[], int explicit_access_size)
{
	int error = 0;
	PACL pACL = NULL;

	CHECK((error = SetEntriesInAcl(explicit_access_size, ea, oldAcl, &pACL)) == ERROR_SUCCESS, false, "[SetPermission] Failed to create ACL Entry. Error: %d", error);

	PSECURITY_DESCRIPTOR sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	CHECK(sd != NULL, false, "[SetPermission] Failed to allocate SecurityDescriptor. Error: %d", GetLastError());

	CHECK(InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION), false, "[SetPermission] Failed to initialize SecurityDescriptor. Error: %d", GetLastError())

		CHECK(SetSecurityDescriptorDacl(sd, TRUE, pACL, FALSE) != ERROR_SUCCESS, false, "[SetPermission] Failed to set the DACL. Error: %d", GetLastError());
	childSD = sd;
	return true;
}

bool disable_subkey_permission(HKEY hive_key, const char* path)
{
	int error;
	HKEY hKey;

	CHECK((error = RegOpenKeyEx(hive_key, path, 0, KEY_READ, &hKey)) == ERROR_SUCCESS, false, "Failed to open key. Error: %d", error);
	PACL oldPACL;
	DWORD oldSDSize;
	
	CHECK((error = GetSecurityInfo(hKey, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION, nullptr, nullptr, &oldPACL, nullptr, nullptr)) == ERROR_SUCCESS, false, "Failed to get security information. Error: %d", error);
	//CHECK((error = RegGetKeySecurity(hKey, DACL_SECURITY_INFORMATION, &oldSD, &oldSDSize)) == ERROR_SUCCESS, false, "Failed to get regkey security! Error: %d", error);
	PSECURITY_DESCRIPTOR sd;

	PSID pEveryoneSID = NULL;
	CHECK(get_everyone_sid(pEveryoneSID), false, "[SetFileSD] Failed to get everyone sid");

	EXPLICIT_ACCESS ea[1];
	ZeroMemory(&ea, sizeof(ea));
	ea[0].grfAccessPermissions = KEY_CREATE_SUB_KEY;
	ea[0].grfAccessMode = DENY_ACCESS;
	ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea[0].Trustee.ptstrName = (LPSTR)pEveryoneSID;
	CHECK(create_security_descriptor(sd, oldPACL, ea, 1), false, "[SetFileSD] Failed to create Security descriptor");

	RegCloseKey(hKey);
	return true;
}

bool delete_reg_key(HKEY hive_key, const char* path, const char* value)
{
	int error;
	HKEY hKey;
	CHECK((error = RegOpenKeyEx(hive_key, path, 0, KEY_SET_VALUE, &hKey)) == ERROR_SUCCESS, false, "Failed to open key. Error: %d", error);
	CHECK((error = RegDeleteValue(hKey, value)) == ERROR_SUCCESS, false, "Failed to delete value. Error: %d", error);
	
	RegCloseKey(hKey);
	return true;
}

int main()
{
	CHECK(create_process(EXE_PATH "\\P1.exe"), false, "[Main] Failed to start and wait for P1 process to finish");
	CHECK(create_process(EXE_PATH "\\P2.exe"), false, "[Main] Failed to start and wait for P2 process to finish");
	CHECK(delete_reg_key(HKEY_CURRENT_USER, "Software\\CSSO\\Tema5\\P3", "Everyone"), 17, "Failed to delete subkey");
	CHECK(disable_subkey_permission(HKEY_CURRENT_USER, "Software\\CSSO\\Tema5"), 17, "Failed to disable subkey permission");
	CHECK(create_process(EXE_PATH "\\P2.exe"), false, "[Main] Failed to start and wait for P2 process to finish");

	return 0;
}