#pragma once
#include <Windows.h>
#include <vector>
#include "Config.h"
#include "Tools.h"
struct Server {
    Server(string n, string p, bool e) {
        enable = e;
        name = n;
        path = p;
        life = 2;
    }
    bool enable;
    string name;
    string path;
    int life;
};

class Daemon
{    
    std::vector<Server> serverList;
    int timeout;
    int timeLeft;
    int startTime;
public:
    Daemon() {
        timeout = 1;
        timeLeft = 1;
        startTime = 60;
    }

    void initiate(nlohmann::json serverListJson, int timeout_, int startTime_) {
        for (int i = 0; i < serverListJson.size(); i++) {
            serverList.push_back(Server(serverListJson[i]["name"], serverListJson[i]["path"], serverListJson[i]["enable"]));
        }
        timeout = timeout_;
        timeLeft = timeout;
        startTime = startTime_;
    }

    void tick() {
        for (int i = 0; i < serverList.size(); i++) {
            if (!getStatus(serverList[i].path)) {
                wakeUp(serverList[i].path);
            }
        }
    }

    void wakeUp(string path) {
        startBDS(path);
    }

    bool getStatus(string path) {
        return getBDSStatus(path);
    }
};
