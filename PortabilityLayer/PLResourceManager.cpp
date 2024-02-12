#include "ResourceManager.h"

#include "BinarySearch.h"
#include "BMPFormat.h"
#include "FileManager.h"
#include "GPArchive.h"
#include "IGpDirectoryCursor.h"
#include "IGpFileSystem.h"
#include "GpIOStream.h"
#include "MacBinary2.h"
#include "MacFileMem.h"
#include "MemReaderStream.h"
#include "MemoryManager.h"
#include "MMHandleBlock.h"
#include "ResourceCompiledTypeList.h"
#include "ResourceFile.h"
#include "VirtualDirectory.h"
#include "WaveFormat.h"
#include "ZipFileProxy.h"
#include "ZipFile.h"

#include "PLDrivers.h"
#include "PLPasStr.h"
#include "PLErrorCodes.h"

#include <stdio.h>
#include <algorithm>

namespace ResourceValidationRules
{
	enum ResourceValidationRule
	{
		kNone,
		kWAV,
		kBMP,
	};
}

static const char *kPICTExtension = ".bmp";

typedef ResourceValidationRules::ResourceValidationRule ResourceValidationRule_t;

namespace PortabilityLayer
{
	class ResourceArchiveZipFileIterator : public IResourceIterator
	{
	public:
		explicit ResourceArchiveZipFileIterator(PortabilityLayer::ZipFileProxy *proxy);

		void Destroy() override;
		bool GetOne(ResTypeID &resTypeID, int16_t &outID) override;

	private:
		~ResourceArchiveZipFileIterator();

		PortabilityLayer::ZipFileProxy *m_proxy;
		size_t m_numFiles;
		size_t m_currentIndex;
	};
}


PortabilityLayer::ResourceArchiveZipFileIterator::ResourceArchiveZipFileIterator(PortabilityLayer::ZipFileProxy *proxy)
	: m_proxy(proxy)
	, m_numFiles(proxy->NumFiles())
	, m_currentIndex(0)
{
}

void PortabilityLayer::ResourceArchiveZipFileIterator::Destroy()
{
	this->~ResourceArchiveZipFileIterator();
	PortabilityLayer::MemoryManager::GetInstance()->Release(this);
}

bool PortabilityLayer::ResourceArchiveZipFileIterator::GetOne(ResTypeID &resTypeID, int16_t &outID)
{
	while (m_currentIndex != m_numFiles)
	{
		const size_t index = m_currentIndex++;

		const char *name = nullptr;
		size_t nameLength = 0;
		m_proxy->GetFileName(index, name, nameLength);

		const bool isParseable = ResourceArchiveZipFile::ParseResFromName(name, nameLength, resTypeID, outID);
		if (isParseable)
			return true;
	}

	return false;
}

PortabilityLayer::ResourceArchiveZipFileIterator::~ResourceArchiveZipFileIterator()
{
}

namespace
{
	// Validation here is only intended to be minimal, to ensure later checks can determine the format size and do certain operations
	// that must be valid.
	static bool ValidateResource(const void *res, size_t size, ResourceValidationRule_t validationRule)
	{
		switch (validationRule)
		{
		case ResourceValidationRules::kWAV:
			{
				if (size < sizeof(PortabilityLayer::RIFFTag))
					return false;

				PortabilityLayer::RIFFTag mainTag;
				memcpy(&mainTag, res, sizeof(mainTag));
				if (mainTag.m_chunkSize > size - sizeof(PortabilityLayer::RIFFTag))
					return false;

				return true;
			}
			break;

		case ResourceValidationRules::kBMP:
			{
				if (size < sizeof(PortabilityLayer::BitmapFileHeader) + sizeof(PortabilityLayer::BitmapInfoHeader))
					return false;

				PortabilityLayer::BitmapFileHeader mainHeader;
				memcpy(&mainHeader, res, sizeof(mainHeader));
				if (mainHeader.m_fileSize > size)
					return false;

				PortabilityLayer::BitmapInfoHeader infoHeader;
				memcpy(&infoHeader, static_cast<const uint8_t*>(res) + sizeof(mainHeader), sizeof(infoHeader));
				if (infoHeader.m_thisStructureSize < sizeof(PortabilityLayer::BitmapInfoHeader))
					return false;

				const size_t sizeForInfoHeader = size - sizeof(PortabilityLayer::BitmapFileHeader);
				if (infoHeader.m_thisStructureSize > sizeForInfoHeader)
					return false;

				if (infoHeader.m_width >= 0x1000 || infoHeader.m_height >= 0x1000 || infoHeader.m_width < 1 || infoHeader.m_height < 1)
					return false;

				return true;
			}
			break;

		default:
			break;
		};

		return false;
	}
}


