
//============================================================================
//----------------------------------------------------------------------------
//								  AnimCursor.c
//----------------------------------------------------------------------------
//============================================================================

#include "Externs.h"
#include "Environ.h"
#include "IGpCursor.h"
#include "IGpDisplayDriver.h"
#include "MemoryManager.h"
#include "ResourceManager.h"

#include "PLBigEndian.h"
#include "PLDrivers.h"
#include "PLResources.h"
#include "PLSysCalls.h"

#include <assert.h>


#define	rAcurID					128
#define rHandCursorID			1000


typedef struct
{
	BEInt16_t		n;
	BEInt16_t		index;
	struct
	{
		BEInt16_t	resID;
		BEInt16_t	reserved;
	} frame[1];
} acurRec;

typedef THandle<acurRec> acurHandle;

typedef struct
{
	struct
	{
		IGpCursor *hwCursor;
	} frame[1];
} compiledAcurRec;

typedef THandle<compiledAcurRec> compiledAcurHandle;


Boolean GetColorCursors (acurHandle, compiledAcurHandle);
void InitAnimatedCursor (acurHandle);


acurHandle			animCursorH = nil;
compiledAcurHandle	compiledAnimCursorH = nil;


//==============================================================  Functions

//--------------------------------------------------------------  GetColorCursors

// Loads color cursors (for animated beach ball).

static uint8_t CompactChannel(const uint8_t *doublet)
{
	unsigned int doubletValue = (doublet[0] << 8) + doublet[1];

	return (doubletValue * 2 + 0x101) / 0x202;
}

