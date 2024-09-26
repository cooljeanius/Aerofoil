#pragma once

#ifndef __PL_MACRSRCHEADER_H__
#define __PL_MACRSRCHEADER_H__

#include "DataTypes.h"

namespace PortabilityLayer
{
	struct MacRsrcHeader
	{
		uint32_t m_resDataOffset;
		uint32_t m_resMapOffset;
		uint32_t m_resDataSize;
		uint32_t m_resMapSize;

		void Load(const void *data);
	};
}

#endif
