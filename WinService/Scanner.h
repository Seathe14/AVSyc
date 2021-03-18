#pragma once
#include "Bases.h"
#include <filesystem>
#include <algorithm>
#include <zipper/zipper.h>
#include <zipper/unzipper.h>
#pragma comment(lib,"Zipper-staticd.lib")

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