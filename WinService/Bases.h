#pragma once
#include <iostream>
#include <unordered_map>
struct Record
{
	std::u16string name, type;
	uint64_t sig, length, sha256, offsetStart, offsetEnd;

};

class Bases
{
public:
	Bases();
	Bases(const std::u16string path);

private:
	std::unordered_map<uint64_t, Record> Base;

};