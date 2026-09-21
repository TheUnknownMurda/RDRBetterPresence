#include "pch.h"
#include "GameState.h"
#include "../Log.h"

namespace
{
	// Plain-old-data mirror of GameSnapshot so the sampling code can live inside a
	// __try/__except block (MSVC forbids objects with destructors in such functions).
	struct RawSnapshot
	{
		bool playerValid;
		bool paused, cutscene, dead, inCombat, deadEye, minigame, hogtied, lassoing;
		bool onTrain, inVehicle, drivingVehicle, riding, inWater, inRoom, usingCover, drunk, handsUp;
		int deadEyePoints;
		MountKind mount;
		bool weaponDrawn;
		int weapon;
		int weaponCategory;
		char weaponName[64];
		float health, maxHealth;
		int hour, minute;
		int weather;
		char district[64];
		float posX, posY, posZ;
		PlayerCharacter character;
		int outfit;
		int gameState;
		char playerName[64];
		int bounty;
		int journalTarget, lastObjective, testMission, validScripts;
	};

	// "Active bounty total" stat id (Foxxyyy, RDR_Stats.c); verified in-game.
	constexpr int kStatBounty = 222;

	void CopyStr(char* dst, size_t dstSize, const char* src)
	{
		if (!src)
		{
			dst[0] = '\0';
			return;
		}
		strncpy_s(dst, dstSize, src, _TRUNCATE);
	}

	MountKind ClassifyMount(Actor mount)
	{
		if (mount <= 0)
		{
			return MountKind::None;
		}
		if (RIDING::IS_ACTOR_HORSE(mount))
		{
			return MountKind::Horse;
		}
		if (RIDING::IS_ACTOR_MULE(mount))
		{
			return MountKind::Mule;
		}

		switch (OBJECT::GET_ACTOR_ENUM(mount))
		{
			case ACTOR_RIDEABLE_ANIMAL_Bull04:
			case ACTOR_RIDEABLE_ANIMAL_Bull05:
			case ACTOR_RIDEABLE_ANIMAL_Bull:
			case ACTOR_RIDEABLE_ANIMAL_Bull01:
			case ACTOR_RIDEABLE_ANIMAL_Bull02:
				return MountKind::Bull;
			case ACTOR_RIDEABLE_ANIMAL_Buffalo:
			case ACTOR_RIDEABLE_ANIMAL_Buffalo04:
				return MountKind::Buffalo;
			default:
				return MountKind::Other;
		}
	}

	// Everything that touches natives lives here.
	void SampleRaw(RawSnapshot& r)
	{
		r.paused = CORE::IS_GAME_PAUSED();
		r.gameState = GAME::GET_GAME_STATE();

		Actor player = ACTOR::GET_PLAYER_ACTOR(-1);
		if (player <= 0 || !ACTOR::IS_LOCAL_PLAYER_VALID(player))
		{
			r.playerValid = false;
			return;
		}
		r.playerValid = true;

		// World
		Time now = TIME::GET_TIME_OF_DAY();
		r.hour = TIME::GET_HOUR(now);
		r.minute = TIME::GET_MINUTE(now);
		r.weather = WEATHER::GET_WEATHER();
		CopyStr(r.district, sizeof(r.district), CORE::GET_DISTRICTS_NAME());
		CopyStr(r.playerName, sizeof(r.playerName), CORE::GET_LOCAL_PLAYER_NAME());

		// Player
		Vector3 pos = ACTOR::GET_POSITION(player);
		r.posX = pos.x; r.posY = pos.y; r.posZ = pos.z;

		int model = OBJECT::GET_ACTOR_ENUM(player);
		r.character = (model == ACTOR_PLAYER_JACK) ? PlayerCharacter::Jack
		            : (model == ACTOR_PLAYER || model == ACTOR_PLAYER_cs) ? PlayerCharacter::John
		            : PlayerCharacter::Unknown;
		r.outfit = OBJECT::GET_CURRENT_ACTOR_ENUM_VARIATION(player);

		r.dead = HEALTH::IS_ACTOR_DEAD(player);
		r.health = HEALTH::GET_ACTOR_HEALTH(player);
		r.maxHealth = HEALTH::GET_ACTOR_MAX_HEALTH(player);
		r.drunk = HEALTH::IS_ACTOR_DRUNK(player);

		r.cutscene = CAMERA::IS_CUTSCENE_TUNER_PLAYINGBACK() != 0 || CORE::IS_D11_CUTSCENE_HACK();
		r.inCombat = ACTOR::IS_PLAYER_IN_COMBAT(player) != 0;
		r.deadEye = ACTOR::IS_PLAYER_DEADEYE(player);
		// The SDK declares this native as int, but the game returns a float (observed in-game:
		// 0x427A9981 = 62.65). Invoke it with the right return type and round.
		r.deadEyePoints = (int)(Invoke<0x86B5C9E1, float>(player) + 0.5f);
		r.usingCover = ACTOR::IS_PLAYER_USING_COVER(player) != 0;
		r.handsUp = ACTOR::IS_ACTOR_HANDSUP(player) != 0;
		r.inRoom = ACTOR::IS_ACTOR_IN_ROOM(player) != 0;
		r.inWater = ENTITY::IS_ACTOR_IN_WATER(player);
		r.minigame = MINIGAME::IS_MINIGAME_RUNNING();

		r.hogtied = LASSO::IS_ACTOR_HOGTIED(player) != 0;
		r.lassoing = LASSO::GET_LASSO_TARGET(player) > 0;

		// Transport
		r.onTrain = VEHICLES::IS_ACTOR_ON_TRAIN(player, false);
		r.inVehicle = VEHICLES::IS_ACTOR_INSIDE_VEHICLE(player);
		r.drivingVehicle = r.inVehicle && VEHICLES::IS_ACTOR_DRIVING_VEHICLE(player);
		r.riding = RIDING::IS_ACTOR_RIDING(player) || RIDING::IS_ACTOR_MOUNTED(player);
		r.mount = r.riding ? ClassifyMount(RIDING::GET_MOUNT(player)) : MountKind::None;

		// Weapon
		r.weaponDrawn = INVENTORY::IS_WEAPON_DRAWN(player) != 0;
		r.weapon = INVENTORY::GET_WEAPON_IN_HAND(player);
		r.weaponCategory = WEAPON_CATEGORY_INVALID;
		r.weaponName[0] = '\0';
		if (r.weapon > WEAPON_INVALID && r.weapon < NUM_WEAPONS)
		{
			r.weaponCategory = WEAPON::GET_WEAPON_CATEGORY_FROM_ENUM((WeaponModel)r.weapon);
			CopyStr(r.weaponName, sizeof(r.weaponName), WEAPON::GET_WEAPON_DISPLAY_NAME((WeaponModel)r.weapon));
		}

		// Stats are stored as floats (observed in-game: the int native returned 0x42960000 = 75.0 for a $75 bounty).
		r.bounty = (int)(STAT::GET_SAGPLAYER_STAT_FLOAT(kStatBounty) + 0.5f);

		// Mission research
		r.journalTarget = JOURNAL::GET_TARGETED_JOURNAL_ENTRY();
		r.lastObjective = JOURNAL::GET_LAST_NOTE_OBJECTIVE();
		r.testMission = CORE::SCRIPT_GETTESTMISSION();
		r.validScripts = 0;
		for (int id = 0; id < 128; ++id)
		{
			if (CORE::IS_SCRIPT_VALID(id)) ++r.validScripts;
		}
	}

