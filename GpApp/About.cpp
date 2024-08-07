//============================================================================
//----------------------------------------------------------------------------
//									About.c
//----------------------------------------------------------------------------
//============================================================================

#include "About.h"
#include "DialogManager.h"
#include "DialogUtils.h"
#include "Environ.h"
#include "Externs.h"
#include "GpApplicationName.h"
#include "GpBuildVersion.h"
#include "IGpSystemServices.h"
#include "RenderedFont.h"
#include "GpRenderedFontMetrics.h"
#include "ResolveCachingColor.h"
#include "ResourceManager.h"
#include "ScanlineMask.h"
#include "WindowDef.h"
#include "WindowManager.h"

#include "PLArrayView.h"
#include "PLDrivers.h"
#include "PLKeyEncoding.h"
#include "PLControlDefinitions.h"
#include "FontFamily.h"
#include "PLButtonWidget.h"
#include "PLLabelWidget.h"
#include "PLNumberFormatting.h"
#include "PLResources.h"
#include "PLSound.h"
#include "PLPasStr.h"
#include "PLScrollBarWidget.h"
#include "PLStandardColors.h"
#include "PLSysCalls.h"
#include "PLTimeTaggedVOSEvent.h"
#include "PLWidgets.h"


static void HiLiteOkayButton (DrawSurface *surface);
static void UnHiLiteOkayButton (DrawSurface *surface);
static void UpdateMainPict (Dialog *);
static int16_t AboutFilter(void *context, Dialog *, const TimeTaggedVOSEvent *evt);
static int16_t AboutFrameworkFilter(void *context, Dialog *, const TimeTaggedVOSEvent *evt);
static int16_t LicenseReaderFilter(void *context, Dialog *, const TimeTaggedVOSEvent *evt);
static void DrawLicenseReader(Window *window);
void DoLicenseReader(int licenseResID);


static Point			okayButtLowerV, okayButtUpperV;
static Rect				okayButtonBounds, mainPICTBounds;
static Boolean			okayButtIsHiLit, clickedDownInOkay;


struct LicenseReaderLine
{
	PortabilityLayer::PascalStr<80> m_text;
};

static const unsigned int kLicenseReaderPointHistoryLength = 3;
static Boolean			licenseReaderIsScrolling;
static Point			licenseReaderPointHistory[kLicenseReaderPointHistoryLength];

static int32_t licenseReaderVelocity;
static unsigned int licenseReaderTextScroll;
static unsigned int licenseReaderTextScrollMax;
static unsigned int licenseReaderNumLines;
static THandle<LicenseReaderLine> licenseReaderLines;
static const unsigned int kLicenseReaderTextSpacing = 10;
static const Rect licenseReaderTextRect = Rect::Create(16, 16, 344, 496);
static PortabilityLayer::ScrollBarWidget *licenseReaderScrollBarWidget;

static void InitLicenseReader(unsigned int numLines)
{
	for (unsigned int i = 0; i < kLicenseReaderPointHistoryLength; i++)
		licenseReaderPointHistory[i] = Point::Create(0, 0);

	licenseReaderIsScrolling = false;

	licenseReaderTextScroll = 0;
	licenseReaderVelocity = 0;
	licenseReaderTextScrollMax = numLines * kLicenseReaderTextSpacing;
	if (licenseReaderTextScrollMax < licenseReaderTextRect.Height())
		licenseReaderTextScrollMax = 0;
	else
		licenseReaderTextScrollMax -= licenseReaderTextRect.Height();

	licenseReaderNumLines = numLines;
	licenseReaderScrollBarWidget = nullptr;
}



//==============================================================  Functions
//--------------------------------------------------------------  DoAbout
// Brings up the About dialog box.

