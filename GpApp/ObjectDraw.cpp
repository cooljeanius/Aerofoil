
//============================================================================
//----------------------------------------------------------------------------
//								 ObjectDraw.c
//----------------------------------------------------------------------------
//============================================================================


#include "Externs.h"
#include "Environ.h"
#include "Objects.h"
#include "QDManager.h"
#include "RectUtils.h"
#include "Room.h"
#include "Vec2i.h"
#include "PLStandardColors.h"
#include "ResolveCachingColor.h"
#include "ScanlineMask.h"
#include "ScanlineMaskConverter.h"


#define k8WhiteColor			0
#define kYellowColor			5
#define kGoldColor				11
#define k8RedColor				35
#define kPaleVioletColor		42
#define k8LtTanColor			52
#define k8BambooColor			53
#define kDarkFleshColor			58
#define k8TanColor				94
#define k8PissYellowColor		95
#define k8OrangeColor			59
#define k8BrownColor			137
#define k8Red4Color				143
#define k8SkyColor				150
#define k8EarthBlueColor		170
#define k8DkRedColor			222
#define k8DkRed2Color			223
#define kIntenseGreenColor		225
#define kIntenseBlueColor		235
#define k8PumpkinColor			101
#define k8LtstGrayColor			245
#define k8LtstGray2Color		246
#define k8LtstGray3Color		43
#define k8LtstGray4Color		247
#define k8LtstGray5Color		248
#define k8LtGrayColor			249
#define k8GrayColor				250
#define k8Gray2Color			251
#define k8DkGrayColor			252
#define k8DkGray2Color			253
#define k8DkGray3Color			172
#define k8DkstGrayColor			254
#define k8BlackColor			255


void DrawClockDigit (short, Rect *);
void DrawClockHands (Point, short, short);
void DrawLargeClockHands (Point, short, short);


//==============================================================  Functions
//--------------------------------------------------------------  DrawSimpleBlowers

