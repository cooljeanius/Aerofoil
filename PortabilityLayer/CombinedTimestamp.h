#pragma once

#include <stdint.h>

namespace PortabilityLayer
{
	struct CombinedTimestamp
	{
		uint8_t m_macEpochTimestamp[8];

		uint8_t m_localYear[4];
		uint8_t m_localMonth;
		uint8_t m_localDay;

		uint8_t m_localHour;
		uint8_t m_localMinute;
		uint8_t m_localSecond;

		uint8_t m_padding[3];

		static const int32_t kMacEpochToUTC = -2082844800;

		int64_t GetMacEpochTime() const;
		void SetMacEpochTime(int64_t timestamp);

		int32_t GetLocalYear() const;
		void SetLocalYear(int32_t year);

		void GetAsMSDOSTimestamp(uint16_t &msdosDate, uint16_t &msdosTime) const;
	};

	inline int64_t CombinedTimestamp::GetMacEpochTime() const
	{
		int64_t result = 0;
		for (int i = 0; i < 8; i++)
			result |= static_cast<int64_t>(m_macEpochTimestamp[i]) << (i * 8);

		return result;
	}

	inline void CombinedTimestamp::SetMacEpochTime(int64_t timestamp)
	{
		for (int i = 0; i < 8; i++)
			m_macEpochTimestamp[i] = static_cast<uint8_t>((timestamp >> (i * 8)) & 0xff);
	}

	inline int32_t CombinedTimestamp::GetLocalYear() const
	{
		int32_t result = 0;
		for (int i = 0; i < 4; i++)
			result |= static_cast<int32_t>(m_localYear[i]) << (i * 8);

		return result;
	}

	inline void CombinedTimestamp::SetLocalYear(int32_t timestamp)
	{
		for (int i = 0; i < 4; i++)
			m_localYear[i] = static_cast<uint8_t>((timestamp >> (i * 8)) & 0xff);
	}

	inline void CombinedTimestamp::GetAsMSDOSTimestamp(uint16_t &msdosDate, uint16_t &msdosTime) const
	{
		int32_t localYear = this->GetLocalYear();
		uint8_t month = this->m_localMonth;
		uint8_t day = this->m_localDay;

		uint8_t hour = this->m_localHour;
		uint8_t minute = this->m_localMinute;
		uint8_t second = this->m_localSecond;

		int32_t yearsSince1980 = localYear - 1980;

		if (localYear < 1980)
		{
			// Time machine
			yearsSince1980 = 0;
			second = 0;
			minute = 0;
			hour = 0;
			day = 1;
			month = 1;
		}
		else if (localYear > 1980 + 127)
		{
			yearsSince1980 = 127;
			second = 59;
			minute = 59;
			hour = 23;
			day = 31;
			month = 12;
		}

		msdosTime = (second / 2) | (minute << 5) | (hour << 11);
		msdosDate = day | (month << 5) | (yearsSince1980 << 9);
	}
}
