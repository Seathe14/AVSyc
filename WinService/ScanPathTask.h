#pragma once
#include "Scanner.h"
#include <string>
#include "ScanTaskBase.h"


class ScanPathTask : public ScanTaskBase
{
	public:

		ScanPathTask(Scanner& scanner) : ScanTaskBase{ scanner } {}
};

