#pragma once

#include "GpVector.h"

#include <stdint.h>
#include <stddef.h>

template<class TChar>
class GpString
{
public:
	explicit GpString(IGpAllocator *alloc);
	GpString(GpString<TChar> &&other);

	bool Set(const TChar *str);
	bool Set(const TChar *str, size_t len);
	bool Set(const GpString<TChar> &other);

	bool Append(const TChar *str);
	bool Append(const TChar *str, size_t len);
	bool Append(const GpString<TChar> &other);

	TChar *Buffer();
	const TChar *Buffer() const ;


private:
	static const size_t kStaticSize = 32;

	GpVector<TChar, kStaticSize> m_chars;
	size_t m_length;
};

typedef GpString<char> GpCString;
typedef GpString<wchar_t> GpWString;

#include <new>
#include <string.h>

template<class TChar>
GpString<TChar>::GpString(IGpAllocator *alloc)
	: m_chars(alloc)
	, m_length(0)
{
	(void)m_chars.Resize(1);
	m_chars[0] = static_cast<TChar>(0);
}

template<class TChar>
GpString<TChar>::GpString(GpString<TChar> &&other)
	: m_chars(static_cast<GpVector<TChar, kStaticSize>&&>(other.m_chars))
	, m_length(other.m_length)
{
	other.m_length = 0;
}


template<class TChar>
bool GpString<TChar>::Set(const TChar *str)
{
	size_t len = 0;
	while (str[len] != static_cast<TChar>(0))
		len++;

	return this->Set(str, len);
}

template<class TChar>
bool GpString<TChar>::Set(const TChar *str, size_t len)
{
	if (!m_chars.ResizeNoConstruct(len + 1))
		return false;

	TChar *chars = m_chars.Buffer();
	memcpy(chars, str, sizeof(TChar) * len);
	chars[len] = static_cast<TChar>(0);

	m_length = len;

	return true;
}

template<class TChar>
bool GpString<TChar>::Set(const GpString<TChar> &other)
{
	if (&other == this)
		return true;

	return this->Set(other.Buffer(), other.m_length);
}

template<class TChar>
bool GpString<TChar>::Append(const TChar *str)
{
	size_t len = 0;
	while (str[len] != static_cast<TChar>(0))
		len++;

	return this->Append(str, len);
}

template<class TChar>
bool GpString<TChar>::Append(const TChar *str, size_t len)
{
	// This is a special path in case we're appending the string to itself and the resize relocates the buffer
	if (len == 0)
		return true;

	assert(str != this->Buffer());

	if (!m_chars.ResizeNoConstruct(m_length + len + 1))
		return false;

	memcpy(m_chars.Buffer() + m_length, str, sizeof(TChar) * len);
	m_chars[m_length + len] = static_cast<TChar>(0);

	m_length += len;

	return true;
}

template<class TChar>
bool GpString<TChar>::Append(const GpString<TChar> &other)
{
	// This is a special path in case we're appending the string to itself and the resize relocates the buffer
	if (other.m_length == 0)
		return true;

	if (!m_chars.ResizeNoConstruct(m_length + other.m_length))
		return false;

	memcpy(m_chars.Buffer() + m_length, other.Buffer(), sizeof(TChar) * other.m_length);
	m_chars[m_length + other.m_length] = static_cast<TChar>(0);

	m_length += other.m_length;

	return true;
}

template<class TChar>
TChar *GpString<TChar>::Buffer()
{
	return m_chars.Buffer();
}

template<class TChar>
const TChar *GpString<TChar>::Buffer() const
{
	return m_chars.Buffer();
}
