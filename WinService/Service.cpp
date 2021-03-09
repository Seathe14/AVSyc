#include "Service.h"
#include <iostream>
#include <strsafe.h>
#include <filesystem>
#include <WtsApi32.h>
#include <userenv.h>
#include "Scanner.h"
#include <ReadWrite.h>
#pragma comment(lib,"wtsapi32.lib")
#pragma comment(lib,"userenv.lib")
#define BUFSIZE 512
Service::Service()
{

}

void Service::ServiceProcess(int argc, TCHAR* argv[])
{
	if (lstrcmp(argv[1], TEXT("install")) == 0)
	{
		ServiceInstance->SvcInstall();
	}
	SERVICE_TABLE_ENTRY DispatchTable[] =
	{
		{SVCNAME,(LPSERVICE_MAIN_FUNCTION)Service::MainProcess},
		{NULL,NULL}
	};
	if (!StartServiceCtrlDispatcher(DispatchTable))
	{
		SvcReportEvent(TEXT("StartServiceCtrlDispatcher"));
	}
}

void Service::SvcInstall()
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	TCHAR szPath[MAX_PATH];
	if (!GetModuleFileName(NULL, szPath, MAX_PATH))
	{
		printf("Cannot install service (%d)\n", GetLastError());
		return;
	}
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	schService = CreateService(schSCManager, SVCNAME, SVCNAME,
		SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL, szPath, NULL, NULL, NULL, NULL, NULL);
	if (schService == NULL)
	{
		printf("CreateService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}
	else printf("Service installed successfully\n");
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

void Service::setServiceInstance(Service* Instance)
{
	if (Instance)
		ServiceInstance = Instance;
}

void Service::MainProcess()
{
	ServiceInstance->gSvcStatusHandle = RegisterServiceCtrlHandler(SVCNAME, (LPHANDLER_FUNCTION)Service::SvcCtrlHandler);
	if (!ServiceInstance->gSvcStatusHandle)
	{
		SvcReportEvent(TEXT("RegisterServiceCtrlHandler"));
		return;
	}
	ServiceInstance->gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ServiceInstance->gSvcStatus.dwServiceSpecificExitCode = 0;
	SetServiceStatus(ServiceInstance->gSvcStatusHandle, &(ServiceInstance->gSvcStatus));
	ServiceInstance->ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
	ServiceInstance->SvcInit();
}

VOID Service::SvcReportEvent(LPTSTR szFunction)
{
	HANDLE hEventSource;
	LPCTSTR lpszStrings[2];
	TCHAR buffer[80];
	hEventSource = RegisterEventSource(NULL, SVCNAME);
	if (NULL != hEventSource)
	{
		StringCchPrintf(buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());
		lpszStrings[0] = SVCNAME;
		lpszStrings[1] = buffer;
		ReportEvent(hEventSource, EVENTLOG_ERROR_TYPE, 0, SVC_ERROR, NULL, 2, 0, lpszStrings, NULL);
		DeregisterEventSource(hEventSource);
	}
}

VOID WINAPI Service::SvcCtrlHandler(DWORD dwCtrl)
{
	switch (dwCtrl)
	{
	case SERVICE_CONTROL_STOP:
		ServiceInstance->ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
		//SetEvent(ghSvcStopEvent);
		ServiceInstance->gSvcStatus.dwWin32ExitCode = 0;
		ServiceInstance->gSvcStatus.dwCurrentState = SERVICE_STOPPED;
		ServiceInstance->ReportSvcStatus(ServiceInstance->gSvcStatus.dwCurrentState, NO_ERROR, 0);
		break;;
	case SERVICE_CONTROL_INTERROGATE:
		break;
	default:
		break;
	}
}

VOID Service::ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;
	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		gSvcStatus.dwControlsAccepted = 0;
	else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED))
		gSvcStatus.dwCheckPoint = 0;
	else gSvcStatus.dwCheckPoint = dwCheckPoint++;
	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

