#pragma once

#include "HostSystemServices.h"
#include "GpCoreDefs.h"
#include "GpWindows.h"

#pragma push_macro("CreateMutex")
#ifdef CreateMutex
#undef CreateMutex
#endif


class GpSystemServices_Win32 final : public PortabilityLayer::HostSystemServices
{
public:
	GpSystemServices_Win32();

	uint32_t GetTime() const override;
	PortabilityLayer::HostMutex *CreateMutex() override;
	PortabilityLayer::HostThreadEvent *CreateThreadEvent(bool autoReset, bool startSignaled) override;

	static GpSystemServices_Win32 *GetInstance();

private:
	static GpSystemServices_Win32 ms_instance;
};

#pragma pop_macro("CreateMutex")