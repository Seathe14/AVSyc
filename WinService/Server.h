#pragma once
#include <windows.h>
#include "ScanPathTask.h"
#include "ScheduleScannerTask.h"
#include "Monitor.h"
#define BUFSIZE 2048

class Server
{
public:
	void launchUI();
	void sendStatistics(const std::u16string& reportToSend, HANDLE pipe);
	void WaitForPipe();
	void AcceptMessages();
	void AcceptPathMessages(ScanPathTask& scPathTask);
	void AcceptScheduleMessages(ScheduleScannerTask& scheduledScanner);
	void AcceptMonitorMessages(Monitor& monitorTask);

private:
	HANDLE hPipe;
	HANDLE hPipeScheduled;
	HANDLE hPipeMonitor;
	LPCTSTR lpszPipeName = TEXT("\\\\.\\pipe\\IPCPipe");
	LPCTSTR lpszScPipeName = TEXT("\\\\.\\pipe\\IPCScheduledPipe");
	LPCTSTR lpszPipeMonitorName = TEXT("\\\\.\\pipe\\IPCMonitorPipe");
};

