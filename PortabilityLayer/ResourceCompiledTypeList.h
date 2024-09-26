#pragma once

#include "ResTypeID.h"
#include "ResourceCompiledRef.h"

#include <stdint.h>

namespace PortabilityLayer
{
	struct ResourceCompiledTypeList
	{
		ResTypeID m_resType;
		ResourceCompiledRef *m_firstRef;
		size_t m_numRefs;
	};
}