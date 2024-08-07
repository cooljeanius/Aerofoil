#ifdef _WIN32
#include <Windows.h>

#include <cstdint>
#else
#include <cstring>
#include <ctime>
#endif

#include <cstdio>

#include "CombinedTimestamp.h"

int main(int argc, const char **argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: MakeTimestamp <output.ts>\n");
		fprintf(stderr, "Outputs the current timestamp in 1904 UTC epoch format");
		return -1;
	}

#ifdef _WIN32
	SYSTEMTIME epochStart;
	epochStart.wYear = 1904;
	epochStart.wMonth = 1;
	epochStart.wDayOfWeek = 5;
	epochStart.wDay = 1;
	epochStart.wHour = 0;
	epochStart.wMinute = 0;
	epochStart.wSecond = 0;
	epochStart.wMilliseconds = 0;

	FILETIME epochStartFT;
	if (!SystemTimeToFileTime(&epochStart, &epochStartFT))
		return 0;

	FILETIME timestampFT;
	GetSystemTimeAsFileTime(&timestampFT);

	int64_t epochStart64 = (static_cast<int64_t>(epochStartFT.dwLowDateTime) & 0xffffffff) | (static_cast<int64_t>(epochStartFT.dwHighDateTime) << 32);
	int64_t currentTime64 = (static_cast<int64_t>(timestampFT.dwLowDateTime) & 0xffffffff) | (static_cast<int64_t>(timestampFT.dwHighDateTime) << 32);
	int64_t timeDelta = (currentTime64 - epochStart64) / 10000000;

	TIME_ZONE_INFORMATION tz;
	GetTimeZoneInformation(&tz);

	SYSTEMTIME utcST;
	FileTimeToSystemTime(&timestampFT, &utcST);

	SYSTEMTIME localST;
	SystemTimeToTzSpecificLocalTime(&tz, &utcST, &localST);

	PortabilityLayer::CombinedTimestamp ts;
	ts.SetMacEpochTime(timeDelta);

	ts.SetLocalYear(localST.wYear);
	ts.m_localMonth = localST.wMonth;
	ts.m_localDay = localST.wDay;

	ts.m_localHour = localST.wHour;
	ts.m_localMinute = localST.wMinute;
	ts.m_localSecond = localST.wSecond;
#else
	time_t currentTimeUnix = time(nullptr);

	tm *currentTimeStruct = localtime(&currentTimeUnix);
	if (currentTimeStruct == nullptr) {
		fprintf(stderr, "Error converting system time to calendar format");
		return -1;
	}

	PortabilityLayer::CombinedTimestamp ts;
	ts.SetMacEpochTime(static_cast<int64_t>(currentTimeUnix) - ts.kMacEpochToUTC);

	ts.SetLocalYear(currentTimeStruct->tm_year + 1900);
	ts.m_localMonth = currentTimeStruct->tm_mon + 1;
	ts.m_localDay = currentTimeStruct->tm_mday;

	ts.m_localHour = currentTimeStruct->tm_hour;
	ts.m_localMinute = currentTimeStruct->tm_min;
	ts.m_localSecond = currentTimeStruct->tm_sec;
#endif

	memset(ts.m_padding, 0, sizeof(ts.m_padding));

	FILE *f = fopen(argv[1], "wb");
	if (!f)
	{
		perror("Error opening output file");
		return -1;
	}

	fwrite(&ts, sizeof(ts), 1, f);

	fclose(f);

	return 0;
}
