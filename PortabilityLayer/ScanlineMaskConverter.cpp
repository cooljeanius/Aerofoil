#include "ScanlineMaskConverter.h"

#include "EllipsePlotter.h"
#include "Rect2i.h"
#include "ScanlineMask.h"
#include "Vec2i.h"
#include "LinePlotter.h"
#include "ScanlineMaskBuilder.h"
#include "IPlotter.h"
#include "PLCore.h"

#include <assert.h>
#include <algorithm>

#define PL_SCANLINE_MASKS_DEBUGGING 0

#if PL_SCANLINE_MASKS_DEBUGGING
#include "stb_image_write.h"
#endif

// The algorithm that we're trying to emulate consistently paints on covered pixels of a line as in-bounds,
// while also identifying which pixels are part of the poly interior.
// 
// We use a plotting algorithm to generate an outline with some rules:
// - If a plot moves vertically in the same direction as the previous vertical movement, then the inversion bit for the pixel is flipped.
// - A scanline is then run across the row, toggling state between inversion points, and filling any that contain a plot point.

namespace PortabilityLayer
{
	enum PlotterVertical
	{
		PlotterVertical_NegY,
		PlotterVertical_PosY,

		PlotterVertical_None,
	};

	static void FillPresenceFlag(uint8_t *bitfield, size_t element)
	{
#if PL_SCANLINE_MASKS_DEBUGGING
		bitfield[element * 4] = 255;
#else
		bitfield[element / 4] |= 1 << ((element & 3) * 2);
#endif
	}

	static bool ReadPresenceFlag(const uint8_t *bitfield, size_t element)
	{
#if PL_SCANLINE_MASKS_DEBUGGING
		return bitfield[element * 4] == 255;;
#else
		return (bitfield[element / 4] & (1 << ((element & 3) * 2))) != 0;
#endif
	}

	static void FlipBorderFlag(uint8_t *bitfield, size_t element)
	{
#if PL_SCANLINE_MASKS_DEBUGGING
		bitfield[element * 4 + 1] ^= 255;
#else
		bitfield[element / 4] ^= 2 << ((element & 3) * 2);
#endif
	}

	static bool ReadBorderFlag(const uint8_t *bitfield, size_t element)
	{
#if PL_SCANLINE_MASKS_DEBUGGING
		return bitfield[element * 4 + 1] == 255;
#else
		return (bitfield[element / 4] & (2 << ((element & 3) * 2))) != 0;
#endif
	}

	static PlotterVertical VerticalForPlotDir(PlotDirection plotDir)
	{
		switch (plotDir)
		{
		case PlotDirection_NegX_NegY:
		case PlotDirection_0X_NegY:
		case PlotDirection_PosX_NegY:
			return PlotterVertical_NegY;
		case PlotDirection_NegX_PosY:
		case PlotDirection_0X_PosY:
		case PlotDirection_PosX_PosY:
			return PlotterVertical_PosY;
		default:
			return PlotterVertical_None;
		}
	}

	static bool FlushScanline(const uint8_t *flagBits, size_t firstElement, size_t width, ScanlineMaskBuilder &maskBuilder)
	{
		size_t spanStart = 0;
		bool isBorderToggleActive = false;
		bool maskSpanState = false;

		for (size_t col = 0; col < width; col++)
		{
			const size_t element = firstElement + col;

			const bool presenceFlag = ReadPresenceFlag(flagBits, element);
			const bool borderFlag = ReadBorderFlag(flagBits, element);

			if (borderFlag)
				isBorderToggleActive = !isBorderToggleActive;

			const bool elementState = (isBorderToggleActive || presenceFlag);

			if (elementState != maskSpanState)
			{
				if (!maskBuilder.AppendSpan(col - spanStart))
					return false;

				spanStart = col;
				maskSpanState = elementState;
			}
		}

		if (!maskBuilder.AppendSpan(width - spanStart))
			return false;

		return true;
	}

