#define _CRT_SECURE_NO_WARNINGS
#include "ScheduleScanner.h"
#include <thread>
#include <algorithm>
#include <Operations.h>
using namespace std::chrono;

ScheduleScanner::ScheduleScanner(int64_t scTime,HANDLE hPipe, std::u16string &lr)
{
	duration<int64_t> dur(scTime);
	scheduledTime = dur;
	pipe = hPipe;
	lr = u"";
	statistics = &lr;
}

void ScheduleScanner::setPath(HANDLE hPipe)
{
	this->path = ReadU16String(hPipe);
}

void ScheduleScanner::doWork()
{
	currTime = duration_cast<seconds>(system_clock::now().time_since_epoch());
	int minute = std::chrono::duration_cast<std::chrono::minutes>(currTime).count();
	int minuteScheduled = std::chrono::duration_cast<std::chrono::minutes>(scheduledTime).count();
	if (minuteScheduled < minute)
	{
		return;
	}
	std::chrono::seconds delta = scheduledTime-currTime;
	std::this_thread::sleep_for(delta);
	std::filesystem::path p(SOLUTION_DIR);
	p += "BaseEditor\\Ghosts.dbs";
	Bases base(p.u16string());
	Scanner sc(base);
	sc.Scan(path);
	Writeint8_t(pipe, OperationResult::SUCCESS);
	*statistics = sc.getStatistics();
	delete this;
	//std::u16string str = sc.getStatistics();
	//statistics += str;
	//finished = true;
	//statistics = sc.getStatistics();
}

// void ScheduleScanner::sendStatistics(HANDLE hPipe)
// {
// 	while (!finished)
// 		Sleep(200);
// 	//HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"StatEvent");
// 	if(finished)
// 	{
// 		//SetEvent(hEvent);
// 		if (statistics.length() > 1024)
// 		{
// 			int numofParts = statistics.length() % 1024 ? statistics.length() / 1024 + 1 : statistics.length() / 1024;
// 			Writeuint32_t(hPipe, numofParts);
// 			for (int i = 0; i < numofParts; i++)
// 			{
// 				std::u16string partToSend = statistics.substr(i * 1024, 1024);
// 				WriteU16String(hPipe, partToSend);
// 			}
// 		}
// 		else
// 		{
// 			Writeint32_t(hPipe, 1);
// 			WriteU16String(hPipe, statistics);
// 		}
// 	}
// 	delete this;
// }
