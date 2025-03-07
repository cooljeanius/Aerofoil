#include "StuffItArsenicDecompressor.h"
#include "CRC.h"
#include "CSInputBuffer.h"

#include <stdlib.h>
#include <string.h>



static const uint16_t RandomizationTable[] =
{
	0xee,  0x56,  0xf8,  0xc3,  0x9d,  0x9f,  0xae,  0x2c,
	0xad,  0xcd,  0x24,  0x9d,  0xa6, 0x101,  0x18,  0xb9,
	0xa1,  0x82,  0x75,  0xe9,  0x9f,  0x55,  0x66,  0x6a,
	0x86,  0x71,  0xdc,  0x84,  0x56,  0x96,  0x56,  0xa1,
	0x84,  0x78,  0xb7,  0x32,  0x6a,   0x3,  0xe3,   0x2,
	0x11, 0x101,   0x8,  0x44,  0x83, 0x100,  0x43,  0xe3,
	0x1c,  0xf0,  0x86,  0x6a,  0x6b,   0xf,   0x3,  0x2d,
	0x86,  0x17,  0x7b,  0x10,  0xf6,  0x80,  0x78,  0x7a,
	0xa1,  0xe1,  0xef,  0x8c,  0xf6,  0x87,  0x4b,  0xa7,
	0xe2,  0x77,  0xfa,  0xb8,  0x81,  0xee,  0x77,  0xc0,
	0x9d,  0x29,  0x20,  0x27,  0x71,  0x12,  0xe0,  0x6b,
	0xd1,  0x7c,   0xa,  0x89,  0x7d,  0x87,  0xc4, 0x101,
	0xc1,  0x31,  0xaf,  0x38,   0x3,  0x68,  0x1b,  0x76,
	0x79,  0x3f,  0xdb,  0xc7,  0x1b,  0x36,  0x7b,  0xe2,
	0x63,  0x81,  0xee,   0xc,  0x63,  0x8b,  0x78,  0x38,
	0x97,  0x9b,  0xd7,  0x8f,  0xdd,  0xf2,  0xa3,  0x77,
	0x8c,  0xc3,  0x39,  0x20,  0xb3,  0x12,  0x11,   0xe,
	0x17,  0x42,  0x80,  0x2c,  0xc4,  0x92,  0x59,  0xc8,
	0xdb,  0x40,  0x76,  0x64,  0xb4,  0x55,  0x1a,  0x9e,
	0xfe,  0x5f,   0x6,  0x3c,  0x41,  0xef,  0xd4,  0xaa,
	0x98,  0x29,  0xcd,  0x1f,   0x2,  0xa8,  0x87,  0xd2,
	0xa0,  0x93,  0x98,  0xef,   0xc,  0x43,  0xed,  0x9d,
	0xc2,  0xeb,  0x81,  0xe9,  0x64,  0x23,  0x68,  0x1e,
	0x25,  0x57,  0xde,  0x9a,  0xcf,  0x7f,  0xe5,  0xba,
	0x41,  0xea,  0xea,  0x36,  0x1a,  0x28,  0x79,  0x20,
	0x5e,  0x18,  0x4e,  0x7c,  0x8e,  0x58,  0x7a,  0xef,
	0x91,   0x2,  0x93,  0xbb,  0x56,  0xa1,  0x49,  0x1b,
	0x79,  0x92,  0xf3,  0x58,  0x4f,  0x52,  0x9c,   0x2,
	0x77,  0xaf,  0x2a,  0x8f,  0x49,  0xd0,  0x99,  0x4d,
	0x98, 0x101,  0x60,  0x93, 0x100,  0x75,  0x31,  0xce,
	0x49,  0x20,  0x56,  0x57,  0xe2,  0xf5,  0x26,  0x2b,
	0x8a,  0xbf,  0xde,  0xd0,  0x83,  0x34,  0xf4,  0x17
};


// Arithmetic decoder model

void StuffItArsenicDecompressor::InitializeArithmeticModel(ArithmeticModel *model,int firstsymbol,int lastsymbol,int increment,int frequencylimit)
{
	model->increment=increment;
	model->frequencylimit=frequencylimit;
	model->numsymbols=lastsymbol-firstsymbol+1;
	for(int i=0;i<model->numsymbols;i++) model->symbols[i].symbol=i+firstsymbol;

	ResetArithmeticModel(model);
}

