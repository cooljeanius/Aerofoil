#pragma once


namespace RegionIDs
{
	enum RegionID
	{
		kNone,

		kMenuBar = 1,
		kContent,
		kTitleBar,
		kClose,
		kResize,
	};
}

typedef RegionIDs::RegionID RegionID_t;
