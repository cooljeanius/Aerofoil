#include "BMPFormat.h"
#include "CFileStream.h"
#include "CombinedTimestamp.h"
#include "GPArchive.h"
#include "GpAllocator_C.h"
#include "MacRomanConversion.h"
#include "MemReaderStream.h"
#include "QDPictDecoder.h"
#include "QDPictEmitContext.h"
#include "QDPictEmitScanlineParameters.h"
#include "QDStandardPalette.h"
#include "MacFileInfo.h"
#include "ResourceFile.h"
#include "ResourceCompiledTypeList.h"
#include "SharedTypes.h"
#include "UTF8.h"
#include "ZipFile.h"
#include "WaveFormat.h"
#include "GpUnicode.h"
#include "PLDrivers.h"

#include "zlib.h"

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

#include "WindowsUnicodeToolShim.h"

#include "macedec.h"

#include <stdio.h>
#include <string>
#include <sstream>
#include <utility>
#include <vector>
#include <algorithm>

enum AudioCompressionCodecID
{
	AudioCompressionCodecID_VariableRate = -2,
	AudioCompressionCodecID_FixedRate = -1,
	AudioCompressionCodecID_Uncompressed = 0,
	AudioCompressionCodecID_TwoToOne = 1,
	AudioCompressionCodecID_EightToOne = 2,
	AudioCompressionCodecID_ThreeToOne = 3,
	AudioCompressionCodecID_SixToOne = 4,
};

struct PlannedEntry
{
	std::vector<uint8_t> m_uncompressedContents;
	std::vector<uint8_t> m_compressedContents;

	std::string m_name;
	std::string m_comment;
	bool m_isDirectory;

	PlannedEntry()
		: m_isDirectory(false)
	{
	}
};

bool EntryAlphaSortPredicate(const PlannedEntry &a, const PlannedEntry &b)
{
	return a.m_name < b.m_name;
}

void AppendStr(std::vector<uint8_t> &array, const char *str)
{
	size_t appendSize = strlen(str);

	if (appendSize == 0)
		return;

	size_t existingSize = array.size();
	if (SIZE_MAX - existingSize < appendSize)
		return;

	array.resize(existingSize + appendSize);
	memcpy(&array[existingSize], str, appendSize);
}

void AppendQuotedStr(std::vector<uint8_t> &array, const char *str)
{
	array.push_back('\"');
	AppendStr(array, str);
	array.push_back('\"');
}

void AppendFmt(std::vector<uint8_t> &array, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);

	int resultSize = vsnprintf(nullptr, 0, fmt, args);
	if (resultSize <= 0)
		return;

	// vsnprintf invalidates the va_list, so we need to
	// reinit args so the next call doesn't print garbage.
	va_end(args);
	va_start(args, fmt);

	size_t appendSize = static_cast<size_t>(resultSize);

	if (SIZE_MAX == appendSize)
		return;

	appendSize++;

	size_t existingSize = array.size();
	if (SIZE_MAX - existingSize < appendSize)
		return;

	array.resize(existingSize + appendSize);
	vsnprintf(reinterpret_cast<char*>(static_cast<uint8_t*>(&array[existingSize])), appendSize, fmt, args);

	// Discard trailing null
	array.pop_back();

	va_end(args);
}

void AppendUTF8(std::vector<uint8_t> &array, uint32_t codePoint)
{
	uint8_t bytes[5];
	size_t sz;

	GpUnicode::UTF8::Encode(bytes, sz, codePoint);
	for (size_t i = 0; i < sz; i++)
		array.push_back(bytes[i]);
}

 template<class T>
 void VectorAppend(std::vector<T> &vec, const T *items, size_t numItems)
 {
	 if (numItems == 0)
		 return;

	 vec.resize(vec.size() + numItems);

	 T *outLoc = &vec[vec.size() - numItems];
	 for (size_t i = 0; i < numItems; i++)
		 outLoc[i] = items[i];
 }

uint8_t FiveToEight(uint8_t v)
{
	return static_cast<uint8_t>((v << 3) | (v >> 2));
}

bool TryDeflate(const std::vector<uint8_t> &uncompressed, std::vector<uint8_t> &outCompressed)
{
	if (uncompressed.size() == 0)
		return false;

	const unsigned int bufferSize = 1024;

	z_stream stream;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	int ret = deflateInit2(&stream, 9, Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY);
	if (ret != Z_OK)
		return false;

	uint8_t buffer[bufferSize];

	stream.next_in = const_cast<unsigned char*>(&uncompressed[0]);
	stream.avail_in = uncompressed.size();
	stream.next_out = buffer;
	stream.avail_out = bufferSize;

	for (;;)
	{
		int deflateResult = deflate(&stream, Z_FINISH);

		size_t newDataSize = stream.next_out - buffer;
		if (newDataSize > 0)
		{
			outCompressed.resize(outCompressed.size() + newDataSize);
			memcpy(&outCompressed[outCompressed.size() - newDataSize], buffer, newDataSize);

			stream.next_out = buffer;
			stream.avail_out = bufferSize;
		}

		if (deflateResult == Z_STREAM_END)
			break;
	}

	deflateEnd(&stream);

	return true;
}

void ExportZipFile(const char *path, std::vector<PlannedEntry> &entries, const PortabilityLayer::CombinedTimestamp &ts)
{
	FILE *outF = fopen_utf8(path, "wb");
	if (!outF)
	{
		fprintf(stderr, "Error opening output path");
		return;
	}

	uint16_t msdosModificationTime = 0;
	uint16_t msdosModificationDate = 0;

	ts.GetAsMSDOSTimestamp(msdosModificationDate, msdosModificationTime);

	std::vector<PortabilityLayer::ZipCentralDirectoryFileHeader> cdirRecords;

	// Why does OMP require signed indexes?  When do I ever want negative iterations?  Uggghh.
	int numEntries = entries.size();

#pragma omp parallel for
	for (int i = 0; i < numEntries; i++)
	{
		PlannedEntry &entry = entries[i];

		if (entry.m_uncompressedContents.size() > 0)
		{
			if (!TryDeflate(entry.m_uncompressedContents, entry.m_compressedContents))
				entry.m_compressedContents.resize(0);

			if (entry.m_compressedContents.size() >= entry.m_uncompressedContents.size())
				entry.m_compressedContents.resize(0);
		}
	}

	for (const PlannedEntry &entry : entries)
	{
		bool isCompressed = entry.m_compressedContents.size() != 0;

		PortabilityLayer::ZipCentralDirectoryFileHeader cdirHeader;

		cdirHeader.m_signature = PortabilityLayer::ZipCentralDirectoryFileHeader::kSignature;
		cdirHeader.m_versionCreated = PortabilityLayer::ZipConstants::kCompressedRequiredVersion;
		cdirHeader.m_versionRequired = PortabilityLayer::ZipConstants::kStoredRequiredVersion;
		cdirHeader.m_flags = 0;
		cdirHeader.m_method = isCompressed ? PortabilityLayer::ZipConstants::kDeflatedMethod : PortabilityLayer::ZipConstants::kStoredMethod;
		cdirHeader.m_modificationTime = msdosModificationTime;
		cdirHeader.m_modificationDate = msdosModificationDate;
		cdirHeader.m_crc = 0;

		if (entry.m_isDirectory)
			cdirHeader.m_versionRequired = PortabilityLayer::ZipConstants::kDirectoryRequiredVersion;
		else if (isCompressed)
			cdirHeader.m_versionRequired = PortabilityLayer::ZipConstants::kCompressedRequiredVersion;

		if (entry.m_uncompressedContents.size() > 0)
			cdirHeader.m_crc = crc32(0, &entry.m_uncompressedContents[0], static_cast<uint32_t>(entry.m_uncompressedContents.size()));

		cdirHeader.m_compressedSize = static_cast<uint32_t>(isCompressed ? entry.m_compressedContents.size() : entry.m_uncompressedContents.size());
		cdirHeader.m_uncompressedSize = static_cast<uint32_t>(entry.m_uncompressedContents.size());
		cdirHeader.m_fileNameLength = static_cast<uint32_t>(entry.m_name.size());
		cdirHeader.m_extraFieldLength = 0;
		cdirHeader.m_commentLength = static_cast<uint32_t>(entry.m_comment.size());
		cdirHeader.m_diskNumber = 0;
		cdirHeader.m_internalAttributes = 0;
		cdirHeader.m_externalAttributes = entry.m_isDirectory ? PortabilityLayer::ZipConstants::kDirectoryAttributes : PortabilityLayer::ZipConstants::kArchivedAttributes;
		cdirHeader.m_localHeaderOffset = ftell(outF);

		cdirRecords.push_back(cdirHeader);

		PortabilityLayer::ZipFileLocalHeader localHeader;

		localHeader.m_signature = PortabilityLayer::ZipFileLocalHeader::kSignature;
		localHeader.m_versionRequired = cdirHeader.m_versionRequired;
		localHeader.m_flags = 0;
		localHeader.m_method = cdirHeader.m_method;
		localHeader.m_modificationTime = cdirHeader.m_modificationTime;
		localHeader.m_modificationDate = cdirHeader.m_modificationDate;
		localHeader.m_crc = cdirHeader.m_crc;
		localHeader.m_compressedSize = cdirHeader.m_compressedSize;
		localHeader.m_uncompressedSize = cdirHeader.m_uncompressedSize;
		localHeader.m_fileNameLength = cdirHeader.m_fileNameLength;
		localHeader.m_extraFieldLength = 0;

		fwrite(&localHeader, 1, sizeof(localHeader), outF);

		fwrite(entry.m_name.c_str(), 1, entry.m_name.size(), outF);

		if (isCompressed)
			fwrite(&entry.m_compressedContents[0], 1, entry.m_compressedContents.size(), outF);
		else if (entry.m_uncompressedContents.size() > 0)
			fwrite(&entry.m_uncompressedContents[0], 1, entry.m_uncompressedContents.size(), outF);
	}

	long cdirPos = ftell(outF);

	for (size_t i = 0; i < entries.size(); i++)
	{
		fwrite(&cdirRecords[i], 1, sizeof(PortabilityLayer::ZipCentralDirectoryFileHeader), outF);
		fwrite(entries[i].m_name.c_str(), 1, entries[i].m_name.size(), outF);
		fwrite(entries[i].m_comment.c_str(), 1, entries[i].m_comment.size(), outF);
	}

	long cdirEndPos = ftell(outF);

	PortabilityLayer::ZipEndOfCentralDirectoryRecord endRecord;

	endRecord.m_signature = PortabilityLayer::ZipEndOfCentralDirectoryRecord::kSignature;
	endRecord.m_thisDiskNumber = 0;
	endRecord.m_centralDirDisk = 0;
	endRecord.m_numCentralDirRecordsThisDisk = static_cast<uint32_t>(entries.size());
	endRecord.m_numCentralDirRecords = static_cast<uint32_t>(entries.size());
	endRecord.m_centralDirectorySizeBytes = cdirEndPos - cdirPos;
	endRecord.m_centralDirStartOffset = cdirPos;
	endRecord.m_commentLength = 0;

	fwrite(&endRecord, 1, sizeof(endRecord), outF);

	fclose(outF);
}



