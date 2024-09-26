#pragma once
#ifndef __PL_FILE_PERMISSION_H__
#define __PL_FILE_PERMISSION_H__

namespace PortabilityLayer
{
	enum EFilePermission
	{
		EFilePermission_Any,
		EFilePermission_Read,
		EFilePermission_ReadWrite,
		EFilePermission_Write,
	};
}

#endif
