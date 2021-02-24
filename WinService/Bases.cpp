#include "BasesLoader.h"

Bases::Bases(const std::u16string path)
{
	Base = BaseLoader::Load(path);
}
