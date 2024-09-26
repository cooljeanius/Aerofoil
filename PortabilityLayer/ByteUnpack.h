#pragma once

#ifndef __PL_BYTEUNPACK_H__
#define __PL_BYTEUNPACK_H__

#include "DataTypes.h"

namespace PortabilityLayer
{
	namespace ByteUnpack
	{
		uint64_t BigUInt64(const uint8_t *bytes);
		uint32_t BigUInt32(const uint8_t *bytes);
		uint16_t BigUInt16(const uint8_t *bytes);

		int64_t BigInt64(const uint8_t *bytes);
		int32_t BigInt32(const uint8_t *bytes);
		int16_t BigInt16(const uint8_t *bytes);
	}
}

namespace PortabilityLayer
{
	namespace ByteUnpack
	{
		inline uint64_t BigUInt64(const uint8_t *bytes)
		{
			return (static_cast<uint64_t>(bytes[0]) << 56)
				| (static_cast<uint64_t>(bytes[1]) << 48)
				| (static_cast<uint64_t>(bytes[2]) << 32)
				| (static_cast<uint64_t>(bytes[3]) << 24)
				| (static_cast<uint64_t>(bytes[4]) << 16)
				| (static_cast<uint64_t>(bytes[5]) << 8)
				| (static_cast<uint64_t>(bytes[6]));
		}

		inline uint32_t BigUInt32(const uint8_t *bytes)
		{
			return (static_cast<uint32_t>(bytes[0]) << 24)
				| (static_cast<uint32_t>(bytes[1]) << 16)
				| (static_cast<uint32_t>(bytes[2]) << 8)
				| (static_cast<uint32_t>(bytes[3]));
		}

		inline uint16_t BigUInt16(const uint8_t *bytes)
		{
			return (static_cast<uint16_t>(bytes[0]) << 8)
				| (static_cast<uint16_t>(bytes[1]));
		}

		inline int64_t BigInt64(const uint8_t *bytes)
		{
			return (static_cast<int64_t>(bytes[0]) << 56)
				| (static_cast<int64_t>(bytes[1]) << 48)
				| (static_cast<int64_t>(bytes[2]) << 32)
				| (static_cast<int64_t>(bytes[3]) << 24)
				| (static_cast<int64_t>(bytes[4]) << 16)
				| (static_cast<int64_t>(bytes[5]) << 8)
				| (static_cast<int64_t>(bytes[6]));
		}

		inline int32_t BigInt32(const uint8_t *bytes)
		{
			return (static_cast<int32_t>(bytes[0]) << 24)
				| (static_cast<int32_t>(bytes[1]) << 16)
				| (static_cast<int32_t>(bytes[2]) << 8)
				| (static_cast<int32_t>(bytes[3]));
		}

		inline int16_t BigInt16(const uint8_t *bytes)
		{
			return (static_cast<int16_t>(bytes[0]) << 8)
				| (static_cast<int16_t>(bytes[1]));
		}
	}
}

#endif
