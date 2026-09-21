#pragma once

// All user-visible presence text, in French and English. Strings are UTF-8
// (the project compiles with /utf-8) and sent verbatim to Discord.

struct Strings
{
	// Activities (details line)
	const char* inMenus;
	const char* paused;
	const char* cutscene;
	const char* dead;
	const char* minigame;
	const char* combat;
	const char* deadEye;
	const char* lassoing;
	const char* hogtied;
	const char* handsUp;
	const char* onTrain;
	const char* driving;
	const char* passenger;
	const char* ridingHorse;
	const char* ridingMule;
	const char* ridingBull;
	const char* ridingBuffalo;
	const char* ridingOther;
	const char* inWater;
	const char* drunk;
	const char* indoors;
	const char* exploring;

	// Scripted activities
	const char* missionPrefix;   // "Mission: "
	const char* strangerPrefix;  // "Stranger: "
	const char* playingPrefix;   // "Playing "
	const char* wanted;          // "Wanted"

	// Fragments
	const char* hp;             // "PV" / "HP"
	const char* deadEyeShort;   // "Dead Eye"
	const char* unknownRegion;
	const char* weaponPrefix;   // "Arme : " / "Weapon: "
	const char* unarmed;

	// Weather, indexed by the base Weather enum (0..5) + extras
	const char* weather[6];
	const char* weatherCave;
	const char* weatherFog;
	const char* weatherForest;

	// Character
	const char* john;
	const char* jack;
	const char* outfits[25];

	// Fallback weapon names, indexed by WeaponModel (used when the game hands us a label instead of text)
	const char* weapons[38];
};

namespace Localization
{
	inline const Strings kFrench =
	{
		"Dans les menus",
		"En pause",
		"Cinématique",
		"Mort",
		"Mini-jeu en cours",
		"En fusillade",
		"Dead Eye activé",
		"Capture au lasso",
		"Ligoté",
		"Mains en l'air",
		"À bord du train",
		"Conduit une diligence",
		"Passager d'une diligence",
		"À cheval",
		"À dos de mule",
		"Sur un taureau",
		"Sur un bison",
		"En selle",
		"Dans l'eau",
		"Ivre",
		"À l'intérieur",
		"Exploration libre",

		"Mission : ",
		"Inconnu : ",
		"Partie de ",
		"Recherché",

		"PV",
		"Dead Eye",
		"Quelque part dans l'Ouest",
		"Arme : ",
		"Mains nues",

		{ "Ciel dégagé", "Beau temps", "Nuageux", "Pluie", "Orage", "Neige" },
		"Grotte",
		"Brouillard",
		"Forêt",

		"John Marston",
		"Jack Marston",
		{
			"Tenue de cow-boy",            // 0  OUT_cowboy
			"Costume élégant",             // 1  OUT_suit
			"Tenue de rebelle de Reyes",   // 2  OUT_rebel
			"Tenue de chasseur de trésors",// 3  OUT_treasur
			"Tenue du gang de Walton",     // 4  OUT_dnd
			"Tenue des jumeaux Bollard",   // 5  OUT_rustler
			"Tenue de bandito",            // 6  OUT_bandito
			"Uniforme du Bureau",          // 7  OUT_fbi
			"Uniforme de marshal",         // 8  OUT_uslaw
			"Uniforme de l'armée",         // 9  OUT_military
			"Vêtements de rancher",        // 10 OUT_rancher
			"Légende de l'Ouest",          // 11 OUT_legend
			"Tenue de cow-boy",            // 12 OUT_cowboy2
			"Tenue de gentleman",          // 13 OUT_social
			"John blessé",                 // 14 OUT_apoc_z
			"John blessé",                 // 15 OUT_zhunter_z
			"Cache-poussière",             // 16 OUT_duster
			"Poncho mexicain",             // 17 OUT_poncho
			"Assassin mortel",             // 18 OUT_gun
			"Chasseur expert",             // 19 OUT_mount
			"Marchand avisé",              // 20 OUT_bruise
			"Sous-vêtements",              // 21 OUT_small
			"Cow-boy mort-vivant",         // 22 OUT_zombie
			"Légende de l'Apocalypse",     // 23 OUT_apoc
			"Chasseur de morts-vivants",   // 24 OUT_zhunter
		},
		{
			"Pistolet Volcanic",           // 0  PISTOL_Volcanic
			"Pistolet semi-automatique",   // 1  PISTOL_SemiAuto
			"Pistolet haute puissance",    // 2  PISTOL_HighPower
			"Pistolet Mauser",             // 3  PISTOL_Mauser
			"Revolver Cattleman",          // 4  REVOLVER_Cattleman
			"Revolver Schofield",          // 5  REVOLVER_Schofield
			"Revolver à double action",    // 6  REVOLVER_DoubleAction
			"Revolver LeMat",              // 7  REVOLVER_Lemat
			"Carabine à répétition",       // 8  REPEATER_Carbine
			"Winchester à répétition",     // 9  REPEATER_Winchester
			"Henry à répétition",          // 10 REPEATER_Henry
			"Evans à répétition",          // 11 REPEATER_Evans
			"Fusil Springfield",           // 12 RIFLE_Springfield
			"Fusil à verrou",              // 13 RIFLE_BoltAction
			"Fusil à bison",               // 14 RIFLE_Buffalo
			"Fusil à canon scié",          // 15 SHOTGUN_SawedOff
			"Fusil à deux canons",         // 16 SHOTGUN_DoubleBarrel
			"Fusil à pompe",               // 17 SHOTGUN_PumpAction
			"Fusil semi-automatique",      // 18 SHOTGUN_SemiAuto
			"Fusil Rolling Block",         // 19 SNIPERRIFLE_RollingBlock
			"Fusil Carcano",               // 20 SNIPERRIFLE_Carcano
			"Lasso",                       // 21 LASSO_Lasso
			"Couteau",                     // 22 MELEE_Knife
			"Cocktail Molotov",            // 23 THROWN_FireBottle
			"Dynamite",                    // 24 THROWN_Dynamite
			"Couteau de lancer",           // 25 THROWN_ThrowingKnife
			"Mitrailleuse Gatling",        // 26 TURRET_Gatling
			"Mitrailleuse Browning",       // 27 TURRET_Browning
			"Canon",                       // 28 DEFAULT_Cannon
			"Tomahawk",                    // 29 THROWN_Tomahawk
			"Arc",                         // 30 BOW_ShortBow
			"Fusil explosif",              // 31 RIFLE_Antitank
			"Crachat de zombie",           // 32 THROWN_ZombieSpit
			"Torche",                      // 33 MELEE_TORCH
			"Tromblon",                    // 34 SHOTGUN_Blunderbuss
			"Eau bénite",                  // 35 THROWN_HolyWater
			"Appât à zombies",             // 36 THROWN_ZombieBait
			"Appât explosif",              // 37 THROWN_ZombieBoomBait
		}
	};