bool ExportBMP(size_t width, size_t height, size_t pitchInElements, const PortabilityLayer::RGBAColor *pixelData, std::vector<uint8_t> &outData)
{
	outData.resize(0);

	bool couldBe15Bit = true;
	bool couldBeIndexed = true;

	PortabilityLayer::BitmapColorTableEntry colorTable[256];
	unsigned int numColors = 0;

	for (size_t row = 0; row < height; row++)
	{
		const PortabilityLayer::RGBAColor *rowData = &pixelData[pitchInElements * row];

		for (size_t col = 0; col < width; col++)
		{
			const PortabilityLayer::RGBAColor &pixel = rowData[col];

			if (couldBe15Bit)
			{
				if (FiveToEight(pixel.r >> 3) != pixel.r || FiveToEight(pixel.g >> 3) != pixel.g || FiveToEight(pixel.b >> 3) != pixel.b)
				{
					couldBe15Bit = false;
					if (!couldBeIndexed)
						break;
				}
			}

			if (couldBeIndexed)
			{
				bool matched = false;
				for (unsigned int ci = 0; ci < numColors; ci++)
				{
					const PortabilityLayer::BitmapColorTableEntry &ctabEntry = colorTable[ci];

					if (ctabEntry.m_r == pixel.r && ctabEntry.m_g == pixel.g && ctabEntry.m_b == pixel.b)
					{
						matched = true;
						break;
					}
				}

				if (matched == false)
				{
					if (numColors == 256)
					{
						couldBeIndexed = false;
						if (!couldBe15Bit)
							break;
					}
					else
					{
						PortabilityLayer::BitmapColorTableEntry &ctabEntry = colorTable[numColors++];
						ctabEntry.m_r = pixel.r;
						ctabEntry.m_g = pixel.g;
						ctabEntry.m_b = pixel.b;
						ctabEntry.m_reserved = 0;
					}
				}
			}
		}

		if (!couldBeIndexed && !couldBe15Bit)
			break;
	}

	unsigned int bpp = 24;
	if (couldBeIndexed)
	{
		if (numColors <= 2)
			bpp = 1;
		else if (numColors <= 16)
			bpp = 4;
		else
			bpp = 8;
	}
	else if (couldBe15Bit)
		bpp = 16;

	const size_t minimalBitsPerRow = bpp * width;
	const size_t rowPitchBytes = ((minimalBitsPerRow + 31) / 32) * 4;	// DWORD alignment

	const size_t colorTableSize = (bpp < 16) ? numColors * 4 : 0;
	const size_t fileHeaderSize = sizeof(PortabilityLayer::BitmapFileHeader);
	const size_t infoHeaderSize = sizeof(PortabilityLayer::BitmapInfoHeader);
	const size_t ctabSize = (bpp < 16) ? (numColors * 4) : 0;
	const size_t imageDataSize = rowPitchBytes * height;
	const size_t postCTabPaddingSize = 2;
	const size_t imageFileSize = fileHeaderSize + infoHeaderSize + ctabSize + postCTabPaddingSize + imageDataSize;

	outData.reserve(imageFileSize);

	PortabilityLayer::BitmapFileHeader fileHeader;
	fileHeader.m_id[0] = 'B';
	fileHeader.m_id[1] = 'M';
	fileHeader.m_fileSize = static_cast<uint32_t>(imageFileSize);
	fileHeader.m_imageDataStart = static_cast<uint32_t>(fileHeaderSize + infoHeaderSize + ctabSize + postCTabPaddingSize);
	fileHeader.m_reserved1 = 0;
	fileHeader.m_reserved2 = 0;

	VectorAppend(outData, reinterpret_cast<const uint8_t*>(&fileHeader), sizeof(fileHeader));

	PortabilityLayer::BitmapInfoHeader infoHeader;
	infoHeader.m_thisStructureSize = sizeof(infoHeader);
	infoHeader.m_width = static_cast<uint32_t>(width);
	infoHeader.m_height = static_cast<uint32_t>(height);
	infoHeader.m_planes = 1;
	infoHeader.m_bitsPerPixel = bpp;
	infoHeader.m_compression = PortabilityLayer::BitmapConstants::kCompressionRGB;
	infoHeader.m_imageSize = static_cast<uint32_t>(imageDataSize);
	infoHeader.m_xPixelsPerMeter = 2835;
	infoHeader.m_yPixelsPerMeter = 2835;
	infoHeader.m_numColors = (bpp < 16) ? numColors : 0;
	infoHeader.m_importantColorCount = 0;

	VectorAppend(outData, reinterpret_cast<const uint8_t*>(&infoHeader), sizeof(infoHeader));

	if (bpp < 16)
		VectorAppend(outData, reinterpret_cast<const uint8_t*>(colorTable), sizeof(PortabilityLayer::BitmapColorTableEntry) * numColors);

	for (size_t i = 0; i < postCTabPaddingSize; i++)
		outData.push_back(0);

	std::vector<uint8_t> rowPackData;
	rowPackData.resize(rowPitchBytes);

	for (size_t row = 0; row < height; row++)
	{
		for (size_t i = 0; i < rowPitchBytes; i++)
			rowPackData[i] = 0;

		const PortabilityLayer::RGBAColor *rowData = &pixelData[pitchInElements * (height - 1 - row)];

		for (size_t col = 0; col < width; col++)
		{
			const PortabilityLayer::RGBAColor &pixel = rowData[col];

			if (bpp == 24)
			{
				rowPackData[col * 3 + 0] = pixel.b;
				rowPackData[col * 3 + 1] = pixel.g;
				rowPackData[col * 3 + 2] = pixel.r;
			}
			else if (bpp == 16)
			{
				int packedValue = 0;
				packedValue |= (pixel.b >> 3);
				packedValue |= ((pixel.g >> 3) << 5);
				packedValue |= ((pixel.r >> 3) << 10);

				rowPackData[col * 2 + 0] = static_cast<uint8_t>(packedValue & 0xff);
				rowPackData[col * 2 + 1] = static_cast<uint8_t>((packedValue >> 8) & 0xff);
			}
			else
			{
				unsigned int colorIndex = 0;
				for (unsigned int ci = 0; ci < numColors; ci++)
				{
					const PortabilityLayer::BitmapColorTableEntry &ctabEntry = colorTable[ci];

					if (ctabEntry.m_r == pixel.r && ctabEntry.m_g == pixel.g && ctabEntry.m_b == pixel.b)
					{
						colorIndex = ci;
						break;
					}
				}

				if (bpp == 8)
					rowPackData[col] = colorIndex;
				else if (bpp == 4)
					rowPackData[col / 2] |= (colorIndex << (8 - (((col & 1) + 1) * 4)));
				else if (bpp == 1)
				{
					if (colorIndex)
						rowPackData[col / 8] |= (0x80 >> (col & 7));
				}
			}
		}

		VectorAppend(outData, &rowPackData[0], rowPackData.size());
	}

	return true;
}

class BMPDumperContext : public PortabilityLayer::QDPictEmitContext
{
public:
	explicit BMPDumperContext(const char *dumpqtDir, int resID);

	bool SpecifyFrame(const Rect &rect) override;
	Rect ConstrainRegion(const Rect &rect) const override;
	void Start(PortabilityLayer::QDPictBlitSourceType sourceType, const PortabilityLayer::QDPictEmitScanlineParameters &params) override;
	void BlitScanlineAndAdvance(const void *) override;
	bool EmitQTContent(GpIOStream *stream, uint32_t dataSize, bool isCompressed) override;
	bool AllocTempBuffers(uint8_t *&buffer1, size_t buffer1Size, uint8_t *&buffer2, size_t buffer2Size) override;

	void ReportError(int errorCode, int subCode) override
	{
		fprintf(stderr, "PICT import failed, error code %i, subcode %i\n", errorCode, subCode);
	}

	bool Export(std::vector<uint8_t> &outData) const;

private:
	std::vector<PortabilityLayer::RGBAColor> m_pixelData;
	size_t m_blitOrigin;
	size_t m_pitchInElements;
	Rect m_frame;
	PortabilityLayer::QDPictEmitScanlineParameters m_blitParams;
	PortabilityLayer::QDPictBlitSourceType m_blitSourceType;

	const char *m_dumpqtDir;
	int m_resID;

	std::vector<uint8_t> m_tempBuffers;
};


BMPDumperContext::BMPDumperContext(const char *dumpqtDir, int resID)
	: m_dumpqtDir(dumpqtDir)
	, m_resID(resID)
{
}

bool BMPDumperContext::SpecifyFrame(const Rect &rect)
{
	if (!rect.IsValid())
		return false;

	m_frame = rect;
	m_pitchInElements = rect.Width();
	m_pixelData.resize(m_pitchInElements * rect.Height());
	for (PortabilityLayer::RGBAColor &color : m_pixelData)
		color = PortabilityLayer::RGBAColor::Create(0, 0, 0, 255);

	return true;
}

Rect BMPDumperContext::ConstrainRegion(const Rect &rect) const
{
	return m_frame.Intersect(rect);
}

void BMPDumperContext::Start(PortabilityLayer::QDPictBlitSourceType sourceType, const PortabilityLayer::QDPictEmitScanlineParameters &params)
{
	m_blitSourceType = sourceType;
	m_blitParams = params;

	int32_t relativeLeft = params.m_constrainedRegionLeft - m_frame.left;
	int32_t relativeTop = params.m_firstY - m_frame.top;

	m_blitOrigin = (static_cast<uint32_t>(relativeTop) * m_frame.Width()) + static_cast<uint32_t>(relativeLeft);
}

