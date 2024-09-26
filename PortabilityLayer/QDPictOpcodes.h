#pragma once

namespace PortabilityLayer
{
	namespace QDOpcodes
	{
		enum Value
		{
#define PL_PICTOP(number, opcode, sizeRule) opcode = number,
#include "QDPictOpcodeDefs.h"
#undef PL_PICTOP
		};
	}
}