void DoAbout (void)
{
	#define			kAboutDialogID			150		// res ID of About dialog
	#define			kTextItemVers			2		// item number of version text
	#define			kPictItemMain			4		// item number of main PICT
	
	Dialog			*aboutDialog;
	Str255			longVersion;
	StringPtr		messagePtr;
	VersRecHndl		version;
	ControlHandle	itemHandle;
	short			hit;
	
	aboutDialog = PortabilityLayer::DialogManager::GetInstance()->LoadDialog(kAboutDialogID, PL_GetPutInFrontWindowPtr(), nullptr);
//	if (aboutDialog == nil)
//		RedAlert(kErrDialogDidntLoad);
	
	version = PortabilityLayer::ResourceManager::GetInstance()->GetAppResource('vers', 1).StaticCast<VersionRecord>();
	if (version != nil)
	{
		messagePtr = (**version).shortVersion + 1 + (**version).shortVersion[0];

		memcpy(&longVersion, messagePtr, ((UInt8)*messagePtr) + 1);
		SetDialogString(aboutDialog, kTextItemVers, longVersion);
	}

	okayButtonBounds = aboutDialog->GetItems()[kOkayButton - 1].GetWidget()->GetRect();

	okayButtUpperV = Point::Create(okayButtonBounds.left + 45, okayButtonBounds.top + 1);
	okayButtLowerV = Point::Create(okayButtUpperV.h - 28, okayButtUpperV.v + 60);

	okayButtIsHiLit = false;				// Initially, button is not hilit
	clickedDownInOkay = false;				// Initially, didn't click in okay button

	mainPICTBounds = aboutDialog->GetItems()[kPictItemMain - 1].GetWidget()->GetRect();

	UpdateMainPict(aboutDialog);
	
	do										// Loop until user wants to exit
	{
		hit = aboutDialog->ExecuteModal(nullptr, PL_FILTER_FUNC(AboutFilter));
	}
	while (hit != kOkayButton);

	aboutDialog->Destroy();
}

void DoLicenseReader(int resID)
{
	static const int kLicenseReaderDialogTemplateID = 2006;

	THandle<uint8_t> licenseTextHandle = PortabilityLayer::ResourceManager::GetInstance()->GetAppResource('LICS', resID).StaticCast<uint8_t>();

	if (!licenseTextHandle)
		return;

	// Count lines
	unsigned int numLines = 1;
	const uint8_t *licenseText = *licenseTextHandle;
	const size_t licenseTextLength = licenseTextHandle.MMBlock()->m_size;

	for (size_t i = 0; i < licenseTextLength; i++)
	{
		if (licenseText[i] == '\n')
			numLines++;
	}

	licenseReaderLines = NewHandle(sizeof(LicenseReaderLine) * numLines).StaticCast<LicenseReaderLine>();
	if (!licenseReaderLines)
	{
		licenseTextHandle.Dispose();
		return;
	}

	LicenseReaderLine *lines = *licenseReaderLines;
	unsigned int lineIndex = 0;
	size_t lineStart = 0;
	for (size_t i = 0; i < licenseTextLength; i++)
	{
		if (licenseText[i] == '\n')
		{
			assert(i - lineStart <= 80);
			lines[lineIndex++].m_text.Set(i - lineStart, reinterpret_cast<const char*>(licenseText + lineStart));
			lineStart = i + 1;
		}
	}
	assert(licenseTextLength - lineStart <= 80);
	lines[lineIndex++].m_text.Set(licenseTextLength - lineStart, reinterpret_cast<const char*>(licenseText + lineStart));

	assert(lineIndex == numLines);

	licenseTextHandle.Dispose();

	const Rect windowRect = Rect::Create(0, 0, 396, 528);

	PortabilityLayer::WindowDef wdef = PortabilityLayer::WindowDef::Create(windowRect, PortabilityLayer::WindowStyleFlags::kAlert, true, 0, 0, PSTR(""));

	PortabilityLayer::DialogManager *dialogManager = PortabilityLayer::DialogManager::GetInstance();
	Dialog *dialog = dialogManager->LoadDialogFromTemplate(kLicenseReaderDialogTemplateID, windowRect, true, false, 0, 0, PL_GetPutInFrontWindowPtr(), PSTR(""), nullptr);

	DrawDefaultButton(dialog);

	int16_t hit = 0;

	InitLicenseReader(numLines);
	DrawLicenseReader(dialog->GetWindow());

	if (licenseReaderTextScrollMax != 0)
	{
		PortabilityLayer::WidgetBasicState state;
		state.m_rect = Rect::Create(licenseReaderTextRect.top, licenseReaderTextRect.right, licenseReaderTextRect.bottom, licenseReaderTextRect.right + 16);
		state.m_refConstant = 2;
		state.m_window = dialog->GetWindow();
		state.m_max = licenseReaderTextScrollMax;
		state.m_state = 0;
		licenseReaderScrollBarWidget = PortabilityLayer::ScrollBarWidget::Create(state, nullptr);
	}

	dialog->GetWindow()->DrawControls();

	do
	{
		hit = dialog->ExecuteModal(nullptr, PL_FILTER_FUNC(LicenseReaderFilter));
	} while (hit != kOkayButton);

	dialog->Destroy();
	licenseReaderLines.Dispose();
}

