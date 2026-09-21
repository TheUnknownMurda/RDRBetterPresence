#pragma once

// Settings read from RDRBetterPresence.ini (placed next to the .red plugin, i.e. in the game folder).

struct Config
{
	// [Discord]
	std::string clientId;
	int updateIntervalMs = 15000;     // minimum delay between two SET_ACTIVITY calls
	int reconnectDelayMs = 10000;     // delay before retrying to connect to Discord

	// [Presence]
	std::string language = "en";      // "en" or "fr"
	bool showTimeOfDay = true;
	bool showWeather = true;
	bool showHealth = true;
	bool showWeapon = true;
	bool showOutfit = true;
	bool showMoney = true;            // cash / honor / fame in the large image hover text
	bool showBounty = true;           // "Wanted $x" on the activity line
	bool showMissions = true;         // story / stranger missions, minigames and jobs by name
	bool showCoordinates = false;     // debugging aid: append X/Y to the state line
	std::string largeImageDefault = "rdr_logo";
	bool useRegionImages = true;      // large image = region_<slug> when a region is known

	// [Buttons]
	std::string button1Label, button1Url;
	std::string button2Label, button2Url;

	// [Debug]
	bool logToFile = true;
	bool logToConsole = true;
	std::string logLevel = "info";    // debug | info | warning | error
	int pollIntervalMs = 500;         // how often the script fiber samples the game state

	static Config Load(const std::string& iniPath);
	static void WriteDefault(const std::string& iniPath, const std::string& clientId);
};
