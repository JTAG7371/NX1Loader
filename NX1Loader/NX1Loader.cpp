#include <stdio.h>
#include <xtl.h>
#include <string>
#include <vector>
#include <cstdint>
#include <cwctype>
#include <algorithm>

#include "GameLib.h"

struct GameOffsets
{
	std::wstring module_name;
	std::uint32_t date_timestamp;
	std::uint32_t cwd;
	std::uint32_t scr_begin_load_scripts;
	std::uint32_t scr_read_file_fastfile;
	std::uint32_t fs_fopen_file_read_for_thread;
	std::uint32_t fs_file_read;
	std::uint32_t fs_file_close;
	std::uint32_t scr_add_source_buffer_internal;
};

GameOffsets game_offsets[8] = 
{
	{ L"0-Convoy Test 1_5.mp.xex", 0x4F05FDAC, 0x825967D8, 0x822FE2D8, 0x822FEF38, 0x8235BFC0, 0x822CAF90, 0x822CB010, 0 },
	{ L"0-Nightly MP maps.mp.xex", 0x4F0A8382, 0x82940590, 0x82480250, 0x824821B8, 0x825096F8, 0x82428438, 0x82428958, 0x82481F78 },
	{ L"1-nx1sp.xex", 0x4EEEC57E, 0x8282EFD8, 0x82465720, 0x82468AD8, 0x824C2B28, 0x82423F48, 0x82424468, 0x82468818 },
	{ L"2-nx1mp.xex", 0x4EEEC590, 0x829A0510, 0x82497148, 0x82499FA0, 0x8252E9D0, 0x82439748, 0x82439C68, 0x82499CE0 },
	{ L"3-nx1sp_fast_server.xex", 0x4EEEC5C5, 0x826FEFD8, 0x823D15A0, 0x823D36C0, 0x8241AE30, 0x8239A7E0, 0x8239AC28, 0x823D33E8 },
	// { L"4-nx1sp_demo.xex", 0, 0, 0, 0, 0, 0, 0, 0 }, // This executable seems to be completely broken
	{ L"5-nx1mp_demo.xex", 0x4F05FDAC, 0x825967D8, 0x822FE2D8, 0x822FEF38, 0x8235BFC0, 0x822CAF90, 0x822CB010, 0 },
	{ L"6-nx1mp_fast_server.xex", 0x4F0A8382, 0x82940590, 0x82480250, 0x824821B8, 0x825096F8, 0x82428438, 0x82428958, 0x82481F78 },
};

GameOffsets* LoadForSupportedGame(std::uint32_t title_id, PLDR_DATA_TABLE_ENTRY data_table_entry)
{
	auto date_timestamp = data_table_entry->TimeDateStamp;
	
	// Make sure this is being loaded on the correct title
	if (title_id != 0x4156089E)
	{
		printf("Invalid TitleID - Title ID: 0x%X - BaseDllName: %ws - Timestamp: %X\n", title_id, data_table_entry->BaseDllName.Buffer, data_table_entry->TimeDateStamp);
		return nullptr;
	}
	
	// Attempt to find correct game executable
	for (auto i = 0; i < ARRAYSIZE(game_offsets); i++)
	{
		// Compare against module name and timestamp
		if (!wcsicmp(data_table_entry->BaseDllName.Buffer, game_offsets[i].module_name.data()) && data_table_entry->TimeDateStamp == game_offsets[i].date_timestamp)
		{
			return &game_offsets[i];
		}

		// Some people may have patched executables but the timestamp should not change
		if (data_table_entry->TimeDateStamp == game_offsets[i].date_timestamp)
		{
			return &game_offsets[i];
		}
	}

	// This is a new executable potentially
	printf("Failed to find compatible executable - Title ID: 0x%X - BaseDllName: %ws - Timestamp: %X\n", title_id, data_table_entry->BaseDllName.Buffer, data_table_entry->TimeDateStamp);
	return nullptr;
}

Detour* Scr_BeginLoadScripts_t = nullptr;
Detour* Scr_ReadFile_FastFile_t = nullptr;

int(__cdecl * FS_FOpenFileReadForThread)(const char* path, int* h, bool isAsync) = nullptr;
int(__cdecl * FS_FileRead)(void* buf, int len, int h) = nullptr;
void(__cdecl * FS_FileClose)(int h) = nullptr;

void(__cdecl * Scr_AddSourceBufferInternal)(const char *extFilename, const char *codePos, char *sourceBuf, int len, bool doEolFixup, bool archive, bool newBuffer) = nullptr;

std::vector<char*> cached_scripts;

void Scr_BeginLoadScripts()
{
	printf("Resetting Script Cache\n");

	for (auto i = 0u; i < cached_scripts.size(); i++)
	{
		free(cached_scripts[i]);
	}

	cached_scripts.clear();

	Scr_BeginLoadScripts_t->CallOriginal();
}

char * Scr_ReadFile_FastFile(const char *filename, const char *extFilename, const char *codePos, bool archive)
{
	char buffer[512] = { 0 };
	sprintf(buffer, "raw/%s", filename);

	auto file_handle = 0;
	auto file_size = FS_FOpenFileReadForThread(buffer, &file_handle, false);

	if (file_size == -1)
	{
		return reinterpret_cast<char*>(Scr_ReadFile_FastFile_t->CallOriginal(filename, extFilename, codePos, archive));
	}

	char* script_buffer = reinterpret_cast<char*>(calloc(file_size + 1, 1));
	FS_FileRead(script_buffer, file_size, file_handle);
	FS_FileClose(file_handle);

	if (Scr_AddSourceBufferInternal != nullptr)
	{
		Scr_AddSourceBufferInternal(extFilename, codePos, script_buffer, file_size, true, archive, true);
	}

	printf("Loaded Script File: %s\n", filename);

	cached_scripts.push_back(script_buffer);
	return script_buffer;
}

BOOL __stdcall DllMain(HANDLE hHandle, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH) 
	{
		printf("NX1Loader Loaded!\n");
		
		if (auto* game = LoadForSupportedGame(XamGetCurrentTitleId(), *XexExecutableModuleHandle))
		{
			// Initialize Functions
			FS_FOpenFileReadForThread = (decltype(FS_FOpenFileReadForThread))game->fs_fopen_file_read_for_thread;
			FS_FileRead = (decltype(FS_FileRead))game->fs_file_read;
			FS_FileClose = (decltype(FS_FileClose))game->fs_file_close;
			Scr_AddSourceBufferInternal = (decltype(Scr_AddSourceBufferInternal))game->scr_add_source_buffer_internal;

			// Replace D: with game: to allow reading using game functions
			strcpy(reinterpret_cast<char*>(game->cwd), "game:");

			Scr_BeginLoadScripts_t = new Detour(game->scr_begin_load_scripts, reinterpret_cast<DWORD>(Scr_BeginLoadScripts));
			Scr_ReadFile_FastFile_t = new Detour(game->scr_read_file_fastfile, reinterpret_cast<DWORD>(Scr_ReadFile_FastFile));

			return TRUE;
		}

		return FALSE;
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		printf("NX1Loader Unloaded!\n");
		
		if (Scr_BeginLoadScripts_t)
		{
			delete Scr_BeginLoadScripts_t;
		}

		if (Scr_ReadFile_FastFile_t)
		{
			delete Scr_ReadFile_FastFile_t;
		}
	}

	return TRUE;
}