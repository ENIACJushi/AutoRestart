#include <iostream>
#include <Windows.h>
#include <thread>
#include <vector>
#include <time.h>
#include "Nlohmann/json.hpp"

#include "Config.h"
#include "Tools.h"
#include "main.h"

const string pluginPath = "./plugins/AutoRestart";

const string basePath = wstring2string(GetProgramDir());
const string serverPath = basePath + "\\bedrock_server_mod.exe";
const string deamonPath = basePath + "\\AutoRestart.exe";

nlohmann::json getChannelMessage() {
    std::ifstream file;
    file.open(pluginPath + "/channel.json", std::ios::in);
    if (file) {
        std::istreambuf_iterator<char> beg(file), end;
        string configString = std::string(beg, end);
        file.seekg(0, std::ios::end); //移动到文件尾部
        file.close();
        // 转为json
        nlohmann::json channelInfo;
        try {
            channelInfo = nlohmann::json::parse(configString.c_str(), nullptr, true);
            return channelInfo;
        }
        catch (const std::exception& ex) {
            logger.error(ex.what());
        }
    }
    return NULL;
}

void setChannelMessage(nlohmann::json message) {
    std::fstream newFile;
    newFile.open(pluginPath + "/channel.json", std::fstream::in | std::fstream::out | std::fstream::trunc);
    newFile << message.dump();
    newFile.close();
}

bool isTimeOut(nlohmann::json channelInfo, time_t timeout) {
    if (channelInfo.contains("time")) {
        time_t now = time(0);
        time_t differ = now - channelInfo["time"];
        if (differ < timeout) {
            return false;
        }
    }
    return false;
}

void setStatus2Start() {
    setChannelMessage(nlohmann::json{
        {"time", time(0) },
        {"instruction", "start"} });
}

int main(int argc, char* argv[])
{
    // 若被服务器调用，则启动一个新进程，关闭当前进程
    for (int argi = 0; argi < argc; argi++) {
        logger.info(argv[argi]);
        if (strcmp(argv[argi], "--server") == 0) {
            logger.info("Initiated by a server, start a new process and quit..");
            ShellExecuteW(NULL, L"open", stringToLPCWSTR(deamonPath), NULL, stringToLPCWSTR(basePath), SW_SHOW);
            /*
            STARTUPINFO si;
            PROCESS_INFORMATION pi;

            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));
            if (CreateProcessW(NULL, (LPWSTR)(LPCWSTR)deamonPath.c_str(), 0, 0, false, CREATE_DEFAULT_ERROR_MODE | CREATE_NO_WINDOW | DETACHED_PROCESS, 0, 0, &si, &pi) != false) {
                
                WaitForSingleObject(pi.hProcess, (2 * 1000));
            }
            else {
                GetLastError();
            }
            */
            return 0;
        }
    }
    
    // 加载配置
    system("chcp 65001");
    logger.info("Loading config..");
    if (!loadConfig(pluginPath, "Config.json")) {
        logger.error("Failed to load config.");
        system("pause");
        return 0;
    }
    logger.info("Config load successfully.");
    
    // 关闭其它守护进程
    closeRunningDaemon(deamonPath);

    // 启动服务器
    {
        nlohmann::json channelInfo = getChannelMessage();
        bool running = false;
        if (channelInfo["instruction"] == "tick") {
            if (channelInfo.contains("time")) {
                time_t now = time(0);
                time_t differ = now - channelInfo["time"];
                if (differ < Config["timeout"]) {
                    running = true;
                }
            }
        }

        if (running) {
            logger.info("The server is already running.");
        }
        else {
            logger.info("Starting server..");
            setStatus2Start();
            startBDS(serverPath);
            while (!getBDSStatus(serverPath)) {
                Sleep(100);
            }
        }
    }

    // 隐藏窗口
    if (Config["hide_window"]) {
        HWND hwnd = GetForegroundWindow();
        ShowWindow(hwnd, 0);
    }

    // 检测循环
    DWORD delay = 1000 * Config["scan_interval"];
    while (true) {
        // 延时
        Sleep(delay);
        nlohmann::json channelInfo = getChannelMessage();

        if (channelInfo != NULL) {
            // 处理
            if (!channelInfo.is_null()) {
                if (channelInfo.contains("instruction")) {
                    if (channelInfo["instruction"] == "start") {
                        // 服务器启动中
                        std::ifstream file;
                        file.open(pluginPath + "/channel.json", std::ios::in);
                        time_t now = time(0);
                        if (now - channelInfo["time"] > Config["start_timeout"]) {
                            // 启动超时，重新启动
                            logger.info("Startup timeout, restart...");
                            setStatus2Start();
                            startBDS(serverPath);
                        }
                        logger.info("The server is starting..");

                    }
                    else if (channelInfo["instruction"] == "stop") {
                        // 关闭守护进程
                        logger.info("Stop daemon..");
                        break;
                    }
                    else if (channelInfo["instruction"] == "restart") {
                        // 等待进程结束后重启，若等待时间过长，消灭进程
                        while (true) {
                            Sleep(3000);
                            time_t now = time(0);
                            time_t differ = now - channelInfo["time"];
                            logger.info("Restart: Wait for the server to shut down..(" + std::to_string(differ)
                                + "/" + std::to_string(time_t(Config["close_timeout"])) + ")");
                            if (getBDSStatus(serverPath) == false) {
                                logger.info("Restart: Wakeup server..");
                                setStatus2Start();
                                startBDS(serverPath);
                                break;
                            }
                            else if (differ > Config["restart_timeout"]) {
                                logger.info("Restart: Server shutdown timeout, forcing stop..");
                                setStatus2Start();
                                startBDS(serverPath);
                                break;
                            }
                        }
                    }
                    else if (channelInfo["instruction"] == "tick") {
                        // 获取信息时间戳，超时则重启
                        if (channelInfo.contains("time")) {
                            time_t now = time(0);
                            time_t differ = now - channelInfo["time"];
                            if (differ > Config["timeout"]) {
                                logger.info("Timeout. Wakeup server..");
                                setStatus2Start();
                                startBDS(serverPath);
                            }
                            else {
                                logger.info("The server is running..(" + std::to_string(differ)
                                    + "/" + std::to_string(time_t(Config["timeout"])) + ")");
                            }
                        }
                    }
                    else {
                        logger.error("Unknow instruction.");
                    }
                }
            }
        }
        else {
            logger.info("Channel file not found.");
        }
    }
    return 1;
}