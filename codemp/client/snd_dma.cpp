/*****************************************************************************
 * name:		snd_dma.c
 *
 * desc:		main control for any streaming sound output device
 *
 *
 *****************************************************************************/
#include "snd_local.h"

#include "snd_mp3.h"
#include "snd_music.h"
#include "client.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

DWORD CURRENT_MUSIC_HANDLE = 0;

qboolean s_shutUp = qfalse;

static void S_Play_f(void);
static void S_Music_f(void);
static void S_SetDynamicMusic_f(void);

void S_StopAllSounds(void);
static void S_UpdateBackgroundTrack( void );
sfx_t *S_FindName( const char *name );
static int SND_FreeSFXMem(sfx_t *sfx);

extern qboolean Sys_LowPhysicalMemory();

#define fDYNAMIC_XFADE_SECONDS (1.0f)

//////////////////////////


// =======================================================================
// Internal sound data & structures
// =======================================================================

// only begin attenuating sound volumes when outside the FULLVOLUME range
#define		SOUND_FULLVOLUME	256

#define		SOUND_ATTENUATE		0.0008f
#define		VOICE_ATTENUATE		0.004f

//const float	SOUND_FMAXVOL=0.75;//1.0;
const float	SOUND_FMAXVOL=1.0; // UQ1: Why was this lowered???
const int	SOUND_MAXVOL=255;

channel_t   s_channels[MAX_CHANNELS];

int			s_soundStarted;
qboolean	s_soundMuted;

dma_t		dma;

int			listener_number;
vec3_t		listener_origin;
matrix3_t	listener_axis;

// MAX_SFX may be larger than MAX_SOUNDS because
// of custom player sounds
sfx_t		s_knownSfx[MAX_SFX];
int			s_numSfx;

#define		LOOP_HASH		128
static	sfx_t		*sfxHash[LOOP_HASH];

cvar_t		*s_disable;
cvar_t		*s_musicSelection;
cvar_t		*s_volume;
cvar_t		*s_volumeVoice;
cvar_t		*s_volumeEffects;
cvar_t		*s_volumeAmbient;
cvar_t		*s_volumeAmbientEfx;
cvar_t		*s_volumeWeapon;
cvar_t		*s_volumeSaber;
cvar_t		*s_volumeItem;
cvar_t		*s_volumeBody;
cvar_t		*s_volumeMusic;
cvar_t		*s_volumeLocal;
cvar_t		*s_testsound;
cvar_t		*s_testvalue0;
cvar_t		*s_testvalue1;
cvar_t		*s_testvalue2;
cvar_t		*s_testvalue3;
cvar_t		*s_testvalue4;
cvar_t		*s_testvalue5;
cvar_t		*s_testvalue6;
cvar_t		*s_testvalue7;
cvar_t		*s_khz;
cvar_t		*s_allowDynamicMusic;
cvar_t		*s_show;
cvar_t		*s_mixahead;
cvar_t		*s_mixPreStep;
cvar_t		*s_separation;
cvar_t		*s_lip_threshold_1;
cvar_t		*s_lip_threshold_2;
cvar_t		*s_lip_threshold_3;
cvar_t		*s_lip_threshold_4;
cvar_t		*s_language;	// note that this is distinct from "g_language"
cvar_t		*s_debugdynamic;
cvar_t		*s_realism;

cvar_t		*s_doppler;

cvar_t		*s_ttscache;

typedef struct
{
	unsigned char	volume;
	vec3_t			origin;
	vec3_t			velocity;
	sfx_t		*sfx;
	int				mergeFrame;
	int			entnum;

	qboolean	doppler;
	float		dopplerScale;

	// For Open AL
	bool	bProcessed;
	bool	bRelative;
} loopSound_t;

vec3_t		s_entityPosition[MAX_GENTITIES];
int			s_entityWavVol[MAX_GENTITIES];
int			s_entityWavVol_back[MAX_GENTITIES];

bool		s_bInWater;				// Underwater effect currently active


// instead of clearing a whole channel_t struct, we're going to skip the MP3SlidingDecodeBuffer[] buffer in the middle...
//
#ifndef offsetof
#include <stddef.h>
#endif

void S_SoundInfo_f(void) {
	Com_Printf("^7----- ^7Sound Info^5 -----\n" );

	if (!s_soundStarted) {
		Com_Printf ("sound system not started\n");
	} else {
		if ( s_soundMuted ) {
			Com_Printf ("sound system is muted\n");
		}
	}
	Com_Printf("^5----------------------\n" );
}



