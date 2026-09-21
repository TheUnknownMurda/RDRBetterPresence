#include "pch.h"
#include "Research.h"
#include "../Log.h"

namespace
{
	constexpr int kStatCount = 700;          // Foxxyyy counted 696 stats in the scripts
	constexpr size_t kScanBytes = 8 * 1024 * 1024;
	constexpr int kMaxHits = 200;

	// Console-era global indices (Foxxyyy / Cain532) used only to test candidate layouts.
	constexpr long long kGlobalPlayerActor = 34573;
	constexpr long long kGlobalMoney = 3391;

	bool IsReadable(const void* p, size_t size)
	{
		MEMORY_BASIC_INFORMATION mbi{};
		if (!p || !VirtualQuery(p, &mbi, sizeof(mbi))) return false;
		if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return false;
		return (const char*)p + size <= (const char*)mbi.BaseAddress + mbi.RegionSize;
	}

	// Size of the readable run starting at p (capped).
	size_t ReadableRun(const void* p, size_t cap)
	{
		size_t total = 0;
		const char* cur = (const char*)p;
		while (total < cap)
		{
			MEMORY_BASIC_INFORMATION mbi{};
			if (!VirtualQuery(cur, &mbi, sizeof(mbi))) break;
			if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) break;
			size_t chunk = (const char*)mbi.BaseAddress + mbi.RegionSize - cur;
			total += chunk;
			cur += chunk;
		}
		return std::min(total, cap);
	}

	int ReadAt(uintptr_t base, long long index, int wordSize)
	{
		const void* addr = (const void*)(base + (uintptr_t)(index * wordSize));
		return IsReadable(addr, 4) ? *(const int*)addr : 0;
	}

	int DumpStatsRaw(FILE* f)
	{
		for (int id = 0; id < kStatCount; ++id)
		{
			int i = STAT::GET_SAGPLAYER_STAT_INT(id);
			float fl = STAT::GET_SAGPLAYER_STAT_FLOAT(id);
			fprintf(f, "%d\t%d\t%g\n", id, i, fl);
		}
		return kStatCount;
	}

	int DumpStatsGuarded(FILE* f)
	{
		__try { return DumpStatsRaw(f); }
		__except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
	}

	void DumpGlobalsRaw(FILE* f, int needle, Actor player)
	{
		uintptr_t base = GetGlobalPtr();
		fprintf(f, "GetGlobalPtr() = 0x%llX   player actor = %d   needle = %d\n\n", (unsigned long long)base, player, needle);
		if (base == 0 || !IsReadable((const void*)base, 64))
		{
			fprintf(f, "base is not readable\n");
			return;
		}

		fprintf(f, "First 16 qwords at base:\n");
		for (int i = 0; i < 16; ++i)
		{
			unsigned long long q = *(const unsigned long long*)(base + i * 8);
			fprintf(f, "  [%2d] 0x%016llX  %s\n", i, q, IsReadable((const void*)q, 8) ? "(readable pointer)" : "");
		}

		// Candidate layouts: flat block at base, or base = table of block pointers (GTA V style).
		struct Layout { const char* name; uintptr_t block; int wordSize; };
		uintptr_t indirect = *(const uintptr_t*)base;
		Layout layouts[] =
		{
			{ "flat, 4-byte words",     base,     4 },
			{ "flat, 8-byte words",     base,     8 },
			{ "indirect, 4-byte words", indirect, 4 },
			{ "indirect, 8-byte words", indirect, 8 },
		};

		fprintf(f, "\nConsole-era indices under each layout (PlayerActor=%lld should equal the player actor, Money=%lld the needle):\n", kGlobalPlayerActor, kGlobalMoney);
		for (const Layout& l : layouts)
		{
			if (!IsReadable((const void*)l.block, 64))
			{
				fprintf(f, "  %-24s block 0x%llX not readable\n", l.name, (unsigned long long)l.block);
				continue;
			}
			fprintf(f, "  %-24s PlayerActor=%d Money=%d\n", l.name, ReadAt(l.block, kGlobalPlayerActor, l.wordSize), ReadAt(l.block, kGlobalMoney, l.wordSize));
		}

		// Scan both blocks for the needle as a 4-byte int and as a float.
		for (int pass = 0; pass < 2; ++pass)
		{
			uintptr_t block = pass == 0 ? base : indirect;
			if (!IsReadable((const void*)block, 4)) continue;
			size_t run = ReadableRun((const void*)block, kScanBytes);
			fprintf(f, "\nScanning %zu bytes from 0x%llX (%s) for %d:\n", run, (unsigned long long)block, pass == 0 ? "base" : "indirect", needle);

			int hits = 0;
			float needleF = (float)needle;
			for (size_t off = 0; off + 4 <= run && hits < kMaxHits; off += 4)
			{
				int v = *(const int*)(block + off);
				float fv = *(const float*)(block + off);
				bool asInt = v == needle;
				bool asFloat = fv == needleF;
				if (asInt || asFloat)
				{
					fprintf(f, "  +0x%06zX  (index4=%zu index8=%zu%s)  %s\n", off, off / 4, off / 8, (off % 8) ? " +4" : "", asInt ? "int" : "float");
					++hits;
				}
			}
			fprintf(f, "  %d hit(s)\n", hits);
		}
	}

	int DumpGlobalsGuarded(FILE* f, int needle, Actor player)
	{
		__try { DumpGlobalsRaw(f, needle, player); return 0; }
		__except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
	}
}

void Research::Dump(const std::string& dir, int needle)
{
	std::string statsPath = dir + "RDRBetterPresence.stats.txt";
	FILE* f = nullptr;
	if (fopen_s(&f, statsPath.c_str(), "w") == 0 && f)
	{
		fprintf(f, "id\tint\tfloat\n");
		int n = DumpStatsGuarded(f);
		fclose(f);
		if (n < 0) Log::Error("Exception while dumping stats");
		else Log::Info("Dumped %d stats to %s", n, statsPath.c_str());
	}

	std::string globalsPath = dir + "RDRBetterPresence.globals.txt";
	if (fopen_s(&f, globalsPath.c_str(), "w") == 0 && f)
	{
		Actor player = ACTOR::GET_PLAYER_ACTOR(-1);
		int rc = DumpGlobalsGuarded(f, needle, player);
		fclose(f);
		if (rc < 0) Log::Error("Exception while probing globals (partial file written)");
		else Log::Info("Probed globals (needle %d) to %s", needle, globalsPath.c_str());
	}
}
