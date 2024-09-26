#pragma once

#include "IDecompressor.h"
#include "LZW.h"


class LZWDecompressor : public IDecompressor
{
public:
	explicit LZWDecompressor(int compressFlags);
	~LZWDecompressor();

	bool Reset(CSInputBuffer *input, size_t compressedSize, size_t decompressedSize) override;
	bool ReadBytes(void *dest, size_t numBytes) override;

private:
	CSInputBuffer *input;
	int compressFlags;

	bool blockmode;

	LZW *lzw;
	int symbolcounter;

	uint8_t *buffer, *bufferend;
};
