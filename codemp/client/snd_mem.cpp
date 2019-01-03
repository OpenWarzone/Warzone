// snd_mem.c: sound caching

#include "snd_local.h"

#include "snd_mp3.h"
#include "snd_ambient.h"

#include <string>

#include "../client/fast_mutex.h"
#include "../client/tinythread.h"

/*
===============================================================================

WAV loading

===============================================================================
*/

byte	*data_p;
byte 	*iff_end;
byte 	*last_chunk;
byte 	*iff_data;
int 	iff_chunk_len;
extern sfx_t		s_knownSfx[];
extern	int			s_numSfx;

extern cvar_t		*s_lip_threshold_1;
extern cvar_t		*s_lip_threshold_2;
extern cvar_t		*s_lip_threshold_3;
extern cvar_t		*s_lip_threshold_4;

short GetLittleShort(void)
{
	short val = 0;
	val = *data_p;
	val = (short)(val + (*(data_p+1)<<8));
	data_p += 2;
	return val;
}

int GetLittleLong(void)
{
	int val = 0;
	val = *data_p;
	val = val + (*(data_p+1)<<8);
	val = val + (*(data_p+2)<<16);
	val = val + (*(data_p+3)<<24);
	data_p += 4;
	return val;
}

//=============================================================================

// adjust filename for foreign languages and WAV/MP3/OGG issues.
//
// returns qfalse if failed to load, else fills in *pData
//
extern	cvar_t	*com_buildScript;

tthread::fast_mutex loadsound_lock; // UQ1: Since sound loading is always the problem, let's lock this f*ker down!!!