void DoAboutOpenSource(void)
{
	static const int kExportSourceItem = 2;
	static const int kAerofoilLicenseButton = 4;
	static const int kGliderPROLicenseButton = 6;
	static const int kOpenSansLicenseButton = 8;
	static const int kRobotoMonoLicenseButton = 10;
	static const int kGochiHandLicenseButton = 12;
	static const int kInterLicenseButton = 14;
	static const int kLibIConvLicenseButton = 16;
	static const int kRapidJSONLicenseButton = 18;
	static const int kZLibLicenseButton = 20;
	static const int kFreeTypeLicenseButton = 22;
	static const int kSDLLicenseButton = 24;

	static const int kLicenseResourceApache = 1000;
	static const int kLicenseResourceGPLv2 = 1001;
	static const int kLicenseResourceLGPLv3 = 1002;
	static const int kLicenseResourceOFL = 1003;
	static const int kLicenseResourceRapidJSON = 1004;
	static const int kLicenseResourceZLib = 1005;
	static const int kLicenseResourceSDL = 1006;

	static const int kAboutOpenSourceDialogTemplateID = 2005;

	const Rect windowRect = Rect::Create(0, 0, 348, 512);

	PortabilityLayer::WindowDef wdef = PortabilityLayer::WindowDef::Create(windowRect, PortabilityLayer::WindowStyleFlags::kAlert, true, 0, 0, PSTR(""));

	PortabilityLayer::DialogManager *dialogManager = PortabilityLayer::DialogManager::GetInstance();
	Dialog *dialog = dialogManager->LoadDialogFromTemplate(kAboutOpenSourceDialogTemplateID, windowRect, true, false, 0, 0, PL_GetPutInFrontWindowPtr(), PSTR(""), nullptr);

	DrawDefaultButton(dialog);

	if (thisMac.isTouchscreen)
		dialog->GetItems()[kExportSourceItem - 1].GetWidget()->SetEnabled(true);

	int16_t hit = 0;

	do
	{
		hit = dialog->ExecuteModal(nullptr, PL_FILTER_FUNC(AboutFrameworkFilter));

		switch (hit)
		{
		case kExportSourceItem:
			DoExportSourceCode();
			break;
		case kAerofoilLicenseButton:
		case kGliderPROLicenseButton:
			DoLicenseReader(kLicenseResourceGPLv2);
			break;
		case kOpenSansLicenseButton:
		case kRobotoMonoLicenseButton:
			DoLicenseReader(kLicenseResourceApache);
			break;
		case kGochiHandLicenseButton:
		case kInterLicenseButton:
			DoLicenseReader(kLicenseResourceOFL);
			break;
		case kLibIConvLicenseButton:
		case kFreeTypeLicenseButton:
			DoLicenseReader(kLicenseResourceLGPLv3);
			break;
		case kZLibLicenseButton:
			DoLicenseReader(kLicenseResourceZLib);
			break;
		case kSDLLicenseButton:
			DoLicenseReader(kLicenseResourceSDL);
			break;
		case kRapidJSONLicenseButton:
			DoLicenseReader(kLicenseResourceRapidJSON);
			break;
		default:
			break;
		}
	} while (hit != kOkayButton);

	dialog->Destroy();
}