void BMPDumperContext::BlitScanlineAndAdvance(const void *scanlineData)
{
	const uint8_t *scanlineBytes = static_cast<const uint8_t *>(scanlineData);
	const size_t rowSize = m_blitParams.m_constrainedRegionRight - m_blitParams.m_constrainedRegionLeft;
	PortabilityLayer::RGBAColor *outRowStart = &m_pixelData[m_blitOrigin];

	m_blitOrigin += m_pitchInElements;
	const size_t planarSeparation = m_blitParams.m_planarSeparation;

	const size_t firstSrcCol = static_cast<size_t>(m_blitParams.m_constrainedRegionLeft - m_blitParams.m_scanlineOriginX);

	switch (m_blitSourceType)
	{
	case PortabilityLayer::QDPictBlitSourceType_1Bit:
		for (size_t i = 0; i < rowSize; i++)
		{
			const size_t originCol = i + firstSrcCol;
			if (scanlineBytes[originCol / 8] & (0x80 >> (originCol & 7)))
				outRowStart[i] = PortabilityLayer::RGBAColor::Create(0, 0, 0, 255);
			else
				outRowStart[i] = PortabilityLayer::RGBAColor::Create(255, 255, 255, 255);
		}
		break;
	case PortabilityLayer::QDPictBlitSourceType_Indexed1Bit:
		for (size_t i = 0; i < rowSize; i++)
		{
			const size_t originCol = i + firstSrcCol;
			const unsigned int colorIndex = (scanlineBytes[originCol / 8] >> (8 - ((originCol & 7) + 1) * 1)) & 1;
			outRowStart[i] = m_blitParams.m_colors[colorIndex];
		}
		break;
	case PortabilityLayer::QDPictBlitSourceType_Indexed2Bit:
		for (size_t i = 0; i < rowSize; i++)
		{
			const size_t originCol = i + firstSrcCol;
			const unsigned int colorIndex = (scanlineBytes[originCol / 4] >> (8 - ((originCol & 3) + 1) * 2)) & 3;
			outRowStart[i] = m_blitParams.m_colors[colorIndex];
		}
		break;
	case PortabilityLayer::QDPictBlitSourceType_Indexed4Bit:
		for (size_t i = 0; i < rowSize; i++)
		{
			const size_t originCol = i + firstSrcCol;
			const unsigned int colorIndex = (scanlineBytes[originCol / 2] >> (8 - ((originCol & 1) + 1) * 4)) & 15;
			outRowStart[i] = m_blitParams.m_colors[colorIndex];
		}
		break;
	case PortabilityLayer::QDPictBlitSourceType_Indexed8Bit:
		for (size_t i = 0; i < rowSize; i++)
		{
			const size_t originCol = i + firstSrcCol;
			const unsigned int colorIndex = scanlineBytes[originCol];
			outRowStart[i] = m_blitParams.m_colors[colorIndex];
		}
		break;
	case PortabilityLayer::QDPictBlitSourceType_RGB15Native:
		for (size_t i = 0; i < rowSize; i++)
		{
			const size_t originCol = i + firstSrcCol;
			const uint16_t item = *reinterpret_cast<const uint16_t*>(scanlineBytes + originCol * 2);
			PortabilityLayer::RGBAColor &outputItem = outRowStart[i];

			outputItem.b = FiveToEight(item & 0x1f);
			outputItem.g = FiveToEight((item >> 5) & 0x1f);
			outputItem.r = FiveToEight((item >> 10) & 0x1f);
			outputItem.a = 255;
		}
		break;
	case PortabilityLayer::QDPictBlitSourceType_RGB15BE:
		for (size_t i = 0; i < rowSize; i++)
		{
			const size_t originCol = i + firstSrcCol;
			const uint8_t *itemBytes = (scanlineBytes + originCol * 2);
			const uint16_t item = static_cast<uint16_t>((itemBytes[0] << 8) | itemBytes[1]);
			PortabilityLayer::RGBAColor &outputItem = outRowStart[i];

			outputItem.b = FiveToEight(item & 0x1f);
			outputItem.g = FiveToEight((item >> 5) & 0x1f);
			outputItem.r = FiveToEight((item >> 10) & 0x1f);
			outputItem.a = 255;
		}
		break;
	case PortabilityLayer::QDPictBlitSourceType_RGB24_Interleaved:
		for (size_t i = 0; i < rowSize; i++)
		{
			const size_t originCol = i + firstSrcCol;
			PortabilityLayer::RGBAColor &outputItem = outRowStart[i];

			outputItem.r = scanlineBytes[originCol * 3 + 0];
			outputItem.g = scanlineBytes[originCol * 3 + 1];
			outputItem.b = scanlineBytes[originCol * 3 + 2];
			outputItem.a = 255;
		}
		break;
	case PortabilityLayer::QDPictBlitSourceType_RGB24_Multiplane:
		for (size_t i = 0; i < rowSize; i++)
		{
			const size_t originCol = i + firstSrcCol;
			PortabilityLayer::RGBAColor &outputItem = outRowStart[i];

			outputItem.r = scanlineBytes[originCol];
			outputItem.g = scanlineBytes[originCol + planarSeparation];
			outputItem.b = scanlineBytes[originCol + planarSeparation * 2];
			outputItem.a = 255;
		}
		break;
	}
}


struct ImageDescriptionData
{
	char m_codecType[4];
	uint8_t m_reserved[8];
	BEUInt16_t m_cdataVersion;
	BEUInt16_t m_cdataRevision;
	char m_vendor[4];
	BEUInt32_t m_temporalQuality;
	BEUInt32_t m_spatialQuality;
	BEUInt16_t m_width;
	BEUInt16_t m_height;
	BEUFixed32_t m_hRes;
	BEUFixed32_t m_vRes;
	BEUInt32_t m_dataSize;
	BEUInt16_t m_frameCount;
	uint8_t m_nameLength;
	uint8_t m_name[31];
	BEUInt16_t m_depth;
	BEUInt16_t m_clutID;
};

struct ImageDescription
{
	BEUInt32_t m_idSize;
	ImageDescriptionData m_data;
};

class MovDumpScope
{
public:
	MovDumpScope(GpIOStream &stream, const char *atom);
	~MovDumpScope();

private:
	GpIOStream &m_stream;
	GpUFilePos_t m_startPos;
};

MovDumpScope::MovDumpScope(GpIOStream &stream, const char *atom)
	: m_stream(stream)
	, m_startPos(stream.Tell())
{
	struct AtomData
	{
		BEUInt32_t m_size;
		char m_type[4];
	};

	AtomData atomData;
	memcpy(atomData.m_type, atom, 4);
	atomData.m_size = 0;

	stream.Write(&atomData, 8);
}

MovDumpScope::~MovDumpScope()
{
	GpUFilePos_t returnPos = m_stream.Tell();
	m_stream.SeekStart(m_startPos);
	BEUInt32_t size = BEUInt32_t(static_cast<uint32_t>(returnPos - m_startPos));

	m_stream.Write(&size, 4);
	m_stream.SeekStart(returnPos);
}


