#pragma once

// Minimal thread-safe logger.
// - Always appends to a log file next to the plugin (if enabled).
// - Mirrors messages to the RedHook F8 console, but only from the script fiber:
//   RedHook's Print() is not documented as thread-safe, so messages logged from
//   the Discord worker thread are queued and flushed by the script loop.

enum class LogLevel
{
	Debug = 0,
	Info,
	Warning,
	Error
};

namespace Log
{
	void Init(const std::string& filePath, LogLevel minLevel, bool toFile, bool toConsole);
	void Shutdown();

	// Call once from the RedHook script fiber so we know which thread may talk to the console.
	void MarkScriptThread();
	// Call regularly from the script fiber to push queued messages to the F8 console.
	void FlushToConsole();

	void Write(LogLevel level, const char* format, ...);

	// File only: never forwarded to the RedHook console. Use for verbose research output -
	// RedHook's Print has crashed the game on some long / bracket-heavy messages.
	void FileOnly(const char* format, ...);

	template <typename... Args>
	inline void Debug(const char* format, Args... args) { Write(LogLevel::Debug, format, args...); }
	template <typename... Args>
	inline void Info(const char* format, Args... args) { Write(LogLevel::Info, format, args...); }
	template <typename... Args>
	inline void Warning(const char* format, Args... args) { Write(LogLevel::Warning, format, args...); }
	template <typename... Args>
	inline void Error(const char* format, Args... args) { Write(LogLevel::Error, format, args...); }
}
