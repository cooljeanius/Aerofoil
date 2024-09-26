#pragma once
#ifndef __PL_BINARY_SEARCH_H__
#define __PL_BINARY_SEARCH_H__

#include <stdint.h>
#include <stddef.h>

namespace PortabilityLayer
{
	template<class TIterator, class TItem, class TPredicate>
	TIterator BinarySearch(const TIterator &startInclusive, const TIterator &endExclusive, const TItem &item, const TPredicate &pred)
	{
		TIterator searchStartInclusive = startInclusive;
		TIterator searchEndExclusive = endExclusive;

		while (searchStartInclusive != searchEndExclusive)
		{
			const ptrdiff_t delta = searchEndExclusive - searchStartInclusive;
			const ptrdiff_t halfDelta = delta / 2;

			const TIterator midPoint = searchStartInclusive + halfDelta;

			const int comparison = pred(item, *midPoint);

			if (comparison < 0)
				searchEndExclusive = midPoint;
			else if (comparison > 0)
				searchStartInclusive = midPoint + 1;
			else
				return midPoint;
		}

		return endExclusive;
	}
}

#endif