static bool DumpMOV(GpIOStream &stream, const ImageDescription &imageDesc, const std::vector<uint8_t> &imageData)
{
	if (imageData.size() == 0)
		return true;

	struct HDLRData
	{
		uint8_t m_version;
		uint8_t m_flags[3];
		BEUInt32_t m_componentType;
		BEUInt32_t m_componentSubtype;
		BEUInt32_t m_componentManufacturer;
		BEUInt32_t m_componentFlags;
		BEUInt32_t m_componentFlagsMask;
		// Var: Name
	};

	const uint32_t duration = 40;

	{
		MovDumpScope ftypScope(stream, "ftyp");

		struct FTYPData
		{
			char m_majorBrand[4];
			BEUInt32_t m_minorVersion;
			char m_compatibleBrands[4];
		};

		FTYPData data;
		memcpy(data.m_majorBrand, "qt  ", 4);
		data.m_minorVersion = 0x200;
		memcpy(data.m_compatibleBrands, "qt  ", 4);

		stream.Write(&data, sizeof(data));
	}

	{
		MovDumpScope wideScope(stream, "wide");
	}

	GpUFilePos_t imageDataStart = 0;
	{
		MovDumpScope mdatScope(stream, "mdat");

		imageDataStart = stream.Tell();
		stream.Write(&imageData[0], imageData.size());
	}

	{
		MovDumpScope moovScope(stream, "moov");

		{
			MovDumpScope mvhdScope(stream, "mvhd");

			struct MVHDData
			{
				uint8_t m_version;
				uint8_t m_flags[3];
				BEUInt32_t m_creationTime;
				BEUInt32_t m_modificationTime;
				BEUInt32_t m_timeScale;
				BEUInt32_t m_duration;
				BEUFixed32_t m_preferredRate;
				BEUFixed16_t m_preferredVolume;
				uint8_t m_reserved[10];
				BESFixed32_t m_matrix[9];
				BEUInt32_t m_previewTime;
				BEUInt32_t m_previewDuration;
				BEUInt32_t m_posterTime;
				BEUInt32_t m_selectionTime;
				BEUInt32_t m_selectionDuration;
				BEUInt32_t m_currentTime;
				BEUInt32_t m_nextTrackID;
			};

			MVHDData mvhdData;
			memset(&mvhdData, 0, sizeof(mvhdData));
			mvhdData.m_timeScale = 1000;
			mvhdData.m_duration = duration;
			mvhdData.m_preferredRate.m_intPart = 1;
			mvhdData.m_preferredVolume.m_intPart = 1;
			mvhdData.m_matrix[0].m_intPart = 1;
			mvhdData.m_matrix[5].m_intPart = 5;
			mvhdData.m_matrix[8].m_intPart = 0x4000;
			mvhdData.m_nextTrackID = 2;

			stream.Write(&mvhdData, sizeof(mvhdData));
		}

		{
			MovDumpScope trakScope(stream, "trak");

			{
				MovDumpScope tkhdScope(stream, "tkhd");

				struct TKHDData
				{
					uint8_t m_version;
					uint8_t m_flags[3];
					BEUInt32_t m_creationTime;
					BEUInt32_t m_modificationTime;
					BEUInt32_t m_trackID;
					uint8_t m_reserved[4];
					BEUInt32_t m_duration;
					uint8_t m_reserved2[8];
					BEUInt16_t m_layer;
					BEUInt16_t m_alternateGroup;
					BEUInt16_t m_volume;
					uint8_t m_reserved3[2];
					BEUFixed32_t m_matrix[9];
					BEUFixed32_t m_trackWidth;
					BEUFixed32_t m_trackHeight;
				};

				TKHDData tkhdData;
				memset(&tkhdData, 0, sizeof(tkhdData));

				tkhdData.m_flags[2] = 0x3;	// Used in movie + enabled
				tkhdData.m_trackID = 1;
				tkhdData.m_duration = duration;
				tkhdData.m_matrix[0].m_intPart = 1;
				tkhdData.m_matrix[5].m_intPart = 5;
				tkhdData.m_matrix[8].m_intPart = 0x4000;
				tkhdData.m_trackWidth.m_intPart = imageDesc.m_data.m_width;
				tkhdData.m_trackHeight.m_intPart = imageDesc.m_data.m_height;

				stream.Write(&tkhdData, sizeof(tkhdData));
			}

			{
				MovDumpScope edtsScope(stream, "edts");

				{
					MovDumpScope elstScope(stream, "elst");

					struct ELSTEntry
					{
						BEUInt32_t m_duration;
						BEUInt32_t m_mediaTime;
						BEUFixed32_t m_mediaRate;
					};

					struct ELSTData
					{
						uint8_t m_version;
						uint8_t m_flags[3];
						BEUInt32_t m_numEntries;

						ELSTEntry m_entry;
					};

					ELSTData elstData;
					memset(&elstData, 0, sizeof(elstData));

					elstData.m_numEntries = 1;
					elstData.m_entry.m_duration = duration;
					elstData.m_entry.m_mediaRate.m_intPart = 1;

					stream.Write(&elstData, sizeof(elstData));
				}
			}

			{
				MovDumpScope mdiaScope(stream, "mdia");

				{
					MovDumpScope mdhdScope(stream, "mdhd");

					struct MDHDData
					{
						uint8_t m_version;
						uint8_t m_flags[3];
						BEUInt32_t m_creationTime;
						BEUInt32_t m_modificationTime;
						BEUFixed32_t m_timeScale;
						BEUInt32_t m_duration;
						BEUInt16_t m_language;
						BEUInt16_t m_quality;
					};

					MDHDData mdhdData;
					memset(&mdhdData, 0, sizeof(mdhdData));

					mdhdData.m_timeScale.m_fracPart = 12800;
					mdhdData.m_duration = 0x200;
					mdhdData.m_language = 0x7fff;

					stream.Write(&mdhdData, sizeof(mdhdData));
				}

				{
					MovDumpScope hdlrScope(stream, "hdlr");

					HDLRData hdlrData;
					memset(&hdlrData, 0, sizeof(hdlrData));

					hdlrData.m_componentType = 0x6d686c72;		// mhlr
					hdlrData.m_componentSubtype = 0x76696465;	// vide

					stream.Write(&hdlrData, sizeof(hdlrData));

					const char *handlerName = "VideoHandler";
					uint8_t handlerNameLen = strlen(handlerName);

					stream.Write(&handlerNameLen, 1);
					stream.Write(handlerName, handlerNameLen);
				}

				{
					MovDumpScope minfScope(stream, "minf");

					{
						MovDumpScope vmhdScope(stream, "vmhd");

						struct VMHDData
						{
							uint8_t m_version;
							uint8_t m_flags[3];
							BEUInt16_t m_graphicsMode;
							BEUInt16_t m_opColor[3];
						};

						VMHDData vmhdData;
						memset(&vmhdData, 0, sizeof(vmhdData));

						vmhdData.m_flags[2] = 1;	// Compatibility flag

						stream.Write(&vmhdData, sizeof(vmhdData));
					}

					{
						MovDumpScope hdlrScope(stream, "hdlr");

						HDLRData hdlrData;
						memset(&hdlrData, 0, sizeof(hdlrData));

						hdlrData.m_componentType = 0x64686c72;		// dhlr
						hdlrData.m_componentSubtype = 0x75726c20;	// url 

						stream.Write(&hdlrData, sizeof(hdlrData));

						const char *handlerName = "VideoHandler";
						uint8_t handlerNameLen = strlen(handlerName);

						stream.Write(&handlerNameLen, 1);
						stream.Write(handlerName, handlerNameLen);
					}

					{
						MovDumpScope dinfScope(stream, "dinf");

						{
							MovDumpScope drefScope(stream, "dref");

							struct DREFData
							{
								uint8_t m_version;
								uint8_t m_flags[3];
								BEUInt32_t m_numEntries;
								// DREFEntry
							};

							struct DREFEntry
							{
								BEUInt32_t m_size;
								BEUInt32_t m_type;
								uint8_t m_version;
								uint8_t m_flags[3];
								// Data
							};

							DREFData drefData;
							memset(&drefData, 0, sizeof(drefData));

							drefData.m_numEntries = 1;

							stream.Write(&drefData, sizeof(drefData));

							DREFEntry drefEntry;
							memset(&drefEntry, 0, sizeof(drefEntry));

							drefEntry.m_size = sizeof(DREFEntry);
							drefEntry.m_type = 0x75726c20;	// url
							drefEntry.m_flags[2] = 1;	// Self-contained

							stream.Write(&drefEntry, sizeof(drefEntry));
						}
					}

					{
						MovDumpScope stblScope(stream, "stbl");

						{
							MovDumpScope stsdScope(stream, "stsd");

							struct STSDData
							{
								uint8_t m_version;
								uint8_t m_flags[3];
								BEUInt32_t m_numEntries;
							};

							STSDData stsdData;
							memset(&stsdData, 0, sizeof(stsdData));
							stsdData.m_numEntries = 1;

							stream.Write(&stsdData, sizeof(stsdData));

							struct STSDEntry
							{
								BEUInt32_t m_size;
								uint8_t m_dataFormat[4];
								uint8_t m_reserved[6];
								BEUInt16_t m_dataRefIndex;
							};

							struct FieldAtom
							{
								BEUInt32_t m_atomSize;
								char m_atomID[4];
								uint8_t m_fieldCount;
								uint8_t m_fieldOrdering;
							};

							struct VideoMediaSampleDescription
							{
								BEUInt16_t m_version;
								BEUInt16_t m_revisionLevel;
								BEUInt32_t m_vendor;
								BEUInt32_t m_temporalQuality;
								BEUInt32_t m_spatialQuality;
								BEUInt16_t m_width;
								BEUInt16_t m_height;
								BEUFixed32_t m_hRes;
								BEUFixed32_t m_vRes;
								BEUInt32_t m_dataSize;
								BEUInt16_t m_frameCountPerSample;
								uint8_t m_compressorNameLength;
								uint8_t m_compressorName[31];
								BEUInt16_t m_depth;
								BEInt16_t m_ctabID;

								FieldAtom m_fieldAtom;
							};

							STSDEntry stsdEntry;
							memset(&stsdEntry, 0, sizeof(stsdEntry));

							stsdEntry.m_size = sizeof(STSDEntry) + sizeof(VideoMediaSampleDescription);
							memcpy(stsdEntry.m_dataFormat, imageDesc.m_data.m_codecType, 4);
							stsdEntry.m_dataRefIndex = 1;

							stream.Write(&stsdEntry, sizeof(stsdEntry));

							const size_t kVideoMediaSampleDescSize = sizeof(VideoMediaSampleDescription);
							GP_STATIC_ASSERT(kVideoMediaSampleDescSize == 80);

							VideoMediaSampleDescription vmsdDesc;
							memset(&vmsdDesc, 0, sizeof(vmsdDesc));

							memcpy(&vmsdDesc.m_vendor, "GR2A", 4);
							vmsdDesc.m_temporalQuality = 0x200;
							vmsdDesc.m_spatialQuality = 0x200;
							vmsdDesc.m_width = imageDesc.m_data.m_width;
							vmsdDesc.m_height = imageDesc.m_data.m_height;
							vmsdDesc.m_hRes.m_intPart = 72;
							vmsdDesc.m_vRes.m_intPart = 72;
							vmsdDesc.m_frameCountPerSample = 1;
							vmsdDesc.m_depth = imageDesc.m_data.m_depth;
							vmsdDesc.m_ctabID = -1;

							memcpy(vmsdDesc.m_fieldAtom.m_atomID, "fild", 4);
							vmsdDesc.m_fieldAtom.m_atomSize = sizeof(vmsdDesc.m_fieldAtom);
							vmsdDesc.m_fieldAtom.m_fieldCount = 1;

							stream.Write(&vmsdDesc, sizeof(vmsdDesc));
						}

						{
							MovDumpScope sttsScope(stream, "stts");

							struct STTSData
							{
								uint8_t m_version;
								uint8_t m_flags[3];
								BEUInt32_t m_numEntries;
							};

							STTSData sttsData;
							memset(&sttsData, 0, sizeof(sttsData));
							sttsData.m_numEntries = 1;

							stream.Write(&sttsData, sizeof(sttsData));

							struct STTSEntry
							{
								BEUInt32_t m_sampleCount;
								BEUInt32_t m_sampleDuration;
							};

							STTSEntry sttsEntry;
							memset(&sttsEntry, 0, sizeof(sttsEntry));
							sttsEntry.m_sampleCount = 1;
							sttsEntry.m_sampleDuration = 0x200;

							stream.Write(&sttsEntry, sizeof(sttsEntry));
						}

						{
							MovDumpScope sttsScope(stream, "stsc");

							struct STSCData
							{
								uint8_t m_version;
								uint8_t m_flags[3];
								BEUInt32_t m_numEntries;
							};

							STSCData stscData;
							memset(&stscData, 0, sizeof(stscData));
							stscData.m_numEntries = 1;

							stream.Write(&stscData, sizeof(stscData));

							struct STSCEntry
							{
								BEUInt32_t m_firstChunk;
								BEUInt32_t m_samplesPerChunk;
								BEUInt32_t m_sampleDescriptionID;
							};

							STSCEntry stscEntry;
							memset(&stscEntry, 0, sizeof(stscEntry));
							stscEntry.m_firstChunk = 1;
							stscEntry.m_samplesPerChunk = 1;
							stscEntry.m_sampleDescriptionID = 1;

							stream.Write(&stscEntry, sizeof(stscEntry));
						}

						{
							MovDumpScope sttsScope(stream, "stsz");

							struct STSZData
							{
								uint8_t m_version;
								uint8_t m_flags[3];
								BEUInt32_t m_sampleSize;
								BEUInt32_t m_numEntries;
							};

							STSZData stszData;
							memset(&stszData, 0, sizeof(stszData));
							stszData.m_sampleSize = static_cast<uint32_t>(imageData.size());
							stszData.m_numEntries = 1;

							stream.Write(&stszData, sizeof(stszData));
						}

						{
							MovDumpScope sttsScope(stream, "stco");

							struct STCOData
							{
								uint8_t m_version;
								uint8_t m_flags[3];
								BEUInt32_t m_numEntries;

								BEUInt32_t m_firstChunkOffset;
							};

							STCOData stcoData;
							memset(&stcoData, 0, sizeof(stcoData));
							stcoData.m_firstChunkOffset = static_cast<uint32_t>(imageDataStart);
							stcoData.m_numEntries = 1;

							stream.Write(&stcoData, sizeof(stcoData));
						}
					}
				}
			}
		}
	}

	return true;
}

static bool ReadImageDesc(GpIOStream *stream, ImageDescription &imageDesc, std::vector<uint8_t> *optAuxData)
{
	GpUFilePos_t imageDescStart = stream->Tell();
	if (!stream->ReadExact(&imageDesc.m_idSize, sizeof(imageDesc.m_idSize)))
		return false;

	if (imageDesc.m_idSize < sizeof(ImageDescription))
		return false;

	if (!stream->ReadExact(&imageDesc.m_data, sizeof(imageDesc.m_data)))
		return false;

	if (optAuxData == nullptr)
	{
		if (!stream->SeekStart(imageDescStart))
			return false;

		if (!stream->SeekCurrent(imageDesc.m_idSize))
			return false;
	}
	else
	{
		const size_t auxSize = imageDesc.m_idSize - sizeof(ImageDescription);
		optAuxData->resize(auxSize);
		if (auxSize > 0)
		{
			if (!stream->ReadExact(&(*optAuxData)[0], auxSize))
				return false;
		}
	}

	return true;
}

bool BMPDumperContext::EmitQTContent(GpIOStream *stream, uint32_t dataSize, bool isCompressed)
{
	GpUFilePos_t startPos = stream->Tell();

	if (isCompressed && !m_dumpqtDir)
		return false;

	// Only one known house ("Magic" seems to use uncompressed, which is partly documented here:
	// https://github.com/gco/xee/blob/master/XeePhotoshopPICTLoader.m

	struct UncompressedDataHeader
	{
		BEUInt16_t m_version;
		BESFixed32_t m_matrix[9];
		BEUInt32_t m_matteSizeBytes;
		BERect m_matteRect;
	};

	struct CompressedDataHeader
	{
		BEUInt16_t m_transferMode;
		BERect m_srcRect;
		BEUInt32_t m_preferredAccuracy;
		BEUInt32_t m_maskSizeBytes;
	};

	UncompressedDataHeader dHeader;
	if (!stream->ReadExact(&dHeader, sizeof(dHeader)))
		return false;

	CompressedDataHeader cHeader;
	ImageDescription matteDesc;
	ImageDescription imageDesc;
	std::vector<uint8_t> imageDescAux;

	bool haveMDesc = false;

	if (isCompressed)
	{
		if (!stream->ReadExact(&cHeader, sizeof(cHeader)))
			return false;
	}

	if (dHeader.m_matteSizeBytes != 0)
	{
		haveMDesc = true;
		if (!ReadImageDesc(stream, matteDesc, nullptr))
			return false;
	}

	if (isCompressed)
	{
		if (cHeader.m_maskSizeBytes > 0)
		{
			if (!stream->SeekCurrent(cHeader.m_maskSizeBytes))
				return false;
		}

		if (!ReadImageDesc(stream, imageDesc, &imageDescAux))
			return false;

		const size_t imageDataSize = imageDesc.m_data.m_dataSize;

		GpUFilePos_t imageDataPrefixSize = stream->Tell() - startPos;
		if (imageDataPrefixSize > dataSize)
			return false;

		const size_t dataAvailable = dataSize - imageDataPrefixSize;

		if (dataAvailable < imageDataSize)
			return false;

		std::vector<uint8_t> imageData;
		imageData.resize(imageDataSize);

		if (!stream->ReadExact(&imageData[0], imageDataSize))
			return false;

		std::stringstream dumpPathStream;
		dumpPathStream << m_dumpqtDir << "/" << m_resID << ".mov";

		FILE *tempFile = fopen_utf8(dumpPathStream.str().c_str(), "wb");
		if (!tempFile)
			return false;

		PortabilityLayer::CFileStream tempFileStream(tempFile);
		bool dumpedOK = DumpMOV(tempFileStream, imageDesc, imageData);
		tempFileStream.Close();

		if (!dumpedOK)
			return false;

		if (!stream->SeekStart(startPos))
			return false;

		if (!stream->SeekCurrent(dataSize))
			return false;

		return false;
	}
	else
	{
		BEUInt16_t subOpcode;
		if (!stream->ReadExact(&subOpcode, sizeof(subOpcode)))
			return false;

		int n = 0;
	}


	// Known compressed cases and codecs:
	// "Egypt" res 10011: JPEG
	// "The Meadows" res 3000: Apple Video (a.k.a. Apple RPZA)
	return false;
}

