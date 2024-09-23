
//============================================================================
//----------------------------------------------------------------------------
//								SelectHouse.c
//----------------------------------------------------------------------------
//============================================================================


#include "PLKeyEncoding.h"
#include "PLResources.h"
#include "PLSound.h"
#include "PLStringCompare.h"
#include "PLStandardColors.h"
#include "DialogUtils.h"
#include "Externs.h"
#include "Environ.h"
#include "FileManager.h"
#include "House.h"
#include "RectUtils.h"
#include "ResolveCachingColor.h"
#include "ResourceFile.h"
#include "ResourceManager.h"
#include "PLTimeTaggedVOSEvent.h"
#include "VirtualDirectory.h"
#include "WindowManager.h"


#define kLoadHouseDialogID		1000
#define kDispFiles				12
#define kLoadTitlePictItem		3
#define kLoadNameFirstItem		5
#define kLoadNameLastItem		16
#define kLoadIconFirstItem		17
#define kLoadIconLastItem		28
#define kScrollUpItem			29
#define kScrollDownItem			30
#define kLoadTitlePict1			1001
#define kLoadTitlePict8			1002
#define kDefaultHousePict1		1003
#define kDefaultHousePict8		1004
#define kGrayedOutUpArrow		1052
#define kGrayedOutDownArrow		1053
#define kMaxExtraHouses			8

void WriteOutPrefs (void);
void UpdateLoadDialog (Dialog *);
void PageUpHouses (Dialog *);
void PageDownHouses (Dialog *);
int16_t LoadFilter (Dialog *, const TimeTaggedVOSEvent *);
void SortHouseList (void);
void DoDirSearch (void);


Rect		loadHouseRects[12];
VFileSpec	*theHousesSpecs;
VFileSpec	extraHouseSpecs[kMaxExtraHouses];
long		lastWhenClick;
Point		lastWhereClick;
short		housesFound, thisHouseIndex, maxFiles, willMaxFiles;
short		housePage, demoHouseIndex, numExtraHouses;
char		fileFirstChar[12];

extern	UInt32			doubleTime;


//==============================================================  Functions
//--------------------------------------------------------------  UpdateLoadWindow

#ifndef COMPILEDEMO
void UpdateLoadDialog (Dialog *theDialog)
{
	Rect		tempRect, dialogRect, dummyRect;
	short		houseStart, houseStop, i, wasResFile, count;
//	char		wasState;
	WindowRef	theWindow;
//	RgnHandle	theRegion;

	theWindow = theDialog->GetWindow();
	DrawSurface *surface = theWindow->GetDrawSurface();

	dialogRect = theWindow->GetDrawSurface()->m_port.GetRect();

	ColorFrameWHRect(theDialog->GetWindow()->GetDrawSurface(), 8, 39, 413, 184, kRedOrangeColor8);	// box around files
	
	houseStart = housePage;
	houseStop = housesFound;
	if ((houseStop - houseStart) > kDispFiles)
		houseStop = houseStart + kDispFiles;
	
	count = 0;
	
	for (i = 0; i < 12; i++)
		fileFirstChar[i] = 0x7F;
	
	for (i = houseStart; i < houseStop; i++)
	{
		SpinCursor(1);
		
		GetDialogItemRect(theDialog, kLoadIconFirstItem + i - housePage, 
				&tempRect);
		
		if (SectRect(&dialogRect, &tempRect, &dummyRect))
		{
			PortabilityLayer::CompositeFile *cfile = PortabilityLayer::FileManager::GetInstance()->OpenCompositeFile(theHousesSpecs[i].m_dir, theHousesSpecs[i].m_name);

			bool haveHouseIcon = false;
			GpIOStream *resStream = nil;

			if (cfile)
			{
				PortabilityLayer::IResourceArchive *resFile = PortabilityLayer::ResourceManager::GetInstance()->LoadResFile(cfile);
				if (resFile != nullptr)
				{
					if (LargeIconPlot(surface, resFile, -16455, tempRect))
						haveHouseIcon = true;

					resFile->Destroy();
				}

				cfile->Close();
			}

			if (!haveHouseIcon)
				LoadDialogPICT(theDialog, kLoadIconFirstItem + i - housePage, 
						kDefaultHousePict8);
		}
		
		fileFirstChar[count] = theHousesSpecs[i].m_name[1];
		if ((fileFirstChar[count] <= 0x7A) && (fileFirstChar[count] > 0x60))
			fileFirstChar[count] -= 0x20;
		count++;
		
		DrawDialogUserText(theDialog, kLoadNameFirstItem + i - housePage, 
				theHousesSpecs[i].m_name, i == (thisHouseIndex + housePage));
		
	}
	
	InitCursor();
}
#endif