namespace PortabilityLayer
{
	class CompositeFile;
	struct MMHandleBlock;
	struct IResourceArchive;

	class ResourceManagerImpl final : public ResourceManager
	{
	public:
		ResourceManagerImpl();

		void Init() override;
		void Shutdown() override;

		THandle<void> GetAppResource(const ResTypeID &resTypeID, int16_t resID) const override;
		IResourceArchive *GetAppResourceArchive() const override;

		IResourceArchive *LoadResFile(CompositeFile *file) const override;
		PLError_t CreateBlankResFile(VirtualDirectory_t virtualDir, const PLPasStr &filename) GP_ASYNCIFY_PARANOID_OVERRIDE;

		void DissociateHandle(MMHandleBlock *hdl) const override;
		const ResourceArchiveRef *ResourceForHandle(MMHandleBlock *hdl) const override;

		static ResourceManagerImpl *GetInstance();

	private:
		void UnloadAndDestroyResourceFile(IResourceArchive *rf);

		IResourceArchive *m_appResArchive;
		CompositeFile *m_appResFile;

		static ResourceManagerImpl ms_instance;
	};

	ResourceManagerImpl::ResourceManagerImpl()
		: m_appResArchive(nullptr)
		, m_appResFile(nullptr)
	{
	}

	void ResourceManagerImpl::Init()
	{
		m_appResFile = PortabilityLayer::FileManager::GetInstance()->OpenCompositeFile(VirtualDirectories::kApplicationData, PSTR("ApplicationResources"));
		if (m_appResFile)
			m_appResArchive = LoadResFile(m_appResFile);
	}

	void ResourceManagerImpl::Shutdown()
	{
		if (m_appResArchive)
			m_appResArchive->Destroy();

		if (m_appResFile)
			m_appResFile->Close();

		m_appResArchive = nullptr;
	}

	void ResourceManagerImpl::DissociateHandle(MMHandleBlock *hdl) const
	{
		assert(hdl->m_rmSelfRef);
		assert(hdl->m_rmSelfRef->m_handle == hdl);
		hdl->m_rmSelfRef->m_handle = nullptr;
		hdl->m_rmSelfRef = nullptr;
	}

	const ResourceArchiveRef *ResourceManagerImpl::ResourceForHandle(MMHandleBlock *hdl) const
	{
		return hdl->m_rmSelfRef;
	}

	void ResourceManagerImpl::UnloadAndDestroyResourceFile(IResourceArchive *rf)
	{
		rf->Destroy();
	}

	ResourceManagerImpl *ResourceManagerImpl::GetInstance()
	{
		return &ms_instance;
	}

	IResourceArchive *ResourceManagerImpl::LoadResFile(CompositeFile *file) const
	{
		GpIOStream *fStream = nullptr;
		ZipFileProxy *proxy = nullptr;
		bool proxyIsShared = false;

		if (file->OpenResources(fStream, proxy, proxyIsShared) != PLErrors::kNone)
			return nullptr;

		IResourceArchive *archive = ResourceArchiveZipFile::Create(proxy, proxyIsShared, fStream);
		if (!archive)
		{
			proxy->Destroy();
			if (fStream)
				fStream->Close();
			return nullptr;
		}

		return archive;
	}

