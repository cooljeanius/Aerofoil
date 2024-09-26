#pragma once

#include <stdint.h>
#include <stddef.h>

#include "DataTypes.h"

namespace PortabilityLayer
{
	uint16_t XModemCRC(const void *bytes, size_t size, uint16_t initialValue);
}
