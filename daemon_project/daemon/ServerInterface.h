#pragma once
#include <iostream>
#include <string>
#include "Nlohmann/json.hpp"
#include "Tools.h"
#include <map>

using std::string;
using std::map;

namespace Message {
	const int amount = 8;
	enum Type {
		unknown = 0, // 未知，将文件内容置空("null")
		// 双向
		stop = 1, // 停服：停止服务器，守护进程也会停止

		// 守护进程→服务端
		stop_delay = 2, // 带延迟的停服：停止服务器，守护进程也会停止
		restart = 3, // 重启：停止服务器，守护进程不会停止
		restart_delay = 4, // 带延迟的重启：停止服务器，守护进程不会停止
		message = 5,  // 发送全服信息
		
		// 服务端→守护进程
		disable = 6, // 禁用重启：停止守护进程
		restart_task = 7, // 创建重启任务，到时间自动发送给服务端执行
	};
	const string Name[] = {
		"unknown",
		"stop",
		"stop_delay",
		"restart",
		"restart_delay",
		"message",
		"disable",
		"restart_task"
	};
	
	struct Message
	{
		Message(Type t, nlohmann::json e=NULL) {
			type = t;
			extra = e;
		};
		Type type;
		nlohmann::json extra;
	};

	// 处理函数
	string type2name(Type t) {
		return Name[t];
	}
	Type name2type(string n) {
		for (int i = 0; i < amount; i++) {
			if (Name[i] == n) {
				return Type(i);
			}
		}
		return Type::unknown;
	}
	Message fromJson(nlohmann::json jsonMessage) {
		if (jsonMessage["type"] != NULL) {
			return Message(name2type(jsonMessage["type"]), jsonMessage["extra"]);
		}
		return Message(Type::unknown, NULL);
	}
	nlohmann::json toJson(Message msg) {
		return nlohmann::json{
			{"type", type2name(msg.type)},
			{ "extra", msg.extra }
		};
	}
}

class ServerInterface
{
private:
	string serverPath; // basePath + "\\bedrock_server_mod.exe"
	string pluginPath;
public:
	ServerInterface(string basePath) {
		serverPath = basePath + "\\bedrock_server_mod.exe";
		pluginPath = basePath + "\\plugins\\AutoRestart";
	}

	// 向服务器发送信息
	bool sendMessage(Message::Message msg) {
		std::fstream newFile;
		newFile.open(pluginPath + "/channel.json", std::fstream::in | std::fstream::out | std::fstream::trunc);
		if (msg.type == Message::Type::unknown) {
			newFile << "null";
		}
		else {
			newFile << Message::toJson(msg).dump();
		}
		newFile.close();
		return true;
	}
	
	// 获取服务器发送的信息
	Message::Message getMessage() {
		std::ifstream file;
		file.open(pluginPath + "/channel.json", std::ios::in);
		if (file) {
			std::istreambuf_iterator<char> beg(file), end;
			string configString = std::string(beg, end);
			file.seekg(0, std::ios::end); //移动到文件尾部
			file.close();
			if (configString != "null") {
				// 转为json
				nlohmann::json channelInfo;
				try {
					channelInfo = nlohmann::json::parse(configString.c_str(), nullptr, true);
					return Message::fromJson(channelInfo);
				}
				catch (const std::exception& ex) {
					logger.error(ex.what());
				}
			}
		}
		return Message::Message(Message::Type::unknown, NULL);
	}
	
	// 发送已收到信息的信息（置空）
	bool sendMessageRecieved() {
		return sendMessage(Message::Message(Message::Type::unknown));
	}

	// 获取服务器运行状态, 正在运行为true
	bool getStatus() {
		return getBDSStatus(serverPath);
	}

	// 强制关闭服务器
	// bool close() {}
	
	// 启动服务器，若已有服务器运行则不会启动并返回false
	bool start() {
		if (getStatus()) {
			return false;
		}
		else {
			startBDS(serverPath);
			return true;
		} 
	}

};

