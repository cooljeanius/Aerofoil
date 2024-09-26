#include "GpFontHandlerFactory.h"
#include "GpFontHandlerProperties.h"

#include <assert.h>

IGpFontHandler *GpFontHandlerFactory::CreateFontHandler(const GpFontHandlerProperties &properties)
{
	assert(properties.m_type < EGpFontHandlerType_Count);

	if (ms_registry[properties.m_type])
		return ms_registry[properties.m_type](properties);
	else
		return nullptr;
}

void GpFontHandlerFactory::RegisterFontHandlerFactory(EGpFontHandlerType type, FactoryFunc_t func)
{
	assert(type < EGpFontHandlerType_Count);

	ms_registry[type] = func;
}

GpFontHandlerFactory::FactoryFunc_t GpFontHandlerFactory::ms_registry[EGpFontHandlerType_Count];
