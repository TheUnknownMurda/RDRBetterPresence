// Smoke test for the Discord IPC layer and the presence builder, runnable without the game.
//
//   tests\run_smoke.ps1 [-ClientId <id>] [-Seconds 20]
//
// Connects to the running Discord client, pushes a few fake game states through the
// exact same PresenceBuilder/DiscordIPC code the plugin uses, then clears the presence.

#include "pch.h"
#include "Config.h"
#include "Log.h"
#include "discord/DiscordIPC.h"
#include "game/GameState.h"
#include "game/Regions.h"
#include "presence/PresenceBuilder.h"

#include <cstdarg>
#include <ctime>

// --- RedHook stubs --------------------------------------------------------------------------
int RH_GetMinorVersion() { return 0; }
int RH_GetMajorVersion() { return 0; }
void ScriptWait(uint64_t ms) { Sleep((DWORD)ms); }
void ScriptRegister(HMODULE, void(*)()) {}
void ScriptUnregister(void(*)()) {}
void ScriptUnregister(HMODULE) {}
void NativeInit(uint32_t) {}
void NativePush64(uint64_t) {}
uint64_t* NativeCall() { static uint64_t zero = 0; return &zero; }
void Print(LogType, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}

// --- Scenarios -------------------------------------------------------------------------------
static GameSnapshot BaseSnapshot()
{
	GameSnapshot s;
	s.playerValid = true;
	s.district = "Cholla Springs";
	s.posX = 2700.0f; s.posY = -1600.0f; s.posZ = 40.0f;
	s.hour = 14; s.minute = 37;
	s.weather = WEATHER_CLOUDY;
	s.health = 850.0f; s.maxHealth = 1000.0f;
	s.character = PlayerCharacter::John;
	s.outfit = 0;
	s.weapon = WEAPON_REVOLVER_Cattleman;
	s.weaponCategory = WEAPON_CATEGORY_REVOLVER;
	s.weaponName = "WEAP_REV_CATTLEMAN";   // deliberately a label, to exercise the fallback table
	s.playerName = "Tester";
	return s;
}

int main(int argc, char** argv)
{
	std::string clientId;
	int seconds = 6;
	for (int i = 1; i < argc; ++i)
	{
		std::string a = argv[i];
		if (a == "--client-id" && i + 1 < argc) clientId = argv[++i];
		else if (a == "--seconds" && i + 1 < argc) seconds = atoi(argv[++i]);
	}
	if (clientId.empty())
	{
		fprintf(stderr, "usage: ipc_smoke --client-id <discord application id> [--seconds N]\n");
		return 2;
	}

	Log::Init("ipc_smoke.log", LogLevel::Debug, true, false);

	Config cfg;
	cfg.clientId = clientId;
	cfg.language = "fr";

	// 1. Builder sanity (pure, no Discord needed)
	GameSnapshot menus;
	Activity aMenus = PresenceBuilder::Build(menus, cfg, 0);
	printf("[builder] menus     : details='%s' state='%s' large='%s'\n", aMenus.details.c_str(), aMenus.state.c_str(), aMenus.largeImage.c_str());

	GameSnapshot roam = BaseSnapshot();
	Activity aRoam = PresenceBuilder::Build(roam, cfg, 0);
	printf("[builder] free roam : details='%s' state='%s' large='%s'/'%s' small='%s'/'%s'\n",
		aRoam.details.c_str(), aRoam.state.c_str(), aRoam.largeImage.c_str(), aRoam.largeText.c_str(), aRoam.smallImage.c_str(), aRoam.smallText.c_str());

	GameSnapshot fight = BaseSnapshot();
	fight.inCombat = true; fight.deadEye = true; fight.deadEyePoints = 4; fight.health = 320.0f;
	Activity aFight = PresenceBuilder::Build(fight, cfg, 0);
	printf("[builder] dead eye  : details='%s' small='%s'/'%s'\n", aFight.details.c_str(), aFight.smallImage.c_str(), aFight.smallText.c_str());

	GameSnapshot ride = BaseSnapshot();
	ride.riding = true; ride.mount = MountKind::Horse; ride.weapon = WEAPON_INVALID; ride.weaponCategory = WEAPON_CATEGORY_INVALID; ride.weaponName.clear();
	ride.district = "Rio Bravo"; ride.weather = WEATHER_STORMY; ride.hour = 22; ride.minute = 5;
	Activity aRide = PresenceBuilder::Build(ride, cfg, 0);
	printf("[builder] riding    : details='%s' state='%s' large='%s' small='%s'/'%s'\n",
		aRide.details.c_str(), aRide.state.c_str(), aRide.largeImage.c_str(), aRide.smallImage.c_str(), aRide.smallText.c_str());

	cfg.language = "en";
	Activity aRideEn = PresenceBuilder::Build(ride, cfg, 0);
	printf("[builder] riding EN : details='%s' state='%s'\n", aRideEn.details.c_str(), aRideEn.state.c_str());
	cfg.language = "fr";

	if (Regions::Slugify("MacFarlane's Ranch") != "macfarlane_s_ranch" || Regions::Slugify("R\xC3\xADo Bravo") != "rio_bravo")
	{
		printf("[builder] FAIL: slugify\n");
		return 1;
	}
	printf("[builder] slugify OK\n");

	// 2. Live Discord round trip
	DiscordIPC ipc(clientId);
	if (!ipc.Connect())
	{
		printf("[ipc] connect FAILED: %s\n", ipc.LastError().c_str());
		return 1;
	}
	printf("[ipc] connected + READY\n");

	long long start = (long long)time(nullptr);
	const Activity* sequence[] = { &aRoam, &aFight, &aRide };
	int perStep = seconds / 3; if (perStep < 1) perStep = 1;

	for (const Activity* a : sequence)
	{
		Activity withTimer = *a;
		withTimer.startTimestamp = start;
		if (!ipc.SetActivity(withTimer))
		{
			printf("[ipc] SET_ACTIVITY FAILED: %s\n", ipc.LastError().c_str());
			return 1;
		}
		printf("[ipc] SET_ACTIVITY ok -> '%s' | '%s'\n", withTimer.details.c_str(), withTimer.state.c_str());
		for (int i = 0; i < perStep * 4; ++i)
		{
			ipc.Pump();
			Sleep(250);
		}
	}

	ipc.ClearActivity();
	ipc.Disconnect();
	printf("[ipc] cleared + disconnected\n");
	Log::Shutdown();
	return 0;
}
