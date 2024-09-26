#pragma once

#include <stdint.h>
#include <stddef.h>

#include "HostSuspendCallID.h"

namespace PortabilityLayer
{
	union HostSuspendCallArgument
	{
		uint32_t m_uint;
		int32_t m_int;
		size_t m_size;
		void *m_pointer;
		const void *m_constPointer;
		void (*m_functionPtr)(void *context);
	};
}