	PLError_t ResourceManagerImpl::CreateBlankResFile(VirtualDirectory_t virtualDir, const PLPasStr &filename)
	{
		GpIOStream *stream = nullptr;
		PLError_t error = FileManager::GetInstance()->RawOpenFileResources(virtualDir, filename, EFilePermission_Write, true, GpFileCreationDispositions::kCreateOrOverwrite, stream);
		if (error)
			return error;

		PortabilityLayer::ZipEndOfCentralDirectoryRecord eocd;
		eocd.m_signature = PortabilityLayer::ZipEndOfCentralDirectoryRecord::kSignature;
		eocd.m_thisDiskNumber = 0;
		eocd.m_centralDirDisk = 0;
		eocd.m_numCentralDirRecordsThisDisk = 0;
		eocd.m_numCentralDirRecords = 0;
		eocd.m_centralDirectorySizeBytes = 0;
		eocd.m_centralDirStartOffset = 0;
		eocd.m_commentLength = 0;

		if (stream->Write(&eocd, sizeof(eocd)) != sizeof(eocd))
		{
			stream->Close();
			return PLErrors::kIOError;
		}

		stream->Close();

		return PLErrors::kNone;
	}

	THandle<void> ResourceManagerImpl::GetAppResource(const ResTypeID &resType, int16_t resID) const
	{
		if (!m_appResArchive)
			return THandle<void>();

		return m_appResArchive->LoadResource(resType, resID);
	}

	IResourceArchive *ResourceManagerImpl::GetAppResourceArchive() const
	{
		return m_appResArchive;
	}

	ResourceManagerImpl ResourceManagerImpl::ms_instance;

	ResourceManager *ResourceManager::GetInstance() { return ResourceManagerImpl::GetInstance(); }

	// ===========================================================================================

	const char *ResourceArchiveBase::GetFileExtensionForResType(const ResTypeID &resTypeID, int &outValidationRule)
	{
		const char *extension = ".bin";
		outValidationRule = ResourceValidationRules::kNone;

		if (resTypeID == ResTypeID('snd '))
		{
			extension = ".wav";
			outValidationRule = ResourceValidationRules::kWAV;
		}
		else if (resTypeID == ResTypeID('Date') || resTypeID == ResTypeID('PICT') || resTypeID == ResTypeID('ICN#') || resTypeID == ResTypeID('icl8') || resTypeID == ResTypeID('icl4') || resTypeID == ResTypeID('ics#') || resTypeID == ResTypeID('ics8') || resTypeID == ResTypeID('ics4'))
		{
			extension = kPICTExtension;
			outValidationRule = ResourceValidationRules::kBMP;
		}
		else if (resTypeID == ResTypeID('STR#') || resTypeID == ResTypeID('LICS'))
			extension = ".txt";
		else if (resTypeID == ResTypeID('DITL') || resTypeID == ResTypeID('muvi'))
			extension = ".json";

		return extension;
	}

	// ===========================================================================================

	ResourceArchiveRef::ResourceArchiveRef()
		: m_handle(nullptr)
		, m_size(0)
		, m_resID(0)
	{
	}

	// ===========================================================================================

	ResourceArchiveZipFile *ResourceArchiveZipFile::Create(ZipFileProxy *zipFileProxy, bool proxyIsShared, GpIOStream *stream)
	{
		PortabilityLayer::MemoryManager *mm = PortabilityLayer::MemoryManager::GetInstance();

		size_t numFiles = zipFileProxy->NumFiles();

		ResourceArchiveRef *refs = nullptr;
		if (numFiles > 0)
		{
			refs = static_cast<ResourceArchiveRef*>(mm->Alloc(sizeof(ResourceArchiveRef) * numFiles));
			if (!refs)
				return nullptr;

			for (size_t i = 0; i < numFiles; i++)
				new (refs + i) ResourceArchiveRef();
		}

		void *storage = mm->Alloc(sizeof(ResourceArchiveZipFile));
		if (!storage)
		{
			mm->Release(refs);
			return nullptr;
		}

		return new (storage) ResourceArchiveZipFile(zipFileProxy, proxyIsShared, stream, refs);
	}

	void ResourceArchiveZipFile::Destroy()
	{
		this->~ResourceArchiveZipFile();
		PortabilityLayer::MemoryManager::GetInstance()->Release(this);
	}

	THandle<void> ResourceArchiveZipFile::LoadResource(const ResTypeID &resTypeID, int id)
	{
		return GetResource(resTypeID, id, true);
	}

	bool ResourceArchiveZipFile::HasAnyResourcesOfType(const ResTypeID &resTypeID) const
	{
		char resPrefix[6];
		resTypeID.ExportAsChars(resPrefix);
		resPrefix[4] = '/';
		resPrefix[5] = '\0';

		return m_zipFileProxy->HasPrefix(resPrefix);
	}

