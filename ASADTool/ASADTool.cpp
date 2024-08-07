#include "WindowsUnicodeToolShim.h"
#include "PLBigEndian.h"
#include "MacFileInfo.h"
#include "CombinedTimestamp.h"
#include "CFileStream.h"
#include "PLCore.h"

#include <algorithm>

// https://tools.ietf.org/rfc/rfc1740

int ProcessFork(FILE *f, uint32_t length, const char *basePath, const char *suffix)
{
	const size_t kBufferSize = 4096;

	uint8_t buffer[kBufferSize];

	std::string combinedPath = std::string(basePath) + suffix;

	FILE *outF = fopen_utf8(combinedPath.c_str(), "wb");
	if (!outF)
	{
		fprintf(stderr, "Failed to open output file '%s'", combinedPath.c_str());
		return -1;
	}

	while (length > 0)
	{
		const size_t amountToCopy = std::min<size_t>(length, kBufferSize);

		if (fread(buffer, 1, amountToCopy, f) != amountToCopy)
		{
			fprintf(stderr, "Failed to copy data");
			fclose(outF);
			return -1;
		}

		if (fwrite(buffer, 1, amountToCopy, outF) != amountToCopy)
		{
			fprintf(stderr, "Failed to copy data");
			fclose(outF);
			return -1;
		}

		length -= static_cast<uint32_t>(amountToCopy);
	}

	fclose(outF);
	return 0;
}

int ProcessFileDatesInfo(FILE *f, uint32_t length, PortabilityLayer::MacFileProperties &mfp, PortabilityLayer::CombinedTimestamp &ts)
{
	struct ASFileDates
	{
		BEInt32_t m_created;
		BEInt32_t m_modified;
		BEInt32_t m_backup;
		BEInt32_t m_access;
	};

	ASFileDates fileDates;
	if (length < sizeof(fileDates))
	{
		fprintf(stderr, "File dates block was truncated");
		return -1;
	}

	if (fread(&fileDates, 1, sizeof(fileDates), f) != sizeof(fileDates))
	{
		fprintf(stderr, "Failed to read file dates");
		return -1;
	}

	const int64_t asEpochToMacEpoch = 3029547600LL;

	// Mac epoch in Unix time: -2082844800
	// ASAD epoch in Unix time: 946702800

	mfp.m_createdTimeMacEpoch = static_cast<int64_t>(fileDates.m_created) + asEpochToMacEpoch;
	mfp.m_modifiedTimeMacEpoch = static_cast<int64_t>(fileDates.m_modified) + asEpochToMacEpoch;
	ts.SetMacEpochTime(mfp.m_modifiedTimeMacEpoch);

	return 0;
}

int ProcessFinderInfo(FILE *f, uint32_t length, PortabilityLayer::MacFileProperties &mfp)
{
	struct ASFinderInfo
	{
		uint8_t m_type[4];
		uint8_t m_creator[4];
		BEUInt16_t m_finderFlags;
		BEPoint m_location;
		BEUInt16_t m_folder;	// ???
	};

	struct ASExtendedFinderInfo
	{
		BEUInt16_t m_iconID;
		uint8_t m_unused[6];
		uint8_t m_scriptCode;
		uint8_t m_xFlags;
		BEUInt16_t m_commentID;
		BEUInt32_t m_putAwayDirectoryID;
	};

	ASFinderInfo finderInfo;
	if (length < sizeof(finderInfo))
	{
		fprintf(stderr, "Finder Info block was truncated");
		return -1;
	}

	if (fread(&finderInfo, 1, sizeof(finderInfo), f) != sizeof(finderInfo))
	{
		fprintf(stderr, "Failed to read Finder info");
		return -1;
	}

	memcpy(mfp.m_fileCreator, finderInfo.m_creator, 4);
	memcpy(mfp.m_fileType, finderInfo.m_type, 4);
	mfp.m_finderFlags = finderInfo.m_finderFlags;
	mfp.m_xPos = finderInfo.m_location.h;
	mfp.m_yPos = finderInfo.m_location.v;

	return 0;
}

int ProcessMacintoshFileInfo(FILE *f, uint32_t length, PortabilityLayer::MacFileProperties &mfp)
{
	struct ASMacInfo
	{
		uint8_t m_filler[3];
		uint8_t m_protected;
	};

	ASMacInfo macInfo;
	if (length < sizeof(macInfo))
	{
		fprintf(stderr, "File dates block was truncated");
		return -1;
	}

	if (fread(&macInfo, 1, sizeof(macInfo), f) != sizeof(macInfo))
	{
		fprintf(stderr, "Failed to read file dates");
		return -1;
	}

	mfp.m_protected = macInfo.m_protected;

	return 0;
}

