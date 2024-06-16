#include <algorithm>
#include <stdio.h>
#include <string>
#include <vector>
#include <assert.h>

#include "BytePack.h"
#include "CFileStream.h"
#include "GpIOStream.h"

#include "MacBinary2.h"
#include "MacFileMem.h"
#include "WindowsUnicodeToolShim.h"

// Very simplified resource compiler

using namespace PortabilityLayer;

struct SimpleResource
{
	char m_type[4];
	int m_resourceID;
	std::string m_name;
	std::vector<uint8_t> m_resourceData;
	size_t m_offsetInResData;
	uint8_t m_attributes;
};

struct ResCatalogData
{
};

bool IsWhitespace(char c)
{
	return c <= ' ';
}

void AppendUInt16(std::vector<uint8_t> &vec, uint16_t value)
{
	uint8_t encoded[2];
	BytePack::BigUInt16(encoded, value);
	vec.push_back(encoded[0]);
	vec.push_back(encoded[1]);
}

void AppendInt16(std::vector<uint8_t> &vec, int16_t value)
{
	AppendUInt16(vec, static_cast<uint16_t>(value));
}

void AppendUInt32(std::vector<uint8_t> &vec, uint32_t value)
{
	uint8_t encoded[4];
	BytePack::BigUInt32(encoded, value);
	vec.push_back(encoded[0]);
	vec.push_back(encoded[1]);
	vec.push_back(encoded[2]);
	vec.push_back(encoded[3]);
}

uint32_t TypeToU32(const char *resType)
{
	uint32_t v = 0;
	for (int i = 0; i < 4; i++)
		v |= static_cast<uint8_t>(resType[i]) << (24 - i * 8);

	return v;
}

bool FindNextNonWhitespace(CFileStream &fs, char &nextChar)
{
	for (;;)
	{
		char c;
		if (!fs.Read(&c, 1))
			return false;

		if (!IsWhitespace(c))
		{
			nextChar = c;
			return true;
		}
	}
}

std::string HandleEscapes(const std::string &str)
{
	const size_t len = str.size();

	std::string processed;

	for (size_t i = 0; i < len; i++)
	{
		const char c = str[i];

		if (c == '\\' && len - i >= 5 && str[i + 1] == '0' && str[i + 2] == 'x')
		{
			const char highNibbleChar = str[i + 3];
			const char lowNibbleChar = str[i + 4];

			int highNibble = highNibbleChar - '0';
			int lowNibble = lowNibbleChar - '0';

			const char reconstructed = (highNibble << 4) | lowNibble;
			processed.append(&reconstructed, 1);

			i += 4;
		}
		else
			processed.append(&c, 1);
	}

	return processed;
}

void DefError()
{
	fprintf(stderr, "Malformed def");
	exit(-1);
}

