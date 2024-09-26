#pragma once

#include "IDecompressor.h"
#include "BWT.h"


class StuffItArsenicDecompressor : public IDecompressor
{
public:
	StuffItArsenicDecompressor();
	~StuffItArsenicDecompressor() override;

	bool Reset(CSInputBuffer *reader, size_t compressedSize, size_t decompressedSize) override;
	bool ReadBytes(void *dest, size_t numBytes) override;

private:
	struct ArithmeticSymbol
	{
		int symbol;
		int frequency;
	};

	struct ArithmeticModel
	{
		int totalfrequency;
		int increment;
		int frequencylimit;

		int numsymbols;
		ArithmeticSymbol symbols[128];
	};

	struct ArithmeticDecoder
	{
		CSInputBuffer *input;
		int range, code;
	};

	ArithmeticModel initialmodel, selectormodel, mtfmodel[7];
	ArithmeticDecoder decoder;
	MTFState mtf;

	int blockbits, blocksize;
	uint8_t *block;
	bool endofblocks;

	int numbytes, bytecount, transformindex;
	uint32_t *transform;

	int randomized, randcount, randindex;

	int repeat, count, last;

	uint32_t crc, compcrc;

	static void ResetArithmeticModel(ArithmeticModel *model);
	static void InitializeArithmeticModel(ArithmeticModel *model, int firstsymbol, int lastsymbol, int increment, int frequencylimit);
	static void IncreaseArithmeticModelFrequency(ArithmeticModel *model, int symindex);
	static bool InitializeArithmeticDecoder(ArithmeticDecoder *decoder, CSInputBuffer *input);
	static bool ReadNextArithmeticCode(ArithmeticDecoder *decoder, int symlow, int symsize, int symtot);
	static bool NextArithmeticSymbol(ArithmeticDecoder *decoder, ArithmeticModel *model, int &outSym);
	static bool NextArithmeticBitString(ArithmeticDecoder *decoder, ArithmeticModel *model, int bits, int &outBitString);

	bool ReadBlock();
};
