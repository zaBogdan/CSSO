#pragma comment(lib, "Wininet") 
#include <string>
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <cstring>
#include <wininet.h>
#include <cstdlib>
#include <sstream>
#define AGENT "310910401RSL201301"
#define CONFIG_HOMEWORK "C:\\Facultate\\CSSO\\Laboratoare\\Week4\\myconfig.txt"
#define DOWNLOAD_PATH "C:\\Facultate\\CSSO\\Laboratoare\\Week4\\Downloads"

#include "Utils.h"

std::string latest_get_response;

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
    HANDLE h = CreateFile(path, FILE_APPEND_DATA, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    CHECK(h != INVALID_HANDLE_VALUE, false, "[Create File] Failed to create a file. Error: %d", GetLastError());

    DWORD bytesWritten;

    CHECK(WriteFile(h, data, input_size, &bytesWritten, NULL), false, "[Create File] Failed to write to file!. Error: %d", GetLastError());
    CloseHandle(h);
    return true;
}

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

void* read_response_callback(void* _ctx)
{
    RequestCTX* ctx = (RequestCTX*)_ctx;
    if (ctx->method == "POST") return (void*)true;

    uint16_t path_location = ctx->path.rfind('/');
    CHECK(path_location != std::string::npos, (void*)true, "[ReadResponse] Failed to split the path");

    std::string new_path = DOWNLOAD_PATH;
    new_path += ctx->path.substr(path_location);
    
    printf("File path: %s\n", new_path.c_str());
    
    CHECK(create_file(new_path.c_str(), ctx->data, ctx->size), (void*) false, "[ReadResponse] Failed to dump file. Erorr: %d", GetLastError());
    latest_get_response.assign(ctx->data);

    return (void*)true;
}

void* output_data(void* _ctx)
{
    RequestCTX* ctx = (RequestCTX*)_ctx;

    printf("\n[OutputData] RESPONSE:\n--------\n%s\n----------\n", ctx->data);
    return (void*)true;
}

void* assign_homework_callback(void* _ctx)
{
    RequestCTX* ctx = (RequestCTX*)_ctx;
    CHECK(create_file(CONFIG_HOMEWORK, ctx->data, ctx->size), (void*)false, "[AssignHomework] Failed to save file.");
    return (void*)true;
}

bool make_request(const char* method, const char* domain, const char* path, const char* data, uint16_t data_size, void* (*callback)(void*))
{
    HINTERNET h = InternetOpen(AGENT, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, NULL);
    CHECK(h != NULL, false, "[MakeRequest] Failed to get InternetOpen handle. Error: %d", GetLastError());
    HINTERNET hConn = InternetConnect(h, domain, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, NULL, NULL);
    CHECK(hConn != NULL, false, "[MakeRequest] Failed to get InternetOpenURL handle. Error: %d", GetLastError());

    HINTERNET httpReq = HttpOpenRequest(hConn, method, path, "1.1", "pground.io", NULL, INTERNET_FLAG_NO_AUTH, NULL);
    CHECK(httpReq != NULL, false, "[MakeRequest] Failed to get HttpOpenRequest handle. Error: %d", GetLastError());

    std::string headers = "";
    if (!strcmp(method, "POST"))
    {
        headers = "Content-Type: application/x-www-form-urlencoded";
    }

    CHECK(HttpSendRequest(httpReq, headers.c_str(), headers.size(), (void*)data, data_size), false, "[MakeRequest] Failed to send request to server");

    uint16_t reqSize = 100000;
    DWORD reqActualSize = 0;
    char* reqBuffer = new char[reqSize + 1];
    CHECK(InternetReadFile(httpReq, reqBuffer, reqSize, &reqActualSize), false, "[MakeRequest] Failed to read data. Error: %d", GetLastError());
    reqBuffer[reqActualSize] = '\n';
    reqBuffer[++reqActualSize] = 0;
    if (callback != nullptr) 
    {
        RequestCTX ctx;
        ctx.data = new char[reqActualSize];
        memcpy(ctx.data, reqBuffer, reqActualSize);
        ctx.size = reqActualSize;
        ctx.method = method;
        ctx.path = path;
        CHECK((bool)((*callback)((void*) &ctx)), false, "[MakeRequest] Callback function failed. Exiting...");
        delete ctx.data;
    }

    printf("[MakeRequest] Successfully finished request.\n");
    delete reqBuffer;
    InternetCloseHandle(httpReq);
    InternetCloseHandle(hConn);
    InternetCloseHandle(h);
    return true;
}

