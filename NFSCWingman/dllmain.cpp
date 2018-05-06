#include "stdafx.h"
#include "stdio.h"
#include <windows.h>
#include "..\includes\injector\injector.hpp"
#include <cstdint>
#include "..\includes\IniReader.h"

int ForceRole, FakeBoss, WingmanEverywhere, CrewRole, HotkeyChangeRole;
char *ForcePreset, *ForceName;
bool HireAnyone;

DWORD WINAPI WingmanThing(LPVOID);

DWORD ForceRoleCodeCaveEntry = 0x4CD888;
DWORD ForceRoleCodeCaveExit = 0x4CD88E;

int(__cdecl* EventSys_Event_Allocate)(int size, int priority) = (int(__cdecl*)(int, int))0x561590;
int(__thiscall* EChangeWingmanRole)(int _EventSys, char const *NewRole) = (int(__thiscall*)(int, char const*))0x66C5D0;

void ChangeCrewRole(char const* NewRole)
{
	int EventSys = EventSys_Event_Allocate(12, 0);
	if (EventSys != 0) EChangeWingmanRole(EventSys, NewRole);
}

char* ForceNameHook()
{
	return ForceName;
}

void __declspec(naked) ForceRoleCodeCave()
{
	_asm
	{
		push ecx
		mov ecx, ForceRole // crew type: 1 = none, 2 = drafter, 3 = blocker, 4 = scout
		mov [eax], ecx
		pop ecx
		
		mov eax, [eax]
		mov ecx, dword ptr ds : [esp + 0x08]
		jmp ForceRoleCodeCaveExit
	}
}

DWORD HireAnyoneCodeCaveEntry = 0x62F056;
DWORD HireAnyoneCodeCaveExit = 0x62F05C;

void __declspec(naked) HireAnyoneCodeCave()
{
	_asm
	{
		mov dword ptr ds : [eax], 0xC49D46EF //InitialCrew = crews/crew_player
		mov ecx, [eax]
		mov dword ptr ds : [esp + 0x2C], ecx
		jmp HireAnyoneCodeCaveExit
	}
}

DWORD CrashAndSoundFixCodeCaveEntry = 0x77FAFE;
DWORD CrashAndSoundFixCodeCaveExit = 0x77FB05;

void __declspec(naked) CrashAndSoundFixCodeCave()
{
	_asm
	{
		je nikki
		mov esi, 0
		jmp CrashAndSoundFixCodeCaveExit

		nikki :
		mov esi, 0x0B
		jmp CrashAndSoundFixCodeCaveExit
	}
}

DWORD ForcePresetCodeCaveEntry = 0x64685B;
DWORD ForcePresetCodeCaveExit = 0x646860;

void __declspec(naked) ForcePresetCodeCave()
{
	_asm
	{
		mov ecx, ForcePreset
		push ecx
		jmp ForcePresetCodeCaveExit
	}
}

int WingmanEverywhereHook()
{
	return WingmanEverywhere;
}

void Init()
{

	CIniReader ini("NFSCWingmanSettings.ini");

	HotkeyChangeRole = ini.ReadInteger("Hotkeys", "ChangeRole", 85); // U

	ForceRole = ini.ReadInteger("Crew", "ForceRole", 0);
	ForcePreset = ini.ReadString("Crew", "ForcePreset", "0");
	ForceName = ini.ReadString("Crew", "ForceName", "0");
	HireAnyone = ini.ReadInteger("Crew", "HireAnyone", 0) == 1;
	WingmanEverywhere = ini.ReadInteger("Crew", "WingmanEverywhere", 0);

	FakeBoss = ini.ReadInteger("Other", "ForceFakeBoss", 0);

	// Force Wingman Role
	if (ForceRole != 0)
	{
		injector::MakeRangedNOP(ForceRoleCodeCaveEntry, ForceRoleCodeCaveExit, true);
		injector::MakeJMP(ForceRoleCodeCaveEntry, ForceRoleCodeCave, true);
	}

	// Hire Anyone into Your Crew from Crew menu
	if (HireAnyone != 0)
	{
		injector::MakeRangedNOP(HireAnyoneCodeCaveEntry, HireAnyoneCodeCaveExit, true);
		injector::MakeJMP(HireAnyoneCodeCaveEntry, HireAnyoneCodeCave, true);

		// Fix crew crash and sound
		injector::MakeRangedNOP(CrashAndSoundFixCodeCaveEntry, CrashAndSoundFixCodeCaveExit, true);
		injector::MakeJMP(CrashAndSoundFixCodeCaveEntry, CrashAndSoundFixCodeCave, true);

		injector::MakeNOP(0x77fb2e, 2, true);
	}

	// Force enable crew member in every event
	if (WingmanEverywhere != 0)
	{
		injector::MakeCALL(0x4A2898, WingmanEverywhereHook, true);
		injector::MakeCALL(0x655FCE, WingmanEverywhereHook, true);
		injector::MakeCALL(0x655FEA, WingmanEverywhereHook, true);
		injector::MakeCALL(0x668A44, WingmanEverywhereHook, true);
	}

	// Force Preset
	if (strcmp(ForcePreset, "0") != 0)
	{
		injector::MakeJMP(ForcePresetCodeCaveEntry, ForcePresetCodeCave, true);
	}

	// Force Name
	if (strcmp(ForceName,"0") != 0)
	{
		injector::WriteMemory(0x9D8874, &ForceNameHook, true);
	}

	// Force Fake Boss
	injector::WriteMemory<unsigned char>(0xA9E66C, FakeBoss, true);

	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&WingmanThing, NULL, 0, NULL);
}


DWORD WINAPI WingmanThing(LPVOID)
{
	while (true)
	{
		
		if ((GetAsyncKeyState(HotkeyChangeRole) & 1) && (*(int*)0xA99BBC == 6))
		{
			CrewRole = (CrewRole + 1) % 4;

			switch (CrewRole)
			{
			case 0:
				ChangeCrewRole("PathFinder");
				break;
			case 1:
				ChangeCrewRole("Blocker");
				break;
			case 2:
				ChangeCrewRole("Drafter");
				break;
			case 3: default:
				ChangeCrewRole("None");
				break;
			}

			while ((GetAsyncKeyState(HotkeyChangeRole) & 0x8000) > 0) { Sleep(1); }
		}
	}
	return 0;
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;

}

