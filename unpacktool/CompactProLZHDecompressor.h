#pragma once

#include "LZSSDecompressor.h"
#include "IFileReader.h"

class XADPrefixCode;

class CompactProLZHDecompressor : public LZSSDecompressor
{
public:
	explicit CompactProLZHDecompressor(int blocksize);
	~CompactProLZHDecompressor();

private:
	bool nextLiteralOrOffset(int *offset, int *length, int &result) override;
	bool resetLZSSHandle() override;

	XADPrefixCode *allocAndParseCodeOfSize(int size);

	XADPrefixCode *literalcode, *lengthcode, *offsetcode;
	int blocksize, blockcount;
	IFileReader::FilePos_t blockstart;
};