/*
================
S_Init
================
*/
void S_Init( void ) {
	cvar_t	*cv;

	Com_Printf("\n------- sound initialization -------\n");

	s_disable = Cvar_Get ("s_disable", "0", CVAR_ARCHIVE);
	s_testvalue0 = Cvar_Get("s_testvalue0", "0", CVAR_ARCHIVE);
	s_testvalue1 = Cvar_Get("s_testvalue1", "0", CVAR_ARCHIVE);
	s_testvalue2 = Cvar_Get("s_testvalue2", "0", CVAR_ARCHIVE);
	s_testvalue3 = Cvar_Get("s_testvalue3", "0", CVAR_ARCHIVE);
	s_testvalue4 = Cvar_Get("s_testvalue4", "0", CVAR_ARCHIVE);
	s_testvalue5 = Cvar_Get("s_testvalue5", "0", CVAR_ARCHIVE);
	s_testvalue6 = Cvar_Get("s_testvalue6", "0", CVAR_ARCHIVE);
	s_testvalue7 = Cvar_Get("s_testvalue7", "0", CVAR_ARCHIVE);
	s_musicSelection = Cvar_Get ("s_musicSelection", "2", CVAR_ARCHIVE);
	s_volume = Cvar_Get ("s_volume", "1.0", CVAR_ARCHIVE);
	s_volumeVoice= Cvar_Get ("s_volumeVoice", "1.0", CVAR_ARCHIVE);
	s_volumeEffects= Cvar_Get ("s_volumeEffects", "0.7", CVAR_ARCHIVE);
	s_volumeAmbient= Cvar_Get ("s_volumeAmbient", "0.5", CVAR_ARCHIVE);
	s_volumeAmbientEfx = Cvar_Get("s_volumeAmbientEfx", "0.7", CVAR_ARCHIVE);
	s_volumeWeapon = Cvar_Get ("s_volumeWeapon", "1.0", CVAR_ARCHIVE);
	s_volumeSaber = Cvar_Get("s_volumeSaber", "1.0", CVAR_ARCHIVE);
	s_volumeItem= Cvar_Get ("s_volumeItem", "0.5", CVAR_ARCHIVE);
	s_volumeBody= Cvar_Get ("s_volumeBody", "0.5", CVAR_ARCHIVE);
	s_volumeMusic = Cvar_Get ("s_volumeMusic", "0.25", CVAR_ARCHIVE);
	s_volumeLocal = Cvar_Get ("s_volumeLocal", "0.5", CVAR_ARCHIVE);
	s_separation = Cvar_Get ("s_separation", "0.5", CVAR_ARCHIVE);
	s_khz = Cvar_Get ("s_khz", "44", CVAR_ARCHIVE|CVAR_LATCH);
	s_allowDynamicMusic = Cvar_Get ("s_allowDynamicMusic", "1", CVAR_ARCHIVE);
	s_mixahead = Cvar_Get ("s_mixahead", "0.2", CVAR_ARCHIVE);
	s_realism = Cvar_Get ("s_realism", "1", CVAR_ARCHIVE);

	s_mixPreStep = Cvar_Get ("s_mixPreStep", "0.05", CVAR_ARCHIVE);
	s_show = Cvar_Get ("s_show", "0", CVAR_CHEAT);
	s_testsound = Cvar_Get ("s_testsound", "0", CVAR_CHEAT);
	s_debugdynamic = Cvar_Get("s_debugdynamic","0", CVAR_CHEAT);
	s_lip_threshold_1 = Cvar_Get("s_threshold1" , "0.5",0);
	s_lip_threshold_2 = Cvar_Get("s_threshold2" , "4.0",0);
	s_lip_threshold_3 = Cvar_Get("s_threshold3" , "7.0",0);
	s_lip_threshold_4 = Cvar_Get("s_threshold4" , "8.0",0);

	s_language = Cvar_Get("s_language","english",CVAR_ARCHIVE | CVAR_NORESTART);

	s_doppler = Cvar_Get("s_doppler", "1", CVAR_ARCHIVE);

	s_ttscache = Cvar_Get("s_ttscache", "1", CVAR_ARCHIVE);

	cv = Cvar_Get ("s_initsound", "1", 0);
	if ( !cv->integer ) {
		s_soundStarted = 0;	// needed in case you set s_initsound to 0 midgame then snd_restart (div0 err otherwise later)
		Com_Printf ("not initializing.\n");
		Com_Printf("^5------------------------------------\n");
		return;
	}

	Cmd_AddCommand("play", S_Play_f);
	Cmd_AddCommand("music", S_Music_f);
	Cmd_AddCommand("soundinfo", S_SoundInfo_f);
	Cmd_AddCommand("soundstop", S_StopAllSounds);

	BASS_Initialize();

	Com_Printf("^5------------------------------------\n");

	Com_Printf("\n^5--- ^7ambient sound initialization^5 ---\n");

	AS_Init();
}

// only called from snd_restart. QA request...
//
void S_memoryFree(sfx_t *sfx)
{
	BASS_FreeSampleMemory(sfx->bassSampleID);
	sfx->bInMemory = qfalse;
}

void S_ReloadAllUsedSounds(void)
{
	{
		// new bit, reload all soundsthat are used on the current level...
		//
		for (int i=1 ; i < s_numSfx ; i++)	// start @ 1 to skip freeing default sound
		{
			sfx_t *sfx = &s_knownSfx[i];

			if (!sfx->bDefaultSound && sfx->iLastLevelUsedOn == re->RegisterMedia_GetLevel()){
				//S_memoryLoad(sfx);
				S_memoryFree(sfx);
				sfx->bInMemory = qfalse;
			}
		}
	}
}