	// Returns false if a native blew up (access violation etc.) - the game keeps running
	// and we simply report "unknown" for this tick.
	bool SampleGuarded(RawSnapshot& r)
	{
		__try
		{
			SampleRaw(r);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	int DetectScriptGuarded()
	{
		__try
		{
			return Scripts::DetectIndex();
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return -2;
		}
	}
}

GameSnapshot GameState::Sample(const GameSnapshot& previous, bool refreshScripts)
{
	RawSnapshot r{};
	static int s_failures = 0;

	if (!SampleGuarded(r))
	{
		if (++s_failures <= 5)
		{
			Log::Error("Exception while sampling game state (occurrence %d) - a native may have changed in this game build", s_failures);
		}
		return GameSnapshot{};
	}

	GameSnapshot s;
	s.playerValid = r.playerValid;
	s.paused = r.paused;
	s.gameState = r.gameState;
	if (!r.playerValid)
	{
		return s;
	}

	s.cutscene = r.cutscene;
	s.dead = r.dead;
	s.inCombat = r.inCombat;
	s.deadEye = r.deadEye;
	s.deadEyePoints = r.deadEyePoints;
	s.minigame = r.minigame;
	s.hogtied = r.hogtied;
	s.lassoing = r.lassoing;
	s.onTrain = r.onTrain;
	s.inVehicle = r.inVehicle;
	s.drivingVehicle = r.drivingVehicle;
	s.riding = r.riding;
	s.mount = r.mount;
	s.inWater = r.inWater;
	s.inRoom = r.inRoom;
	s.usingCover = r.usingCover;
	s.drunk = r.drunk;
	s.handsUp = r.handsUp;
	s.weaponDrawn = r.weaponDrawn;
	s.weapon = (WeaponModel)r.weapon;
	s.weaponCategory = (WeaponCategory)r.weaponCategory;
	s.weaponName = r.weaponName;
	s.health = r.health;
	s.maxHealth = r.maxHealth;
	s.hour = r.hour;
	s.minute = r.minute;
	s.weather = (Weather)r.weather;
	s.district = r.district;
	s.posX = r.posX; s.posY = r.posY; s.posZ = r.posZ;
	s.character = r.character;
	s.outfit = r.outfit;
	s.playerName = r.playerName;
	s.bounty = r.bounty;
	s.journalTarget = r.journalTarget;
	s.lastObjective = r.lastObjective;
	s.testMission = r.testMission;
	s.validScripts = r.validScripts;

	// Script detection is ~200 native calls, so it is refreshed every few samples only.
	static bool s_scriptsDisabled = false;
	s.script = previous.script;
	if (refreshScripts && !s_scriptsDisabled)
	{
		int index = DetectScriptGuarded();
		if (index == -2)
		{
			Log::Error("Exception during script detection - mission names disabled for this session");
			s_scriptsDisabled = true;
			s.script = ActiveScript{};
		}
		else
		{
			s.script = Scripts::Describe(index);
		}
	}
	return s;
}

