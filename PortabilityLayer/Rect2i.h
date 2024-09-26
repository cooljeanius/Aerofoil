#pragma once

#include <stdint.h>

#include "Vec2i.h"

struct Rect;

namespace PortabilityLayer
{
	struct Vec2i;

	struct Rect2i
	{
		Vec2i m_topLeft;
		Vec2i m_bottomRight;

		Rect2i();
		Rect2i(const Rect2i &other);
		explicit Rect2i(const Rect &other);
		Rect2i(const Vec2i &topLeft, const Vec2i &bottomRight);
		Rect2i(int32_t top, int32_t left, int32_t bottom, int32_t right);

		Rect2i operator+(const Vec2i &other) const;
		Rect2i operator-(const Vec2i &other) const;

		Rect2i &operator+=(const Vec2i &other);
		Rect2i &operator-=(const Vec2i &other);

		int32_t Top() const;
		void SetTop(int32_t i);

		int32_t Left() const;
		void SetLeft(int32_t i);

		int32_t Bottom() const;
		void SetBottom(int32_t i);

		int32_t Right() const;
		void SetRight(int32_t i);

		bool IsValid() const;
		Rect2i Intersect(const Rect2i &other) const;
		bool Contains(const Vec2i &pt) const;

		Rect ToShortRect() const;
	};
}

#include "SharedTypes.h"
#include <algorithm>

namespace PortabilityLayer
{
	inline Rect2i::Rect2i()
	{
	}

	inline Rect2i::Rect2i(const Rect2i &other)
		: m_topLeft(other.m_topLeft)
		, m_bottomRight(other.m_bottomRight)
	{
	}

	inline Rect2i::Rect2i(const Rect &other)
		: m_topLeft(other.left, other.top)
		, m_bottomRight(other.right, other.bottom)
	{
	}

	inline Rect2i::Rect2i(int32_t top, int32_t left, int32_t bottom, int32_t right)
		: m_topLeft(left, top)
		, m_bottomRight(right, bottom)
	{
	}

	inline Rect2i::Rect2i(const Vec2i &topLeft, const Vec2i &bottomRight)
		: m_topLeft(topLeft)
		, m_bottomRight(bottomRight)
	{
	}

	inline Rect2i Rect2i::operator+(const Vec2i &other) const
	{
		return Rect2i(m_topLeft + other, m_bottomRight + other);
	}

	inline Rect2i Rect2i::operator-(const Vec2i &other) const
	{
		return Rect2i(m_topLeft - other, m_bottomRight - other);
	}

	inline Rect2i &Rect2i::operator+=(const Vec2i &other)
	{
		m_topLeft += other;
		m_bottomRight += other;
		return *this;
	}

	inline Rect2i &Rect2i::operator-=(const Vec2i &other)
	{
		m_topLeft -= other;
		m_bottomRight -= other;
		return *this;
	}

	inline int32_t Rect2i::Top() const
	{
		return m_topLeft.m_y;
	}

	inline void Rect2i::SetTop(int32_t i)
	{
		m_topLeft.m_y = i;
	}

	inline int32_t Rect2i::Left() const
	{
		return m_topLeft.m_x;
	}

	inline void Rect2i::SetLeft(int32_t i)
	{
		m_topLeft.m_x = i;
	}

	inline int32_t Rect2i::Bottom() const
	{
		return m_bottomRight.m_y;
	}

	inline void Rect2i::SetBottom(int32_t i)
	{
		m_bottomRight.m_y = i;
	}

	inline int32_t Rect2i::Right() const
	{
		return m_bottomRight.m_x;
	}

	inline void Rect2i::SetRight(int32_t i)
	{
		m_bottomRight.m_x = i;
	}

	inline bool Rect2i::IsValid() const
	{
		return m_bottomRight.m_x >= m_topLeft.m_x && m_bottomRight.m_y >= m_topLeft.m_y;
	}

	inline Rect2i Rect2i::Intersect(const Rect2i &other) const
	{
		const int32_t top = std::max(m_topLeft.m_y, other.m_topLeft.m_y);
		const int32_t left = std::max(m_topLeft.m_x, other.m_topLeft.m_x);
		const int32_t bottom = std::min(m_bottomRight.m_y, other.m_bottomRight.m_y);
		const int32_t right = std::min(m_bottomRight.m_x, other.m_bottomRight.m_x);

		return Rect2i(top, left, bottom, right);
	}

	inline bool Rect2i::Contains(const Vec2i &pt) const
	{
		return pt.m_x >= m_topLeft.m_x && pt.m_x < m_bottomRight.m_x && pt.m_y >= m_topLeft.m_y && pt.m_y < m_bottomRight.m_y;
	}

	inline Rect Rect2i::ToShortRect() const
	{
		return Rect::Create(static_cast<int16_t>(m_topLeft.m_y), static_cast<int16_t>(m_topLeft.m_x), static_cast<int16_t>(m_bottomRight.m_y), static_cast<int16_t>(m_bottomRight.m_x));
	}
}
