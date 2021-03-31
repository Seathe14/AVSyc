#include "Monitor.h"

void Monitor::monitorFolder(std::filesystem::path path)
{
	dirHandle = FindFirstChangeNotification(path.c_str(), TRUE, FILE_NOTIFY_CHANGE_FILE_NAME);
	hEvent = CreateEvent(NULL, FALSE, FALSE, L"StopMonitoringEvent");

	//std::vector<HANDLE> objectsToWait = { dirHandle,hEvent };
	HANDLE handles[2] = { dirHandle,hEvent };
	while (true)
	{
		_ts = TaskStatus::Monitoring;
		//WaitForSingleObject(dirHandle, INFINITE);
		WaitForMultipleObjects(2, handles, FALSE, INFINITE);
		if (toStop)
		{
			toStop = false;
			//ResetEvent(hEvent);
			_ts = TaskStatus::Stopped;
			CloseHandle(hEvent);
			CloseHandle(dirHandle);
			return;
		}
		Sleep(1000);
		scan(path);
		CloseHandle(dirHandle);
		dirHandle = FindFirstChangeNotification(path.c_str(), TRUE, FILE_NOTIFY_CHANGE_FILE_NAME);
		Sleep(200);
	}
}

void Monitor::stop()
{
	toStop = true;
	SetEvent(hEvent);
}