//--------------------------------------------------------------  PageUpHouses

#ifndef COMPILEDEMO
void PageUpHouses (Dialog *theDial)
{
	Rect		tempRect;
	DrawSurface	*surface = theDial->GetWindow()->GetDrawSurface();
	
	if (housePage < kDispFiles)
	{
		SysBeep(1);
		return;
	}
	
	housePage -= kDispFiles;
	thisHouseIndex = kDispFiles - 1;
	
	theDial->SetItemVisibility(kScrollDownItem - 1, true);
	if (housePage < kDispFiles)
	{
		GetDialogItemRect(theDial, kScrollUpItem, &tempRect);
		theDial->SetItemVisibility(kScrollUpItem - 1, false);
		DrawCIcon(surface, kGrayedOutUpArrow, tempRect.left, tempRect.top);
	}
	
	QSetRect(&tempRect, 8, 39, 421, 223);

	PortabilityLayer::ResolveCachingColor whiteColor = StdColors::White();
	surface->FillRect(tempRect, whiteColor);

	UpdateLoadDialog(theDial);
}
#endif

//--------------------------------------------------------------  PageDownHouses

#ifndef COMPILEDEMO
void PageDownHouses (Dialog *theDial)
{
	Rect		tempRect;
	DrawSurface	*surface = theDial->GetWindow()->GetDrawSurface();
	
	if (housePage >= (housesFound - kDispFiles))
	{
		SysBeep(1);
		return;
	}
	
	housePage += kDispFiles;
	thisHouseIndex = 0;
	
	theDial->SetItemVisibility(kScrollUpItem - 1, true);
	if (housePage >= (housesFound - kDispFiles))
	{
		GetDialogItemRect(theDial, kScrollDownItem, &tempRect);
		theDial->SetItemVisibility(kScrollDownItem - 1, false);
		DrawCIcon(surface, kGrayedOutDownArrow, tempRect.left, tempRect.top);
	}
	
	QSetRect(&tempRect, 8, 39, 421, 223);
	PortabilityLayer::ResolveCachingColor whiteColor = StdColors::White();
	surface->FillRect(tempRect, whiteColor);

	UpdateLoadDialog(theDial);
}
#endif

//--------------------------------------------------------------  LoadFilter
#ifndef COMPILEDEMO

