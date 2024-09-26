#pragma once
#ifndef __PL_PASCAL_STR_LITERAL_H__
#define __PL_PASCAL_STR_LITERAL_H__

#include "DataTypes.h"

namespace PortabilityLayer
{
	template<size_t TSize>
	class PascalStrLiteral
	{
	public:
		PascalStrLiteral(const char (&literalStr)[TSize]);
		const char *GetStr() const;

		static const uint8_t kLength = TSize - 1;

	private:
		const char *m_literal;
	};
}

namespace PortabilityLayer
{
	template<size_t TSize>
	inline PascalStrLiteral<TSize>::PascalStrLiteral(const char(&literalStr)[TSize])
		: m_literal(literalStr)
	{
	}

	template<size_t TSize>
	inline const char *PascalStrLiteral<TSize>::GetStr() const
	{
		return m_literal;
	}
}

#define PSTR(n) (::PortabilityLayer::PascalStrLiteral<sizeof(n)>(n))

#endif
