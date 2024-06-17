#include <algorithm>
#include <string>
#include <vector>

#include "FontFamily.h"
#include "FontManager.h"
#include "FontRenderer.h"
#include "IGpFont.h"
#include "IGpFontHandler.h"
#include "GpAllocator_C.h"
#include "GpAppInterface.h"
#include "GpDriverIndex.h"
#include "GpFontHandlerProperties.h"
#include "CFileStream.h"

#include "PLDrivers.h"
#include "RenderedFont.h"
#include "RenderedFontCatalog.h"

#include "WindowsUnicodeToolShim.h"

extern "C"
#ifdef _MSC_VER
__declspec(dllimport)
#endif
IGpFontHandler *GpDriver_CreateFontHandler_FreeType2(const GpFontHandlerProperties &properties);

class MemBufferStream final : public GpIOStream
{
public:
	size_t Read(void *bytesOut, size_t size) override;
	size_t Write(const void *bytes, size_t size) override;
	bool IsSeekable() const override;
	bool IsReadOnly() const override;
	bool IsWriteOnly() const override;
	bool SeekStart(GpUFilePos_t loc) override;
	bool SeekCurrent(GpFilePos_t loc) override;
	bool SeekEnd(GpUFilePos_t loc) override;
	GpUFilePos_t Size() const override;
	GpUFilePos_t Tell() const override;
	void GP_ASYNCIFY_PARANOID_NAMED(Close)() override;
	void Flush() override;

	const uint8_t *GetBytes() const;

private:
	size_t m_writeOffset;
	std::vector<uint8_t> m_buffer;
};

size_t MemBufferStream::Read(void *bytesOut, size_t size)
{
	return 0;
}

size_t MemBufferStream::Write(const void *bytes, size_t size)
{
	const uint8_t *bytesUInt8 = static_cast<const uint8_t*>(bytes);

	for (size_t i = 0; i < size; i++)
	{
		if (m_writeOffset == m_buffer.size())
			m_buffer.push_back(bytesUInt8[i]);
		else
			m_buffer[m_writeOffset] = bytesUInt8[i];

		m_writeOffset++;
	}

	return size;
}

bool MemBufferStream::IsSeekable() const
{
	return true;
}

bool MemBufferStream::IsReadOnly() const
{
	return false;
}

bool MemBufferStream::IsWriteOnly() const
{
	return true;
}

bool MemBufferStream::SeekStart(GpUFilePos_t loc)
{
	m_writeOffset = static_cast<size_t>(loc);
	return true;
}

bool MemBufferStream::SeekCurrent(GpFilePos_t loc)
{
	m_writeOffset = static_cast<size_t>(static_cast<GpFilePos_t>(m_writeOffset) + loc);
	return true;
}

bool MemBufferStream::SeekEnd(GpUFilePos_t loc)
{
	m_writeOffset = this->Size() - loc;
	return true;
}

GpUFilePos_t MemBufferStream::Size() const
{
	return m_buffer.size();
}

GpUFilePos_t MemBufferStream::Tell() const
{
	return m_writeOffset;
}

void MemBufferStream::Close()
{
}

void MemBufferStream::Flush()
{
}


const uint8_t *MemBufferStream::GetBytes() const
{
	return &m_buffer[0];
}

struct KnownFontSpec
{
public:
	KnownFontSpec();
	KnownFontSpec(const char *path, int size, bool aa, PortabilityLayer::FontHacks hacks);

	bool operator==(const KnownFontSpec &other) const;
	bool operator!=(const KnownFontSpec &other) const;

private:
	std::string m_path;
	int m_size;
	bool m_aa;
	PortabilityLayer::FontHacks m_hacks;
};


KnownFontSpec::KnownFontSpec()
	: m_size(0)
	, m_aa(false)
	, m_hacks(PortabilityLayer::FontHacks_None)
{
}

KnownFontSpec::KnownFontSpec(const char *path, int size, bool aa, PortabilityLayer::FontHacks hacks)
	: m_path(path)
	, m_size(size)
	, m_aa(aa)
	, m_hacks(hacks)
{
}

bool KnownFontSpec::operator==(const KnownFontSpec &other) const
{
	return !((*this) != other);
}

bool KnownFontSpec::operator!=(const KnownFontSpec &other) const
{
	return m_path != other.m_path ||
		m_size != other.m_size ||
		m_aa != other.m_aa ||
		m_hacks != other.m_hacks;
}