void* read_file_size(WIN32_FIND_DATA fdd, void* _ctx)
{
    Metrics* m = (Metrics*)_ctx;
    printf("File is: %s. Size: %d\n", fdd.cFileName, fdd.nFileSizeLow);
    m->total_file_size += fdd.nFileSizeHigh + fdd.nFileSizeLow;
    return (void*)true;
}

bool send_metrics(Metrics& m)
{
    CHECK(list_directory(DOWNLOAD_PATH "\\*", &m, read_file_size), false, "[SendMetrics] Failed to get file size");
    std::string payload = "id=" AGENT;
    payload += "&total=";
    payload += std::to_string(m.get_requests + m.post_requests);
    payload += "&get=";
    payload += std::to_string(m.get_requests);
    payload += "&post=";
    payload += std::to_string(m.post_requests);
    payload += "&size=";
    payload += std::to_string(m.total_file_size);

    printf("Metrics: %s", payload.c_str());
    make_request("POST", "cssohw.herokuapp.com", "/endhomework", payload.c_str(), payload.size(), output_data);

    return true;
}

bool do_homework()
{
    Metrics metrics;
    std::string data;
    CHECK(read_from_file(CONFIG_HOMEWORK, data), false, "[DoHomework] Failed to read from file.");
    auto ss = std::stringstream{ data };
    std::string post_data = "";
    for (std::string line; std::getline(ss, line, '\n');)
    {
        if (line[0] != 'G' && line[0] != 'P') continue;
        uint16_t split_position = line.find(':');

        if (split_position == std::string::npos) continue;

        std::string verb = line.substr(0, split_position);
        for (auto& c : verb) c = toupper(c);

        uint16_t path_position = line.rfind('/');
        if (path_position == std::string::npos) continue;
        std::string path = "dohomework";
        path += line.substr(path_position);

        printf("Verb: '%s', PATH: '%s'\n", verb.c_str(), path.c_str());

        if (verb == "POST") {
            post_data = "id=" AGENT;
            post_data += "&value=";
            size_t newline_loc = latest_get_response.find('\n');
            if (newline_loc != std::string::npos)
            {
                latest_get_response = latest_get_response.substr(0, newline_loc);
            }
            post_data += latest_get_response;
            ++metrics.post_requests;
        }
        else {
            ++metrics.get_requests;
        }
        make_request(verb.c_str(), "cssohw.herokuapp.com", path.c_str(), post_data.c_str(), post_data.size(), read_response_callback);
    }

    CHECK(send_metrics(metrics), false, "[DoHomework] Failed to send metrics.");

    return true;
}

int main()
{
    CHECK(create_directory("C:\\Facultate\\CSSO\\Laboratoare\\Week4"), FAIL_EXIT_CODE, "Failed to create directory");
    CHECK(create_directory("C:\\Facultate\\CSSO\\Laboratoare\\Week4\\Downloads"), FAIL_EXIT_CODE, "Failed to create directory");
    CHECK(make_request("GET", "cssohw.herokuapp.com", "assignhomework/" AGENT, NULL,0, assign_homework_callback), FAIL_EXIT_CODE, "Failed to make request to assignhomework");
    CHECK(do_homework(), FAIL_EXIT_CODE, "Failed to do the homework :(");

    return 0;
}