int16_t LoadFilter(void *context, Dialog *dial, const TimeTaggedVOSEvent *evt)
{
	short		screenCount, i, wasIndex;

	if (!evt)
		return -1;

	if (evt->IsKeyDownEvent())
	{
		const intptr_t keyCode = PackVOSKeyCode(evt->m_vosEvent.m_event.m_keyboardInputEvent);
		switch (keyCode)
		{
		case PL_KEY_SPECIAL(kEnter):
		case PL_KEY_NUMPAD_SPECIAL(kEnter):
			FlashDialogButton(dial, kOkayButton);
			return kOkayButton;

		case PL_KEY_SPECIAL(kEscape):
			FlashDialogButton(dial, kCancelButton);
			return kCancelButton;

		case PL_KEY_SPECIAL(kPageUp):
			return kScrollUpItem;

		case PL_KEY_SPECIAL(kPageDown):
			return kScrollDownItem;

		case PL_KEY_SPECIAL(kUpArrow):
			thisHouseIndex -= 4;
			if (thisHouseIndex < 0)
			{
				screenCount = housesFound - housePage;
				if (screenCount > kDispFiles)
					screenCount = kDispFiles;

				thisHouseIndex += 4;
				thisHouseIndex = (((screenCount - 1) / 4) * 4) +
					(thisHouseIndex % 4);
				if (thisHouseIndex >= screenCount)
					thisHouseIndex -= 4;
			}
			UpdateLoadDialog(dial);
			return 0;

		case PL_KEY_SPECIAL(kDownArrow):
			thisHouseIndex += 4;
			screenCount = housesFound - housePage;
			if (screenCount > kDispFiles)
				screenCount = kDispFiles;
			if (thisHouseIndex >= screenCount)
				thisHouseIndex %= 4;
			UpdateLoadDialog(dial);
			return 0;

		case PL_KEY_SPECIAL(kLeftArrow):
			thisHouseIndex--;
			if (thisHouseIndex < 0)
			{
				screenCount = housesFound - housePage;
				if (screenCount > kDispFiles)
					screenCount = kDispFiles;
				thisHouseIndex = screenCount - 1;
			}
			UpdateLoadDialog(dial);
			return 0;

		case PL_KEY_SPECIAL(kTab):
		case PL_KEY_SPECIAL(kRightArrow):
			thisHouseIndex++;
			screenCount = housesFound - housePage;
			if (screenCount > kDispFiles)
				screenCount = kDispFiles;
			if (thisHouseIndex >= screenCount)
				thisHouseIndex = 0;
			UpdateLoadDialog(dial);
			return 0;

		default:
			if (PL_KEY_GET_EVENT_TYPE(keyCode) == KeyEventType_ASCII)
			{
				char theChar = static_cast<char>(PL_KEY_GET_VALUE(keyCode));

				if (theChar >= 'A' && theChar <= 'Z')
				{
					wasIndex = thisHouseIndex;
					thisHouseIndex = -1;
					i = 0;
					do
					{
						if ((fileFirstChar[i] >= theChar) && (fileFirstChar[i] != 0x7F))
							thisHouseIndex = i;
						i++;
					} while ((thisHouseIndex == -1) && (i < 12));

					if (thisHouseIndex == -1)
					{
						screenCount = housesFound - housePage;
						if (screenCount > kDispFiles)
							screenCount = kDispFiles;
						thisHouseIndex = screenCount - 1;
					}
					if (wasIndex != thisHouseIndex)
						UpdateLoadDialog(dial);
				}
				return 0;
			}
			else
				return -1;
		}
	}
	else if (evt->IsLMouseDownEvent())
	{
		const GpMouseInputEvent &mouseEvt = evt->m_vosEvent.m_event.m_mouseInputEvent;

		lastWhenClick = evt->m_timestamp - lastWhenClick;
		lastWhereClick -= Point::Create(mouseEvt.m_x, mouseEvt.m_y);

		return -1;
	}
	else if (evt->IsLMouseUpEvent())
	{
		const GpMouseInputEvent &mouseEvt = evt->m_vosEvent.m_event.m_mouseInputEvent;

		lastWhenClick = evt->m_timestamp;
		lastWhereClick = Point::Create(mouseEvt.m_x, mouseEvt.m_y);

		return -1;
	}

	return -1;
}
#endif

//--------------------------------------------------------------  DoLoadHouse