int S_NEXT_SOUND_FREE_CHECK = 0;

void S_FreeOldSamples(void)
{// Remove any bass sample memory that has not been used in the last 30 seconds...
//#define __SOUND_CLEANUP_DEBUG__ // Enable to debug what gets cleaned...

	// Run cleanup once every 5 seconds...
	if (S_NEXT_SOUND_FREE_CHECK > Com_Milliseconds()) return;

	//S_NEXT_SOUND_FREE_CHECK = Com_Milliseconds() + 5000; // 5 sec between cleanups...
	S_NEXT_SOUND_FREE_CHECK = Com_Milliseconds(); // Instant cleanups...

#ifdef __SOUND_CLEANUP_DEBUG__
	Com_Printf("Sound cleanup at time %i.\n", Com_Milliseconds() / 1000);
#endif //__SOUND_CLEANUP_DEBUG__

	for (int i = 0; i < s_numSfx; i++)
	{
		sfx_t *sfx = &s_knownSfx[i];

		if (sfx)
		{
			if (sfx->bInMemory && !sfx->bDefaultSound)
			{
#ifdef __SOUND_CLEANUP_DEBUG__
				Com_Printf("[%i] Sample: %s. Loaded: %s. LastUse: %i.", i, sfx->sSoundName, sfx->bInMemory ? "true" : "false", sfx->iLastTimeUsed / 1000);
#endif //__SOUND_CLEANUP_DEBUG__

				if (sfx->iLastTimeUsed <= Com_Milliseconds() - 30000)
				{
					if (!BASS_SampleIsPlaying(sfx->bassSampleID) 
						&& !StringContainsWord(sfx->sSoundName, "***default***") 
						&& !StringContainsWord(sfx->sSoundName, "sound/atmospherics") 
						&& !StringContainsWord(sfx->sSoundName, "music/"))
					{// Never free the ambient weather sounds, or music, they take a moment to load again due to their size...
						S_memoryFree(sfx);
#ifdef __SOUND_CLEANUP_DEBUG__
						Com_Printf("- Freeing sample from memory.");
#endif //__SOUND_CLEANUP_DEBUG__
					}
				}

#ifdef __SOUND_CLEANUP_DEBUG__
				Com_Printf("\n");
#endif //__SOUND_CLEANUP_DEBUG__
			}
		}
	}

	//Com_Printf("Sound cleanup completed.\n");
}

// =======================================================================
// Shutdown sound engine
// =======================================================================

void S_Shutdown( void )
{
	if ( !s_soundStarted ) {
		return;
	}

	BASS_Shutdown();
	s_soundStarted = 0;

	Cmd_RemoveCommand("play");
	Cmd_RemoveCommand("music");
	Cmd_RemoveCommand("stopsound");
	Cmd_RemoveCommand("soundinfo");
	Cmd_RemoveCommand("soundstop");
	AS_Free();

#ifdef _WIN32
	ShutdownTextToSpeechThread();
#endif //_WIN32
}


// =======================================================================
// Load a sound
// =======================================================================
/*
================
return a hash value for the sfx name
================
*/
static long S_HashSFXName(const char *name) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (name[i] != '\0') {
		letter = tolower(name[i]);
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash &= (LOOP_HASH-1);
	return hash;
}

/*
==================
S_FindName

Will allocate a new sfx if it isn't found
==================
*/
sfx_t *S_FindName( const char *name ) {
	int		i;
	int		hash;

	sfx_t	*sfx;

	if (!name) {
		Com_Error (ERR_FATAL, "S_FindName: NULL");
	}
	if (!name[0]) {
		Com_Error (ERR_FATAL, "S_FindName: empty name");
	}

	if (strlen(name) >= MAX_SOUNDPATH) {
		Com_Error (ERR_FATAL, "Sound name too long: %s", name);
	}

	char sSoundNameNoExt[MAX_SOUNDPATH];
	COM_StripExtension(name,sSoundNameNoExt, sizeof( sSoundNameNoExt ));
	Q_strlwr(sSoundNameNoExt);//UQ1: force it down low before hashing too?!?!?!?!

	hash = S_HashSFXName(sSoundNameNoExt);

	sfx = sfxHash[hash];


	// see if already loaded
	while (sfx) {
		if (!Q_stricmp(sfx->sSoundName, sSoundNameNoExt) ) {
			return sfx;
		}
		sfx = sfx->next;
	}

/*
	// find a free sfx
	for (i=0 ; i < s_numSfx ; i++) {
		if (!s_knownSfx[i].soundName[0]) {
			break;
		}
	}
*/
	i = s_numSfx;	//we don't clear the soundName after failed loads any more, so it'll always be the last entry

	if (s_numSfx == MAX_SFX)
	{
		// ok, no sfx's free, but are there any with defaultSound set? (which the registering ent will never
		//	see because he gets zero returned if it's default...)
		//
		for (i=0 ; i < s_numSfx ; i++) {
			if (s_knownSfx[i].bDefaultSound) {
				break;
			}
		}

		if (i==s_numSfx)
		{
			// genuinely out of handles...

			// if we ever reach this, let me know and I'll either boost the array or put in a map-used-on
			//	reference to enable sfx_t recycling. TA codebase relies on being able to have structs for every sound
			//	used anywhere, ever, all at once (though audio bit-buffer gets recycled). SOF1 used about 1900 distinct
			//	events, so current MAX_SFX limit should do, or only need a small boost...	-ste
			//

			Com_Error (ERR_FATAL, "S_FindName: out of sfx_t");
		}
	}
	else
	{
		s_numSfx++;
	}

	sfx = &s_knownSfx[i];
	memset (sfx, 0, sizeof(*sfx));
	Q_strncpyz(sfx->sSoundName, sSoundNameNoExt, sizeof(sfx->sSoundName));
	Q_strlwr(sfx->sSoundName);//force it down low

	sfx->next = sfxHash[hash];
	sfxHash[hash] = sfx;

	return sfx;
}

