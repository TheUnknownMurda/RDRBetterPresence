#pragma once

// Finds the mission the player is currently in by reading the game's journal.
//
// Verified in-game: when a story mission starts, an entry of type 1 whose handle is
// STRING_TO_HASH("miss<N>_short") is appended to journal list 0 (<N> = the mission's
// UI id, e.g. miss11 = "You Shall Not Give False Testimony, Except for Profit"), and it
// disappears when the mission ends. Available missions sit in list 1 as hash("miss<N>").
// Duels are expected to use the "beat_duel_short" label the same way (unverified).

enum class MissionKind
{
	None,
	Story,
	Duel
};

struct ActiveMission
{
	MissionKind kind = MissionKind::None;
	std::string title;

	bool operator==(const ActiveMission&) const = default;
};

namespace Journal
{
	// Computes the label hashes through the game's own hash native. Script fiber only.
	void Init();

	// Script fiber only. Returns the highest-priority mission found in the journal.
	ActiveMission Detect();
}
