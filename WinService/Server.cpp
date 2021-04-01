#include "Server.h"
#include <filesystem>
#include <WtsApi32.h>
#include <Operations.h>
#include <ReadWrite.h>
#include <userenv.h>
#pragma comment(lib,"wtsapi32.lib")
#pragma comment(lib,"userenv.lib")
void Server::launchUI()
{
// 	std::filesystem::path p1(SOLUTION_DIR);
// #if _DEBUG
// 	p1 += "AVSyc\\debug\\AVSyc.exe";
// 	std::wstring path = p1.wstring();
// #else 
// 	p1 += "AVSyc\\release\\AVSyc.exe";
// 	std::wstring path = p1.wstring();
// #endif
	std::filesystem::path p = std::filesystem::current_path();
	p += "\\AVSyc.exe";
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
	CreateProcessAsUserW(token, p.c_str(), NULL, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT, penv, NULL, &si, &pi);
}

void Server::sendStatistics(const std::u16string& reportToSend, HANDLE pipe)
{
	if (reportToSend.length() > 1024)
	{
		int numofParts = reportToSend.length() % 1024 ? reportToSend.length() / 1024 + 1 : reportToSend.length() / 1024;
		Writeuint32_t(pipe, numofParts);
		for (int i = 0; i < numofParts; i++)
		{
			std::u16string partToSend = reportToSend.substr(i * 1024, 1024);
			WriteU16String(pipe, partToSend);
		}
	}
	else
	{
		Writeint32_t(pipe, 1);
		WriteU16String(pipe, reportToSend);
	}
}

void Server::WaitForPipe()
{
	SECURITY_DESCRIPTOR sd = { 0, };
	SECURITY_ATTRIBUTES sa = { sizeof(sa), };
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, true, NULL, false);
	sa.lpSecurityDescriptor = &sd;
	hPipe = CreateNamedPipe(lpszPipeName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, BUFSIZE, BUFSIZE, 0, &sa);
	ConnectNamedPipe(hPipe, NULL);
	hPipeScheduled = CreateNamedPipe(lpszScPipeName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, BUFSIZE, BUFSIZE, 0, &sa);
	ConnectNamedPipe(hPipeScheduled, NULL);
	hPipeMonitor = CreateNamedPipe(lpszPipeMonitorName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, BUFSIZE, BUFSIZE, 0, &sa);
	ConnectNamedPipe(hPipeMonitor, NULL);
	//return;
}

void Server::AcceptMessages()
{
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
	std::thread scanPathThread(&Server::AcceptPathMessages, this, scPathTask);
	std::thread scanScheduleThread(&Server::AcceptScheduleMessages, this, scheduledScanner);
	std::thread monitorThread(&Server::AcceptMonitorMessages, this, monitorTask);
	scanPathThread.join();
	monitorThread.join();
	scanScheduleThread.join();
	//return 0;
}

void Server::AcceptPathMessages(ScanPathTask& scPathTask)
{
	DWORD cbRead, cbWritten;
	UCHAR code;
	while (true)
	{
		if (ReadFile(hPipe, &code, sizeof(UCHAR), &cbRead, NULL) != 0)
		{
			switch (code)
			{
			case Operation::SCANPATH:
			{

				Writeint8_t(hPipe, OperationResult::WAITING);
				std::filesystem::path path;
				path = ReadU16String(hPipe);
				std::thread t1 = std::thread(&ScanPathTask::scan, &scPathTask, path);
				t1.detach();
				Sleep(500);
				break;
			}
			case Operation::GET_STATE:

				if (scPathTask.getStatus() == TaskStatus::Complete)
					Writeint8_t(hPipe, OperationResult::SUCCESS);
				else if (scPathTask.getStatus() == TaskStatus::Running)
				{
					Writeint8_t(hPipe, OperationResult::RUNNING);
				}
				else
				{
					Writeint8_t(hPipe, OperationResult::WAITING);
				}
				break;
			case Operation::STOP:
				scPathTask.stop();
				break;
			case Operation::GET_STATISTICS:
				std::u16string result = scPathTask.getTaskStatistic();
				sendStatistics(result, hPipe);
				break;

			}
		}
		else
		{
			DisconnectNamedPipe(hPipe);
			ConnectNamedPipe(hPipe, NULL);

		}
	}
}

