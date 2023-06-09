#pragma once
#include <iostream>
#include <csignal>
#include <minwindef.h>
#include <processenv.h>
#include <string>
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include "Config.h"

typedef std::uint64_t hash_t;

constexpr hash_t prime = 0x100000001B3ull;
constexpr hash_t basis = 0xCBF29CE484222325ull;

hash_t hash_(char const* str)
{
    hash_t ret{ basis };

    while (*str) {
        ret ^= *str;
        ret *= prime;
        str++;
    }

    return ret;
}

constexpr hash_t hash_compile_time(char const* str, hash_t last_value = basis)
{
    return *str ? hash_compile_time(str + 1, (*str ^ last_value) * prime) : last_value;
}

constexpr unsigned long long operator "" _hash(char const* p, size_t)
{
    return hash_compile_time(p);
}

//将string转换成wstring  
std::wstring string2wstring(string str)
{
    std::wstring result;
    //获取缓冲区大小，并申请空间，缓冲区大小按字符计算  
    int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);
    TCHAR* buffer = new TCHAR[len + 1];
    //多字节编码转换成宽字节编码  
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), buffer, len);
    buffer[len] = '\0';             //添加字符串结尾  
    //删除缓冲区并返回值  
    result.append(buffer);
    delete[] buffer;
    return result;
}

//将wstring转换成string  
string wstring2string(std::wstring wstr)
{
    string result;
    //获取缓冲区大小，并申请空间，缓冲区大小事按字节计算的  
    int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
    char* buffer = new char[len + 1];
    //宽字节编码转换成多字节编码  
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len, NULL, NULL);
    buffer[len] = '\0';
    //删除缓冲区并返回值  
    result.append(buffer);
    delete[] buffer;
    return result;
}

LPCWSTR stringToLPCWSTR(std::string orig)
{
    size_t origsize = orig.length() + 1;
    const size_t newsize = 100;
    size_t convertedChars = 0;
    wchar_t* wcstring = (wchar_t*)malloc(sizeof(wchar_t) * (orig.length() - 1));
    mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);

    return wcstring;
}

std::wstring GetProgramDir()
{
    TCHAR exeFullPath[MAX_PATH]; // Full path 
    GetModuleFileName(NULL, exeFullPath, MAX_PATH);
    std::wstring strPath = __TEXT("");
    strPath = (std::wstring)exeFullPath;    // Get full path of the file 
    int pos = strPath.find_last_of(L'\\', strPath.length());
    return strPath.substr(0, pos);  // Return the directory without the file name 
}

void startBDS(string exePath) {
    constexpr const DWORD MAX_PATH_LEN = 32767;
    auto* buffer = new wchar_t[MAX_PATH_LEN];

    // get all processes id with name "bedrock_server.exe" or "bedrock_server_mod.exe"
    // and pid is not current process
    std::vector<DWORD> pids;
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return;
    }
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            if (pe32.th32ProcessID != GetCurrentProcessId() &&
                (wcscmp(pe32.szExeFile, L"bedrock_server.exe") == 0 ||
                    wcscmp(pe32.szExeFile, L"bedrock_server_mod.exe") == 0)) {
                pids.push_back(pe32.th32ProcessID);
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }
    CloseHandle(hProcessSnap);

    // Get current process path
    /*std::wstring currentPath;
    GetModuleFileNameW(nullptr, buffer, MAX_PATH_LEN);
    currentPath = buffer;

    logger.info("currentPath");
    logger.info(wstring2string(currentPath));*/

    // Get the BDS process paths
    for (auto& pid : pids) {
        // Open process handle
        auto handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, false, pid);
        if (handle) {
            // Get the full path of the process
            std::wstring path;
            GetModuleFileNameExW(handle, nullptr, buffer, MAX_PATH_LEN);
            path = buffer;
            
            // Compare the path
            if (path == string2wstring(exePath)) {
                logger.info("Server is already running, terminate..");
                // Terminate
                TerminateProcess(handle, 1);
            }

            CloseHandle(handle);
        }
    }

    // start
    string cmd = "start \"\" \"" + exePath + "\"";
    system(cmd.c_str());

    delete[] buffer;
}

bool getBDSStatus(string exePath) {
    constexpr const DWORD MAX_PATH_LEN = 32767;
    auto* buffer = new wchar_t[MAX_PATH_LEN];

    // get all processes id with name "bedrock_server.exe" or "bedrock_server_mod.exe"
    // and pid is not current process
    std::vector<DWORD> pids;
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return false;
    }
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            if (pe32.th32ProcessID != GetCurrentProcessId() &&
                (wcscmp(pe32.szExeFile, L"bedrock_server.exe") == 0 ||
                    wcscmp(pe32.szExeFile, L"bedrock_server_mod.exe") == 0)) {
                pids.push_back(pe32.th32ProcessID);
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }
    CloseHandle(hProcessSnap);

    for (auto& pid : pids) {
        // Open process handle
        auto handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, false, pid);
        if (handle) {
            // Get the full path of the process
            std::wstring path;
            GetModuleFileNameExW(handle, nullptr, buffer, MAX_PATH_LEN);
            path = buffer;
            // Compare the path
            if (path == string2wstring(exePath)) {
                return true;
            }
            CloseHandle(handle);
        }
    }

    return false;
}

bool closeRunningDaemon(string deamonPath) {
    constexpr const DWORD MAX_PATH_LEN = 32767;
    auto* buffer = new wchar_t[MAX_PATH_LEN];

    // get all processes id with name "AutoRestart.exe"
    // and pid is not current process
    std::vector<DWORD> pids;
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return false;
    }
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            if (pe32.th32ProcessID != GetCurrentProcessId() &&
                (wcscmp(pe32.szExeFile, L"AutoRestart.exe") == 0)) {
                pids.push_back(pe32.th32ProcessID);
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }
    CloseHandle(hProcessSnap);

    for (auto& pid : pids) {
        // Open process handle
        auto handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, false, pid);
        if (handle) {
            // Get the full path of the process
            std::wstring path;
            GetModuleFileNameExW(handle, nullptr, buffer, MAX_PATH_LEN);
            path = buffer;
            // Compare the path
            if (path == string2wstring(deamonPath)) {
                logger.info("Terminate running AutoRestart process:" + std::to_string(pid));
                TerminateProcess(handle, 1);
            }
            CloseHandle(handle);
        }
    }

    return false;
}
