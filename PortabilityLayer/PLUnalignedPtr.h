#pragma once

namespace PortabilityLayer
{
	template<class T>
	class UnalignedPtr
	{
	public:
		UnalignedPtr();
		explicit UnalignedPtr(const T *ref);

		const T *GetRawPtr() const;
		T Get() const;

	private:
		const T *m_ref;
	};

	template<class T>
	UnalignedPtr<T>::UnalignedPtr()
		: m_ref(nullptr)
	{
	}

	template<class T>
	UnalignedPtr<T>::UnalignedPtr(const T *ref)
		: m_ref(ref)
	{
	}
}

#include <string.h>

namespace PortabilityLayer
{
	template<class T>
	const T *UnalignedPtr<T>::GetRawPtr() const
	{
		return m_ref;
	}

	template<class T>
	T UnalignedPtr<T>::Get() const
	{
		T result;
		memcpy(&result, m_ref, sizeof(T));
		return result;
	}
}
