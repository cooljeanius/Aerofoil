#include "DeflateCodec.h"

#include "GpIOStream.h"
#include "MemoryManager.h"

#ifdef GP_ZLIB_BUILTIN
#include <zlib.h>
#else
#include "zlib.h"
#endif

#include <string.h>

namespace
{
	static voidpf ZlibAllocShim(voidpf opaque, uInt items, uInt size)
	{
		return static_cast<PortabilityLayer::MemoryManager*>(opaque)->Alloc(static_cast<size_t>(items) * size);
	}

	void ZlibFreeShim(voidpf opaque, voidpf address)
	{
		static_cast<PortabilityLayer::MemoryManager*>(opaque)->Release(address);
	}
}

bool PortabilityLayer::DeflateCodec::DecompressStream(GpIOStream *stream, size_t inSize, void *outBuffer, size_t outSize)
{
	z_stream zstream;
	zstream.zalloc = ZlibAllocShim;
	zstream.zfree = ZlibFreeShim;
	zstream.opaque = MemoryManager::GetInstance();

	if (inflateInit2(&zstream, -15) != Z_OK)
		return false;

	const size_t bufferSize = 1024;
	uint8_t buffer[1024];

	zstream.avail_out = outSize;
	zstream.next_out = static_cast<Bytef*>(outBuffer);
	zstream.avail_in = 0;
	zstream.next_in = buffer;

	bool failed = false;
	for (;;)
	{
		if (zstream.avail_in == 0)
		{
			const size_t sizeToRead = (bufferSize < inSize) ? bufferSize : inSize;

			if (sizeToRead == 0)
			{
				// Ran out of input
				failed = true;
				break;
			}

			if (stream->Read(buffer, sizeToRead) != sizeToRead)
			{
				failed = true;
				break;
			}

			zstream.avail_in = sizeToRead;
			zstream.next_in = buffer;
			inSize -= sizeToRead;
		}

		int result = inflate(&zstream, Z_NO_FLUSH);
		if (result == Z_STREAM_END)
		{
			failed = (zstream.avail_out != 0);
			break;
		}

		if (result != Z_OK)
		{
			failed = true;
			break;
		}
	}

	inflateEnd(&zstream);

	return !failed;
}

namespace PortabilityLayer
{
	class DeflateContextImpl final : public DeflateContext
	{
	public:
		DeflateContextImpl(GpIOStream *stream, int compressionLevel);
		~DeflateContextImpl();

		bool Init();

		void Destroy() override;
		bool Append(const void *buffer, size_t size) override;

		bool Flush() override;

	private:
		GpIOStream *m_ioStream;
		z_stream m_zStream;
		int m_compressionLevel;
		bool m_streamInitialized;

		uint8_t m_flushBuffer[1024];
	};

	class InflateContextImpl final : public InflateContext
	{
	public:
		static InflateContext *Create();

		void Destroy() override;

		bool Append(const void *buffer, size_t size, size_t &sizeConsumed) override;
		bool Read(void *buffer, size_t size, size_t &sizeRead) override;

		bool Reset() override;

		bool Init();

	private:
		InflateContextImpl();
		~InflateContextImpl();

		bool m_streamInitialized;
		bool m_isEndOfStream;
		z_stream m_zStream;

		uint8_t m_flushBuffer[1024];
		const uint8_t *m_readPos;
	};
}

PortabilityLayer::DeflateContextImpl::DeflateContextImpl(GpIOStream *stream, int compressionLevel)
	: m_ioStream(stream)
	, m_compressionLevel(compressionLevel)
	, m_streamInitialized(false)
{
	memset(&m_zStream, 0, sizeof(m_zStream));
}

PortabilityLayer::DeflateContextImpl::~DeflateContextImpl()
{
	if (m_streamInitialized)
		deflateEnd(&m_zStream);
}

