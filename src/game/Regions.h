#pragma once

#include "GameState.h"

// Turns the player's position into a displayable location.
//
// Red Dead Redemption uses a Y-up world: X grows eastward, Z grows southward, Y is the
// altitude. The game's own CORE::GET_DISTRICTS_NAME() only knows one district for the
// whole single-player map ("swall"), so regions are derived from a built-in table of
// landmarks with known coordinates: the region is the one of the nearest landmark, and
// the landmark's name is reported when the player is within its radius.
//
// Users can add rectangles in RDRBetterPresence.regions.ini
// ([Regions] Name=xmin,zmin,xmax,zmax) which take priority over the built-in table.

struct RegionInfo
{
	std::string region;      // e.g. "Cholla Springs"
	std::string regionSlug;  // e.g. "cholla_springs"  (asset key = region_<slug>)
	std::string place;       // nearest landmark when close enough, else empty (e.g. "Armadillo")
};

namespace Regions
{
	void LoadOverrides(const std::string& iniPath);
	std::optional<RegionInfo> Resolve(const GameSnapshot& snapshot);
	std::string Slugify(std::string_view text);
}
