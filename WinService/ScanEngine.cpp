#include "ScanEngine.h"
#include <Windows.h>
using namespace zipper;
std::string ANSIToUTF8(std::string str)
{
	std::string res;
	int result_u, result_c;
	result_u = MultiByteToWideChar(866, 0, str.c_str(), -1, 0, 0);
	if (!result_u) { return 0; }
	wchar_t* ures = new wchar_t[result_u];
	if (!MultiByteToWideChar(866, 0, str.c_str(), -1, ures, result_u)) {
		delete[] ures;
		return 0;
	}
	result_c = WideCharToMultiByte(1251, 0, ures, -1, 0, 0, 0, 0);
	if (!result_c) {
		delete[] ures;
		return 0;
	}
	char* cres = new char[result_c];
	if (!WideCharToMultiByte(1251, 0, ures, -1, cres, result_c, 0, 0)) {
		delete[] cres;
		return 0;
	}
	delete[] ures;
	res.append(cres);
	delete[] cres;
	return res;
}

void ScanEngine::ScanZip(ScanObject scanObj, std::u16string& stat)
{
	memset(&contents[0], 0, bufferSize);
	Unzipper* unzip;
	if (scanObj.sstream != nullptr)
	{
		try {
		unzip = new Unzipper(*scanObj.sstream);
		}
		catch (std::runtime_error err) {
			return;
		}
	}
	else
	{
		unzip = new Unzipper(scanObj.getPath().string());
	}
	std::vector<ZipEntry> entries = unzip->entries();
	int numofViruses = 0;
	for (int i = 0; i < entries.size(); i++)
	{
		std::stringstream ss;
		memset(&contents[0], 0, bufferSize);
		unzip->extractEntryToStream(entries[i].name, ss);
		ss.seekg(0, std::ios::end);
		std::streamsize size = ss.tellg();
		ss.seekg(0, std::ios::beg);
		size_t offStart = 0;
		ss.read(&contents[0], bufferSize);
		if (contents[0] == 'P' && contents[1] == 'K')
		{
			scanObj.sstream = &ss;
			ScanZip(scanObj, stat);
			continue;
		}
		else if (contents[0] != 'M' && contents[1] != 'Z')
		{
			statistics += u"File is not infected\n";
			continue;
		}
		uint64_t iterNum = size % bufferSize ? size / bufferSize + 1 : size / bufferSize;

		for (int i = 0; i < iterNum; i++)
		{
			for (int j = 0; j < bufferSize - MINSIGLENGTH; j++)
			{
				if (base.find((uint8_t*)&contents[j], offStart, virusName))
				{
					statistics += virusName;
					statistics += u" in " + path.u16string() + u"\n";
					numofViruses++;
				}
				offStart++;
			}
			updateString(ss);
		}
		if (numofViruses == 0)
		{
			statistics += u"File is not infected\n";
		}
	}
	unzip->close();
	delete unzip;
	stat = statistics;
}

void ScanEngine::ScanPath(ScanObject scanObj, std::u16string& stat)
{
	if (scanObj.type == OBJDIR)
	{
		ScanFolder(scanObj, stat);
	}
	else if (scanObj.type == OBJFILE)
	{
		ScanFile(scanObj, stat);
	}
}

void ScanEngine::ScanFolder(ScanObject scanObj, std::u16string& stat)
{
	for (const auto& entry : std::filesystem::recursive_directory_iterator(scanObj.getPath()))
	{
		if (std::filesystem::is_directory(entry))
			continue;
 		path = entry.path();
		ScanObject scanObject(path);
		//scanObject.setPath(path);

// 		this->path = path;
		ScanFile(scanObject,stat);
	}
}

void ScanEngine::ScanFile(ScanObject scanObj,std::u16string &stat)
{
	memset(&contents[0], 0, bufferSize);
	std::ifstream ifs(scanObj.getPath().wstring(), std::ios::binary);
	ifs.seekg(0, std::ios::end);
	std::streamsize size = ifs.tellg();
	ifs.seekg(0, std::ios::beg);
	size_t offStart = 0;
	ifs.read(&contents[0], bufferSize);
	if (contents[0] == 'P' && contents[1] == 'K')
	{
		ScanObject scanObject(scanObj.getPath());
		scanObj.type = OBJZIP;
		ScanZip(scanObj,stat);
		return;
	}
	else if(contents[0] != 'M' && contents[1] != 'Z')
	{
		statistics += u"File is not infected\n";
		stat = statistics;
		return;
	}

	uint64_t iterNum = size % bufferSize ? size / bufferSize + 1 : size / bufferSize;
	int numofViruses = 0;

	for (int i = 0; i < iterNum; i++)
	{
		for (int j = 0; j < bufferSize - MINSIGLENGTH; j++)
		{
			if (base.find((uint8_t*)&contents[j], offStart, virusName))
			{
				statistics += virusName;
				statistics += u" in " + scanObj.getPath().u16string() + u"\n";
				numofViruses++;
			}
			offStart++;
		}
		updateString(ifs);
	}
	if (numofViruses == 0)
	{
		statistics += u"File is not infected\n";
	}
	stat = statistics;
}

void ScanEngine::updateString(std::ifstream& ifs)
{
	std::copy(contents.end() - MINSIGLENGTH, contents.end(), contents.begin());
	ifs.read(&contents[0] + MINSIGLENGTH, bufferSize - MINSIGLENGTH);
}

void ScanEngine::updateString(std::istream& is)
{
	std::copy(contents.end() - MINSIGLENGTH, contents.end(), contents.begin());
	is.read(&contents[0] + MINSIGLENGTH, bufferSize - MINSIGLENGTH);
}
