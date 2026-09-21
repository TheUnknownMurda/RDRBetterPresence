#include "pch.h"
#include "Regions.h"
#include "../Log.h"

namespace
{
	struct Rect
	{
		std::string name;
		float xMin, yMin, xMax, yMax;
	};

	// Smaller rectangles first so towns inside a region win over the region itself.
	std::vector<Rect> g_overrides;

	// Known GXT-style labels -> display names, in case the game hands us a label
	// instead of the localized text. Extend as new values show up in the log.
	const std::unordered_map<std::string, std::string> kLabelToName =
	{
		{ "DIST_CHOLLA",     "Cholla Springs" },
		{ "DIST_RIOBRAVO",   "Rio Bravo" },
		{ "DIST_GAPTOOTH",   "Gaptooth Ridge" },
		{ "DIST_HENNIGAN",   "Hennigan's Stead" },
		{ "DIST_DIEZ",       "Diez Coronas" },
		{ "DIST_PUNTA",      "Punta Orgullosa" },
		{ "DIST_PERDIDO",    "Perdido" },
		{ "DIST_TALLTREES",  "Tall Trees" },
		{ "DIST_GREATPLAINS","Great Plains" },
	};

	// Strip the most common accented characters (UTF-8) so slugs stay ASCII.
	std::string StripAccents(std::string_view in)
	{
		static const std::pair<const char*, char> kMap[] =
		{
			{ "\xC3\xA1", 'a' }, { "\xC3\xA0", 'a' }, { "\xC3\xA2", 'a' }, { "\xC3\xA4", 'a' }, { "\xC3\xA3", 'a' },
			{ "\xC3\xA9", 'e' }, { "\xC3\xA8", 'e' }, { "\xC3\xAA", 'e' }, { "\xC3\xAB", 'e' },
			{ "\xC3\xAD", 'i' }, { "\xC3\xAC", 'i' }, { "\xC3\xAE", 'i' }, { "\xC3\xAF", 'i' },
			{ "\xC3\xB3", 'o' }, { "\xC3\xB2", 'o' }, { "\xC3\xB4", 'o' }, { "\xC3\xB6", 'o' }, { "\xC3\xB5", 'o' },
			{ "\xC3\xBA", 'u' }, { "\xC3\xB9", 'u' }, { "\xC3\xBB", 'u' }, { "\xC3\xBC", 'u' },
			{ "\xC3\xB1", 'n' }, { "\xC3\xA7", 'c' },
			{ "\xC3\x81", 'A' }, { "\xC3\x89", 'E' }, { "\xC3\x8D", 'I' }, { "\xC3\x93", 'O' }, { "\xC3\x9A", 'U' },
			{ "\xC3\x91", 'N' }, { "\xC3\x87", 'C' },
		};

		std::string out;
		out.reserve(in.size());
		for (size_t i = 0; i < in.size(); ++i)
		{
			bool replaced = false;
			if ((static_cast<unsigned char>(in[i]) & 0x80) && i + 1 < in.size())
			{
				for (const auto& [seq, repl] : kMap)
				{
					if (in[i] == seq[0] && in[i + 1] == seq[1])
					{
						out += repl;
						++i;
						replaced = true;
						break;
					}
				}
			}
			if (!replaced)
			{
				out += in[i];
			}
		}
		return out;
	}
}

std::string Regions::Slugify(std::string_view text)
{
	std::string ascii = StripAccents(text);
	std::string slug;
	bool lastUnderscore = true;
	for (unsigned char c : ascii)
	{
		if (isalnum(c))
		{
			slug += (char)tolower(c);
			lastUnderscore = false;
		}
		else if (!lastUnderscore)
		{
			slug += '_';
			lastUnderscore = true;
		}
	}
	while (!slug.empty() && slug.back() == '_')
	{
		slug.pop_back();
	}
	return slug;
}

void Regions::LoadOverrides(const std::string& iniPath)
{
	g_overrides.clear();

	// GetPrivateProfileSection returns "key=value\0key=value\0\0".
	std::vector<char> buffer(32 * 1024);
	DWORD len = GetPrivateProfileSectionA("Regions", buffer.data(), (DWORD)buffer.size(), iniPath.c_str());
	if (len == 0)
	{
		return;
	}

	const char* p = buffer.data();
	while (*p)
	{
		std::string line(p);
		p += line.size() + 1;

		size_t eq = line.find('=');
		if (eq == std::string::npos) continue;

		Rect r;
		r.name = line.substr(0, eq);
		std::string values = line.substr(eq + 1);
		for (auto& ch : values) if (ch == ';') ch = ',';

		float v[4];
		int n = sscanf_s(values.c_str(), "%f,%f,%f,%f", &v[0], &v[1], &v[2], &v[3]);
		if (n != 4)
		{
			Log::Warning("regions.ini: cannot parse '%s' (expected Name=xmin,ymin,xmax,ymax)", line.c_str());
			continue;
		}
		r.xMin = std::min(v[0], v[2]); r.xMax = std::max(v[0], v[2]);
		r.yMin = std::min(v[1], v[3]); r.yMax = std::max(v[1], v[3]);
		g_overrides.push_back(r);
	}

	std::sort(g_overrides.begin(), g_overrides.end(), [](const Rect& a, const Rect& b)
	{
		return (a.xMax - a.xMin) * (a.yMax - a.yMin) < (b.xMax - b.xMin) * (b.yMax - b.yMin);
	});

	Log::Info("Loaded %zu region override(s) from regions.ini", g_overrides.size());
}

std::optional<RegionInfo> Regions::Resolve(const GameSnapshot& s)
{
	// 1. User-defined rectangles (towns, landmarks) have priority: they are more precise.
	for (const Rect& r : g_overrides)
	{
		if (s.posX >= r.xMin && s.posX <= r.xMax && s.posY >= r.yMin && s.posY <= r.yMax)
		{
			return RegionInfo{ r.name, Slugify(r.name) };
		}
	}

	// 2. What the game says.
	if (!s.district.empty())
	{
		auto it = kLabelToName.find(s.district);
		std::string name = (it != kLabelToName.end()) ? it->second : s.district;
		return RegionInfo{ name, Slugify(name) };
	}

	return std::nullopt;
}
