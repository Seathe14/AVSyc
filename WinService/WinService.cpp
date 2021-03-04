#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <windows.h>
#include <TCHAR.h>
#include <strsafe.h>
#include <aclapi.h>
#include "sample.h"
#include <string>
#include "Bases.h"
#include "Scanner.h"
#include <userenv.h>
#include <fstream>
#include "ReadWrite.h"
#include <TlHelp32.h>
#include <WtsApi32.h>
#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"wtsapi32.lib")
#pragma comment(lib,"userenv.lib")
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
	UCHAR code;
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
				if (check == 6)
				{
					base.updateBase(u"C:\\Users\\imynn\\source\\repos\\Antivirus\\BaseEditor\\Ghosts.dbs");
				}
				if (check == 7)
				{

					
				}
				check = 0;

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
				Scanner sc(base);
				
				sc.Scan(buf);
				std::u16string report = sc.getStatistics();
				WriteU16String(hPipe, report);

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
DWORD getProcID(const wchar_t* procName)
{
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 procEntry = {};
	procEntry.dwSize = sizeof(procEntry);
	Process32First(hSnap, &procEntry);
	DWORD procID;
	while (Process32Next(hSnap, &procEntry))
	{
		if (!wcscmp(procEntry.szExeFile, procName))
		{
			procID = procEntry.th32ProcessID;
		}
	}
	CloseHandle(hSnap);
	return procID;
}
DWORD WINAPI WaitForPipe(LPVOID lpvParam)
{
	HANDLE* hPipe = (HANDLE*)lpvParam;
	LPCTSTR lpszPipeName = TEXT("\\\\.\\pipe\\IPCPipe");
	SECURITY_DESCRIPTOR sd = { 0, };
	SECURITY_ATTRIBUTES sa = { sizeof(sa), };
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, true, NULL, false);
	sa.lpSecurityDescriptor = &sd;
	*hPipe = CreateNamedPipe(lpszPipeName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, BUFSIZE, BUFSIZE, 0, &sa);
	ConnectNamedPipe(*hPipe, NULL);
	hPipe = NULL;
	return 0;
}
void launchUI()
{
#if _DEBUG
	wchar_t path[MAX_PATH] = L"C:\\Users\\imynn\\source\\repos\\Antivirus\\AVSyc\\debug\\AVSyc.exe";
#else 
	wchar_t path[MAX_PATH] = L"C:\\Users\\imynn\\source\\repos\\Antivirus\\AVSyc\\release\\AVSyc.exe";
#endif
	STARTUPINFO si = { 0 };
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi = { 0 };
	ZeroMemory(&pi, sizeof(pi));
	HANDLE token;
	HANDLE newToken;
	WTSQueryUserToken(WTSGetActiveConsoleSessionId(), &token);
	TOKEN_LINKED_TOKEN tlt;
	DuplicateTokenEx(token, TOKEN_ALL_ACCESS, NULL, SecurityIdentification, TokenPrimary, &newToken);
	SetTokenInformation(newToken, TokenLinkedToken, &tlt, sizeof(tlt));
	LPVOID penv = 0;
	CreateEnvironmentBlock(&penv, newToken, FALSE);
	CreateProcessAsUserW(newToken, path, path, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, penv, NULL, &si, &pi);
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
	HANDLE threadPipe;
	threadPipe = CreateThread(NULL, 0, WaitForPipe, &hPipe, 0, NULL);
	launchUI();
	WaitForSingleObject(threadPipe, INFINITE);
	CreateThread(NULL, 0, AcceptMessages, (LPVOID)hPipe, 0, NULL);

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
