#pragma once
#include "ReadWrite.h"
#include <windows.h>
#include <filesystem>
#include <ctime>
#include <chrono>
#include "Scanner.h"
#include "ScanTaskBase.h"
class ScheduleScanner
{
public:
	ScheduleScanner(int64_t scTime,HANDLE hPipe, std::u16string &lr);
	void setPath(HANDLE hPipe);
	void doWork();
	void sendStatistics(HANDLE hPipe);
	//std::u16string getStatistics() { return statistics; }

private:
	std::filesystem::path path;
	std::chrono::seconds scheduledTime;
	std::chrono::seconds currTime;
	std::u16string* statistics;
	HANDLE pipe;
	bool finished = false;
};

