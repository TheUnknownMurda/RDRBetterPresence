#pragma once

// Research helpers bound to F9 in game. They write files next to the plugin so that
// unknown stat ids / script globals can be identified by comparing dumps taken while
// an in-game value (money, honor...) is known. Not used by the presence itself.

namespace Research
{
	// Must be called from the RedHook script fiber.
	// - <dir>RDRBetterPresence.stats.txt   : every SAG player stat as int and float
	// - <dir>RDRBetterPresence.globals.txt : layout probe of RedHook's GetGlobalPtr() block and
	//                                        the 4-byte slots holding `needle` (e.g. the money shown in the pause menu)
	void Dump(const std::string& dir, int needle);
}