/*
=================
S_DefaultSound
=================
*/
void S_DefaultSound( sfx_t *sfx ) {
	int		i;

	sfx->indexSize	= 512;								// #samples, ie shorts
	sfx->indexData				= (byte *)	Z_Malloc(512*2, TAG_SND_RAWDATA, qfalse);	// ... so *2 for alloc bytes
	sfx->bInMemory				= qtrue;

	for ( i=0 ; i < sfx->indexSize ; i++ )
	{
		sfx->indexData[i] = i;
	}

	sfx->bassSampleID = BASS_LoadMemorySample( sfx->indexData, sfx->indexSize );
	Z_Free(sfx->indexData);
}

/*
===================
S_DisableSounds

Disables sounds until the next S_BeginRegistration.
This is called when the hunk is cleared and the sounds
are no longer valid.
===================
*/
void S_DisableSounds( void ) {
	S_StopAllSounds();
	s_soundMuted = qtrue;
}

/*
=====================
S_BeginRegistration

=====================
*/
void S_BeginRegistration( void )
{
	s_soundMuted = qfalse;		// we can play again

	if (s_numSfx == 0) {
		SND_setup();

		Com_Printf("Registering default sound.\n");

		s_numSfx = 0;
		
		memset( s_knownSfx, 0, sizeof( s_knownSfx ) );
		memset(sfxHash, 0, sizeof(sfx_t *)*LOOP_HASH);

//#ifdef _DEBUG
		sfx_t *sfx = S_FindName( "***DEFAULT***" );
		S_DefaultSound( sfx );
/*#else
		S_RegisterSound("sound/null.wav");
#endif*/
	}
}

/*
==================
S_RegisterSound

Creates a default buzz sound if the file can't be loaded
==================
*/
sfxHandle_t	S_RegisterSound( const char *name)
{
	sfx_t	*sfx;

	if (!s_soundStarted) {
		return 0;
	}

	if ( strlen( name ) >= MAX_SOUNDPATH ) {
		Com_Printf( S_COLOR_RED"Sound name exceeds MAX_SOUNDPATH - %s\n", name );
		assert(0);
		return 0;
	}

	sfx = S_FindName( name );

	//SND_TouchSFX(sfx);

	if ( sfx->bDefaultSound )
		return 0;

	if ( sfx->bassSampleID )
	{
		return sfx - s_knownSfx;
	}

	sfx->bInMemory = qfalse;

	//S_memoryLoad(sfx);

	if ( sfx->bDefaultSound ) {
#ifndef FINAL_BUILD
		if (!s_shutUp)
		{
			Com_Printf( S_COLOR_YELLOW "WARNING: could not find %s - using default\n", sfx->sSoundName );
		}
#endif
		return 0;
	}

	return sfx - s_knownSfx;
}

void S_memoryLoad(sfx_t	*sfx)
{
	// load the sound file...
	//
	if ( !S_LoadSound( sfx ) )
	{
//		Com_Printf( S_COLOR_YELLOW "WARNING: couldn't load sound: %s\n", sfx->sSoundName );
		sfx->bDefaultSound = qtrue;
	}
	sfx->bInMemory = qtrue;
}

// =======================================================================
// Start a sound effect
// =======================================================================

/*
====================
S_StartAmbientSound

Starts an ambient, 'one-shot" sound.
====================
*/

