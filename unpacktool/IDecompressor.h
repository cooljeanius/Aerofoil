#pragma once

#include <cstddef>

struct CSInputBuffer;

class IDecompressor
{
public:
	virtual ~IDecompressor() { }
	virtual bool Reset(CSInputBuffer *input, size_t compressedSize, size_t decompressedSize) = 0;
	virtual bool ReadBytes(void *dest, size_t numBytes) = 0;
};
