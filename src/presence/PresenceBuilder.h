#pragma once

#include "../Config.h"
#include "../discord/DiscordIPC.h"
#include "../game/GameState.h"

namespace PresenceBuilder
{
	// Converts a sampled game state into the activity Discord should display.
	// sessionStart is a unix timestamp (seconds) used for the "elapsed" timer.
	Activity Build(const GameSnapshot& snapshot, const Config& config, long long sessionStart);
}
