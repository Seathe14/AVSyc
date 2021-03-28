#pragma once
#include "Bases.h"
#include <filesystem>
#include <algorithm>
#include <string>
#include "ScanEngine.h"
#include "ScanObject.h"
#include <zipper/zipper.h>
#include <zipper/unzipper.h>
#include <mutex>

class Scanner
{
public:
	Scanner(Bases& base);
	void Scan(const std::filesystem::path& path);

	std::u16string getStatistics() { return statistics; }
private:
	Bases base;
	static inline std::mutex mtx;
	std::u16string statistics;
};