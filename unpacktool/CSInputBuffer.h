#pragma once

#include <stdint.h>

#include "UPByteSwap.h"
#include "IFileReader.h"

struct IFileReader;

struct CSInputBuffer
{
	IFileReader *parent;
	IFileReader::FilePos_t startoffs;
	bool eof;

	uint8_t *buffer;
	unsigned int bufsize, bufbytes, currbyte;

	uint32_t bits;
	unsigned int numbits;
};



// Allocation and management

CSInputBuffer *CSInputBufferAlloc(IFileReader *parent, int size);
CSInputBuffer *CSInputBufferAllocWithBuffer(const uint8_t *buffer, int length, IFileReader::FilePos_t startoffs);
CSInputBuffer *CSInputBufferAllocEmpty();
void CSInputBufferFree(CSInputBuffer *self);

void CSInputSetMemoryBuffer(CSInputBuffer *self, uint8_t *buffer, int length, size_t startoffs);

static inline IFileReader *CSInputHandle(CSInputBuffer *self)
{
	return self->parent;
}



// Buffer and file positioning

void CSInputRestart(CSInputBuffer *self);
void CSInputFlush(CSInputBuffer *self);

void CSInputSynchronizeFileOffset(CSInputBuffer *self);
void CSInputSeekToFileOffset(CSInputBuffer *self, IFileReader::FilePos_t offset);
void CSInputSeekToBufferOffset(CSInputBuffer *self, IFileReader::FilePos_t offset);
void CSInputSetStartOffset(CSInputBuffer *self, IFileReader::FilePos_t offset);
IFileReader::FilePos_t CSInputBufferOffset(CSInputBuffer *self);
IFileReader::FilePos_t CSInputFileOffset(CSInputBuffer *self);
IFileReader::FilePos_t CSInputBufferBitOffset(CSInputBuffer *self);

void _CSInputFillBuffer(CSInputBuffer *self);




// Byte reading

#define CSInputBufferLookAhead 4

static inline int _CSInputBytesLeftInBuffer(CSInputBuffer *self)
{
	return self->bufbytes - self->currbyte;
}

static inline void _CSInputCheckAndFillBuffer(CSInputBuffer *self)
{
	if (!self->eof&&_CSInputBytesLeftInBuffer(self) <= CSInputBufferLookAhead) _CSInputFillBuffer(self);
}

static inline bool CSInputSkipBytes(CSInputBuffer *self, int num)
{
	self->currbyte += num;

	return true;
}

static inline int _CSInputPeekByteWithoutEOF(CSInputBuffer *self, int offs)
{
	return self->buffer[self->currbyte + offs];
}

static inline bool CSInputPeekByte(CSInputBuffer *self, int offs, int &byte)
{
	_CSInputCheckAndFillBuffer(self);
	if (offs >= _CSInputBytesLeftInBuffer(self))
	{
		byte = -1;
		return true;
	}
	byte = _CSInputPeekByteWithoutEOF(self, offs);
	return true;
}

static inline bool CSInputNextByte(CSInputBuffer *self, int &byte)
{
	if (!CSInputPeekByte(self, 0, byte))
		return false;
	if (!CSInputSkipBytes(self, 1))
		return false;
	return true;
}

static inline bool CSInputAtEOF(CSInputBuffer *self, bool &isEOF)
{
	_CSInputCheckAndFillBuffer(self);
	isEOF = _CSInputBytesLeftInBuffer(self) <= 0;
	return true;
}




// Bitstream reading

bool _CSInputFillBits(CSInputBuffer *self);
bool _CSInputFillBitsLE(CSInputBuffer *self);

bool CSInputNextBit(CSInputBuffer *self, unsigned int &bit);
bool CSInputNextBitLE(CSInputBuffer *self, unsigned int &bit);
bool CSInputNextBitString(CSInputBuffer *self, int numbits, unsigned int &bits);
bool CSInputNextBitStringLE(CSInputBuffer *self, int numbits, unsigned int &bits);
bool CSInputNextLongBitString(CSInputBuffer *self, int numbits, unsigned int &bits);
bool CSInputNextLongBitStringLE(CSInputBuffer *self, int numbits, unsigned int &bits);

