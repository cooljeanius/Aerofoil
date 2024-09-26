#pragma once

#include "RGBAColor.h"

union ResolvedColor
{
	uint8_t m_indexed;
	PortabilityLayer::RGBAColor m_rgba;
};
