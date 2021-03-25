#pragma once
#include "ReadWrite.h"
#include <windows.h>
#include <filesystem>
#include <ctime>
#include <chrono>
#include "Scanner.h"
class ScheduleScanner
{
public:
	ScheduleScanner(int64_t scTime);
	void setPath(const std::filesystem::path& path) { this->path = path; }
	void doWork();
	void sendStatistics(HANDLE hPipe);
	std::u16string getStatistics() { return statistics; }

private:
	std::filesystem::path path;
	std::chrono::seconds scheduledTime;
	std::chrono::seconds currTime;
	std::u16string statistics;
	bool finished = false;
};

