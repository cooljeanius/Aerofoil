#pragma once

#include <stdint.h>

template<class T>
struct LEInteger
{
public:
	LEInteger();
	LEInteger(const LEInteger<T> &other);
	explicit LEInteger(T i);

	operator T() const;
	LEInteger<T> &operator=(T value);

	template<class TOther>
	LEInteger<T> &operator+=(TOther value);

	template<class TOther>
	LEInteger<T> &operator-=(TOther value);

	template<class TOther>
	LEInteger<T> &operator*=(TOther value);

	template<class TOther>
	LEInteger<T> &operator/=(TOther value);

	template<class TOther>
	LEInteger<T> &operator%=(TOther value);

	LEInteger<T>& operator--();
	LEInteger<T> operator--(int);

	LEInteger<T>& operator++();
	LEInteger<T> operator++(int);

private:
	uint8_t m_leValueBytes[sizeof(T)];
};

template<class T>
struct LEInteger_SwapHelper
{
};

#include "ByteSwap.h"

template<>
struct LEInteger_SwapHelper<int16_t>
{
	inline static void Swap(int16_t &v)
	{
		PortabilityLayer::ByteSwap::LittleInt16(v);
	}
};

template<>
struct LEInteger_SwapHelper<int32_t>
{
	inline static void Swap(int32_t &v)
	{
		PortabilityLayer::ByteSwap::LittleInt32(v);
	}
};

template<>
struct LEInteger_SwapHelper<uint16_t>
{
	inline static void Swap(uint16_t &v)
	{
		PortabilityLayer::ByteSwap::LittleUInt16(v);
	}
};

template<>
struct LEInteger_SwapHelper<uint32_t>
{
	inline static void Swap(uint32_t &v)
	{
		PortabilityLayer::ByteSwap::LittleUInt32(v);
	}
};

#include <string.h>

template<class T>
inline LEInteger<T>::LEInteger()
{
	memset(m_leValueBytes, 0, sizeof(T));
}

template<class T>
inline LEInteger<T>::LEInteger(const LEInteger<T> &other)
{
	memcpy(m_leValueBytes, other.m_leValueBytes, sizeof(T));
}

template<class T>
inline LEInteger<T>::LEInteger(T i)
{
	LEInteger_SwapHelper<T>::Swap(i);
	memcpy(m_leValueBytes, &i, sizeof(T));
}

template<class T>
inline LEInteger<T>::operator T() const
{
	T result;
	memcpy(&result, m_leValueBytes, sizeof(T));
	LEInteger_SwapHelper<T>::Swap(result);
	return result;
}

template<class T>
inline LEInteger<T> &LEInteger<T>::operator=(T value)
{
	LEInteger_SwapHelper<T>::Swap(value);
	memcpy(m_leValueBytes, &value, sizeof(T));
	return *this;
}

template<class T>
template<class TOther>
LEInteger<T> &LEInteger<T>::operator+=(TOther value)
{
	T storedValue;
	memcpy(&storedValue, m_leValueBytes, sizeof(T));

	LEInteger_SwapHelper<T>::Swap(storedValue);
	storedValue += value;
	LEInteger_SwapHelper<T>::Swap(storedValue);

	memcpy(m_leValueBytes, &storedValue, sizeof(T));
	return *this;
}

template<class T>
template<class TOther>
LEInteger<T> &LEInteger<T>::operator-=(TOther value)
{
	T storedValue;
	memcpy(&storedValue, m_leValueBytes, sizeof(T));

	LEInteger_SwapHelper<T>::Swap(storedValue);
	storedValue -= value;
	LEInteger_SwapHelper<T>::Swap(storedValue);

	memcpy(m_leValueBytes, &storedValue, sizeof(T));
	return *this;
}

template<class T>
template<class TOther>
LEInteger<T> &LEInteger<T>::operator*=(TOther value)
{
	T storedValue;
	memcpy(&storedValue, m_leValueBytes, sizeof(T));

	LEInteger_SwapHelper<T>::Swap(storedValue);
	storedValue *= value;
	LEInteger_SwapHelper<T>::Swap(storedValue);

	memcpy(m_leValueBytes, &storedValue, sizeof(T));
	return *this;
}

template<class T>
template<class TOther>
LEInteger<T> &LEInteger<T>::operator/=(TOther value)
{
	T storedValue;
	memcpy(&storedValue, m_leValueBytes, sizeof(T));

	LEInteger_SwapHelper<T>::Swap(storedValue);
	storedValue /= value;
	LEInteger_SwapHelper<T>::Swap(storedValue);

	memcpy(m_leValueBytes, &storedValue, sizeof(T));
	return *this;
}

template<class T>
template<class TOther>
LEInteger<T> &LEInteger<T>::operator%=(TOther value)
{
	T storedValue;
	memcpy(&storedValue, m_leValueBytes, sizeof(T));

	LEInteger_SwapHelper<T>::Swap(storedValue);
	storedValue %= value;
	LEInteger_SwapHelper<T>::Swap(storedValue);

	memcpy(m_leValueBytes, &storedValue, sizeof(T));
	return *this;
}

template<class T>
LEInteger<T>& LEInteger<T>::operator--()
{
	T storedValue;
	memcpy(&storedValue, m_leValueBytes, sizeof(T));

	LEInteger_SwapHelper<T>::Swap(storedValue);
	storedValue--;
	LEInteger_SwapHelper<T>::Swap(storedValue);

	memcpy(m_leValueBytes, &storedValue, sizeof(T));
	return *this;
}

template<class T>
LEInteger<T> LEInteger<T>::operator--(int)
{
	LEInteger<T> orig(*this);
	--(*this);
	return orig;
}

template<class T>
LEInteger<T>& LEInteger<T>::operator++()
{
	T storedValue;
	memcpy(&storedValue, m_leValueBytes, sizeof(T));

	LEInteger_SwapHelper<T>::Swap(storedValue);
	storedValue++;
	LEInteger_SwapHelper<T>::Swap(storedValue);

	memcpy(m_leValueBytes, &storedValue, sizeof(T));
	return *this;
}

template<class T>
LEInteger<T> LEInteger<T>::operator++(int)
{
	LEInteger<T> orig(*this);
	++(*this);
	return orig;
}


typedef LEInteger<int16_t> LEInt16_t;
typedef LEInteger<int32_t> LEInt32_t;
typedef LEInteger<uint16_t> LEUInt16_t;
typedef LEInteger<uint32_t> LEUInt32_t;
