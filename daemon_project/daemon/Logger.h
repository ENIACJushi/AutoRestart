#pragma once
#include <iostream>
#include <string>
#include<time.h>

using std::string;
using std::cout;
using std::endl;

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

	void head() {
		cout << GetFormatTime() << " [" << name << "] ";
	}
public:
	Logger(){
		name = "Wheat";
	}
	Logger(string name_) {
		name = name_;
	}

	void info(string msg) {
		head();
		cout << (msg) << endl;
	}
	void info(int msg) {
		head();
		cout << msg << endl;
	}
	void error(string msg) {
		head();
		cout << msg << endl;
	}
	void error(int msg) {
		head();
		cout << msg << endl;
	}
};
