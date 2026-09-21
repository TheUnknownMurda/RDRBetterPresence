#include "pch.h"
#include "Journal.h"
#include "../Log.h"

namespace
{
	// Story mission titles by UI id (miss<N>), from the game's own string table
	// (RDR_Missions_UIIDs research by Foxxyyy).
	const char* const kStoryTitles[] =
	{
		"Exodus in America",                                       // miss0  (intro00)
		"Exodus in America",                                       // miss1  (intro01)
		"New Friends, Old Problems",                               // miss2  (ranch01)
		"Obstacles in Our Path",                                   // miss3  (ranch03)
		"This is Armadillo, USA",                                  // miss4  (ranch02)
		"Women and Cattle",                                        // miss5  (ranch07)
		"Wild Horses, Tamed Passions",                             // miss6  (ranch06)
		"A Tempest Looms",                                         // miss7  (ranch08)
		"Political Realities in Armadillo",                        // miss8  (marshal01)
		"Justice in Pike's Basin",                                 // miss9  (marshal02)
		"Old Swindler Blues",                                      // miss10 (merchant01)
		"You Shall Not Give False Testimony, Except for Profit",   // miss11 (merchant02)
		"Exhuming and Other Fine Hobbies",                         // miss12 (grave01)
		"A Gentle Drive with Friends",                             // miss13 (grave02)
		"Let the Dead Bury Their Dead",                            // miss14 (grave03)
		"Liars, Cheats and Other Proud Americans",                 // miss15 (merchant05)
		"Can a Swindler Change His Spots?",                        // miss16 (merchant03)
		"The Sport of Kings, and Liars",                           // miss17 (merchant04)
		"A Frenchman, a Welshman and an Irishman",                 // miss18 (outlaw01)
		"Man is Born Unto Trouble",                                // miss19 (outlaw02)
		"On Shaky's Ground",                                       // miss20 (outlaw03)
		"Spare the Rod, Spoil the Bandit",                         // miss21 (marshal04)
		"The Burning",                                             // miss22 (ranch04)
		"Hanging Bonnie MacFarlane",                               // miss23 (marshal03)
		"The Assault on Fort Mercer",                              // miss24 (fort01)
		"We Shall Be Together in Paradise",                        // miss25 (fort02)
		"Civilization, at Any Price",                              // miss26 (mexarmy02)
		"The Demon Drink",                                         // miss27 (mexarmy01)
		"Empty Promises",                                          // miss28 (mexarmy03)
		"Mexican Caesar",                                          // miss29 (mexarmy04)
		"The Gunslinger's Tragedy",                                // miss30 (gun01)
		"Lucky in Love",                                           // miss31 (gun05)
		"Landon Ricketts Rides Again",                             // miss32 (gun03)
		"The Mexican Wagon Train",                                 // miss33 (gun02)
		"My Sister's Keeper",                                      // miss34 (mexgirl01)
		"Must a Savior Die?",                                      // miss35 (mexgirl03)
		"Cowards Die Many Times",                                  // miss36 (mexarmy05)
		"The Great Mexican Train Robbery",                         // miss37 (rebel03)
		"Father Abraham",                                          // miss38 (rebel04)
		"Captain De Santa's Downfall",                             // miss39 (rebel02)
		"The Gates of El Presidio",                                // miss40 (rebel06)
		"An Appointed Time",                                       // miss41 (rebel05)
		"Bear One Another's Burdens",                              // miss42 (fbi05)
		"Great Men Are Not Always Wise",                           // miss43 (fbi01)
		"At Home with Dutch",                                      // miss44 (anthro04)
		"For Purely Scientific Purposes",                          // miss45 (anthro01)
		"The Prodigal Son Returns (to Yale)",                      // miss46 (anthro03)
		"And You Will Know the Truth",                             // miss47 (fbi02)
		"And the Truth Will Set You Free",                         // miss48 (fbi04)
		"The Outlaw's Return",                                     // miss49 (home01)
		"Pestilence",                                              // miss50
		"Old Friends, New Problems",                               // miss51 (home02_wife03)
		"John Marston and Son",                                    // miss52 (home02_son01)
		"Wolves, Dogs and Sons",                                   // miss53 (home02_son02)
		"Spare the Love, Spoil the Child",                         // miss54 (home02_son03)
		"By Sweat and Toil",                                       // miss55 (home02_ranch01)
		"A Continual Feast",                                       // miss56 (home02_ranch03)
		"The Last Enemy That Shall Be Destroyed",                  // miss57 (home03)
	};
	constexpr int kStoryCount = (int)(sizeof(kStoryTitles) / sizeof(kStoryTitles[0]));

	constexpr int kMaxEntries = 64;        // list 0 = in progress, list 1 = available missions
	constexpr int kMissionEntryType = 1;

	// Hash tables, filled by Init() through the game's STRING_TO_HASH.
	unsigned g_storyShortHash[kStoryCount];
	unsigned g_storyHash[kStoryCount];
	unsigned g_duelHash = 0;
	bool g_ready = false;

	// Result of one scan, plain data so it can be filled inside __try.
	struct Scan
	{
		int story;          // miss<N> or -1
		bool duel;
	};

	void InitRaw()
	{
		char label[32];
		for (int n = 0; n < kStoryCount; ++n)
		{
			snprintf(label, sizeof(label), "miss%d_short", n);
			g_storyShortHash[n] = STRING::STRING_TO_HASH(label);
			snprintf(label, sizeof(label), "miss%d", n);
			g_storyHash[n] = STRING::STRING_TO_HASH(label);
		}
		g_duelHash = STRING::STRING_TO_HASH("beat_duel_short");
	}

	void ScanRaw(Scan& out)
	{
		// Only list 0 holds what is in progress; list 1 is the catalogue of available missions.
		int count = JOURNAL::GET_NUM_JOURNAL_ENTRIES_IN_LIST(0);
		if (count < 0 || count > kMaxEntries)
		{
			return;
		}
		for (int i = 0; i < count; ++i)
		{
			int entry = JOURNAL::GET_JOURNAL_ENTRY_IN_LIST(0, i);
			if (JOURNAL::GET_JOURNAL_ENTRY_TYPE(entry) != kMissionEntryType)
			{
				continue;
			}
			unsigned h = (unsigned)entry;
			for (int n = 0; n < kStoryCount; ++n)
			{
				if (h == g_storyShortHash[n] || h == g_storyHash[n])
				{
					out.story = n;
					return;
				}
			}
			if (h == g_duelHash)
			{
				out.duel = true;
			}
		}
	}

	bool InitGuarded()
	{
		__try { InitRaw(); return true; }
		__except (EXCEPTION_EXECUTE_HANDLER) { return false; }
	}

	bool ScanGuarded(Scan& out)
	{
		__try { ScanRaw(out); return true; }
		__except (EXCEPTION_EXECUTE_HANDLER) { return false; }
	}
}

void Journal::Init()
{
	g_ready = InitGuarded();
	if (!g_ready)
	{
		Log::Error("Exception while hashing journal labels - mission names disabled");
	}
}

ActiveMission Journal::Detect()
{
	ActiveMission result;
	if (!g_ready)
	{
		return result;
	}

	Scan scan{};
	scan.story = -1;
	if (!ScanGuarded(scan))
	{
		Log::Error("Exception while reading the journal - mission names disabled");
		g_ready = false;
		return result;
	}

	if (scan.story >= 0)
	{
		result.kind = MissionKind::Story;
		result.title = kStoryTitles[scan.story];
	}
	else if (scan.duel)
	{
		result.kind = MissionKind::Duel;
		result.title = "Duel";
	}
	return result;
}
