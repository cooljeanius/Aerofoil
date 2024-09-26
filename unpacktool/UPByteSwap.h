#pragma once

#include <stdint.h>

#include "ByteSwap.h"

inline uint32_t ParseUInt32BE(const uint8_t *bytes)
{
	return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

inline int32_t ParseInt32BE(const uint8_t *bytes)
{
	return static_cast<int32_t>(ParseUInt32BE(bytes));
}

inline uint16_t ParseUInt16BE(const uint8_t *bytes)
{
	return (bytes[0] << 8) | bytes[1];
}

inline int16_t ParseInt16BE(const uint8_t *bytes)
{
	return static_cast<int16_t>(ParseInt16BE(bytes));
}

inline uint32_t ParseUInt32LE(const uint8_t *bytes)
{
	return (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
}

inline int32_t ParseInt32LE(const uint8_t *bytes)
{
	return static_cast<int32_t>(ParseUInt32LE(bytes));
}

inline uint16_t ParseUInt16LE(const uint8_t *bytes)
{
	return (bytes[1] << 8) | bytes[0];
}

inline int16_t ParseInt16LE(const uint8_t *bytes)
{
	return static_cast<int16_t>(ParseInt16LE(bytes));
}
