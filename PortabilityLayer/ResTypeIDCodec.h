#pragma once
#ifndef __PL_RES_TYPE_ID_CODEC_H__
#define __PL_RES_TYPE_ID_CODEC_H__

#include <stdint.h>

namespace PortabilityLayer
{
	template<int32_t T>
	class ResTypeIDCodecResolver
	{
	};

	template<>
	class ResTypeIDCodecResolver<0x64636261>
	{
	public:
		static void Encode(int32_t id, char *chars);
		static int32_t Decode(const char *chars);
	};

	template<>
	class ResTypeIDCodecResolver<0x61626364>
	{
	public:
		static void Encode(int32_t id, char *chars);
		static int32_t Decode(const char *chars);
	};

	typedef ResTypeIDCodecResolver<'abcd'> ResTypeIDCodec;
}

namespace PortabilityLayer
{
	inline void ResTypeIDCodecResolver<0x64636261>::Encode(int32_t id, char *chars)
	{
		chars[0] = static_cast<char>((id >> 0) & 0xff);
		chars[1] = static_cast<char>((id >> 8) & 0xff);
		chars[2] = static_cast<char>((id >> 16) & 0xff);
		chars[3] = static_cast<char>((id >> 24) & 0xff);
	}

	inline int32_t ResTypeIDCodecResolver<0x64636261>::Decode(const char *chars)
	{
		return static_cast<int32_t>(
			((chars[0] & 0xff) << 0)
			| ((chars[1] & 0xff) << 8)
			| ((chars[2] & 0xff) << 16)
			| ((chars[3] & 0xff) << 24));
	}

	inline void ResTypeIDCodecResolver<0x61626364>::Encode(int32_t id, char *chars)
	{
		chars[0] = static_cast<char>((id >> 24) & 0xff);
		chars[1] = static_cast<char>((id >> 16) & 0xff);
		chars[2] = static_cast<char>((id >> 8) & 0xff);
		chars[3] = static_cast<char>((id >> 0) & 0xff);
	}

	inline int32_t ResTypeIDCodecResolver<0x61626364>::Decode(const char *chars)
	{
		return static_cast<int32_t>(
			((chars[0] & 0xff) << 24)
			| ((chars[1] & 0xff) << 16)
			| ((chars[2] & 0xff) << 8)
			| ((chars[3] & 0xff) << 0));
	}
}

#endif
