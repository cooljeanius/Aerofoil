#pragma once

#ifndef __PL_MACRSRCMAP_H__
#define __PL_MACRSRCMAP_H__

#include "DataTypes.h"

namespace PortabilityLayer
{
	struct MacRsrcMap
	{
		uint16_t m_resTypeListOffset;
		uint16_t m_resNameListOffset;
		uint16_t m_numTypesMinusOne;

		void Load(const void *data);
	};
}

#endif
