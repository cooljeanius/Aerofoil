#pragma once

#include <stdint.h>

namespace PortabilityLayer
{
	struct RGBAColor;

	struct QDPictEmitScanlineParameters
	{
		const RGBAColor *m_colors;
		size_t m_numColors;
		size_t m_planarSeparation;

		int32_t m_scanlineOriginX;
		int32_t m_firstY;
		int32_t m_constrainedRegionLeft;
		int32_t m_constrainedRegionRight;
	};
}