bool BMPDumperContext::AllocTempBuffers(uint8_t *&buffer1, size_t buffer1Size, uint8_t *&buffer2, size_t buffer2Size)
{
	m_tempBuffers.resize(buffer1Size + buffer2Size);

	if (m_tempBuffers.size() == 0)
	{
		buffer1 = nullptr;
		buffer2 = nullptr;
	}
	else
	{
		buffer1 = &m_tempBuffers[0];
		buffer2 = buffer1 + buffer1Size;
	}

	return true;
}

bool BMPDumperContext::Export(std::vector<uint8_t> &outData) const
{
	return ExportBMP(m_frame.Width(), m_frame.Height(), m_pitchInElements, &m_pixelData[0], outData);
}

bool ImportPICT(std::vector<uint8_t> &outBMP, const void *inData, size_t inSize, const char *dumpqtDir, int resID)
{
	PortabilityLayer::MemReaderStream stream(inData, inSize);
	BMPDumperContext context(dumpqtDir, resID);

	PortabilityLayer::QDPictDecoder decoder;
	if (decoder.DecodePict(&stream, &context))
	{
		if (context.Export(outBMP))
			return true;

		return false;
	}
	else
		return false;
}

size_t PadRiffChunk(size_t sz)
{
	return sz + (sz & 1);
}

void PadAlignWave(std::vector<uint8_t> &outWAV)
{
	if (outWAV.size() & 1)
		outWAV.push_back(0);
}

bool DecompressSound(int compressionID, int channelCount, const void *sndData, size_t sndDataSize, size_t numFrames, std::vector<uint8_t> &decompressed)
{
	if (compressionID == AudioCompressionCodecID_ThreeToOne)
	{
		if (channelCount != 1)
		{
			fprintf(stderr, "Unsupported MACE decode channel layout\n");
			return false;
		}

		if (sndDataSize < numFrames * 2)
		{
			fprintf(stderr, "Sound data is too small\n");
			return false;
		}

		MaceChannelDecState state;
		memset(&state, 0, sizeof(state));

		const uint8_t *packets = static_cast<const uint8_t*>(sndData);
		for (size_t i = 0; i < numFrames; i++)
		{
			for (size_t subPkt = 0; subPkt < 2; subPkt++)
			{
				uint8_t samples[3];
				DecodeMACE3(&state, packets[i * 2 + subPkt], samples);

				for (int s = 0; s < 3; s++)
					decompressed.push_back(samples[s]);
			}
		}
	}
	else if (compressionID = AudioCompressionCodecID_SixToOne)
	{
		if (channelCount != 1)
		{
			fprintf(stderr, "Unsupported MACE decode channel layout\n");
			return false;
		}

		if (sndDataSize < numFrames)
		{
			fprintf(stderr, "Sound data is too small\n");
			return false;
		}

		MaceChannelDecState state;
		memset(&state, 0, sizeof(state));

		const uint8_t *packets = static_cast<const uint8_t*>(sndData);
		for (size_t i = 0; i < numFrames; i++)
		{
			uint8_t samples[6];
			DecodeMACE6(&state, packets[i], samples);

			for (int s = 0; s < 6; s++)
				decompressed.push_back(samples[s]);
		}
	}
	else
	{
		fprintf(stderr, "Unknown audio compression format\n");
		return false;
	}

	return true;
}

