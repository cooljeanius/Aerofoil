#pragma once
#ifndef __PL_HOST_DISPLAY_DRIVER_H__
#define __PL_HOST_DISPLAY_DRIVER_H__

#include "GpPixelFormat.h"
#include "EGpStandardCursor.h"

struct IGpColorCursor;

namespace PortabilityLayer
{
	class HostDisplayDriver
	{
	public:
		virtual void GetDisplayResolution(unsigned int *width, unsigned int *height, GpPixelFormat_t *pixelFormat) = 0;
		virtual IGpColorCursor *LoadColorCursor(int id) = 0;
		virtual void SetColorCursor(IGpColorCursor *colorCursor) = 0;
		virtual void SetStandardCursor(EGpStandardCursor_t standardCursor) = 0;

		static void SetInstance(HostDisplayDriver *instance);
		static HostDisplayDriver *GetInstance();

	private:
		static HostDisplayDriver *ms_instance;
	};
}

#endif