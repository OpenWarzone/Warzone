#pragma once

// snd_local.h -- private sound definations

#include "snd_public.h"

#include "mp3code/mp3struct.h"

#define		MAX_SOUNDPATH	512
#define		MAX_SFX			14000	//512 * 2

#define ForceCrash() { refdef_t *blah = NULL; blah->time = 1; }

extern void S_FreeOldSamples(void);

extern qboolean S_ShouldCull ( vec3_t org, qboolean check_angles, int entityNum );
extern void S_TextToSpeech( const char *text, const char *voice, int entityNum, float *origin );
extern qboolean S_DownloadVoice( const char *text, const char *voice );

extern qboolean BASS_Initialize ( void );
extern void BASS_Shutdown ( void );
extern void BASS_UnloadSamples ( void );
extern void BASS_FreeSampleMemory(DWORD sample);
extern qboolean BASS_SampleIsPlaying(DWORD sample);
extern DWORD BASS_LoadMemorySample ( void *memory, int length );
extern DWORD BASS_LoadMusicSample ( void *memory, int length );
extern void BASS_AddDynamicTrack ( char *name );
extern void BASS_InitDynamicList ( void );
extern void BASS_UpdateDynamicMusic( void );
extern void BASS_AddMemoryChannel ( DWORD samplechan, int entityNum, int entityChannel, vec3_t origin, float volume, char *filename );
extern void BASS_AddMemoryLoopChannel ( DWORD samplechan, int entityNum, int entityChannel, vec3_t origin, float volume, char *filename);
extern void BASS_StartStreamingSound ( char *filename, int entityNum, int entityChannel, vec3_t origin );
extern void BASS_StopLoopChannel ( int entityNum );
extern void BASS_StopAllChannels ( void );
extern void BASS_StopAllLoopChannels ( void );
extern void BASS_StopEntityChannel ( int entityNum, int entchannel );
extern void BASS_FindAndStopSound ( DWORD handle );
extern void BASS_StartMusic ( DWORD samplechan );
extern void BASS_UpdateSounds ( void );
extern void BASS_StopMusic( DWORD samplechan );

extern void BASS_SetEAX_NORMAL ( void );
extern void BASS_SetEAX_FOREST(void);
extern void BASS_SetEAX_MOUNTAINS(void);
extern void BASS_SetEAX_CAVE(void);
extern void BASS_SetEAX_UNDERWATER(void);

// Added for Open AL to know when to mute all sounds (e.g when app. loses focus)
void S_AL_MuteAllSounds(qboolean bMute);


//from SND_AMBIENT
extern void AS_Init( void );
extern void AS_Free( void );


#define	PAINTBUFFER_SIZE	1024


// !!! if this is changed, the asm code must change !!!
typedef struct portable_samplepair_s {
	int			left;	// the final values will be clamped to +/- 0x00ffff00 and shifted down
	int			right;
} portable_samplepair_t;

typedef struct sfx_s {
	qboolean		bDefaultSound;			// couldn't be loaded, so use buzz
	qboolean		bInMemory;				// not in Memory, set qtrue when loaded, and qfalse when its buffers are freed up because of being old, so can be reloaded
	char 			sSoundName[MAX_SOUNDPATH];
	int				iLastTimeUsed;
	float			fVolRange;				// used to set the highest volume this sample has at load time - used for lipsynching
	int				iLastLevelUsedOn;		// used for cacheing purposes

	int			qhandle;
	DWORD		bassSampleID;
	int			indexSize;
	byte		*indexData;

	struct sfx_s	*next;					// only used because of hash table when registering
} sfx_t;

typedef struct dma_s {
	int			channels;
	int			samples;				// mono samples in buffer
	int			submission_chunk;		// don't mix less than this #
	int			samplebits;
	int			speed;
	byte		*buffer;
} dma_t;


#define START_SAMPLE_IMMEDIATE	0x7fffffff

#define NUM_STREAMING_BUFFERS	4
#define STREAMING_BUFFER_SIZE	4608		// 4 decoded MP3 frames

#define QUEUED		1
#define UNQUEUED	2


