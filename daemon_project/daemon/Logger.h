#pragma once
#include <iostream>
#include <string>
#include <time.h>
#include <fstream>

using std::string;
using std::cout;
using std::endl;
using std::ofstream;

class Logger
{
private:
	string name;
	string GetFormatTime()
	{
		time_t currentTime;
		time(&currentTime);
		tm* t_tm = new tm();
		localtime_s(t_tm, &currentTime);

		char formatTime[64] = { 0 };
		snprintf(formatTime, 64, "%04d-%02d-%02d %02d:%02d:%02d",
			t_tm->tm_year + 1900,
			t_tm->tm_mon + 1,
			t_tm->tm_mday,
			t_tm->tm_hour,
			t_tm->tm_min,
			t_tm->tm_sec);
		return std::string(formatTime);
	}
	string GetFileName()
	{
		time_t currentTime;
		time(&currentTime);
		tm* t_tm = new tm();
		localtime_s(t_tm, &currentTime);

		char formatTime[64] = { 0 };
		snprintf(formatTime, 64, "%04d-%02d-%02d.txt",
			t_tm->tm_year + 1900,
			t_tm->tm_mon + 1,
			t_tm->tm_mday);
		return std::string(formatTime);
	}
	string addHead(string msg) {
		return GetFormatTime() + " [" + name + "] " + msg;
	}
public:
	Logger(){
		name = "Wheat";
	}
	Logger(string name_) {
		name = name_;
	}
	void writeFile(string msg) {
		if (_access("logs/AutoRestart", 0) == -1) {
			info("Log path not exist, create..", false);
			if (_mkdir("logs/AutoRestart") == -1) {
				error("Dir \"logs/AutoRestart\" make failed.", false);
			}
			else {
				info("Create successfully.", false);
			}
		}
		ofstream ofileAgain;
		ofileAgain.open("logs/AutoRestart/" + GetFileName(), std::ios::app);
		ofileAgain << msg << endl;
		ofileAgain.close();
	}
	void info(string msg, bool write=true) {
		string newMsg = addHead(msg);
		cout << newMsg << endl;
		if (write) writeFile(newMsg);
	}
	void info(int msg, bool write = true) {
		string newMsg = addHead(std::to_string(msg));
		cout << newMsg << endl;
		if (write) writeFile(newMsg);
	}
	void error(string msg, bool write = true) {
		string newMsg = "Error " + addHead(msg);
		cout << newMsg << endl;
		if (write) writeFile(newMsg);
	}
	void error(int msg, bool write = true) {
		string newMsg = "Error " + addHead(std::to_string(msg));
		cout << newMsg << endl;
		if (write) writeFile(newMsg);
	}
};