	ScanlineMask *ComputePlot(uint32_t width, uint32_t height, const Vec2i &minPoint, IPlotter &plotter)
	{
		assert(width > 0 && height > 0);

		if (static_cast<uint64_t>(width) * static_cast<uint64_t>(height) > SIZE_MAX)
			return nullptr;

		const size_t numElements = static_cast<uint64_t>(width) * static_cast<uint64_t>(height);

#if PL_SCANLINE_MASKS_DEBUGGING
		const size_t storageSize = numElements * 4;
#else
		const size_t storageSize = (numElements * 2 + 7) / 8;
#endif
		void *storage = NewPtr(storageSize);

		if (!storage)
			return nullptr;

		uint8_t *flagBits = static_cast<uint8_t*>(storage);
		memset(flagBits, 0, storageSize);

#if PL_SCANLINE_MASKS_DEBUGGING
		{
			for (size_t i = 0; i < storageSize; i += 4)
				flagBits[i + 3] = 255;
		}
#endif

		const Vec2i initialPoint = plotter.GetPoint() - minPoint;

		bool haveFirstVertical = false;
		size_t firstVerticalElement = 0;	// First element that contains a vertical movement on the first row
		PlotterVertical firstVertical = PlotterVertical::PlotterVertical_None;
		PlotterVertical lastVertical = PlotterVertical::PlotterVertical_None;

		size_t currentElement = static_cast<uint32_t>(initialPoint.m_y) * width + static_cast<uint32_t>(initialPoint.m_x);
		FillPresenceFlag(flagBits, currentElement);

		for (;;)
		{
			const size_t prevElement = currentElement;

			const PlotDirection plotDir = plotter.PlotNext();
			if (plotDir == PlotDirection_Exhausted)
				break;

			switch (plotDir)
			{
			case PlotDirection_NegX_NegY:
				currentElement = currentElement - 1 - width;
				break;
			case PlotDirection_0X_NegY:
				currentElement = currentElement - width;
				break;
			case PlotDirection_PosX_NegY:
				currentElement = currentElement + 1 - width;
				break;

			case PlotDirection_NegX_PosY:
				currentElement = currentElement + width - 1;
				break;
			case PlotDirection_0X_PosY:
				currentElement = currentElement + width;
				break;
			case PlotDirection_PosX_PosY:
				currentElement = currentElement + width + 1;
				break;

			case PlotDirection_NegX_0Y:
				currentElement = currentElement - 1;
				break;
			case PlotDirection_PosX_0Y:
				currentElement = currentElement + 1;
				break;
			default:
				break;
			}

			FillPresenceFlag(flagBits, currentElement);

#if PL_SCANLINE_MASKS_DEBUGGING
			const Vec2i expectedPoint = plotter.GetPoint() - minPoint;
			const int64_t expectedElement = static_cast<uint32_t>(expectedPoint.m_y) * width + expectedPoint.m_x;
			assert(currentElement == expectedElement);
#endif
			assert(currentElement < numElements);

			const PlotterVertical verticalForPlotDir = VerticalForPlotDir(plotDir);
			if (verticalForPlotDir != PlotterVertical_None)
			{
				if (!haveFirstVertical)
				{
					haveFirstVertical = true;
					firstVerticalElement = prevElement;
					firstVertical = verticalForPlotDir;
				}

				if (verticalForPlotDir == lastVertical)
					FlipBorderFlag(flagBits, prevElement);

				lastVertical = verticalForPlotDir;
			}
		}

		if (lastVertical == firstVertical)
			FlipBorderFlag(flagBits, firstVerticalElement);

		assert(plotter.GetPoint() - minPoint == initialPoint);

		ScanlineMaskBuilder maskBuilder;

		bool failed = false;
		for (size_t rowFirstElement = 0; rowFirstElement < numElements; rowFirstElement += width)
		{
			if (!FlushScanline(flagBits, rowFirstElement, width, maskBuilder))
			{
				failed = true;
				break;
			}
		}

#if PL_SCANLINE_MASKS_DEBUGGING
		static int debugID = 0;
		char path[256];
		sprintf_s(path, "DebugData/ScanlineMask%i.png", debugID++);
		stbi_write_png(path, width, height, 4, flagBits, width * 4);
#endif

		DisposePtr(storage);

		return ScanlineMask::Create(Rect::Create(minPoint.m_y, minPoint.m_x, minPoint.m_y + static_cast<int16_t>(height), minPoint.m_x + static_cast<int16_t>(width)), maskBuilder);
	}

