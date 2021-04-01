#pragma once
#include "ScanTaskBase.h"
class ScheduleScannerTask : public ScanTaskBase
{
public:
	ScheduleScannerTask(Scanner& scanner) : ScanTaskBase(scanner) {}
	void cancel();
	void schedule(int64_t scTime, std::filesystem::path path);

private:
	std::chrono::seconds scheduledTime;
	std::chrono::seconds currTime;

	bool toCancel = false;
};

