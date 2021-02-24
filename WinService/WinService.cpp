#include <iostream>
#include <windows.h>
#include <TCHAR.h>
#include <strsafe.h>
#include <aclapi.h>
#include "sample.h"
#include <string>
#include "Bases.h"
#include "ReadWrite.h"
#pragma comment(lib,"advapi32.lib")

#define BUFSIZE 512
#define SVCNAME TEXT("AVSyc")

SERVICE_STATUS gSvcStatus;
SERVICE_STATUS_HANDLE gSvcStatusHandle;
HANDLE ghSvcStopEvent = NULL;

VOID SvcInstall(void);
VOID WINAPI SvcCtrlHandler(DWORD);
VOID WINAPI SvcMain(DWORD, LPTSTR*);

VOID ReportSvcStatus(DWORD, DWORD, DWORD);
VOID SvcInit(DWORD, LPTSTR*);
VOID SvcReportEvent(LPTSTR);


int __cdecl _tmain(int argc, TCHAR* argv[])
{
	if (lstrcmp(argv[1], TEXT("install")) == 0)
	{
		SvcInstall();
		return 0;
	}
	SERVICE_TABLE_ENTRY DispatchTable[] =
	{
		{SVCNAME,(LPSERVICE_MAIN_FUNCTION)SvcMain},
		{NULL,NULL}
	};
	if (!StartServiceCtrlDispatcher(DispatchTable))
	{
		SvcReportEvent(TEXT("StartServiceCtrlDispatcher"));
	}
}



VOID SvcInstall()
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

VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
	gSvcStatusHandle = RegisterServiceCtrlHandler(SVCNAME, SvcCtrlHandler);
	if (!gSvcStatusHandle)
	{
		SvcReportEvent(TEXT("RegisterServiceCtrlHandler"));
		return;
	}
	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gSvcStatus.dwServiceSpecificExitCode = 0;
	ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
	SvcInit(dwArgc, lpszArgv);
}

DWORD WINAPI AcceptMessages(LPVOID lpvParam)
{
	DWORD cbRead, cbWritten;
	HANDLE hPipe = (HANDLE)lpvParam;
	//TCHAR buf[BUFSIZE] = { 0 };
	UCHAR code;
	//Sleep(5000);
	Bases base(u"C:\\Users\\imynn\\source\\repos\\Antivirus\\BaseEditor\\Ghosts.dbs");
	while (true)
	{
		if (ReadFile(hPipe, &code, sizeof(UCHAR), &cbRead, NULL) != 0)
		{
			switch (code)
			{
			case codes::codes::INT8:
			{
				int8_t check;
				check = Readint8_t(hPipe);
				Writeint8_t(hPipe, check);
				if (check == 5)
				{
					ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
					return 0;
				}
				break;
			}
			case codes::codes::INT16:
			{
				int16_t check;
				check = Readint16_t(hPipe);
				Writeint16_t(hPipe, check);
				break;
			}
			case codes::codes::INT32:
			{
				int32_t check;
				check = Readint32_t(hPipe);
				Writeint32_t(hPipe, check);
				break;
			}
			case codes::codes::INT64:
			{
				int64_t check;
				check = Readint64_t(hPipe);
				Writeint64_t(hPipe, check);
				break;
			}
			case codes::codes::UINT8:
			{
				uint8_t check;
				check = Readuint8_t(hPipe);
				Writeuint8_t(hPipe, check);
				break;
			}
			case codes::codes::UINT16:
			{
				uint16_t check;
				check = Readuint16_t(hPipe);
				Writeuint16_t(hPipe, check);
				break;
			}
			case codes::codes::UINT32:
			{
				uint32_t check;
				check = Readuint32_t(hPipe);
				Writeuint32_t(hPipe, check);
				break;
			}
			case codes::codes::UINT64:
			{
				uint64_t check;
				check = Readuint64_t(hPipe);
				Writeuint64_t(hPipe, check);
				break;
			}
			case codes::codes::PATH:
			{
				TCHAR buf[MAX_PATH] = { 0 };
				ReadPath(hPipe, buf);
				WritePath(hPipe, buf);
				for (int i = 0; i < MAX_PATH; i++)
					buf[i] = 0;
			}
			}

	
		}
		else
		{
			DisconnectNamedPipe(hPipe);
			ConnectNamedPipe(hPipe, NULL);
		}
	}
}
VOID SvcInit(DWORD dwArgc, LPTSTR* lpszArgv)
{
	// Declare variables

	ghSvcStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (ghSvcStopEvent == NULL)
	{
		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}
	ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
	// TO_DO Perform work
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	LPCTSTR lpszPipeName = TEXT("\\\\.\\pipe\\IPCPipe");
	BOOL fConnected;
	//Sleep(5000);
	SECURITY_DESCRIPTOR sd = { 0, };
	SECURITY_ATTRIBUTES sa = { sizeof(sa), };
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, true, NULL, false);
	sa.lpSecurityDescriptor = &sd;
	hPipe = CreateNamedPipe(lpszPipeName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, BUFSIZE, BUFSIZE, 0, &sa);
	ConnectNamedPipe(hPipe, NULL);
	CreateThread(NULL, 0, AcceptMessages, (LPVOID)hPipe, 0, 0);
	
}

VOID ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
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

VOID WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
	switch (dwCtrl)
	{
	case SERVICE_CONTROL_STOP:
		ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
		SetEvent(ghSvcStopEvent);

		ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);
		return;
	case SERVICE_CONTROL_INTERROGATE:
		break;
	default:
		break;
	}
}

VOID SvcReportEvent(LPTSTR szFunction)
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
