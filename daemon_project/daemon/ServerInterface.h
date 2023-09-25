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
		unknown = 0, // δ֪�����ļ������ÿ�("null")
		// ˫��
		stop = 1, // ͣ����ֹͣ���������ػ�����Ҳ��ֹͣ

		// �ػ����̡������
		stop_delay = 2, // ���ӳٵ�ͣ����ֹͣ���������ػ�����Ҳ��ֹͣ
		restart = 3, // ������ֹͣ���������ػ����̲���ֹͣ
		restart_delay = 4, // ���ӳٵ�������ֹͣ���������ػ����̲���ֹͣ
		message = 5,  // ����ȫ����Ϣ
		
		// ����ˡ��ػ�����
		disable = 6, // ����������ֹͣ�ػ�����
		restart_task = 7, // �����������񣬵�ʱ���Զ����͸������ִ��
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

	// ������
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

	// �������������Ϣ
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
	
	// ��ȡ���������͵���Ϣ
	Message::Message getMessage() {
		std::ifstream file;
		file.open(pluginPath + "/channel.json", std::ios::in);
		if (file) {
			std::istreambuf_iterator<char> beg(file), end;
			string configString = std::string(beg, end);
			file.seekg(0, std::ios::end); //�ƶ����ļ�β��
			file.close();
			if (configString != "null") {
				// תΪjson
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
	
	// �������յ���Ϣ����Ϣ���ÿգ�
	bool sendMessageRecieved() {
		return sendMessage(Message::Message(Message::Type::unknown));
	}

	// ��ȡ����������״̬, ��������Ϊtrue
	bool getStatus() {
		return getBDSStatus(serverPath);
	}

	// ǿ�ƹرշ�����
	// bool close() {}
	
	// �����������������з����������򲻻�����������false
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

