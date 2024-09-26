#pragma once

#include <vector>
#include <stdint.h>

namespace StringCommon
{
	void ConvertMacRomanFileName(std::vector<uint8_t> &utf8FileName, const uint8_t *macRomanName, size_t macRomanLength);
}
