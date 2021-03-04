#pragma once
#include "Bases.h"
#include <fstream>
#include <algorithm>
class Scanner
{
public:
	Scanner(Bases& base);
	void Scan(const std::wstring& path);
	void updateString(std::ifstream& ifs);
	std::u16string getStatistics() { return statistics; }
private:
	Bases base;
	std::string s;
	uint64_t bufferSize = 1 << 20;
	std::u16string virusName;
	std::u16string statistics;
};