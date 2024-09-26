#pragma once

#ifndef __PL_UNSAFEPASCALSTR_H__
#define __PL_UNSAFEPASCALSTR_H__

#include "DataTypes.h"
#include "SmallestInt.h"

#include <stddef.h>

class PLPasStr;

namespace PortabilityLayer
{
	template<size_t TSize, bool TCStr>
	class UnsafePascalStr
	{
	public:
		UnsafePascalStr();
		UnsafePascalStr(size_t size, const char *str);

		char &operator[](size_t index);
		const char &operator[](size_t index) const;
		void Set(size_t size, const char *str);
		void SetLength(size_t size);
		size_t Length() const;

		const char *UnsafeCharPtr() const;
		PLPasStr ToShortStr() const;

	private:
		char m_chars[TSize + (TCStr ? 1 : 0)];
		typename SmallestUInt<TSize>::ValueType_t m_size;
	};
}

#include <string.h>
#include <assert.h>

#include "PLPasStr.h"

namespace PortabilityLayer
{
	template<size_t TSize, bool TCStr>
	inline UnsafePascalStr<TSize, TCStr>::UnsafePascalStr(size_t size, const char *str)
		: m_size(static_cast<typename SmallestUInt<TSize>::ValueType_t>(size))
	{
		assert(size <= TSize);

		if (size)
			memcpy(m_chars, str, size);

		if (TCStr)
			m_chars[size] = '\0';
	}

	template<size_t TSize, bool TCStr>
	inline char &UnsafePascalStr<TSize, TCStr>::operator[](size_t index)
	{
		assert(index < m_size);
		return m_chars[index];
	}

	template<size_t TSize, bool TCStr>
	inline const char &UnsafePascalStr<TSize, TCStr>::operator[](size_t index) const
	{
		assert(index < m_size);
		return m_chars[index];
	}

	template<size_t TSize, bool TCStr>
	inline void UnsafePascalStr<TSize, TCStr>::Set(size_t size, const char *str)
	{
		assert(size <= TSize);

		memcpy(m_chars, str, size);
		m_size = static_cast<typename SmallestUInt<TSize>::ValueType_t>(size);

		if (TCStr)
			m_chars[size] = '\0';
	}

	template<size_t TSize, bool TCStr>
	inline void UnsafePascalStr<TSize, TCStr>::SetLength(size_t size)
	{
		assert(size <= TSize);

		if (TCStr)
			m_chars[size] = '\0';
	}

	template<size_t TSize, bool TCStr>
	inline size_t UnsafePascalStr<TSize, TCStr>::Length() const
	{
		return m_size;
	}

	template<size_t TSize, bool TCStr>
	inline const char *UnsafePascalStr<TSize, TCStr>::UnsafeCharPtr() const
	{
		return m_chars;
	}

	template<size_t TSize, bool TCStr>
	PLPasStr UnsafePascalStr<TSize, TCStr>::ToShortStr() const
	{
		if (m_size > 255)
			return PLPasStr(255, m_chars);
		else
			return PLPasStr(static_cast<uint8_t>(m_size), m_chars);
	}
}

#endif
