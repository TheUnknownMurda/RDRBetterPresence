#pragma once

#include "Scripts.h"

// Snapshot of everything the presence needs, sampled on the RedHook script fiber
// (natives may only be called from there) and consumed by the Discord worker thread.

enum class PlayerCharacter
{
	Unknown,
	John,
	Jack
};

enum class MountKind
{
	None,
	Horse,
	Mule,
	Bull,
	Buffalo,
	Other
};

struct GameSnapshot
{
	// When false the player does not exist yet: main menu, loading screen, etc.
	bool playerValid = false;

	bool paused = false;
	bool cutscene = false;
	bool dead = false;
	bool inCombat = false;
	bool deadEye = false;
	int deadEyePoints = 0;
	bool minigame = false;
	bool hogtied = false;
	bool lassoing = false;
	bool onTrain = false;
	bool inVehicle = false;
	bool drivingVehicle = false;
	bool riding = false;
	MountKind mount = MountKind::None;
	bool inWater = false;
	bool inRoom = false;
	bool usingCover = false;
	bool drunk = false;
	bool handsUp = false;

	bool weaponDrawn = false;
	WeaponModel weapon = WEAPON_INVALID;
	WeaponCategory weaponCategory = WEAPON_CATEGORY_INVALID;
	std::string weaponName;       // as returned by the game (display name or GXT label)

	float health = 0.0f;
	float maxHealth = 0.0f;

	int hour = 0;
	int minute = 0;
	Weather weather = WEATHER_CLEAR;

	std::string district;         // raw GET_DISTRICTS_NAME() value
	float posX = 0.0f, posY = 0.0f, posZ = 0.0f;

	PlayerCharacter character = PlayerCharacter::Unknown;
	int outfit = -1;
	int gameState = 0;
	std::string playerName;

	// Player stats (SAG player stat ids documented by the community: 0 money, 1 honor, 3 fame, 222 bounty)
	int money = 0;
	int honor = 0;
	int fame = 0;
	int bounty = 0;

	// Scripted activity (story mission, stranger, minigame...). Refreshed less often than the rest.
	ActiveScript script;

	bool operator==(const GameSnapshot&) const = default;
};

namespace GameState
{
	// Must be called from the RedHook script fiber. `previous` supplies the fields that are
	// refreshed only every few samples (script detection).
	GameSnapshot Sample(const GameSnapshot& previous, bool refreshScripts);

	// Research aid (F9 in game): writes every SAG player stat as int and float to `path`.
	bool DumpStats(const std::string& path);
}