IGpCursor *LoadColorCursor(int16_t resID)
{
	const THandle<void> resHdl = PortabilityLayer::ResourceManager::GetInstance()->GetAppResource('crsr', resID);
	if (!resHdl)
		return nullptr;

	// The color cursor format is not fully documented.  Appears to be:
	// Header
	// CursorPixMapPrefix (at pixMapOffset)
	// BEPixMap
	// Pixel data
	// BEColorTableHeader
	// BEColorTableItem[...]

	struct CursorPixMapPrefix
	{
		BEUInt32_t m_unknown;	// Seems to always be zero
		BEUInt16_t m_subFormat;	// 0x8002 = 2 colors, 0x8004 = 4 colors, 0x8008 = 16 colors, 0x8010 = 256 colors
	};

	struct CursorHeader
	{
		BEUInt16_t m_cursorType;
		BEUInt32_t m_pixMapOffset;
		BEUInt32_t m_pixDataOffset;
		BEUInt32_t m_expandedData;
		BEUInt16_t m_expandedDataDepth;
		BEUInt32_t m_unused;
		uint8_t m_bwCursor[32];
		uint8_t m_mask[32];
		BEUInt16_t m_hotSpotY;
		BEUInt16_t m_hotSpotX;
		BEUInt32_t m_colorTableResourceID;
		BEUInt32_t m_cursorResourceID;
	};

	const void *cursorDataBase = *resHdl;
	const void *cursorData = cursorDataBase;

	const CursorHeader *cursorHeader = static_cast<const CursorHeader *>(cursorData);
	cursorData = cursorHeader + 1;
	cursorData = static_cast<const void*>(reinterpret_cast<const uint8_t*>(cursorDataBase) + cursorHeader->m_pixMapOffset);

	const CursorPixMapPrefix *cursorPixMapPrefix = static_cast<const CursorPixMapPrefix *>(cursorData);

	cursorData = cursorPixMapPrefix + 1;

	const BEPixMap *pixMap = reinterpret_cast<const BEPixMap*>(reinterpret_cast<const uint8_t*>(cursorData));
	cursorData = pixMap + 1;

	cursorData = static_cast<const void*>(reinterpret_cast<const uint8_t*>(cursorDataBase) + cursorHeader->m_pixDataOffset);
	const uint8_t *pixMapDataBytes = static_cast<const uint8_t*>(cursorData);

	const Rect rect = pixMap->m_bounds.ToRect();
	const int width = rect.right - rect.left;
	const int height = rect.bottom - rect.top;
	const int componentSize = static_cast<int>(pixMap->m_componentSize);

	switch (componentSize)
	{
	case 1:
	case 2:
	case 4:
	case 8:
		break;
	default:
		assert(false);
		break;
	}

	const int bitsRequired = width * height * componentSize;
	const int bytesRequired = (bitsRequired + 7) / 8;

	PortabilityLayer::MemoryManager *mm = PortabilityLayer::MemoryManager::GetInstance();

	uint8_t *colorValues = static_cast<uint8_t*>(mm->Alloc(static_cast<size_t>(width) * height));
	if (!colorValues)
	{
		resHdl.Dispose();
		return nullptr;
	}

	const int numPixels = width * height;

	switch (componentSize)
	{
	case 1:
		for (int i = 0; i < numPixels; i++)
			colorValues[i] = (pixMapDataBytes[i / 8] >> (7 - (i & 7) * 1)) & 0x1;
		break;
	case 2:
		for (int i = 0; i < numPixels; i++)
			colorValues[i] = (pixMapDataBytes[i / 4] >> (6 - (i & 3) * 2)) & 0x3;
		break;
	case 4:
		for (int i = 0; i < numPixels; i++)
			colorValues[i] = (pixMapDataBytes[i / 2] >> (4 - (i & 1) * 4)) & 0xf;
		break;
	case 8:
		for (int i = 0; i < numPixels; i++)
			colorValues[i] = pixMapDataBytes[i];
		break;
	default:
		assert(false);
		break;
	}

	cursorData = static_cast<const void*>(pixMapDataBytes + bytesRequired);
	const BEColorTableHeader *colorTableHeader = static_cast<const BEColorTableHeader *>(cursorData);

	cursorData = colorTableHeader + 1;

	const BEColorTableItem *ctabItems = static_cast<const BEColorTableItem *>(cursorData);

	uint32_t decodedCTabItems[256];
	const int numCTabItems = colorTableHeader->m_numItemsMinusOne + 1;

	for (int i = 0; i < numCTabItems; i++)
	{
		const BEColorTableItem &item = ctabItems[i];
		unsigned char rgba[4];
		rgba[0] = CompactChannel(item.m_red);
		rgba[1] = CompactChannel(item.m_green);
		rgba[2] = CompactChannel(item.m_blue);
		rgba[3] = 255;

		int index = item.m_index;
		assert(index >= 0 && index < 256);

		memcpy(decodedCTabItems + index, rgba, 4);
	}

	uint32_t *pixelDataRGBA = static_cast<uint32_t*>(mm->Alloc(static_cast<size_t>(width) * static_cast<size_t>(height) * 4));
	if (!pixelDataRGBA)
	{
		mm->Release(colorValues);
		resHdl.Dispose();
		return nullptr;
	}

	for (int i = 0; i < numPixels; i++)
	{
		const uint8_t ctabIndex = colorValues[i];
		pixelDataRGBA[i] = decodedCTabItems[ctabIndex];

		if (((cursorHeader->m_mask[i / 8] >> (7 - (i & 7))) & 1) == 0)
			reinterpret_cast<uint8_t*>(pixelDataRGBA + i)[3] = 0;
	}

	mm->Release(colorValues);

	IGpCursor *cursor = PLDrivers::GetDisplayDriver()->CreateColorCursor(width, height, pixelDataRGBA, cursorHeader->m_hotSpotX, cursorHeader->m_hotSpotY);

	mm->Release(pixelDataRGBA);

	resHdl.Dispose();

	return cursor;
}

Boolean GetColorCursors (acurHandle ballCursH, compiledAcurHandle compiledBallCursH)
{
	short			i, j;
	IGpCursor *hwCursor;
	Boolean			result = true;

	if (ballCursH)
	{
		j = (*ballCursH)->n;				// Get the number of cursors
		HideCursor();						// Hide the cursor
		for (i = 0; i < j; i++)				// Walk through the acur resource
		{
			hwCursor = LoadColorCursor((*ballCursH)->frame[i].resID);	// Get the cursor
			if (hwCursor == nil)			// Make sure a real cursor was returned
			{								// If not, trash all cursors loaded
				for (j = 0; j < i; j++)
					(*compiledBallCursH)->frame[j].hwCursor->Destroy();
				result = false;				// Tell calling proc we failed
				break;						// And break out of the loop
			}
			else							// But, if the cursor loaded ok
			{								// add it to our list or cursor handles
				(*compiledBallCursH)->frame[i].hwCursor = hwCursor;
				PLDrivers::GetDisplayDriver()->SetCursor(hwCursor);
			}
		}
		InitCursor();						// Show the cursor again (as arrow)
	}
	return(result);							// Return to calling proc w/ results
}

