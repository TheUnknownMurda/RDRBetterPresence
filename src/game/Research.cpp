#include "pch.h"
#include "Research.h"
#include "../Log.h"

namespace
{
	constexpr int kStatCount = 700;          // Foxxyyy counted 696 stats in the scripts
	constexpr int kMaxHits = 20000;

	bool IsReadable(const void* p, size_t size)
	{
		MEMORY_BASIC_INFORMATION mbi{};
		if (!p || !VirtualQuery(p, &mbi, sizeof(mbi))) return false;
		if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return false;
		return (const char*)p + size <= (const char*)mbi.BaseAddress + mbi.RegionSize;
	}

	// ---------------------------------------------------------------------------------------
	// Stats dump (script fiber)
	// ---------------------------------------------------------------------------------------

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

	// ---------------------------------------------------------------------------------------
	// Process-wide value scan (background thread, so the game does not freeze)
	// ---------------------------------------------------------------------------------------

	struct ScanJob
	{
		std::string path;
		int needle;
		uintptr_t globalsBase;
	};

	std::atomic<bool> g_scanRunning = false;

	bool ScannableRegion(const MEMORY_BASIC_INFORMATION& mbi)
	{
		if (mbi.State != MEM_COMMIT) return false;
		if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;
		DWORD p = mbi.Protect & 0xFF;
		return p == PAGE_READWRITE || p == PAGE_EXECUTE_READWRITE || p == PAGE_WRITECOPY || p == PAGE_EXECUTE_WRITECOPY;
	}

	DWORD WINAPI ScanThread(LPVOID param)
	{
		std::unique_ptr<ScanJob> job((ScanJob*)param);

		FILE* f = nullptr;
		if (fopen_s(&f, job->path.c_str(), "w") != 0 || !f)
		{
			Log::Error("Cannot write %s", job->path.c_str());
			g_scanRunning = false;
			return 1;
		}

		auto t0 = std::chrono::steady_clock::now();
		float needleF = (float)job->needle;
		unsigned long long scanned = 0;
		int hits = 0;
		int regions = 0;

		fprintf(f, "needle=%d  GetGlobalPtr()=0x%llX\n", job->needle, (unsigned long long)job->globalsBase);
		fprintf(f, "address\ttype\tregion_base\tregion_type\toffset_from_globals\n");

		SYSTEM_INFO si;
		GetSystemInfo(&si);
		const char* cur = (const char*)si.lpMinimumApplicationAddress;
		const char* end = (const char*)si.lpMaximumApplicationAddress;

		while (cur < end && hits < kMaxHits)
		{
			MEMORY_BASIC_INFORMATION mbi{};
			if (!VirtualQuery(cur, &mbi, sizeof(mbi))) break;
			const char* next = (const char*)mbi.BaseAddress + mbi.RegionSize;

			if (ScannableRegion(mbi))
			{
				++regions;
				const char* regionType = (mbi.Type == MEM_IMAGE) ? "image" : (mbi.Type == MEM_MAPPED) ? "mapped" : "private";
				const char* p = (const char*)mbi.BaseAddress;
				size_t size = mbi.RegionSize;
				scanned += size;

				for (size_t off = 0; off + 4 <= size && hits < kMaxHits; off += 4)
				{
					int v = *(const int*)(p + off);
					bool asInt = v == job->needle;
					bool asFloat = !asInt && *(const float*)(p + off) == needleF;
					if (asInt || asFloat)
					{
						uintptr_t addr = (uintptr_t)(p + off);
						long long fromGlobals = (long long)addr - (long long)job->globalsBase;
						fprintf(f, "0x%llX\t%s\t0x%llX\t%s\t%lld\n", (unsigned long long)addr, asInt ? "int" : "float",
							(unsigned long long)mbi.BaseAddress, regionType, fromGlobals);
						++hits;
					}
				}
			}
			cur = next;
		}

		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
		fprintf(f, "\n%d hit(s) in %d region(s), %.1f MB scanned in %lld ms%s\n",
			hits, regions, scanned / (1024.0 * 1024.0), (long long)ms, hits >= kMaxHits ? " (hit cap reached)" : "");
		fclose(f);

		Log::Info("Memory scan for %d: %d hit(s), %.0f MB in %lld ms -> %s", job->needle, hits, scanned / (1024.0 * 1024.0), (long long)ms, job->path.c_str());
		g_scanRunning = false;
		return 0;
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

	if (needle == 0)
	{
		Log::Info("ResearchNeedle is 0: memory scan skipped");
		return;
	}
	if (g_scanRunning.exchange(true))
	{
		Log::Warning("A memory scan is already running");
		return;
	}

	auto* job = new ScanJob{ dir + "RDRBetterPresence.scan.txt", needle, GetGlobalPtr() };
	HANDLE h = CreateThread(nullptr, 0, ScanThread, job, 0, nullptr);
	if (!h)
	{
		delete job;
		g_scanRunning = false;
		Log::Error("Cannot start the memory scan thread");
		return;
	}
	CloseHandle(h);
	Log::Info("Memory scan for %d started in the background", needle);
}
