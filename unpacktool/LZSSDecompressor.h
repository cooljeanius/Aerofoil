#pragma once

#include "IDecompressor.h"

#include <stdint.h>

#define XADLZSSMatch -1
#define XADLZSSEnd -2


class LZSSDecompressor : public IDecompressor
{
public:
	explicit LZSSDecompressor(int windowsize);
	~LZSSDecompressor();

	bool Reset(CSInputBuffer *input, size_t compressedSize, size_t decompressedSize) override;
	bool ReadBytes(void *dest, size_t numBytes) override;

	uint8_t ByteFromWindow(size_t absolutepos);

private:
	virtual bool nextLiteralOrOffset(int *offset, int *length, int &result) = 0;
	virtual bool resetLZSSHandle() = 0;

	bool ReadOneByte(uint8_t &b);

	uint8_t *windowbuffer;
	int windowmask,matchlength,matchoffset;
	int windowsize;
	int pos;

protected:
	CSInputBuffer *input;
};

inline uint8_t LZSSDecompressor::ByteFromWindow(size_t absolutepos)
{
	return this->windowbuffer[absolutepos&this->windowmask];
}
