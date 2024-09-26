#pragma once

#include "IArchiveParser.h"

class StuffItParser : public IArchiveParser
{
public:
	bool Check(IFileReader &reader) override;
	ArchiveItemList *Parse(IFileReader &reader) override;
};
