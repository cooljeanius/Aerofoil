#pragma once

#include "EGpFontHandlerType.h"

struct IGpFontHandler;

struct GpFontHandlerProperties;

class GpFontHandlerFactory
{
public:
	typedef IGpFontHandler *(*FactoryFunc_t)(const GpFontHandlerProperties &properties);

	static IGpFontHandler *CreateFontHandler(const GpFontHandlerProperties &properties);
	static void RegisterFontHandlerFactory(EGpFontHandlerType type, FactoryFunc_t func);

private:
	static FactoryFunc_t ms_registry[EGpFontHandlerType_Count];
};
