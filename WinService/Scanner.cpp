#include "Scanner.h"
#include <string>
#include <codecvt>
Scanner::Scanner(Bases &base)
{
	this->base = base;
	contents.resize(bufferSize);
}

void Scanner::Scan(const std::filesystem::path& path)
{
	memset(&contents[0], 0, bufferSize);
	std::ifstream ifs(path.wstring(), std::ios::binary);
	ifs.seekg(0, std::ios::end);
	std::streamsize size = ifs.tellg();
	ifs.seekg(0, std::ios::beg);
	size_t offStart = 0;
	ifs.read(&contents[0], bufferSize);
	uint64_t iterNum = size % bufferSize ? size / bufferSize + 1 : size / bufferSize;
	//size_t j;
	int numofViruses = 0;
	if (contents[0] != 'M' && contents[1] != 'Z')
	{
		statistics += u"File is not infected\n";
		return;
	}
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
		updateString(ifs);
	}
	if (numofViruses == 0)
	{
		statistics += u"File is not infected\n";
	}
}

void Scanner::updateString(std::ifstream& ifs)
{
	std::copy(contents.end() - MINSIGLENGTH, contents.end(), contents.begin());
	ifs.read(&contents[0] + MINSIGLENGTH, bufferSize - MINSIGLENGTH);
}

