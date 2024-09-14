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

typedef struct _XEX_EXECUTION_ID { 
	DWORD MediaID; // 0x0 sz:0x4
	DWORD Version; // 0x4 sz:0x4
	DWORD BaseVersion; // 0x8 sz:0x4
	union {
		DWORD TitleID; // 0xC sz:0x4
		struct {
			WORD PublisherID; // 0xC sz:0x2
			WORD GameID; // 0xE sz:0x2
		};
	};
	BYTE Platform; // 0x10 sz:0x1
	BYTE ExecutableType; // 0x11 sz:0x1
	BYTE DiscNum; // 0x12 sz:0x1
	BYTE DiscsInSet; // 0x13 sz:0x1
	DWORD SaveGameID; // 0x14 sz:0x4
} XEX_EXECUTION_ID, *PXEX_EXECUTION_ID; // size 24
C_ASSERT(sizeof(XEX_EXECUTION_ID) == 0x18);

typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} STRING, *PSTRING;

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _LDR_DATA_TABLE_ENTRY { 
	LIST_ENTRY InLoadOrderLinks;  // 0x0 sz:0x8
	LIST_ENTRY InClosureOrderLinks;  // 0x8 sz:0x8
	LIST_ENTRY InInitializationOrderLinks; // 0x10 sz:0x8
	PVOID NtHeadersBase; // 0x18 sz:0x4
	PVOID ImageBase; // 0x1C sz:0x4
	DWORD SizeOfNtImage; // 0x20 sz:0x4
	UNICODE_STRING FullDllName; // 0x24 sz:0x8
	UNICODE_STRING BaseDllName; // 0x2C sz:0x8
	DWORD Flags; // 0x34 sz:0x4
	DWORD SizeOfFullImage; // 0x38 sz:0x4
	PVOID EntryPoint; // 0x3C sz:0x4
	WORD LoadCount; // 0x40 sz:0x2
	WORD ModuleIndex; // 0x42 sz:0x2
	PVOID DllBaseOriginal; // 0x44 sz:0x4
	DWORD CheckSum; // 0x48 sz:0x4
	DWORD ModuleLoadFlags; // 0x4C sz:0x4
	DWORD TimeDateStamp; // 0x50 sz:0x4
	PVOID LoadedImports; // 0x54 sz:0x4
	PVOID XexHeaderBase; // 0x58 sz:0x4
	union{
		STRING LoadFileName; // 0x5C sz:0x8
		struct {
			PVOID ClosureRoot; // 0x5C sz:0x4 LDR_DATA_TABLE_ENTRY
			PVOID TraversalParent; // 0x60 sz:0x4 LDR_DATA_TABLE_ENTRY
		} asEntry;
	} inf;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY; // size 100

#define NTSTATUS long

extern "C"
{
	NTSYSAPI
	NTSTATUS
	NTAPI
	XamGetExecutionId(
		IN OUT	PXEX_EXECUTION_ID* xid
	);

	NTSYSAPI
	DWORD
	NTAPI
	XamGetCurrentTitleId(
		VOID
	);

	extern PLDR_DATA_TABLE_ENTRY* XexExecutableModuleHandle;
}

void InitializeGameLib();

HANDLE CreateSystemThread(void* Function, void* Parameter, bool Close);