void Service::launchUI()
{
	std::filesystem::path p1(SOLUTION_DIR);
#if _DEBUG
	p1 += "AVSyc\\release\\AVSyc.exe";
	std::wstring path = p1.wstring();
#else 
	p1 += "AVSyc\\release\\AVSyc.exe";
	std::wstring path = p1.wstring();
#endif
	STARTUPINFO si = { 0 };
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi = { 0 };
	ZeroMemory(&pi, sizeof(pi));
	HANDLE token;
	WTSQueryUserToken(WTSGetActiveConsoleSessionId(), &token);
	LPVOID penv = 0;
	CreateEnvironmentBlock(&penv, token, FALSE);
	CreateProcessAsUserW(token, path.c_str(), (wchar_t*)path.c_str(), NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, penv, NULL, &si, &pi);
}

DWORD WINAPI Service::AcceptMessages(LPVOID lpvParam)
{
	DWORD cbRead, cbWritten;
	UCHAR code;
	Bases base(u"C:\\Users\\imynn\\source\\repos\\Antivirus\\BaseEditor\\Ghosts.dbs");
	while (true)
	{
		if (ReadFile(ServiceInstance->hPipe, &code, sizeof(UCHAR), &cbRead, NULL) != 0)
		{
			switch (code)
			{
			case codes::codes::INT8:
			{
				int8_t check;
				check = Readint8_t(ServiceInstance->hPipe);
				Writeint8_t(ServiceInstance->hPipe, check);
				if (check == 5)
				{
					ServiceInstance->ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
					return 0;
				}
				if (check == 6)
				{
					base.updateBase(u"C:\\Users\\imynn\\source\\repos\\Antivirus\\BaseEditor\\Ghosts.dbs");
				}
				break;
			}
			case codes::codes::INT16:
			{
				int16_t check;
				check = Readint16_t(ServiceInstance->hPipe);
				Writeint16_t(ServiceInstance->hPipe, check);
				break;
			}
			case codes::codes::INT32:
			{
				int32_t check;
				check = Readint32_t(ServiceInstance->hPipe);
				Writeint32_t(ServiceInstance->hPipe, check);
				break;
			}
			case codes::codes::INT64:
			{
				int64_t check;
				check = Readint64_t(ServiceInstance->hPipe);
				Writeint64_t(ServiceInstance->hPipe, check);
				break;
			}
			case codes::codes::UINT8:
			{
				uint8_t check;
				check = Readuint8_t(ServiceInstance->hPipe);
				Writeuint8_t(ServiceInstance->hPipe, check);
				break;
			}
			case codes::codes::UINT16:
			{
				uint16_t check;
				check = Readuint16_t(ServiceInstance->hPipe);
				Writeuint16_t(ServiceInstance->hPipe, check);
				break;
			}
			case codes::codes::UINT32:
			{
				uint32_t check;
				check = Readuint32_t(ServiceInstance->hPipe);
				Writeuint32_t(ServiceInstance->hPipe, check);
				break;
			}
			case codes::codes::UINT64:
			{
				uint64_t check;
				check = Readuint64_t(ServiceInstance->hPipe);
				Writeuint64_t(ServiceInstance->hPipe, check);
				break;
			}
			case codes::codes::PATH:
			{

				TCHAR buf[MAX_PATH] = { 0 };
				ReadPath(ServiceInstance->hPipe, buf);
				Scanner sc(base);

				sc.Scan(buf);
				std::u16string report = sc.getStatistics();
				WriteU16String(ServiceInstance->hPipe, report);

			}
			}

		}
		else
		{
			DisconnectNamedPipe(ServiceInstance->hPipe);
			ConnectNamedPipe(ServiceInstance->hPipe, NULL);
		}
	}
}
DWORD WINAPI Service::WaitForPipe(LPVOID lpvParam)
{
	SECURITY_DESCRIPTOR sd = { 0, };
	SECURITY_ATTRIBUTES sa = { sizeof(sa), };
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, true, NULL, false);
	sa.lpSecurityDescriptor = &sd;
	ServiceInstance->hPipe = CreateNamedPipe(ServiceInstance->lpszPipeName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, BUFSIZE, BUFSIZE, 0, &sa);
	ConnectNamedPipe(ServiceInstance->hPipe, NULL);
	return 0;
}
VOID Service::SvcInit()
{
	ServiceInstance->ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
	// TO_DO Perform work

	HANDLE threadPipe;
	threadPipe = CreateThread(NULL, 0, WaitForPipe, NULL, 0, NULL);
	ServiceInstance->launchUI();
	WaitForSingleObject(threadPipe, INFINITE);
	CreateThread(NULL, 0, AcceptMessages, (LPVOID)hPipe, 0, NULL);
}
