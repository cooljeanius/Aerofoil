#pragma once

namespace PortabilityLayer
{
	enum PlotDirection
	{
		PlotDirection_NegX_NegY,
		PlotDirection_0X_NegY,
		PlotDirection_PosX_NegY,

		PlotDirection_NegX_PosY,
		PlotDirection_0X_PosY,
		PlotDirection_PosX_PosY,

		PlotDirection_NegX_0Y,
		PlotDirection_PosX_0Y,

		PlotDirection_Exhausted,
	};
}
