#include "pch.h"
#include "PresenceBuilder.h"
#include "Localization.h"
#include "../game/Regions.h"

namespace
{
	constexpr const char* kSeparator = " \xC2\xB7 ";   // " · "

	// Weather values beyond the SDK enum (see EvilBlunt's eWeather research).
	enum ExtendedWeather
	{
		ExtWeather_InteriorFirst = 6,
		ExtWeather_InteriorLast = 11,
		ExtWeather_Cave = 12,
		ExtWeather_Thieves = 13,
		ExtWeather_Forest = 14,
		ExtWeather_InteriorThieves = 18,
		ExtWeather_InteriorForest = 19
	};

	std::string WeatherText(int weather, const Strings& t)
	{
		if (weather >= ExtWeather_InteriorFirst && weather <= ExtWeather_InteriorLast)
		{
			weather -= ExtWeather_InteriorFirst;
		}
		if (weather >= 0 && weather < 6)
		{
			return t.weather[weather];
		}
		switch (weather)
		{
			case ExtWeather_Cave:            return t.weatherCave;
			case ExtWeather_Thieves:
			case ExtWeather_InteriorThieves: return t.weatherFog;
			case ExtWeather_Forest:
			case ExtWeather_InteriorForest:  return t.weatherForest;
			default:                         return "";
		}
	}

	std::string TimeText(int hour, int minute, const std::string& language)
	{
		// Quarter-hour granularity keeps the update rate reasonable.
		minute -= minute % 15;

		char buf[32];
		if (language == "fr")
		{
			snprintf(buf, sizeof(buf), "%02dh%02d", hour, minute);
		}
		else
		{
			int h12 = hour % 12;
			if (h12 == 0) h12 = 12;
			snprintf(buf, sizeof(buf), "%d:%02d %s", h12, minute, hour < 12 ? "AM" : "PM");
		}
		return buf;
	}

	const char* WeaponCategorySlug(WeaponCategory category)
	{
		switch (category)
		{
			case WEAPON_CATEGORY_PISTOL:           return "weapon_pistol";
			case WEAPON_CATEGORY_REVOLVER:         return "weapon_revolver";
			case WEAPON_CATEGORY_REPEATER:         return "weapon_repeater";
			case WEAPON_CATEGORY_RIFLE:            return "weapon_rifle";
			case WEAPON_CATEGORY_SHOTGUN:          return "weapon_shotgun";
			case WEAPON_CATEGORY_SNIPERRIFLE:      return "weapon_sniper";
			case WEAPON_CATEGORY_LASSO:            return "weapon_lasso";
			case WEAPON_CATEGORY_MELEE:            return "weapon_melee";
			case WEAPON_CATEGORY_THROWN_EXPLODING: return "weapon_explosive";
			case WEAPON_CATEGORY_THROWN:           return "weapon_thrown";
			case WEAPON_CATEGORY_GATLING:
			case WEAPON_CATEGORY_BROWNING:         return "weapon_turret";
			case WEAPON_CATEGORY_CANNON:           return "weapon_cannon";
			case WEAPON_CATEGORY_BOW:              return "weapon_bow";
			default:                               return "";
		}
	}

	// The game may return a GXT label ("WEAP_REV_CATTLEMAN") instead of localized text.
	bool LooksLikeLabel(const std::string& s)
	{
		if (s.empty()) return true;
		bool hasLower = false, hasSpace = false;
		for (unsigned char c : s)
		{
			if (islower(c)) hasLower = true;
			if (c == ' ') hasSpace = true;
		}
		return !hasLower && !hasSpace;
	}

	std::string WeaponText(const GameSnapshot& s, const Strings& t)
	{
		if (s.weapon <= WEAPON_INVALID || s.weapon >= NUM_WEAPONS)
		{
			return "";
		}
		if (!LooksLikeLabel(s.weaponName))
		{
			return s.weaponName;
		}
		return t.weapons[s.weapon];
	}

	std::string HealthText(const GameSnapshot& s, const Strings& t)
	{
		if (s.maxHealth <= 0.0f)
		{
			return "";
		}
		int pct = (int)(100.0f * s.health / s.maxHealth + 0.5f);
		if (pct < 0) pct = 0;
		if (pct > 100) pct = 100;
		return std::string(t.hp) + " " + std::to_string(pct) + "%";
	}

