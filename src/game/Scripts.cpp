#include "pch.h"
#include "Scripts.h"
#include "../Log.h"

namespace
{
	struct KnownScript
	{
		const char* name;    // script name (last component of the game path)
		const char* path;    // full game path, in case the native wants that form
		ScriptKind kind;
		const char* title;
		const char* place;   // minigames only
	};

	// Story missions. Order = priority within the kind; length ordering is applied at runtime
	// in case the native does substring matching (so "home02_ranch01" beats "ranch01").
	#define STORY(script, region, title) { script, "/content/" region "/missions/" script "/" script, ScriptKind::StoryMission, title, "" }
	#define HOME02(script, title)        { script, "/content/north/missions/home02/" script "/" script, ScriptKind::StoryMission, title, "" }
	#define RCM(folder, part, title)     { "rcm_" folder part, "/content/rcm/" folder "/rcm_" folder part, ScriptKind::StrangerMission, title, "" }
	#define MINI(script, region, title, place) { script, "/content/" region "/mini_games/" script "/" script, ScriptKind::Minigame, title, place }

	const KnownScript kScripts[] =
	{
		// --- New Austin (frontier) ---------------------------------------------------------------
		STORY("intro00",    "frontier", "Exodus in America"),
		STORY("intro01",    "frontier", "Exodus in America"),
		STORY("ranch01",    "frontier", "New Friends, Old Problems"),
		STORY("ranch02",    "frontier", "This is Armadillo, USA"),
		STORY("ranch03",    "frontier", "Obstacles in Our Path"),
		STORY("ranch04",    "frontier", "The Burning"),
		STORY("ranch06",    "frontier", "Wild Horses, Tamed Passions"),
		STORY("ranch07",    "frontier", "Women and Cattle"),
		STORY("ranch08",    "frontier", "A Tempest Looms"),
		STORY("marshal01",  "frontier", "Political Realities in Armadillo"),
		STORY("marshal02",  "frontier", "Justice in Pike's Basin"),
		STORY("marshal03",  "frontier", "Hanging Bonnie MacFarlane"),
		STORY("marshal04",  "frontier", "Spare the Rod, Spoil the Bandit"),
		STORY("merchant01", "frontier", "Old Swindler Blues"),
		STORY("merchant02", "frontier", "You Shall Not Give False Testimony, Except for Profit"),
		STORY("merchant03", "frontier", "Can a Swindler Change His Spots?"),
		STORY("merchant04", "frontier", "The Sport of Kings, and Liars"),
		STORY("merchant05", "frontier", "Liars, Cheats and Other Proud Americans"),
		STORY("grave01",    "frontier", "Exhuming and Other Fine Hobbies"),
		STORY("grave02",    "frontier", "A Gentle Drive with Friends"),
		STORY("grave03",    "frontier", "Let the Dead Bury Their Dead"),
		STORY("outlaw01",   "frontier", "A Frenchman, a Welshman and an Irishman"),
		STORY("outlaw02",   "frontier", "Man is Born Unto Trouble"),
		STORY("outlaw03",   "frontier", "On Shaky's Ground"),
		STORY("fort01",     "frontier", "The Assault on Fort Mercer"),
		STORY("fort02",     "frontier", "We Shall Be Together in Paradise"),

		// --- Nuevo Paraíso (mexico) --------------------------------------------------------------
		STORY("mexarmy01",  "mexico", "The Demon Drink"),
		STORY("mexarmy02",  "mexico", "Civilization, at Any Price"),
		STORY("mexarmy03",  "mexico", "Empty Promises"),
		STORY("mexarmy04",  "mexico", "Mexican Caesar"),
		STORY("mexarmy05",  "mexico", "Cowards Die Many Times"),
		STORY("gun01",      "mexico", "The Gunslinger's Tragedy"),
		STORY("gun02",      "mexico", "The Mexican Wagon Train"),
		STORY("gun03",      "mexico", "Landon Ricketts Rides Again"),
		STORY("gun05",      "mexico", "Lucky in Love"),
		STORY("mexgirl01",  "mexico", "My Sister's Keeper"),
		STORY("mexgirl03",  "mexico", "Must a Savior Die?"),
		STORY("rebel02",    "mexico", "Captain De Santa's Downfall"),
		STORY("rebel03",    "mexico", "The Great Mexican Train Robbery"),
		STORY("rebel04",    "mexico", "Father Abraham"),
		STORY("rebel05",    "mexico", "An Appointed Time"),
		STORY("rebel06",    "mexico", "The Gates of El Presidio"),

		// --- West Elizabeth (north) ------------------------------------------------------------
		STORY("fbi01",      "north", "Great Men Are Not Always Wise"),
		STORY("fbi02",      "north", "And You Will Know the Truth"),
		STORY("fbi04",      "north", "And the Truth Will Set You Free"),
		STORY("fbi05",      "north", "Bear One Another's Burdens"),
		STORY("anthro01",   "north", "For Purely Scientific Purposes"),
		STORY("anthro03",   "north", "The Prodigal Son Returns (to Yale)"),
		STORY("anthro04",   "north", "At Home with Dutch"),
		STORY("home01",     "north", "The Outlaw's Return"),
		HOME02("home02_wife02",  "Old Friends, New Problems"),
		HOME02("home02_wife03",  "Old Friends, New Problems"),
		HOME02("home02_son01",   "John Marston and Son"),
		HOME02("home02_son02",   "Wolves, Dogs and Sons"),
		HOME02("home02_son03",   "Spare the Love, Spoil the Child"),
		HOME02("home02_ranch01", "By Sweat and Toil"),
		HOME02("home02_ranch03", "A Continual Feast"),
		STORY("home03",     "north", "The Last Enemy That Shall Be Destroyed"),

		// --- Strangers (each mission is several scripts, one per part) ----------------------------
		RCM("abandoned", "1", "Let No Man Put Asunder"),
		RCM("abandoned", "3", "Let No Man Put Asunder"),
		RCM("abandoned", "4", "Let No Man Put Asunder"),
		RCM("abandoned", "6", "Let No Man Put Asunder"),
		RCM("aztecgold", "1", "Aztec Gold"),
		RCM("aztecgold", "2", "Aztec Gold"),
		RCM("aztecgold", "3", "Aztec Gold"),
		RCM("aztecgold", "4", "Aztec Gold"),
		RCM("aztecgold", "7", "Aztec Gold"),
		RCM("aztecgold", "8", "Aztec Gold"),
		RCM("california", "1", "California"),
		RCM("california", "2", "California"),
		RCM("california", "3", "California"),
		RCM("california", "4", "California"),
		RCM("california", "5", "California"),
		RCM("cannibalfamily", "1", "American Appetites"),
		RCM("cannibalfamily", "2", "American Appetites"),
		RCM("cannibalfamily", "3", "American Appetites"),
		RCM("cannibalfamily", "4", "American Appetites"),
		RCM("cannibalfamily", "5", "American Appetites"),
		RCM("cannibalfamily", "6", "American Appetites"),
		RCM("corpse", "1", "Flowers for a Lady"),
		RCM("corpse", "2", "Flowers for a Lady"),
		RCM("dealer", "1", "Poppycock"),
		RCM("dealer", "2", "Poppycock"),
		RCM("dealer", "3", "Poppycock"),
		RCM("dealer", "4", "Poppycock"),
		RCM("dealer", "5", "Poppycock"),
		RCM("funnyman", "1", "Funny Man"),
		RCM("funnyman", "2", "Funny Man"),
		RCM("funnyman", "3", "Funny Man"),
		RCM("funnyman", "4", "Funny Man"),
		RCM("icarus", "1", "Deadalus and Son"),
		RCM("icarus", "2", "Deadalus and Son"),
		RCM("icarus", "3", "Deadalus and Son"),
		RCM("iknowyou", "1", "I Know You"),
		RCM("iknowyou", "2", "I Know You"),
		RCM("iknowyou", "3", "I Know You"),
		RCM("iknowyou", "4", "I Know You"),
		RCM("iknowyou", "8", "I Know You"),
		RCM("jenny", "1", "Jenny's Faith"),
		RCM("jenny", "2", "Jenny's Faith"),
		RCM("killerpimp", "1", "Eva in Peril"),
		RCM("killerpimp", "1a", "Eva in Peril"),
		RCM("killerpimp", "2", "Eva in Peril"),
		RCM("killerpimp", "3", "Eva in Peril"),
		RCM("lights", "1", "Lights, Camera, Action"),
		RCM("lights", "2", "Lights, Camera, Action"),
		RCM("lights", "3", "Lights, Camera, Action"),
		RCM("lights", "4", "Lights, Camera, Action"),
		RCM("lights", "5", "Lights, Camera, Action"),
		RCM("lovesahorse", "1", "Who Are You to Judge?"),
		RCM("lovesahorse", "2", "Who Are You to Judge?"),
		RCM("lovesahorse", "3", "Who Are You to Judge?"),
		RCM("opiate", "1", "Love is the Opiate"),
		RCM("opiate", "2", "Love is the Opiate"),
		RCM("opiate", "3", "Love is the Opiate"),
		RCM("politician", "1", "American Lobbyist"),
		RCM("politician", "2", "American Lobbyist"),
		RCM("politician", "3", "American Lobbyist"),
		RCM("prohibitionist", "1", "The Prohibitionist"),
		RCM("prohibitionist", "2", "The Prohibitionist"),
		RCM("prohibitionist", "3", "The Prohibitionist"),
		RCM("prohibitionist", "4", "The Prohibitionist"),
		RCM("remember", "1", "Remember My Family"),
		RCM("remember", "2", "Remember My Family"),
		RCM("remember", "3", "Remember My Family"),
		RCM("remember", "4", "Remember My Family"),
		RCM("water", "1", "Water and Honesty"),
		RCM("water", "2", "Water and Honesty"),
		RCM("water", "3", "Water and Honesty"),
		RCM("water", "4", "Water and Honesty"),
		RCM("wronged", "1", "The Wronged Woman"),
		RCM("wronged", "2", "The Wronged Woman"),
		RCM("wronged", "3", "The Wronged Woman"),
		RCM("wronged", "4", "The Wronged Woman"),
		RCM("wronged", "6", "The Wronged Woman"),

		// --- Jobs / ambient activities -----------------------------------------------------------
		{ "job_nightwatch",     "/content/ambient/jobsystem/job_nightwatch",          ScriptKind::Job,    "Night Watch", "" },
		{ "job_horsebreaking",  "/content/ambient/jobsystem/job_horsebreaking",       ScriptKind::Job,    "Horsebreaking", "" },
		{ "event_bountyhunter", "/content/ambient/pointofinterest/event_bountyhunter", ScriptKind::Bounty, "Bounty hunting", "" },
		{ "beat_duel_lowhonor", "/content/ambient/town/beat_duel_lowhonor",           ScriptKind::Duel,   "Duel", "" },
		{ "beat_duel_notoriety","/content/ambient/town/beat_duel_notoriety",          ScriptKind::Duel,   "Duel", "" },
		{ "beat_duel_rude",     "/content/ambient/town/beat_duel_rude",               ScriptKind::Duel,   "Duel", "" },

		// --- Minigames (script suffix = table location) --------------------------------------------
		MINI("poker_arm",            "frontier", "Poker",              "Armadillo"),
		MINI("poker_hen",            "frontier", "Poker",              "MacFarlane's Ranch"),
		MINI("poker_thi",            "frontier", "Poker",              "Thieves' Landing"),
		MINI("poker_cas",            "mexico",   "Poker",              "Casa Madrugada"),
		MINI("poker_chu",            "mexico",   "Poker",              "Chuparosa"),
		MINI("poker_beh",            "north",    "Poker",              "Beecher's Hope"),
		MINI("blackjack_rat",        "frontier", "Blackjack",          "Rathskeller Fork"),
		MINI("blackjack_thi",        "frontier", "Blackjack",          "Thieves' Landing"),
		MINI("blackjack_chu",        "mexico",   "Blackjack",          "Chuparosa"),
		MINI("blackjack_blk",        "north",    "Blackjack",          "Blackwater"),
		MINI("liarsdice_thi",        "frontier", "Liar's Dice",        "Thieves' Landing"),
		MINI("liarsdice_cas",        "mexico",   "Liar's Dice",        "Casa Madrugada"),
		MINI("liarsdice_esc",        "mexico",   "Liar's Dice",        "Escalera"),
		MINI("horseshoes_hen",       "frontier", "Horseshoes",         "MacFarlane's Ranch"),
		MINI("horseshoes_rat",       "frontier", "Horseshoes",         "Rathskeller Fork"),
		MINI("horseshoes_lsh",       "mexico",   "Horseshoes",         "Las Hermanas"),
		MINI("horseshoes_beh",       "north",    "Horseshoes",         "Beecher's Hope"),
		MINI("fivefingerfillet_arm", "frontier", "Five Finger Fillet", "Armadillo"),
		MINI("fivefingerfillet_thi", "frontier", "Five Finger Fillet", "Thieves' Landing"),
		MINI("fivefingerfillet_esc", "mexico",   "Five Finger Fillet", "Escalera"),
		MINI("fivefingerfillet_tor", "mexico",   "Five Finger Fillet", "Torquemada"),
		MINI("fivefingerfillet_mtp", "north",    "Five Finger Fillet", "Manzanita Post"),
		MINI("armwrestling_pln",     "frontier", "Arm Wrestling",      "Plainview"),
		MINI("armwrestling_agv",     "mexico",   "Arm Wrestling",      "Agave Viejo"),
		MINI("armwrestling_elm",     "mexico",   "Arm Wrestling",      "El Matadero"),
		MINI("armwrestling_upr",     "north",    "Arm Wrestling",      "Pacific Union Camp"),
	};

