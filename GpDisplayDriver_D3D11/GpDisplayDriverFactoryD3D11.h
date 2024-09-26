#pragma once

struct IGpDisplayDriver;
struct GpDisplayDriverProperties;

class GpDisplayDriverFactoryD3D11
{
public:
	static IGpDisplayDriver *Create(const GpDisplayDriverProperties &properties);
};