	inline const Strings kEnglish =
	{
		"In the menus",
		"Paused",
		"Watching a cutscene",
		"Dead",
		"Playing a minigame",
		"In a gunfight",
		"Dead Eye active",
		"Lassoing",
		"Hogtied",
		"Hands up",
		"Riding the train",
		"Driving a stagecoach",
		"Stagecoach passenger",
		"On horseback",
		"Riding a mule",
		"Riding a bull",
		"Riding a buffalo",
		"Riding",
		"In the water",
		"Drunk",
		"Indoors",
		"Free roaming",

		"Mission: ",
		"Stranger: ",
		"Playing ",
		"Wanted",

		"HP",
		"Dead Eye",
		"Somewhere out West",
		"Weapon: ",
		"Unarmed",

		{ "Clear", "Fair", "Cloudy", "Rain", "Storm", "Snow" },
		"Cave",
		"Fog",
		"Forest",

		"John Marston",
		"Jack Marston",
		{
			"Cowboy Outfit",               // 0
			"Elegant Suit",                // 1
			"Reyes' Rebel Outfit",         // 2
			"Treasure Hunter Outfit",      // 3
			"Walton's Gang Outfit",        // 4
			"Bollard Twins Outfit",        // 5
			"Bandito Outfit",              // 6
			"Bureau Uniform",              // 7
			"US Marshal Uniform",          // 8
			"US Army Uniform",             // 9
			"Rancher Clothing",            // 10
			"Legend of the West",          // 11
			"Cowboy Outfit",               // 12
			"Gentleman's Attire",          // 13
			"Injured John",                // 14
			"Injured John",                // 15
			"Duster Coat",                 // 16
			"Mexican Poncho",              // 17
			"Deadly Assassin",             // 18
			"Expert Hunter",               // 19
			"Savvy Merchant",              // 20
			"Union Suit",                  // 21
			"Undead Cowboy",               // 22
			"Legend of the Apocalypse",    // 23
			"Undead Hunter",               // 24
		},
		{
			"Volcanic Pistol",             // 0
			"Semi-automatic Pistol",       // 1
			"High Power Pistol",           // 2
			"Mauser Pistol",               // 3
			"Cattleman Revolver",          // 4
			"Schofield Revolver",          // 5
			"Double-action Revolver",      // 6
			"LeMat Revolver",              // 7
			"Repeater Carbine",            // 8
			"Winchester Repeater",         // 9
			"Henry Repeater",              // 10
			"Evans Repeater",              // 11
			"Springfield Rifle",           // 12
			"Bolt Action Rifle",           // 13
			"Buffalo Rifle",               // 14
			"Sawed-off Shotgun",           // 15
			"Double-barreled Shotgun",     // 16
			"Pump-action Shotgun",         // 17
			"Semi-auto Shotgun",           // 18
			"Rolling Block Rifle",         // 19
			"Carcano Rifle",               // 20
			"Lasso",                       // 21
			"Knife",                       // 22
			"Fire Bottle",                 // 23
			"Dynamite",                    // 24
			"Throwing Knife",              // 25
			"Gatling Gun",                 // 26
			"Browning Gun",                // 27
			"Cannon",                      // 28
			"Tomahawk",                    // 29
			"Bow",                         // 30
			"Explosive Rifle",             // 31
			"Zombie Spit",                 // 32
			"Torch",                       // 33
			"Blunderbuss",                 // 34
			"Holy Water",                  // 35
			"Zombie Bait",                 // 36
			"Boom Bait",                   // 37
		}
	};

	inline const Strings& Get(const std::string& language)
	{
		return language == "fr" ? kFrench : kEnglish;
	}

	// Mission titles stay in English (they are the game's own); only generic activities translate.
	inline std::string TranslateActivity(const std::string& language, const std::string& title)
	{
		if (language == "fr" && title == "Duel") return "Duel";
		return title;
	}
}
