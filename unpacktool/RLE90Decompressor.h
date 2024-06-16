#pragma once

#include <cstdint>

#include "IDecompressor.h"

class RLE90Decompressor : public IDecompressor
{
public:
	RLE90Decompressor();

	bool Reset(CSInputBuffer *input, size_t compressedSize, size_t decompressedSize) override;
	bool ReadBytes(void *dest, size_t numBytes) override;

private:
	CSInputBuffer *m_input;
	uint8_t m_repeatedByte;
	uint8_t m_count;
};
