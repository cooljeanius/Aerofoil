#include "GpDisplayDriverFactoryD3D11.h"
#include "GpDisplayDriverD3D11.h"

IGpDisplayDriver *GpDisplayDriverFactoryD3D11::Create(const GpDisplayDriverProperties &properties)
{
	return GpDisplayDriverD3D11::Create(properties);
}