void S_StartAmbientSound( const vec3_t origin, int entityNum, unsigned char volume, sfxHandle_t sfxHandle )
{
	if (sfxHandle >= MAX_SFX) return;

	if (!s_knownSfx[sfxHandle].bInMemory)
		S_memoryLoad(&s_knownSfx[sfxHandle]);

	if (s_knownSfx[ sfxHandle ].bassSampleID < 0) return;
	/*
	if (origin)
		Com_Printf("BASS_DEBUG: Entity %i playing AMBIENT sound %s on channel %i at org %f %f %f.\n", entityNum, s_knownSfx[ sfxHandle ].sSoundName, CHAN_AMBIENT, origin[0], origin[1], origin[2]);
	else
		Com_Printf("BASS_DEBUG: Entity %i playing AMBIENT sound %s on channel %i at NULL org.\n", entityNum, s_knownSfx[ sfxHandle ].sSoundName, CHAN_AMBIENT);
	*/

	if (S_ShouldCull((float *)origin, qfalse, entityNum))
		BASS_AddMemoryChannel(s_knownSfx[ sfxHandle ].bassSampleID, entityNum, CHAN_AMBIENT, (float *)origin, (float)((float)(volume*0.25)/255.0), s_knownSfx[sfxHandle].sSoundName);
	else
		BASS_AddMemoryChannel(s_knownSfx[ sfxHandle ].bassSampleID, entityNum, CHAN_AMBIENT, (float *)origin, (float)((float)volume/255.0), s_knownSfx[sfxHandle].sSoundName);
	return;
}

/*
====================
S_MuteSound

Mutes sound on specified channel for specified entity.
====================
*/
void S_MuteSound(int entityNum, int entchannel)
{
	BASS_StopEntityChannel( entityNum, entchannel );
}

/*
====================
S_StartSound

Validates the parms and ques the sound up
if pos is NULL, the sound will be dynamically sourced from the entity
entchannel 0 will never override a playing sound
====================
*/
void S_StartSound(const vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfxHandle )
{
	if (sfxHandle >= MAX_SFX) return;

	if (!s_knownSfx[sfxHandle].bInMemory)
		S_memoryLoad(&s_knownSfx[sfxHandle]);

	if (s_knownSfx[ sfxHandle ].bassSampleID < 0) return;

	// Always force sounds in the saber folder to use saber channel... Override for game sending events, etc...
	qboolean forceSaberChannel = (Q_stricmpn("sound/weapons/saber/", s_knownSfx[sfxHandle].sSoundName, 20) && origin != NULL) ? qtrue : qfalse;

	/*
	if (origin)
		Com_Printf("BASS_DEBUG: Entity %i playing sound %s (handle %i - bass id %ld) on channel %i at org %f %f %f.\n", entityNum, s_knownSfx[ sfxHandle ].sSoundName, sfxHandle, s_knownSfx[ sfxHandle ].bassSampleID, entchannel, origin[0], origin[1], origin[2]);
	else
		Com_Printf("BASS_DEBUG: Entity %i playing sound %s (handle %i - bass id %ld) on channel %i at NULL org.\n", entityNum, s_knownSfx[ sfxHandle ].sSoundName, sfxHandle, s_knownSfx[ sfxHandle ].bassSampleID, entchannel);
	*/

	if (!BASS_EntityChannelHasSpecialCullrange(entchannel) && S_ShouldCull((float *)origin, qfalse, entityNum))
		BASS_AddMemoryChannel(s_knownSfx[ sfxHandle ].bassSampleID, entityNum, forceSaberChannel ? CHAN_SABER : entchannel, (float *)origin, 0.25, s_knownSfx[sfxHandle].sSoundName);
	else
		BASS_AddMemoryChannel(s_knownSfx[ sfxHandle ].bassSampleID, entityNum, forceSaberChannel ? CHAN_SABER : entchannel, (float *)origin, 1.0, s_knownSfx[sfxHandle].sSoundName);
	return;
}

/*
==================
S_StartLocalSound
==================
*/
void S_StartLocalSound( sfxHandle_t sfxHandle, int channelNum ) {
	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	if ( sfxHandle < 0 || sfxHandle >= s_numSfx ) {
		Com_Error( ERR_DROP, "S_StartLocalSound: handle %i out of range", sfxHandle );
	}

	S_StartSound (NULL, listener_number, channelNum, sfxHandle );
}


/*
==================
S_StartLocalLoopingSound
==================
*/
void S_StartLocalLoopingSound( sfxHandle_t sfxHandle) {
	vec3_t nullVec = {0,0,0};

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	if ( sfxHandle < 0 || sfxHandle >= s_numSfx ) {
		Com_Error( ERR_DROP, "S_StartLocalLoopingSound: handle %i out of range", sfxHandle );
	}

	S_AddLoopingSound( listener_number, nullVec, nullVec, sfxHandle, CHAN_LOCAL);
}

// kinda kludgy way to stop a special-use sfx_t playing...
//
void S_CIN_StopSound(sfxHandle_t sfxHandle)
{
	if ( sfxHandle < 0 || sfxHandle >= s_numSfx ) {
		Com_Error( ERR_DROP, "S_CIN_StopSound: handle %i out of range", sfxHandle );
	}

	if (sfxHandle >= MAX_SFX) return;
	if (s_knownSfx[ sfxHandle ].bassSampleID < 0) return;

	BASS_FindAndStopSound(s_knownSfx[ sfxHandle ].bassSampleID);
}


/*
==================
S_StopAllSounds
==================
*/
void S_StopSounds(void)
{
	BASS_StopAllChannels();
}

/*
==================
S_StopAllSounds
 and music
==================
*/
void S_StopAllSounds(void) {
	// stop the background music
	S_StopBackgroundTrack();

	S_StopSounds();
}

