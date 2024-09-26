#pragma once
#ifndef __PL_RANDOM_NUMBER_GENERATOR_H__
#define __PL_RANDOM_NUMBER_GENERATOR_H__

#include <stdint.h>

namespace PortabilityLayer
{
	class RandomNumberGenerator
	{
	public:
		virtual void Seed(uint32_t seed) = 0;
		virtual uint32_t GetNextAndAdvance() = 0;

		static RandomNumberGenerator *GetInstance();
	};
}

#endif
