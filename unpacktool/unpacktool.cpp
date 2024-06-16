#include <stdio.h>

#include "IArchiveParser.h"
#include "IFileReader.h"
#include "StuffItParser.h"
#include "StuffIt5Parser.h"
#include "CompactProParser.h"
#include "CFileStream.h"

#include "GpUnicode.h"

#include "ArchiveDescription.h"
#include "IDecompressor.h"

#include "NullDecompressor.h"
#include "RLE90Decompressor.h"
#include "LZWDecompressor.h"
#include "StuffIt13Decompressor.h"
#include "StuffItHuffmanDecompressor.h"
#include "StuffItArsenicDecompressor.h"
#include "CompactProRLEDecompressor.h"
#include "CompactProLZHRLEDecompressor.h"

#include "CSInputBuffer.h"
#include "WindowsUnicodeToolShim.h"

#include "CombinedTimestamp.h"

#include <string.h>

#include <string>
#include <vector>

class CFileReader final : public IFileReader
{
public:
	explicit CFileReader(FILE *f);

	size_t Read(void *buffer, size_t sz);
	size_t FileSize() const override;

	bool SeekStart(FilePos_t pos) override;
	bool SeekCurrent(FilePos_t pos) override;
	bool SeekEnd(FilePos_t pos) override;
	FilePos_t GetPosition() const override;

private:
	FILE *m_file;
	long m_size;
};


CFileReader::CFileReader(FILE *f)
	: m_file(f)
{
	fseek(f, 0, SEEK_END);
	m_size = ftell(f);
	fseek(f, 0, SEEK_SET);
}

size_t CFileReader::Read(void *buffer, size_t sz)
{
	return fread(buffer, 1, sz, m_file);
}

size_t CFileReader::FileSize() const
{
	return static_cast<size_t>(m_size);
}

bool CFileReader::SeekStart(FilePos_t pos)
{
	return !_fseeki64(m_file, pos, SEEK_SET);
}

bool CFileReader::SeekCurrent(FilePos_t pos)
{
	return !_fseeki64(m_file, pos, SEEK_CUR);
}

bool CFileReader::SeekEnd(FilePos_t pos)
{
	return !_fseeki64(m_file, pos, SEEK_END);
}

IFileReader::FilePos_t CFileReader::GetPosition() const
{
	return _ftelli64(m_file);
}

StuffItParser g_stuffItParser;
StuffIt5Parser g_stuffIt5Parser;
CompactProParser g_compactProParser;

static bool IsSeparator(char c)
{
	return c == '/' || c == '\\';
}

