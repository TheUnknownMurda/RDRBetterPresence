#include "pch.h"
#include "Plugin.h"

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls(module);
			Plugin::Initialize(module);
			break;

		case DLL_PROCESS_DETACH:
			// reserved != NULL means the process is terminating (not a FreeLibrary / RedHook "unload").
			Plugin::Shutdown(module, reserved != nullptr);
			break;
	}
	return TRUE;
}
