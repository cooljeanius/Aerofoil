#pragma once

#include <stdint.h>

struct IFileReader
{
public:
	typedef int64_t FilePos_t;
	typedef uint64_t UFilePos_t;

	virtual size_t Read(void *buffer, size_t sz) = 0;
	virtual size_t FileSize() const = 0;
	virtual bool SeekStart(FilePos_t pos) = 0;
	virtual bool SeekCurrent(FilePos_t pos) = 0;
	virtual bool SeekEnd(FilePos_t pos) = 0;
	virtual FilePos_t GetPosition() const = 0;

	inline bool ReadExact(void *buffer, size_t sz)
	{
		return this->Read(buffer, sz) == sz;
	}
};