bool ImportSound(std::vector<uint8_t> &outWAV, const void *inData, size_t inSize, int resID)
{
	// Glider PRO has a hard-coded expectation that the sound will have exactly 20 bytes of prefix.
	// The resource format can have more than that, we'll just follow this base expectation
	const size_t hardCodedPrefixSize = 20;

	struct BufferHeader
	{
		BEUInt32_t m_samplePtr;
		BEUInt32_t m_length;
		BEUFixed32_t m_sampleRate;
		BEUInt32_t m_loopStart;
		BEUInt32_t m_loopEnd;
		uint8_t m_encoding;
		uint8_t m_baseFrequency;
	};

	struct ExtHeader
	{
		BEUInt32_t m_samplePtr;
		BEUInt32_t m_channelCount;
		BEUFixed32_t m_sampleRate;
		BEUInt32_t m_loopStart;
		BEUInt32_t m_loopEnd;
		uint8_t m_encoding;
		uint8_t m_baseFrequency;
		BEUInt32_t m_numSamples;
		BEUInt16_t m_sampleRateExponentAndSign;
		BEUInt32_t m_sampleRateFractionHigh;
		BEUInt32_t m_sampleRateFractionLow;
		BEUInt32_t m_markerPtr;
		BEUInt32_t m_instrumentPtr;
		BEUInt32_t m_recordingDevicesPtr;
		BEUInt16_t m_sampleSize;
		uint8_t m_reserved[14];
	};

	struct CmpHeader
	{
		BEUInt32_t m_samplePtr;
		BEUInt32_t m_channelCount;
		BEUFixed32_t m_sampleRate;
		BEUInt32_t m_loopStart;
		BEUInt32_t m_loopEnd;
		uint8_t m_encoding;
		uint8_t m_baseFrequency;
		BEUInt32_t m_numFrames;
		BEUInt16_t m_sampleRateExponentAndSign;
		BEUInt32_t m_sampleRateFractionHigh;
		BEUInt32_t m_sampleRateFractionLow;
		BEUInt32_t m_markerPtr;
		BEUInt32_t m_format;
		uint8_t m_reserved1[4];

		BEUInt32_t m_stateVars;
		BEUInt32_t m_leftOverSamples;
		BEInt16_t m_compressionID;
		BEUInt16_t m_packetSize;
		BEUInt16_t m_synthesizerID;
		BEUInt16_t m_sampleSize;
	};

	const int hs = sizeof(CmpHeader);

	GP_STATIC_ASSERT(sizeof(ExtHeader) == 64);
	GP_STATIC_ASSERT(sizeof(CmpHeader) == 64);

	if (inSize < hardCodedPrefixSize)
		return false;

	const uint8_t *sndBufferData = static_cast<const uint8_t*>(inData) + hardCodedPrefixSize;
	inSize -= hardCodedPrefixSize;

	if (inSize < sizeof(BufferHeader))
		return false;

	BufferHeader header;
	memcpy(&header, sndBufferData, sizeof(header));

	std::vector<uint8_t> decompressedSound;
	std::vector<uint8_t> resampledSound;
	std::vector<uint8_t> downmixedSound;

	uint32_t inputDataLength = 0;
	uint32_t outputDataLength = 0;
	if (header.m_encoding == 0xfe)
	{
		if (inSize < sizeof(CmpHeader))
			return false;

		CmpHeader cmpHeader;
		memcpy(&cmpHeader, sndBufferData, sizeof(cmpHeader));

		sndBufferData += sizeof(ExtHeader);
		inSize -= sizeof(ExtHeader);

		if (!DecompressSound(cmpHeader.m_compressionID, cmpHeader.m_channelCount, sndBufferData, inSize, cmpHeader.m_numFrames, decompressedSound))
			return false;

		outputDataLength = decompressedSound.size();
		if (decompressedSound.size() > 0)
			sndBufferData = &decompressedSound[0];
	}
	else if (header.m_encoding == 0xff)
	{
		if (inSize < sizeof(ExtHeader))
			return false;

		ExtHeader extHeader;
		memcpy(&extHeader, sndBufferData, sizeof(extHeader));

#if 0
		uint64_t sampleRateFraction = (static_cast<uint64_t>(static_cast<uint32_t>(extHeader.m_sampleRateFractionHigh)) << 32) | static_cast<uint32_t>(extHeader.m_sampleRateFractionLow);
		uint16_t sampleRateExponentAndSign = extHeader.m_sampleRateExponentAndSign;

		int32_t sampleRateExponent = static_cast<int32_t>(sampleRateExponentAndSign & 0x7fff) - 16447;

		double sampleRate = static_cast<double>(sampleRateFraction) * pow(2.0, sampleRateExponent);
#endif

		uint16_t bitsPerSample = extHeader.m_sampleSize;

		if (bitsPerSample != 8 && bitsPerSample != 16)
		{
			fprintf(stderr, "Sound had unexpected bit rate\n");
			return false;
		}

		if (extHeader.m_channelCount != 1 && extHeader.m_channelCount != 2)
		{
			fprintf(stderr, "Sound had unexpected channel count\n");
			return false;
		}

		inputDataLength = extHeader.m_numSamples * extHeader.m_channelCount * (bitsPerSample / 8);
		outputDataLength = extHeader.m_numSamples;

		sndBufferData += sizeof(ExtHeader);
		inSize -= sizeof(ExtHeader);

		if (inputDataLength > inSize)
			return false;

		if (inputDataLength == 0)
			return false;

		int channelCount = extHeader.m_channelCount;
		if (channelCount == 2)
		{
			fprintf(stderr, "WARNING: Downmixing sound resource %i to mono\n", resID);

			if (bitsPerSample == 16)
			{
				const size_t numSamples = extHeader.m_numSamples;
				downmixedSound.resize(numSamples * 2);

				for (size_t i = 0; i < numSamples; i++)
				{
					BEInt16_t channels[2];
					memcpy(channels, sndBufferData + i * 4, 4);

					const BEInt16_t downmixedSample = BEInt16_t(static_cast<int16_t>((static_cast<int32_t>(channels[0]) + static_cast<int32_t>(channels[1]) + 1) >> 1));

					memcpy(&downmixedSound[i * 2], &downmixedSample, 2);
				}

				sndBufferData = &downmixedSound[0];
				inputDataLength = downmixedSound.size();
			}
			else
			{
				const size_t numSamples = extHeader.m_numSamples;
				downmixedSound.resize(numSamples);

				for (size_t i = 0; i < numSamples; i++)
				{
					const uint8_t *channels = sndBufferData + i * 2;
					const uint8_t downmixedSample = static_cast<uint8_t>((channels[0] + channels[1] + 1) >> 1);

					downmixedSound[i] = downmixedSample;
				}

				sndBufferData = &downmixedSound[0];
				inputDataLength = downmixedSound.size();
			}
		}

		if (bitsPerSample == 16)
		{
			fprintf(stderr, "WARNING: Downsampling sound resource %i to 8 bit\n", resID);

			const size_t numSamples = extHeader.m_numSamples;
			resampledSound.resize(numSamples);

			for (size_t i = 0; i < numSamples; i++)
			{
				const uint16_t sample16BitSigned = ((static_cast<int16_t>(sndBufferData[i * 2]) << 8) | sndBufferData[i * 2 + 1]) ^ 0x8000;
				const uint8_t sample8Bit = (static_cast<uint32_t>(sample16BitSigned) * 2 + 257) / 514;

				resampledSound[i] = sample8Bit;
			}

			sndBufferData = &resampledSound[0];
			inputDataLength = resampledSound.size();
		}
	}
	else
	{
		inputDataLength = outputDataLength = header.m_length;

		sndBufferData += sizeof(header);
		inSize -= sizeof(header);

		if (inputDataLength > inSize)
			return false;
	}

	uint32_t sampleRate = header.m_sampleRate.m_intPart;
	if (static_cast<int>(header.m_sampleRate.m_fracPart) >= 0x8000)
		sampleRate++;

	if (sampleRate == 0)
		return false;

	std::vector<uint8_t> rateChangedSound;
	const uint32_t minSampleRate = 22000;
	const uint32_t maxSampleRate = 23000;
	if (sampleRate > maxSampleRate || sampleRate < minSampleRate)
	{
		uint32_t sampleRateRatioNumerator = 1;
		uint32_t sampleRateRatioDenominator = 1;

		uint32_t targetSampleRate = 0;
		if (sampleRate < minSampleRate)
		{
			targetSampleRate = sampleRate;
			while (targetSampleRate * 2 <= maxSampleRate)
			{
				targetSampleRate *= 2;
				sampleRateRatioNumerator *= 2;
			}
		}
		else
		{
			targetSampleRate = sampleRate;
			while (minSampleRate * 2 <= targetSampleRate)
			{
				targetSampleRate /= 2;
				sampleRateRatioDenominator *= 2;
			}
		}

		if (targetSampleRate > maxSampleRate || targetSampleRate < minSampleRate)
		{
			targetSampleRate = 0x56ef;
			sampleRateRatioNumerator = targetSampleRate;
			sampleRateRatioDenominator = sampleRate;
		}


		targetSampleRate = sampleRate * sampleRateRatioNumerator / sampleRateRatioDenominator;

		fprintf(stderr, "WARNING: Resampling sound resource %i from %i to %i Hz\n", resID, static_cast<int>(sampleRate), static_cast<int>(targetSampleRate));

		uint32_t postInterpolateSampleRateRatioNumerator = sampleRateRatioNumerator;
		uint32_t postInterpolateSampleRateRatioDenominator = sampleRateRatioDenominator;

		std::vector<uint8_t> interpolatableSound(sndBufferData, sndBufferData + inputDataLength);
		while (postInterpolateSampleRateRatioNumerator * 2 <= postInterpolateSampleRateRatioDenominator)
		{
			size_t halfSize = interpolatableSound.size() / 2;
			for (size_t i = 0; i < halfSize; i++)
				interpolatableSound[i] = (interpolatableSound[i * 2] + interpolatableSound[i * 2 + 1] + 1) / 2;
			interpolatableSound.resize(halfSize);
			postInterpolateSampleRateRatioNumerator *= 2;

			if (postInterpolateSampleRateRatioNumerator % 2 == 0 && postInterpolateSampleRateRatioDenominator % 2 == 0)
			{
				postInterpolateSampleRateRatioNumerator /= 2;
				postInterpolateSampleRateRatioDenominator /= 2;
			}
		}

		if (postInterpolateSampleRateRatioNumerator != postInterpolateSampleRateRatioDenominator)
		{
			size_t originalSize = interpolatableSound.size();
			size_t targetSize = originalSize * postInterpolateSampleRateRatioNumerator / postInterpolateSampleRateRatioDenominator;
			rateChangedSound.resize(targetSize);

			for (size_t i = 0; i < targetSize; i++)
			{
				size_t sampleIndexTimes256 = i * postInterpolateSampleRateRatioDenominator * 256 / postInterpolateSampleRateRatioNumerator;
				size_t startSampleIndex = sampleIndexTimes256 / 256;
				size_t endSampleIndex = startSampleIndex + 1;
				if (startSampleIndex >= originalSize)
					startSampleIndex = originalSize - 1;
				if (endSampleIndex >= originalSize)
					endSampleIndex = originalSize - 1;

				size_t interpolation = sampleIndexTimes256 % 256;
				uint8_t startSample = interpolatableSound[startSampleIndex];
				uint8_t endSample = interpolatableSound[endSampleIndex];

				rateChangedSound[i] = ((startSample * (256 - interpolation) + endSample * interpolation + 128) / 256);
			}
			sampleRate = (sampleRate * sampleRateRatioNumerator * 2 + sampleRateRatioDenominator) / (sampleRateRatioDenominator * 2);
		}
		else
			rateChangedSound = interpolatableSound;

		sndBufferData = &rateChangedSound[0];
		inputDataLength = rateChangedSound.size();
		outputDataLength = inputDataLength;
	}

	PortabilityLayer::WaveFormatChunkV1 formatChunk;

	const size_t riffTagSize = sizeof(PortabilityLayer::RIFFTag);
	const size_t waveMarkerSize = 4;
	const size_t fmtTagSize = sizeof(PortabilityLayer::RIFFTag);
	const size_t fmtContentSize = PadRiffChunk(sizeof(formatChunk));
	const size_t dataTagSize = sizeof(PortabilityLayer::RIFFTag);
	const size_t dataContentSize = PadRiffChunk(outputDataLength);

	// Structure:
	// riffTag
	//   waveMarker
	//   fmtTag
	//     fmtContent
	//   dataTag
	//     dataContent

	outWAV.resize(0);
	outWAV.reserve(riffTagSize + waveMarkerSize + fmtTagSize + fmtContentSize + dataTagSize + dataContentSize);

	PortabilityLayer::RIFFTag riffTag;
	riffTag.m_tag = PortabilityLayer::WaveConstants::kRiffChunkID;
	riffTag.m_chunkSize = static_cast<uint32_t>(waveMarkerSize + fmtTagSize + fmtContentSize + dataTagSize + dataContentSize);

	VectorAppend(outWAV, reinterpret_cast<const uint8_t*>(&riffTag), sizeof(riffTag));

	VectorAppend(outWAV, reinterpret_cast<const uint8_t*>("WAVE"), 4);

	PortabilityLayer::RIFFTag formatTag;
	formatTag.m_tag = PortabilityLayer::WaveConstants::kFormatChunkID;
	formatTag.m_chunkSize = sizeof(formatChunk);

	VectorAppend(outWAV, reinterpret_cast<const uint8_t*>(&formatTag), sizeof(formatTag));

	formatChunk.m_formatCode = PortabilityLayer::WaveConstants::kFormatPCM;
	formatChunk.m_numChannels = 1;
	formatChunk.m_sampleRate = sampleRate;
	formatChunk.m_bytesPerSecond = sampleRate;
	formatChunk.m_blockAlignmentBytes = 1;	// channels * bits per sample / 8
	formatChunk.m_bitsPerSample = 8;

	VectorAppend(outWAV, reinterpret_cast<const uint8_t*>(&formatChunk), sizeof(formatChunk));
	PadAlignWave(outWAV);

	PortabilityLayer::RIFFTag dataTag;
	dataTag.m_tag = PortabilityLayer::WaveConstants::kDataChunkID;
	dataTag.m_chunkSize = outputDataLength;

	VectorAppend(outWAV, reinterpret_cast<const uint8_t*>(&dataTag), sizeof(dataTag));

	VectorAppend(outWAV, sndBufferData, outputDataLength);
	PadAlignWave(outWAV);

	return true;
}

bool ImportIndexedString(std::vector<uint8_t> &outTXT, const void *inData, size_t inSize)
{
	size_t remainingSize = inSize;
	const uint8_t *inBytes = static_cast<const uint8_t*>(inData);

	if (remainingSize < 2)
		return false;

	const size_t arraySize = (inBytes[0] << 8) + inBytes[1];

	inBytes += 2;
	remainingSize -= 2;

	for (size_t i = 0; i < arraySize; i++)
	{
		if (remainingSize < 1)
			return false;

		uint8_t strLength = *inBytes;

		inBytes++;
		remainingSize--;

		if (strLength > remainingSize)
			return false;

		if (i != 0)
			outTXT.push_back('\n');

		for (size_t j = 0; j < strLength; j++)
		{
			outTXT.push_back(*inBytes);
			inBytes++;
			remainingSize--;
		}
	}

	return true;
}

namespace SerializedDialogItemTypeCodes
{
	enum SerializedDialogItemTypeCode
	{
		kUserItem = 0x00,
		kButton = 0x04,
		kCheckBox = 0x05,
		kRadioButton = 0x06,
		kCustomControl = 0x07,
		kLabel = 0x08,
		kEditBox = 0x10,
		kIcon = 0x20,
		kImage = 0x40,
	};
}