void DoAboutFramework (void)
{
	static const int kAboutFrameworkDialogTemplateID = 2000;
	static const int kAboutOpenSourceButton = 2;

	const Rect windowRect = Rect::Create(0, 0, 332, 450);

	PortabilityLayer::WindowDef wdef = PortabilityLayer::WindowDef::Create(windowRect, PortabilityLayer::WindowStyleFlags::kAlert, true, 0, 0, PSTR(""));

	PortabilityLayer::ResolveCachingColor blackColor = StdColors::Black();
	PortabilityLayer::RenderedFont *font = GetFont(PortabilityLayer::FontPresets::kApplication12Bold);
	PortabilityLayer::RenderedFont *fontLight = GetFont(PortabilityLayer::FontPresets::kApplication8);

	int16_t verticalPoint = 16 + font->GetMetrics().m_ascent;
	int16_t horizontalOffset = 16;
	int16_t creditsHorizontalOffset = 80;
	const int16_t spacing = 12;

	PortabilityLayer::DialogManager *dialogManager = PortabilityLayer::DialogManager::GetInstance();
	Dialog *dialog = dialogManager->LoadDialogFromTemplate(kAboutFrameworkDialogTemplateID, windowRect, true, false, 0, 0, PL_GetPutInFrontWindowPtr(), PSTR(""), nullptr);

#if !GP_DEBUG_CONFIG
	#define ABOUT_DIALOG_CONFIGURATION_TAG "Release"
#else
	#define ABOUT_DIALOG_CONFIGURATION_TAG "Debug"
#endif

	Window *window = dialog->GetWindow();

	DrawSurface *surface = window->GetDrawSurface();

	int lineNum = 0;
	surface->DrawString(Point::Create(horizontalOffset, verticalPoint + spacing * (lineNum++)), PSTR(GP_APPLICATION_NAME " " GP_APPLICATION_VERSION_STRING "   \xa9" GP_APPLICATION_COPYRIGHT_STRING), blackColor, font);
	(lineNum++);
	surface->DrawString(Point::Create(horizontalOffset, verticalPoint + spacing * (lineNum)), PSTR("Credits:"), blackColor, font);
	surface->DrawString(Point::Create(creditsHorizontalOffset, verticalPoint + spacing * (lineNum++)), PSTR("Eric Lasota - Programming, admin"), blackColor, font);
	surface->DrawString(Point::Create(creditsHorizontalOffset, verticalPoint + spacing * (lineNum++)), PSTR("Thijs Verboon - macOS programming"), blackColor, font);
	surface->DrawString(Point::Create(creditsHorizontalOffset, verticalPoint + spacing * (lineNum++)), PSTR("Phil Marell - macOS programming"), blackColor, font);
	surface->DrawString(Point::Create(creditsHorizontalOffset, verticalPoint + spacing * (lineNum++)), PSTR("Iain King-Speir - Linux programming"), blackColor, font);
	(lineNum++);
	surface->DrawString(Point::Create(horizontalOffset, verticalPoint + spacing * (lineNum++)), PSTR(GP_APPLICATION_NAME " is an unoffical third-party port of Glider PRO."), blackColor, font);
	(lineNum++);
	surface->DrawString(Point::Create(horizontalOffset, verticalPoint + spacing * (lineNum++)), PSTR("This software is not maintained by, supported by, endorsed by, or"), blackColor, font);
	surface->DrawString(Point::Create(horizontalOffset, verticalPoint + spacing * (lineNum++)), PSTR("otherwise associated with the authors or publishers of Glider PRO."), blackColor, font);
	(lineNum++);
	surface->DrawString(Point::Create(horizontalOffset, verticalPoint + spacing * (lineNum++)), PSTR("Please do not contact any of them regarding issues that you have"), blackColor, font);
	surface->DrawString(Point::Create(horizontalOffset, verticalPoint + spacing * (lineNum++)), PSTR("with " GP_APPLICATION_NAME "."), blackColor, font);
	(lineNum++);
	surface->DrawString(Point::Create(horizontalOffset, verticalPoint + spacing * (lineNum++)), PSTR("If you would like to contribute to this project, visit:"), blackColor, font);
	surface->DrawString(Point::Create(horizontalOffset, verticalPoint + spacing * (lineNum++)), PSTR(GP_APPLICATION_WEBSITE_STRING), blackColor, font);
	(lineNum++);
	surface->DrawString(Point::Create(horizontalOffset, verticalPoint + spacing * (lineNum++)), PSTR("To report a problem or request support, submit an issue via"), blackColor, font);
	surface->DrawString(Point::Create(horizontalOffset, verticalPoint + spacing * (lineNum++)), PSTR("the website above."), blackColor, font);
	(lineNum++);
	surface->DrawString(Point::Create(horizontalOffset, verticalPoint + spacing * (lineNum++)), PSTR("For more information, please see the accompanying documentation."), blackColor, font);

	surface->DrawString(Point::Create(horizontalOffset, windowRect.bottom - 16), PSTR("Build: " __DATE__ " " __TIME__ " " ABOUT_DIALOG_CONFIGURATION_TAG), blackColor, fontLight);

	DrawDefaultButton(dialog);

	int16_t hit = 0;

	do
	{
		hit = dialog->ExecuteModal(nullptr, PL_FILTER_FUNC(AboutFrameworkFilter));

		if (hit == kAboutOpenSourceButton)
			DoAboutOpenSource();
	} while (hit != kOkayButton);

	dialog->Destroy();
}

