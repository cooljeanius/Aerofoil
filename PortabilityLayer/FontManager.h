#pragma once

#include "FontHacks.h"

namespace PortabilityLayer
{
	class FontFamily;
	class HostFont;
	class RenderedFont;

	class FontManager
	{
	public:
		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual FontFamily *GetSystemFont(int fontSize, int variationFlags) const = 0;
		virtual FontFamily *GetApplicationFont(int fontSize, int variationFlags) const = 0;

		virtual RenderedFont *GetRenderedFont(HostFont *font, int size, FontHacks fontHacks) = 0;

		static FontManager *GetInstance();
	};
}