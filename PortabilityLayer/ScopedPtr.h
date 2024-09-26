#pragma once

#include "CoreDefs.h"

namespace PortabilityLayer
{
	template<class T>
	class ScopedPtr
	{
	public:
		ScopedPtr();
		ScopedPtr(T *ref);
		~ScopedPtr();

		void Swap(ScopedPtr<T> &other);

		operator T*();
		operator const T*() const;
		T *operator->();
		const T *operator->() const;

		void Set(T *ref);

	private:
		ScopedPtr(const ScopedPtr<T> &other) GP_DELETED;
		void operator=(const ScopedPtr<T> &other) GP_DELETED;
		T *m_ref;
	};
}

namespace PortabilityLayer
{
	template<class T>
	inline ScopedPtr<T>::ScopedPtr()
		: m_ref(nullptr)
	{
	}

	template<class T>
	inline ScopedPtr<T>::ScopedPtr(T *ref)
		: m_ref(ref)
	{
	}

	template<class T>
	inline ScopedPtr<T>::~ScopedPtr()
	{
		if (m_ref)
			m_ref->Destroy();
	}

	template<class T>
	inline void ScopedPtr<T>::Swap(ScopedPtr<T> &other)
	{
		T *temp = m_ref;
		m_ref = other.m_ref;
		other.m_ref = temp;
	}

	template<class T>
	inline ScopedPtr<T>::operator T*()
	{
		return m_ref;
	}

	template<class T>
	inline ScopedPtr<T>::operator const T*() const
	{
		return m_ref;
	}

	template<class T>
	inline T *ScopedPtr<T>::operator->()
	{
		return m_ref;
	}

	template<class T>
	inline const T *ScopedPtr<T>::operator->() const
	{
		return m_ref;
	}

	template<class T>
	inline void ScopedPtr<T>::Set(T *ref)
	{
		if (m_ref && m_ref != ref)
			m_ref->Destroy();

		m_ref = ref;
	}
}