std::string LegalizeWindowsFileName(const std::string &path, bool paranoid)
{
	const size_t length = path.length();

	std::string legalizedPath;

	for (size_t i = 0; i < length; i++)
	{
		const char c = path[i];
		bool isLegalChar = true;
		if (c >= '\0' && c <= 31)
			isLegalChar = false;
		else if (c == '<' || c == '>' || c == ':' || c == '\"' || c == '/' || c == '\\' || c == '|' || c == '?' || c == '*')
			isLegalChar = false;
		else if (c == ' ' || c == '.')
		{
			if (i == length - 1)
				isLegalChar = false;
		}

		if (paranoid && isLegalChar)
			isLegalChar = c == '_' || c == ' ' || c == '.' || c == ',' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');

		if (isLegalChar)
			legalizedPath.append(&c, 1);
		else
		{
			const char *hexChars = "0123456789abcdef";
			char legalizedCharacter[3];
			legalizedCharacter[0] = '$';
			legalizedCharacter[1] = hexChars[(c >> 4) & 0xf];
			legalizedCharacter[2] = hexChars[c & 0xf];

			legalizedPath.append(legalizedCharacter, 3);
		}
	}

	const char *bannedNames[] =
	{
		"CON",
		"PRN",
		"AUX",
		"NUL",
		"COM1",
		"COM2",
		"COM3",
		"COM4",
		"COM5",
		"COM6",
		"COM7",
		"COM8",
		"COM9",
		"LPT1",
		"LPT2",
		"LPT3",
		"LPT4",
		"LPT5",
		"LPT6",
		"LPT7",
		"LPT8",
		"LPT9"
	};

	const size_t numBannedNames = sizeof(bannedNames) / sizeof(bannedNames[0]);

	for (size_t i = 0; i < numBannedNames; i++)
	{
		const size_t banLength = strlen(bannedNames[i]);
		const size_t legalizedPathLength = legalizedPath.length();

		bool isThisBannedName = false;
		if (legalizedPathLength >= banLength)
		{
			bool startsWithBannedName = true;
			for (size_t ci = 0; ci < banLength; ci++)
			{
				int charDelta = bannedNames[i][ci] - legalizedPath[ci];
				if (charDelta != 0 && charDelta != ('A' - 'a'))
				{
					startsWithBannedName = false;
					break;
				}
			}

			if (startsWithBannedName)
			{
				if (legalizedPathLength == banLength)
				{
					legalizedPath.append("$");
					break;
				}
				else if (legalizedPath[banLength] == '.')
				{
					legalizedPath = legalizedPath.substr(0, banLength) + "$" + legalizedPath.substr(banLength);
					break;
				}
			}
		}
	}

	if (legalizedPath.length() == 0)
		legalizedPath = "$";

	return legalizedPath;
}

void MakeIntermediateDirectories(const std::string &path)
{
	size_t l = path.length();

	for (size_t i = 0; i < l; i++)
	{
		if (path[i] == '/' || path[i] == '\\')
			mkdir_utf8(path.substr(0, i).c_str());
	}
}

int RecursiveExtractFiles(int depth, ArchiveItemList *itemList, const std::string &path, bool pathParanoid, IFileReader &reader, const PortabilityLayer::CombinedTimestamp &ts);

int ExtractSingleFork(const ArchiveCompressedChunkDesc &chunkDesc, const std::string &path, IFileReader &reader)
{
	if (chunkDesc.m_uncompressedSize == 0)
		return 0;

	if (!reader.SeekStart(chunkDesc.m_filePosition))
	{
		fprintf(stderr, "Could not seek to input position\n");
		return -1;
	}

	FILE *metadataF = fopen_utf8(path.c_str(), "wb");
	if (!metadataF)
	{
		fprintf(stderr, "Could not open output file %s\n", path.c_str());
		return -1;
	}

	IDecompressor *decompressor = nullptr;
	switch (chunkDesc.m_compressionMethod)
	{
	case CompressionMethods::kNone:
		decompressor = new NullDecompressor();
		break;
	case CompressionMethods::kStuffItRLE90:
		decompressor = new RLE90Decompressor();
		break;
	case CompressionMethods::kStuffItLZW:
		decompressor = new LZWDecompressor(0x8e);
		break;
	case CompressionMethods::kStuffItHuffman:
		decompressor = new StuffItHuffmanDecompressor();
		break;
	case CompressionMethods::kStuffIt13:
		decompressor = new StuffIt13Decompressor();
		break;
	case CompressionMethods::kStuffItArsenic:
		decompressor = new StuffItArsenicDecompressor();
		break;
	case CompressionMethods::kCompactProRLE:
		decompressor = new CompactProRLEDecompressor();
		break;
	case CompressionMethods::kCompactProLZHRLE:
		decompressor = new CompactProLZHRLEDecompressor(0x1fff0);
		break;
	default:
		break;
	}

	if (!decompressor)
	{
		fprintf(stderr, "Could not decompress file %s, compression method %i is not implemented\n", path.c_str(), static_cast<int>(chunkDesc.m_compressionMethod));
		fclose(metadataF);
		return -1;
	}

	CSInputBuffer *input = CSInputBufferAlloc(&reader, 2048);

	if (!input)
	{
		fprintf(stderr, "Could not decompress file %s, buffer init failed\n", path.c_str());
		delete decompressor;
		fclose(metadataF);
		return -1;
	}

	if (!decompressor->Reset(input, chunkDesc.m_compressedSize, chunkDesc.m_uncompressedSize))
	{
		fprintf(stderr, "Could not decompress file %s, decompression init failed\n", path.c_str());
		CSInputBufferFree(input);
		delete decompressor;
		fclose(metadataF);
		return -1;
	}

	const size_t kDecompressionBufferSize = 4096;
	uint8_t decompressionBuffer[kDecompressionBufferSize];
	size_t decompressedBytesRemaining = chunkDesc.m_uncompressedSize;

	while (decompressedBytesRemaining > 0)
	{
		size_t decompressAmount = decompressedBytesRemaining;
		if (decompressAmount > kDecompressionBufferSize)
			decompressAmount = kDecompressionBufferSize;

		if (!decompressor->ReadBytes(decompressionBuffer, decompressAmount))
		{
			fprintf(stderr, "Could not decompress file %s, byte read failed\n", path.c_str());
			CSInputBufferFree(input);
			delete decompressor;
			fclose(metadataF);
			return -1;
		}

		if (fwrite(decompressionBuffer, 1, decompressAmount, metadataF) != decompressAmount)
		{
			fprintf(stderr, "Could not decompress file %s, write failed\n", path.c_str());
			CSInputBufferFree(input);
			delete decompressor;
			fclose(metadataF);
			return -1;
		}

		decompressedBytesRemaining -= decompressAmount;
	}

	delete decompressor;
	CSInputBufferFree(input);
	fclose(metadataF);
	return 0;
}