#ifndef COMPILEDEMO
void DoLoadHouse (void)
{
	Rect			tempRect;
	Dialog			*theDial;
	short			i, wasIndex, screenCount;
	Boolean			leaving, whoCares;
	bool			houseNameDirty = false;

	PortabilityLayer::WindowManager *wm = PortabilityLayer::WindowManager::GetInstance();
	
	BringUpDialog(&theDial, kLoadHouseDialogID, nullptr);

	DrawSurface *surface = theDial->GetWindow()->GetDrawSurface();

	if (housesFound <= kDispFiles)
	{
		GetDialogItemRect(theDial, kScrollUpItem, &tempRect);
		theDial->SetItemVisibility(kScrollUpItem - 1, false);
		DrawCIcon(surface, kGrayedOutUpArrow, tempRect.left, tempRect.top);
		
		GetDialogItemRect(theDial, kScrollDownItem, &tempRect);
		theDial->SetItemVisibility(kScrollDownItem - 1, false);
		DrawCIcon(surface, kGrayedOutDownArrow, tempRect.left, tempRect.top);
	}
	else
	{
		if (thisHouseIndex < kDispFiles)
		{
			GetDialogItemRect(theDial, kScrollUpItem, &tempRect);
			theDial->SetItemVisibility(kScrollUpItem - 1, false);
			DrawCIcon(surface, kGrayedOutUpArrow, tempRect.left, tempRect.top);
		}
		else if (thisHouseIndex > (housesFound - kDispFiles))
		{				
			GetDialogItemRect(theDial, kScrollDownItem, &tempRect);
			theDial->SetItemVisibility(kScrollDownItem - 1, false);
			DrawCIcon(surface, kGrayedOutDownArrow, tempRect.left, tempRect.top);
		}
	}
	wasIndex = thisHouseIndex;
	housePage = (thisHouseIndex / kDispFiles) * kDispFiles;
	thisHouseIndex -= housePage;
	
	for (i = 0; i < 12; i++)
	{
		GetDialogItemRect(theDial, kLoadNameFirstItem + i, 
				&loadHouseRects[i]);
		GetDialogItemRect(theDial, kLoadIconFirstItem + i, 
				&tempRect);
		loadHouseRects[i].top = tempRect.top;
		loadHouseRects[i].bottom++;
	}
	
	leaving = false;

	Window *exclWindow = theDial->GetWindow();
	wm->SwapExclusiveWindow(exclWindow);	// Push exclusive window

	UpdateLoadDialog(theDial);
	
	while (!leaving)
	{
		int16_t item = theDial->ExecuteModal(nullptr, PL_FILTER_FUNC(LoadFilter));

		bool requiresRedraw = false;
		
		if (item == kOkayButton)
		{
			thisHouseIndex += housePage;
			if (thisHouseIndex != wasIndex)
			{
				whoCares = CloseHouse();
				PasStringCopy(theHousesSpecs[thisHouseIndex].m_name,
						thisHouseName);
				if (OpenHouse(true))
					houseNameDirty = true;
			}
			leaving = true;
		}
		else if (item == kCancelButton)
		{
			thisHouseIndex = wasIndex;
			leaving = true;
		}
		else if ((item >= kLoadNameFirstItem) && 
				(item <= kLoadNameLastItem))
		{
			screenCount = housesFound - housePage;
			if (screenCount > kDispFiles)
				screenCount = kDispFiles;
			if ((item - kLoadNameFirstItem != thisHouseIndex) && 
					(item - kLoadNameFirstItem < screenCount))
			{
				thisHouseIndex = item - kLoadNameFirstItem;
				requiresRedraw = true;
			}
						
			if (lastWhereClick.h < 0)
				lastWhereClick.h = -lastWhereClick.h;
			if (lastWhereClick.v < 0)
				lastWhereClick.v = -lastWhereClick.v;
			if ((lastWhenClick < doubleTime) && (lastWhereClick.h < 5) && 
					(lastWhereClick.v < 5))
			{
				thisHouseIndex += housePage;
				if (thisHouseIndex != wasIndex)
				{
					MyDisableControl(theDial, kOkayButton);
					MyDisableControl(theDial, kCancelButton);
					whoCares = CloseHouse();
					PasStringCopy(theHousesSpecs[thisHouseIndex].m_name,
							thisHouseName);
					if (OpenHouse(true))
						houseNameDirty = true;
				}
				leaving = true;
			}
		}
		else if ((item >= kLoadIconFirstItem) && 
				(item <= kLoadIconLastItem))
		{
			screenCount = housesFound - housePage;
			if (screenCount > kDispFiles)
				screenCount = kDispFiles;
			if ((item - kLoadIconFirstItem != thisHouseIndex) && 
					(item - kLoadIconFirstItem < screenCount))
			{
				thisHouseIndex = item - kLoadIconFirstItem;
				requiresRedraw = true;
			}
			
			if (lastWhereClick.h < 0)
				lastWhereClick.h = -lastWhereClick.h;
			if (lastWhereClick.v < 0)
				lastWhereClick.v = -lastWhereClick.v;
			if ((lastWhenClick < doubleTime) && (lastWhereClick.h < 5) && 
					(lastWhereClick.v < 5))
			{
				thisHouseIndex += housePage;
				if (thisHouseIndex != wasIndex)
				{
					MyDisableControl(theDial, kOkayButton);
					MyDisableControl(theDial, kCancelButton);
					whoCares = CloseHouse();
					PasStringCopy(theHousesSpecs[thisHouseIndex].m_name,
							thisHouseName);
					if (OpenHouse(true))
						houseNameDirty = true;
				}
				leaving = true;
			}
		}
		else if (item == kScrollUpItem)
		{
			PageUpHouses(theDial);
		}
		else if (item == kScrollDownItem)
		{
			PageDownHouses(theDial);
		}

		if (requiresRedraw)
			UpdateLoadDialog(theDial);
	}

	if (houseNameDirty)
		WriteOutPrefs();

	wm->SwapExclusiveWindow(exclWindow);	// Pop exclusive window

	theDial->Destroy();
}
#endif

//--------------------------------------------------------------  SortHouseList

