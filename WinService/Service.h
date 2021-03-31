#pragma once
#include <windows.h>
#include "sample.h"
#include <string>
#include "ScanPathTask.h"
#include "ScheduleScannerTask.h"
#include "Monitor.h"
#include "Server.h"
#ifndef NDEBUG
#define SVCNAME TEXT("AVSycDbg")
#else
#define SVCNAME TEXT("AVSyc")
#endif // DEBUG

class Service
{
public:
	Service();
	static void ServiceProcess(int argc, TCHAR* argv[]);
	void SvcInstall();
	void SvcUninstall();
	static void setServiceInstance(Service* Instance);
private:
	static void MainProcess();
	static VOID SvcReportEvent(LPTSTR szFunction);
	static VOID WINAPI SvcCtrlHandler(DWORD dwCtrl);
	VOID ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
	static DWORD WINAPI AcceptMessages(LPVOID lpvParam);
	//static DWORD WINAPI AcceptPathMessages(LPVOID lpvParam);
	static DWORD WINAPI WaitForPipe(LPVOID lpvParam);
	VOID startSvc();
	VOID WINAPI DoUpdateSvcDacl(void);
	static inline Service* ServiceInstance;

	LPWSTR ServiceName;
	SERVICE_STATUS gSvcStatus;
	SERVICE_STATUS_HANDLE gSvcStatusHandle;

	void sendStatistics(const std::u16string &reportToSend);
	void AcceptPathMessages(ScanPathTask& scPathTask);
	void AcceptScheduleMessages(ScheduleScannerTask& scheduledScanner);
	void AcceptMonitorMessages(Monitor& monitorTask);
	void createRegistryRecord();
	void setWorkingDirectory();
	void deleteRegistryRecord();
	void launchUI();
	VOID SvcInit();

private:
	static inline Server serv;
	HANDLE hPipe;
	HANDLE hPipeScheduled;
	HANDLE hPipeOper;
	LPCTSTR lpszPipeName = TEXT("\\\\.\\pipe\\IPCPipe");
	LPCTSTR lpszScPipeName = TEXT("\\\\.\\pipe\\IPCScheduledPipe");
	LPCTSTR lpszPipeOperName = TEXT("\\\\.\\pipe\\IPCOperPipe");
	TCHAR WorkingDirectory[MAX_PATH];

};

