#pragma once

#include <stdint.h>

template<class T>
struct BEInteger
{
public:
	BEInteger();
	BEInteger(const BEInteger<T> &other);
	explicit BEInteger(T i);

	operator T() const;
	BEInteger<T> &operator=(T value);

	template<class TOther>
	BEInteger<T> &operator+=(TOther value);

	template<class TOther>
	BEInteger<T> &operator-=(TOther value);

	template<class TOther>
	BEInteger<T> &operator*=(TOther value);

	template<class TOther>
	BEInteger<T> &operator/=(TOther value);

	template<class TOther>
	BEInteger<T> &operator%=(TOther value);

	BEInteger<T>& operator--();
	BEInteger<T> operator--(int);

	BEInteger<T>& operator++();
	BEInteger<T> operator++(int);

private:
	uint8_t m_beValueBytes[sizeof(T)];
};

template<class T>
struct BEInteger_SwapHelper
{
};

#include "ByteSwap.h"

template<>
struct BEInteger_SwapHelper<int16_t>
{
	inline static void Swap(int16_t &v)
	{
		PortabilityLayer::ByteSwap::BigInt16(v);
	}
};

template<>
struct BEInteger_SwapHelper<int32_t>
{
	inline static void Swap(int32_t &v)
	{
		PortabilityLayer::ByteSwap::BigInt32(v);
	}
};

template<>
struct BEInteger_SwapHelper<uint16_t>
{
	inline static void Swap(uint16_t &v)
	{
		PortabilityLayer::ByteSwap::BigUInt16(v);
	}
};

template<>
struct BEInteger_SwapHelper<uint32_t>
{
	inline static void Swap(uint32_t &v)
	{
		PortabilityLayer::ByteSwap::BigUInt32(v);
	}
};

#include <string.h>

template<class T>
inline BEInteger<T>::BEInteger()
{
	memset(m_beValueBytes, 0, sizeof(T));
}

template<class T>
inline BEInteger<T>::BEInteger(const BEInteger<T> &other)
{
	memcpy(m_beValueBytes, other.m_beValueBytes, sizeof(T));
}

template<class T>
inline BEInteger<T>::BEInteger(T i)
{
	BEInteger_SwapHelper<T>::Swap(i);
	memcpy(m_beValueBytes, &i, sizeof(T));
}

template<class T>
inline BEInteger<T>::operator T() const
{
	T result;
	memcpy(&result, m_beValueBytes, sizeof(T));
	BEInteger_SwapHelper<T>::Swap(result);
	return result;
}

template<class T>
inline BEInteger<T> &BEInteger<T>::operator=(T value)
{
	BEInteger_SwapHelper<T>::Swap(value);
	memcpy(m_beValueBytes, &value, sizeof(T));
	return *this;
}

template<class T>
template<class TOther>
BEInteger<T> &BEInteger<T>::operator+=(TOther value)
{
	T storedValue;
	memcpy(&storedValue, m_beValueBytes, sizeof(T));

	BEInteger_SwapHelper<T>::Swap(storedValue);
	storedValue += value;
	BEInteger_SwapHelper<T>::Swap(storedValue);

	memcpy(m_beValueBytes, &storedValue, sizeof(T));
	return *this;
}

template<class T>
template<class TOther>
BEInteger<T> &BEInteger<T>::operator-=(TOther value)
{
	T storedValue;
	memcpy(&storedValue, m_beValueBytes, sizeof(T));

	BEInteger_SwapHelper<T>::Swap(storedValue);
	storedValue -= value;
	BEInteger_SwapHelper<T>::Swap(storedValue);

	memcpy(m_beValueBytes, &storedValue, sizeof(T));
	return *this;
}

template<class T>
template<class TOther>
BEInteger<T> &BEInteger<T>::operator*=(TOther value)
{
	T storedValue;
	memcpy(&storedValue, m_beValueBytes, sizeof(T));

	BEInteger_SwapHelper<T>::Swap(storedValue);
	storedValue *= value;
	BEInteger_SwapHelper<T>::Swap(storedValue);

	memcpy(m_beValueBytes, &storedValue, sizeof(T));
	return *this;
}

template<class T>
template<class TOther>
BEInteger<T> &BEInteger<T>::operator/=(TOther value)
{
	T storedValue;
	memcpy(&storedValue, m_beValueBytes, sizeof(T));

	BEInteger_SwapHelper<T>::Swap(storedValue);
	storedValue /= value;
	BEInteger_SwapHelper<T>::Swap(storedValue);

	memcpy(m_beValueBytes, &storedValue, sizeof(T));
	return *this;
}

template<class T>
template<class TOther>
BEInteger<T> &BEInteger<T>::operator%=(TOther value)
{
	T storedValue;
	memcpy(&storedValue, m_beValueBytes, sizeof(T));

	BEInteger_SwapHelper<T>::Swap(storedValue);
	storedValue %= value;
	BEInteger_SwapHelper<T>::Swap(storedValue);

	memcpy(m_beValueBytes, &storedValue, sizeof(T));
	return *this;
}

template<class T>
BEInteger<T>& BEInteger<T>::operator--()
{
	T storedValue;
	memcpy(&storedValue, m_beValueBytes, sizeof(T));

	BEInteger_SwapHelper<T>::Swap(storedValue);
	storedValue--;
	BEInteger_SwapHelper<T>::Swap(storedValue);

	memcpy(m_beValueBytes, &storedValue, sizeof(T));
	return *this;
}

template<class T>
BEInteger<T> BEInteger<T>::operator--(int)
{
	BEInteger<T> orig(*this);
	--(*this);
	return orig;
}

template<class T>
BEInteger<T>& BEInteger<T>::operator++()
{
	T storedValue;
	memcpy(&storedValue, m_beValueBytes, sizeof(T));

	BEInteger_SwapHelper<T>::Swap(storedValue);
	storedValue++;
	BEInteger_SwapHelper<T>::Swap(storedValue);

	memcpy(m_beValueBytes, &storedValue, sizeof(T));
	return *this;
}

template<class T>
BEInteger<T> BEInteger<T>::operator++(int)
{
	BEInteger<T> orig(*this);
	++(*this);
	return orig;
}


typedef BEInteger<int16_t> BEInt16_t;
typedef BEInteger<int32_t> BEInt32_t;
typedef BEInteger<uint16_t> BEUInt16_t;
typedef BEInteger<uint32_t> BEUInt32_t;

struct BESFixed32_t
{
	BEInt16_t m_intPart;
	BEUInt16_t m_fracPart;
};

struct BEUFixed32_t
{
	BEUInt16_t m_intPart;
	BEUInt16_t m_fracPart;
};

struct BESFixed16_t
{
	int8_t m_intPart;
	uint8_t m_fracPart;
};

struct BEUFixed16_t
{
	uint8_t m_intPart;
	uint8_t m_fracPart;
};