	#undef STORY
	#undef HOME02
	#undef RCM
	#undef MINI

	// Which spelling the native accepts is discovered at runtime (see Detect).
	enum class NameForm { Unknown, Short, Path };
	NameForm g_nameForm = NameForm::Unknown;

	int Priority(ScriptKind kind)
	{
		switch (kind)
		{
			case ScriptKind::StoryMission:    return 0;
			case ScriptKind::StrangerMission: return 1;
			case ScriptKind::Bounty:          return 2;
			case ScriptKind::Job:             return 3;
			case ScriptKind::Duel:            return 4;
			case ScriptKind::GangHideout:     return 5;
			case ScriptKind::Minigame:        return 6;
			default:                          return 9;
		}
	}

	// Sorted view: priority first, then longer names first (guards against substring matches).
	const std::vector<const KnownScript*>& SortedScripts()
	{
		static std::vector<const KnownScript*> sorted = []
		{
			std::vector<const KnownScript*> v;
			for (const KnownScript& s : kScripts) v.push_back(&s);
			std::sort(v.begin(), v.end(), [](const KnownScript* a, const KnownScript* b)
			{
				int pa = Priority(a->kind), pb = Priority(b->kind);
				if (pa != pb) return pa < pb;
				return strlen(a->name) > strlen(b->name);
			});
			return v;
		}();
		return sorted;
	}

