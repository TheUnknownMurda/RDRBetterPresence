#pragma once

namespace Plugin
{
	void Initialize(HMODULE module);
	// processTerminating: true when the game itself is exiting (no cleanup possible / needed).
	void Shutdown(HMODULE module, bool processTerminating);
}
