#pragma once

#include <stddef.h>
#include <stdint.h>

struct GpRenderedGlyphMetrics
{
	size_t m_glyphDataPitch;

	uint32_t m_glyphWidth;
	uint32_t m_glyphHeight;

	int16_t m_bearingX;
	int16_t m_bearingY;
	int16_t m_advanceX;
	int16_t m_bitmapOffsetX;
	int16_t m_bitmapOffsetY;
};