	bool IsRunning(const KnownScript& s)
	{
		switch (g_nameForm)
		{
			case NameForm::Short:
				return CORE::_IS_ANY_NAMED_SCRIPT_RUNNING(s.name);
			case NameForm::Path:
				return CORE::_IS_ANY_NAMED_SCRIPT_RUNNING(s.path);
			default:
				if (CORE::_IS_ANY_NAMED_SCRIPT_RUNNING(s.name))
				{
					g_nameForm = NameForm::Short;
					Log::Info("Script lookup works with short names (matched '%s')", s.name);
					return true;
				}
				if (CORE::_IS_ANY_NAMED_SCRIPT_RUNNING(s.path))
				{
					g_nameForm = NameForm::Path;
					Log::Info("Script lookup works with full paths (matched '%s')", s.path);
					return true;
				}
				return false;
		}
	}
}

int Scripts::DetectIndex()
{
	const auto& sorted = SortedScripts();
	for (size_t i = 0; i < sorted.size(); ++i)
	{
		if (IsRunning(*sorted[i]))
		{
			return (int)i;
		}
	}
	return -1;
}

ActiveScript Scripts::Describe(int index)
{
	ActiveScript a;
	const auto& sorted = SortedScripts();
	if (index >= 0 && (size_t)index < sorted.size())
	{
		const KnownScript* s = sorted[(size_t)index];
		a.kind = s->kind;
		a.name = s->name;
		a.title = s->title;
		a.place = s->place;
	}
	return a;
}