bool ImportDialogItemTemplate(std::vector<uint8_t> &outTXT, const void *inData, size_t inSize)
{
	const uint8_t *inBytes = static_cast<const uint8_t*>(inData);

	if (inSize < 2)
		return false;

	BEInt16_t numItemsMinusOne;
	memcpy(&numItemsMinusOne, inBytes, 2);

	if (numItemsMinusOne < -1)
		return false;

	uint16_t numItems = static_cast<uint16_t>(numItemsMinusOne + 1);

	inBytes += 2;
	inSize -= 2;


	AppendStr(outTXT, "{\n");
	AppendStr(outTXT, "\t\"items\" :\n");
	AppendStr(outTXT, "\t[");

	for (unsigned int item = 0; item < numItems; item++)
	{
		if (item != 0)
			outTXT.push_back(',');

		AppendStr(outTXT, "\n\t\t{");
		if (inSize < 14)
			return false;

		BERect itemRect;
		uint8_t packedItemType;

		memcpy(&itemRect, inBytes + 4, 8);

		packedItemType = inBytes[12];

		uint8_t nameLength = inBytes[13];

		inBytes += 14;
		inSize -= 14;

		const uint8_t *nameBytes = inBytes;

		size_t nameLengthPadded = nameLength;
		if ((nameLength & 1) == 1)
			nameLengthPadded++;

		if (inSize < nameLengthPadded)
			return false;

		inBytes += nameLengthPadded;
		inSize -= nameLengthPadded;

		const Rect rect = itemRect.ToRect();
		BEInt16_t id(0);

		uint8_t serializedType = (packedItemType & 0x7f);
		bool enabled = ((packedItemType & 0x80) == 0);

		if (nameLength >= 2 && (serializedType == SerializedDialogItemTypeCodes::kCustomControl || serializedType == SerializedDialogItemTypeCodes::kImage || serializedType == SerializedDialogItemTypeCodes::kIcon))
		{
			memcpy(&id, nameBytes, 2);
			nameLength = 0;
		}

		AppendStr(outTXT, "\n\t\t\t\"name\" : \"");

		for (size_t i = 0; i < nameLength; i++)
		{
			uint8_t nameByte = nameBytes[i];
			switch (nameByte)
			{
			case '\"':
				AppendStr(outTXT, "\\\"");
				break;
			case '\\':
				AppendStr(outTXT, "\\\\");
				break;
			case '\b':
				AppendStr(outTXT, "\\b");
				break;
			case '\f':
				AppendStr(outTXT, "\\f");
				break;
			case '\n':
				AppendStr(outTXT, "\\n");
				break;
			case '\r':
				AppendStr(outTXT, "\\r");
				break;
			case '\t':
				AppendStr(outTXT, "\\r");
				break;
			default:
				uint16_t unicodeCodePoint = MacRoman::ToUnicode(nameByte);
				if (unicodeCodePoint < 0x20 || unicodeCodePoint == 0x7f)
					AppendFmt(outTXT, "\\u%04x", static_cast<int>(unicodeCodePoint));
				else if (unicodeCodePoint > 0x7f)
					AppendUTF8(outTXT, unicodeCodePoint);
				else
					outTXT.push_back(nameByte);
				break;
			}
		}

		outTXT.push_back('\"');

		AppendStr(outTXT, ",\n\t\t\t\"itemType\" : ");

		switch (serializedType)
		{
		case SerializedDialogItemTypeCodes::kUserItem:
			AppendQuotedStr(outTXT, "UserItem");
			break;
		case SerializedDialogItemTypeCodes::kButton:
			AppendQuotedStr(outTXT, "Button");
			break;
		case SerializedDialogItemTypeCodes::kCheckBox:
			AppendQuotedStr(outTXT, "CheckBox");
			break;
		case SerializedDialogItemTypeCodes::kRadioButton:
			AppendQuotedStr(outTXT, "RadioButton");
			break;
		case SerializedDialogItemTypeCodes::kCustomControl:
			AppendQuotedStr(outTXT, "CustomControl");
			break;
		case SerializedDialogItemTypeCodes::kLabel:
			AppendQuotedStr(outTXT, "Label");
			break;
		case SerializedDialogItemTypeCodes::kEditBox:
			AppendQuotedStr(outTXT, "EditBox");
			break;
		case SerializedDialogItemTypeCodes::kIcon:
			AppendQuotedStr(outTXT, "Icon");
			break;
		case SerializedDialogItemTypeCodes::kImage:
			AppendQuotedStr(outTXT, "Image");
			break;
		default:
			AppendFmt(outTXT, "%i", static_cast<int>(serializedType));
			break;
		}

		AppendFmt(outTXT, ",\n\t\t\t\"pos\" : [ %i, %i ]", rect.left, rect.top);
		AppendFmt(outTXT, ",\n\t\t\t\"size\" : [ %i, %i ]", rect.right - rect.left, rect.bottom - rect.top);
		AppendFmt(outTXT, ",\n\t\t\t\"id\" : %i", static_cast<int16_t>(id));
		AppendFmt(outTXT, ",\n\t\t\t\"enabled\" : %s", enabled ? "true" : "false");
		AppendStr(outTXT, "\n\t\t}");
	}

	AppendStr(outTXT, "\n\t]\n");
	AppendStr(outTXT, "}");

	return true;
}

bool ImportIcon(std::vector<uint8_t> &outBMP, const void *inData, size_t inSize, uint8_t width, uint8_t height, uint8_t bpp)
{
	size_t expectedSize = width * height * bpp / 8;
	if (inSize != expectedSize)
		return false;

	PortabilityLayer::RGBAColor bwColors[] = { PortabilityLayer::RGBAColor::Create(255, 255, 255, 255), PortabilityLayer::RGBAColor::Create(0, 0, 0, 255) };
	const PortabilityLayer::RGBAColor *palette = nullptr;

	if (bpp == 1)
		palette = bwColors;
	else if (bpp == 4)
		palette = PortabilityLayer::Icon4BitPalette::GetInstance()->GetColors();
	else if (bpp == 8)
		palette = PortabilityLayer::StandardPalette::GetInstance()->GetColors();
	else
		return false;


	size_t numPixels = width * height;

	std::vector<PortabilityLayer::RGBAColor> pixelData;
	pixelData.resize(numPixels);

	int bitOffset = 8;
	size_t byteOffset = 0;
	int mask = (1 << bpp) - 1;
	for (size_t i = 0; i < numPixels; i++)
	{
		if (bitOffset == 0)
		{
			byteOffset++;
			bitOffset = 8;
		}

		bitOffset -= bpp;

		int value = static_cast<const uint8_t*>(inData)[byteOffset];
		value = (value >> bitOffset) & mask;
		pixelData[i] = palette[value];
	}

	return ExportBMP(width, height, width, &pixelData[0], outBMP);
}

void ReadFileToVector(FILE *f, std::vector<uint8_t> &vec)
{
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	vec.resize(fsize);

	if (fsize > 0)
	{
		fseek(f, 0, SEEK_SET);
		fread(&vec[0], 1, fsize, f);
	}
}

bool ParsePatchNames(const std::vector<uint8_t> &patchFileContents, std::vector<std::string> &names)
{
	rapidjson::Document document;
	document.Parse(reinterpret_cast<const char*>(&patchFileContents[0]), patchFileContents.size());
	if (document.HasParseError())
	{
		fprintf(stderr, "Error occurred parsing patch data");
		fprintf(stderr, "Error code %i  Location %i", static_cast<int>(document.GetParseError()), static_cast<int>(document.GetErrorOffset()));
		return false;
	}

	if (!document.IsObject())
	{
		fprintf(stderr, "Patch document is not an object");
		return false;
	}

	if (document.HasMember("add"))
	{
		const rapidjson::Value &addValue = document["add"];
		if (!addValue.IsObject())
		{
			fprintf(stderr, "Patch add list is not an object");
			return false;
		}

		for (rapidjson::Value::ConstMemberIterator it = addValue.MemberBegin(), itEnd = addValue.MemberEnd(); it != itEnd; ++it)
		{
			const rapidjson::Value &itemName = it->name;
			if (!itemName.IsString())
			{
				fprintf(stderr, "Patch add list item key is not a string");
				return false;
			}

			const char *itemNameStr = itemName.GetString();
			names.push_back(std::string(itemNameStr));
		}
	}

	if (document.HasMember("delete"))
	{
		const rapidjson::Value &deleteValue = document["delete"];
		if (!deleteValue.IsArray())
		{
			fprintf(stderr, "Patch add list is not an object");
			return false;
		}

		for (const rapidjson::Value *it = deleteValue.Begin(), *itEnd = deleteValue.End(); it != itEnd; ++it)
		{
			const rapidjson::Value &item = *it;
			if (!item.IsString())
			{
				fprintf(stderr, "Patch delete list item key is not a string");
				return false;
			}

			names.push_back(std::string(item.GetString()));
		}
	}

	return true;
}

bool ApplyPatch(const std::vector<uint8_t> &patchFileContents, std::vector<PlannedEntry> &archive)
{
	rapidjson::Document document;
	document.Parse(reinterpret_cast<const char*>(&patchFileContents[0]), patchFileContents.size());
	if (document.HasParseError())
	{
		fprintf(stderr, "Error occurred parsing patch data");
		fprintf(stderr, "Error code %i  Location %i", static_cast<int>(document.GetParseError()), static_cast<int>(document.GetErrorOffset()));
		return false;
	}

	if (!document.IsObject())
	{
		fprintf(stderr, "Patch document is not an object");
		return false;
	}

	if (document.HasMember("add"))
	{
		const rapidjson::Value &addValue = document["add"];
		if (!addValue.IsObject())
		{
			fprintf(stderr, "Patch add list is not an object");
			return false;
		}

		for (rapidjson::Value::ConstMemberIterator it = addValue.MemberBegin(), itEnd = addValue.MemberEnd(); it != itEnd; ++it)
		{
			const rapidjson::Value &itemName = it->name;
			if (!itemName.IsString())
			{
				fprintf(stderr, "Patch add list item key is not a string");
				return false;
			}

			const rapidjson::Value &itemValue = it->value;
			if (!itemValue.IsString())
			{
				fprintf(stderr, "Patch add list item value is not a string");
				return false;
			}

			const char *itemNameStr = itemName.GetString();
			for (size_t i = 0; itemNameStr[i] != '\0'; i++)
			{
				if (itemNameStr[i] == '/')
				{
					PlannedEntry *directoryEntry = nullptr;
					std::string dirName = std::string(itemNameStr, i);

					for (std::vector<PlannedEntry>::iterator it = archive.begin(), itEnd = archive.end(); it != itEnd; ++it)
					{
						if (it->m_name == dirName)
						{
							directoryEntry = &(*it);
							break;
						}
					}

					if (!directoryEntry)
					{
						archive.push_back(PlannedEntry());
						directoryEntry = &archive.back();
						directoryEntry->m_name = dirName;
						directoryEntry->m_isDirectory = true;
					}
				}
			}

			PlannedEntry *entry = nullptr;
			for (std::vector<PlannedEntry>::iterator it = archive.begin(), itEnd = archive.end(); it != itEnd; ++it)
			{
				if (it->m_name == itemName.GetString())
				{
					entry = &(*it);
					break;
				}
			}

			if (!entry)
			{
				archive.push_back(PlannedEntry());
				entry = &archive.back();
				entry->m_name = itemName.GetString();
			}

			FILE *f = fopen_utf8(itemValue.GetString(), "rb");
			if (!f)
			{
				fprintf(stderr, "Could not find source file %s for patch", static_cast<const char*>(itemValue.GetString()));
				return false;
			}

			entry->m_isDirectory = false;
			entry->m_uncompressedContents.clear();
			ReadFileToVector(f, entry->m_uncompressedContents);
			fclose(f);
		}
	}

	if (document.HasMember("delete"))
	{
		const rapidjson::Value &deleteValue = document["delete"];
		if (!deleteValue.IsArray())
		{
			fprintf(stderr, "Patch add list is not an object");
			return false;
		}

		for (const rapidjson::Value *it = deleteValue.Begin(), *itEnd = deleteValue.End(); it != itEnd; ++it)
		{
			const rapidjson::Value &item = *it;
			if (!item.IsString())
			{
				fprintf(stderr, "Patch delete list item key is not a string");
				return false;
			}

			PlannedEntry *entry = nullptr;
			for (std::vector<PlannedEntry>::iterator it = archive.begin(), itEnd = archive.end(); it != itEnd; ++it)
			{
				if (it->m_name == item.GetString())
				{
					archive.erase(it);
					break;
				}
			}
		}
	}

	return true;
}

bool ContainsName(const std::vector<std::string> &names, const char *name)
{
	for (const std::string &vname : names)
		if (vname == name)
			return true;

	return false;
}