//==============================================================  Static Functions
//--------------------------------------------------------------  HiLiteOkayButton
// Draws my pseudo-button to appear as though it is clicked on.

static void HiLiteOkayButton (DrawSurface *surface)
{
	#define		kOkayButtPICTHiLit		151		// res ID of unhilit button PICT
	THandle<BitmapImage>	thePict;
	
	if (!okayButtIsHiLit)
	{
		thePict = PortabilityLayer::ResourceManager::GetInstance()->GetAppResource('PICT', kOkayButtPICTHiLit).StaticCast<BitmapImage>();
		if (thePict != nil)
		{
			surface->DrawPicture(thePict, okayButtonBounds);
			thePict.Dispose();
			
			okayButtIsHiLit = true;
		}
	}
}

//--------------------------------------------------------------  UnHiLiteOkayButton

// Draws my pseudo-button normal (not clicked on).

static void UnHiLiteOkayButton (DrawSurface *surface)
{
	#define		kOkayButtPICTNotHiLit	150		// res ID of hilit button PICT
	THandle<BitmapImage>	thePict;
	
	if (okayButtIsHiLit)
	{
		thePict = PortabilityLayer::ResourceManager::GetInstance()->GetAppResource('PICT', kOkayButtPICTNotHiLit).StaticCast<BitmapImage>();
		if (thePict != nil)
		{
			surface->DrawPicture(thePict, okayButtonBounds);
			thePict.Dispose();
			
			okayButtIsHiLit = false;
		}
	}
}

//--------------------------------------------------------------  UpdateMainPict
// Redraws the main graphic in the dialog (in response to an update event).

