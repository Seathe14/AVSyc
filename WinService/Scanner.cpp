#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include "Scanner.h"

Scanner::Scanner(Bases &base)
{
	this->base = base;
}
using namespace zipper;

// std::string UTF8ToANSI(std::string s)
// {
// 	BSTR    bstrWide;
// 	char* pszAnsi;
// 	int     nLength;
// 	const char* pszCode = s.c_str();
// 
// 	nLength = MultiByteToWideChar(CP_UTF8, 0, pszCode, strlen(pszCode) + 1, NULL, NULL);
// 	bstrWide = SysAllocStringLen(NULL, nLength);
// 
// 	MultiByteToWideChar(CP_UTF8, 0, pszCode, strlen(pszCode) + 1, bstrWide, nLength);
// 
// 	nLength = WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, NULL, 0, NULL, NULL);
// 	pszAnsi = new char[nLength];
// 
// 	WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, pszAnsi, nLength, NULL, NULL);
// 	SysFreeString(bstrWide);
// 
// 	std::string r(pszAnsi);
// 	delete[] pszAnsi;
// 	return r;
// }

void Scanner::Scan(const std::filesystem::path& path)
{
	ScanEngine scanEngine(path, base);
	ScanObject scanObj;
	scanObj.setPath(path);
	scanEngine.ScanPath(scanObj,statistics);
}
