#pragma once

#include "EGpAudioDriverType.h"

struct IGpAudioDriver;
struct GpAudioDriverProperties;

class GpAudioDriverFactory
{
public:
	typedef IGpAudioDriver *(*FactoryFunc_t)(const GpAudioDriverProperties &properties);

	static IGpAudioDriver *CreateAudioDriver(const GpAudioDriverProperties &properties);
	static void RegisterAudioDriverFactory(EGpAudioDriverType type, FactoryFunc_t func);

private:
	static FactoryFunc_t ms_registry[EGpAudioDriverType_Count];
};
