#pragma once
#include "Scanner.h"
#include <string>
#include "ScanTaskBase.h"


class ScanPathTask : public ScanTaskBase
{
	public:

		ScanPathTask(Scanner& scanner) : ScanTaskBase{ scanner } {}

// 		void stop() override
// 		{
// 			if(_ts == TaskStatus::Running)
// 			{
// 				_sc->Stop();
// 				_ts = TaskStatus::Stopped;
// 			}
// 		}
// private:
// 	std::filesystem::path _path;
};