static void UpdateMainPict (Dialog *theDial)
{
	Str255		theStr, theStr2;
	uint64_t	freeMemory;

	freeMemory = PLDrivers::GetSystemServices()->GetFreeMemoryCosmetic();
	
	PasStringCopy(PSTR("Memory:   "), theStr);		// display free memory

	long totalSize = static_cast<long>(freeMemory / 1024);
	NumToString(totalSize, theStr2);
	PasStringConcat(theStr, theStr2);
	PasStringConcat(theStr, PSTR("K"));
	DrawDialogUserText2(theDial, 7, theStr);
	
	PasStringCopy(PSTR("Screen:   "), theStr);		// display screen size/depth
	NumToString((long)(thisMac.fullScreen.right - thisMac.fullScreen.left), theStr2);
	PasStringConcat(theStr, theStr2);
	PasStringConcat(theStr, PSTR("x"));
	NumToString((long)(thisMac.fullScreen.bottom - thisMac.fullScreen.top), theStr2);
	PasStringConcat(theStr, theStr2);
	PasStringConcat(theStr, PSTR("x"));
	NumToString((long)thisMac.isDepth, theStr2);
	PasStringConcat(theStr, theStr2);
	DrawDialogUserText2(theDial, 8, theStr);
}

static bool PointIsInDiagonalOkayButton(const Point &pt)
{
	const Point upperVPt = pt - okayButtUpperV;
	const Point lowerVPt = pt - okayButtLowerV;

	const bool edge1 = (upperVPt.h + upperVPt.v) >= 0;
	const bool edge2 = (-upperVPt.h + upperVPt.v) >= 0;
	const bool edge3 = (lowerVPt.h - lowerVPt.v) >= 0;
	const bool edge4 = (-lowerVPt.h - lowerVPt.v) >= 0;

	return edge1 && edge2 && edge3 && edge4;
}

//--------------------------------------------------------------  AboutFilter
// Dialog filter for the About dialog.

static int16_t AboutFilter(void *context, Dialog *dialog, const TimeTaggedVOSEvent *evt)
{
	bool		handledIt = false;
	int16_t		hit = -1;

	if (!evt)
		return -1;

	Window *window = dialog->GetWindow();
	DrawSurface *surface = window->GetDrawSurface();

	if (evt->IsKeyDownEvent())
	{
		switch (PackVOSKeyCode(evt->m_vosEvent.m_event.m_keyboardInputEvent))
		{
		case PL_KEY_SPECIAL(kEnter):
		case PL_KEY_NUMPAD_SPECIAL(kEnter):
			HiLiteOkayButton(surface);
			PLSysCalls::Sleep(8);
			UnHiLiteOkayButton(surface);
			hit = kOkayButton;
			handledIt = true;
			break;

		default:
			handledIt = false;
			break;
		}
	}
	else if (evt->m_vosEvent.m_eventType == GpVOSEventTypes::kMouseInput)
	{
		const GpMouseInputEvent &mouseEvt = evt->m_vosEvent.m_event.m_mouseInputEvent;
		const Point mousePt = window->MouseToLocal(mouseEvt);

		if (mouseEvt.m_eventType == GpMouseEventTypes::kDown)
		{
			if (PointIsInDiagonalOkayButton(mousePt))
			{
				HiLiteOkayButton(surface);
				clickedDownInOkay = true;
				handledIt = false;
			}
			else
				handledIt = false;
		}
		else if (mouseEvt.m_eventType == GpMouseEventTypes::kUp)
		{
			if (PointIsInDiagonalOkayButton(mousePt) && clickedDownInOkay)
			{
				UnHiLiteOkayButton(surface);
				hit = kOkayButton;
				handledIt = true;
			}
			else
			{
				clickedDownInOkay = false;
				handledIt = false;
			}
		}
		else if (mouseEvt.m_eventType == GpMouseEventTypes::kMove)
		{
			if (clickedDownInOkay)
			{
				if (PointIsInDiagonalOkayButton(mousePt))
					HiLiteOkayButton(surface);
				else
					UnHiLiteOkayButton(surface);
			}
		}
	}

	if (!handledIt)
		return -1;

	return hit;
}



//--------------------------------------------------------------  AboutFrameworkFilter
// Dialog filter for the About Framework dialog.

