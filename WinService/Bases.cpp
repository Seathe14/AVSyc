#include "Bases.h"
#include "BasesLoader.h"
Bases::Bases(const std::u16string path)
{
	Base = BaseLoader::Load(path);
}

bool Bases::find(uint8_t* begin, size_t size, uint32_t offStart,std::u16string& name)
{
	uint64_t key = *((uint64_t*)(begin));
	std::unordered_map<uint64_t, Record>::iterator got = Base.find(key);

	if (got != Base.end())
		if (got->second.length == 8)
		{
	
			SHA256 sha;
			std::string s;
			s.resize(got->second.length);
			memcpy(&s[0], begin + 8, got->second.length);

			sha.update(s);
			uint8_t* digest = sha.digest();
			std::string temp = SHA256::toString(digest);
			std::u16string hash(temp.begin(), temp.end());
			delete[] digest;


			if (got->second.sha256 == hash)
			{
				std::u16string s = got->second.name;
				name = got->second.name;
				std::string a(s.begin(), s.end());
				return true;
			}
		}
	return false;
}

void Bases::updateBase(const std::u16string path)
{
	Base = BaseLoader::Load(path);
}
