#pragma once

// Detects what scripted activity the player is in (story mission, stranger mission,
// minigame, job...) by asking the game which of the known script threads are running.
//
// Script names come from the game's own string tables (e.g. the story mission
// "New Friends, Old Problems" is /content/frontier/missions/ranch01/ranch01). Titles for
// story missions come from the miss<N>d UI ids documented by Foxxyyy, stranger missions
// were matched by hand from their folder names.

enum class ScriptKind
{
	None,
	StoryMission,
	StrangerMission,
	Minigame,
	Job,
	Bounty,
	Duel,
	GangHideout
};

struct ActiveScript
{
	ScriptKind kind = ScriptKind::None;
	std::string name;    // script name as known by the game, e.g. "ranch01"
	std::string title;   // display title, e.g. "New Friends, Old Problems" or "Poker"
	std::string place;   // for minigames: where the table is, e.g. "Armadillo"

	bool operator==(const ActiveScript&) const = default;
};

namespace Scripts
{
	// Must be called from the RedHook script fiber. Returns the index of the highest-priority
	// running known script (story > stranger > bounty/job/duel > minigame), or -1.
	// Kept free of C++ objects so callers can wrap it in __try/__except.
	int DetectIndex();

	// Describes an index returned by DetectIndex (-1 -> kind None).
	ActiveScript Describe(int index);
}