void StuffItArsenicDecompressor::ResetArithmeticModel(ArithmeticModel *model)
{
	model->totalfrequency=model->increment*model->numsymbols;
	for(int i=0;i<model->numsymbols;i++) model->symbols[i].frequency=model->increment;
}

void StuffItArsenicDecompressor::IncreaseArithmeticModelFrequency(ArithmeticModel *model,int symindex)
{
	model->symbols[symindex].frequency+=model->increment;
	model->totalfrequency+=model->increment;

	if(model->totalfrequency>model->frequencylimit)
	{
		model->totalfrequency=0;
		for(int i=0;i<model->numsymbols;i++)
		{
			model->symbols[i].frequency++;
			model->symbols[i].frequency>>=1;
			model->totalfrequency+=model->symbols[i].frequency;
		}
	}
}



// Arithmetic decoder

#define NumBits 26
#define One (1<<(NumBits-1))
#define Half (1<<(NumBits-2))

bool StuffItArsenicDecompressor::InitializeArithmeticDecoder(ArithmeticDecoder *decoder,CSInputBuffer *input)
{
	unsigned int initialCode;
	if (!CSInputNextLongBitString(input, NumBits, initialCode))
		return false;
	decoder->input=input;
	decoder->range=One;
	decoder->code = initialCode;

	return true;
}

bool StuffItArsenicDecompressor::ReadNextArithmeticCode(ArithmeticDecoder *decoder,int symlow,int symsize,int symtot)
{
	int renorm_factor=decoder->range/symtot;
	int lowincr=renorm_factor*symlow;

	decoder->code-=lowincr;
	if(symlow+symsize==symtot) decoder->range-=lowincr;
	else decoder->range=symsize*renorm_factor;

	while(decoder->range<=Half)
	{
		decoder->range<<=1;

		unsigned int bit;
		if (!CSInputNextBit(decoder->input, bit))
			return false;

		decoder->code = (decoder->code << 1) | bit;
	}

	return true;
}

bool StuffItArsenicDecompressor::NextArithmeticSymbol(ArithmeticDecoder *decoder,ArithmeticModel *model,int &outSymbol)
{
	int frequency=decoder->code/(decoder->range/model->totalfrequency);
	int cumulative=0,n;
	for(n=0;n<model->numsymbols-1;n++)
	{
		if(cumulative+model->symbols[n].frequency>frequency) break;
		cumulative+=model->symbols[n].frequency;
	}

	if (!ReadNextArithmeticCode(decoder, cumulative, model->symbols[n].frequency, model->totalfrequency))
		return false;

	IncreaseArithmeticModelFrequency(model,n);

	outSymbol = model->symbols[n].symbol;
	return true;
}

bool StuffItArsenicDecompressor::NextArithmeticBitString(ArithmeticDecoder *decoder,ArithmeticModel *model,int bits,int &outBits)
{
	int res=0;
	for (int i = 0; i < bits; i++)
	{
		int sym;
		if (!NextArithmeticSymbol(decoder, model, sym))
			return false;

		if (sym)
			res |= 1 << i;
	}
	outBits = res;

	return true;
}

StuffItArsenicDecompressor::StuffItArsenicDecompressor()
	: block(NULL)
	, transform(NULL)
{
}


StuffItArsenicDecompressor::~StuffItArsenicDecompressor()
{
	delete[] block;
	delete[] transform;
}

