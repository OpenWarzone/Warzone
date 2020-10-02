#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:			
			DisableThreadLibraryCalls(hModule);
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:			
			break;
		case DLL_PROCESS_DETACH:
			FreeLibraryAndExitThread (hModule, 0);
			break;
	}
	return TRUE;
}