int ConvertSingleFile(const char *resPath, const PortabilityLayer::CombinedTimestamp &ts, FILE *patchF, const char *dumpqtDir, const char *outPath)
{
	FILE *inF = fopen_utf8(resPath, "rb");
	if (!inF)
	{
		fprintf(stderr, "Error opening input file");
		return -1;
	}

	bool havePatchFile = false;
	std::vector<uint8_t> patchFileContents;

	if (patchF)
	{
		havePatchFile = true;
		ReadFileToVector(patchF, patchFileContents);
		fclose(patchF);
	}

	PortabilityLayer::CFileStream cfs(inF);

	GpDriverCollection *drivers = PLDrivers::GetDriverCollection();
	drivers->SetDriver<GpDriverIDs::kAlloc>(GpAllocator_C::GetInstance());

	PortabilityLayer::ResourceFile *resFile = PortabilityLayer::ResourceFile::Create();
	resFile->Load(&cfs);

	PortabilityLayer::ResourceCompiledTypeList *typeLists = nullptr;
	size_t typeListCount = 0;
	resFile->GetAllResourceTypeLists(typeLists, typeListCount);

	std::vector<PlannedEntry> contents;

	const PortabilityLayer::ResTypeID pictTypeID = PortabilityLayer::ResTypeID('PICT');
	const PortabilityLayer::ResTypeID dateTypeID = PortabilityLayer::ResTypeID('Date');
	const PortabilityLayer::ResTypeID sndTypeID = PortabilityLayer::ResTypeID('snd ');
	const PortabilityLayer::ResTypeID indexStringTypeID = PortabilityLayer::ResTypeID('STR#');
	const PortabilityLayer::ResTypeID ditlTypeID = PortabilityLayer::ResTypeID('DITL');

	struct IconTypeSpec
	{
		uint8_t m_width;
		uint8_t m_height;
		uint8_t m_bpp;
		PortabilityLayer::ResTypeID m_resTypeID;
	};

	const IconTypeSpec iconTypeSpecs[] =
	{
		{ 32, 64, 1, PortabilityLayer::ResTypeID('ICN#') },
		{ 32, 32, 4, PortabilityLayer::ResTypeID('icl4') },
		{ 32, 32, 8, PortabilityLayer::ResTypeID('icl8') },
		{ 16, 32, 1, PortabilityLayer::ResTypeID('ics#') },
		{ 16, 16, 4, PortabilityLayer::ResTypeID('ics4') },
		{ 16, 16, 8, PortabilityLayer::ResTypeID('ics8') },
	};

	std::vector<std::string> reservedNames;

	if (havePatchFile)
	{
		if (!ParsePatchNames(patchFileContents, reservedNames))
			return -1;
	}

	for (size_t tlIndex = 0; tlIndex < typeListCount; tlIndex++)
	{
		const PortabilityLayer::ResourceCompiledTypeList &typeList = typeLists[tlIndex];
		const PortabilityLayer::GpArcResourceTypeTag resTag = PortabilityLayer::GpArcResourceTypeTag::Encode(typeList.m_resType);

		const PortabilityLayer::ResourceCompiledRef *refList = typeList.m_firstRef;
		const size_t numRefs = typeList.m_numRefs;

		{
			PlannedEntry entry;
			entry.m_name = resTag.m_id;
			entry.m_isDirectory = true;

			contents.push_back(std::move(entry));
		}

		for (size_t rlIndex = 0; rlIndex < numRefs; rlIndex++)
		{
			const PortabilityLayer::ResourceCompiledRef &res = refList[rlIndex];
			const void *resData = res.m_resData;
			const size_t resSize = res.GetSize();

			std::string resComment;

			if (res.m_resNameOffset >= 0)
			{
				const uint8_t *pstrStart = resFile->GetResNames() + res.m_resNameOffset;
				resComment = std::string(reinterpret_cast<const char*>(pstrStart + 1), pstrStart[0]);
			}

			if (typeList.m_resType == pictTypeID || typeList.m_resType == dateTypeID)
			{
				std::string resName = (
					std::ostringstream() << resTag.m_id << '/' << res.m_resID << ".bmp"
				).str();

				if (ContainsName(reservedNames, resName.c_str()))
					continue;

				PlannedEntry entry;
				entry.m_name = std::move(resName);
				entry.m_comment = resComment;

				if (ImportPICT(entry.m_uncompressedContents, resData, resSize, dumpqtDir, res.m_resID))
					contents.push_back(entry);
				else
					fprintf(stderr, "Failed to import PICT res %i\n", static_cast<int>(res.m_resID));
			}
			else if (typeList.m_resType == sndTypeID)
			{
				std::string resName = (
					std::ostringstream() << resTag.m_id << '/' << res.m_resID << ".wav"
				).str();

				if (ContainsName(reservedNames, resName.c_str()))
					continue;

				PlannedEntry entry;
				entry.m_name = std::move(resName);
				entry.m_comment = resComment;

				if (ImportSound(entry.m_uncompressedContents, resData, resSize, res.m_resID))
					contents.push_back(entry);
				else
					fprintf(stderr, "Failed to import snd res %i\n", static_cast<int>(res.m_resID));
			}
			else if (typeList.m_resType == indexStringTypeID)
			{
				std::string resName = (
					std::ostringstream() << resTag.m_id << '/' << res.m_resID << ".txt"
				).str();

				if (ContainsName(reservedNames, resName.c_str()))
					continue;

				PlannedEntry entry;
				entry.m_name = std::move(resName);
				entry.m_comment = resComment;

				if (ImportIndexedString(entry.m_uncompressedContents, resData, resSize))
					contents.push_back(entry);
			}
			else if (typeList.m_resType == ditlTypeID)
			{
				std::string resName = (
					std::ostringstream() << resTag.m_id << '/' << res.m_resID << ".json"
				).str();

				if (ContainsName(reservedNames, resName.c_str()))
					continue;

				PlannedEntry entry;
				entry.m_name = std::move(resName);
				entry.m_comment = resComment;

				if (ImportDialogItemTemplate(entry.m_uncompressedContents, resData, resSize))
					contents.push_back(entry);
			}
			else
			{
				bool isIcon = false;

				for (int i = 0; i < sizeof(iconTypeSpecs) / sizeof(iconTypeSpecs[0]); i++)
				{
					const IconTypeSpec &iconSpec = iconTypeSpecs[i];
					if (typeList.m_resType == iconSpec.m_resTypeID)
					{
						std::string resName = (
							std::ostringstream() << resTag.m_id << '/' << res.m_resID << ".bmp"
						).str();

						if (!ContainsName(reservedNames, resName.c_str()))
						{
							isIcon = true;

							PlannedEntry entry;
							entry.m_name = std::move(resName);
							entry.m_comment = resComment;

							if (ImportIcon(entry.m_uncompressedContents, resData, resSize, iconSpec.m_width, iconSpec.m_height, iconSpec.m_bpp))
								contents.push_back(entry);

							break;
						}
					}
				}

				if (!isIcon)
				{
					std::string resName = (
						std::ostringstream() << resTag.m_id << '/' << res.m_resID << ".bin"
					).str();

					if (ContainsName(reservedNames, resName.c_str()))
						continue;

					PlannedEntry entry;

					entry.m_name = std::move(resName);
					entry.m_comment = resComment;
					entry.m_uncompressedContents.resize(res.GetSize());

					memcpy(&entry.m_uncompressedContents[0], resData, resSize);

					contents.push_back(entry);
				}
			}
		}
	}

	cfs.Close();

	if (havePatchFile)
	{
		if (!ApplyPatch(patchFileContents, contents))
			return -1;
	}

	std::sort(contents.begin(), contents.end(), EntryAlphaSortPredicate);

	ExportZipFile(outPath, contents, ts);

	resFile->Destroy();

	return 0;
}

int ConvertDirectory(const std::string &basePath, const PortabilityLayer::CombinedTimestamp &ts)
{
	std::vector<std::string> paths;
	ScanDirectoryForExtension(paths, basePath.c_str(), ".gpr", true);

	for (std::vector<std::string>::const_iterator it = paths.begin(), itEnd = paths.end(); it != itEnd; ++it)
	{
		const std::string &resPath = *it;
		std::string housePathBase = resPath.substr(0, resPath.length() - 4);

		std::string metaPath = housePathBase + ".gpf";

		FILE *metaF = fopen_utf8(metaPath.c_str(), "rb");
		if (!metaF)
		{
			fprintf(stderr, "Failed to open metadata file ");
			fputs_utf8(metaPath.c_str(), stderr);
			fprintf(stderr, "\n");
			return -1;
		}

		PortabilityLayer::MacFilePropertiesSerialized mfps;
		if (fread(mfps.m_data, 1, PortabilityLayer::MacFilePropertiesSerialized::kSize, metaF) != PortabilityLayer::MacFilePropertiesSerialized::kSize)
		{
			fclose(metaF);
			fprintf(stderr, "Failed to load metadata file ");
			fputs_utf8(metaPath.c_str(), stderr);
			fprintf(stderr, "\n");
			return -1;
		}
		fclose(metaF);

		PortabilityLayer::MacFileProperties mfp;
		mfps.Deserialize(mfp);

		if (mfp.m_fileType[0] == 'g' && mfp.m_fileType[1] == 'l' && mfp.m_fileType[2] == 'i' && mfp.m_fileType[3] == 'H')
		{
			std::string houseArchivePath = (housePathBase + ".gpa");
			fprintf(stdout, "Importing ");
			fputs_utf8(houseArchivePath.c_str(), stdout);
			fprintf(stdout, "\n");

			int returnCode = ConvertSingleFile(resPath.c_str(), ts, nullptr, nullptr, houseArchivePath.c_str());
			if (returnCode)
			{
				fprintf(stderr, "An error occurred while converting\n");
				fputs_utf8(resPath.c_str(), stderr);
				fprintf(stderr, "\n");
				return returnCode;
			}
		}
	}

	return 0;
}

int PrintUsage()
{
	fprintf(stderr, "Usage: gpr2gpa <input.gpr> <input.ts> <output.gpa> [options]\n");
	fprintf(stderr, "       gpr2gpa <input dir>\\* <input.ts>\n");
	fprintf(stderr, "       gpr2gpa <input dir>/* <input.ts>\n");
	fprintf(stderr, "       gpr2gpa * <input.ts>\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "       -patch patch.json\n");
	fprintf(stderr, "       -dumpqt <temp dir>\n");

	return -1;
}

int toolMain(int argc, const char **argv)
{
	if (argc < 3)
		return PrintUsage();

	FILE *timestampF = fopen_utf8(argv[2], "rb");
	if (!timestampF)
	{
		fprintf(stderr, "Error opening timestamp file");
		return -1;
	}

	PortabilityLayer::CombinedTimestamp ts;
	if (fread(&ts, 1, sizeof(ts), timestampF) != sizeof(ts))
	{
		fprintf(stderr, "Error reading timestamp");
		return -1;
	}

	fclose(timestampF);

	std::string base = argv[1];

	if (base == "*")
		return ConvertDirectory(".", ts);

	if (base.length() >= 2)
	{
		std::string baseEnding = base.substr(base.length() - 2, 2);
		if (baseEnding == "\\*" || baseEnding == "/*")
			return ConvertDirectory(base.substr(0, base.length() - 2), ts);
	}

	const char *dumpqtPath = nullptr;
	FILE *patchF = nullptr;
	if (argc > 4)
	{
		for (int optArgIndex = 4; optArgIndex < argc; )
		{
			const char *optArg = argv[optArgIndex++];
			if (!strcmp(optArg, "-patch"))
			{
				if (optArgIndex == argc)
					return PrintUsage();

				if (patchF != nullptr)
				{
					fprintf(stderr, "Already specified patch file");
					return -1;
				}

				const char *patchPath = argv[optArgIndex++];
				patchF = fopen_utf8(patchPath, "rb");
				if (!patchF)
				{
					fprintf(stderr, "Error reading patch file");
					return -1;
				}
			}
			else if (!strcmp(optArg, "-dumpqt"))
			{
				if (optArgIndex == argc)
					return PrintUsage();

				dumpqtPath = argv[optArgIndex++];
			}
			else
				return PrintUsage();
		}
	}

	return ConvertSingleFile(argv[1], ts, patchF, dumpqtPath, argv[3]);
}
