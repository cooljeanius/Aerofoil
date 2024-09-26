#pragma once
#ifndef __PL_APPLICATION_H__
#define __PL_APPLICATION_H__

#include "PLCore.h"

namespace PortabilityLayer
{
	template<size_t TSize>
	class PascalStrLiteral;
}

namespace PortabilityLayer
{
	namespace Utils
	{
		void MakePStr(unsigned char *dest, size_t sz, const char *src);
	}
}

void PasStringCopy(const unsigned char *src, unsigned char *dest);

template<size_t TSize>
void PasStringCopy(const PortabilityLayer::PascalStrLiteral<TSize> &src, unsigned char *dest);

void SysBeep(int duration);

typedef void (*BeepFunction_t)(int duration);
void SetBeepFunction(BeepFunction_t beepFunction);


///////////////////////////////////////////////////////////////////////////////
#include "PascalStrLiteral.h"

template<size_t TSize>
inline void PasStringCopy(const PortabilityLayer::PascalStrLiteral<TSize> &src, unsigned char *dest)
{
	PortabilityLayer::Utils::MakePStr(dest, TSize - 1, src.GetStr());
}

#endif
