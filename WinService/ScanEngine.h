#pragma once
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <zipper/zipper.h>
#include <zipper/unzipper.h>
#include "Bases.h"
#pragma comment(lib,"Zipper-staticd.lib")

#define MINSIGLENGTH 8
class ScanEngine
{
public:
	ScanEngine(const std::filesystem::path& path,const Bases& base)
		:
		path(path),
		base(base)
	{
		contents.resize(bufferSize);
	}
	void ScanZip(std::u16string& stat);
	void ScanZip(std::u16string& stat,std::istream &is);

	void ScanFolder(std::u16string& stat);
	void ScanFile(std::u16string &stat);

private:
	std::filesystem::path path;
	std::string contents;
	uint64_t bufferSize = 1 << 20;
	std::u16string virusName;
	std::u16string statistics;
	Bases base;
private:
	void updateString(std::ifstream& ifs);
	void updateString(std::istream& is);
};