int toolMain(int argc, const char **argv)
{
	if (argc < 3)
	{
		fputs("Usage: GenerateFonts <resources dir> <output dir>\n", stderr);
		return -1;
	}
	std::string resourcesDir(argv[1]);
	resourcesDir += '/';
	std::string outputDir(argv[2]);
	outputDir += '/';

	IGpAllocator *alloc = GpAllocator_C::GetInstance();

	GpFontHandlerProperties fhProperties;
	fhProperties.m_type = EGpFontHandlerType_FreeType2;
	fhProperties.m_alloc = alloc;

	GpDriverCollection *drivers = PLDrivers::GetDriverCollection();
	drivers->SetDriver<GpDriverIDs::kAlloc>(alloc);

	IGpFontHandler *ft2Handler = GpDriver_CreateFontHandler_FreeType2(fhProperties);

	PortabilityLayer::FontManager *fontManager = PortabilityLayer::FontManager::GetInstance();
	PortabilityLayer::FontRenderer *fontRenderer = PortabilityLayer::FontRenderer::GetInstance();
	fontManager->Init();

	std::vector<std::string> paths;
	std::vector<PortabilityLayer::RenderedFontCatalogRFontEntry> catalog;
	std::vector<KnownFontSpec> fontSpecs;

	std::string tmpPath(outputDir + "FontCacheManifest.json");
	FILE *manifestF = fopen_utf8(tmpPath.c_str(), "wb");

	fprintf(manifestF, "{\n");
	fprintf(manifestF, "\t\"add\" :\n");
	fprintf(manifestF, "\t{\n");
	fprintf(manifestF, "\t\t\"RFCT/1000.bin\" : \"%sFontCacheCatalog.bin\"", outputDir.c_str());

	int numFontsEmitted = 0;
	for (int presetIndex = 0; presetIndex < PortabilityLayer::FontPresets::kCount; presetIndex++)
	{
		int size = 0;
		int flags = 0;
		bool aa = false;
		PortabilityLayer::FontFamilyID_t fontFamilyID;

		fontManager->GetFontPreset(static_cast<PortabilityLayer::FontPreset_t>(presetIndex), &fontFamilyID, &size, &flags, &aa);
		PortabilityLayer::FontFamily *fontFamily = fontManager->GetFont(fontFamilyID);

		int variation = fontFamily->GetVariationForFlags(flags);

		PortabilityLayer::FontHacks hacks;
		PortabilityLayer::VirtualDirectory_t vDir;
		const char *path = nullptr;
		int typeFaceIndex = 0;
		fontFamily->GetFontSpec(variation, hacks, vDir, path, typeFaceIndex);

		KnownFontSpec spec(path, size, aa, hacks);

		if (std::find(fontSpecs.begin(), fontSpecs.end(), spec) != fontSpecs.end())
			continue;

		fontSpecs.push_back(spec);

		tmpPath = resourcesDir;
		tmpPath += path;
		FILE *fontFile = fopen_utf8(tmpPath.c_str(), "rb");

		if (fontFile)
		{
			PortabilityLayer::CFileStream stream(fontFile);

			IGpFont *font = ft2Handler->LoadFont(&stream, typeFaceIndex);
			if (!ft2Handler->KeepStreamOpen())
				stream.Close();

			PortabilityLayer::RenderedFont *rfont = fontRenderer->RenderFont(font, size, aa, hacks);


			{
				char fontPath[1024];
				sprintf(fontPath, "%sCachedFont%i.bin", outputDir.c_str(), numFontsEmitted);

				FILE *cachedFontF = fopen_utf8(fontPath, "wb");
				PortabilityLayer::CFileStream cacheStream(cachedFontF);

				fontRenderer->SaveCache(rfont, &cacheStream);
				cacheStream.Close();

				fprintf(manifestF, ",\n\t\t\"RFNT/%i.bin\" : \"%s\"", 1000 + numFontsEmitted, fontPath);
			}

			rfont->Destroy();

			font->Destroy();

			PortabilityLayer::RenderedFontCatalogRFontEntry catEntry;
			catEntry.m_fontSize = static_cast<uint8_t>(size);
			catEntry.m_hacks = static_cast<uint8_t>(hacks);
			catEntry.m_isAA = aa ? 1 : 0;
			catEntry.m_pathIndex = 0;

			bool foundPath = false;
			for (size_t i = 0; i < paths.size(); i++)
			{
				if (paths[i] == path)
				{
					catEntry.m_pathIndex = static_cast<uint8_t>(i);
					foundPath = true;
					break;
				}
			}

			if (!foundPath)
			{
				catEntry.m_pathIndex = static_cast<uint8_t>(paths.size());
				paths.push_back(std::string(path));
			}

			catalog.push_back(catEntry);

			numFontsEmitted++;
		}
	}

	fprintf(manifestF, "\n\t},\n");
	fprintf(manifestF, "\t\"delete\" :\n");
	fprintf(manifestF, "\t[\n");
	fprintf(manifestF, "\t]\n");
	fprintf(manifestF, "}\n");

	fclose(manifestF);

	PortabilityLayer::RenderedFontCatalogHeader catHeader;

	tmpPath = outputDir;
	tmpPath += "FontCacheCatalog.bin";
	FILE *catF = fopen_utf8(tmpPath.c_str(), "wb");

	catHeader.m_version = PortabilityLayer::RenderedFontCatalogHeader::kVersion;
	catHeader.m_pathsOffset = static_cast<uint32_t>(sizeof(PortabilityLayer::RenderedFontCatalogHeader) + paths.size() * sizeof(PortabilityLayer::RenderedFontCatalogPathEntry) + numFontsEmitted * sizeof(PortabilityLayer::RenderedFontCatalogRFontEntry));
	catHeader.m_numPaths = static_cast<uint16_t>(paths.size());
	catHeader.m_numRFonts = static_cast<uint16_t>(numFontsEmitted);

	fwrite(&catHeader, sizeof(catHeader), 1, catF);

	PortabilityLayer::RenderedFontCatalogPathEntry pathEntry;
	pathEntry.m_pathOffset = 0;
	pathEntry.m_pathSize = 0;
	for (size_t i = 0; i < paths.size(); i++)
	{
		pathEntry.m_pathOffset = static_cast<uint16_t>(pathEntry.m_pathOffset) + pathEntry.m_pathSize;
		pathEntry.m_pathSize = static_cast<uint16_t>(paths[i].size());

		fwrite(&pathEntry, sizeof(pathEntry), 1, catF);
	}

	fwrite(&catalog[0], sizeof(PortabilityLayer::RenderedFontCatalogRFontEntry), catalog.size(), catF);

	for (size_t i = 0; i < paths.size(); i++)
	{
		const std::string &str = paths[i];
		fwrite(&str[0], str.size(), 1, catF);
	}

	fclose(catF);

	return 0;
}
