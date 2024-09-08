#pragma once
#include <stdio.h>
#include <xtl.h>

#define GAME_TITLE_ID 0x41560817
#define GAME_TITLE_UPDATE 0x8
#define GAME_POINTER_CHECK *(DWORD*)0x82022CC4
#define GAME_POINTER_VALUE 0x2F2F2067

extern "C" void * _ReturnAddress(void);

class Detour {
private:
	static byte HookSection[0x10000];
	static DWORD HookCount;

	DWORD dwAddress,
		*pAddress,
		*dwStubAddress,
		dwDestination,
		dwRestoreInstructions[4],
		dwCurrentInstructions[4];

	DWORD AllocStub();
	DWORD ResolveBranch(DWORD dwInstruction, DWORD dwBranchAddress);
	void PatchInJump(DWORD dwAddress, DWORD dwDestination, bool Linked);

public:
	Detour(DWORD dwAddress, DWORD dwDestination);
	~Detour();

	void* (*CallOriginal)(...);
};

void InitializeGameLib();

HANDLE CreateSystemThread(void* Function, void* Parameter, bool Close);