static int16_t AboutFrameworkFilter(void *context, Dialog *dialog, const TimeTaggedVOSEvent *evt)
{
	bool		handledIt = false;
	int16_t		hit = -1;

	if (!evt)
		return -1;

	Window *window = dialog->GetWindow();
	DrawSurface *surface = window->GetDrawSurface();

	if (evt->IsKeyDownEvent())
	{
		switch (PackVOSKeyCode(evt->m_vosEvent.m_event.m_keyboardInputEvent))
		{
		case PL_KEY_SPECIAL(kEnter):
		case PL_KEY_NUMPAD_SPECIAL(kEnter):
			dialog->GetItems()[kOkayButton - 1].GetWidget()->SetHighlightStyle(kControlButtonPart, true);
			PLSysCalls::Sleep(8);
			dialog->GetItems()[kOkayButton - 1].GetWidget()->SetHighlightStyle(kControlButtonPart, false);
			hit = kOkayButton;
			handledIt = true;
			break;

		default:
			handledIt = false;
			break;
		}
	}

	if (!handledIt)
		return -1;

	return hit;
}

void DrawLicenseReader(Window *window)
{
	PortabilityLayer::RenderedFont *rfont = GetFont(PortabilityLayer::FontPresets::kMono10);
	if (!rfont)
		return;

	PortabilityLayer::ResolveCachingColor whiteColor(StdColors::White());
	PortabilityLayer::ResolveCachingColor blackColor(StdColors::Black());

	DrawSurface *surface = window->GetDrawSurface();
	surface->FillRect(licenseReaderTextRect, whiteColor);

	int32_t ascent = rfont->GetMetrics().m_ascent;

	const LicenseReaderLine *lines = *licenseReaderLines;

	for (unsigned int i = 0; i < licenseReaderNumLines; i++)
	{
		int32_t lineY = licenseReaderTextRect.top + ascent + static_cast<int32_t>(kLicenseReaderTextSpacing * i);
		lineY -= static_cast<int32_t>(licenseReaderTextScroll);

		surface->DrawStringConstrained(Point::Create(licenseReaderTextRect.left, lineY), lines[i].m_text.ToShortStr(), licenseReaderTextRect, blackColor, rfont);
	}
}

static void CycleLicenseReaderMouseHistory()
{
	for (unsigned int ri = 1; ri < kLicenseReaderPointHistoryLength; ri++)
	{
		unsigned int i = kLicenseReaderPointHistoryLength - ri;
		licenseReaderPointHistory[i] = licenseReaderPointHistory[i - 1];
	}
}

static void HandleLicenseReaderScroll(Window *window, int32_t offset)
{
	int32_t newScroll = static_cast<int32_t>(licenseReaderTextScroll) + offset;
	if (newScroll < 0)
		newScroll = 0;
	else if (newScroll > static_cast<int32_t>(licenseReaderTextScrollMax))
		newScroll = licenseReaderTextScrollMax;

	if (newScroll != licenseReaderTextScroll)
	{
		licenseReaderTextScroll = newScroll;
		licenseReaderScrollBarWidget->SetState(newScroll);
		DrawLicenseReader(window);
	}
}

static void AutoScrollLicenseReader(Window *window)
{
	if (licenseReaderIsScrolling)
		return;

	int32_t decayRate = 2;
	if (licenseReaderVelocity < 0)
	{
		HandleLicenseReaderScroll(window, licenseReaderVelocity);
		licenseReaderVelocity += decayRate;
		if (licenseReaderVelocity > 0)
			licenseReaderVelocity = 0;
	}
	else if (licenseReaderVelocity > 0)
	{
		HandleLicenseReaderScroll(window, licenseReaderVelocity);
		licenseReaderVelocity -= decayRate;
		if (licenseReaderVelocity < 0)
			licenseReaderVelocity = 0;
	}
}

static void ComputeLicenseReaderVelocity()
{
	int32_t velocity = licenseReaderPointHistory[kLicenseReaderPointHistoryLength - 1].v - licenseReaderPointHistory[0].v;
	licenseReaderVelocity = velocity;
}

