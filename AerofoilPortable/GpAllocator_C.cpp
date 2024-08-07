#include "GpAllocator_C.h"
#include "CoreDefs.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

struct GpAllocator_C_MMBlock
{
	uint8_t m_offsetFromAllocLocation;

	static size_t AlignedSize();
};

size_t GpAllocator_C_MMBlock::AlignedSize()
{
	const size_t paddedSize = sizeof(GpAllocator_C_MMBlock) + GP_SYSTEM_MEMORY_ALIGNMENT - 1;
	const size_t paddedSizeTruncated = paddedSize - (paddedSize % GP_SYSTEM_MEMORY_ALIGNMENT);

	return paddedSizeTruncated;
}

void *GpAllocator_C::Realloc(void *buf, size_t newSize)
{
	if (buf == nullptr)
	{
		if (newSize == 0)
			return nullptr;

		return this->Alloc(newSize);
	}

	if (newSize == 0)
	{
		this->Free(buf);
		return nullptr;
	}

	assert(buf != nullptr);

	const size_t mmBlockSize = GpAllocator_C_MMBlock::AlignedSize();
	uint8_t *oldBufBytes = static_cast<uint8_t*>(buf);
	const GpAllocator_C_MMBlock *oldBufMMBlock = reinterpret_cast<const GpAllocator_C_MMBlock*>(oldBufBytes - GpAllocator_C_MMBlock::AlignedSize());

	const size_t oldBufOffsetFromAlignLoc = oldBufMMBlock->m_offsetFromAllocLocation;
	uint8_t *oldBufBase = oldBufBytes - GpAllocator_C_MMBlock::AlignedSize() - oldBufOffsetFromAlignLoc;

	const size_t mmBlockSizeWithMaxPadding = GpAllocator_C_MMBlock::AlignedSize() + GP_SYSTEM_MEMORY_ALIGNMENT - 1;
	if (SIZE_MAX - newSize < mmBlockSizeWithMaxPadding)
		return nullptr;

	const size_t newBufferSize = newSize + mmBlockSizeWithMaxPadding;
	uint8_t *newBuffer = static_cast<uint8_t*>(realloc(oldBufBase, newSize + mmBlockSizeWithMaxPadding));
	if (!newBuffer)
		return nullptr;

	const intptr_t offsetFromAlignPoint = reinterpret_cast<intptr_t>(newBuffer) & static_cast<intptr_t>(GP_SYSTEM_MEMORY_ALIGNMENT - 1);
	intptr_t alignPadding = 0;
	if (offsetFromAlignPoint != 0)
		alignPadding = static_cast<intptr_t>(GP_SYSTEM_MEMORY_ALIGNMENT) - offsetFromAlignPoint;

	// Check if the alignment changed, if so relocate
	if (static_cast<size_t>(alignPadding) != oldBufOffsetFromAlignLoc)
		memmove(newBuffer + alignPadding, newBuffer + oldBufOffsetFromAlignLoc, GpAllocator_C_MMBlock::AlignedSize() + newSize);

	GpAllocator_C_MMBlock *newMMBlock = reinterpret_cast<GpAllocator_C_MMBlock*>(newBuffer + alignPadding);
	newMMBlock->m_offsetFromAllocLocation = static_cast<uint8_t>(alignPadding);

	return newBuffer + alignPadding + GpAllocator_C_MMBlock::AlignedSize();
}

void *GpAllocator_C::Alloc(size_t size)
{
	if (size == 0)
		return nullptr;

	const size_t mmBlockSizeWithMaxPadding = GpAllocator_C_MMBlock::AlignedSize() + GP_SYSTEM_MEMORY_ALIGNMENT - 1;
	if (SIZE_MAX - size < mmBlockSizeWithMaxPadding)
		return nullptr;

	uint8_t *buffer = static_cast<uint8_t*>(realloc(nullptr, size + mmBlockSizeWithMaxPadding));
	if (!buffer)
		return nullptr;

	const intptr_t offsetFromAlignPoint = reinterpret_cast<intptr_t>(buffer) & static_cast<intptr_t>(GP_SYSTEM_MEMORY_ALIGNMENT - 1);
	intptr_t alignPadding = 0;
	if (offsetFromAlignPoint != 0)
		alignPadding = static_cast<intptr_t>(GP_SYSTEM_MEMORY_ALIGNMENT) - offsetFromAlignPoint;

	GpAllocator_C_MMBlock *mmBlock = reinterpret_cast<GpAllocator_C_MMBlock*>(buffer + alignPadding);
	mmBlock->m_offsetFromAllocLocation = static_cast<uint8_t>(alignPadding);

	return buffer + alignPadding + GpAllocator_C_MMBlock::AlignedSize();
}

void GpAllocator_C::Free(void *buf)
{
	if (!buf)
		return;

	const size_t mmBlockSize = GpAllocator_C_MMBlock::AlignedSize();

	uint8_t *bytes = static_cast<uint8_t*>(buf);
	const GpAllocator_C_MMBlock *mmBlock = reinterpret_cast<const GpAllocator_C_MMBlock*>(bytes - GpAllocator_C_MMBlock::AlignedSize());

	void *freeLoc = bytes - GpAllocator_C_MMBlock::AlignedSize() - mmBlock->m_offsetFromAllocLocation;
	free(freeLoc);
}

GpAllocator_C *GpAllocator_C::GetInstance()
{
	return &ms_instance;
}

GpAllocator_C GpAllocator_C::ms_instance;
