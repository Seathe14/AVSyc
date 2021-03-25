#pragma once
#include <windows.h>
#include "sample.h"
#include <string>
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
	static void setServiceInstance(Service* Instance);
private:
	static void MainProcess();
	static VOID SvcReportEvent(LPTSTR szFunction);
	static VOID WINAPI SvcCtrlHandler(DWORD dwCtrl);
	VOID ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
	static DWORD WINAPI AcceptMessages(LPVOID lpvParam);
	static DWORD WINAPI WaitForPipe(LPVOID lpvParam);
	VOID startSvc();
	VOID WINAPI DoUpdateSvcDacl(void);
	static inline Service* ServiceInstance;

	LPWSTR ServiceName;
	SERVICE_STATUS gSvcStatus;
	SERVICE_STATUS_HANDLE gSvcStatusHandle;

	void sendStatistics(const std::u16string &reportToSend);
	void launchUI();
	VOID SvcInit();

private:
	HANDLE hPipe;
	LPCTSTR lpszPipeName = TEXT("\\\\.\\pipe\\IPCPipe");

};