void Server::AcceptScheduleMessages(ScheduleScannerTask& scheduledScanner)
{
	DWORD cbRead, cbWritten;
	UCHAR code;
	while (true)
	{
		if (ReadFile(hPipeScheduled, &code, sizeof(UCHAR), &cbRead, NULL) != 0)
		{
			switch (code)
			{
			case Operation::SCANSCHEDULED:
			{
				int64_t time = Readint64_t(hPipeScheduled);
				std::filesystem::path path;
				path = ReadU16String(hPipeScheduled);
				std::thread t1 = std::thread(&ScheduleScannerTask::schedule, &scheduledScanner, time, path);
				t1.detach();
				Writeint8_t(hPipeScheduled, OperationResult::SCHEDULED);
				Sleep(500);

				break;
			}
			case Operation::GET_STATE:
				if (scheduledScanner.getStatus() == TaskStatus::Complete)
					Writeint8_t(hPipeScheduled, OperationResult::SUCCESS);
				else if (scheduledScanner.getStatus() == TaskStatus::Scheduled)
				{
					Writeint8_t(hPipeScheduled, OperationResult::SCHEDULED);
				}
				else if (scheduledScanner.getStatus() == TaskStatus::Failed)
				{
					Writeint8_t(hPipeScheduled, OperationResult::FAILED);

				}
				else if (scheduledScanner.getStatus() == TaskStatus::Waiting)
				{
					Writeint8_t(hPipeScheduled, OperationResult::WAITING);
				}
				else
				{
					Writeint8_t(hPipeScheduled, OperationResult::RUNNING);
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
				sendStatistics(result, hPipeScheduled);
				break;
			}
		}
		else
		{
			DisconnectNamedPipe(hPipeScheduled);
			ConnectNamedPipe(hPipeScheduled, NULL);

		}
	}
}

void Server::AcceptMonitorMessages(Monitor& monitorTask)
{
	DWORD cbRead, cbWritten;
	UCHAR code;
	while (true)
	{
		if (ReadFile(hPipeMonitor, &code, sizeof(UCHAR), &cbRead, NULL) != 0)
		{
			switch (code)
			{
			case Operation::MONITOR:
			{
				std::filesystem::path path;
				path = ReadU16String(hPipeMonitor);
				std::thread t1 = std::thread(&Monitor::monitorFolder, &monitorTask, path);
				t1.detach();
				Writeint8_t(hPipeMonitor, OperationResult::MONITORING);
				break;
			}
			case Operation::GET_STATE:
				if (monitorTask.getStatus() == TaskStatus::Complete)
					Writeint8_t(hPipeMonitor, OperationResult::SUCCESS);
				else if (monitorTask.getStatus() == TaskStatus::Monitoring)
				{
					Writeint8_t(hPipeMonitor, OperationResult::MONITORING);
				}
				else if (monitorTask.getStatus() == TaskStatus::Stopped)
				{
					Writeint8_t(hPipeMonitor, OperationResult::STOPPED);

				}
				else
				{
					Writeint8_t(hPipeMonitor, OperationResult::RUNNING);
				}
				break;
			case Operation::STOP:
				monitorTask.stop();
				break;
			case Operation::GET_STATISTICS:
				std::u16string result = monitorTask.getTaskStatistic();
				sendStatistics(result, hPipeMonitor);
				break;
			}
		}
		else
		{
			DisconnectNamedPipe(hPipeMonitor);
			ConnectNamedPipe(hPipeMonitor, NULL);
		}
	}
}
