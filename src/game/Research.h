#pragma once

// Research helpers bound to F9 in game. They write files next to the plugin so that
// unknown stat ids / script globals can be identified by comparing dumps taken while
// an in-game value (money, honor...) is known. Not used by the presence itself.

namespace Research
{
	// Must be called from the RedHook script fiber.
	// - <dir>RDRBetterPresence.stats.txt   : every SAG player stat as int and float
	// - <dir>RDRBetterPresence.scan.txt    : every 4-byte slot of the process holding `needle` (background thread);
	//                                        take two scans with different known values and intersect the addresses
	void Dump(const std::string& dir, int needle);
}