/*
==============================================================

continuous looping sounds are added each frame

==============================================================
*/

/*
==================
S_ClearLoopingSounds

==================
*/
void S_ClearLoopingSounds( void )
{
	//BASS_StopAllLoopChannels();
}

/*
==================
S_StopLoopingSound

Stops all active looping sounds on a specified entity.
Sort of a slow method though, isn't there some better way?
==================
*/
void S_StopLoopingSound( int entityNum )
{
	BASS_StopLoopChannel(entityNum);
}

/*
==================
S_AddLoopingSound

Called during entity generation for a frame
Include velocity in case I get around to doing doppler...
==================
*/
void S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfxHandle, int entchannel) {
	if (sfxHandle >= MAX_SFX || sfxHandle < 0) return;

	if (!s_knownSfx[sfxHandle].bInMemory) // Hmm guess I should make sure it's loaded before trying to use it :)
		S_memoryLoad(&s_knownSfx[sfxHandle]);

	if (s_knownSfx[ sfxHandle ].bassSampleID < 0) return;
	
	if (entityNum >= 0 /*&& (cl.entityBaselines[entityNum].eType == ET_NPC || cl.entityBaselines[entityNum].eType == ET_PLAYER)*/)
	{
		//if (origin)	if (Distance(cl.snap.ps.origin, origin) > 2048) return;
		
		/*
		if (origin)
			Com_Printf("BASS_DEBUG: Entity %i playing LOOPING sound %s (ID %i) on channel %i at org %f %f %f.\n", entityNum, s_knownSfx[ sfxHandle ].sSoundName, (int)s_knownSfx[sfxHandle].bassSampleID, CHAN_BODY, origin[0], origin[1], origin[2]);
		else
			Com_Printf("BASS_DEBUG: Entity %i playing LOOPING sound %s (ID %i) on channel %i at NULL org.\n", entityNum, s_knownSfx[ sfxHandle ].sSoundName, (int)s_knownSfx[sfxHandle].bassSampleID, CHAN_BODY);
		*/
		
		if (!BASS_EntityChannelHasSpecialCullrange(entchannel) && S_ShouldCull((float *)origin, qfalse, entityNum))
			BASS_AddMemoryLoopChannel(s_knownSfx[ sfxHandle ].bassSampleID, entityNum, entchannel ? entchannel : CHAN_WEAPON, (float *)origin, 0.25, s_knownSfx[sfxHandle].sSoundName);
		else
			BASS_AddMemoryLoopChannel(s_knownSfx[ sfxHandle ].bassSampleID, entityNum, entchannel ? entchannel : CHAN_WEAPON, (float *)origin, 1.0, s_knownSfx[sfxHandle].sSoundName);
	}
	else
	{
		//if (origin)	if (Distance(cl.snap.ps.origin, origin) > 2048) return;
		/*
		if (origin)
			Com_Printf("BASS_DEBUG: Entity %i playing LOOPING sound %s on channel %i at org %f %f %f.\n", entityNum, s_knownSfx[ sfxHandle ].sSoundName, CHAN_AMBIENT, origin[0], origin[1], origin[2]);
		else
			Com_Printf("BASS_DEBUG: Entity %i playing LOOPING sound %s on channel %i at NULL org.\n", entityNum, s_knownSfx[ sfxHandle ].sSoundName, CHAN_AMBIENT);
		*/

		if (!BASS_EntityChannelHasSpecialCullrange(entchannel) && S_ShouldCull((float *)origin, qfalse, entityNum))
			BASS_AddMemoryLoopChannel(s_knownSfx[ sfxHandle ].bassSampleID, entityNum, entchannel ? entchannel : CHAN_AMBIENT, (float *)origin, 0.25, s_knownSfx[sfxHandle].sSoundName);
		else
			BASS_AddMemoryLoopChannel(s_knownSfx[ sfxHandle ].bassSampleID, entityNum, entchannel ? entchannel : CHAN_AMBIENT, (float *)origin, 1.0, s_knownSfx[sfxHandle].sSoundName);
	}
}


/*
==================
S_AddAmbientLoopingSound
==================
*/
void S_AddAmbientLoopingSound( const vec3_t origin, unsigned char volume, sfxHandle_t sfxHandle )
{
	if (sfxHandle >= MAX_SFX) return;
	if (s_knownSfx[ sfxHandle ].bassSampleID < 0) return;

	/*
	if (origin)
		Com_Printf("BASS_DEBUG: Entity %i playing AMBIENT LOOPING sound %s on channel %i at org %f %f %f.\n", -1, s_knownSfx[ sfxHandle ].sSoundName, CHAN_AMBIENT, origin[0], origin[1], origin[2]);
	else
		Com_Printf("BASS_DEBUG: Entity %i playing AMBIENT LOOPING sound %s on channel %i at NULL org.\n", -1, s_knownSfx[ sfxHandle ].sSoundName, CHAN_AMBIENT);
	*/

	if (origin)	if (Distance(cl.snap.ps.origin, origin) > 2048) return;

	if (S_ShouldCull((float *)origin, qfalse, -1))
		BASS_AddMemoryLoopChannel(s_knownSfx[ sfxHandle ].bassSampleID, -1, CHAN_AMBIENT, (float *)origin, (float)((float)(volume*0.25)/255.0), s_knownSfx[sfxHandle].sSoundName);
	else
		BASS_AddMemoryLoopChannel(s_knownSfx[ sfxHandle ].bassSampleID, -1, CHAN_AMBIENT, (float *)origin, (float)((float)volume/255.0), s_knownSfx[sfxHandle].sSoundName);
}



