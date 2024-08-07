#include "GpSystemServices_Web.h"

#include "IGpClipboardContents.h"
#include "IGpThreadEvent.h"
#include "SDL2/SDL.h"

#include <time.h>
#include <unistd.h>
#include <string>

class GpMutex_Web_Null final : public IGpMutex
{
public:
	void Destroy() override;

	void Lock() override;
	void Unlock() override;
	
	static IGpMutex *GetInstance();

private:
	static GpMutex_Web_Null ms_instance;
};

void GpMutex_Web_Null::Destroy()
{
}

void GpMutex_Web_Null::Lock()
{
}

void GpMutex_Web_Null::Unlock()
{
}

IGpMutex *GpMutex_Web_Null::GetInstance()
{
	return &ms_instance;
}

GpMutex_Web_Null GpMutex_Web_Null::ms_instance;


class GpThreadEvent_Web_Null final : public IGpThreadEvent
{
public:
	void Wait() override;
	bool WaitTimed(uint32_t msec) override;
	void Signal() override;
	void Destroy() override;

	static IGpThreadEvent *GetInstance();

private:
	static GpThreadEvent_Web_Null ms_instance;
};

GpThreadEvent_Web_Null GpThreadEvent_Web_Null::ms_instance;


void GpThreadEvent_Web_Null::Wait()
{
}

bool GpThreadEvent_Web_Null::WaitTimed(uint32_t msec)
{
	return true;
}

void GpThreadEvent_Web_Null::Signal()
{
}

void GpThreadEvent_Web_Null::Destroy()
{
}

IGpThreadEvent *GpThreadEvent_Web_Null::GetInstance()
{
	return &ms_instance;
}


GpSystemServices_Web::GpSystemServices_Web()
	: m_textInputEnabled(false)
	, m_clipboardContents(nullptr)
{
}

int64_t GpSystemServices_Web::GetTime() const
{
	time_t t = time(nullptr);
	return static_cast<int64_t>(t) + 2082844800;
}

void GpSystemServices_Web::GetLocalDateTime(unsigned int &year, unsigned int &month, unsigned int &day, unsigned int &hour, unsigned int &minute, unsigned int &second) const
{
	time_t t = time(nullptr);
	tm *tmObject = localtime(&t);
	year = static_cast<unsigned int>(tmObject->tm_year);
	month = static_cast<unsigned int>(tmObject->tm_mon + 1);
	hour = static_cast<unsigned int>(tmObject->tm_hour);
	minute = static_cast<unsigned int>(tmObject->tm_min);
	second = static_cast<unsigned int>(tmObject->tm_sec);
}

IGpMutex *GpSystemServices_Web::CreateMutex()
{
	return GpMutex_Web_Null::GetInstance();
}

IGpMutex *GpSystemServices_Web::CreateRecursiveMutex()
{
	return GpMutex_Web_Null::GetInstance();
}

IGpThreadEvent *GpSystemServices_Web::CreateThreadEvent(bool autoReset, bool startSignaled)
{
	return GpThreadEvent_Web_Null::GetInstance();
}

uint64_t GpSystemServices_Web::GetFreeMemoryCosmetic() const
{
	return 0;
}

GpSystemServices_Web::~GpSystemServices_Web()
{
	if (m_clipboardContents)
		m_clipboardContents->Destroy();
}

void *GpSystemServices_Web::CreateThread(ThreadFunc_t threadFunc, void *context)
{
	return nullptr;
}

bool GpSystemServices_Web::Beep() const
{
	return false;
}

bool GpSystemServices_Web::IsTouchscreen() const
{
	return false;
}

bool GpSystemServices_Web::IsUsingMouseAsTouch() const
{
	return false;
}

bool GpSystemServices_Web::IsTextInputObstructive() const
{
	return false;
}

bool GpSystemServices_Web::IsFullscreenPreferred() const
{
	return false;
}

bool GpSystemServices_Web::IsFullscreenOnStartup() const
{
	return false;
}

bool GpSystemServices_Web::HasNativeFileManager() const
{
	return false;
}

GpOperatingSystem_t GpSystemServices_Web::GetOperatingSystem() const
{
	return GpOperatingSystems::kWeb;
}

GpOperatingSystemFlavor_t GpSystemServices_Web::GetOperatingSystemFlavor() const
{
	return GpOperatingSystemFlavors::kGeneric;
}

unsigned int GpSystemServices_Web::GetCPUCount() const
{
	return SDL_GetCPUCount();
}

void GpSystemServices_Web::SetTextInputEnabled(bool isEnabled)
{
	m_textInputEnabled = isEnabled;
}

bool GpSystemServices_Web::IsTextInputEnabled() const
{
	return m_textInputEnabled;
}

bool GpSystemServices_Web::AreFontResourcesSeekable() const
{
	return true;
}

IGpClipboardContents *GpSystemServices_Web::GetClipboardContents() const
{
	return m_clipboardContents;
}

void GpSystemServices_Web::SetClipboardContents(IGpClipboardContents *contents)
{
	if (contents != m_clipboardContents)
	{
		if (m_clipboardContents)
			m_clipboardContents->Destroy();

		m_clipboardContents = contents;
	}
}

GpSystemServices_Web *GpSystemServices_Web::GetInstance()
{
	return &ms_instance;
}

GpSystemServices_Web GpSystemServices_Web::ms_instance;
