#pragma once

#include "GameState.h"

// Turns the raw game data into a displayable location.
//
// Primary source: CORE::GET_DISTRICTS_NAME(). Fallback: rectangles listed in
// RDRBetterPresence.regions.ini ([Regions] Name=xmin,ymin,xmax,ymax), which lets
// the user add towns / landmarks by coordinates without recompiling.

struct RegionInfo
{
	std::string name;   // display name
	std::string slug;   // asset-key friendly: lowercase ascii, '_' separated
};

namespace Regions
{
	void LoadOverrides(const std::string& iniPath);
	std::optional<RegionInfo> Resolve(const GameSnapshot& snapshot);
	std::string Slugify(std::string_view text);
}
