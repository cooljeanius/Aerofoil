#pragma once

#include <stdint.h>
#include <assert.h>

#include "GpCoreDefs.h"

template<class TItem, size_t TCapacity>
class GpRingBuffer
{
public:
	GpRingBuffer()
		: m_Size(0)
		, m_Start(0)
	{
	}

	TItem &operator[](size_t index)
	{
		assert(index < m_Size);
		return m_Items[(m_Start + index) % TCapacity];
	}

	void RemoveFromStart()
	{
		assert(m_Size >= 1);
		m_Start = (m_Start + 1) % TCapacity;
		m_Size--;
	}

	const uint8_t *GetContiguousAtStart(size_t count)
	{
		assert(m_Size >= count);
		assert(m_Start + count <= TCapacity);
		const uint8_t *ptr = m_Items + m_Start;
		m_Start = (m_Start + count) % TCapacity;
		m_Size -= count;

		return ptr;
	}

	void RemoveCountFromStart(size_t count)
	{
		assert(m_Size >= count);
		m_Start = (m_Start + count) % TCapacity;
		m_Size -= count;
	}

	void RemoveFromEnd()
	{
		assert(m_Size >= 1);
		m_Size--;
	}

	void RemoveCountFromEnd(size_t count)
	{
		assert(m_Size >= count);
		m_Size -= count;
	}

	void Clear()
	{
		m_Size = 0;
		m_Start = 0;
	}

	size_t Size() const
	{
		return m_Size;
	}

	TItem *Append()
	{
		if (m_Size == TCapacity)
			return nullptr;

		m_Size++;
		return &m_Items[(m_Start + (m_Size - 1)) % TCapacity];
	}

	size_t SizeContiguous() const
	{
		if (m_Size >= TCapacity - m_Start)
			return TCapacity - m_Start;
		else
			return m_Size;
	}

	size_t AppendMultiple(size_t quantity, TItem *& outPtr)
	{
		if (TCapacity - m_Start > m_Size)
		{
			const size_t available = TCapacity - (m_Start + m_Size);
			outPtr = m_Items + m_Start + m_Size;
			if (available < quantity)
			{
				m_Size += available;
				return available;
			}
			else
			{
				m_Size += quantity;
				return quantity;
			}
		}
		else
		{
			const size_t available = TCapacity - m_Size;
			outPtr = m_Items + ((m_Start + m_Size) % TCapacity);
			if (available < quantity)
			{
				m_Size += available;
				return available;
			}
			else
			{
				m_Size += quantity;
				return quantity;
			}
		}
	}

	static const size_t CAPACITY = TCapacity;

private:
	TItem m_Items[TCapacity];
	size_t m_Size;
	size_t m_Start;
};
