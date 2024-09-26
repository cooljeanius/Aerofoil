#include "IGpSystemServices.h"

#include "PLApplication.h"
#include "PLCore.h"
#include "PLDrivers.h"

#include <string.h>
#include <assert.h>

namespace PortabilityLayer
{
	namespace Utils
	{
		void MakePStr(unsigned char *dest, size_t sz, const char *src)
		{
			assert(sz <= 255);
			dest[0] = static_cast<uint8_t>(sz);
			memcpy(dest + 1, src, sz);
		}
	}

	static BeepFunction_t gs_beepFunction = nullptr;
}

void SysBeep(int duration)
{
	if (!PLDrivers::GetSystemServices()->Beep())
		if (PortabilityLayer::gs_beepFunction != nullptr)
			PortabilityLayer::gs_beepFunction(duration);
}

void SetBeepFunction(BeepFunction_t beepFunction)
{
	PortabilityLayer::gs_beepFunction = beepFunction;
}
