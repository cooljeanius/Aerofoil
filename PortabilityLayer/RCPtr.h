#pragma once

#include "CoreDefs.h"

namespace PortabilityLayer
{
	class RefCountedBase;

	template<class T>
	class RCPtr
	{
	public:
		RCPtr();
		RCPtr(T *other);
		RCPtr(const RCPtr<T> &other);
#if IS_CPP11
		RCPtr(RCPtr<T> &&other);
#endif
		~RCPtr();

		operator T*() const;
		T* operator ->() const;

		RCPtr<T> &operator=(T *other);
#if IS_CPP11
		RCPtr<T> &operator=(RCPtr<T> &&other);
#endif

		T *Get() const;

	private:
		RefCountedBase *m_target;
	};
}

#include "RefCounted.h"

template<class T>
inline PortabilityLayer::RCPtr<T>::RCPtr()
	: m_target(nullptr)
{
}

template<class T>
inline PortabilityLayer::RCPtr<T>::RCPtr(const RCPtr<T> &other)
	: m_target(other.m_target)
{
	if (m_target)
		m_target->IncRef();
}

template<class T>
inline PortabilityLayer::RCPtr<T>::RCPtr(T *other)
	: m_target(other)
{
	if (other)
		other->IncRef();
}

#if IS_CPP11
template<class T>
inline PortabilityLayer::RCPtr<T>::RCPtr(RCPtr<T> &&other)
	: m_target(other.m_target)
{
	other.m_target = nullptr;
}
#endif

template<class T>
inline PortabilityLayer::RCPtr<T>::~RCPtr()
{
	if (m_target)
		m_target->DecRef();
}

template<class T>
inline T *PortabilityLayer::RCPtr<T>::operator T*() const
{
	return m_target;
}

template<class T>
inline T *PortabilityLayer::RCPtr<T>::operator->() const
{
	return m_target;
}

template<class T>
inline PortabilityLayer::RCPtr<T>::RCPtr<T> &PortabilityLayer::RCPtr<T>::operator=(T *other)
{
	RefCountedBase *old = m_target;

	m_target = other;
	if (other)
		other->IncRef();

	if (old)
		old->DecRef();

	return *this;
}

#if IS_CPP11
template<class T>
inline PortabilityLayer::RCPtr<T>& PortabilityLayer::RCPtr<T>::RCPtr<T> &operator=(RCPtr<TOther> &&other)
{
	RefCountedBase *old = m_target;
	RefCountedBase *newRC = other.m_target;

	other.m_target = nullptr;

	m_target = newRC;
	if (newRC)
		newRC->IncRef();

	if (old)
		old->DecRef();

	return *this;
}
#endif
