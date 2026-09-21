#include "pch.h"
#include "Config.h"

namespace
{
	std::string ReadString(const std::string& ini, const char* section, const char* key, const std::string& fallback)
	{
		char buffer[1024];
		DWORD len = GetPrivateProfileStringA(section, key, fallback.c_str(), buffer, sizeof(buffer), ini.c_str());
		return std::string(buffer, len);
	}

	bool ReadBool(const std::string& ini, const char* section, const char* key, bool fallback)
	{
		std::string v = ReadString(ini, section, key, fallback ? "true" : "false");
		for (auto& c : v) c = (char)tolower((unsigned char)c);
		if (v == "1" || v == "true" || v == "yes" || v == "on") return true;
		if (v == "0" || v == "false" || v == "no" || v == "off") return false;
		return fallback;
	}

	int ReadInt(const std::string& ini, const char* section, const char* key, int fallback)
	{
		std::string v = ReadString(ini, section, key, "");
		if (v.empty()) return fallback;
		char* end = nullptr;
		long n = strtol(v.c_str(), &end, 10);
		return (end && *end == '\0') ? (int)n : fallback;
	}
}

Config Config::Load(const std::string& ini)
{
	Config c;

	c.clientId          = ReadString(ini, "Discord", "ClientId", "");
	c.updateIntervalMs  = ReadInt(ini, "Discord", "UpdateIntervalMs", c.updateIntervalMs);
	c.reconnectDelayMs  = ReadInt(ini, "Discord", "ReconnectDelayMs", c.reconnectDelayMs);

	c.language          = ReadString(ini, "Presence", "Language", c.language);
	c.showTimeOfDay     = ReadBool(ini, "Presence", "ShowTimeOfDay", c.showTimeOfDay);
	c.showWeather       = ReadBool(ini, "Presence", "ShowWeather", c.showWeather);
	c.showHealth        = ReadBool(ini, "Presence", "ShowHealth", c.showHealth);
	c.showWeapon        = ReadBool(ini, "Presence", "ShowWeapon", c.showWeapon);
	c.showOutfit        = ReadBool(ini, "Presence", "ShowOutfit", c.showOutfit);
	c.showCoordinates   = ReadBool(ini, "Presence", "ShowCoordinates", c.showCoordinates);
	c.largeImageDefault = ReadString(ini, "Presence", "LargeImageDefault", c.largeImageDefault);
	c.useRegionImages   = ReadBool(ini, "Presence", "UseRegionImages", c.useRegionImages);

	c.button1Label      = ReadString(ini, "Buttons", "Button1Label", "");
	c.button1Url        = ReadString(ini, "Buttons", "Button1Url", "");
	c.button2Label      = ReadString(ini, "Buttons", "Button2Label", "");
	c.button2Url        = ReadString(ini, "Buttons", "Button2Url", "");

	c.logToFile         = ReadBool(ini, "Debug", "LogToFile", c.logToFile);
	c.logToConsole      = ReadBool(ini, "Debug", "LogToConsole", c.logToConsole);
	c.logLevel          = ReadString(ini, "Debug", "LogLevel", c.logLevel);
	c.pollIntervalMs    = ReadInt(ini, "Debug", "PollIntervalMs", c.pollIntervalMs);

	// Clamp to sane values: Discord rate-limits SET_ACTIVITY to ~5 updates / 20 s.
	if (c.updateIntervalMs < 4000) c.updateIntervalMs = 4000;
	if (c.reconnectDelayMs < 2000) c.reconnectDelayMs = 2000;
	if (c.pollIntervalMs < 100) c.pollIntervalMs = 100;
	if (c.language != "fr" && c.language != "en") c.language = "en";

	return c;
}

void Config::WriteDefault(const std::string& iniPath, const std::string& clientId)
{
	std::ofstream f(iniPath, std::ios::out | std::ios::trunc);
	if (!f.is_open())
	{
		return;
	}

	f <<
		"; RDRBetterPresence - Discord Rich Presence for Red Dead Redemption (PC)\n"
		"; This file is read once when the plugin loads (game start, or `reload` in the F8 RedHook console).\n"
		"\n"
		"[Discord]\n"
		"; Application ID from https://discord.com/developers/applications\n"
		"ClientId=" << clientId << "\n"
		"; Minimum delay between two presence updates, in milliseconds (Discord rate-limits at ~5 per 20 s).\n"
		"UpdateIntervalMs=15000\n"
		"; Delay before retrying when Discord is not running / the pipe is closed.\n"
		"ReconnectDelayMs=10000\n"
		"\n"
		"[Presence]\n"
		"; Language of the presence text: fr or en\n"
		"Language=fr\n"
		"ShowTimeOfDay=true\n"
		"ShowWeather=true\n"
		"ShowHealth=true\n"
		"ShowWeapon=true\n"
		"ShowOutfit=true\n"
		"; Append the player's X/Y to the state line (useful to calibrate regions.ini).\n"
		"ShowCoordinates=false\n"
		"; Asset key (uploaded in the Discord app > Rich Presence > Art Assets) or an https:// image URL.\n"
		"LargeImageDefault=rdr_logo\n"
		"; When true, the large image becomes region_<slug> (e.g. region_cholla_springs) if the region is known.\n"
		"UseRegionImages=true\n"
		"\n"
		"[Buttons]\n"
		"; Up to two buttons shown under the presence (leave empty to hide).\n"
		"Button1Label=\n"
		"Button1Url=\n"
		"Button2Label=\n"
		"Button2Url=\n"
		"\n"
		"[Debug]\n"
		"LogToFile=true\n"
		"LogToConsole=true\n"
		"; debug | info | warning | error   (debug logs every sampled game state)\n"
		"LogLevel=info\n"
		"; How often the game state is sampled, in milliseconds.\n"
		"PollIntervalMs=500\n";
}