//--------------------------------------------------------------  InitAnimatedCursor

// Loads and sets up animated beach ball cursor structures.

void InitAnimatedCursor (acurHandle ballCursH)
{
	if (thisMac.isTouchscreen)
		return;

	compiledAcurHandle	compiledBallCursorH;

	if (ballCursH == nil)
		ballCursH = PortabilityLayer::ResourceManager::GetInstance()->GetAppResource('acur', 128).StaticCast<acurRec>();
	if (ballCursH && ballCursH != animCursorH)
	{
		compiledBallCursorH = NewHandle(sizeof(compiledAcurRec) * (*ballCursH)->n).StaticCast<compiledAcurRec>();
		if (!compiledBallCursorH)
			RedAlert(kErrFailedResourceLoad);

		DisposCursors();

		GetColorCursors(ballCursH, compiledBallCursorH);

		animCursorH = ballCursH;
		compiledAnimCursorH = compiledBallCursorH;
		(*ballCursH)->index = 0;
	}
	else
		RedAlert(kErrFailedResourceLoad);
}

//--------------------------------------------------------------  LoadCursors

// Just calls the above function.  Other code could be added here though?
// to add additional cursors.

void LoadCursors (void)
{
	InitAnimatedCursor(PortabilityLayer::ResourceManager::GetInstance()->GetAppResource('acur', rAcurID).StaticCast<acurRec>());
}

//--------------------------------------------------------------  DisposCursors

// Disposes of all memory allocated by anaimated beach ball cursors.

void DisposCursors (void)
{
	short		i, j;

	if (compiledAnimCursorH != nil)
	{
		j = (*animCursorH)->n;

		for (i = 0; i < j; i++)
		{
			if ((*compiledAnimCursorH)->frame[i].hwCursor != nil)
				(*compiledAnimCursorH)->frame[i].hwCursor->Destroy();
		}

		compiledAnimCursorH.Dispose();
		compiledAnimCursorH = nil;
	}

	if (animCursorH != nil)
	{
		animCursorH.Dispose();
		animCursorH = nil;
	}
}

//--------------------------------------------------------------  IncrementCursor

// Advances the beach ball cursor one frame.

void IncrementCursor (void)
{
	if (animCursorH == nullptr)
		InitAnimatedCursor(nil);
	if (animCursorH)
	{
		acurRec *acur = *animCursorH;
		acur->index++;
		acur->index %= acur->n;

		PLDrivers::GetDisplayDriver()->SetCursor((*compiledAnimCursorH)->frame[(*animCursorH)->index].hwCursor);
	}
	else
		PLDrivers::GetDisplayDriver()->SetStandardCursor(EGpStandardCursors::kWait);
}

//--------------------------------------------------------------  DecrementCursor

// Reverses the beach ball cursor one frame.

void DecrementCursor (void)
{
	if (animCursorH == nullptr)
		InitAnimatedCursor(nil);
	if (animCursorH)
	{
		(*animCursorH)->index--;
		if (((*animCursorH)->index) < 0)
			(*animCursorH)->index = ((*animCursorH)->n) - 1;

		PLDrivers::GetDisplayDriver()->SetCursor((*compiledAnimCursorH)->frame[(*animCursorH)->index].hwCursor);
	}
	else
		PLDrivers::GetDisplayDriver()->SetStandardCursor(EGpStandardCursors::kWait);
}

//--------------------------------------------------------------  SpinCursor

// Advances the beach ball cursor the number of frames specified.

void SpinCursor (short incrementIndex)
{
	PL_ASYNCIFY_PARANOID_DISARM_FOR_SCOPE();

	UInt32		dummyLong;
	short		i;

	for (i = 0; i < incrementIndex; i++)
	{
		IncrementCursor();
		Delay(1, &dummyLong);
	}
}

//--------------------------------------------------------------  BackSpinCursor

// Reverses the beach ball cursor the number of frames specified.

void BackSpinCursor (short decrementIndex)
{
	UInt32		dummyLong;
	short		i;

	for (i = 0; i < decrementIndex; i++)
	{
		DecrementCursor();
		Delay(1, &dummyLong);
	}
}