void SortHouseList (void)
{
	VFileSpec	tempSpec;
	int		i, h, whosFirst;
	
	i = 0;			// remove exact duplicate houses
	while (i < housesFound)
	{
		h = i + 1;
		while (h < housesFound)
		{
			if ((StrCmp::Equal(theHousesSpecs[i].m_name, theHousesSpecs[h].m_name)) &&
					(theHousesSpecs[i].m_dir == theHousesSpecs[i].m_dir))
			{
				theHousesSpecs[h] = theHousesSpecs[housesFound - 1];
				housesFound--;
			}
			h++;
		}
		i++;
	}
	
	for (i = 0; i < housesFound - 1; i++)
	{
		for (h = 0; h < (housesFound - i - 1); h++)
		{
			whosFirst = WhichStringFirst(theHousesSpecs[h].m_name,
					theHousesSpecs[h + 1].m_name);
			if (whosFirst == 1)
			{
				tempSpec = theHousesSpecs[h + 1];
				theHousesSpecs[h + 1] = theHousesSpecs[h];
				theHousesSpecs[h] = tempSpec;
			}
		}
	}
}

//--------------------------------------------------------------  DoDirSearch

void DoDirSearch (void)
{
	#define		kMaxDirectories		32
	PortabilityLayer::VirtualDirectory_t		theDirs[kMaxDirectories];
	PLError_t		theErr;
	short		count, i, numDirs;
	int currentDir;
	
	for (i = 0; i < kMaxDirectories; i++)
		theDirs[i] = PortabilityLayer::VirtualDirectories::kUnspecified;
	currentDir = 0;
	theDirs[0] = PortabilityLayer::VirtualDirectories::kGameData;
	theDirs[1] = PortabilityLayer::VirtualDirectories::kUserData;
	numDirs = 2;

	PortabilityLayer::FileManager *fm = PortabilityLayer::FileManager::GetInstance();
	
	while ((currentDir < numDirs) && (currentDir < kMaxDirectories))
	{
		count = 1;
		theErr = PLErrors::kNone;

		long dirID = theDirs[currentDir];

		DirectoryFileListEntry *firstFile = GetDirectoryFiles(theDirs[currentDir]);

		for (DirectoryFileListEntry *f = firstFile; f; f = f->nextEntry)
		{
			SpinCursor(1);

			if ((f->finderInfo.fdType == 'gliH') && (f->finderInfo.fdCreator == 'ozm5') && (housesFound < maxFiles))
			{
				theHousesSpecs[housesFound] = MakeVFileSpec(theDirs[currentDir], f->name);

				if (fm->CompositeFileExists(theDirs[currentDir], f->name))
					housesFound++;
			}
		}

		DisposeDirectoryFiles(firstFile);

		currentDir++;
	}
	
	if (housesFound < 1)
	{
		thisHouseIndex = -1;
		YellowAlert(kYellowNoHouses, 0);
	}
	else
	{
		SortHouseList();
		thisHouseIndex = 0;
		for (i = 0; i < housesFound; i++)
		{
			if (StrCmp::Equal(theHousesSpecs[i].m_name, thisHouseName))
			{
				thisHouseIndex = i;
				break;
			}
		}
		PasStringCopy(theHousesSpecs[thisHouseIndex].m_name, thisHouseName);
		
		demoHouseIndex = -1;
		for (i = 0; i < housesFound; i++)
		{
			if (StrCmp::Equal(theHousesSpecs[i].m_name, PSTR("Demo House")))
			{
				demoHouseIndex = i;
				break;
			}
		}
	}
}

//--------------------------------------------------------------  BuildHouseList

void BuildHouseList (void)
{
	short		i;
	
	if (thisMac.hasSystem7)
	{
		housesFound = 0;						// zero the number of houses found
		for (i = 0; i < numExtraHouses; i++)	// 1st, insert extra houses into list
		{
			theHousesSpecs[housesFound] = extraHouseSpecs[i];
			housesFound++;
		}
		DoDirSearch();							// now, search folders for the rest
	}
}

//--------------------------------------------------------------  AddExtraHouse

void AddExtraHouse (const VFileSpec &newHouse)
{
	if (numExtraHouses >= kMaxExtraHouses)
		return;
	
	extraHouseSpecs[numExtraHouses] = newHouse;
	numExtraHouses++;
}

PL_IMPLEMENT_FILTER_FUNCTION(LoadFilter)