int toolMain(int argc, const char **argv)
{
	if (argc != 3)
	{
		fprintf(stderr, "Usage: MiniRez <input.r> <output.bin>");
		return -1;
	}

	FILE *f = fopen_utf8(argv[1], "rb");
	if (!f)
	{
		fprintf(stderr, "Failed to open input file");
		return -1;
	}

	CFileStream fs(f, true, false, true);

	int defsParsed = 0;

	std::vector<SimpleResource*> resources;

	for (;;)
	{
		printf("%i defs parsed...\n", defsParsed);
		defsParsed++;
		GpUFilePos_t defStartOffset = fs.Tell();

		char nextChar = 0;
		if (!FindNextNonWhitespace(fs, nextChar))
			break;

		if (IsWhitespace(nextChar))
			continue;

		if (nextChar == 'd')
		{
			char nextChars[3];
			if (fs.Read(nextChars, 3) != 3 || nextChars[0] != 'a' || nextChars[1] != 't' || nextChars[2] != 'a')
			{
				fprintf(stderr, "Unknown token");
				return -1;
			}

			if (!FindNextNonWhitespace(fs, nextChar) || nextChar != '\'')
				DefError();

			char resType[5];
			if (!fs.Read(resType, 5) || resType[4] != '\'')
				DefError();

			if (!FindNextNonWhitespace(fs, nextChar) || nextChar != '(')
				DefError();

			int resID = 0;
			bool hasProperty = false;
			bool isNegative = false;
			for (;;)
			{
				if (!fs.Read(&nextChar, 1))
					DefError();

				if (nextChar == ')')
					break;
				else if (nextChar == ',')
				{
					hasProperty = true;
					break;
				}
				else if (nextChar == '-')
					isNegative = true;
				else if (nextChar >= '0' && nextChar <= '9')
					resID = resID * 10 + (nextChar - '0');
				else
					DefError();
			}

			std::string resName;
			std::string propName;
			std::vector<uint8_t> resData;

			if (hasProperty)
			{
				char singleCharStr[2];
				singleCharStr[1] = '\0';

				if (!FindNextNonWhitespace(fs, singleCharStr[0]))
					DefError();


				if (singleCharStr[0] == '\"')
				{
					hasProperty = false;

					for (;;)
					{
						if (!fs.Read(singleCharStr, 1))
							DefError();

						if (singleCharStr[0] == '"')
							break;

						resName += singleCharStr;
					}

					if (!FindNextNonWhitespace(fs, nextChar))
						DefError();

					if (nextChar == ')')
					{
					}
					else if (nextChar == ',')
					{
						hasProperty = true;
						if (!FindNextNonWhitespace(fs, singleCharStr[0]))
							DefError();
					}
					else
						DefError();
				}

				if (hasProperty)
				{
					propName = singleCharStr;

					for (;;)
					{
						if (!fs.Read(singleCharStr, 1))
							DefError();

						if (singleCharStr[0] == ')')
							break;

						propName += singleCharStr;
					}
				}
			}

			if (!FindNextNonWhitespace(fs, nextChar) || nextChar != '{')
				DefError();

			// Data item parse loop
			for (;;)
			{
				if (!FindNextNonWhitespace(fs, nextChar))
					DefError();

				if (nextChar == '$')
				{
					// Res data bytes
					uint8_t b = 0;
					bool highNibble = true;

					if (!fs.Read(&nextChar, 1) || nextChar != '\"')
						DefError();

					for (;;)
					{
						if (!fs.Read(&nextChar, 1))
							DefError();

						uint8_t fragment = 0;

						if (nextChar == '\"')
							break;
						else if (nextChar >= '0' && nextChar <= '9')
							fragment = nextChar - '0';
						else if (nextChar >= 'a' && nextChar <= 'f')
							fragment = nextChar - 'a' + 0xa;
						else if (nextChar >= 'A' && nextChar <= 'F')
							fragment = nextChar - 'A' + 0xa;
						else if (nextChar == ' ')
						{
							continue;
						}
						else
							DefError();

						if (highNibble)
						{
							b = fragment << 4;
							highNibble = false;
						}
						else
						{
							resData.push_back(b | fragment);
							highNibble = true;
						}
					}

					continue;
				}
				else if (nextChar == '}')
					break;
				else if (nextChar == '/')
				{
					if (!fs.Read(&nextChar, 1))
						DefError();

					if (nextChar == '/')
					{
						for (;;)
						{
							if (!fs.Read(&nextChar, 1))
								DefError();

							if (nextChar == '\r' || nextChar == '\n')
								break;
						}
					}
					else if (nextChar == '*')
					{
						char prevChar = nextChar;

						for (;;)
						{
							if (!fs.Read(&nextChar, 1))
								DefError();

							if (nextChar == '/' && prevChar == '*')
								break;

							prevChar = nextChar;
						}
					}
					else
						DefError();
				}
				else
					DefError();
			}

			if (!FindNextNonWhitespace(fs, nextChar) || nextChar != ';')
				DefError();

			SimpleResource *res = new SimpleResource();
			res->m_name = HandleEscapes(resName);
			res->m_resourceID = resID;
			memcpy(res->m_type, resType, 4);
			res->m_resourceData.swap(resData);
			res->m_attributes = 0;

			if (propName == "purgeable")
				res->m_attributes |= (1 << 5);


			resources.push_back(res);
		}
		else
		{
			fprintf(stderr, "Unknown token");
			return -1;
		}
	}

	std::vector<uint8_t> resourceForkData;

	for (size_t i = 0; i < 16; i++)
		resourceForkData.push_back(0);

	size_t resForkDataStart = resourceForkData.size();

	std::vector<uint32_t> uniqueResTypes;
	std::vector<unsigned int> resTypeCounts;
	std::vector<std::string> uniqueResNames;

	printf("Re-emitting defs...\n");
	for (size_t i = 0; i < resources.size(); i++)
	{
		SimpleResource *res = resources[i];

		res->m_offsetInResData = resourceForkData.size() - resForkDataStart;

		uint8_t packedLen[4];
		BytePack::BigUInt32(packedLen, static_cast<uint32_t>(res->m_resourceData.size()));

		for (int j = 0; j < 4; j++)
			resourceForkData.push_back(packedLen[j]);

		for (size_t j = 0; j < res->m_resourceData.size(); j++)
			resourceForkData.push_back(res->m_resourceData[j]);

		uint32_t resTypeCode = TypeToU32(res->m_type);
		std::vector<uint32_t>::iterator uniqueResTypeIndex = std::find(uniqueResTypes.begin(), uniqueResTypes.end(), resTypeCode);

		if (uniqueResTypeIndex == uniqueResTypes.end())
		{
			uniqueResTypes.push_back(resTypeCode);
			resTypeCounts.push_back(1);
		}
		else
			resTypeCounts[uniqueResTypeIndex - uniqueResTypes.begin()]++;

		if (!res->m_name.empty())
		{
			if (std::find(uniqueResNames.begin(), uniqueResNames.end(), res->m_name) == uniqueResNames.end())
				uniqueResNames.push_back(res->m_name);
		}
	}

	printf("Re-emit OK\n");

	const size_t resDataSize = resourceForkData.size() - resForkDataStart;
	const size_t resMapPos = resourceForkData.size();

	// Reserved space for resource header copy (16), handle to next res map (4), file ref number (2)
	for (size_t j = 0; j < 22; j++)
		resourceForkData.push_back(0);

	uint16_t resForkAttributes = 0;	// We don't use any of these

	const size_t typeListEntrySize = 8;
	const size_t refListEntrySize = 12;

	const size_t resourceTypeListStartLoc = 28;
	const size_t resourceTypeListSize = 2 + uniqueResTypes.size() * typeListEntrySize;
	const size_t resourceRefListStartLoc = resourceTypeListStartLoc + resourceTypeListSize;
	const size_t resourceNameListStartLoc = resourceRefListStartLoc + resources.size() * refListEntrySize;

	AppendUInt16(resourceForkData, resForkAttributes);
	AppendUInt16(resourceForkData, static_cast<uint16_t>(resourceTypeListStartLoc));
	AppendUInt16(resourceForkData, static_cast<uint16_t>(resourceNameListStartLoc));
	AppendUInt16(resourceForkData, static_cast<uint16_t>(uniqueResTypes.size() - 1));

	std::vector<size_t> refListStartForType;
	refListStartForType.resize(resTypeCounts.size());

	if (resTypeCounts.size() > 0)
	{
		refListStartForType[0] = 0;
		for (size_t i = 1; i < refListStartForType.size(); i++)
			refListStartForType[i] = refListStartForType[i - 1] + resTypeCounts[i - 1];
	}

	std::vector<size_t> nameListStarts;
	nameListStarts.resize(uniqueResNames.size());

	if (nameListStarts.size() > 0)
	{
		nameListStarts[0] = 0;
		for (size_t i = 1; i < nameListStarts.size(); i++)
			nameListStarts[i] = nameListStarts[i - 1] + uniqueResNames[i - 1].size() + 1;
	}

	// Write resource type list
	for (size_t i = 0; i < uniqueResTypes.size(); i++)
	{
		AppendUInt32(resourceForkData, uniqueResTypes[i]);
		AppendUInt16(resourceForkData, resTypeCounts[i] - 1);
		AppendUInt16(resourceForkData, static_cast<uint16_t>(refListStartForType[i] * refListEntrySize + resourceTypeListSize));
	}

	// Write reference lists
	for (size_t ti = 0; ti < uniqueResTypes.size(); ti++)
	{
		uint32_t resType = uniqueResTypes[ti];

		for (size_t i = 0; i < resources.size(); i++)
		{
			const SimpleResource *res = resources[i];
			if (TypeToU32(res->m_type) != resType)
				continue;

			AppendInt16(resourceForkData, res->m_resourceID);
			if (res->m_name.empty())
				AppendInt16(resourceForkData, -1);
			else
			{
				const std::vector<std::string>::iterator nameIt = std::find(uniqueResNames.begin(), uniqueResNames.end(), res->m_name);
				const size_t nameIndex = nameIt - uniqueResNames.begin();

				AppendUInt16(resourceForkData, static_cast<uint16_t>(nameListStarts[nameIndex]));
			}

			resourceForkData.push_back(res->m_attributes);

			const size_t resDataStart = res->m_offsetInResData;

			resourceForkData.push_back((resDataStart >> 16) & 0xff);
			resourceForkData.push_back((resDataStart >> 8) & 0xff);
			resourceForkData.push_back((resDataStart >> 0) & 0xff);

			for (int j = 0; j < 4; j++)
				resourceForkData.push_back(0);	// Reserved
		}
	}

	// Write name lists
	for (size_t ni = 0; ni < uniqueResNames.size(); ni++)
	{
		const std::string &name = uniqueResNames[ni];
		const size_t nameLen = name.size();

		resourceForkData.push_back(static_cast<uint8_t>(nameLen));

		for (size_t i = 0; i < nameLen; i++)
			resourceForkData.push_back(name[i]);
	}

	const size_t resMapSize = resourceForkData.size() - resMapPos;

	BytePack::BigUInt32(&resourceForkData[0], static_cast<uint32_t>(resForkDataStart));
	BytePack::BigUInt32(&resourceForkData[4], static_cast<uint32_t>(resMapPos));
	BytePack::BigUInt32(&resourceForkData[8], static_cast<uint32_t>(resDataSize));
	BytePack::BigUInt32(&resourceForkData[12], static_cast<uint32_t>(resMapSize));

	printf("Writing to %s...", argv[2]);

	FILE *outF = fopen_utf8(argv[2], "wb");
	if (!outF)
	{
		fprintf(stderr, "Failed to open output file");
		return -1;
	}

	fwrite(&resourceForkData[0], 1, resourceForkData.size(), outF);
	fclose(outF);

	return 0;
}
