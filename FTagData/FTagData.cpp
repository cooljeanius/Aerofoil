#include <stdio.h>
#include <string>

#include "MacFileInfo.h"
#include "CFileStream.h"
#include "CombinedTimestamp.h"

#include "WindowsUnicodeToolShim.h"

int toolMain(int argc, const char **argv)
{
	if (argc < 7)
	{
		fprintf(stderr, "FTagData <timestamp> <output.gpf> <file type ID> <file creator ID> <x pos> <y pos> [flags]");
		return -1;
	}

	std::string timestampPath = argv[1];
	std::string outPath = argv[2];

	if (strlen(argv[3]) != 4)
	{
		fprintf(stderr, "File type ID must be 4 characters");
		return -2;
	}

	if (strlen(argv[4]) != 4)
	{
		fprintf(stderr, "File creator ID must be 4 characters");
		return -3;
	}

	FILE *tsF = fopen_utf8(timestampPath.c_str(), "rb");
	int64_t timestamp = 0;
	PortabilityLayer::CombinedTimestamp ts;

	if (tsF)
	{
		if (fread(&ts, 1, sizeof(ts), tsF) != sizeof(ts))
		{
			fprintf(stderr, "Error reading timestamp file");
			return -1;
		}

		fclose(tsF);
	}

	PortabilityLayer::MacFileProperties mfp;
	memcpy(mfp.m_fileType, argv[3], 4);
	memcpy(mfp.m_fileCreator, argv[4], 4);
	mfp.m_xPos = atoi(argv[5]);
	mfp.m_yPos = atoi(argv[6]);
	mfp.m_finderFlags = 0;
	mfp.m_protected = 0;
	mfp.m_modifiedTimeMacEpoch = mfp.m_createdTimeMacEpoch = timestamp;

	for (int i = 7; i < argc; i++)
	{
		const char *arg = argv[i];
		if (!strcmp(arg, "alias"))
			mfp.m_finderFlags |= PortabilityLayer::FINDER_FILE_FLAG_ALIAS;
		else if (!strcmp(arg, "invisible"))
			mfp.m_finderFlags |= PortabilityLayer::FINDER_FILE_FLAG_INVISIBLE;
		else if (!strcmp(arg, "bundle"))
			mfp.m_finderFlags |= PortabilityLayer::FINDER_FILE_FLAG_BUNDLE;
		else if (!strcmp(arg, "namelocked"))
			mfp.m_finderFlags |= PortabilityLayer::FINDER_FILE_FLAG_NAME_LOCKED;
		else if (!strcmp(arg, "stationary"))
			mfp.m_finderFlags |= PortabilityLayer::FINDER_FILE_FLAG_STATIONARY;
		else if (!strcmp(arg, "customicon"))
			mfp.m_finderFlags |= PortabilityLayer::FINDER_FILE_FLAG_CUSTOM_ICON;
		else if (!strcmp(arg, "inited"))
			mfp.m_finderFlags |= PortabilityLayer::FINDER_FILE_FLAG_INITED;
		else if (!strcmp(arg, "noinits"))
			mfp.m_finderFlags |= PortabilityLayer::FINDER_FILE_FLAG_NO_INITS;
		else if (!strcmp(arg, "shared"))
			mfp.m_finderFlags |= PortabilityLayer::FINDER_FILE_FLAG_SHARED;
		else if (!strcmp(arg, "locked"))
			mfp.m_protected = 1;
		else if (!strcmp(arg, "color"))
		{
			int color = 0;
			if (i < argc - 1)
			{
				i++;
				color = atoi(argv[i + 1]);
			}
			else
			{
				fprintf(stderr, "Color should be followed by a number ranging from 0 to 7");
				return -1;
			}

			if (color & 4)
				mfp.m_finderFlags |= PortabilityLayer::FINDER_FILE_FLAG_COLOR_BIT2;
			if (color & 2)
				mfp.m_finderFlags |= PortabilityLayer::FINDER_FILE_FLAG_COLOR_BIT1;
			if (color & 1)
				mfp.m_finderFlags |= PortabilityLayer::FINDER_FILE_FLAG_COLOR_BIT0;
		}
		else
		{
			fprintf(stderr, "Unknown flag: %s", arg);
			return -1;
		}
	}

	PortabilityLayer::MacFilePropertiesSerialized mps;
	mps.Serialize(mfp);

	FILE *file = fopen_utf8(outPath.c_str(), "wb");
	if (file)
	{
		PortabilityLayer::CFileStream stream(file);

		mps.WriteAsPackage(stream, ts);

		stream.Close();
	}

	return 0;
}
