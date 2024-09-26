#pragma once

#include "GpPixelFormat.h"
#include <stdint.h>

namespace PortabilityLayer
{
	class SimpleImage final
	{
	public:
		void Destroy();
		void *GetData();
		const void *GetData() const;

		static SimpleImage *Create(GpPixelFormat_t pixelFormat, size_t pitch, size_t dataSize);

	private:
		GpPixelFormat_t m_pixelFormat;
		size_t m_pitch;
		void *m_data;
	};
}
