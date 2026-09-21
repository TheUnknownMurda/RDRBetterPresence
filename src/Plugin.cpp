#include "pch.h"
#include "Plugin.h"
#include "Config.h"
#include "Log.h"
#include "discord/DiscordIPC.h"
#include "game/GameState.h"
#include "game/Regions.h"
#include "presence/PresenceBuilder.h"

#include <ctime>

// Two threads cooperate here:
//  * the RedHook script fiber (ScriptMain) samples the game through natives and
//    publishes a GameSnapshot;
//  * a worker thread (DiscordWorker) owns the Discord pipe and pushes the presence.
// Natives are never called from the worker, and the pipe is never touched from the fiber.

namespace
{
	constexpr const char* kPluginVersion = "1.0.0";
	constexpr const char* kIniName = "RDRBetterPresence.ini";
	constexpr const char* kRegionsIniName = "RDRBetterPresence.regions.ini";
	constexpr const char* kLogName = "RDRBetterPresence.log";


	Config g_config;
	long long g_sessionStart = 0;

	std::mutex g_snapshotMutex;
	GameSnapshot g_snapshot;

	// Heartbeat of the script fiber. The game freezes its script VM while paused (pause
	// menu, map, loading), so a stalled heartbeat is how we notice the pause.
	std::atomic<long long> g_lastFiberTickMs = 0;

	long long NowMs()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	std::atomic<bool> g_stop = false;
	HANDLE g_workerThread = nullptr;
	HANDLE g_workerDone = nullptr;

	std::string ModuleDirectory(HMODULE module)
	{
		char path[MAX_PATH];
		DWORD len = GetModuleFileNameA(module, path, MAX_PATH);
		std::string dir(path, len);
		size_t slash = dir.find_last_of("\\/");
		return (slash == std::string::npos) ? "" : dir.substr(0, slash + 1);
	}

	LogLevel ParseLogLevel(const std::string& s)
	{
		if (s == "debug") return LogLevel::Debug;
		if (s == "warning" || s == "warn") return LogLevel::Warning;
		if (s == "error") return LogLevel::Error;
		return LogLevel::Info;
	}

	bool FileExists(const std::string& path)
	{
		DWORD attr = GetFileAttributesA(path.c_str());
		return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
	}

	// ---------------------------------------------------------------------------------------
	// Script fiber
	// ---------------------------------------------------------------------------------------

	void LogInterestingChanges(const GameSnapshot& prev, const GameSnapshot& cur)
	{
		// These raw values are what we need to see once to calibrate the mapping tables.
		if (cur.playerValid && !prev.playerValid)
		{
			Log::Info("Player is now valid: name='%s' character=%d outfit=%d gameState=%d",
				cur.playerName.c_str(), (int)cur.character, cur.outfit, cur.gameState);
		}
		else if (!cur.playerValid && prev.playerValid)
		{
			Log::Info("Player is no longer valid (menus / loading), gameState=%d", cur.gameState);
		}

		if (cur.playerValid)
		{
			std::optional<RegionInfo> before = prev.playerValid ? Regions::Resolve(prev) : std::nullopt;
			std::optional<RegionInfo> after = Regions::Resolve(cur);
			std::string beforeText = before ? before->region + "/" + before->place : "?";
			std::string afterText = after ? after->region + "/" + after->place : "?";
			if (beforeText != afterText)
			{
				Log::Info("Location changed: %s -> %s at x=%.0f z=%.0f (y=%.0f, district='%s')",
					beforeText.c_str(), afterText.c_str(), cur.posX, cur.posZ, cur.posY, cur.district.c_str());
			}
		}

		if (cur.playerValid && cur.weapon != prev.weapon)
		{
			Log::Info("Weapon changed: model=%d category=%d displayName='%s'",
				(int)cur.weapon, (int)cur.weaponCategory, cur.weaponName.c_str());
		}

		if (cur.playerValid && cur.weather != prev.weather)
		{
			Log::Info("Weather changed: %d -> %d", (int)prev.weather, (int)cur.weather);
		}

		if (cur.playerValid && cur.gameState != prev.gameState)
		{
			Log::Info("Game state changed: %d -> %d", prev.gameState, cur.gameState);
		}

		if (cur.playerValid && !(cur.script == prev.script))
		{
			Log::Info("Active script: kind=%d name='%s' title='%s' place='%s'",
				(int)cur.script.kind, cur.script.name.c_str(), cur.script.title.c_str(), cur.script.place.c_str());
		}

		if (cur.playerValid && cur.bounty != prev.bounty)
		{
			Log::Info("Bounty: $%d", cur.bounty);
		}

		if (cur.paused != prev.paused)
		{
			Log::Info("IS_GAME_PAUSED -> %d", cur.paused);
		}
	}

	void ScriptMain()
	{
		Log::MarkScriptThread();
		Log::Info("Script fiber started (poll every %d ms)", g_config.pollIntervalMs);

		GameSnapshot previous;
		unsigned tick = 0;
		while (true)
		{
			Log::FlushToConsole();
			g_lastFiberTickMs = NowMs();

			// Script detection is the expensive part: refresh it every ~2 s.
			bool refreshScripts = (tick++ % (unsigned)std::max(1, 2000 / g_config.pollIntervalMs)) == 0;
			GameSnapshot current = GameState::Sample(previous, refreshScripts);
			LogInterestingChanges(previous, current);

			if (g_config.logLevel == "debug" && current.playerValid)
			{
				Log::Debug("pos=(%.1f,%.1f,%.1f) district='%s' time=%02d:%02d weather=%d hp=%.0f/%.0f combat=%d deadeye=%d riding=%d train=%d vehicle=%d paused=%d cutscene=%d minigame=%d weapon=%d",
					current.posX, current.posY, current.posZ, current.district.c_str(), current.hour, current.minute,
					(int)current.weather, current.health, current.maxHealth, current.inCombat, current.deadEye,
					current.riding, current.onTrain, current.inVehicle, current.paused, current.cutscene,
					current.minigame, (int)current.weapon);
			}

			{
				std::lock_guard<std::mutex> lock(g_snapshotMutex);
				g_snapshot = current;
			}
			previous = std::move(current);

			ScriptWait(g_config.pollIntervalMs);
		}
	}

