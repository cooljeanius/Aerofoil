#pragma once

#include "IDecompressor.h"
#include "PrefixCode.h"

struct IFileReader;
class IDecompressor;
class CompactProLZHDecompressor;
class CompactProRLEDecompressor;

class CompactProLZHRLEDecompressor : public IDecompressor
{
public:
	explicit CompactProLZHRLEDecompressor(int blockSize);
	~CompactProLZHRLEDecompressor();

	bool Reset(CSInputBuffer *input, size_t compressedSize, size_t decompressedSize) override;
	bool ReadBytes(void *dest, size_t numBytes)  override;

private:
	IFileReader *m_proxyReader;
	CSInputBuffer *m_proxyInput;
	IDecompressor *m_lzhDecompressor;
	IDecompressor *m_rleDecompressor;
	int m_blockSize;
};
