#include <stdio.h>
#include <xtl.h>
#include <vector>

#include "GameLib.h"

Detour* Scr_BeginLoadScripts_t = nullptr;
Detour* Scr_ReadFile_FastFile_t = nullptr;

int(__cdecl * FS_FOpenFileReadForThread)(const char* path, int* h, bool unk) = (decltype(FS_FOpenFileReadForThread))0x824C2B28;
int(__cdecl * FS_FileRead)(void* buf, int len, int h) = (decltype(FS_FileRead))0x82423F48;
void(__cdecl * FS_FileClose)(int h) = (decltype(FS_FileClose))0x82424468;

void(__cdecl * Scr_AddSourceBufferInternal)(const char *extFilename, const char *codePos, char *sourceBuf, int len, bool doEolFixup, bool archive, bool newBuffer) = (decltype(Scr_AddSourceBufferInternal))0x82468818;

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

	Scr_AddSourceBufferInternal(extFilename, codePos, script_buffer, file_size, true, archive, true);

	printf("Loaded Script File: %s\n", filename);

	cached_scripts.push_back(script_buffer);
	return script_buffer;
}

BOOL __stdcall DllMain(HANDLE hHandle, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH) 
	{
		printf("NX1Loader Loaded!\n");
		
		// Replace D: with game: to allow reading using game functions
		strcpy(reinterpret_cast<char*>(0x8282EFD8), "game:");

		Scr_BeginLoadScripts_t = new Detour(0x82465720, reinterpret_cast<DWORD>(Scr_BeginLoadScripts));
		Scr_ReadFile_FastFile_t = new Detour(0x82468AD8, reinterpret_cast<DWORD>(Scr_ReadFile_FastFile));
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