	class SinglePointPlotter final : public IPlotter
	{
	public:
		explicit SinglePointPlotter(const Vec2i &point);

		PlotDirection PlotNext() override;
		const Vec2i &GetPoint() const override;

	private:
		const Vec2i &m_point;
	};

	SinglePointPlotter::SinglePointPlotter(const Vec2i &point)
		: m_point(point)
	{
	}

	PlotDirection SinglePointPlotter::PlotNext()
	{
		return PlotDirection_Exhausted;
	}

	const Vec2i &SinglePointPlotter::GetPoint() const
	{
		return m_point;
	}

	class PolyPlotter final : public IPlotter
	{
	public:
		PolyPlotter(const Vec2i *points, size_t numPoints);

		PlotDirection PlotNext() override;
		const Vec2i &GetPoint() const override;

	private:
		LinePlotter m_linePlotter;
		const Vec2i *m_points;
		size_t m_currentTargetPoint;
		size_t m_numPoints;
	};

	PolyPlotter::PolyPlotter(const Vec2i *points, size_t numPoints)
		: m_points(points)
		, m_currentTargetPoint(0)
		, m_numPoints(numPoints)
	{
		m_linePlotter.Reset(points[numPoints - 1], points[0]);
	}

	PlotDirection PolyPlotter::PlotNext()
	{
		if (m_currentTargetPoint == m_numPoints)
			return PlotDirection_Exhausted;

		for (;;)
		{
			const PlotDirection plotDir = m_linePlotter.PlotNext();
			if (plotDir == PlotDirection_Exhausted)
			{
				m_currentTargetPoint++;
				if (m_currentTargetPoint == m_numPoints)
					return PlotDirection_Exhausted;

				m_linePlotter.Reset(m_points[m_currentTargetPoint - 1], m_points[m_currentTargetPoint]);
			}
			else
				return plotDir;
		}
	}

	const Vec2i &PolyPlotter::GetPoint() const
	{
		return m_linePlotter.GetPoint();
	}

	ScanlineMask *ScanlineMaskConverter::CompilePoly(const Vec2i *points, size_t numPoints)
	{
		assert(numPoints > 0);

		Vec2i minPoint = points[0];
		Vec2i maxPoint = points[0];

		for (size_t i = 1; i < numPoints; i++)
		{
			minPoint.m_x = std::min<int32_t>(minPoint.m_x, points[i].m_x);
			minPoint.m_y = std::min<int32_t>(minPoint.m_y, points[i].m_y);
			maxPoint.m_x = std::max<int32_t>(maxPoint.m_x, points[i].m_x);
			maxPoint.m_y = std::max<int32_t>(maxPoint.m_y, points[i].m_y);
		}

		const uint32_t width = static_cast<uint32_t>(maxPoint.m_x - minPoint.m_x) + 1;
		const uint32_t height = static_cast<uint32_t>(maxPoint.m_y - minPoint.m_y) + 1;

		PolyPlotter polyPlotter(points, numPoints);
		return ComputePlot(width, height, minPoint, polyPlotter);
	}

	ScanlineMask *ScanlineMaskConverter::CompileEllipse(const Rect2i &rect)
	{
		if (!rect.IsValid() || rect.m_topLeft.m_x == rect.m_bottomRight.m_x || rect.m_topLeft.m_y == rect.m_bottomRight.m_y)
			return nullptr;

		const uint32_t width = rect.m_bottomRight.m_x - rect.m_topLeft.m_x;
		const uint32_t height = rect.m_bottomRight.m_y - rect.m_topLeft.m_y;

		if (width == 1 || height == 1)
		{
			if (width == 1 && height == 1)
			{
				SinglePointPlotter plotter(rect.m_topLeft);
				return ComputePlot(1, 1, rect.m_topLeft, plotter);
			}
			else
			{
				LinePlotter plotter;
				plotter.Reset(rect.m_topLeft, rect.m_bottomRight - Vec2i(1, 1));
				return ComputePlot(width, height, rect.m_topLeft, plotter);
			}
		}
		else
		{
			EllipsePlotter plotter;
			plotter.Reset(rect);
			return ComputePlot(width, height, rect.m_topLeft, plotter);
		}
	}
}
