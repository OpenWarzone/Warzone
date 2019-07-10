/* 
 * The contents of this file is licenced. You may obtain a copy of
 * the license at https://github.com/thsmi/libSharedMemory or request
 * it via email from the author. Do not remove or change this comment. 
 * 
 * The initial author of the code is:
 *   Thomas Schmid <schmid-thomas@gmx.net>
 */

#ifndef SHAREDMEMORY_H_
#define SHAREDMEMORY_H_

#include <windows.h>

typedef struct {
  HANDLE hFile;
  HANDLE hFileView;
  HANDLE hFileMutex;
} hSharedMemory;

hSharedMemory* OpenSharedMemory(char* MapFileName, char* Mutex, int size);
void CloseSharedMemory(hSharedMemory* handle);
void* LockSharedMemory(hSharedMemory* handle);
void UnlockSharedMemory(hSharedMemory* handle);

#endif /* SHAREDMEMORY_H_ */
