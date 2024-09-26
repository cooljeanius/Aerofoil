#pragma once

struct IGpAudioDriver;
struct GpAudioDriverProperties;

class GpAudioDriverFactoryXAudio2
{
public:
	static IGpAudioDriver *Create(const GpAudioDriverProperties &properties);
};
