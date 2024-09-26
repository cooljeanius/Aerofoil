#include "GpInputDriverFactory.h"
#include "GpInputDriverProperties.h"

#include <assert.h>

IGpInputDriver *GpInputDriverFactory::CreateInputDriver(const GpInputDriverProperties &properties)
{
	assert(properties.m_type < EGpInputDriverType_Count);

	if (ms_registry[properties.m_type])
		return ms_registry[properties.m_type](properties);
	else
		return nullptr;
}

void GpInputDriverFactory::RegisterInputDriverFactory(EGpInputDriverType type, FactoryFunc_t func)
{
	assert(type < EGpInputDriverType_Count);

	ms_registry[type] = func;
}

GpInputDriverFactory::FactoryFunc_t GpInputDriverFactory::ms_registry[EGpInputDriverType_Count];