bool PortabilityLayer::DeflateContextImpl::Init()
{
	m_zStream.zalloc = ZlibAllocShim;
	m_zStream.zfree = ZlibFreeShim;
	m_zStream.opaque = MemoryManager::GetInstance();

	if (deflateInit2(&m_zStream, m_compressionLevel, Z_DEFLATED, -15, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK)
		return false;

	m_streamInitialized = true;
	return true;
}

void PortabilityLayer::DeflateContextImpl::Destroy()
{
	this->~DeflateContextImpl();
	PortabilityLayer::MemoryManager::GetInstance()->Release(this);
}

bool PortabilityLayer::DeflateContextImpl::Append(const void *buffer, size_t size)
{
	m_zStream.avail_out = sizeof(m_flushBuffer);
	m_zStream.next_out = m_flushBuffer;

	m_zStream.next_in = reinterpret_cast<Bytef*>(const_cast<void*>(buffer));
	m_zStream.avail_in = size;

	while (m_zStream.avail_in > 0)
	{
		if (deflate(&m_zStream, Z_NO_FLUSH) != Z_OK)
			return false;

		if (m_zStream.avail_out != sizeof(m_flushBuffer))
		{
			size_t amountOut = sizeof(m_flushBuffer) - m_zStream.avail_out;
			if (m_ioStream->Write(m_flushBuffer, amountOut) != amountOut)
				return false;

			m_zStream.avail_out = sizeof(m_flushBuffer);
			m_zStream.next_out = m_flushBuffer;
		}
	}

	return true;
}

bool PortabilityLayer::DeflateContextImpl::Flush()
{
	for (;;)
	{
		const int deflateCode = deflate(&m_zStream, Z_FINISH);
		if (deflateCode != Z_OK && deflateCode != Z_STREAM_END)
			return false;

		if (m_zStream.avail_out != sizeof(m_flushBuffer))
		{
			size_t amountOut = sizeof(m_flushBuffer) - m_zStream.avail_out;
			if (m_ioStream->Write(m_flushBuffer, amountOut) != amountOut)
				return false;

			m_zStream.avail_out = sizeof(m_flushBuffer);
			m_zStream.next_out = m_flushBuffer;
		}

		if (deflateCode == Z_STREAM_END)
			break;
	}

	return true;
}



PortabilityLayer::InflateContext *PortabilityLayer::InflateContextImpl::Create()
{
	void *storage = PortabilityLayer::MemoryManager::GetInstance()->Alloc(sizeof(PortabilityLayer::InflateContextImpl));
	if (!storage)
		return nullptr;

	InflateContextImpl *obj = new (storage) InflateContextImpl();
	if (!obj->Init())
	{
		obj->Destroy();
		return nullptr;
	}

	return obj;
}

void PortabilityLayer::InflateContextImpl::Destroy()
{
	this->~InflateContextImpl();
	PortabilityLayer::MemoryManager::GetInstance()->Release(this);
}

bool PortabilityLayer::InflateContextImpl::Append(const void *buffer, size_t size, size_t &sizeConsumed)
{
	size_t consumed = 0;

	m_zStream.avail_in = size;
	m_zStream.next_in = static_cast<Bytef*>(const_cast<void*>(buffer));

	for (;;)
	{
		if (m_isEndOfStream)
		{
			m_zStream.avail_in = 0;
			m_zStream.next_in = nullptr;
		}

		if (m_zStream.avail_in == 0 || m_zStream.avail_out == 0)
		{
			sizeConsumed = consumed;
			return true;
		}

		size_t lastAvailIn = m_zStream.avail_in;

		int result = inflate(&m_zStream, Z_NO_FLUSH);
		if (result == Z_STREAM_END)
			m_isEndOfStream = true;
		else if (result != Z_OK)
			return false;

		consumed += lastAvailIn - m_zStream.avail_in;
	}
}

bool PortabilityLayer::InflateContextImpl::Read(void *buffer, size_t size, size_t &sizeRead)
{
	size_t amountInOutputBuffer = static_cast<const uint8_t*>(m_zStream.next_out) - m_readPos;

	if (size > amountInOutputBuffer)
		size = amountInOutputBuffer;

	if (size > 0)
	{
		if (buffer)
			memcpy(buffer, m_readPos, size);

		m_readPos += size;

		if (m_readPos == m_zStream.next_out)
		{
			m_zStream.avail_out = sizeof(m_flushBuffer);
			m_zStream.next_out = m_flushBuffer;
			m_readPos = m_flushBuffer;
		}
	}

	sizeRead = size;
	return true;
}

bool PortabilityLayer::InflateContextImpl::Reset()
{
	if (inflateReset2(&m_zStream, -15) != Z_OK)
		return false;

	m_isEndOfStream = false;
	m_zStream.avail_out = sizeof(m_flushBuffer);
	m_zStream.next_out = m_flushBuffer;
	m_readPos = m_flushBuffer;

	return true;
}

bool PortabilityLayer::InflateContextImpl::Init()
{
	m_zStream.zalloc = ZlibAllocShim;
	m_zStream.zfree = ZlibFreeShim;
	m_zStream.opaque = MemoryManager::GetInstance();

	if (inflateInit2(&m_zStream, -15) != Z_OK)
		return false;

	m_zStream.next_out = m_flushBuffer;
	m_zStream.avail_out = sizeof(m_flushBuffer);

	m_streamInitialized = true;
	return true;
}

PortabilityLayer::InflateContextImpl::InflateContextImpl()
	: m_streamInitialized(false)
	, m_isEndOfStream(false)
	, m_readPos(m_flushBuffer)
{
	memset(&m_zStream, 0, sizeof(m_zStream));
}

PortabilityLayer::InflateContextImpl::~InflateContextImpl()
{
	if (m_streamInitialized)
		inflateEnd(&m_zStream);
}

PortabilityLayer::DeflateContext *PortabilityLayer::DeflateContext::Create(GpIOStream *stream, int compressionLevel)
{
	void *storage = PortabilityLayer::MemoryManager::GetInstance()->Alloc(sizeof(PortabilityLayer::DeflateContextImpl));
	if (!storage)
		return nullptr;

	DeflateContextImpl *obj = new (storage) DeflateContextImpl(stream, compressionLevel);
	if (!obj->Init())
	{
		obj->Destroy();
		return nullptr;
	}

	return obj;
}

uint32_t PortabilityLayer::DeflateContext::CRC32(uint32_t inputValue, const void *buffer, size_t bufferLength)
{
	return crc32(inputValue, static_cast<const Bytef*>(buffer), bufferLength);
}


PortabilityLayer::InflateContext *PortabilityLayer::InflateContext::Create()
{
	return InflateContextImpl::Create();
}
