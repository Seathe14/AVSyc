#pragma once
#include "Bases.h"
#include <filesystem>
#include <algorithm>
#include <string>
#include "ScanEngine.h"
#include "ScanObject.h"
#include <zipper/zipper.h>
#include <zipper/unzipper.h>


class Scanner
{
public:
	Scanner(Bases& base);
	void Scan(const std::filesystem::path& path);

	std::u16string getStatistics() { return statistics; }
private:
	Bases base;
	std::u16string statistics;
};