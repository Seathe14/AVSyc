#pragma once
#include "Bases.h"
#include <filesystem>
#include <algorithm>
class Scanner
{
public:
	Scanner(Bases& base);
	void Scan(const std::filesystem::path& path);
	void updateString(std::ifstream& ifs);
	std::u16string getStatistics() { return statistics; }
private:
	Bases base;
	std::string contents;
	uint64_t bufferSize = 1 << 20;
	std::u16string virusName;
	std::u16string statistics;
};