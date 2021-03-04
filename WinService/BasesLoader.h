#pragma once
#include <windows.h>
#include <ReadWrite.h>
#include "Bases.h"
#include <sstream>
class BaseLoader
{
public:
	static std::unordered_map<uint64_t, Record> Load(const std::u16string path);
};