	struct ActivityChoice
	{
		std::string text;
		const char* icon;   // small image asset key, "" = use the weapon icon instead
	};

	// What the scripts say the player is doing: a mission, a stranger, a minigame, a job.
	// Empty text = nothing scripted is running.
	ActivityChoice ChooseScriptedActivity(const GameSnapshot& s, const Config& cfg, const Strings& t)
	{
		if (!cfg.showMissions)
		{
			return { s.minigame ? t.minigame : "", s.minigame ? "minigame" : "" };
		}

		const ActiveScript& sc = s.script;
		std::string title = Localization::TranslateActivity(cfg.language, sc.title);
		switch (sc.kind)
		{
			case ScriptKind::StoryMission:
				return { t.missionPrefix + title, "mission" };
			case ScriptKind::StrangerMission:
				return { t.strangerPrefix + title, "stranger" };
			case ScriptKind::Bounty:
				return { title, "bounty" };
			case ScriptKind::Job:
			case ScriptKind::GangHideout:
				return { title, "job" };
			case ScriptKind::Duel:
				return { title, "duel" };
			case ScriptKind::Minigame:
				// The table scripts may also run while merely sitting nearby: trust the engine flag.
				if (s.minigame)
				{
					std::string text = t.playingPrefix + title;
					if (!sc.place.empty()) text += kSeparator + sc.place;
					return { text, "minigame" };
				}
				break;
			default:
				break;
		}
		if (s.minigame)
		{
			return { t.minigame, "minigame" };
		}
		return { "", "" };
	}

	std::string FormatMoney(int amount)
	{
		// $1,234
		std::string digits = std::to_string(amount < 0 ? -amount : amount);
		std::string out;
		int count = 0;
		for (auto it = digits.rbegin(); it != digits.rend(); ++it)
		{
			if (count && count % 3 == 0) out.insert(out.begin(), ',');
			out.insert(out.begin(), *it);
			++count;
		}
		return (amount < 0 ? "-$" : "$") + out;
	}

	ActivityChoice ChooseActivity(const GameSnapshot& s, const Strings& t)
	{
		if (s.paused)   return { t.paused, "paused" };
		if (s.cutscene) return { t.cutscene, "cutscene" };
		if (s.dead)     return { t.dead, "dead" };
		if (s.hogtied)  return { t.hogtied, "lasso" };
		if (s.handsUp)  return { t.handsUp, "" };
		if (s.deadEye)  return { t.deadEye, "deadeye" };
		if (s.inCombat) return { t.combat, "" };
		if (s.lassoing) return { t.lassoing, "lasso" };
		if (s.onTrain)  return { t.onTrain, "train" };
		if (s.inVehicle) return { s.drivingVehicle ? t.driving : t.passenger, "stagecoach" };
		if (s.riding)
		{
			switch (s.mount)
			{
				case MountKind::Mule:    return { t.ridingMule, "horse" };
				case MountKind::Bull:    return { t.ridingBull, "horse" };
				case MountKind::Buffalo: return { t.ridingBuffalo, "horse" };
				case MountKind::Horse:   return { t.ridingHorse, "horse" };
				default:                 return { t.ridingOther, "horse" };
			}
		}
		if (s.inWater)  return { t.inWater, "" };
		if (s.drunk)    return { t.drunk, "" };
		if (s.inRoom)   return { t.indoors, "" };
		return { t.exploring, "" };
	}
}

