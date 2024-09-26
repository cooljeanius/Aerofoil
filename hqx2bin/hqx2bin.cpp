#include <stdio.h>

/*
Copyright 2019 Eric Lasota

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "CFileStream.h"
#include "ScopedPtr.h"
#include "BinHex4.h"
#include "MacBinary2.h"
#include "MacFileMem.h"
#include "GpAllocator_C.h"

using namespace PortabilityLayer;

int main(int argc, const char **argv)
{
	if (argc != 3)
	{
		fprintf(stderr, "Usage: hqx2bin <input.hqx> <output.bin>");
		return -1;
	}

#ifdef _CRT_INSECURE_DEPRECATE
	FILE *f = nullptr;
	if (fopen_s(&f, argv[1], "rb"))
		f = nullptr;
#else
	FILE *f = fopen(argv[1], "rb");
#endif

	if (!f)
	{
		fprintf(stderr, "Could not open input file");
		return -1;
	}

	CFileStream fs(f, true, false, true);

	ScopedPtr<MacFileMem> memFile = BinHex4::LoadHQX(&fs, GpAllocator_C::GetInstance());

	fs.Close();

#ifdef _CRT_INSECURE_DEPRECATE
	FILE *outF = nullptr;
	if (fopen_s(&outF, argv[2], "wb"))
		outF = nullptr;
#else
	FILE *outF = fopen(argv[2], "wb");
#endif

	CFileStream outFS(outF, false, true, true);

	MacBinary2::WriteBin(memFile, &outFS);

	outFS.Close();

	return 0;
}
