#pragma once

#include "EGpInputDriverType.h"

struct IGpInputDriver;
struct GpInputDriverProperties;

class GpInputDriverFactory
{
public:
	typedef IGpInputDriver *(*FactoryFunc_t)(const GpInputDriverProperties &properties);

	static IGpInputDriver *CreateInputDriver(const GpInputDriverProperties &properties);
	static void RegisterInputDriverFactory(EGpInputDriverType type, FactoryFunc_t func);

private:
	static FactoryFunc_t ms_registry[EGpInputDriverType_Count];
};