int ExtractFile(const ArchiveItem &item, const std::string &path, IFileReader &reader, const PortabilityLayer::CombinedTimestamp &ts)
{
	PortabilityLayer::MacFilePropertiesSerialized mfps;
	mfps.Serialize(item.m_macProperties);

	std::string metadataPath = (path + ".gpf");
	std::string dataPath = (path + ".gpd");
	std::string resPath = (path + ".gpr");

	FILE *metadataF = fopen_utf8(metadataPath.c_str(), "wb");
	if (!metadataF)
	{
		fprintf(stderr, "Could not open metadata output file %s", metadataPath.c_str());
		return -1;
	}

	PortabilityLayer::CFileStream metadataStream(metadataF);

	if (!mfps.WriteAsPackage(metadataStream, ts))
	{
		fprintf(stderr, "A problem occurred writing metadata");
		metadataStream.Close();
		return -1;
	}

	metadataStream.Close();

	int returnCode = ExtractSingleFork(item.m_dataForkDesc, dataPath, reader);
	if (returnCode)
		return returnCode;

	returnCode = ExtractSingleFork(item.m_resourceForkDesc, resPath, reader);
	if (returnCode)
		return returnCode;

	return 0;
}

int ExtractItem(int depth, const ArchiveItem &item, const std::string &dirPath, bool pathParanoid, IFileReader &reader, const PortabilityLayer::CombinedTimestamp &ts)
{
	std::string path(reinterpret_cast<const char*>(item.m_fileNameUTF8.data()), item.m_fileNameUTF8.size());

	for (int i = 0; i < depth; i++)
		printf("  ");

	fputs_utf8(path.c_str(), stdout);
	printf("\n");

	path = LegalizeWindowsFileName(path, pathParanoid);

	path = dirPath + path;

	if (item.m_isDirectory)
	{
		mkdir_utf8(path.c_str());

		path.append("/");

		int returnCode = RecursiveExtractFiles(depth + 1, item.m_children, path, pathParanoid, reader, ts);
		if (returnCode)
			return returnCode;

		return 0;
	}
	else
		return ExtractFile(item, path, reader, ts);
}

