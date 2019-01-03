#pragma once

// Filename:-	cl_mp3.h
//
// (Interface to the rest of the game for the MP3 functions)
//

#include "snd_local.h"

typedef struct id3v1_1 {
    char id[3];
    char title[30];		// <file basename>
    char artist[30];	// "Raven Software"
    char album[30];		// "#UNCOMP %d"		// needed
    char year[4];		// "2000"
    char comment[28];	// "#MAXVOL %g"		// needed
    char zero;
    char track;
    char genre;
} id3v1_1;	// 128 bytes in size

extern const char sKEY_MAXVOL[];
extern const char sKEY_UNCOMP[];

qboolean	MP3_IsMusic				( const char *psLocalFilename, void *pvData, int iDataLen, qboolean bStereoDesired = qfalse );

///////////////////////////////////////
//
// the real worker code deep down in the MP3 C code...  (now externalised here so the music streamer can access one)
//
#ifdef __cplusplus
extern "C"
{
#endif

qboolean	C_MP3_IsMusic			(void *pvData, int iDataLen, int bStereoDesired);

#ifdef __cplusplus
}
#endif
//
///////////////////////////////////////


///////////////// eof /////////////////////

