// Filename:-	cl_mp3.cpp
//
// (The interface module between all the MP3 stuff and the game)


#include "client.h"
#include "snd_local.h"
#include "snd_mp3.h"					// only included directly by a few snd_xxxx.cpp files plus this one

// expects data already loaded, filename arg is for error printing only
//
// returns success/fail
//
qboolean MP3_IsMusic( const char *psLocalFilename, void *pvData, int iDataLen, qboolean bStereoDesired /* = qfalse */)
{
	return C_MP3_IsMusic(pvData, iDataLen, bStereoDesired);
}