	bool ResourceArchiveZipFile::FindFirstResourceOfType(const ResTypeID &resTypeID, int16_t &outID) const
	{
		char resPrefix[6];
		resTypeID.ExportAsChars(resPrefix);
		resPrefix[4] = '/';
		resPrefix[5] = '\0';

		size_t firstFileIndex = 0;
		if (!m_zipFileProxy->FindFirstWithPrefix(resPrefix, firstFileIndex))
			return false;

		const size_t numFiles = m_zipFileProxy->NumFiles();

		bool haveAny = false;
		int16_t lowestID = 32767;

		for (size_t fileIndex = firstFileIndex; fileIndex < numFiles; fileIndex++)
		{
			const char *resName = nullptr;
			size_t fnLength = 0;
			m_zipFileProxy->GetFileName(fileIndex, resName, fnLength);

			if (fnLength <= 5 || memcmp(resName, resPrefix, 5))
				break;

			const char *idChars = resName + 5;
			size_t idCharsRemaining = fnLength - 5;

			const size_t extLength = strlen(kPICTExtension);

			if (idCharsRemaining <= extLength)
				continue;

			if (memcmp(idChars + idCharsRemaining - extLength, kPICTExtension, extLength))
				continue;

			idCharsRemaining -= extLength;

			bool isNegative = false;

			if (idChars[0] == '-')
			{
				isNegative = true;
				idCharsRemaining--;
				idChars++;
			}

			if (idCharsRemaining == 0)
				continue;

			if (idChars[0] == '0' && (idCharsRemaining > 1 || isNegative))
				continue;

			int32_t resID = 0;
			bool failedName = false;
			while (idCharsRemaining)
			{
				const char idChar = *idChars;

				if (idChar < '0' || idChar > '9')
				{
					failedName = true;
					break;
				}

				resID = resID * 10;
				if (isNegative)
				{
					resID -= (idChar - '0');
					if (resID < -32768)
					{
						failedName = true;
						break;
					}
				}
				else
				{
					resID += (idChar - '0');
					if (resID > 32767)
					{
						failedName = true;
						break;
					}
				}

				idChars++;
				idCharsRemaining--;
			}

			if (failedName)
				continue;

			if (haveAny == false || resID < lowestID)
			{
				lowestID = resID;
				haveAny = true;
			}
		}

		if (haveAny)
			outID = static_cast<int16_t>(lowestID);

		return haveAny;
	}

	IResourceIterator *ResourceArchiveZipFile::EnumerateResources() const
	{
		PortabilityLayer::MemoryManager *mm = PortabilityLayer::MemoryManager::GetInstance();
		void *storage = mm->Alloc(sizeof(ResourceArchiveZipFileIterator));
		if (!storage)
			return nullptr;

		return new (storage) ResourceArchiveZipFileIterator(m_zipFileProxy);
	}

	bool ResourceArchiveZipFile::ParseResFromName(const char *name, size_t nameLength, ResTypeID &outResTypeID, int16_t &outID)
	{
		size_t slashPos = nameLength;
		for (size_t i = 0; i < nameLength; i++)
		{
			if (name[i] == '/')
			{
				slashPos = i;
				break;
			}
		}

		if (slashPos == nameLength)
			return false;

		GpArcResourceTypeTag resTag;
		if (!resTag.Load(name, slashPos))
			return false;

		if (!resTag.Decode(outResTypeID))
			return false;

		int validationRule = 0;
		const char *expectedExtension = GetFileExtensionForResType(outResTypeID, validationRule);

		const size_t idStart = slashPos + 1;
		if (idStart == nameLength)
			return false;

		bool isNegative = false;
		size_t digitsStart = idStart;

		if (name[idStart] == '-')
		{
			isNegative = true;
			digitsStart++;
		}

		size_t digitsEnd = nameLength;
		int32_t resID = 0;
		for (size_t i = digitsStart; i < nameLength; i++)
		{
			if (name[i] == '.')
			{
				digitsEnd = i;
				break;
			}
			else
			{
				char digit = name[i];
				if (digit < '0' || digit > '9')
					return false;

				int32_t digitNumber = static_cast<int32_t>(digit) - '0';
				resID *= 10;
				if (isNegative)
					resID -= digitNumber;
				else
					resID += digitNumber;

				if (resID < -32768 || resID > 32767)
					return false;
			}
		}

		outID = static_cast<int16_t>(resID);

		const size_t extAvailable = nameLength - digitsEnd;
		if (strlen(expectedExtension) != extAvailable)
			return false;

		if (memcmp(name + digitsEnd, expectedExtension, extAvailable) != 0)
			return false;

		return true;
	}

