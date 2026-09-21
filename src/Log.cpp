#include "pch.h"
#include "Log.h"

#include <cstdarg>
#include <deque>

namespace
{
	std::mutex g_mutex;
	std::ofstream g_file;
	LogLevel g_minLevel = LogLevel::Info;
	bool g_toFile = true;
	bool g_toConsole = true;
	DWORD g_scriptThreadId = 0;

	struct Pending
	{
		LogLevel level;
		std::string text;
	};
	std::deque<Pending> g_pending;

	const char* LevelTag(LogLevel level)
	{
		switch (level)
		{
			case LogLevel::Debug:   return "DEBUG";
			case LogLevel::Info:    return "INFO ";
			case LogLevel::Warning: return "WARN ";
			case LogLevel::Error:   return "ERROR";
		}
		return "?????";
	}

	LogType ToRedHookType(LogLevel level)
	{
		switch (level)
		{
			case LogLevel::Warning: return Log_Warning;
			case LogLevel::Error:   return Log_Error;
			default:                return Log_Info;
		}
	}

	std::string Timestamp()
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		char buf[32];
		snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		return buf;
	}

	// RedHook's Print formats into a fixed buffer with a checked CRT function that aborts the
	// whole game on overflow (seen in a crash dump with a ~1 KB message), so console lines are
	// cut well below that. The log file always gets the full text.
	constexpr size_t kMaxConsoleChars = 600;

	// Must be called with g_mutex held, from the script thread.
	void PrintToConsoleLocked(LogLevel level, const std::string& text)
	{
		if (text.size() <= kMaxConsoleChars)
		{
			Print(ToRedHookType(level), "[BetterPresence] %s", text.c_str());
		}
		else
		{
			Print(ToRedHookType(level), "[BetterPresence] %s... (see log file)", text.substr(0, kMaxConsoleChars).c_str());
		}
	}
}

void Log::Init(const std::string& filePath, LogLevel minLevel, bool toFile, bool toConsole)
{
	std::lock_guard<std::mutex> lock(g_mutex);
	g_minLevel = minLevel;
	g_toFile = toFile;
	g_toConsole = toConsole;
	if (g_toFile)
	{
		g_file.open(filePath, std::ios::out | std::ios::trunc);
	}
}

void Log::Shutdown()
{
	std::lock_guard<std::mutex> lock(g_mutex);
	if (g_file.is_open())
	{
		g_file.flush();
		g_file.close();
	}
	g_pending.clear();
	g_scriptThreadId = 0;
}

void Log::MarkScriptThread()
{
	std::lock_guard<std::mutex> lock(g_mutex);
	g_scriptThreadId = GetCurrentThreadId();
}

void Log::FlushToConsole()
{
	std::lock_guard<std::mutex> lock(g_mutex);
	if (!g_toConsole || GetCurrentThreadId() != g_scriptThreadId)
	{
		return;
	}
	while (!g_pending.empty())
	{
		PrintToConsoleLocked(g_pending.front().level, g_pending.front().text);
		g_pending.pop_front();
	}
}

void Log::Write(LogLevel level, const char* format, ...)
{
	if (level < g_minLevel)
	{
		return;
	}

	char buffer[2048];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	std::lock_guard<std::mutex> lock(g_mutex);

	if (g_toFile && g_file.is_open())
	{
		g_file << Timestamp() << " [" << LevelTag(level) << "] " << buffer << '\n';
		g_file.flush();
	}

	if (g_toConsole && g_scriptThreadId != 0)
	{
		if (GetCurrentThreadId() == g_scriptThreadId)
		{
			PrintToConsoleLocked(level, buffer);
		}
		else
		{
			// Keep the queue bounded in case the script fiber never flushes (e.g. game stuck in a loading screen).
			if (g_pending.size() > 200)
			{
				g_pending.pop_front();
			}
			g_pending.push_back({ level, buffer });
		}
	}
}
