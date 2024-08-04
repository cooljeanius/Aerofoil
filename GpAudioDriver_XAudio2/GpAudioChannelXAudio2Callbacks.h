#pragma once

#include <xaudio2.h>

class GpAudioChannelXAudio2;

class GpAudioChannelXAudio2Callbacks final : public IXAudio2VoiceCallback
{
public:
	explicit GpAudioChannelXAudio2Callbacks(GpAudioChannelXAudio2 *owner);

	void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 BytesRequired) override;
	void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override;
	void STDMETHODCALLTYPE OnStreamEnd() override;
	void STDMETHODCALLTYPE OnBufferStart(void* pBufferContext) override;
	void STDMETHODCALLTYPE OnBufferEnd(void* pBufferContext) override;
	void STDMETHODCALLTYPE OnLoopEnd(void* pBufferContext) override;
	void STDMETHODCALLTYPE OnVoiceError(void* pBufferContext, HRESULT Error) override;

private:
	GpAudioChannelXAudio2 *m_owner;
};
