#pragma once

#include "IDecompressor.h"

class NullDecompressor : public IDecompressor
{
public:
	NullDecompressor();

	bool Reset(CSInputBuffer *input, size_t compressedSize, size_t decompressedSize) override;
	bool ReadBytes(void *dest, size_t numBytes) override;

private:
	CSInputBuffer *m_input;
};