int ProcessFile(FILE *f, const char *outPath, PortabilityLayer::CombinedTimestamp ts, bool isDouble)
{
	struct ASHeader
	{
		BEUInt32_t m_version;
		uint8_t m_filler[16];
		BEUInt16_t m_numEntries;
	};

	struct ASEntry
	{
		BEUInt32_t m_entryID;
		BEUInt32_t m_offset;
		BEUInt32_t m_length;
	};

	ASHeader header;
	if (fread(&header, 1, sizeof(header), f) != sizeof(header))
	{
		fprintf(stderr, "Failed to read header");
		return -1;
	}

	const uint32_t numEntries = header.m_numEntries;

	if (numEntries > 0xffff)
	{
		fprintf(stderr, "Too many entries");
		return -1;
	}

	if (numEntries == 0)
		return 0;

	std::vector<ASEntry> entries;
	entries.resize(static_cast<uint32_t>(numEntries));

	PortabilityLayer::MacFileProperties mfp;

	if (fread(&entries[0], 1, sizeof(ASEntry) * numEntries, f) != sizeof(ASEntry) * numEntries)
	{
		fprintf(stderr, "Failed to read entries");
		return -1;
	}

	for (const ASEntry &asEntry : entries)
	{
		int fseekResult = fseek(f, asEntry.m_offset, SEEK_SET);
		if (fseekResult != 0)
			return fseekResult;

		int rc = 0;
		switch (static_cast<uint32_t>(asEntry.m_entryID))
		{
		case 1:
			if (asEntry.m_length > 0)
				rc = ProcessFork(f, asEntry.m_length, outPath, ".gpd");
			break;
		case 2:
			if (asEntry.m_length > 0)
				rc = ProcessFork(f, asEntry.m_length, outPath, ".gpr");
			break;
		case 4:
			if (asEntry.m_length > 0)
				rc = ProcessFork(f, asEntry.m_length, outPath, ".gpc");
			break;
		case 8:
			rc = ProcessFileDatesInfo(f, asEntry.m_length, mfp, ts);
			break;
		case 9:
			rc = ProcessFinderInfo(f, asEntry.m_length, mfp);
			break;
		case 10:
			rc = ProcessMacintoshFileInfo(f, asEntry.m_length, mfp);
			break;
		case 3:		// Real name
		case 5:		// B&W icon
		case 6:		// Color icon
		case 11:	// ProDOS file info
		case 12:	// MS-DOS file info
		case 13:	// AFP short name
		case 14:	// AFP file info
		case 15:	// AFP directory ID
			break;
		default:
			fprintf(stderr, "Unknown entry type %i", static_cast<int>(static_cast<uint32_t>(asEntry.m_entryID)));
			return -1;
		}

		if (rc != 0)
			return rc;
	}

	PortabilityLayer::MacFilePropertiesSerialized mfps;
	mfps.Serialize(mfp);

	std::string gpfPath = std::string(outPath) + ".gpf";

	FILE *gpfFile = fopen_utf8(gpfPath.c_str(), "wb");
	if (!gpfFile)
	{
		fprintf(stderr, "Failed to open output gpf");
		return -1;
	}

	PortabilityLayer::CFileStream gpfStream(gpfFile);
	mfps.WriteAsPackage(gpfStream, ts);

	gpfStream.Close();

	return 0;
}

int toolMain(int argc, const char **argv)
{
	BEUInt32_t magic;

	if (argc != 4)
	{
		fprintf(stderr, "Usage: ASADTool <input> <timestamp.ts> <output>");
		return -1;
	}

	PortabilityLayer::CombinedTimestamp ts;
	FILE *tsFile = fopen_utf8(argv[2], "rb");
	if (!tsFile)
	{
		fprintf(stderr, "Could not open timestamp file");
		return -1;
	}

	if (fread(&ts, 1, sizeof(ts), tsFile) != sizeof(ts))
	{
		fprintf(stderr, "Could not read timestamp file");
		return -1;
	}

	fclose(tsFile);

	FILE *asadFile = fopen_utf8(argv[1], "rb");
	if (!asadFile)
	{
		fprintf(stderr, "Could not open input file");
		return -1;
	}

	if (fread(&magic, 1, 4, asadFile) != 4)
	{
		fprintf(stderr, "Could not read file magic");
		return -1;
	}

	int returnCode = 0;
	if (magic == 0x00051607)
		returnCode = ProcessFile(asadFile, argv[3], ts, true);
	else if (magic == 0x00051600)
		returnCode = ProcessFile(asadFile, argv[3], ts, false);
	else
	{
		fprintf(stderr, "Unknown file type %x", static_cast<int>(magic));
		return -1;
	}

	fclose(asadFile);

	return returnCode;
}