static qboolean S_LoadSound_FileLoadAndNameAdjuster(char *psFilename, byte **pData, int *piSize, int iNameStrlen)
{
	char *psVoice = strstr(psFilename,"chars");

	loadsound_lock.lock();

	if (psVoice)
	{
		// cache foreign voices...
		//
		if (com_buildScript->integer)
		{
			fileHandle_t hFile;
			//German
			strncpy(psVoice,"chr_d",5);	// same number of letters as "chars"
			FS_FOpenFileRead(psFilename, &hFile, qfalse);		//cache the wav
			if (!hFile)
			{
				strcpy(&psFilename[iNameStrlen-3],"mp3");		//not there try mp3
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the mp3
			}
			if (!hFile)
			{
				strcpy(&psFilename[iNameStrlen-3],"ogg");		//not there try ogg
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the ogg
			}
			if (hFile)
			{
				FS_FCloseFile(hFile);
			}
			strcpy(&psFilename[iNameStrlen-3],"wav");	//put it back to wav

			//French
			strncpy(psVoice,"chr_f",5);	// same number of letters as "chars"
			FS_FOpenFileRead(psFilename, &hFile, qfalse);		//cache the wav
			if (!hFile)
			{
				strcpy(&psFilename[iNameStrlen-3],"mp3");		//not there try mp3
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the mp3
			}
			if (!hFile)
			{
				strcpy(&psFilename[iNameStrlen-3],"ogg");		//not there try ogg
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the ogg
			}
			if (hFile)
			{
				FS_FCloseFile(hFile);
			}
			strcpy(&psFilename[iNameStrlen-3],"wav");	//put it back to wav

			//Spanish
			strncpy(psVoice,"chr_e",5);	// same number of letters as "chars"
			FS_FOpenFileRead(psFilename, &hFile, qfalse);		//cache the wav
			if (!hFile)
			{
				strcpy(&psFilename[iNameStrlen-3],"mp3");		//not there try mp3
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the mp3
			}
			if (!hFile)
			{
				strcpy(&psFilename[iNameStrlen-3],"ogg");		//not there try ogg
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the ogg
			}
			if (hFile)
			{
				FS_FCloseFile(hFile);
			}
			strcpy(&psFilename[iNameStrlen-3],"wav");	//put it back to wav

			strncpy(psVoice,"chars",5);	//put it back to chars
		}

		// account for foreign voices...
		//
		extern cvar_t* s_language;
		if ( s_language ) {
				 if ( !Q_stricmp( "DEUTSCH", s_language->string ) )
				strncpy( psVoice, "chr_d", 5 );	// same number of letters as "chars"
			else if ( !Q_stricmp( "FRANCAIS", s_language->string ) )
				strncpy( psVoice, "chr_f", 5 );	// same number of letters as "chars"
			else if ( !Q_stricmp( "ESPANOL", s_language->string ) )
				strncpy( psVoice, "chr_e", 5 );	// same number of letters as "chars"
			else
				psVoice = NULL;
		}
		else
		{
			psVoice = NULL;	// use this ptr as a flag as to whether or not we substituted with a foreign version
		}
	}

	//
	// WAV Support...
	//
	*piSize = FS_ReadFile( psFilename, (void **)pData );	// try WAV
	
	//
	// MP3 Support...
	//
	if ( !*pData ) {
		psFilename[iNameStrlen-3] = 'm';
		psFilename[iNameStrlen-2] = 'p';
		psFilename[iNameStrlen-1] = '3';
		*piSize = FS_ReadFile( psFilename, (void **)pData );	// try MP3

		if ( !*pData )
		{
			//hmmm, not found, ok, maybe we were trying a foreign noise ("arghhhhh.mp3" that doesn't matter?) but it
			// was missing?   Can't tell really, since both types are now in sound/chars. Oh well, fall back to English for now...

			if (psVoice)	// were we trying to load foreign?
			{
				// yep, so fallback to re-try the english...
				//
#ifndef FINAL_BUILD
				Com_Printf(S_COLOR_YELLOW "Foreign file missing: \"%s\"! (using English...)\n",psFilename);
#endif

				strncpy(psVoice,"chars",5);

				psFilename[iNameStrlen-3] = 'w';
				psFilename[iNameStrlen-2] = 'a';
				psFilename[iNameStrlen-1] = 'v';
				*piSize = FS_ReadFile( psFilename, (void **)pData );	// try English WAV
				if ( !*pData )
				{
					psFilename[iNameStrlen-3] = 'm';
					psFilename[iNameStrlen-2] = 'p';
					psFilename[iNameStrlen-1] = '3';
					*piSize = FS_ReadFile( psFilename, (void **)pData );	// try English MP3
				}
			}
		}
	}

	//
	// OGG Support...
	//
	if ( !*pData ) {
		psFilename[iNameStrlen-3] = 'o';
		psFilename[iNameStrlen-2] = 'g';
		psFilename[iNameStrlen-1] = 'g';
		*piSize = FS_ReadFile( psFilename, (void **)pData );	// try OGG

		if ( !*pData )
		{
			//hmmm, not found, ok, maybe we were trying a foreign noise ("arghhhhh.mp3" that doesn't matter?) but it
			// was missing?   Can't tell really, since both types are now in sound/chars. Oh well, fall back to English for now...

			if (psVoice)	// were we trying to load foreign?
			{
				// yep, so fallback to re-try the english...
				//
#ifndef FINAL_BUILD
				Com_Printf(S_COLOR_YELLOW "Foreign file missing: \"%s\"! (using English...)\n",psFilename);
#endif

				strncpy(psVoice,"chars",5);

				psFilename[iNameStrlen-3] = 'o';
				psFilename[iNameStrlen-2] = 'g';
				psFilename[iNameStrlen-1] = 'g';
				*piSize = FS_ReadFile( psFilename, (void **)pData );	// try English OGG
			}
		}
	}

	if (!*pData)
	{
		loadsound_lock.unlock();
		return qfalse;	// sod it, give up...
	}

	loadsound_lock.unlock();
	return qtrue;
}

// returns qtrue if this dir is allowed to keep loaded MP3s, else qfalse if they should be WAV'd instead...
//
// note that this is passed the original, un-language'd name
//
// (I was going to remove this, but on kejim_post I hit an assert because someone had got an ambient sound when the
//	perimter fence goes online that was an MP3, then it tried to get added as looping. Presumably it sounded ok or
//	they'd have noticed, but we therefore need to stop other levels using those. "sound/ambience" I can check for,
//	but doors etc could be anything. Sigh...)
//
#define SOUND_CHARS_DIR "sound/chars/"
#define SOUND_CHARS_DIR_LENGTH 12 // strlen( SOUND_CHARS_DIR )
static qboolean S_LoadSound_DirIsAllowedToKeepMP3s(const char *psFilename)
{
	if ( Q_stricmpn( psFilename, SOUND_CHARS_DIR, SOUND_CHARS_DIR_LENGTH ) == 0 )
		return qtrue;	// found a dir that's allowed to keep MP3s

	return qfalse;
}

