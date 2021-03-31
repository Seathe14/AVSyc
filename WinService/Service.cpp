#define _CRT_SECURE_NO_WARNINGS
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
#include "ScanPathTask.h"
#include <tchar.h>
#include "ScheduleScannerTask.h"
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
		SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START,
		SERVICE_ERROR_NORMAL, szPath, NULL, NULL, NULL, NULL, NULL);
	if (schService == NULL)
	{
		printf("CreateService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}
	else printf("Service installed successfully\n");
	createRegistryRecord();
	DoUpdateSvcDacl();
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

void Service::SvcUninstall()
{
	DWORD dwBytesNeeded;
	SERVICE_STATUS_PROCESS ssp;

	SC_HANDLE schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	SC_HANDLE schService = OpenService(
		schSCManager,         // SCM database 
		SVCNAME,            // name of service 
		SERVICE_ALL_ACCESS |
		SERVICE_QUERY_STATUS |
		SERVICE_ENUMERATE_DEPENDENTS);

	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}



	DeleteService(schService);
	deleteRegistryRecord();
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
// 	case SERVICE_CONTROL_STOP:
// 		ServiceInstance->ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
// 		//SetEvent(ghSvcStopEvent);
// 		ServiceInstance->gSvcStatus.dwWin32ExitCode = 0;
// 		ServiceInstance->gSvcStatus.dwCurrentState = SERVICE_STOPPED;
// 		ServiceInstance->ReportSvcStatus(ServiceInstance->gSvcStatus.dwCurrentState, NO_ERROR, 0);
// 		break;;
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


void Service::AcceptPathMessages(ScanPathTask& scPathTask)
{
	DWORD cbRead, cbWritten;
	UCHAR code;
	while (true)
	{
		if (ReadFile(ServiceInstance->hPipe, &code, sizeof(UCHAR), &cbRead, NULL) != 0)
		{
			switch (code)
			{
			case Operation::SCANPATH:
			{
// 				if (scPathTask.getStatus() != TaskStatus::Stopped && scPathTask.getStatus() != TaskStatus::Complete)
// 				{
// 					Writeint8_t(ServiceInstance->hPipe, OperationResult::RUNNING);
// 					break;
// 				}
				Writeint8_t(ServiceInstance->hPipe, OperationResult::WAITING);
				std::filesystem::path path;
				// 				Scanner sc(base);
				path = ReadU16String(ServiceInstance->hPipe);
				std::thread t1 = std::thread(&ScanPathTask::scan, &scPathTask, path);
				t1.detach();
				Sleep(500);
				break;
			}
			case Operation::GET_STATE:

				if (scPathTask.getStatus() == TaskStatus::Complete)
					Writeint8_t(ServiceInstance->hPipe, OperationResult::SUCCESS);
				else if(scPathTask.getStatus() == TaskStatus::Running)
				{
					Writeint8_t(ServiceInstance->hPipe, OperationResult::RUNNING);
				}
				else 
				{
					Writeint8_t(ServiceInstance->hPipe, OperationResult::WAITING);
				}
				break;
			case Operation::STOP:
				scPathTask.stop();
				break;
			case Operation::GET_STATISTICS:
				std::u16string result = scPathTask.getTaskStatistic();
				sendStatistics(result);
// 				if (result.length() > 1024)
// 				{
// 					int numofParts = result.length() % 1024 ? result.length() / 1024 + 1 : result.length() / 1024;
// 					Writeuint32_t(ServiceInstance->hPipe, numofParts);
// 					for (int i = 0; i < numofParts; i++)
// 					{
// 						std::u16string partToSend = result.substr(i * 1024, 1024);
// 						WriteU16String(ServiceInstance->hPipe, partToSend);
// 					}
// 				}
// 				else
// 				{
// 					Writeint32_t(ServiceInstance->hPipe, 1);
// 					WriteU16String(ServiceInstance->hPipe, result);
// 				}
				break;

			}
		}
		else
		{
			DisconnectNamedPipe(ServiceInstance->hPipe);
			ConnectNamedPipe(ServiceInstance->hPipe, NULL);

		}
	}
}

void Service::AcceptScheduleMessages(ScheduleScannerTask& scheduledScanner)
{
	DWORD cbRead, cbWritten;
	UCHAR code;
	while (true)
	{
		if (ReadFile(ServiceInstance->hPipeScheduled, &code, sizeof(UCHAR), &cbRead, NULL) != 0)
		{
			switch (code)
			{
			case Operation::SCANSCHEDULED:
			{
				int64_t time = Readint64_t(ServiceInstance->hPipeScheduled);
				std::filesystem::path path;
				path = ReadU16String(ServiceInstance->hPipeScheduled);
				std::thread t1 = std::thread(&ScheduleScannerTask::schedule, &scheduledScanner, time, path);
				t1.detach();
				Writeint8_t(ServiceInstance->hPipeScheduled, OperationResult::SCHEDULED);
				Sleep(500);

				break;
			}
			case Operation::GET_STATE:
				if (scheduledScanner.getStatus() == TaskStatus::Complete)
					Writeint8_t(ServiceInstance->hPipeScheduled, OperationResult::SUCCESS);
				else if(scheduledScanner.getStatus() == TaskStatus::Scheduled)
				{
					Writeint8_t(ServiceInstance->hPipeScheduled, OperationResult::SCHEDULED);
				}
				else if (scheduledScanner.getStatus() == TaskStatus::Failed)
				{
					Writeint8_t(ServiceInstance->hPipeScheduled, OperationResult::FAILED);

				}
				else if(scheduledScanner.getStatus() == TaskStatus::Waiting)
				{
					Writeint8_t(ServiceInstance->hPipeScheduled, OperationResult::WAITING);
				}
				else
				{
					Writeint8_t(ServiceInstance->hPipeScheduled, OperationResult::RUNNING);
				}
				break;
			case Operation::STOP:
				scheduledScanner.stop();
				break;
			case Operation::CANCELSCHEDULE:
				scheduledScanner.cancel();
				break;
			case Operation::GET_STATISTICS:
				std::u16string result = scheduledScanner.getTaskStatistic();
				if (result.length() > 1024)
				{
					int numofParts = result.length() % 1024 ? result.length() / 1024 + 1 : result.length() / 1024;
					Writeuint32_t(ServiceInstance->hPipeScheduled, numofParts);
					for (int i = 0; i < numofParts; i++)
					{
						std::u16string partToSend = result.substr(i * 1024, 1024);
						WriteU16String(ServiceInstance->hPipeScheduled, partToSend);
					}
				}
				else
				{
					Writeint32_t(ServiceInstance->hPipeScheduled, 1);
					WriteU16String(ServiceInstance->hPipeScheduled, result);
				}
				break;
			}
		}
		else
		{
			DisconnectNamedPipe(ServiceInstance->hPipeScheduled);
			ConnectNamedPipe(ServiceInstance->hPipeScheduled, NULL);

		}
	}
}

void Service::AcceptMonitorMessages(Monitor& monitorTask)
{
	DWORD cbRead, cbWritten;
	UCHAR code;
	while (true)
	{
		if (ReadFile(ServiceInstance->hPipeOper, &code, sizeof(UCHAR), &cbRead, NULL) != 0)
		{
			switch (code)
			{
			case Operation::MONITOR:
			{
				std::filesystem::path path;
				path = ReadU16String(ServiceInstance->hPipeOper);
				std::thread t1 = std::thread(&Monitor::monitorFolder, &monitorTask, path);
				t1.detach();
				Writeint8_t(ServiceInstance->hPipeOper, OperationResult::MONITORING);
				break;
			}
			case Operation::GET_STATE:
				if (monitorTask.getStatus() == TaskStatus::Complete)
					Writeint8_t(ServiceInstance->hPipeOper, OperationResult::SUCCESS);
				else if (monitorTask.getStatus() == TaskStatus::Monitoring)
				{
					Writeint8_t(ServiceInstance->hPipeOper, OperationResult::MONITORING);
				}
				else if (monitorTask.getStatus() == TaskStatus::Stopped)
				{
					Writeint8_t(ServiceInstance->hPipeOper, OperationResult::STOPPED);

				}
				else
				{
					Writeint8_t(ServiceInstance->hPipeOper, OperationResult::RUNNING);
				}
				break;
			case Operation::STOP:
				monitorTask.stop();
				break;
			case Operation::GET_STATISTICS:
				std::u16string result = monitorTask.getTaskStatistic();
				if (result.length() > 1024)
				{
					int numofParts = result.length() % 1024 ? result.length() / 1024 + 1 : result.length() / 1024;
					Writeuint32_t(ServiceInstance->hPipeOper, numofParts);
					for (int i = 0; i < numofParts; i++)
					{
						std::u16string partToSend = result.substr(i * 1024, 1024);
						WriteU16String(ServiceInstance->hPipeOper, partToSend);
					}
				}
				else
				{
					Writeint32_t(ServiceInstance->hPipeOper, 1);
					WriteU16String(ServiceInstance->hPipeOper, result);
				}
				break;
			}
		}
		else
		{
			DisconnectNamedPipe(ServiceInstance->hPipeOper);
			ConnectNamedPipe(ServiceInstance->hPipeOper, NULL);

		}
	}
}

void Service::createRegistryRecord()
{
	TCHAR buffer[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, buffer);
	DWORD dwDisposition;
	HKEY  hKey;
	DWORD Ret;
	Ret =
		RegCreateKeyEx(
			HKEY_LOCAL_MACHINE,
			TEXT("SOFTWARE\\AVSyc"),
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS | KEY_WOW64_64KEY,
			NULL,
			&hKey,
			&dwDisposition);

	if (Ret != ERROR_SUCCESS)
	{
		printf("Error opening or creating new key\n");
		return;
	}

	RegSetValueEx(hKey,
		TEXT("Working Directory"),
		0,
		REG_SZ,
		(LPBYTE)(buffer),
		((((DWORD)lstrlen(buffer) + 1)) * sizeof(TCHAR)));

	RegCloseKey(hKey);
}

void Service::setWorkingDirectory()
{
	DWORD len = 1024;
	DWORD readDataLen = len;

	DWORD Ret;
	HKEY hKey;

	Ret = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		TEXT("SOFTWARE\\AVSyc"),
		0,
		KEY_READ | KEY_WOW64_64KEY,
		&hKey
	);

	Ret = RegQueryValueEx(
		hKey,
		TEXT("Working Directory"),
		NULL,
		NULL,
		(BYTE*)WorkingDirectory,
		&readDataLen
	);
	if (Ret != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return;
	}
	RegCloseKey(hKey);

	_tcscat(WorkingDirectory, TEXT("\\"));
	SetCurrentDirectory(WorkingDirectory);

	TCHAR buffer[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, buffer);
}

void Service::deleteRegistryRecord()
{
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\AVSyc"),
		0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
	{
		RegCloseKey(hKey);

		RegDeleteTree(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\AVSyc"));
	}
}

DWORD WINAPI Service::AcceptMessages(LPVOID lpvParam)
{
 	//std::filesystem::path p(SOLUTION_DIR);
 	//p += "BaseEditor\\Ghosts.dbs";
	std::filesystem::path p = std::filesystem::current_path();
	p += "\\Ghosts.dbs";
 	Bases base(p.u16string());
 	static std::u16string lastResult;
 	std::mutex mtx;
 	Scanner scanner(base);
 	ScanPathTask scPathTask(scanner);
	ScheduleScannerTask scheduledScanner(scanner);
	Monitor monitorTask(scanner);
 	std::string name = "thread123";
	std::thread scanPathThread(&Service::AcceptPathMessages, ServiceInstance,scPathTask);
	std::thread scanScheduleThread(&Service::AcceptScheduleMessages, ServiceInstance, scheduledScanner);
	std::thread monitorThread(&Service::AcceptMonitorMessages, ServiceInstance, monitorTask);

	scanPathThread.join();
	monitorThread.join();
	scanScheduleThread.join();
	return 0;
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
	setWorkingDirectory();
	ServiceInstance->ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
	serv.launchUI();

/*	HANDLE threadPipe;*/
	std::thread t1(&Server::WaitForPipe, &serv);
 	t1.join();
 	std::thread t2(&Server::AcceptMessages, &serv);
 	t2.detach();
	//threadPipe = CreateThread(NULL, 0, &serv.WaitForPipe(), NULL, 0, NULL);
	//WaitForSingleObject(threadPipe, INFINITE);
	//CreateThread(NULL, 0, AcceptMessages, (LPVOID)hPipe, 0, NULL);
	// TO_DO Perform work
	//ServiceInstance->launchUI();

// 	HANDLE threadPipe;
// 	threadPipe = CreateThread(NULL, 0, WaitForPipe, NULL, 0, NULL);
// 	WaitForSingleObject(threadPipe, INFINITE);
// 	CreateThread(NULL, 0, AcceptMessages, (LPVOID)hPipe, 0, NULL);
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
