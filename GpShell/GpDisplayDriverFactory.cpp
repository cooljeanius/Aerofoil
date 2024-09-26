#include "GpDisplayDriverFactory.h"
#include "GpDisplayDriverProperties.h"

#include <assert.h>

IGpDisplayDriver *GpDisplayDriverFactory::CreateDisplayDriver(const GpDisplayDriverProperties &properties)
{
	assert(properties.m_type < EGpDisplayDriverType_Count);

	if (ms_registry[properties.m_type])
		return ms_registry[properties.m_type](properties);
	else
		return nullptr;
}

void GpDisplayDriverFactory::RegisterDisplayDriverFactory(EGpDisplayDriverType type, FactoryFunc_t func)
{
	assert(type < EGpDisplayDriverType_Count);

	ms_registry[type] = func;
}

GpDisplayDriverFactory::FactoryFunc_t GpDisplayDriverFactory::ms_registry[EGpDisplayDriverType_Count];
