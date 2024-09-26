#pragma once

#ifndef __PL_SMALLESTINT_H__
#define __PL_SMALLESTINT_H__

#include "DataTypes.h"

namespace PortabilityLayer
{
	namespace Internal_SmallestInt
	{
		template<bool TFits8, bool TFits16, bool TFits32, bool TFits64>
		class SmallestIntSignedResolver
		{
		};

		template<bool TFits8, bool TFits16, bool TFits32, bool TFits64>
		class SmallestIntUnsignedResolver
		{
		};

		template<>
		class SmallestIntSignedResolver<1, 1, 1, 1>
		{
		public:
			typedef int8_t ValueType_t;
		};

		template<>
		class SmallestIntSignedResolver<0, 1, 1, 1>
		{
		public:
			typedef int16_t ValueType_t;
		};

		template<>
		class SmallestIntSignedResolver<0, 0, 1, 1>
		{
		public:
			typedef int32_t ValueType_t;
		};

		template<>
		class SmallestIntSignedResolver<0, 0, 0, 1>
		{
		public:
			typedef int64_t ValueType_t;
		};

		template<>
		class SmallestIntUnsignedResolver<1, 1, 1, 1>
		{
		public:
			typedef uint8_t ValueType_t;
		};

		template<>
		class SmallestIntUnsignedResolver<0, 1, 1, 1>
		{
		public:
			typedef uint16_t ValueType_t;
		};

		template<>
		class SmallestIntUnsignedResolver<0, 0, 1, 1>
		{
		public:
			typedef uint32_t ValueType_t;
		};

		template<>
		class SmallestIntUnsignedResolver<0, 0, 0, 1>
		{
		public:
			typedef uint64_t ValueType_t;
		};
	}

	template<LargestInt_t TValue>
	struct SmallestInt
	{
		typedef typename Internal_SmallestInt::SmallestIntSignedResolver<
			TValue >= -128 && TValue <= 127,
			TValue >= -32768 && TValue <= 32767,
			TValue >= -2147483648LL && TValue <= 2147483647LL,
			1>::ValueType_t ValueType_t;
	};

	template<LargestUInt_t TValue>
	struct SmallestUInt
	{
		typedef typename Internal_SmallestInt::SmallestIntUnsignedResolver<
			TValue <= 256,
			TValue <= 65536,
			 TValue <= 4294967295,
			1>::ValueType_t ValueType_t;
	};
}

#endif
