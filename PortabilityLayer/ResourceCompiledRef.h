#pragma once

#include <stdint.h>

namespace PortabilityLayer
{
	struct MMHandleBlock;

	struct ResourceCompiledRef
	{
		const uint8_t *m_resData;
		int16_t m_resID;
		int16_t m_resNameOffset;
		uint8_t m_attributes;

		uint32_t GetSize() const;
	};
}
