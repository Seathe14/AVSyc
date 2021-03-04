#include "Scanner.h"

Scanner::Scanner(Bases &base)
{
	this->base = base;
	s.resize(bufferSize);
}

void Scanner::Scan(const std::wstring& path)
{
	std::ifstream ifs(path, std::ios::binary);
	ifs.seekg(0, std::ios::end);
	std::streamsize size = ifs.tellg();
	ifs.seekg(0, std::ios::beg);
	size_t offStart = 0;
	ifs.read(&s[0], bufferSize);

	for (size_t i = 0; i < bufferSize - 8; i++)
	{
		if (i + 1024 == bufferSize)
		{
			updateString(ifs);
			i = 0;
		}
		if (base.find((uint8_t*)&s[i], 8, offStart,virusName))
		{
			
			break;
		}
		//if (offStart == 0xF1D00)
		//	break;
		offStart++;
	}	
	statistics += virusName;
	statistics += u" in " + std::u16string(path.begin(),path.end());
}

void Scanner::updateString(std::ifstream& ifs)
{
	std::copy(s.end() - 1024, s.end(), s.begin());
	memset(s.data() + 1024, 0, bufferSize - 1024);
	ifs.read(s.data() + 1024, bufferSize - 1024);
}

