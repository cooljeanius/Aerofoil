#pragma once

struct IFileReader;
struct ArchiveItemList;

class IArchiveParser
{
public:
	virtual bool Check(IFileReader &reader) = 0;
	virtual ArchiveItemList *Parse(IFileReader &reader) = 0;
};
