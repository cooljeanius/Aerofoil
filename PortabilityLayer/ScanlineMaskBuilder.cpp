#include "ScanlineMaskBuilder.h"

#include <stdlib.h>

namespace PortabilityLayer
{
	ScanlineMaskBuilder::ScanlineMaskBuilder()
		: m_spans(nullptr)
		, m_numSpans(0)
		, m_capacity(0)
		, m_longestSpan(0)
	{
	}

	ScanlineMaskBuilder::~ScanlineMaskBuilder()
	{
		if (m_spans)
			free(m_spans);
	}

	bool ScanlineMaskBuilder::AppendSpan(size_t span)
	{
		if (span > m_longestSpan)
			m_longestSpan = span;

		if (m_capacity == 0)
		{
			m_capacity = 8;
			m_spans = static_cast<size_t*>(realloc(nullptr, sizeof(size_t) * m_capacity));
			if (!m_spans)
				return false;
		}

		if (m_numSpans == m_capacity)
		{
			if (m_capacity >= (SIZE_MAX / sizeof(size_t) / 2))
				return false;

			m_capacity *= 2;
			void *newSpans = realloc(m_spans, sizeof(size_t) * m_capacity);
			if (!newSpans)
				return false;

			m_spans = static_cast<size_t*>(newSpans);
		}

		m_spans[m_numSpans++] = span;

		return true;
	}

	size_t ScanlineMaskBuilder::GetLongestSpan() const
	{
		return m_longestSpan;
	}

	const size_t *ScanlineMaskBuilder::GetSpans() const
	{
		return m_spans;
	}

	size_t ScanlineMaskBuilder::GetNumSpans() const
	{
		return m_numSpans;
	}
}
