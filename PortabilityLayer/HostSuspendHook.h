#pragma once

#include "HostSuspendCallID.h"

namespace PortabilityLayer
{
	union HostSuspendCallArgument;

	typedef void(*HostSuspendHook_t)(void *context, HostSuspendCallID callID, const HostSuspendCallArgument *args, HostSuspendCallArgument *returnValue);

	void RenderFrames(unsigned int ticks);
}
