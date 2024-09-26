#pragma once

namespace PortabilityLayer
{
	class RefCountedBase
	{
	public:
		RefCountedBase();
		virtual ~RefCountedBase();
		virtual void Release() = 0;

		void IncRef();
		void DecRef();

	private:
		unsigned int m_refCount;
	};
}

inline PortabilityLayer::RefCountedBase::RefCountedBase()
	: m_refCount(0)
{
}

inline PortabilityLayer::RefCountedBase::~RefCountedBase()
{
}

inline void PortabilityLayer::RefCountedBase::IncRef()
{
	m_refCount++;
}

inline void PortabilityLayer::RefCountedBase::DecRef()
{
	if (--m_refCount == 0)
		this->Release();
}
