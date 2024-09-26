#include "GpAudioDriverFactory.h"
#include "GpAudioDriverProperties.h"

#include <assert.h>

IGpAudioDriver *GpAudioDriverFactory::CreateAudioDriver(const GpAudioDriverProperties &properties)
{
	assert(properties.m_type < EGpAudioDriverType_Count);

	if (ms_registry[properties.m_type])
		return ms_registry[properties.m_type](properties);
	else
		return nullptr;
}

void GpAudioDriverFactory::RegisterAudioDriverFactory(EGpAudioDriverType type, FactoryFunc_t func)
{
	assert(type < EGpAudioDriverType_Count);

	ms_registry[type] = func;
}

GpAudioDriverFactory::FactoryFunc_t GpAudioDriverFactory::ms_registry[EGpAudioDriverType_Count];
