#pragma once
#include "ScanTaskBase.h"
#include <windows.h>
class Monitor : public ScanTaskBase
{
public:
	Monitor(Scanner& scanner) : ScanTaskBase(scanner) 
	{
	}
	void monitorFolder(std::filesystem::path path);
	void stop() override;
private:
	HANDLE dirHandle;
	HANDLE hEvent;
	bool toStop;
};