Activity PresenceBuilder::Build(const GameSnapshot& s, const Config& cfg, long long sessionStart)
{
	const Strings& t = Localization::Get(cfg.language);

	Activity a;
	a.startTimestamp = sessionStart;
	a.largeImage = cfg.largeImageDefault;

	if (!cfg.button1Label.empty() && !cfg.button1Url.empty()) a.buttons.push_back({ cfg.button1Label, cfg.button1Url });
	if (!cfg.button2Label.empty() && !cfg.button2Url.empty()) a.buttons.push_back({ cfg.button2Label, cfg.button2Url });

	if (!s.playerValid)
	{
		a.details = t.inMenus;
		a.largeText = "Red Dead Redemption";
		return a;
	}

	// --- Line 1: what the player is doing -------------------------------------------------
	// A scripted activity (mission, minigame...) owns the line; the moment-to-moment state
	// (on horseback, in a gunfight...) then moves to the small image. Pause / cutscene / death
	// always win.
	ActivityChoice moment = ChooseActivity(s, t);
	ActivityChoice scripted = ChooseScriptedActivity(s, cfg, t);
	bool interrupted = s.paused || s.cutscene || s.dead;
	bool useScripted = !scripted.text.empty() && !interrupted;

	ActivityChoice choice = useScripted ? scripted : moment;
	a.details = choice.text;
	if (cfg.showHealth && !s.dead && !s.paused)
	{
		std::string hp = HealthText(s, t);
		if (!hp.empty())
		{
			a.details += kSeparator + hp;
		}
		if (s.inCombat && !s.deadEye && !useScripted)
		{
			a.details += kSeparator + std::string(t.deadEyeShort) + " " + std::to_string(s.deadEyePoints);
		}
	}
	if (cfg.showBounty && s.bounty > 0 && !interrupted)
	{
		a.details += kSeparator + std::string(t.wanted) + " " + FormatMoney(s.bounty);
	}

	// --- Line 2: where / when ---------------------------------------------------------------
	std::optional<RegionInfo> region = Regions::Resolve(s);
	a.state = region ? region->region : t.unknownRegion;
	if (region && !region->place.empty())
	{
		a.state += kSeparator + region->place;
	}

	if (cfg.showTimeOfDay)
	{
		a.state += kSeparator + TimeText(s.hour, s.minute, cfg.language);
	}
	if (cfg.showWeather)
	{
		std::string w = WeatherText(s.weather, t);
		if (!w.empty())
		{
			a.state += kSeparator + w;
		}
	}
	if (cfg.showCoordinates)
	{
		char buf[64];
		// Y is the altitude in this game; the map plane is (X, Z).
		snprintf(buf, sizeof(buf), " (%.0f, %.0f)", s.posX, s.posZ);
		a.state += buf;
	}

	// --- Images -------------------------------------------------------------------------------
	if (region && cfg.useRegionImages)
	{
		a.largeImage = "region_" + region->regionSlug;
	}

	std::string character = (s.character == PlayerCharacter::Jack) ? t.jack : t.john;
	a.largeText = character;
	if (cfg.showOutfit && s.outfit >= 0 && s.outfit < 25)
	{
		a.largeText += kSeparator + std::string(t.outfits[s.outfit]);
	}
	if (cfg.showMoney)
	{
		a.largeText += kSeparator + FormatMoney(s.money)
			+ kSeparator + t.honorLabel + " " + std::to_string(s.honor)
			+ kSeparator + t.fameLabel + " " + std::to_string(s.fame);
	}

	std::string weapon = WeaponText(s, t);
	bool momentIsPlain = (moment.text == t.exploring || moment.text == t.indoors);
	if (useScripted && !momentIsPlain)
	{
		// The scripted activity owns line 1, so the moment-to-moment state goes here:
		// e.g. horse icon, "On horseback · Cattleman Revolver".
		a.smallImage = moment.icon[0] != '\0' ? moment.icon
		             : (s.weaponCategory != WEAPON_CATEGORY_INVALID ? WeaponCategorySlug(s.weaponCategory) : "");
		a.smallText = moment.text;
		if (cfg.showWeapon && !weapon.empty())
		{
			a.smallText += kSeparator + weapon;
		}
	}
	else if (choice.icon[0] != '\0')
	{
		a.smallImage = choice.icon;
		a.smallText = choice.text;
		if (cfg.showWeapon && !weapon.empty())
		{
			a.smallText += kSeparator + weapon;
		}
	}
	else if (cfg.showWeapon && s.weaponCategory != WEAPON_CATEGORY_INVALID)
	{
		a.smallImage = WeaponCategorySlug(s.weaponCategory);
		a.smallText = weapon.empty() ? t.unarmed : t.weaponPrefix + weapon;
	}

	return a;
}
