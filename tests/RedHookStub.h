#pragma once

// Declarations mirroring sdk/RedHook.h without __declspec(dllimport), so the
// test harness can provide its own implementations (see ipc_smoke.cpp).

enum LogType
{
	Log_Info = 1,
	Log_Warning,
	Log_Error
};

int RH_GetMinorVersion();
int RH_GetMajorVersion();

void ScriptWait(uint64_t _Ms);
void ScriptRegister(HMODULE _Module, void(*_Function)());
void ScriptUnregister(void(*_Function)());
void ScriptUnregister(HMODULE _Module);

void NativeInit(uint32_t _Hash);
void NativePush64(uint64_t _Value);
uint64_t* NativeCall();

void Print(LogType _Type, const char* _Format, ...);
