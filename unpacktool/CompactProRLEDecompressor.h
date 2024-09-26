#pragma once

#include "IDecompressor.h"

class CompactProRLEDecompressor : public IDecompressor
{
public:
	CompactProRLEDecompressor();
	~CompactProRLEDecompressor();

	bool Reset(CSInputBuffer *input, size_t compressedSize, size_t decompressedSize);
	bool ReadBytes(void *dest, size_t numBytes);

public:
	CSInputBuffer *input;
	int saved, repeat;
	bool halfescaped;

	bool EmitOneByte(uint8_t &b);
};