DWORD S_LoadMusic( char *sSoundName )
{
	if (s_volumeMusic->value <= 0)
	{// No point...
		return 0;
	}

	DWORD		bassSampleID;
	int			indexSize = 0;
	byte		*indexData = NULL;
	char		*psExt;
	char		sLoadName[MAX_SOUNDPATH];
	int			len = strlen(sSoundName);
	qboolean	isMusic = qfalse;

	if (len<5)
	{
		return 0;
	}

	if (!FS_Initialized())
	{
		return 0;
	}

	// player specific sounds are never directly loaded...
	//
	if ( sSoundName[0] == '*') {
		return 0;
	}

	// make up a local filename to try wav/mp3 substitutes...
	//
	Q_strncpyz(sLoadName, sSoundName, sizeof(sLoadName));
	Q_strlwr( sLoadName );

	//
	// Ensure name has an extension (which it must have, but you never know), and get ptr to it...
	//
	psExt = &sLoadName[strlen(sLoadName)-4];

	if (*psExt != '.')
	{
		//Com_Printf( "WARNING: soundname '%s' does not have 3-letter extension\n",sLoadName);
		COM_DefaultExtension(sLoadName,sizeof(sLoadName),".wav");	// so psExt below is always valid
		psExt = &sLoadName[strlen(sLoadName)-4];
		len = strlen(sLoadName);
	}

	if (!S_LoadSound_FileLoadAndNameAdjuster(sLoadName, &indexData, &indexSize, len))
	{
		return 0;
	}

	// Load whole sound file into ram...
	bassSampleID = BASS_LoadMusicSample( indexData, indexSize );

	if (bassSampleID <= 0) {
		Com_Printf("BASS: Failed to load music %s from memory.\n", sLoadName);
		FS_FreeFile( indexData );
		return 0;
	}

	//Com_Printf("BASS: Registered music [ID %i] %s. Length %i.\n", sfx->qhandle, sLoadName, sfx->indexSize);

	FS_FreeFile( indexData );
	return bassSampleID;
}

/*
==============
S_LoadSound

The filename may be different than sfx->name in the case
of a forced fallback of a player specific sound	(or of a wav/mp3 substitution now -Ste)
==============
*/
qboolean gbInsideLoadSound = qfalse;
static qboolean S_LoadSound_Actual( sfx_t *sfx )
{
	char		*psExt;
	char		sLoadName[MAX_SOUNDPATH];
	int			len = strlen(sfx->sSoundName);
	qboolean	isMusic = qfalse;

	if (len<5)
	{
		return qfalse;
	}

	// player specific sounds are never directly loaded...
	//
	if ( sfx->sSoundName[0] == '*') {
		return qfalse;
	}

	// make up a local filename to try wav/mp3 substitutes...
	//
	Q_strncpyz(sLoadName, sfx->sSoundName, sizeof(sLoadName));
	Q_strlwr( sLoadName );

	//
	// Ensure name has an extension (which it must have, but you never know), and get ptr to it...
	//
	psExt = &sLoadName[strlen(sLoadName)-4];

	if (*psExt != '.')
	{
		//Com_Printf( "WARNING: soundname '%s' does not have 3-letter extension\n",sLoadName);
		COM_DefaultExtension(sLoadName,sizeof(sLoadName),".wav");	// so psExt below is always valid
		psExt = &sLoadName[strlen(sLoadName)-4];
		len = strlen(sLoadName);
	}

	if (!S_LoadSound_FileLoadAndNameAdjuster(sLoadName, &sfx->indexData, &sfx->indexSize, len))
	{
		return qfalse;
	}

	SND_TouchSFX(sfx);

	sfx->bassSampleID = BASS_LoadMemorySample( sfx->indexData, sfx->indexSize );
	sfx->qhandle = sfx - s_knownSfx;

	if (sfx->bassSampleID <= 0) {
		Com_Printf("BASS: Failed to load sound [ID %i] %s from memory.\n", sfx->qhandle, sLoadName);
		sfx->bassSampleID = s_knownSfx[0].bassSampleID; // link to default sound bass ID...
		sfx->bDefaultSound = qtrue;
		FS_FreeFile( sfx->indexData );
		return qfalse;
	}

	//Com_Printf("BASS: Registered sound [ID %i] %s. Length %i.\n", sfx->qhandle, sLoadName, sfx->indexSize);

	FS_FreeFile( sfx->indexData );
	return qtrue;
}


// wrapper function for above so I can guarantee that we don't attempt any audio-dumping during this call because
//	of a z_malloc() fail recovery...
//
qboolean S_LoadSound( sfx_t *sfx )
{
	gbInsideLoadSound = qtrue;	// !!!!!!!!!!!!!

	qboolean bReturn = S_LoadSound_Actual( sfx );

	gbInsideLoadSound = qfalse;	// !!!!!!!!!!!!!

	return bReturn;
}


