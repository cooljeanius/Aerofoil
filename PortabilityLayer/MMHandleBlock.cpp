#include "MMHandleBlock.h"

namespace PortabilityLayer
{
	MMHandleBlock::MMHandleBlock(void *contents, size_t size)
		: m_contents(contents)
		, m_rmSelfRef(nullptr)
		, m_size(size)
	{
	}

	void **MMHandleBlock::AsHandle()
	{
		return &m_contents;
	}

}
