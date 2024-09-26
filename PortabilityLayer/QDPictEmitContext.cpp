#include "QDPictEmitContext.h"

namespace PortabilityLayer
{
	bool QDPictBlitSourceType_IsIndexed(QDPictBlitSourceType sourceType)
	{
		switch (sourceType)
		{
		case QDPictBlitSourceType_Indexed1Bit:
		case QDPictBlitSourceType_Indexed2Bit:
		case QDPictBlitSourceType_Indexed4Bit:
		case QDPictBlitSourceType_Indexed8Bit:
			return true;
		default:
			return false;
		}
	}
}
