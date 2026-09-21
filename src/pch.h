#pragma once

// Precompiled header for RDRBetterPresence.
// Windows + STL first, then the RedHook SDK (which expects <windows.h> to be included before it).

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

// RedHook SDK (vendored in /sdk, MIT - (c) K3rhos 2024)
#ifdef RDRBP_NO_REDHOOK
// Test harness build: same declarations without dllimport, implemented as stubs in tests/.
#include "../tests/RedHookStub.h"
#else
#include <RedHook.h>
#endif
#include <NativesCaller.h>
#include <RHMath.h>
#include <Enums.h>
#include <Structs.h>
#include <Natives.h>