bool StuffItArsenicDecompressor::Reset(CSInputBuffer *input, size_t compressedSize, size_t decompressedSize)
{
	if (!InitializeArithmeticDecoder(&decoder, input))
		return false;

	InitializeArithmeticModel(&initialmodel,0,1,1,256);
	InitializeArithmeticModel(&selectormodel,0,10,8,1024);
	InitializeArithmeticModel(&mtfmodel[0],2,3,8,1024);
	InitializeArithmeticModel(&mtfmodel[1],4,7,4,1024);
	InitializeArithmeticModel(&mtfmodel[2],8,15,4,1024);
	InitializeArithmeticModel(&mtfmodel[3],16,31,4,1024);
	InitializeArithmeticModel(&mtfmodel[4],32,63,2,1024);
	InitializeArithmeticModel(&mtfmodel[5],64,127,2,1024);
	InitializeArithmeticModel(&mtfmodel[6],128,255,1,1024);

	int bits;
	if (!NextArithmeticBitString(&decoder, &initialmodel, 8, bits) || bits != 'A')
		return false;
	if (!NextArithmeticBitString(&decoder, &initialmodel, 8, bits) || bits != 's')
		return false;

	if (!NextArithmeticBitString(&decoder, &initialmodel, 4, blockbits))
		return false;

	blockbits+=9;
	blocksize=1<<blockbits;
	numbytes=bytecount=0;
	repeat=0;

	delete[] block;
	block=new uint8_t[blocksize];

	crc=0xffffffff;
	compcrc=0;

	// check first end marker
	int eobsym;
	if (!NextArithmeticSymbol(&decoder, &initialmodel, eobsym))
		return false;

	endofblocks = (eobsym != 0);

	return true;
}

bool StuffItArsenicDecompressor::ReadBlock()
{
	ResetMTFDecoder(&mtf);

	if (!NextArithmeticSymbol(&decoder, &initialmodel, randomized))
		return false;
	if (!NextArithmeticBitString(&decoder, &initialmodel, blockbits, transformindex))
		return false;
	numbytes=0;

	for(;;)
	{
		int sel;
		if (!NextArithmeticSymbol(&decoder, &selectormodel, sel))
			return false;

		if(sel==0||sel==1) // zero counting
		{
			int zerostate=1,zerocount=0;
			while(sel<2)
			{
				if(sel==0) zerocount+=zerostate;
				else if(sel==1) zerocount+=2*zerostate;
				zerostate*=2;
				if (!NextArithmeticSymbol(&decoder, &selectormodel, sel))
					return false;
			}

			if (numbytes + zerocount > blocksize)
				return false;

			memset(&block[numbytes],DecodeMTF(&mtf,0),zerocount);
			numbytes+=zerocount;
		}

		int symbol;
		if(sel==10)
			break;
		else if(sel==2)
			symbol=1;
		else
		{
			if (!NextArithmeticSymbol(&decoder, &mtfmodel[sel - 3], symbol))
				return false;
		}

		if (numbytes >= blocksize)
			return false;

		block[numbytes++] = DecodeMTF(&mtf, symbol);
	}

	if (transformindex >= numbytes)
		return false;

	ResetArithmeticModel(&selectormodel);
	for(int i=0;i<7;i++) ResetArithmeticModel(&mtfmodel[i]);

	int endMarkerSym;
	if (!NextArithmeticSymbol(&decoder, &initialmodel, endMarkerSym))
		return false;

	if(endMarkerSym) // end marker
	{
		int compcrcsym;
		if (!NextArithmeticBitString(&decoder, &initialmodel, 32, compcrcsym))
			return false;
		compcrc = compcrcsym;
		endofblocks=true;
	}

	delete[] transform;
	transform = new uint32_t[numbytes];
	CalculateInverseBWT(transform,block,numbytes);

	return true;
}

bool StuffItArsenicDecompressor::ReadBytes(void *dest, size_t numBytes)
{
	while (numBytes)
	{
		uint8_t outbyte;

		if (repeat)
		{
			repeat--;
			outbyte = last;
		}
		else
		{
		retry:
			if (bytecount >= numbytes)
			{
				if (endofblocks)
					return false;

				if (!this->ReadBlock())
					return false;

				bytecount = 0;
				count = 0;
				last = 0;

				randindex = 0;
				randcount = RandomizationTable[0];
			}

			transformindex = transform[transformindex];
			int byte = block[transformindex];

			if (randomized&&randcount == bytecount)
			{
				byte ^= 1;
				randindex = (randindex + 1) & 255;
				randcount += RandomizationTable[randindex];
			}

			bytecount++;

			if (count == 4)
			{
				count = 0;
				if (byte == 0) goto retry;
				repeat = byte - 1;
				outbyte = last;
			}
			else
			{
				if (byte == last) count++;
				else { count = 1; last = byte; }
				outbyte = byte;
			}
		}

		crc = XADCRC(crc, outbyte, XADCRCTable_edb88320);

		uint8_t *destBytes = static_cast<uint8_t*>(dest);
		destBytes[0] = outbyte;

		numBytes--;
		dest = destBytes + 1;
	}

	return true;
}
