#pragma once

#include <stdint.h>
#include "SharedTypes.h"

class GpIOStream;

namespace PortabilityLayer
{
	class QDPictHeader
	{
	public:
		QDPictHeader();
		bool Load(GpIOStream *stream);

		int GetVersion() const;
		const Rect &GetFrame() const;

	private:
		Rect m_frame;

		int m_pictVersion;
	};
}
