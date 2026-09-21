#include "pch.h"
#include "Regions.h"
#include "../Log.h"

namespace
{
	// -------------------------------------------------------------------------------------------
	// Built-in landmarks. Coordinates (x, z) come from community teleport tables for the PC port
	// (connor-ms/RDR-Trainer, TheRouletteBoi/RedDeadRedemption_PS3); region membership from the
	// Red Dead wiki. Units are roughly metres; a radius of ~200 covers a town.
	// -------------------------------------------------------------------------------------------
	struct Landmark
	{
		const char* name;
		const char* region;
		float x, z;
		float radius;
	};

	constexpr const char* kChollaSprings  = "Cholla Springs";
	constexpr const char* kRioBravo       = "R\xC3\xADo Bravo";        // Río Bravo
	constexpr const char* kGaptoothRidge  = "Gaptooth Ridge";
	constexpr const char* kHennigansStead = "Hennigan's Stead";
	constexpr const char* kPuntaOrgullo   = "Punta Orgullo";
	constexpr const char* kPerdido        = "Perdido";
	constexpr const char* kDiezCoronas    = "Diez Coronas";
	constexpr const char* kTallTrees      = "Tall Trees";
	constexpr const char* kGreatPlains    = "Great Plains";

	const Landmark kLandmarks[] =
	{
		// --- New Austin ---
		{ "Armadillo",              kChollaSprings,  -2176.0f, 2614.0f, 260.0f },
		{ "Coot's Chapel",          kChollaSprings,  -1793.0f, 2837.0f, 150.0f },
		{ "Twin Rocks",             kChollaSprings,  -2425.0f, 2139.0f, 150.0f },
		{ "Lake Don Julio",         kChollaSprings,  -1955.0f, 3256.0f, 250.0f },
		{ "Ridgewood Farm",         kChollaSprings,  -3275.0f, 2720.0f, 180.0f },
		{ "Fort Mercer",            kRioBravo,       -2623.0f, 3391.0f, 220.0f },
		{ "Plainview",              kRioBravo,       -3126.0f, 3724.0f, 220.0f },
		{ "Tumbleweed",             kGaptoothRidge,  -4007.0f, 2935.0f, 240.0f },
		{ "Gaptooth Breach",        kGaptoothRidge,  -4462.0f, 3310.0f, 220.0f },
		{ "Rathskeller Fork",       kGaptoothRidge,  -3662.0f, 2125.0f, 200.0f },
		{ "Benedict Point",         kGaptoothRidge,  -3687.0f, 3493.0f, 200.0f },
		{ "MacFarlane's Ranch",     kHennigansStead,  -887.0f, 2421.0f, 300.0f },
		{ "Thieves' Landing",       kHennigansStead,   112.0f, 2319.0f, 250.0f },
		{ "Pacific Union Camp",     kHennigansStead,  -274.0f, 2113.0f, 160.0f },

		// --- Nuevo Paraíso ---
		{ "Escalera",               kPuntaOrgullo,   -4279.0f, 4448.0f, 280.0f },
		{ "Nosalida",               kPuntaOrgullo,   -4702.0f, 3959.0f, 180.0f },
		{ "Tesoro Azul",            kPuntaOrgullo,   -3288.0f, 4547.0f, 200.0f },
		{ "Chuparosa",              kPerdido,        -2715.0f, 4252.0f, 260.0f },
		{ "Las Hermanas",           kPerdido,        -1700.0f, 4242.0f, 200.0f },
		{ "Agave Viejo",            kPerdido,        -1545.0f, 3913.0f, 180.0f },
		{ "El Presidio",            kDiezCoronas,     -698.0f, 3323.0f, 240.0f },
		{ "Casa Madrugada",         kDiezCoronas,     -789.0f, 3730.0f, 200.0f },
		{ "El Matadero",            kDiezCoronas,     -455.0f, 3927.0f, 180.0f },
		{ "Torquemada",             kDiezCoronas,      377.0f, 3460.0f, 220.0f },

		// --- West Elizabeth ---
		{ "Blackwater",             kGreatPlains,      711.0f, 1253.0f, 350.0f },
		{ "Beecher's Hope",         kGreatPlains,      -83.0f, 1374.0f, 220.0f },
		{ "Wreck of the Serendipity", kGreatPlains,    325.0f, 1940.0f, 150.0f },
		{ "Manzanita Post",         kTallTrees,       -428.0f, 1616.0f, 200.0f },
		{ "Cochinay",               kTallTrees,       -739.0f,  785.0f, 200.0f },
	};

	struct Rect
	{
		std::string name;
		float xMin, zMin, xMax, zMax;
	};

	// Smaller rectangles first so a town inside a region rectangle wins over the region.
	std::vector<Rect> g_overrides;

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
			Log::Warning("regions.ini: cannot parse '%s' (expected Name=xmin,zmin,xmax,zmax)", line.c_str());
			continue;
		}
		r.xMin = std::min(v[0], v[2]); r.xMax = std::max(v[0], v[2]);
		r.zMin = std::min(v[1], v[3]); r.zMax = std::max(v[1], v[3]);
		g_overrides.push_back(r);
	}

	std::sort(g_overrides.begin(), g_overrides.end(), [](const Rect& a, const Rect& b)
	{
		return (a.xMax - a.xMin) * (a.zMax - a.zMin) < (b.xMax - b.xMin) * (b.zMax - b.zMin);
	});

	Log::Info("Loaded %zu region override(s) from regions.ini", g_overrides.size());
}

std::optional<RegionInfo> Regions::Resolve(const GameSnapshot& s)
{
	// Nearest built-in landmark decides the region; it also names the place when close.
	const Landmark* nearest = nullptr;
	float nearestDist2 = 0.0f;
	for (const Landmark& l : kLandmarks)
	{
		float dx = s.posX - l.x;
		float dz = s.posZ - l.z;
		float d2 = dx * dx + dz * dz;
		if (!nearest || d2 < nearestDist2)
		{
			nearest = &l;
			nearestDist2 = d2;
		}
	}

	RegionInfo info;
	if (nearest)
	{
		info.region = nearest->region;
		info.regionSlug = Slugify(nearest->region);
		if (nearestDist2 <= nearest->radius * nearest->radius)
		{
			info.place = nearest->name;
		}
	}

	// User rectangles override the place name (and the region when none is known).
	for (const Rect& r : g_overrides)
	{
		if (s.posX >= r.xMin && s.posX <= r.xMax && s.posZ >= r.zMin && s.posZ <= r.zMax)
		{
			info.place = r.name;
			if (info.region.empty())
			{
				info.region = r.name;
				info.regionSlug = Slugify(r.name);
			}
			break;
		}
	}

	if (info.region.empty())
	{
		return std::nullopt;
	}
	return info;
}