	// ---------------------------------------------------------------------------------------
	// Discord worker thread
	// ---------------------------------------------------------------------------------------

	DWORD WINAPI DiscordWorker(LPVOID)
	{
		using clock = std::chrono::steady_clock;

		DiscordIPC ipc(g_config.clientId);
		Activity lastSent;
		bool hasSent = false;
		clock::time_point lastSend{};
		clock::time_point lastConnectAttempt{};
		bool loggedMissingDiscord = false;

		while (!g_stop)
		{
			auto now = clock::now();

			if (!ipc.IsConnected())
			{
				if (now - lastConnectAttempt >= std::chrono::milliseconds(g_config.reconnectDelayMs) || lastConnectAttempt == clock::time_point{})
				{
					lastConnectAttempt = now;
					hasSent = false;
					if (ipc.Connect())
					{
						loggedMissingDiscord = false;
					}
					else if (!loggedMissingDiscord)
					{
						Log::Warning("Cannot reach Discord: %s (retrying every %d ms)", ipc.LastError().c_str(), g_config.reconnectDelayMs);
						loggedMissingDiscord = true;
					}
				}
				Sleep(250);
				continue;
			}

			ipc.Pump();

			GameSnapshot snapshot;
			{
				std::lock_guard<std::mutex> lock(g_snapshotMutex);
				snapshot = g_snapshot;
			}

			// A frozen script fiber while the player exists means the game is paused.
			long long lastTick = g_lastFiberTickMs.load();
			bool fiberStalled = lastTick != 0 && (NowMs() - lastTick) > std::max(1500, g_config.pollIntervalMs * 3);
			if (fiberStalled && snapshot.playerValid)
			{
				snapshot.paused = true;
			}

			Activity activity = PresenceBuilder::Build(snapshot, g_config, g_sessionStart);

			bool changed = !hasSent || !(activity == lastSent);
			bool intervalElapsed = !hasSent || (now - lastSend >= std::chrono::milliseconds(g_config.updateIntervalMs));

			if (changed && intervalElapsed)
			{
				if (ipc.SetActivity(activity))
				{
					Log::Info("Presence: [%s] [%s]", activity.details.c_str(), activity.state.c_str());
					lastSent = activity;
					hasSent = true;
					lastSend = clock::now();
				}
			}

			Sleep(250);
		}

		if (ipc.IsConnected())
		{
			ipc.ClearActivity();
			ipc.Disconnect();
		}

		SetEvent(g_workerDone);
		return 0;
	}
}

void Plugin::Initialize(HMODULE module)
{
	std::string dir = ModuleDirectory(module);
	std::string iniPath = dir + kIniName;

	if (!FileExists(iniPath))
	{
		Config::WriteDefault(iniPath, "");
	}
	g_config = Config::Load(iniPath);

	Log::Init(dir + kLogName, ParseLogLevel(g_config.logLevel), g_config.logToFile, g_config.logToConsole);
	Log::Info("RDRBetterPresence v%s loading (RedHook %d.%d)", kPluginVersion, RH_GetMajorVersion(), RH_GetMinorVersion());
	Log::Info("Config: language=%s updateInterval=%dms poll=%dms", g_config.language.c_str(), g_config.updateIntervalMs, g_config.pollIntervalMs);

	Regions::LoadOverrides(dir + kRegionsIniName);

	if (g_config.clientId.empty())
	{
		Log::Error("No ClientId in %s - set the Discord Application ID and reload the plugin (F8 console: reload \"RDRBetterPresence\")", kIniName);
	}

	g_sessionStart = (long long)time(nullptr);
	g_stop = false;

	g_workerDone = CreateEventA(nullptr, TRUE, FALSE, nullptr);
	if (!g_config.clientId.empty())
	{
		g_workerThread = CreateThread(nullptr, 0, DiscordWorker, nullptr, 0, nullptr);
	}

	ScriptRegister(module, ScriptMain);
}

void Plugin::Shutdown(HMODULE module, bool processTerminating)
{
	if (processTerminating)
	{
		// The process is going away: threads are already gone, just drop the log file cleanly.
		Log::Shutdown();
		return;
	}

	ScriptUnregister(module);

	g_stop = true;
	if (g_workerThread)
	{
		// Do NOT join the thread here: DllMain runs under the loader lock and a thread cannot
		// finish exiting while we hold it. The worker signals g_workerDone as its very last
		// action instead, right before it returns into ExitThread.
		WaitForSingleObject(g_workerDone, 5000);
		CloseHandle(g_workerThread);
		g_workerThread = nullptr;
	}
	if (g_workerDone)
	{
		CloseHandle(g_workerDone);
		g_workerDone = nullptr;
	}

	Log::Info("RDRBetterPresence unloaded");
	Log::Shutdown();
}
