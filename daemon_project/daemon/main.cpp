#include <iostream>
#include <Windows.h>
#include <thread>
#include <vector>
#include <time.h>
#include "Nlohmann/json.hpp"

#include "ServerInterface.h"
#include "Config.h"
#include "Tools.h"

#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )

const string basePath = wstring2string(GetProgramDir());
const string deamonPath = basePath + "\\AutoRestart.exe";
const string pluginPath = basePath + "\\plugins\\AutoRestart";
ServerInterface server(basePath);

int taskTime = -1; // 距离重启任务的时间（秒） 约定为-1时，不执行任务
const int task_delay = 30; // 发送重启信息提前的时间（秒）
// 每30秒更新一次状态
bool restartTask() {
    while (true) {
        Sleep(30000);
        if (taskTime == -1) continue;
        
        taskTime -= task_delay;
        if (taskTime <= 0) {
            taskTime = -1;
            logger.info("Running restart task..");
            server.sendMessage(Message::Message(Message::Type::restart_delay, task_delay));
        }
    }
}

int main(int argc, char* argv[])
{
    logger.writeFile("=======================================");
    
    /// 参数启动 | none; start; server
    bool start = false;
    for (int argi = 0; argi < argc; argi++) {
        // server | 若被服务器调用，则启动一个新进程，关闭当前进程
        if (strcmp(argv[argi], "--server") == 0) {
            logger.info("Initiated by a server, start a new process and quit..");
            ShellExecuteW(NULL, L"open", stringToLPCWSTR(deamonPath), stringToLPCWSTR("--start"), stringToLPCWSTR(basePath), SW_SHOW);
            return 0;
        }
        // start | 若以start参数启动，则正常开始运行
        else if (strcmp(argv[argi], "--start") == 0) {
            start = true;
        }
    }
    // none | 若以无参数模式启动，则关闭同路径下的监控进程，然后关闭自己
    // 用于服务器无法正常启动但监控进程仍然在运行，导致无限重启的情况
    if (!start) {
        logger.info("Start with no argument, close the other daemons, and then the current process.");
        closeRunningDaemon(deamonPath);
        return 0;
    }

    /// 加载配置
    // system("chcp 65001");// TODO: 看看删了之后还会不会弹窗
    logger.info("Loading config..");
    if (!loadConfig(pluginPath, "Config.json")) {
        logger.error("Failed to load config.");
        system("pause");
        return 0;
    }
    
    /// 关闭其它守护进程
    closeRunningDaemon(deamonPath);

    /// 启动服务器
    
    {
        if (server.getStatus()) {
            logger.info("The server is already running.");
        }
        else {
            logger.info("Starting server..");
            server.start();
            while (!server.getStatus()) {
                Sleep(100);
            }
            logger.info("Server started.");
        }
    }

    /// TODO: 启动任务线程
    

    /// 检测循环
    std::thread t1(restartTask);
    DWORD delay = 1000 * Config["scan_interval"];
    while (true) {
        // 延时
        Sleep(delay);

        // 处理消息
        Message::Message msg = server.getMessage();
        bool stop = false;
        switch (msg.type) {
            case Message::Type::stop:
            case Message::Type::disable:
                logger.info("Stop daemon..");
                server.sendMessageRecieved();
                stop = true;
                break;
            case Message::Type::restart_task:
                taskTime = msg.extra - task_delay;
                server.sendMessageRecieved();
                break;
            default:
                break;
        }
        if (stop) break;

        // 监视状态
        if (!server.getStatus()) {
            logger.info("Server closed, waking up..");
            server.start();
        }
    }
    t1.detach();
    return 1;
}