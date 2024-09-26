#include "MacFileMem.h"

namespace PortabilityLayer
{
	MacFileMem::MacFileMem(IGpAllocator *alloc, const MacFileInfo &fileInfo)
		: m_alloc(alloc)
		, m_info(fileInfo)
		, m_data(alloc)
	{
	}

	bool MacFileMem::Init(const uint8_t *dataFork, const uint8_t *resourceFork, const char* comment)
	{
		const size_t combinedSize = m_info.m_dataForkSize + m_info.m_resourceForkSize + m_info.m_commentSize + 1;
		if (!m_data.Resize(combinedSize))
			return false;

		uint8_t *buffer = m_data.Buffer();
		memcpy(buffer, dataFork, m_info.m_dataForkSize);
		buffer += m_info.m_dataForkSize;

		memcpy(buffer, resourceFork, m_info.m_resourceForkSize);
		buffer += m_info.m_resourceForkSize;

		memcpy(buffer, comment, m_info.m_commentSize);
		buffer += m_info.m_commentSize;

		*buffer = 0;

		return true;
	}

	MacFileMem::~MacFileMem()
	{
	}

	MacFileMem *MacFileMem::Create(IGpAllocator *alloc, const uint8_t *dataFork, const uint8_t *resourceFork, const char* comment, const MacFileInfo &fileInfo)
	{
		void *storage = alloc->Alloc(sizeof(MacFileMem));
		if (!storage)
			return nullptr;

		MacFileMem *result = new (storage) MacFileMem(alloc, fileInfo);
		if (!result->Init(dataFork, resourceFork, comment))
		{
			result->Destroy();
			return nullptr;
		}

		return result;
	}

	void MacFileMem::Destroy()
	{
		IGpAllocator *alloc = m_alloc;
		this->~MacFileMem();
		alloc->Release(this);
	}
}
