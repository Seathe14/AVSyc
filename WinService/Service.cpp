#include "Service.h"
#include <iostream>
#include <strsafe.h>
#include <filesystem>
#include <WtsApi32.h>
#include <userenv.h>
#include "Scanner.h"
#include "ScheduleScanner.h"
#include <future>
#include <aclapi.h>
#include <ReadWrite.h>
#include <Operations.h>
#include <thread>
#pragma comment(lib,"wtsapi32.lib")
#pragma comment(lib,"userenv.lib")
#define BUFSIZE 2048
Service::Service()
{

}

void Service::ServiceProcess(int argc, TCHAR* argv[])
{
	if (lstrcmp(argv[1], TEXT("install")) == 0)
	{
		ServiceInstance->SvcInstall();
	}
	if (lstrcmp(argv[1], TEXT("update")) == 0)
	{
		ServiceInstance->DoUpdateSvcDacl();
	}
	SERVICE_TABLE_ENTRY DispatchTable[] =
	{
		{SVCNAME,(LPSERVICE_MAIN_FUNCTION)Service::MainProcess},
		{NULL,NULL}
	};
	ServiceInstance->startSvc();
	if (!StartServiceCtrlDispatcher(DispatchTable))
	{
		SvcReportEvent(TEXT("StartServiceCtrlDispatcher"));
	}
	//SC_HANDLE schSCManager;
	//SC_HANDLE schService;
	//schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	//schService = OpenService(schSCManager, SVCNAME, SERVICE_ALL_ACCESS);
	//StartServiceA(schService, NULL, NULL);
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
	//ServiceInstance->startSvc();
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
	case CTRL_CLOSE_EVENT:
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

void Service::sendStatistics(const std::u16string &reportToSend)
{
	if (reportToSend.length() > 1024)
	{
		int numofParts = reportToSend.length() % 1024 ? reportToSend.length() / 1024 + 1 : reportToSend.length() / 1024;
		Writeuint32_t(ServiceInstance->hPipe, numofParts);
		for (int i = 0; i < numofParts; i++)
		{
			std::u16string partToSend = reportToSend.substr(i * 1024, 1024);
			WriteU16String(ServiceInstance->hPipe, partToSend);
		}
	}
	else
	{
		Writeint32_t(ServiceInstance->hPipe, 1);
		WriteU16String(ServiceInstance->hPipe, reportToSend);
	}
}

void Service::launchUI()
{
	std::filesystem::path p1(SOLUTION_DIR);
#if _DEBUG
	p1 += "AVSyc\\debug\\AVSyc.exe";
	std::wstring path = p1.wstring();
#else 
	p1 += "AVSyc\\release\\AVSyc.exe";
	std::wstring path = p1.wstring();
#endif
	STARTUPINFO si = { 0 };
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.lpDesktop = L"winsta0\\default";
	PROCESS_INFORMATION pi = { 0 };
	ZeroMemory(&pi, sizeof(pi));
	HANDLE token;
	WTSQueryUserToken(WTSGetActiveConsoleSessionId(), &token);
	LPVOID penv = 0;
	CreateEnvironmentBlock(&penv, token, TRUE);
	CreateProcessAsUserW(token, path.c_str(), NULL, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT, penv, NULL, &si, &pi);
}

DWORD WINAPI Service::AcceptMessages(LPVOID lpvParam)
{
	//HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, L"StatEvent");
	DWORD cbRead, cbWritten;
	UCHAR code;
	std::filesystem::path p(SOLUTION_DIR);
	p += "BaseEditor\\Ghosts.dbs";
	Bases base(p.u16string());
	static std::u16string lastResult;
	std::mutex mtx;

	while (true)
	{
		if (ReadFile(ServiceInstance->hPipeOper, &code, sizeof(UCHAR), &cbRead, NULL) != 0)
		{
			switch (code)
			{
			case Operation::SCANSCHEDULED:
			{

				int64_t time = Readint64_t(ServiceInstance->hPipeScheduled);
				ScheduleScanner* scScanner = new ScheduleScanner(time,ServiceInstance->hPipeScheduled,lastResult);
				//std::filesystem::path path;
				scScanner->setPath(ServiceInstance->hPipeScheduled);
				std::thread t1(&ScheduleScanner::doWork, scScanner);
				t1.detach();
				break;
			}
			case Operation::SCANPATH:
			{
				std::filesystem::path path;
				Scanner sc(base);
				path = ReadU16String(ServiceInstance->hPipe);
				sc.Scan(path);
				Writeint8_t(ServiceInstance->hPipe, OperationResult::SUCCESS);
				lastResult = sc.getStatistics();
				break;
			}
			case Operation::GET_STATISTICS:
				mtx.lock();
				if (lastResult.length() > 1024)
				{
					int numofParts = lastResult.length() % 1024 ? lastResult.length() / 1024 + 1 : lastResult.length() / 1024;
					Writeuint32_t(ServiceInstance->hPipeOper, numofParts);
					for (int i = 0; i < numofParts; i++)
					{
						std::u16string partToSend = lastResult.substr(i * 1024, 1024);
						WriteU16String(ServiceInstance->hPipeOper, partToSend);
					}
				}
				else
				{
					Writeint32_t(ServiceInstance->hPipeOper, 1);
					WriteU16String(ServiceInstance->hPipeOper, lastResult);
				}
				mtx.unlock();
				//sendStatistics(lastResult);
				//WriteU16String(ServiceInstance->hPipe, lastResult);
				break;
			}

		}
		else
		{
			DisconnectNamedPipe(ServiceInstance->hPipe);
			DisconnectNamedPipe(ServiceInstance->hPipeScheduled);
			DisconnectNamedPipe(ServiceInstance->hPipeOper);
			ConnectNamedPipe(ServiceInstance->hPipeOper, NULL);
			ConnectNamedPipe(ServiceInstance->hPipe, NULL);
			ConnectNamedPipe(ServiceInstance->hPipeScheduled, NULL);
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
	ServiceInstance->hPipeScheduled = CreateNamedPipe(ServiceInstance->lpszScPipeName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, BUFSIZE, BUFSIZE, 0, &sa);
	ConnectNamedPipe(ServiceInstance->hPipeScheduled, NULL);
	ServiceInstance->hPipeOper = CreateNamedPipe(ServiceInstance->lpszPipeOperName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, BUFSIZE, BUFSIZE, 0, &sa);
	ConnectNamedPipe(ServiceInstance->hPipeOper, NULL);
	return 0;
}

VOID Service::startSvc()
{
	SERVICE_STATUS_PROCESS ssStatus;
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	schService = OpenService(schSCManager, SVCNAME, SERVICE_ALL_ACCESS);


	StartServiceA(schService, NULL, NULL);

	CloseServiceHandle(schSCManager);
	CloseServiceHandle(schService);

}

VOID Service::SvcInit()
{
	ServiceInstance->ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
	// TO_DO Perform work
	ServiceInstance->launchUI();
	HANDLE threadPipe;
	threadPipe = CreateThread(NULL, 0, WaitForPipe, NULL, 0, NULL);
	WaitForSingleObject(threadPipe, INFINITE);
	CreateThread(NULL, 0, AcceptMessages, (LPVOID)hPipe, 0, NULL);
}



VOID WINAPI Service::DoUpdateSvcDacl(void)
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;

	EXPLICIT_ACCESS      ea;
	SECURITY_DESCRIPTOR  sd;
	PSECURITY_DESCRIPTOR psd = NULL;
	PACL                 pacl = NULL;
	PACL                 pNewAcl = NULL;
	BOOL                 bDaclPresent = FALSE;
	BOOL                 bDaclDefaulted = FALSE;
	DWORD                dwError = 0;
	DWORD                dwSize = 0;
	DWORD                dwBytesNeeded = 0;
	schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the service

	schService = OpenService(
		schSCManager,              // SCManager database 
		SVCNAME,                 // name of service 
		READ_CONTROL | WRITE_DAC); // access

	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	// Get the current security descriptor.

	if (!QueryServiceObjectSecurity(schService,
		DACL_SECURITY_INFORMATION,
		&psd,           // using NULL does not work on all versions
		0,
		&dwBytesNeeded))
	{
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			dwSize = dwBytesNeeded;
			psd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(),
				HEAP_ZERO_MEMORY, dwSize);
			if (psd == NULL)
			{
				// Note: HeapAlloc does not support GetLastError.
				printf("HeapAlloc failed\n");
				goto dacl_cleanup;
			}

			if (!QueryServiceObjectSecurity(schService,
				DACL_SECURITY_INFORMATION, psd, dwSize, &dwBytesNeeded))
			{
				printf("QueryServiceObjectSecurity failed (%d)\n", GetLastError());
				goto dacl_cleanup;
			}
		}
		else
		{
			printf("QueryServiceObjectSecurity failed (%d)\n", GetLastError());
			goto dacl_cleanup;
		}
	}

	// Get the DACL.

	if (!GetSecurityDescriptorDacl(psd, &bDaclPresent, &pacl,
		&bDaclDefaulted))
	{
		printf("GetSecurityDescriptorDacl failed(%d)\n", GetLastError());
		goto dacl_cleanup;
	}

	// Build the ACE.
	memset(&ea, 0, sizeof(ea));
	BuildExplicitAccessWithName(&ea, TEXT("CURRENT_USER"),
		SERVICE_START,
		SET_ACCESS, NO_INHERITANCE);

	dwError = SetEntriesInAcl(1, &ea, pacl, &pNewAcl);
	if (dwError != ERROR_SUCCESS)
	{
		printf("SetEntriesInAcl failed(%d)\n", dwError);
		goto dacl_cleanup;
	}

	// Initialize a new security descriptor.

	if (!InitializeSecurityDescriptor(&sd,
		SECURITY_DESCRIPTOR_REVISION))
	{
		printf("InitializeSecurityDescriptor failed(%d)\n", GetLastError());
		goto dacl_cleanup;
	}

	// Set the new DACL in the security descriptor.

	if (!SetSecurityDescriptorDacl(&sd, TRUE, pNewAcl, FALSE))
	{
		printf("SetSecurityDescriptorDacl failed(%d)\n", GetLastError());
		goto dacl_cleanup;
	}

	// Set the new DACL for the service object.

	if (!SetServiceObjectSecurity(schService,
		DACL_SECURITY_INFORMATION, &sd))
	{
		printf("SetServiceObjectSecurity failed(%d)\n", GetLastError());
		goto dacl_cleanup;
	}
	else printf("Service DACL updated successfully\n");

dacl_cleanup:
	CloseServiceHandle(schSCManager);
	CloseServiceHandle(schService);

	if (NULL != pNewAcl)
		LocalFree((HLOCAL)pNewAcl);
	if (NULL != psd)
		HeapFree(GetProcessHeap(), 0, (LPVOID)psd);
}
