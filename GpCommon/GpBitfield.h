#pragma once

template<unsigned int TSize>
class GpBitfield
{
public:
	GpBitfield();
	GpBitfield(const GpBitfield<TSize> &other);

	bool operator==(const GpBitfield<TSize> &other) const;
	bool operator!=(const GpBitfield<TSize> &other) const;

	void Set(unsigned int index, bool v);
	bool Get(unsigned int index) const;

private:
	static const unsigned int TSizeBytes = (TSize + 7) / 8;
	uint8_t m_bytes[TSizeBytes];
};

template<unsigned int TSize>
inline GpBitfield<TSize>::GpBitfield()
{
	for (unsigned int i = 0; i < TSizeBytes; i++)
		m_bytes[i] = 0;
}

template<unsigned int TSize>
inline GpBitfield<TSize>::GpBitfield(const GpBitfield<TSize> &other)
	: m_bytes(other.m_bytes)
{
}

template<unsigned int TSize>
inline bool GpBitfield<TSize>::operator==(const GpBitfield<TSize> &other) const
{
	for (unsigned int i = 0; i < TSizeBytes; i++)
		if (m_bytes[i] != other.m_bytes[i])
			return false;

	return true;
}

template<unsigned int TSize>
inline bool GpBitfield<TSize>::operator!=(const GpBitfield<TSize> &other) const
{
	return !((*this) == other);
}

template<unsigned int TSize>
void GpBitfield<TSize>::Set(unsigned int index, bool v)
{
	if (v)
		m_bytes[index / 8] |= (1 << (index & 7));
	else
		m_bytes[index / 8] &= ~(1 << (index & 7));
}

template<unsigned int TSize>
bool GpBitfield<TSize>::Get(unsigned int index) const
{
	return (m_bytes[index / 8] & (1 << (index & 7))) != 0;
}
