#include "ScanObject.h"

void ScanObject::setPath(const std::filesystem::path &path)
{
	this->path = path;
	if (std::filesystem::is_directory(path))
	{
		type = OBJDIR;
	}
	else
	{
		type = OBJFILE; // File until it's scanned, then it might become zip.
	}
	
}