	bool ResourceArchiveZipFile::IndexResource(const ResTypeID &resTypeID, int id, size_t &outIndex, int &outValidationRule) const
	{
		const char *extension = GetFileExtensionForResType(resTypeID, outValidationRule);

		char resourceFile[64];

		GpArcResourceTypeTag resTag = GpArcResourceTypeTag::Encode(resTypeID);

		snprintf(resourceFile, sizeof(resourceFile) - 1, "%s/%i%s", resTag.m_id, id, extension);

		size_t index = 0;
		return m_zipFileProxy->IndexFile(resourceFile, outIndex);
	}

	THandle<void> ResourceArchiveZipFile::GetResource(const ResTypeID &resTypeID, int id, bool load)
	{
		int validationRule = 0;
		size_t index = 0;
		if (!IndexResource(resTypeID, id, index, validationRule))
			return THandle<void>();

		ResourceArchiveRef *ref = m_resourceHandles + index;

		MMHandleBlock *handle = nullptr;
		if (ref->m_handle != nullptr)
			handle = ref->m_handle;
		else
		{
			const size_t resSize = m_zipFileProxy->GetFileSize(index);

			if (resSize > kMaxResourceSize)
				return THandle<void>();

			handle = MemoryManager::GetInstance()->AllocHandle(0);
			if (!handle)
				return THandle<void>();

			handle->m_rmSelfRef = ref;
			ref->m_handle = handle;
			ref->m_resID = static_cast<int16_t>(id);
			ref->m_size = resSize;
		}

		if (handle->m_contents == nullptr && load)
		{
			if (ref->m_size > 0)
			{
				void *contents = MemoryManager::GetInstance()->Alloc(ref->m_size);
				handle->m_contents = contents;
				handle->m_size = ref->m_size;

				if (!m_zipFileProxy->LoadFile(index, contents) || (validationRule != ResourceValidationRules::kNone && !ValidateResource(contents, ref->m_size, static_cast<ResourceValidationRule_t>(validationRule))))
				{
					MemoryManager::GetInstance()->Release(contents);
					handle->m_contents = nullptr;
					handle->m_size = 0;

					return THandle<void>();
				}
			}
		}

		return THandle<void>(handle);
	}

	ResourceArchiveZipFile::ResourceArchiveZipFile(ZipFileProxy *zipFileProxy, bool proxyIsShared, GpIOStream *stream, ResourceArchiveRef *resourceHandles)
		: m_zipFileProxy(zipFileProxy)
		, m_proxyIsShared(proxyIsShared)
		, m_stream(stream)
		, m_resourceHandles(resourceHandles)
	{
	}

	ResourceArchiveZipFile::~ResourceArchiveZipFile()
	{
		MemoryManager *mm = MemoryManager::GetInstance();

		const size_t numFiles = m_zipFileProxy->NumFiles();
		for (size_t i = 0; i < numFiles; i++)
		{
			ResourceArchiveRef &ref = m_resourceHandles[numFiles - 1 - i];
			if (ref.m_handle)
				mm->ReleaseHandle(ref.m_handle);

			ref.~ResourceArchiveRef();
		}

		mm->Release(m_resourceHandles);

		if (!m_proxyIsShared)
			m_zipFileProxy->Destroy();

		if (m_stream)
			m_stream->Close();
	}

#if GP_ASYNCIFY_PARANOID
	PLError_t ResourceManager::CreateBlankResFile(VirtualDirectory_t virtualDir, const PLPasStr &filename)
	{
		return static_cast<ResourceManagerImpl*>(this)->CreateBlankResFile(virtualDir, filename);
	}
#endif
}
