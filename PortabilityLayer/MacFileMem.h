#pragma once

#include "DataTypes.h"
#include "MacFileInfo.h"
#include "GpVector.h"

struct IGpAllocator;

namespace PortabilityLayer
{
	class MacFileMem
	{
	public:
		const MacFileInfo &FileInfo() const;
		const uint8_t *DataFork() const;
		const uint8_t *ResourceFork() const;
		const char *Comment() const;

		static MacFileMem *Create(IGpAllocator *alloc, const uint8_t *dataFork, const uint8_t *resourceFork, const char* comment, const MacFileInfo &fileInfo);
		void Destroy();

	private:
		MacFileMem(IGpAllocator *alloc, const MacFileInfo &fileInfo);
		~MacFileMem();

		bool Init(const uint8_t *dataFork, const uint8_t *resourceFork, const char* comment);

		GpVector<uint8_t> m_data;
		MacFileInfo m_info;
		IGpAllocator *m_alloc;
	};
}

namespace PortabilityLayer
{
	inline const MacFileInfo &MacFileMem::FileInfo() const
	{
		return m_info;
	}

	inline const uint8_t *MacFileMem::DataFork() const
	{
		return m_data.Buffer();
	}

	inline const uint8_t *MacFileMem::ResourceFork() const
	{
		return m_data.Buffer() + m_info.m_dataForkSize;
	}

	inline const char *MacFileMem::Comment() const
	{
		return reinterpret_cast<const char*>(m_data.Buffer() + m_info.m_dataForkSize + m_info.m_resourceForkSize);
	}
}