typedef struct channel_s {
// back-indented fields new in TA codebase, will re-format when MP3 code finished -ste
// note: field missing in TA: qboolean	loopSound;		// from an S_AddLoopSound call, cleared each frame
//
	unsigned int startSample;	// START_SAMPLE_IMMEDIATE = set immediately on next mix
	int			entnum;			// to allow overriding a specific sound
	int			entchannel;		// to allow overriding a specific sound
	int			volumechannel;	// to allow overriding a specific sound
	int			leftvol;		// 0-255 volume after spatialization
	int			rightvol;		// 0-255 volume after spatialization
	int			master_vol;		// 0-255 volume before spatialization


	vec3_t		origin;			// only use if fixed_origin is set

	qboolean	fixed_origin;	// use origin instead of fetching entnum's origin
	sfx_t		*thesfx;		// sfx structure
	qboolean	loopSound;		// from an S_AddLoopSound call, cleared each frame
	//
	MP3STREAM	MP3StreamHeader;
	byte		MP3SlidingDecodeBuffer[50000/*12000*/];	// typical back-request = -3072, so roughly double is 6000 (safety), then doubled again so the 6K pos is in the middle of the buffer)
	int			iMP3SlidingDecodeWritePos;
	int			iMP3SlidingDecodeWindowPos;

	qboolean	doppler;
	float		dopplerScale;

	// Open AL specific
	bool	bLooping;	// Signifies if this channel / source is playing a looping sound
//	bool	bAmbient;	// Signifies if this channel / source is playing a looping ambient sound
	bool	bProcessed;	// Signifies if this channel / source has been processed
	bool	bStreaming;	// Set to true if the data needs to be streamed (MP3 or dialogue)
	bool		bPlaying;		// Set to true when a sound is playing on this channel / source
	int			iStartTime;		// Time playback of Source begins
	int			lSlotID;		// ID of Slot rendering Source's environment (enables a send to this FXSlot)
} channel_t;


#define	WAV_FORMAT_PCM		1
#define WAV_FORMAT_ADPCM	2	// not actually implemented, but is the value that you get in a header
#define WAV_FORMAT_MP3		3	// not actually used this way, but just ensures we don't match one of the legit formats


typedef struct wavinfo_s {
	int			format;
	int			rate;
	int			width;
	int			channels;
	int			samples;
	int			dataofs;		// chunk starts this many bytes from file start
} wavinfo_t;



/*
====================================================================

  SYSTEM SPECIFIC FUNCTIONS

====================================================================
*/

//====================================================================

#define	MAX_CHANNELS			256

extern	channel_t   s_channels[MAX_CHANNELS];

extern	vec3_t	listener_origin;

extern cvar_t	*s_disable;
extern cvar_t	*s_musicSelection;
extern cvar_t	*s_volume;
extern cvar_t	*s_volumeAmbient;
extern cvar_t	*s_volumeAmbientEfx;
extern cvar_t	*s_volumeVoice;
extern cvar_t	*s_volumeEffects;
extern cvar_t	*s_volumeWeapon;
extern cvar_t	*s_volumeSaber;
extern cvar_t	*s_volumeItem;
extern cvar_t	*s_volumeBody;
extern cvar_t	*s_volumeMusic;
extern cvar_t	*s_volumeLocal;

extern cvar_t		*s_testvalue0;
extern cvar_t		*s_testvalue1;
extern cvar_t		*s_testvalue2;
extern cvar_t		*s_testvalue3;
extern cvar_t		*s_testvalue4;
extern cvar_t		*s_testvalue5;
extern cvar_t		*s_testvalue6;
extern cvar_t		*s_testvalue7;

extern cvar_t	*s_nosound;
extern cvar_t	*s_khz;
extern cvar_t	*s_allowDynamicMusic;
extern cvar_t	*s_show;
extern cvar_t	*s_mixahead;
extern cvar_t	*s_realism;

extern cvar_t	*s_testsound;
extern cvar_t	*s_separation;

extern cvar_t	*s_doppler;

extern cvar_t	*s_ttscache;

wavinfo_t GetWavinfo (const char *name, byte *wav, int wavlength);

qboolean S_LoadSound( sfx_t *sfx );


void S_PaintChannels(int endtime);

// picks a channel based on priorities, empty slots, number of channels
channel_t *S_PickChannel(int entnum, int entchannel);

// spatializes a channel
void S_Spatialize(channel_t *ch);


//////////////////////////////////
//
// new stuff from TA codebase

byte	*SND_malloc(int iSize, sfx_t *sfx);
void	 SND_setup();
void	 SND_TouchSFX(sfx_t *sfx);

qboolean SND_RegisterAudio_LevelLoadEnd(qboolean bDeleteEverythingNotUsedThisLevel /* 99% qfalse */);

void S_memoryLoad(sfx_t *sfx);
//
//////////////////////////////////