void DrawSimpleBlowers (short what, Rect *theRect)
{
	CopyMask((BitMap *)*GetGWorldPixMap(blowerSrcMap), 
			(BitMap *)*GetGWorldPixMap(blowerMaskMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&srcRects[what], &srcRects[what], theRect);
}

//--------------------------------------------------------------  DrawTiki

void DrawTiki (Rect *theRect, short down)
{
#define kTikiPoleBase	300
	long		darkGrayC, lightWoodC, darkWoodC;
	
	if (thisMac.isDepth == 4)
	{
		darkGrayC = 14;
		lightWoodC = 6;
		darkWoodC = 9;
	}
	else
	{
		darkGrayC = k8DkstGrayColor;
		lightWoodC = k8BambooColor;
		darkWoodC = k8PissYellowColor;
	}
	
	if (theRect->bottom < kTikiPoleBase + down)
	{
		ColorLine(backSrcMap, theRect->left + 11, theRect->bottom - 1,
				theRect->left + 11, kTikiPoleBase + down - 1, darkGrayC);
		ColorLine(backSrcMap, theRect->left + 12, theRect->bottom - 1,
				theRect->left + 12, kTikiPoleBase + down, lightWoodC);
		ColorLine(backSrcMap, theRect->left + 13, theRect->bottom - 1,
				theRect->left + 13, kTikiPoleBase + down, darkWoodC);
		ColorLine(backSrcMap, theRect->left + 14, theRect->bottom - 1,
				theRect->left + 14, kTikiPoleBase + down, darkWoodC);
		ColorLine(backSrcMap, theRect->left + 15, theRect->bottom - 1,
				theRect->left + 15, kTikiPoleBase + down - 1, darkGrayC);
	}
	
	CopyMask((BitMap *)*GetGWorldPixMap(blowerSrcMap), 
			(BitMap *)*GetGWorldPixMap(blowerMaskMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&srcRects[kTiki], &srcRects[kTiki], theRect);
}

//--------------------------------------------------------------  DrawInvisibleBlower

void DrawInvisibleBlower (Rect *theRect)
{
	Rect		tempRect;

	QSetRect(&tempRect, 0, 0, 24, 24);
	QOffsetRect(&tempRect, theRect->left, theRect->top);
	
	ColorFrameRect(backSrcMap, tempRect, 192);
}

//--------------------------------------------------------------  DrawLiftArea

void DrawLiftArea (Rect *theRect)
{	
	ColorFrameRect(backSrcMap, *theRect, 192);
}

//--------------------------------------------------------------  DrawTable

void DrawTable (Rect *tableTop, short down)
{
	#define		kTableBaseTop		296
	#define		kTableShadowTop		312
	#define		kTableShadowOffset	12
	Rect		tempRect;
	long		brownC, tanC, dkRedC, blackC;
	short		hCenter, vShadow;
	Pattern		dummyPattern;
	
	if (thisMac.isDepth == 4)
	{
		brownC = 11;
		tanC = 9;
		dkRedC = 14;
		blackC = 15;
	}
	else
	{
		brownC = k8BrownColor;
		tanC = k8TanColor;
		dkRedC = k8DkRed2Color;
		blackC = k8BlackColor;
	}
	
	QSetRect(&tempRect, tableTop->left, 0, tableTop->right, 
			RectWide(tableTop) / 10);
	QOffsetRect(&tempRect, 0, 
			-HalfRectTall(&tempRect) + kTableShadowTop + down);
	QOffsetRect(&tempRect, kTableShadowOffset, -kTableShadowOffset);

	GetQDGlobalsGray(&dummyPattern);

	if (thisMac.isDepth == 4)
		ColorOvalMaskPattern(backSrcMap, tempRect, 15, dummyPattern);
	else
		ColorOvalMaskPattern(backSrcMap, tempRect, k8DkstGrayColor, dummyPattern);
	
	InsetRect(tableTop, 0, 1);
	ColorRect(backSrcMap, *tableTop, brownC);
	InsetRect(tableTop, 0, -1);
	
	ColorLine(backSrcMap, tableTop->left, tableTop->top + 1,
			tableTop->left, tableTop->top + 1, k8WhiteColor);
	ColorLine(backSrcMap, tableTop->left + 1, tableTop->top,
			tableTop->right - 2, tableTop->top, k8WhiteColor);
	ColorLine(backSrcMap, tableTop->right - 1, tableTop->top + 1,
			tableTop->right - 1, tableTop->top + 1, k8WhiteColor);
	
	ColorLine(backSrcMap, tableTop->left + 1, tableTop->top + 1,
			tableTop->right - 2, tableTop->top + 1, tanC);
	ColorLine(backSrcMap, tableTop->left, tableTop->top + 2,
			tableTop->left, tableTop->bottom - 2, tanC);
	
	ColorLine(backSrcMap, tableTop->left + 1, tableTop->bottom - 1,
			tableTop->right - 2, tableTop->bottom - 1, blackC);
	ColorLine(backSrcMap, tableTop->right - 1, tableTop->top + 2,
			tableTop->right - 1, tableTop->bottom - 2, blackC);
	
	ColorLine(backSrcMap, tableTop->left + 1, tableTop->bottom - 2,
			tableTop->right - 2, tableTop->bottom - 2, dkRedC);
	
	if (tableTop->bottom < kTableBaseTop + down)
	{
		hCenter = (tableTop->left + tableTop->right) / 2;
		
		ColorLine(backSrcMap, hCenter - 3, tableTop->bottom,
				hCenter - 3, kTableBaseTop + down, blackC);
		ColorLine(backSrcMap, hCenter - 2, tableTop->bottom,
				hCenter - 2, kTableBaseTop + down, k8LtGrayColor);
		ColorLine(backSrcMap, hCenter - 1, tableTop->bottom,
				hCenter - 1, kTableBaseTop + down, k8GrayColor);
		ColorLine(backSrcMap, hCenter, tableTop->bottom,
				hCenter, kTableBaseTop + down, k8DkGrayColor);
		ColorLine(backSrcMap, hCenter + 1, tableTop->bottom,
				hCenter + 1, kTableBaseTop + down, blackC);
	
		vShadow = tableTop->bottom + RectWide(tableTop) / 4 - 2;
		if (vShadow > kTableBaseTop + down)
		{
			ColorLine(backSrcMap, hCenter - 2, tableTop->bottom,
					hCenter - 2, kTableBaseTop + down, k8DkGrayColor);
			ColorLine(backSrcMap, hCenter - 1, tableTop->bottom,
					hCenter - 1, kTableBaseTop + down, k8DkGrayColor);
			ColorLine(backSrcMap, hCenter, tableTop->bottom,
					hCenter, kTableBaseTop + down, blackC);
		}
		else
		{
			ColorLine(backSrcMap, hCenter - 2, tableTop->bottom,
					hCenter - 2, vShadow, k8DkGrayColor);
			ColorLine(backSrcMap, hCenter - 1, tableTop->bottom,
					hCenter - 1, vShadow + 1, k8DkGrayColor);
			ColorLine(backSrcMap, hCenter, tableTop->bottom,
					hCenter, vShadow + 2, blackC);
		}
	}
	
	tempRect = tableSrc;
	QOffsetRect(&tempRect, -HalfRectWide(&tableSrc) + tableTop->left + 
			HalfRectWide(tableTop), kTableBaseTop + down);
	CopyMask((BitMap *)*GetGWorldPixMap(furnitureSrcMap), 
			(BitMap *)*GetGWorldPixMap(furnitureMaskMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&tableSrc, &tableSrc, &tempRect);
}

//--------------------------------------------------------------  DrawShelf

void DrawShelf (Rect *shelfTop)
{
	#define		kBracketInset		18
	#define		kShelfDeep			4
	#define		kBracketThick		5
	#define		kShelfShadowOff		12
	Rect		tempRect;
	long		brownC, ltTanC, tanC, dkRedC, blackC;
	Pattern		dummyPattern;
	
	if (thisMac.isDepth == 4)
	{
		brownC = 11;
		ltTanC = 7;
		tanC = 9;
		dkRedC = 14;
		blackC = 15;
	}
	else
	{
		brownC = k8BrownColor;
		ltTanC = k8LtTanColor;
		tanC = k8TanColor;
		dkRedC = k8DkRed2Color;
		blackC = k8BlackColor;
	}

	PortabilityLayer::Vec2i poly[5];
	poly[0] = PortabilityLayer::Vec2i(shelfTop->left, shelfTop->bottom);
	poly[1] = poly[0] + PortabilityLayer::Vec2i(kShelfShadowOff, kShelfShadowOff);
	poly[2] = poly[1] + PortabilityLayer::Vec2i(RectWide(shelfTop) - kShelfDeep, 0);
	poly[3] = poly[2] + PortabilityLayer::Vec2i(0, -kShelfThick + 1);
	poly[4] = poly[3] + PortabilityLayer::Vec2i(-kShelfShadowOff, -kShelfShadowOff);

	PortabilityLayer::ScanlineMask *mask = PortabilityLayer::ScanlineMaskConverter::CompilePoly(poly, sizeof(poly) / sizeof(poly[0]));

	if (mask)
	{
		GetQDGlobalsGray(&dummyPattern);
		if (thisMac.isDepth == 4)
			ColorRegionMaskPattern(backSrcMap, mask, 15, dummyPattern);
		else
			ColorRegionMaskPattern(backSrcMap, mask, k8DkstGrayColor, dummyPattern);
		mask->Destroy();
	}

	InsetRect(shelfTop, 0, 1);
	ColorRect(backSrcMap, *shelfTop, brownC);
	InsetRect(shelfTop, 0, -1);
	
	ColorLine(backSrcMap, shelfTop->left + 1, shelfTop->top,
			shelfTop->left + 1 + kShelfDeep, shelfTop->top, ltTanC);
	ColorLine(backSrcMap, shelfTop->left, shelfTop->top + 1,
			shelfTop->left + kShelfDeep, shelfTop->top + 1, tanC);
	ColorLine(backSrcMap, shelfTop->left, shelfTop->top + 2,
			shelfTop->left + kShelfDeep, shelfTop->top + 2, tanC);
	ColorLine(backSrcMap, shelfTop->left, shelfTop->top + 3,
			shelfTop->left + kShelfDeep, shelfTop->top + 3, tanC);
	ColorLine(backSrcMap, shelfTop->left + 1, shelfTop->bottom - 1,
			shelfTop->left + 1 + kShelfDeep, shelfTop->bottom - 1, dkRedC);
	ColorLine(backSrcMap, shelfTop->left + 2 + kShelfDeep, shelfTop->bottom - 1,
			shelfTop->right - 2, shelfTop->bottom - 1, blackC);
	ColorLine(backSrcMap, shelfTop->left + 2 + kShelfDeep, shelfTop->top,
			shelfTop->right - 2, shelfTop->top, tanC);
	ColorLine(backSrcMap, shelfTop->right - 1, shelfTop->top + 1,
			shelfTop->right - 1, shelfTop->bottom - 2, blackC);
	
	tempRect = shelfSrc;
	ZeroRectCorner(&tempRect);
	QOffsetRect(&tempRect, shelfTop->left + kBracketInset, shelfTop->bottom);
	CopyMask((BitMap *)*GetGWorldPixMap(furnitureSrcMap), 
			(BitMap *)*GetGWorldPixMap(furnitureMaskMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&shelfSrc, &shelfSrc, &tempRect);
	
	ZeroRectCorner(&tempRect);
	QOffsetRect(&tempRect, shelfTop->right - kBracketInset - kShelfDeep - 
			kBracketThick, shelfTop->bottom);
	CopyMask((BitMap *)*GetGWorldPixMap(furnitureSrcMap), 
			(BitMap *)*GetGWorldPixMap(furnitureMaskMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&shelfSrc, &shelfSrc, &tempRect);
}

//--------------------------------------------------------------  DrawCabinet

void DrawCabinet (Rect *cabinet)
{
	#define		kCabinetDeep		4
	#define		kCabinetShadowOff	6
	Rect		tempRect;
	long		brownC, dkGrayC, ltTanC, tanC, dkRedC, blackC;
	Pattern		dummyPattern;
	
	if (thisMac.isDepth == 4)
	{
		brownC = 11;
		dkGrayC = 14;
		ltTanC = 7;
		tanC = 9;
		dkRedC = 14;
		blackC = 15;
	}
	else
	{
		brownC = k8BrownColor;
		dkGrayC = k8DkstGrayColor;
		ltTanC = k8LtTanColor;
		tanC = k8TanColor;
		dkRedC = k8DkRed2Color;
		blackC = k8BlackColor;
	}

	{

		PortabilityLayer::Vec2i poly[5];
		poly[0] = PortabilityLayer::Vec2i(cabinet->left, cabinet->bottom);
		poly[1] = poly[0] + PortabilityLayer::Vec2i(kCabinetShadowOff, kCabinetShadowOff);
		poly[2] = poly[1] + PortabilityLayer::Vec2i(RectWide(cabinet), 0);
		poly[3] = poly[2] + PortabilityLayer::Vec2i(0, -RectTall(cabinet) + kCabinetDeep);
		poly[4] = poly[3] + PortabilityLayer::Vec2i(-kCabinetShadowOff, -kCabinetShadowOff);

		PortabilityLayer::ScanlineMask *mask = PortabilityLayer::ScanlineMaskConverter::CompilePoly(poly, sizeof(poly) / sizeof(poly[0]));

		if (mask)
		{
			GetQDGlobalsGray(&dummyPattern);
			if (thisMac.isDepth == 4)
				ColorRegionMaskPattern(backSrcMap, mask, 15, dummyPattern);
			else
				ColorRegionMaskPattern(backSrcMap, mask, dkGrayC, dummyPattern);

			mask->Destroy();
		}
	}
	
	InsetRect(cabinet, 1, 1);		// fill bulk of cabinet brown
	ColorRect(backSrcMap, *cabinet, brownC);
	InsetRect(cabinet, -1, -1);
	
	tempRect = *cabinet;			// add lighter left side
	tempRect.right = tempRect.left + kCabinetDeep;
	ColorRect(backSrcMap, tempRect, tanC);
									// hilight top edge
	ColorLine(backSrcMap, cabinet->left + 1, cabinet->top + 1,
			cabinet->left + kCabinetDeep, cabinet->top + 1, ltTanC);
	ColorLine(backSrcMap, cabinet->left + kCabinetDeep, cabinet->top + 1,
			cabinet->right - 3, cabinet->top + 1, tanC);
									// shadow bottom edge
	
	ColorLine(backSrcMap, cabinet->left + kCabinetDeep + 3, cabinet->top + 5,
			cabinet->left + kCabinetDeep + 3, cabinet->bottom - 6, tanC);
	ColorLine(backSrcMap, cabinet->left + kCabinetDeep + 4, cabinet->top + 5,
			cabinet->left + kCabinetDeep + 4, cabinet->bottom - 6, tanC);
	ColorLine(backSrcMap, cabinet->left + kCabinetDeep + 9, cabinet->top + 10,
			cabinet->left + kCabinetDeep + 9, cabinet->bottom - 11, dkGrayC);
	
	ColorLine(backSrcMap, cabinet->right - 4, cabinet->top + 6,
			cabinet->right - 4, cabinet->bottom - 5, dkRedC);
	ColorLine(backSrcMap, cabinet->right - 5, cabinet->top + 5,
			cabinet->right - 5, cabinet->bottom - 6, dkGrayC);
	ColorLine(backSrcMap, cabinet->right - 10, cabinet->top + 10,
			cabinet->right - 10, cabinet->bottom - 11, tanC);
	
	ColorLine(backSrcMap, cabinet->left + kCabinetDeep + 4, cabinet->top + 4,
			cabinet->left + kCabinetDeep + 4, cabinet->top + 4, ltTanC);
	ColorLine(backSrcMap, cabinet->left + kCabinetDeep + 5, cabinet->top + 4,
			cabinet->right - 6, cabinet->top + 4, tanC);
	ColorLine(backSrcMap, cabinet->left + kCabinetDeep + 10, cabinet->top + 9,
			cabinet->right - 11, cabinet->top + 9, dkGrayC);
	
	ColorLine(backSrcMap, cabinet->right - 5, cabinet->bottom - 5,
			cabinet->right - 5, cabinet->bottom - 5, dkRedC);
	ColorLine(backSrcMap, cabinet->left + kCabinetDeep + 6, cabinet->bottom - 4,
			cabinet->right - 5, cabinet->bottom - 4, dkRedC);
	ColorLine(backSrcMap, cabinet->left + kCabinetDeep + 5, cabinet->bottom - 5,
			cabinet->right - 6, cabinet->bottom - 5, dkGrayC);
	
	ColorLine(backSrcMap, cabinet->left + kCabinetDeep + 10, cabinet->bottom - 10,
			cabinet->right - 11, cabinet->bottom - 10, tanC);
	
	tempRect = hingeSrc;
	ZeroRectCorner(&tempRect);
	QOffsetRect(&tempRect, cabinet->left + kCabinetDeep + 2, cabinet->top + 10);
	CopyMask((BitMap *)*GetGWorldPixMap(furnitureSrcMap), 
			(BitMap *)*GetGWorldPixMap(furnitureMaskMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&hingeSrc, &hingeSrc, &tempRect);
	
	tempRect = hingeSrc;
	ZeroRectCorner(&tempRect);
	QOffsetRect(&tempRect, cabinet->left + kCabinetDeep + 2, cabinet->bottom - 26);
	CopyMask((BitMap *)*GetGWorldPixMap(furnitureSrcMap), 
			(BitMap *)*GetGWorldPixMap(furnitureMaskMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&hingeSrc, &hingeSrc, &tempRect);
	
	tempRect = handleSrc;
	ZeroRectCorner(&tempRect);
	QOffsetRect(&tempRect, cabinet->right - 8, cabinet->top + 
			HalfRectTall(cabinet) - HalfRectTall(&handleSrc));
	CopyMask((BitMap *)*GetGWorldPixMap(furnitureSrcMap), 
			(BitMap *)*GetGWorldPixMap(furnitureMaskMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&handleSrc, &handleSrc, &tempRect);

	PortabilityLayer::ResolveCachingColor blackColor = StdColors::Black();
	backSrcMap->FrameRect(*cabinet, blackColor);
}

//--------------------------------------------------------------  DrawSimpleFurniture

void DrawSimpleFurniture (short what, Rect *theRect)
{
	CopyMask((BitMap *)*GetGWorldPixMap(furnitureSrcMap), 
			(BitMap *)*GetGWorldPixMap(furnitureMaskMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&srcRects[what], &srcRects[what], theRect);
}

//--------------------------------------------------------------  DrawCounter

void DrawCounter(Rect *counter)
{
#define		kCounterFooterHigh	12
#define		kCounterStripWide	6
#define		kCounterStripTall	29
#define		kCounterPanelDrop	12
	Rect		tempRect;
	long		brownC, dkGrayC, tanC, blackC, dkstRedC;
	short		nRects, width, i;
	DrawSurface	*wasCPort;
	Pattern		dummyPattern;

	if (thisMac.isDepth == 4)
	{
		brownC = 11;
		dkGrayC = 14;
		tanC = 9;
		blackC = 15;
		dkstRedC = 15;
	}
	else
	{
		brownC = k8BrownColor;
		dkGrayC = k8DkstGrayColor;
		tanC = k8TanColor;
		blackC = k8BlackColor;
		dkstRedC = k8DkRed2Color;
	}

	{
		PortabilityLayer::Vec2i poly[6];
		poly[0] = PortabilityLayer::Vec2i(counter->right - 2, counter->bottom);
		poly[1] = poly[0] + PortabilityLayer::Vec2i(10, -10);
		poly[2] = poly[1] + PortabilityLayer::Vec2i(0, -RectTall(counter) + 29);
		poly[3] = poly[2] + PortabilityLayer::Vec2i(2, 0);
		poly[4] = poly[3] + PortabilityLayer::Vec2i(0, -7);
		poly[5] = poly[4] + PortabilityLayer::Vec2i(-12, -12);

		PortabilityLayer::ScanlineMask *mask = PortabilityLayer::ScanlineMaskConverter::CompilePoly(poly, sizeof(poly) / sizeof(poly[0]));

		if (mask)
		{
			GetQDGlobalsGray(&dummyPattern);
			if (thisMac.isDepth == 4)
				ColorRegionMaskPattern(backSrcMap, mask, 15, dummyPattern);
			else
				ColorRegionMaskPattern(backSrcMap, mask, dkGrayC, dummyPattern);

			mask->Destroy();
		}
	}
	
	InsetRect(counter, 2, 2);
	ColorRect(backSrcMap, *counter, brownC);
	InsetRect(counter, -2, -2);
	
	tempRect = *counter;
	tempRect.top = tempRect.bottom - kCounterFooterHigh;
	tempRect.left += 2;
	tempRect.right -= 2;
	ColorRect(backSrcMap, tempRect, dkGrayC);
	ColorLine(backSrcMap, counter->left + 2, counter->bottom - kCounterFooterHigh,
			counter->right - 3, counter->bottom - kCounterFooterHigh, blackC);
	ColorLine(backSrcMap, counter->left + 2, counter->bottom - kCounterFooterHigh + 1,
			counter->right - 3, counter->bottom - kCounterFooterHigh + 1, blackC);
	ColorLine(backSrcMap, counter->right - 3, counter->bottom - kCounterFooterHigh,
			counter->right - 3, counter->bottom - 1, blackC);
	ColorLine(backSrcMap, counter->left + 2, counter->bottom - kCounterFooterHigh,
			counter->left + 2, counter->bottom - 1, k8DkGrayColor);
	
	ColorLine(backSrcMap, counter->right - 2, counter->top,
			counter->right - 2, counter->bottom - kCounterFooterHigh - 1, dkstRedC);
	ColorLine(backSrcMap, counter->left + 1, counter->top + 8,
			counter->left + 1, counter->bottom - kCounterFooterHigh - 1, tanC);
	
	if (thisMac.isDepth == 4)
	{
		ColorLine(backSrcMap, counter->left - 1, counter->top,
				counter->right, counter->top, 1);
		ColorLine(backSrcMap, counter->left - 1, counter->top + 1,
				counter->right, counter->top + 1, 2);
		ColorLine(backSrcMap, counter->left - 1, counter->top + 2,
				counter->right, counter->top + 2, 3);
		ColorLine(backSrcMap, counter->left - 1, counter->top + 3,
				counter->right, counter->top + 3, 4);
		ColorLine(backSrcMap, counter->left - 1, counter->top + 4,
				counter->right, counter->top + 4, 5);
		ColorLine(backSrcMap, counter->left - 1, counter->top + 5,
				counter->right, counter->top + 5, 5);
		ColorLine(backSrcMap, counter->left - 1, counter->top + 6,
				counter->right, counter->top + 6, 5);
		ColorLine(backSrcMap, counter->left - 1, counter->top,
				counter->left - 1, counter->top + 6, 1);
	}
	else
	{
		ColorLine(backSrcMap, counter->left - 1, counter->top,
				counter->right, counter->top, k8LtstGrayColor);
		ColorLine(backSrcMap, counter->left - 1, counter->top + 1,
				counter->right, counter->top + 1, k8LtstGray2Color);
		ColorLine(backSrcMap, counter->left - 1, counter->top + 2,
				counter->right, counter->top + 2, k8LtstGray3Color);
		ColorLine(backSrcMap, counter->left - 1, counter->top + 3,
				counter->right, counter->top + 3, k8LtstGray4Color);
		ColorLine(backSrcMap, counter->left - 1, counter->top + 4,
				counter->right, counter->top + 4, k8LtstGray5Color);
		ColorLine(backSrcMap, counter->left - 1, counter->top + 5,
				counter->right, counter->top + 5, k8LtstGray5Color);
		ColorLine(backSrcMap, counter->left - 1, counter->top + 6,
				counter->right, counter->top + 6, k8LtstGray5Color);
		ColorLine(backSrcMap, counter->left - 1, counter->top,
				counter->left - 1, counter->top + 6, k8LtstGrayColor);
	}
	
	ColorLine(backSrcMap, counter->right, counter->top,
			counter->right, counter->top + 6, k8LtGrayColor);
	ColorLine(backSrcMap, counter->left + 1, counter->top + 7,
			counter->right - 2, counter->top + 7, dkstRedC);
	ColorLine(backSrcMap, counter->left + 1, counter->top + 8,
			counter->right - 2, counter->top + 8, dkstRedC);
	
	nRects = RectWide(counter) / 40;
	if (nRects == 0)
		nRects = 1;
	width = ((RectWide(counter) - kCounterStripWide) / nRects) - kCounterStripWide;
	QSetRect(&tempRect, 0, 0, width, RectTall(counter) - kCounterStripTall);
	QOffsetRect(&tempRect, counter->left + kCounterStripWide, 
			counter->top + kCounterPanelDrop);
	for (i = 0; i < nRects; i++)
	{
		HiliteRect(backSrcMap, tempRect, tanC, dkstRedC);
		InsetRect(&tempRect, 4, 4);
		HiliteRect(backSrcMap, tempRect, dkstRedC, tanC);
		InsetRect(&tempRect, -4, -4);
		QOffsetRect(&tempRect, kCounterStripWide + width, 0);
	}
}

//--------------------------------------------------------------  DrawDresser

void DrawDresser(Rect *dresser)
{
#define		kDresserTopThick	4
#define		kDresserCrease		9
#define		kDresserDrawerDrop	12
#define		kDresserSideSpare	14
	Rect		tempRect, dest;
	long		yellowC, brownC, dkGrayC, ltTanC, dkstRedC;
	short		nRects, height, i;
	Pattern		dummyPattern;

	DrawSurface *surface = backSrcMap;

	if (thisMac.isDepth == 4)
	{
		yellowC = 9;
		brownC = 11;
		dkGrayC = 14;
		ltTanC = 7;
		dkstRedC = 15;
	}
	else
	{
		yellowC = k8PissYellowColor;
		brownC = k8BrownColor;
		dkGrayC = k8DkstGrayColor;
		ltTanC = k8LtTanColor;
		dkstRedC = k8DkRed2Color;
	}

	{
		PortabilityLayer::Vec2i poly[6];
		poly[0] = PortabilityLayer::Vec2i(dresser->left + 10, dresser->bottom + 9);
		poly[1] = poly[0] + PortabilityLayer::Vec2i(RectWide(dresser) - 11, 0);
		poly[2] = poly[1] + PortabilityLayer::Vec2i(9, -9);
		poly[3] = poly[2] + PortabilityLayer::Vec2i(0, -RectTall(dresser) + 12);
		poly[4] = poly[3] + PortabilityLayer::Vec2i(-9, -9);
		poly[5] = poly[4] + PortabilityLayer::Vec2i(-RectWide(dresser) + 11, 0);

		PortabilityLayer::ScanlineMask *mask = PortabilityLayer::ScanlineMaskConverter::CompilePoly(poly, sizeof(poly) / sizeof(poly[0]));
	}
	
	InsetRect(dresser, 2, 2);
	ColorRect(backSrcMap, *dresser, k8PumpkinColor);
	HiliteRect(backSrcMap, *dresser, k8OrangeColor, dkstRedC);
	InsetRect(dresser, -2, -2);
	
	tempRect = *dresser;
	tempRect.bottom = tempRect.top + kDresserTopThick;
	ColorRect(backSrcMap, tempRect, k8PissYellowColor);
	HiliteRect(backSrcMap, tempRect, ltTanC, dkstRedC);
	ColorLine(backSrcMap, dresser->left + 2, dresser->top + kDresserTopThick,
			dresser->right - 3, dresser->top + kDresserTopThick, k8Red4Color);
	
	ColorLine(backSrcMap, dresser->left + kDresserCrease, dresser->top + kDresserTopThick + 1,
			dresser->left + kDresserCrease, dresser->bottom - 4, k8Red4Color);
	ColorLine(backSrcMap, dresser->right - kDresserCrease, dresser->top + kDresserTopThick + 1,
			dresser->right - kDresserCrease, dresser->bottom - 4, k8OrangeColor);
	
	nRects = RectTall(dresser) / 30;
	if (nRects == 0)
		nRects = 1;
	height = (RectTall(dresser) - 14) / nRects - 4;
	QSetRect(&tempRect, 0, 0, RectWide(dresser) - kDresserSideSpare, height);
	QOffsetRect(&tempRect, dresser->left + 7, dresser->top + 10);
	for (i = 0; i < nRects; i++)
	{
		ColorLine(backSrcMap, tempRect.left + 1, tempRect.bottom,
				tempRect.right, tempRect.bottom, dkstRedC);
		ColorLine(backSrcMap, tempRect.right, tempRect.top + 1,
				tempRect.right, tempRect.bottom, dkstRedC);
		ColorRect(backSrcMap, tempRect, yellowC);
		HiliteRect(backSrcMap, tempRect, ltTanC, brownC);
		InsetRect(&tempRect, 1, 1);
		HiliteRect(backSrcMap, tempRect, ltTanC, brownC);
		InsetRect(&tempRect, -1, -1);
		
		QSetRect(&dest, -4, -4, 4, 4);
		QOffsetRect(&dest, HalfRectTall(&tempRect), HalfRectTall(&tempRect));
		QOffsetRect(&dest, tempRect.left, tempRect.top);
		CopyBits((BitMap *)*GetGWorldPixMap(furnitureSrcMap), 
				(BitMap *)*GetGWorldPixMap(backSrcMap), 
				&knobSrc, &dest, srcCopy);
		
		QSetRect(&dest, -4, -4, 4, 4);
		QOffsetRect(&dest, -HalfRectTall(&tempRect), HalfRectTall(&tempRect));
		QOffsetRect(&dest, tempRect.right, tempRect.top);
		CopyBits((BitMap *)*GetGWorldPixMap(furnitureSrcMap), 
				(BitMap *)*GetGWorldPixMap(backSrcMap), 
				&knobSrc, &dest, srcCopy);
		
		QOffsetRect(&tempRect, 0, kDresserTopThick + height);
	}
	
	dest = leftFootSrc;
	ZeroRectCorner(&dest);
	QOffsetRect(&dest, dresser->left + 6, dresser->bottom - 2);
	
	CopyMask((BitMap *)*GetGWorldPixMap(furnitureSrcMap), 
			(BitMap *)*GetGWorldPixMap(furnitureMaskMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&leftFootSrc, &leftFootSrc, &dest);
	
	dest = rightFootSrc;
	ZeroRectCorner(&dest);
	QOffsetRect(&dest, dresser->right - 19, dresser->bottom - 2);
	
	CopyMask((BitMap *)*GetGWorldPixMap(furnitureSrcMap), 
			(BitMap *)*GetGWorldPixMap(furnitureMaskMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&rightFootSrc, &rightFootSrc, &dest);
}

//--------------------------------------------------------------  DrawDeckTable

void DrawDeckTable (Rect *tableTop, short down)
{
	#define		kTableBaseTop		296
	#define		kTableShadowTop		312
	#define		kTableShadowOffset	12
	
	Rect		tempRect;
	long		bambooC, brownC, dkGrayC;
	short		hCenter, vShadow;
	DrawSurface	*wasCPort;
	Pattern		dummyPattern;
	
	if (thisMac.isDepth == 4)
	{
		bambooC = 6;
		brownC = 11;
		dkGrayC = 14;
	}
	else
	{
		bambooC = k8BambooColor;
		brownC = k8BrownColor;
		dkGrayC = k8DkstGrayColor;
	}
	
	QSetRect(&tempRect, tableTop->left, 0, tableTop->right, 
			RectWide(tableTop) / 10);
	QOffsetRect(&tempRect, 0, 
			-HalfRectTall(&tempRect) + kTableShadowTop + down);
	QOffsetRect(&tempRect, kTableShadowOffset, -kTableShadowOffset);
	GetQDGlobalsGray(&dummyPattern);
	ColorOvalMaskPattern(backSrcMap, tempRect, dkGrayC, dummyPattern);
	
	InsetRect(tableTop, 0, 1);
	ColorRect(backSrcMap, *tableTop, kGoldColor);
	InsetRect(tableTop, 0, -1);
	
	ColorLine(backSrcMap, tableTop->left, tableTop->top + 1,
			tableTop->left, tableTop->top + 1, k8WhiteColor);
	ColorLine(backSrcMap, tableTop->left + 1, tableTop->top,
			tableTop->right - 2, tableTop->top, k8WhiteColor);
	ColorLine(backSrcMap, tableTop->right - 1, tableTop->top + 1,
			tableTop->right - 1, tableTop->top + 1, k8WhiteColor);
	
	ColorLine(backSrcMap, tableTop->left + 1, tableTop->top + 1,
			tableTop->right - 2, tableTop->top + 1, kYellowColor);
	ColorLine(backSrcMap, tableTop->left, tableTop->top + 2,
			tableTop->left, tableTop->bottom - 2, kYellowColor);
	
	ColorLine(backSrcMap, tableTop->left + 1, tableTop->bottom - 1,
			tableTop->right - 2, tableTop->bottom - 1, brownC);
	ColorLine(backSrcMap, tableTop->right - 1, tableTop->top + 2,
			tableTop->right - 1, tableTop->bottom - 2, brownC);
	
	ColorLine(backSrcMap, tableTop->left + 1, tableTop->bottom - 2,
			tableTop->right - 2, tableTop->bottom - 2, bambooC);
	
	if (tableTop->bottom < kTableBaseTop + down)
	{
		hCenter = (tableTop->left + tableTop->right) / 2;
		
		ColorLine(backSrcMap, hCenter - 3, tableTop->bottom,
				hCenter - 3, kTableBaseTop + down, dkGrayC);
		ColorLine(backSrcMap, hCenter - 2, tableTop->bottom,
				hCenter - 2, kTableBaseTop + down, k8WhiteColor);
		ColorLine(backSrcMap, hCenter - 1, tableTop->bottom,
				hCenter - 1, kTableBaseTop + down, k8WhiteColor);
		ColorLine(backSrcMap, hCenter, tableTop->bottom,
				hCenter, kTableBaseTop + down, k8LtGrayColor);
		ColorLine(backSrcMap, hCenter + 1, tableTop->bottom,
				hCenter + 1, kTableBaseTop + down, dkGrayC);
		
		vShadow = tableTop->bottom + RectWide(tableTop) / 4 - 2;
		if (vShadow > kTableBaseTop + down)
		{
			ColorLine(backSrcMap, hCenter - 2, tableTop->bottom,
					hCenter - 2, kTableBaseTop + down, k8LtGrayColor);
			ColorLine(backSrcMap, hCenter - 1, tableTop->bottom,
					hCenter - 1, kTableBaseTop + down, k8LtGrayColor);
			ColorLine(backSrcMap, hCenter, tableTop->bottom,
					hCenter, kTableBaseTop + down, dkGrayC);
		}
		else
		{
			ColorLine(backSrcMap, hCenter - 2, tableTop->bottom,
					hCenter - 2, vShadow, k8LtGrayColor);
			ColorLine(backSrcMap, hCenter - 1, tableTop->bottom,
					hCenter - 1, vShadow + 1, k8LtGrayColor);
			ColorLine(backSrcMap, hCenter, tableTop->bottom,
					hCenter, vShadow + 2, dkGrayC);
		}
	}
	
	tempRect = deckSrc;
	ZeroRectCorner(&tempRect);
	QOffsetRect(&tempRect, -HalfRectWide(&deckSrc) + tableTop->left + 
			HalfRectWide(tableTop), kTableBaseTop + down);
	CopyMask((BitMap *)*GetGWorldPixMap(furnitureSrcMap), 
			(BitMap *)*GetGWorldPixMap(furnitureMaskMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&deckSrc, &deckSrc, &tempRect);
}

//--------------------------------------------------------------  DrawStool

void DrawStool (Rect *theRect, short down)
{
	#define		kStoolBase	304
	long		grayC, dkGrayC;
	
	if (thisMac.isDepth == 4)
	{
		grayC = 13;
		dkGrayC = 14;
	}
	else
	{
		grayC = k8DkGray2Color;
		dkGrayC = k8DkstGrayColor;
	}
	
	if (theRect->bottom < kStoolBase + down)
	{
		ColorLine(backSrcMap, theRect->left + 21, theRect->bottom - 1,
				theRect->left + 21, kStoolBase + down - 1, k8DkGrayColor);
		ColorLine(backSrcMap, theRect->left + 22, theRect->bottom - 1,
				theRect->left + 22, kStoolBase + down, k8Gray2Color);
		ColorLine(backSrcMap, theRect->left + 23, theRect->bottom - 1,
				theRect->left + 23, kStoolBase + down, k8DkGrayColor);
		ColorLine(backSrcMap, theRect->left + 24, theRect->bottom - 1,
				theRect->left + 24, kStoolBase + down, k8DkGray3Color);
		ColorLine(backSrcMap, theRect->left + 25, theRect->bottom - 1,
				theRect->left + 25, kStoolBase + down, grayC);
		ColorLine(backSrcMap, theRect->left + 26, theRect->bottom - 1,
				theRect->left + 26, kStoolBase + down - 1, dkGrayC);
	}
	
	CopyMask((BitMap *)*GetGWorldPixMap(furnitureSrcMap), 
			(BitMap *)*GetGWorldPixMap(furnitureMaskMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&srcRects[kStool], &srcRects[kStool], theRect);
}

//--------------------------------------------------------------  DrawInvisObstacle

void DrawInvisObstacle (Rect *theRect)
{	
	ColorFrameRect(backSrcMap, *theRect, k8BrownColor);
}

//--------------------------------------------------------------  DrawInvisBounce

void DrawInvisBounce (Rect *theRect)
{
	ColorFrameRect(backSrcMap, *theRect, k8RedColor);
}

//--------------------------------------------------------------  DrawRedClock

void DrawRedClock (Rect *theRect)
{
	DateTimeRec	timeRec;
	Rect		dest;
	short		hour, minutes;
	
	CopyMask((BitMap *)*GetGWorldPixMap(bonusSrcMap), 
			(BitMap *)*GetGWorldPixMap(bonusMaskMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&srcRects[kRedClock], &srcRects[kRedClock], theRect);
	
	GetTime(&timeRec);
	hour = timeRec.hour % 12;
	if (hour == 0)
		hour = 12;
	minutes = timeRec.minute;
	
	QSetRect(&dest, 0, 0, 4, 6);
	QOffsetRect(&dest, theRect->left + 5, theRect->top + 7);
	if (hour > 9)
		DrawClockDigit(hour / 10, &dest);
	QOffsetRect(&dest, 4, 0);
	DrawClockDigit(hour % 10, &dest);
	QOffsetRect(&dest, 6, 0);
	DrawClockDigit(minutes / 10, &dest);
	QOffsetRect(&dest, 4, 0);
	DrawClockDigit(minutes % 10, &dest);
}

//--------------------------------------------------------------  DrawClockDigit

void DrawClockDigit (short number, Rect *dest)
{
	CopyBits((BitMap *)*GetGWorldPixMap(bonusSrcMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&digits[number], dest, srcCopy);
}

//--------------------------------------------------------------  DrawBlueClock

void DrawBlueClock (Rect *theRect)
{
	DateTimeRec	timeRec;
	Point		dest;
	short		hour, minutes;
	
	CopyMask((BitMap *)*GetGWorldPixMap(bonusSrcMap), 
			(BitMap *)*GetGWorldPixMap(bonusMaskMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&srcRects[kBlueClock], &srcRects[kBlueClock], theRect);
	
	dest.h = theRect->left + 13;
	dest.v = theRect->top + 13;
	GetTime(&timeRec);
	hour = timeRec.hour % 12;
	minutes = ((timeRec.minute + 2) / 5) % 12;
	DrawClockHands(dest, minutes, hour);
}

//--------------------------------------------------------------  DrawYellowClock

void DrawYellowClock (Rect *theRect)
{
	DateTimeRec	timeRec;
	Point		dest;
	short		hour, minutes;
	
	CopyMask((BitMap *)*GetGWorldPixMap(bonusSrcMap), 
			(BitMap *)*GetGWorldPixMap(bonusMaskMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&srcRects[kYellowClock], &srcRects[kYellowClock], theRect);
	
	dest.h = theRect->left + 13;
	dest.v = theRect->top + 15;
	GetTime(&timeRec);
	hour = timeRec.hour % 12;
	minutes = ((timeRec.minute + 2) / 5) % 12;
	DrawClockHands(dest, minutes, hour);
}

//--------------------------------------------------------------  DrawCuckoo

void DrawCuckoo (Rect *theRect)
{
	DateTimeRec	timeRec;
	Point		dest;
	short		hour, minutes;
	
	CopyMask((BitMap *)*GetGWorldPixMap(bonusSrcMap), 
			(BitMap *)*GetGWorldPixMap(bonusMaskMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&srcRects[kCuckoo], &srcRects[kCuckoo], theRect);
	
	dest.h = theRect->left + 19;
	dest.v = theRect->top + 31;
	GetTime(&timeRec);
	hour = timeRec.hour % 12;
	minutes = ((timeRec.minute + 2) / 5) % 12;
	DrawLargeClockHands(dest, minutes, hour);
}

//--------------------------------------------------------------  DrawClockHands

void DrawClockHands (Point where, short bigHand, short littleHand)
{
	DrawSurface	*surface = backSrcMap;

	PortabilityLayer::ResolveCachingColor blackColor = StdColors::Black();
	const Point midPoint = Point::Create(where.h, where.v);
	Point otherPoint = midPoint;

	switch (bigHand)
	{
		case 0:
		otherPoint = midPoint + Point::Create(0, -6);
		break;
		
		case 1:
		otherPoint = midPoint + Point::Create(3, -5);
		break;
		
		case 2:
		otherPoint = midPoint + Point::Create(5, -3);
		break;
		
		case 3:
		otherPoint = midPoint + Point::Create(6, 0);
		break;
		
		case 4:
		otherPoint = midPoint + Point::Create(5, 3);
		break;
		
		case 5:
		otherPoint = midPoint + Point::Create(3, 5);
		break;
		
		case 6:
		otherPoint = midPoint + Point::Create(0, 6);
		break;
		
		case 7:
		otherPoint = midPoint + Point::Create(-3, 5);
		break;
		
		case 8:
		otherPoint = midPoint + Point::Create(-5, 3);
		break;
		
		case 9:
		otherPoint = midPoint + Point::Create(-6, 0);
		break;
		
		case 10:
		otherPoint = midPoint + Point::Create(-5, -3);
		break;
		
		case 11:
		otherPoint = midPoint + Point::Create(-3, -5);
		break;
	}

	surface->DrawLine(midPoint, otherPoint, blackColor);
	
	switch (littleHand)
	{
		case 0:
		otherPoint = midPoint + Point::Create(0, -4);
		break;
		
		case 1:
		otherPoint = midPoint + Point::Create(2, -3);
		break;
		
		case 2:
		otherPoint = midPoint + Point::Create(3, -2);
		break;
		
		case 3:
		otherPoint = midPoint + Point::Create(4, 0);
		break;
		
		case 4:
		otherPoint = midPoint + Point::Create(3, 2);
		break;
		
		case 5:
		otherPoint = midPoint + Point::Create(2, 3);
		break;
		
		case 6:
		otherPoint = midPoint + Point::Create(0, 4);
		break;
		
		case 7:
		otherPoint = midPoint + Point::Create(-2, 3);
		break;
		
		case 8:
		otherPoint = midPoint + Point::Create(-3, 2);
		break;
		
		case 9:
		otherPoint = midPoint + Point::Create(-4, 0);
		break;
		
		case 10:
		otherPoint = midPoint + Point::Create(-3, -2);
		break;
		
		case 11:
		otherPoint = midPoint + Point::Create(-2, -3);
		break;
	}

	surface->DrawLine(midPoint, otherPoint, blackColor);
}

//--------------------------------------------------------------  DrawClockHands

void DrawLargeClockHands (Point where, short bigHand, short littleHand)
{
	DrawSurface	*surface = backSrcMap;

	PortabilityLayer::ResolveCachingColor whiteColor = StdColors::White();
	
	const Point midPoint = Point::Create(where.h, where.v);
	Point otherPoint = midPoint;

	switch (bigHand)
	{
		case 0:
		otherPoint = midPoint + Point::Create(0, -10);
		break;
		
		case 1:
		otherPoint = midPoint + Point::Create(5, -9);
		break;
		
		case 2:
		otherPoint = midPoint + Point::Create(9, -5);
		break;
		
		case 3:
		otherPoint = midPoint + Point::Create(10, 0);
		break;
		
		case 4:
		otherPoint = midPoint + Point::Create(9, 5);
		break;
		
		case 5:
		otherPoint = midPoint + Point::Create(5, 9);
		break;
		
		case 6:
		otherPoint = midPoint + Point::Create(0, 10);
		break;
		
		case 7:
		otherPoint = midPoint + Point::Create(-5, 9);
		break;
		
		case 8:
		otherPoint = midPoint + Point::Create(-9, 5);
		break;
		
		case 9:
		otherPoint = midPoint + Point::Create(-10, 0);
		break;
		
		case 10:
		otherPoint = midPoint + Point::Create(-9, -5);
		break;
		
		case 11:
		otherPoint = midPoint + Point::Create(-5, -9);
		break;
	}

	surface->DrawLine(midPoint, otherPoint, whiteColor);
	
	switch (littleHand)
	{
		case 0:
		otherPoint = midPoint + Point::Create(0, -6);
		break;
		
		case 1:
		otherPoint = midPoint + Point::Create(3, -5);
		break;
		
		case 2:
		otherPoint = midPoint + Point::Create(5, -3);
		break;
		
		case 3:
		otherPoint = midPoint + Point::Create(6, 0);
		break;
		
		case 4:
		otherPoint = midPoint + Point::Create(5, 3);
		break;
		
		case 5:
		otherPoint = midPoint + Point::Create(3, 5);
		break;
		
		case 6:
		otherPoint = midPoint + Point::Create(0, 6);
		break;
		
		case 7:
		otherPoint = midPoint + Point::Create(-3, 5);
		break;
		
		case 8:
		otherPoint = midPoint + Point::Create(-5, 3);
		break;
		
		case 9:
		otherPoint = midPoint + Point::Create(-6, 0);
		break;
		
		case 10:
		otherPoint = midPoint + Point::Create(-5, -3);
		break;
		
		case 11:
		otherPoint = midPoint + Point::Create(-3, -5);
		break;
	}

	surface->DrawLine(midPoint, otherPoint, whiteColor);
}

//--------------------------------------------------------------  DrawSimplePrizes

void DrawSimplePrizes (short what, Rect *theRect)
{
	CopyMask((BitMap *)*GetGWorldPixMap(bonusSrcMap), 
			(BitMap *)*GetGWorldPixMap(bonusMaskMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&srcRects[what], &srcRects[what], theRect);
}

//--------------------------------------------------------------  DrawGreaseRt

void DrawGreaseRt (Rect *theRect, short distance, Boolean state, Boolean drawSpill)
{
	Rect		spill, dest;
	
	dest = *theRect;
	if (state)		// grease upright
	{
		CopyMask((BitMap *)*GetGWorldPixMap(bonusSrcMap), 
				(BitMap *)*GetGWorldPixMap(bonusMaskMap), 
				(BitMap *)*GetGWorldPixMap(backSrcMap), 
				&greaseSrcRt[0], &greaseSrcRt[0], &dest);
	}
	else			// grease spilled
	{
		QOffsetRect(&dest, 6, 0);
		CopyMask((BitMap *)*GetGWorldPixMap(bonusSrcMap), 
				(BitMap *)*GetGWorldPixMap(bonusMaskMap), 
				(BitMap *)*GetGWorldPixMap(backSrcMap), 
				&greaseSrcRt[3], &greaseSrcRt[3], &dest);

		if (drawSpill)
		{
			QSetRect(&spill, 0, -2, distance - 5, 0);
			QOffsetRect(&spill, dest.right - 1, dest.bottom);

			PortabilityLayer::ResolveCachingColor blackColor = StdColors::Black();
			backSrcMap->FillRect(spill, blackColor);
		}
	}
}

//--------------------------------------------------------------  DrawGreaseLf

void DrawGreaseLf (Rect *theRect, short distance, Boolean state, Boolean drawSpill)
{
	Rect		spill, dest;
	DrawSurface	*wasCPort;
	
	dest = *theRect;
	if (state)		// grease upright
	{
		CopyMask((BitMap *)*GetGWorldPixMap(bonusSrcMap), 
				(BitMap *)*GetGWorldPixMap(bonusMaskMap), 
				(BitMap *)*GetGWorldPixMap(backSrcMap), 
				&greaseSrcLf[0], &greaseSrcLf[0], &dest);
	}
	else			// grease spilled
	{
		QOffsetRect(&dest, -6, 0);
		CopyMask((BitMap *)*GetGWorldPixMap(bonusSrcMap), 
				(BitMap *)*GetGWorldPixMap(bonusMaskMap), 
				(BitMap *)*GetGWorldPixMap(backSrcMap), 
				&greaseSrcLf[3], &greaseSrcLf[3], &dest);

		if (drawSpill)
		{
			PortabilityLayer::ResolveCachingColor blackColor = StdColors::Black();

			QSetRect(&spill, -distance + 5, -2, 0, 0);
			QOffsetRect(&spill, dest.left + 1, dest.bottom);
			backSrcMap->FillRect(spill, blackColor);
		}
	}
}

//--------------------------------------------------------------  DrawBands

void DrawFoil (Rect *theRect)
{
	CopyMask((BitMap *)*GetGWorldPixMap(bonusSrcMap), 
			(BitMap *)*GetGWorldPixMap(bonusMaskMap), 
			(BitMap *)*GetGWorldPixMap(backSrcMap), 
			&srcRects[kFoil], &srcRects[kFoil], theRect);
}

//--------------------------------------------------------------  DrawInvisBonus

void DrawInvisBonus (Rect *theRect)
{
	DrawSurface	*wasCPort;

	ColorFrameOval(backSrcMap, *theRect, 227);
}

//--------------------------------------------------------------  DrawSlider

void DrawSlider (Rect *theRect)
{
	PortabilityLayer::ResolveCachingColor blackColor = StdColors::Black();
	backSrcMap->FrameRect(*theRect, blackColor);
}

