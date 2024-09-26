#pragma once
#ifndef __PL_PASSTR_H__
#define __PL_PASSTR_H__

#include "CoreDefs.h"

namespace PortabilityLayer
{
	template<size_t TSize>
	class PascalStrLiteral;
}

class PLPasStr
{
public:
	PLPasStr();
	PLPasStr(const unsigned char *str);
	template<size_t TSize> PLPasStr(const PortabilityLayer::PascalStrLiteral<TSize> &pstrLiteral);
	PLPasStr(const PLPasStr &other);
	PLPasStr(unsigned char length, const char *str);

	unsigned char Length() const;
	const char *Chars() const;
	const unsigned char *UChars() const;

private:
	unsigned char m_length;
	const char *m_chars;
};

#include "PascalStrLiteral.h"

inline PLPasStr::PLPasStr()
	: m_length(0)
	, m_chars(nullptr)
{
}

inline PLPasStr::PLPasStr(const unsigned char *str)
	: m_length(str[0])
	, m_chars(reinterpret_cast<const char*>(str) + 1)
{
}

template<size_t TSize>
inline PLPasStr::PLPasStr(const PortabilityLayer::PascalStrLiteral<TSize> &pstrLiteral)
	: m_length(pstrLiteral.kLength)
	, m_chars(pstrLiteral.GetStr())
{
}

inline PLPasStr::PLPasStr(const PLPasStr &other)
	: m_length(other.m_length)
	, m_chars(other.m_chars)
{
}

inline PLPasStr::PLPasStr(uint8_t length, const char *str)
	: m_length(length)
	, m_chars(str)
{
}

inline unsigned char PLPasStr::Length() const
{
	return m_length;
}

inline const char *PLPasStr::Chars() const
{
	return m_chars;
}

inline const unsigned char *PLPasStr::UChars() const
{
	return reinterpret_cast<const unsigned char*>(m_chars);
}

#endif