//=============================================================================

/*
=====================
S_UpdateEntityPosition

let the sound system know where an entity currently is
======================
*/
void S_UpdateEntityPosition( int entityNum, const vec3_t origin )
{
	if ( entityNum < 0 || entityNum >= MAX_GENTITIES ) {
		Com_Error( ERR_DROP, "S_UpdateEntityPosition: bad entitynum %i", entityNum );
	}

	VectorCopy( origin, s_entityPosition[entityNum] );
}

/*
============
S_Respatialize

Change the volumes of all the playing sounds for changes in their positions
============
*/
void S_Respatialize( int entityNum, const vec3_t head, matrix3_t axis, int inwater )
{
	// Check if the Listener is underwater
	if (inwater)
	{
		// Check if we have already applied Underwater effect
		if (!s_bInWater)
		{
			s_bInWater = true;
			BASS_SetEAX_UNDERWATER();
		}
		else
		{
			if (s_bInWater)
			{
				s_bInWater = false;
				BASS_SetEAX_NORMAL();
			}
		}
	}
	else if (s_bInWater)
	{
		s_bInWater = false;
		BASS_SetEAX_NORMAL();
	}
}

/*
============
S_Update

Called once each time through the main loop
============
*/
void S_Update( void ) 
{
	BASS_UpdateSounds();
	BASS_UpdateDynamicMusic(); // check for any dynamic music updates...
}

/*
===============================================================================

console functions

===============================================================================
*/

static void S_Play_f( void ) {
	int 	i;
	sfxHandle_t	h;
	char name[256];

	i = 1;
	while ( i<Cmd_Argc() ) {
		if ( !strrchr(Cmd_Argv(i), '.') ) {
			Com_sprintf( name, sizeof(name), "%s.wav", Cmd_Argv(1) );
		} else {
			Q_strncpyz( name, Cmd_Argv(i), sizeof(name) );
		}
		h = S_RegisterSound( name );
		if( h ) {
			S_StartLocalSound( h, CHAN_LOCAL_SOUND );
		}
		i++;
	}
}

static void S_Music_f( void ) {
	int		c;

	c = Cmd_Argc();

	if ( c == 2 ) {
		S_StartBackgroundTrack( Cmd_Argv(1), Cmd_Argv(1), qfalse );
	} else if ( c == 3 ) {
		S_StartBackgroundTrack( Cmd_Argv(1), Cmd_Argv(2), qfalse );
	} else {
		Com_Printf ("music <musicfile> [loopfile]\n");
		return;
	}
}

/*
===============================================================================

background music functions

===============================================================================
*/

// fixme: need to move this into qcommon sometime?, but too much stuff altered by other people and I won't be able
//	to compile again for ages if I check that out...
//
// DO NOT replace this with a call to FS_FileExists, that's for checking about writing out, and doesn't work for this.
//
qboolean S_FileExists( const char *psFilename )
{
#if 0
	fileHandle_t fhTemp;

	FS_FOpenFileRead (psFilename, &fhTemp, qtrue);	// qtrue so I can fclose the handle without closing a PAK
	if (!fhTemp)
		return qfalse;

	FS_FCloseFile(fhTemp);
	return qtrue;
#else
	return FS_FileExists(psFilename);
#endif
}


static void S_StopBackgroundTrack_Actual( void )
{
	if (CURRENT_MUSIC_HANDLE)
	{// Need to free the old sound!
		BASS_StopMusic(CURRENT_MUSIC_HANDLE);
	}
}

void S_StopBackgroundTrack( void )
{
	S_StopBackgroundTrack_Actual();
}

extern DWORD S_LoadMusic( char *sSoundName );
extern qboolean BASS_MUSIC_UPDATE_THREAD_STOP;
extern qboolean BASS_UPDATE_THREAD_STOP;

qboolean S_StartBackgroundTrack_Actual( const char *intro, const char *loop )
{
	DWORD	NEW_MUSIC_HANDLE = 0;
	char	name[MAX_SOUNDPATH];

	if (BASS_MUSIC_UPDATE_THREAD_STOP || BASS_UPDATE_THREAD_STOP) return qfalse;

	Q_strncpyz( name, intro, sizeof( name ) - 4 );
	COM_DefaultExtension( name, sizeof( name ), ".mp3" );

	NEW_MUSIC_HANDLE = S_LoadMusic( name );

	if (NEW_MUSIC_HANDLE && !BASS_UPDATE_THREAD_STOP && !BASS_MUSIC_UPDATE_THREAD_STOP)
	{
		// Stop old track...
		S_StopBackgroundTrack_Actual();

		// Start new track...
		CURRENT_MUSIC_HANDLE = NEW_MUSIC_HANDLE;
		BASS_StartMusic(CURRENT_MUSIC_HANDLE);
	}
	else
	{
		return qfalse;
	}

	return qtrue;
}