int RecursiveExtractFiles(int depth, ArchiveItemList *itemList, const std::string &path, bool pathParanoid, IFileReader &reader, const PortabilityLayer::CombinedTimestamp &ts)
{
	const std::vector<ArchiveItem> &items = itemList->m_items;

	const size_t numChildren = items.size();
	for (size_t i = 0; i < numChildren; i++)
	{
		int returnCode = ExtractItem(depth, items[i], path, pathParanoid, reader, ts);
		if (returnCode)
			return returnCode;
	}

	return 0;
}

int PrintUsage()
{
	fprintf(stderr, "Usage: unpacktool <archive file> <timestamp.ts> <destination> [options]");
	fprintf(stderr, "Usage: unpacktool -bulk <timestamp.ts> <archive files>");
	return -1;
}

int decompMain(int argc, const char **argv)
{
	for (int i = 0; i < argc; i++)
		printf("%s\n", argv[i]);

	if (argc < 4)
		return PrintUsage();

	bool isBulkMode = !strcmp(argv[1], "-bulk");

	if (!isBulkMode && argc < 4)
		return PrintUsage();

	FILE *tsFile = fopen_utf8(argv[2], "rb");

	if (!tsFile)
	{
		fprintf(stderr, "Could not open timestamp file");
		return -1;
	}

	PortabilityLayer::CombinedTimestamp ts;
	if (!fread(&ts, sizeof(ts), 1, tsFile))
	{
		fprintf(stderr, "Could not read timestamp");
		return -1;
	}

	fclose(tsFile);

	int arcArg = 1;
	int numArgArcs = 1;

	if (isBulkMode)
	{
		arcArg = 3;
		numArgArcs = argc - 3;
	}

	bool pathParanoid = false;
	if (!isBulkMode)
	{
		for (int optArgIndex = 4; optArgIndex < argc; )
		{
			const char *optArg = argv[optArgIndex++];

			if (!strcmp(optArg, "-paranoid"))
				pathParanoid = true;
			else
			{
				fprintf(stderr, "Unknown option %s\n", optArg);
				return -1;
			}
		}
	}

	for (int arcArgIndex = 0; arcArgIndex < numArgArcs; arcArgIndex++)
	{
		const char *arcPath = argv[arcArg + arcArgIndex];

		FILE *inputArchive = fopen_utf8(arcPath, "rb");

		std::string destPath;
		if (isBulkMode)
		{
			destPath = arcPath;
			size_t lastSepIndex = 0;
			for (size_t i = 1; i < destPath.size(); i++)
			{
				if (destPath[i] == '/' || destPath[i] == '\\')
					lastSepIndex = i;
			}

			destPath = destPath.substr(0, lastSepIndex);
		}
		else
			destPath = argv[3];

		if (!inputArchive)
		{
			fprintf(stderr, "Could not open input archive");
			return -1;
		}

		CFileReader reader(inputArchive);

		IArchiveParser *parsers[] =
		{
			&g_compactProParser,
			&g_stuffItParser,
			&g_stuffIt5Parser
		};

		ArchiveItemList *archiveItemList = nullptr;

		printf("Reading archive '%s'...\n", arcPath);

		for (IArchiveParser *parser : parsers)
		{
			if (parser->Check(reader))
			{
				archiveItemList = parser->Parse(reader);
				break;
			}
		}

		if (!archiveItemList)
		{
			fprintf(stderr, "Failed to open archive");
			return -1;
		}

		printf("Decompressing files...\n");

		std::string currentPath = destPath;
		TerminateDirectoryPath(currentPath);

		MakeIntermediateDirectories(currentPath);

		int returnCode = RecursiveExtractFiles(0, archiveItemList, currentPath, pathParanoid, reader, ts);
		if (returnCode != 0)
		{
			fprintf(stderr, "Error decompressing archive");
			return returnCode;
		}

		delete archiveItemList;
	}

	return 0;
}


int toolMain(int argc, const char **argv)
{
	int returnCode = decompMain(argc, argv);
	return returnCode;
}