static void LicenseReaderScrollBarUpdate(void *captureContext, PortabilityLayer::Widget *widget, int part)
{
	Window *window = static_cast<Window*>(captureContext);	// This is stupid and very lazy...


	const int incrementalStepping = kLicenseReaderTextSpacing;
	const int pageStepping = 20;

	switch (part)
	{
	case kControlUpButtonPart:
		widget->SetState(widget->GetState() - incrementalStepping);
		break;
	case kControlDownButtonPart:
		widget->SetState(widget->GetState() + incrementalStepping);
		break;
	case kControlPageUpPart:
		widget->SetState(widget->GetState() - pageStepping * incrementalStepping);
		break;
	case kControlPageDownPart:
		widget->SetState(widget->GetState() + pageStepping * incrementalStepping);
		break;
	default:
		break;
	};

	licenseReaderTextScroll = widget->GetState();
	DrawLicenseReader(window);
}

static int16_t LicenseReaderFilter(void *context, Dialog *dialog, const TimeTaggedVOSEvent *evt)
{
	bool		handledIt = false;
	int16_t		hit = -1;

	if (!evt)
	{
		if (licenseReaderIsScrolling)
			CycleLicenseReaderMouseHistory();
		else
			AutoScrollLicenseReader(dialog->GetWindow());
		return -1;
	}

	Window *window = dialog->GetWindow();
	DrawSurface *surface = window->GetDrawSurface();

	if (evt->IsKeyDownEvent())
	{
		switch (PackVOSKeyCode(evt->m_vosEvent.m_event.m_keyboardInputEvent))
		{
		case PL_KEY_SPECIAL(kEnter):
		case PL_KEY_NUMPAD_SPECIAL(kEnter):
			dialog->GetItems()[kOkayButton - 1].GetWidget()->SetHighlightStyle(kControlButtonPart, true);
			PLSysCalls::Sleep(8);
			dialog->GetItems()[kOkayButton - 1].GetWidget()->SetHighlightStyle(kControlButtonPart, false);
			hit = kOkayButton;
			handledIt = true;
			break;

		default:
			handledIt = false;
			break;
		}
	}

	if (evt->m_vosEvent.m_eventType == GpVOSEventTypes::kMouseInput)
	{
		const GpMouseInputEvent &mouseEvt = evt->m_vosEvent.m_event.m_mouseInputEvent;

		Point mouseLocalPt = window->MouseToLocal(mouseEvt);

		switch (mouseEvt.m_eventType)
		{
		case GpMouseEventTypes::kDown:
			if (licenseReaderTextRect.Contains(mouseLocalPt))
			{
				for (unsigned int i = 0; i < kLicenseReaderPointHistoryLength; i++)
					licenseReaderPointHistory[i] = mouseLocalPt;
				licenseReaderIsScrolling = true;
			}
			else if (licenseReaderScrollBarWidget != nullptr && licenseReaderScrollBarWidget->GetRect().Contains(mouseLocalPt))
			{
				licenseReaderScrollBarWidget->Capture(dialog->GetWindow(), mouseLocalPt, LicenseReaderScrollBarUpdate);
				licenseReaderTextScroll = licenseReaderScrollBarWidget->GetState();
				DrawLicenseReader(dialog->GetWindow());
			}
			break;
		case GpMouseEventTypes::kLeave:
		case GpMouseEventTypes::kUp:
			licenseReaderIsScrolling = false;
			ComputeLicenseReaderVelocity();
			break;
		case GpMouseEventTypes::kMove:
			if (licenseReaderIsScrolling)
			{
				Point prevPoint = licenseReaderPointHistory[0];
				licenseReaderPointHistory[0] = mouseLocalPt;
				HandleLicenseReaderScroll(window, prevPoint.v - mouseLocalPt.v);
			}
			break;
		default:
			break;
		}
	}

	if (!handledIt)
		return -1;

	return hit;
}

PL_IMPLEMENT_FILTER_FUNCTION(AboutFilter)
PL_IMPLEMENT_FILTER_FUNCTION(LicenseReaderFilter)
PL_IMPLEMENT_FILTER_FUNCTION(AboutFrameworkFilter)