static char gsIntroMusic[MAX_SOUNDPATH]={0};
static char gsLoopMusic [MAX_SOUNDPATH]={0};

void S_RestartMusic( void )
{
	S_StartBackgroundTrack( gsIntroMusic, gsLoopMusic, qfalse );	// ( default music start will set the state to EXPLORE )
}

// Basic logic here is to see if the intro file specified actually exists, and if so, then it's not dynamic music,
//	When called by the cgame start it loads up, then stops the playback (because of stutter issues), so that when the
//	actual snapshot is received and the real play request is processed the data has already been loaded so will be quicker.
//
// to be honest, although the code still plays WAVs some of the file-check logic only works for MP3s, so if you ever want
//	to use WAV music you'll have to do some tweaking below (but I've got other things to do so it'll have to wait - Ste)
//
void S_StartBackgroundTrack( const char *intro, const char *loop, qboolean bCalledByCGameStart )
{
	if (!s_soundStarted)
	{	//we have no sound, so don't even bother trying
		return;
	}

	if ( !intro ) {
		intro = "";
	}
	if ( !loop || !loop[0] ) {
		loop = intro;
	}

	char sNameIntro[MAX_SOUNDPATH];
	char sNameLoop [MAX_SOUNDPATH];
	Q_strncpyz(sNameIntro,	intro,	sizeof(sNameIntro));
	Q_strncpyz(sNameLoop,	loop,	sizeof(sNameLoop));

	COM_DefaultExtension( sNameIntro, sizeof( sNameIntro ), ".mp3" );
	COM_DefaultExtension( sNameLoop,  sizeof( sNameLoop),	".mp3" );

	if (s_allowDynamicMusic->integer)
	{// Check dynamic music loading...
		BASS_AddDynamicTrack(sNameIntro);

		if (strcmp(sNameLoop, sNameIntro)) 
			BASS_AddDynamicTrack(sNameLoop);

		BASS_UpdateDynamicMusic();

		return;
	}

	// if dynamic music not allowed, then just stream the explore music instead of playing dynamic...
	//
	if (Music_DynamicDataAvailable(intro))	// "intro", NOT "sName" (i.e. don't use version with ".mp3" extension)
	{
		const char *psMusicName = Music_GetFileNameForState( eBGRNDTRACK_DATABEGIN );
		if (psMusicName && S_FileExists( psMusicName ))
		{
			Q_strncpyz(sNameIntro,psMusicName,sizeof(sNameIntro));
			Q_strncpyz(sNameLoop, psMusicName,sizeof(sNameLoop ));
		}
	}

	// conceptually we always play the 'intro'[/sName] track, intro-to-loop transition is handled in UpdateBackGroundTrack().
	//
	if ( (strstr(sNameIntro,"/") && S_FileExists( sNameIntro )) )	// strstr() check avoids extra file-exists check at runtime if reverting from streamed music to dynamic since literal files all need at least one slash in their name (eg "music/blah")
	{
		const char *psLoopName = S_FileExists( sNameLoop ) ? sNameLoop : sNameIntro;
		Com_DPrintf("S_StartBackgroundTrack: Found/using non-dynamic music track '%s' (loop: '%s')\n", sNameIntro, psLoopName);
		S_StartBackgroundTrack_Actual( sNameIntro, psLoopName );
	}

	/*
	if (bCalledByCGameStart)
	{
		S_StopBackgroundTrack();
	}
	*/
}

cvar_t *s_soundpoolmegs = NULL;

// currently passing in sfx as a param in case I want to do something with it later.
//
byte *SND_malloc(int iSize, sfx_t *sfx)
{
	byte *pData = (byte *) Z_Malloc(iSize, TAG_SND_RAWDATA, qfalse);	// don't bother asking for zeroed mem
	return pData;
}

// called once-only in EXE lifetime...
//
void SND_setup()
{
	s_soundpoolmegs = Cvar_Get("s_soundpoolmegs", "25", CVAR_ARCHIVE);
	if (Sys_LowPhysicalMemory() )
	{
		Cvar_Set("s_soundpoolmegs", "0");
	}

	Com_Printf("Sound memory manager started\n");
}

// ask how much mem an sfx has allocated...
//
static int SND_MemUsed(sfx_t *sfx)
{
	int iSize = 0;
	if (sfx->indexSize){
		iSize += sfx->indexSize;
	}

	return iSize;
}

void SND_TouchSFX(sfx_t *sfx)
{
	sfx->iLastTimeUsed		= Com_Milliseconds()+1;
	sfx->iLastLevelUsedOn	= re->RegisterMedia_GetLevel();
}

qboolean SND_RegisterAudio_LevelLoadEnd(qboolean bDeleteEverythingNotUsedThisLevel /* 99% qfalse */)
{
	return qfalse;
}