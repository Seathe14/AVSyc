#pragma once
#include "ScanTaskBase.h"
class ScheduleScannerTask : public ScanTaskBase
{
public:
	ScheduleScannerTask(Scanner& scanner) : ScanTaskBase(scanner) {}
	void cancel();
	void schedule(int64_t scTime, std::filesystem::path path);
	//void scan(std::filesystem::path path) override;
/*	void stop() override;*/
private:
	std::chrono::seconds scheduledTime;
	std::chrono::seconds currTime;

	bool toCancel = false;
};

