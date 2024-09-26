#include "ResourceCompiledRef.h"
#include "ByteSwap.h"

#include <string.h>

namespace PortabilityLayer
{
	uint32_t ResourceCompiledRef::GetSize() const
	{
		uint32_t resSize;
		memcpy(&resSize, m_resData - 4, 4);

		ByteSwap::BigUInt32(resSize);

		return resSize;
	}
}