#pragma once

#include "PlotDirection.h"

namespace PortabilityLayer
{
	struct Vec2i;

	struct IPlotter
	{
		virtual PlotDirection PlotNext() = 0;
		virtual const Vec2i &GetPoint() const = 0;
	};
}
