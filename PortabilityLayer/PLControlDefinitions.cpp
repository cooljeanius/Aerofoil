#include "PLControlDefinitions.h"


int FindControl(Point point, WindowPtr window, ControlHandle *outControl)
{
	PL_NotYetImplemented();
	return 0;
}

void SetControlValue(ControlHandle control, int value)
{
	PL_NotYetImplemented();
}

void SetControlMaximum(ControlHandle control, int value)
{
	PL_NotYetImplemented();
}

void MoveControl(ControlHandle control, int x, int y)
{
	PL_NotYetImplemented();
}

void SizeControl(ControlHandle control, int width, int height)
{
	PL_NotYetImplemented();
}

int GetControlValue(ControlHandle control)
{
	PL_NotYetImplemented();
	return 0;
}

ControlHandle NewControl(WindowPtr window, const Rect *rect, const PLPasStr &label, Boolean visible, int value, int minValue, int maxValue, int cdef, long userdata)
{
	PL_NotYetImplemented();
	return nullptr;
}

ControlActionUPP NewControlActionUPP(ControlActionProc proc)
{
	return proc;
}

void DisposeControlActionUPP(ControlActionUPP upp)
{
}

Boolean TrackControl(ControlHandle control, Point point, ControlActionUPP proc)
{
	PL_NotYetImplemented();
	return false;
}

long GetControlReference(ControlHandle control)
{
	PL_NotYetImplemented();
	return 0;
}

ControlHandle GetNewControl(int resID, WindowPtr window)
{
	PL_NotYetImplemented();
	return nullptr;
}

void HiliteControl(ControlHandle control, int unknown)
{
	PL_NotYetImplemented();
}