bool CSInputSkipBits(CSInputBuffer *self, int numbits);
bool CSInputSkipBitsLE(CSInputBuffer *self, int numbits);
bool CSInputOnByteBoundary(CSInputBuffer *self);
bool CSInputSkipToByteBoundary(CSInputBuffer *self);
bool CSInputSkipTo16BitBoundary(CSInputBuffer *self);

static inline unsigned int CSInputBitsLeftInBuffer(CSInputBuffer *self)
{
	_CSInputCheckAndFillBuffer(self);
	return _CSInputBytesLeftInBuffer(self) * 8 + (self->numbits & 7);
}

static inline bool _CSInputCheckAndFillBits(CSInputBuffer *self, int numbits)
{
	if (static_cast<unsigned int>(numbits) > self->numbits)
		return _CSInputFillBits(self);
	return true;
}

static inline bool _CSInputCheckAndFillBitsLE(CSInputBuffer *self, int numbits)
{
	if (static_cast<unsigned int>(numbits) > self->numbits)
		return _CSInputFillBitsLE(self);
	return true;
}

static inline bool CSInputPeekBitString(CSInputBuffer *self, int numbits, unsigned int &bits)
{
	if (numbits == 0)
	{
		bits = 0;
		return true;
	}
	if (!_CSInputCheckAndFillBits(self, numbits))
		return false;
	bits = self->bits >> (32 - numbits);
	return true;
}

static inline bool CSInputPeekBitStringLE(CSInputBuffer *self, int numbits, unsigned int &bits)
{
	if (numbits == 0)
	{
		bits = 0;
		return true;
	}
	if (!_CSInputCheckAndFillBitsLE(self, numbits))
		return false;
	bits = self->bits&((1 << numbits) - 1);
	return true;
}

static inline bool CSInputSkipPeekedBits(CSInputBuffer *self, int numbits)
{
	int numbytes = (numbits - (self->numbits & 7) + 7) >> 3;
	CSInputSkipBytes(self, numbytes);

	if (_CSInputBytesLeftInBuffer(self) < 0)
		return false;

	self->bits <<= numbits;
	self->numbits -= numbits;

	return true;
}

static inline bool CSInputSkipPeekedBitsLE(CSInputBuffer *self, int numbits)
{
	int numbytes = (numbits - (self->numbits & 7) + 7) >> 3;
	CSInputSkipBytes(self, numbytes);

	if (_CSInputBytesLeftInBuffer(self) < 0)
		return false;

	self->bits >>= numbits;
	self->numbits -= numbits;

	return true;
}




// Multibyte reading

#define CSInputNextValueImpl(type,name,conv) \
static inline type name(CSInputBuffer *self) \
{ \
	_CSInputCheckAndFillBuffer(self); \
	type val=conv(self->buffer+self->currbyte); \
	CSInputSkipBytes(self,sizeof(type)); \
	return val; \
}

CSInputNextValueImpl(int16_t, CSInputNextInt16LE, ParseInt16LE)
CSInputNextValueImpl(int32_t, CSInputNextInt32LE, ParseInt32LE)
CSInputNextValueImpl(uint16_t, CSInputNextUInt16LE, ParseUInt16LE)
CSInputNextValueImpl(uint32_t, CSInputNextUInt32LE, ParseUInt32LE)
CSInputNextValueImpl(int16_t, CSInputNextInt16BE, ParseInt16BE)
CSInputNextValueImpl(int32_t, CSInputNextInt32BE, ParseInt32BE)
CSInputNextValueImpl(uint16_t, CSInputNextUInt16BE, ParseUInt16BE)
CSInputNextValueImpl(uint32_t, CSInputNextUInt32BE, ParseUInt32BE)



