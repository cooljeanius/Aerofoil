#pragma once

#include <stdint.h>

struct MaceChannelDecState
{
	int16_t index, factor, prev2, previous, level;
};

void DecodeMACE3(MaceChannelDecState *chd, uint8_t pkt, uint8_t *output);
void DecodeMACE6(MaceChannelDecState *chd, uint8_t pkt, uint8_t *output);
