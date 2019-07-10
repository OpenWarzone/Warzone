/*
 * The contents of this file is licenced. You may obtain a copy of
 * the license at https://github.com/thsmi/libSharedMemory or request
 * it via email from the author. Do not remove or change this comment.
 *
 * The initial author of the code is:
 *   Thomas Schmid <schmid-thomas@gmx.net>
 */

#include "sharedMemory.h"
#include <Windows.h>
#include <stdio.h>

hSharedMemory* OpenSharedMemory(char* MapFileName, char* mutex, int size)
{
	hSharedMemory *handle = (hSharedMemory*)malloc(sizeof(hSharedMemory));
	handle->hFile = NULL;
	handle->hFileView = NULL;
	handle->hFileMutex = NULL;

	// Try to open File Mapping...
	handle->hFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, MapFileName);

	// ... ok it might not exist, so create try to create it...
	if (handle->hFile == NULL)
	{
		handle->hFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0/*size & 0xFFFF0000*/, size /*& 0xFFFF*/, MapFileName);
	}

	// ... if create fails we are lost...
	if (handle->hFile == 0)
	{
		CloseSharedMemory(handle);
		return NULL;
	}

	handle->hFileView = MapViewOfFile(handle->hFile, FILE_MAP_ALL_ACCESS, 0, 0, size);

	if (handle->hFileView == NULL)
	{
		CloseSharedMemory(handle);
		return NULL;
	}

	handle->hFileMutex = CreateMutexA(NULL, FALSE, mutex);

	if (handle->hFileMutex == NULL)
	{
		CloseSharedMemory(handle);
		return NULL;
	}

	return handle;
}

void CloseSharedMemory(hSharedMemory* handle)
{
	if (!handle)
		return;

	if (handle->hFileView)
		UnmapViewOfFile(handle->hFileView);

	handle->hFileView = NULL;

	if (handle->hFile)
		CloseHandle(handle->hFile);

	handle->hFile = NULL;

	if (handle->hFileMutex)
		CloseHandle(handle->hFileMutex);

	handle->hFileMutex = NULL;
	free(handle);
}

void* LockSharedMemory(hSharedMemory* handle)
{
	WaitForSingleObject(handle->hFileMutex, INFINITE);
	return handle->hFileView;
}

void UnlockSharedMemory(hSharedMemory* handle)
{
	ReleaseMutex(handle->hFileMutex);
}
