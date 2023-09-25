#pragma once
#include <fstream>
#include <direct.h>
#include <string>
#include <cstring>
#include <io.h>
#include "Nlohmann/json.hpp"
#include "Logger.h"

Logger logger("Wheat", false);
nlohmann::json Config;
const nlohmann::json defaultConfig = nlohmann::json{
    {"scan_interval", 15}
};

bool loadConfig(string folderPath, string fileName) {
    if (_access(folderPath.c_str(), 0) == -1) {
        logger.info("Plugin path not exist, create..");
        if (_mkdir(folderPath.c_str()) == -1) {
            logger.error("Dir \"" + folderPath + "\" make failed.");
        }
        else {
            logger.info("Create successfully.");
        }
    }

    // load Config.json
    std::fstream file;
    file.open(folderPath + "/" + fileName, std::ios::in);
    // 不存在，使用默认配置（不创建）
    if (!file){
        logger.info("Config file not exist, using default config..");
        Config = defaultConfig;
        return true;
    }
    else {
        std::istreambuf_iterator<char> beg(file), end;
        string configString = std::string(beg, end);
        file.seekg(0, std::ios::end); //移动到文件尾部
        file.close();
        try {
            Config = nlohmann::json::parse(configString.c_str(), nullptr, true);
            // logger.info(Config.dump(1));
        }
        catch (const std::exception& ex) {
            logger.error(ex.what());
            return false;
        }
        return true;
    }
    return true;
}
