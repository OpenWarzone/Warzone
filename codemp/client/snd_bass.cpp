#include "client.h"
#include "snd_local.h"

#define __BASS_STREAM_MUSIC__
//#define __BASS_FX__ // Well that's 2 days I won't be getting back, it only supports streams - fucking pk3s...
//#define __BASS_DEBUG__

#include "snd_bass.h"
#include "dirent.h"
#include "fast_mutex.h"
#include "tinythread.h"

#ifdef __BASS_FX__
#pragma comment(lib, "../../lib/bass_fx.lib")
#include "snd_bass_fx.h"
#endif //__BASS_FX__

using namespace tthread;

extern cvar_t		*s_khz;
extern vec3_t		s_entityPosition[MAX_GENTITIES];

extern cvar_t		*s_testvalue0;
extern cvar_t		*s_testvalue1;
extern cvar_t		*s_testvalue2;
extern cvar_t		*s_testvalue3;
extern cvar_t		*s_testvalue4;
extern cvar_t		*s_testvalue5;
extern cvar_t		*s_testvalue6;
extern cvar_t		*s_testvalue7;

extern int			s_soundStarted;
extern int			s_numSfx;

extern qboolean S_StartBackgroundTrack_Actual( const char *intro, const char *loop );

qboolean BASS_INITIALIZED = qfalse;
qboolean EAX_SUPPORTED = qtrue;

#define MAX_BASS_CHANNELS		256
#define MAX_SAMPLE_CHANNELS		16

#define SOUND_3D_METHOD					BASS_3DMODE_NORMAL //BASS_3DMODE_RELATIVE

float	MIN_SOUND_RANGE					= 256.0;
//#define MIN_SOUND_RANGE s_testvalue0->value
float	MAX_SOUND_RANGE					=	2048.0; // 3072.0

int		SOUND_CONE_INSIDE_ANGLE			=	-1;//120;
int		SOUND_CONE_OUTSIDE_ANGLE		=	-1;//120;
float	SOUND_CONE_OUTSIDE_VOLUME		=	0;//0.9;

// Use meters as distance unit, real world rolloff, real doppler effect
// 1.0 = use meters, 0.9144 = use yards, 0.3048 = use feet.
float	SOUND_DISTANCE_UNIT_SIZE		= 0.3048; // UQ1: It would seem that this is close to the right conversion for Q3 units... unsure though...
//#define SOUND_DISTANCE_UNIT_SIZE s_testvalue1->value
// 0.0 = no rolloff, 1.0 = real world, 2.0 = 2x real.
float	SOUND_REAL_WORLD_FALLOFF		= 1.0;//0.3048;//1.0; //0.3048
//#define SOUND_REAL_WORLD_FALLOFF s_testvalue2->value
// 0.0 = no doppler, 1.0 = real world, 2.0 = 2x real.
float	SOUND_REAL_WORLD_DOPPLER		= 0.1048; //1.0 //0.3048
//#define SOUND_REAL_WORLD_DOPPLER s_testvalue3->value


qboolean BASS_UPDATE_THREAD_RUNNING = qfalse;
qboolean BASS_UPDATE_THREAD_STOP = qfalse;
qboolean BASS_MUSIC_UPDATE_THREAD_RUNNING = qfalse;
qboolean BASS_MUSIC_UPDATE_THREAD_STOP = qfalse;

thread *BASS_UPDATE_THREAD;
thread *BASS_MUSIC_UPDATE_THREAD;

extern qboolean FS_STARTUP_COMPLETE; // needed for multi-threading...

DWORD				BASS_FLOAT_AVAILABLE = 0;

// channel (sample/music) info structure
typedef struct {
	DWORD			channel, originalChannel;			// the channel
	BASS_3DVECTOR	pos, vel, ang;	// position,velocity,angles
	vec3_t			origin;
	int				entityNum;
	int				entityChannel;
	float			volume;
	qboolean		isActive;
	qboolean		startRequest;
	qboolean		isLooping;
	float			cullRange;
#ifdef __BASS_FX__
	HFX				fxReverb;
	HFX				fxPeakEQ;
#endif //__BASS_FX__
} Channel;

Channel		MUSIC_CHANNEL;
Channel		MUSIC_CHANNEL2;

qboolean	SOUND_CHANNELS_INITIALIZED = qfalse;
Channel		SOUND_CHANNELS[MAX_BASS_CHANNELS];

//
// Channel Utils...
//

void BASS_InitChannel ( Channel *c )
{
	c->ang.x = 0;
	c->ang.y = 0;
	c->ang.z = 0;
	c->vel.x = 0;
	c->vel.y = 0;
	c->vel.z = 0;
	c->pos.x = 0;
	c->pos.y = 0;
	c->pos.z = 0;
	c->origin[0] = 0;
	c->origin[1] = 0;
	c->origin[2] = 0;
	c->channel = 0;
	c->entityChannel = 0;
	c->entityNum = 0;
	c->isActive = qfalse;
	c->isLooping = qfalse;
	c->originalChannel = 0;
	c->startRequest = qfalse;
	c->volume = 0;

#ifdef __BASS_FX__
	c->fxReverb = 0;
	c->fxPeakEQ = 0;
#endif //__BASS_FX__
}

void BASS_InitializeChannels ( void )
{
	if (!SOUND_CHANNELS_INITIALIZED)
	{
		for (int c = 0; c < MAX_BASS_CHANNELS; c++) 
		{// Set up this channel...
			BASS_InitChannel(&SOUND_CHANNELS[c]);
		}

		SOUND_CHANNELS_INITIALIZED = qtrue;
	}
}

void BASS_FreeSampleMemory(DWORD sample)
{
	BASS_SampleFree(sample);
	BASS_MusicFree(sample);
	BASS_StreamFree(sample);
}

qboolean BASS_SampleIsPlaying(DWORD sample)
{
	if (BASS_ChannelIsActive(sample) >= BASS_ACTIVE_PLAYING) return qtrue;

	return qfalse;
}

void BASS_StopChannel ( int chanNum )
{
	Channel *c = &SOUND_CHANNELS[chanNum];

	/*if (c->entityChannel == CHAN_SABER || c->entityChannel == CHAN_SABERLOCAL)
	{
		Com_Printf("Channel %i stopping on entityNum %i. entityChannel %i. Looping %s. Active %s.\n", chanNum, c->entityNum, c->entityChannel, c->isLooping ? "TRUE" : "FALSE", c->isActive ? "TRUE" : "FALSE");
	}*/

	qboolean IS_LOCAL_SOUND = qfalse;

	if (c->origin[0] == 0
		&& c->origin[1] == 0
		&& c->origin[2] == 0)
	{// Must be a local sound...
		IS_LOCAL_SOUND = qtrue;
	}
	else if (c->entityChannel == CHAN_AMBIENT || c->entityChannel == CHAN_WEAPONLOCAL || c->entityChannel == CHAN_SABERLOCAL)
	{// Force all ambient sounds to local...
		IS_LOCAL_SOUND = qtrue;
	}

	c->volume = 0.0;

	if (IS_LOCAL_SOUND)
		BASS_ChannelSet3DAttributes(c->channel, BASS_3DMODE_OFF, 1, 1, 1, 0, c->volume);
	else
		BASS_ChannelSet3DAttributes(c->channel, SOUND_3D_METHOD, c->cullRange > 0 ? c->cullRange : MAX_SOUND_RANGE, c->cullRange > 0 ? c->cullRange : MAX_SOUND_RANGE, 0, 0, c->volume);

	BASS_ChannelSetAttribute(c->channel, BASS_ATTRIB_VOL, c->volume);
	//BASS_ChannelFlags(c->channel, 0, BASS_SAMPLE_LOOP); // remove the LOOP flag

	BASS_ChannelStop(c->channel);
	BASS_Apply3D();

#ifdef __BASS_FX__
	if (c->fxReverb)
	{
		BASS_ChannelRemoveFX(c->channel, c->fxReverb);
		c->fxReverb = 0;
	}

	if (c->fxPeakEQ)
	{
		BASS_ChannelRemoveFX(c->channel, c->fxPeakEQ);
		c->fxPeakEQ = 0;
	}
#endif //__BASS_FX__

	if (c->channel != c->originalChannel)
	{// free the copied channel's sample memory...
		BASS_SampleFree(c->channel);
		BASS_MusicFree(c->channel);
		BASS_StreamFree(c->channel);
	}

	c->isActive = qfalse;
	c->startRequest = qfalse;
	c->isLooping = qfalse;

	BASS_InitChannel(c);
}

void BASS_StopEntityChannel ( int entityNum, int entchannel )
{
	//Com_Printf("BASS_StopEntityChannel called for entityNum %i entchannel %i.\n", entityNum, entchannel);

	for (int ch = 0; ch < MAX_BASS_CHANNELS; ch++)
	{
		Channel *c = &SOUND_CHANNELS[ch];

		//if (c->isActive || c->startRequest || BASS_ChannelIsActive(c->channel) == BASS_ACTIVE_PLAYING)
		//{
		//	Com_Printf("BASS_StopEntityChannel: chan %i - entCh %i. entNum %i. active %s. playing %s.\n", ch, c->entityChannel, c->entityNum, c->isActive ? "TRUE" : "FALSE", (BASS_ChannelIsActive(c->channel) == BASS_ACTIVE_PLAYING) ? "TRUE" : "FALSE");
		//}

		if ((c->isActive || c->startRequest || BASS_ChannelIsActive(c->channel) == BASS_ACTIVE_PLAYING) && c->entityNum == entityNum && c->entityChannel == entchannel)
		{
			BASS_StopChannel(ch);
		}
	}
}

void BASS_FindAndStopSound ( DWORD handle )
{
	for (int ch = 0; ch < MAX_BASS_CHANNELS; ch++)
	{
		Channel *c = &SOUND_CHANNELS[ch];

		if (c->originalChannel == handle && c->isActive)
		{
			BASS_StopChannel(ch);
		}
	}
}

void BASS_StopAllChannels ( void )
{
	for (int ch = 0; ch < MAX_BASS_CHANNELS; ch++)
	{
		Channel *c = &SOUND_CHANNELS[ch];

		if (c->isActive)
		{
			BASS_StopChannel(ch);
		}
	}
}

void BASS_StopLoopChannel ( int entityNum )
{
	for (int ch = 0; ch < MAX_BASS_CHANNELS; ch++)
	{
		Channel *c = &SOUND_CHANNELS[ch];

		if (c->entityNum == entityNum && c->isActive && c->isLooping)
		{
			BASS_StopChannel(ch);
		}
	}
}

void BASS_StopAllLoopChannels ( void )
{
	for (int ch = 0; ch < MAX_BASS_CHANNELS; ch++) 
	{
		Channel *c = &SOUND_CHANNELS[ch];

		if (c->isActive && c->isLooping)
		{
			BASS_StopChannel(ch);
		}
	}
}

int BASS_FindFreeChannel ( void )
{
	// Fall back to full lookup when we have started too many sounds for the update threade to catch up...
	for (int ch = 0; ch < MAX_BASS_CHANNELS; ch++) 
	{
		Channel *c = &SOUND_CHANNELS[ch];

		if (!c->isActive && !c->startRequest)
		{
			return ch;
		}
	}

	return -1;
}

void BASS_UnloadSamples ( void )
{
	//if (BASS_ChannelIsActive(MUSIC_CHANNEL.channel) == BASS_ACTIVE_PLAYING)
	{
		BASS_ChannelStop(MUSIC_CHANNEL.channel);
		BASS_SampleFree(MUSIC_CHANNEL.channel);
		BASS_MusicFree(MUSIC_CHANNEL.channel);
		BASS_StreamFree(MUSIC_CHANNEL.channel);
	}

	if (SOUND_CHANNELS_INITIALIZED)
	{
		for (int c = 0; c < MAX_BASS_CHANNELS; c++) 
		{// Free channel...
			BASS_StopChannel(c);
			BASS_SampleFree(SOUND_CHANNELS[c].channel);
			BASS_MusicFree(SOUND_CHANNELS[c].channel);
			BASS_StreamFree(SOUND_CHANNELS[c].channel);
		}

		SOUND_CHANNELS_INITIALIZED = qfalse;
	}

	if (BASS_UPDATE_THREAD_RUNNING && thread::hardware_concurrency() > 1)
	{// More then one CPU core. We need to shut down the update thread...
		BASS_UPDATE_THREAD_STOP = qtrue;

		// Wait for update thread to finish...
		BASS_UPDATE_THREAD->join();
		BASS_UPDATE_THREAD_RUNNING = qfalse;
	}

	if (BASS_MUSIC_UPDATE_THREAD_RUNNING && thread::hardware_concurrency() > 1)
	{// More then one CPU core. We need to shut down the update thread...
		BASS_MUSIC_UPDATE_THREAD_STOP = qtrue;

		// Wait for update thread to finish...
		BASS_MUSIC_UPDATE_THREAD->join();
		BASS_MUSIC_UPDATE_THREAD_RUNNING = qfalse;
	}
}


qboolean			MUSIC_LOADING = qfalse;
HINSTANCE			bass = 0;								// bass handle
char				tempfile[MAX_PATH+1];						// temporary BASS.DLL

void BASS_Shutdown ( void )
{
	BASS_UPDATE_THREAD_STOP = qtrue;
	BASS_MUSIC_UPDATE_THREAD_STOP = qtrue;

	while (MUSIC_LOADING)
	{
		this_thread::sleep_for(chrono::milliseconds(1));
	}

	//if (BASS_ChannelIsActive(MUSIC_CHANNEL.channel) == BASS_ACTIVE_PLAYING)
	{
		BASS_ChannelStop(MUSIC_CHANNEL.channel);
		BASS_SampleFree(MUSIC_CHANNEL.channel);
		BASS_MusicFree(MUSIC_CHANNEL.channel);
		BASS_StreamFree(MUSIC_CHANNEL.channel);
	}

	if (SOUND_CHANNELS_INITIALIZED)
	{
		for (int c = 0; c < MAX_BASS_CHANNELS; c++) 
		{// Free channel...
			BASS_StopChannel(c);
			BASS_SampleFree(SOUND_CHANNELS[c].channel);
			BASS_MusicFree(SOUND_CHANNELS[c].channel);
			BASS_StreamFree(SOUND_CHANNELS[c].channel);
		}

		SOUND_CHANNELS_INITIALIZED = qfalse;
	}

	if (BASS_UPDATE_THREAD_RUNNING && thread::hardware_concurrency() > 1)
	{// More then one CPU core. We need to shut down the update thread...
		BASS_UPDATE_THREAD_STOP = qtrue;

		// Wait for update thread to finish...
		if (BASS_UPDATE_THREAD->joinable())
			BASS_UPDATE_THREAD->join();
		//BASS_UPDATE_THREAD->~thread();
		//BASS_UPDATE_THREAD_RUNNING = qfalse;
	}

	if (BASS_MUSIC_UPDATE_THREAD_RUNNING && thread::hardware_concurrency() > 1)
	{// More then one CPU core. We need to shut down the update thread...
		BASS_MUSIC_UPDATE_THREAD_STOP = qtrue;
	
		// Wait for update thread to finish...
		//if (BASS_MUSIC_UPDATE_THREAD->joinable())
		//	BASS_MUSIC_UPDATE_THREAD->join();
		//BASS_MUSIC_UPDATE_THREAD->~thread();
		//BASS_MUSIC_UPDATE_THREAD_RUNNING = qfalse;
	}

	BASS_Free();

	BASS_INITIALIZED = qfalse;
}

qboolean BASS_CheckSoundDisabled( void )
{
	if (s_disable->integer)
	{
		//if (BASS_ChannelIsActive(MUSIC_CHANNEL.channel) == BASS_ACTIVE_PLAYING)
		{
			BASS_ChannelStop(MUSIC_CHANNEL.channel);
			BASS_SampleFree(MUSIC_CHANNEL.channel);
			BASS_MusicFree(MUSIC_CHANNEL.channel);
			BASS_StreamFree(MUSIC_CHANNEL.channel);
		}

		if (SOUND_CHANNELS_INITIALIZED)
		{
			for (int c = 0; c < MAX_BASS_CHANNELS; c++) 
			{// Free channel...
				BASS_StopChannel(c);
				BASS_SampleFree(SOUND_CHANNELS[c].channel);
				BASS_MusicFree(SOUND_CHANNELS[c].channel);
				BASS_StreamFree(SOUND_CHANNELS[c].channel);
			}

			BASS_InitializeChannels();

			SOUND_CHANNELS_INITIALIZED = qfalse;
		}

		if (BASS_UPDATE_THREAD_RUNNING && thread::hardware_concurrency() > 1)
		{// More then one CPU core. We need to shut down the update thread...
			BASS_UPDATE_THREAD_STOP = qtrue;
		}

		if (BASS_MUSIC_UPDATE_THREAD_RUNNING && thread::hardware_concurrency() > 1)
		{// More then one CPU core. We need to shut down the update thread...
			BASS_MUSIC_UPDATE_THREAD_STOP = qtrue;
		}

		return qtrue;
	}

	return qfalse;
}

void BASS_D3DConversion(vec3_t org, vec3_t &outOrg)
{
	float temp = org[1];
	outOrg[0] = org[0];
	outOrg[1] = org[2];
	outOrg[2] = temp;
}

qboolean BASS_Initialize ( void )
{
	if (s_disable->integer)
	{
		return qfalse;
	}

#if 0
	if (!LoadBASS()) Com_Error(ERR_FATAL, "Unable to load BASS sound library.\n");
#endif

	EAX_SUPPORTED = qfalse;

	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
		Com_Printf("An incorrect version of BASS.DLL was loaded.");
		return qfalse;
	}

#ifdef __BASS_FX__
	// check the correct BASS_FX was loaded
	if (HIWORD(BASS_FX_GetVersion()) != BASSVERSION) {
		Com_Printf("An incorrect version of BASS_FX.DLL was loaded (2.4 is required)", "Incorrect BASS_FX.DLL", MB_ICONERROR);
		return qfalse;
	}
#endif //__BASS_FX__

	HWND	win = 0;
	DWORD	freq = 44100;

	if (s_khz->integer == 96)
		freq = 96000;
	else if (s_khz->integer == 48)
		freq = 48000;
	else if (s_khz->integer == 44)
		freq = 44100;
	else if (s_khz->integer == 22)
		freq = 22050;
	else
		freq = 11025;

	Com_Printf("^3BASS Sound System Initializing...\n");

#ifdef __BASS_FX__
	BASS_FX_GetVersion();
	BASS_SetConfig(BASS_CONFIG_FLOATDSP, TRUE);
#endif //__BASS_FX__

	// Initialize the default output device with 3D support
	if (!BASS_Init(-1, freq, /*BASS_DEVICE_DSOUND|*/BASS_DEVICE_3D, win, NULL)) {
		Com_Printf("^3- ^5BASS could not find a sound device.");
		Com_Printf("^3BASS Sound System Initialization ^1FAILED^7!\n");
		return qfalse;
	}

	Com_Printf("^5BASS Selected Device:\n");
	BASS_DEVICEINFO info;
	BASS_GetDeviceInfo(BASS_GetDevice(), &info);
	Com_Printf("^3%s^5.\n", info.name);

	// Use meters as distance unit, real world rolloff, real doppler effect
	BASS_Set3DFactors(SOUND_DISTANCE_UNIT_SIZE, SOUND_REAL_WORLD_FALLOFF, SOUND_REAL_WORLD_DOPPLER);

	// Turn EAX off (volume=0), if error then EAX is not supported
	if (!BASS_SetEAXParameters(-1,0,-1,-1))
	{
		Com_Printf("^3+ ^5EAX Features Supported.\n");
		EAX_SUPPORTED = qtrue;
		//BASS_SetEAX_NORMAL();
		//BASS_SetEAX_FOREST();
	}
	else
	{
		Com_Printf("^1- ^5EAX Features NOT Supported.\n");
		EAX_SUPPORTED = qfalse;
	}

	BASS_Start();

	BASS_SetConfig(BASS_CONFIG_GVOL_MUSIC, (DWORD)(float)(s_volume->value*10000.0));
	BASS_SetConfig(BASS_CONFIG_GVOL_SAMPLE, (DWORD)(float)(s_volume->value*10000.0));
	BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, (DWORD)(float)(s_volume->value*10000.0));

	//BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, (DWORD)16);
	BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, (DWORD)1);
	//BASS_SetConfig(BASS_CONFIG_BUFFER, (DWORD)100); // set the buffer length

	BASS_SetConfig(BASS_CONFIG_DEV_NONSTOP, 1);

	// check for floating-point capability
	BASS_FLOAT_AVAILABLE = BASS_StreamCreate(44100, 2, BASS_SAMPLE_FLOAT, 0, 0);
	if (BASS_FLOAT_AVAILABLE) 
	{
		BASS_StreamFree(BASS_FLOAT_AVAILABLE);  //woohoo!
		BASS_FLOAT_AVAILABLE = BASS_SAMPLE_FLOAT;
		Com_Printf("^3+ ^532 bit floating point sample capability available.\n");
	}
	else
	{
		Com_Printf("^1- ^532 bit floating point sample capability not available.\n");
	}

	//Com_Printf("Volume %f. Sample Volume %i. Stream Volume %i.\n", BASS_GetVolume(), (int)BASS_GetConfig(BASS_CONFIG_GVOL_SAMPLE), (int)BASS_GetConfig(BASS_CONFIG_GVOL_STREAM));

	// Try to use the WDM full 3D algorythm...
	if (BASS_SetConfig(BASS_CONFIG_3DALGORITHM, BASS_3DALG_FULL))
	{
		Com_Printf("^3+ ^5Enhanced Surround Sound Enabled.\n");
	}
	else
	{
		BASS_SetConfig(BASS_CONFIG_3DALGORITHM, BASS_3DALG_DEFAULT);
		Com_Printf("^1- ^5Default Surround Sound Enabled. You need a WDM sound driver to use Enhanced Surround.\n");
	}

	// Set view angles and position to 0,0,0. We will rotate the sound angles...
	vec3_t forward, right, up;
	BASS_3DVECTOR pos, ang, top, vel;

	vel.x = cl.snap.ps.velocity[0];
	vel.y = cl.snap.ps.velocity[2];
	vel.z = cl.snap.ps.velocity[1];

	pos.x = cl.snap.ps.origin[0];
	pos.y = cl.snap.ps.origin[2];
	pos.z = cl.snap.ps.origin[1];

	AngleVectors(cl.snap.ps.viewangles, forward, right, up);

	ang.x = forward[0];
	ang.y = forward[2];
	ang.z = forward[1];

	top.x = up[0];
	top.y = up[2];
	top.z = up[1];

	BASS_Set3DPosition(&pos, &vel, &ang, &top);

	Com_Printf("^3BASS Sound System initialized ^7OK^3!\n");

	BASS_UPDATE_THREAD_STOP = qfalse;
	BASS_MUSIC_UPDATE_THREAD_STOP = qfalse;

	// Initialize all the sound channels ready for use...
	BASS_InitializeChannels();

	//BASS_SetConfig(BASS_CONFIG_CURVE_VOL, true);

	// UQ1: Play a BASS startup sound...
	//BASS_AddStreamChannel ( "warzone/sound/startup.wav", -1, s_volume->value, NULL );

	s_soundStarted = 1;
	s_numSfx = 0;
	S_BeginRegistration();

	BASS_INITIALIZED = qtrue;

	return qtrue;
}


//
// Position Utils...
//

float BASS_GetVolumeForChannel ( int entchannel )
{
	float		volume = 1;
	float		master_vol, voice_vol, effects_vol, ambient_vol, weapon_vol, saber_vol, item_vol, body_vol, music_vol, local_vol, ambient_effects_vol;

	if (BASS_CheckSoundDisabled())
	{
		return 0.0;
	}

	master_vol			= s_volume->value;
	voice_vol			= (s_volumeVoice->value*master_vol);
	ambient_vol			= (s_volumeAmbient->value*master_vol);
	ambient_effects_vol	= (s_volumeAmbientEfx->value*master_vol);
	music_vol			= (s_volumeMusic->value*master_vol);
	local_vol			= (s_volumeLocal->value*master_vol);

	effects_vol			= (s_volumeEffects->value*master_vol);
	weapon_vol			= (s_volumeWeapon->value*effects_vol);
	saber_vol			= (s_volumeSaber->value*effects_vol);
	item_vol			= (s_volumeItem->value*effects_vol);
	body_vol			= (s_volumeBody->value*effects_vol);

	if ( entchannel == CHAN_VOICE || entchannel == CHAN_VOICE_ATTEN )
	{
		volume = voice_vol;
	}
	else if (entchannel == CHAN_WEAPON || entchannel == CHAN_WEAPONLOCAL)
	{
		volume = weapon_vol;
	}
	else if (entchannel == CHAN_SABER || entchannel == CHAN_SABERLOCAL)
	{
		volume = saber_vol;
	}
	else if (entchannel == CHAN_ITEM)
	{
		volume = item_vol;
	}
	else if (entchannel == CHAN_BODY || entchannel == CHAN_CULLRANGE_8192 || entchannel == CHAN_CULLRANGE_16384 || entchannel == CHAN_CULLRANGE_32768)
	{
		volume = body_vol;
	}
	else if (entchannel == CHAN_LESS_ATTEN || entchannel == CHAN_AUTO)
	{
		volume = effects_vol;
	}
	else if (entchannel == CHAN_AMBIENT_EFX)
	{
		volume = ambient_effects_vol;
	}
	else if (entchannel == CHAN_AMBIENT)
	{
		volume = ambient_vol;
	}
	else if (entchannel == CHAN_MUSIC)
	{
		volume = music_vol;
	}
	else //CHAN_LOCAL //CHAN_ANNOUNCER //CHAN_LOCAL_SOUND //CHAN_MENU1 //CHAN_VOICE_GLOBAL
	{
		volume = local_vol;
	}

	return volume;
}


#ifdef __BASS_FX__
void UpdateFreeverbFX(int chan)
{
	Channel *c = &SOUND_CHANNELS[chan];

	BASS_BFX_FREEVERB frb;

	float dryMix = 0.0;// s_testvalue0->value;
	float wetMix = 1.0;// s_testvalue1->value;
	float roomSize = 0.5;// s_testvalue2->value;
	float damp = 0.5;// s_testvalue3->value;
	float width = 1.0;// s_testvalue4->value;
	//float mode = s_testvalue5->value;

	BASS_FXGetParameters(c->fxReverb, &frb);
	frb.fDryMix = dryMix;
	frb.fWetMix = wetMix;
	frb.fRoomSize = roomSize;
	frb.fDamp = damp;
	frb.fWidth = width;
	frb.lMode = 0;
	frb.lChannel = BASS_BFX_CHANALL;
	BASS_FXSetParameters(c->fxReverb, &frb);

	//BASS_FXGetParameters(c->fxReverb, &frb);
	//Com_Printf("dryMix %f. wetMix %f. roomSize %f. damp %f. fWidth %f.\n", frb.fDryMix, frb.fWetMix, frb.fRoomSize, frb.fDamp, frb.fWidth);
}

// set dsp eq
void UpdatePeakEQFX(int chan, float fGain, float fBandwidth, float fQ, float fCenter_Bass, float fCenter_Mid, float fCenter_Treble)
{
	BASS_BFX_PEAKEQ eq;

	// set peaking equalizer effect with no bands
	Channel *c = &SOUND_CHANNELS[chan];
	DWORD channel = c->channel;

	if (!c->fxPeakEQ)
	{
		c->fxPeakEQ = BASS_ChannelSetFX(channel, BASS_FX_BFX_PEAKEQ, 1);
	}

	eq.fGain = fGain;
	eq.fQ = fQ;
	eq.fBandwidth = fBandwidth;
	eq.lChannel = BASS_BFX_CHANALL;

	// create 1st band for bass
	eq.lBand = 0;
	eq.fCenter = fCenter_Bass;
	BASS_FXSetParameters(c->fxPeakEQ, &eq);

	// create 2nd band for mid
	eq.lBand = 1;
	eq.fCenter = fCenter_Mid;
	BASS_FXSetParameters(c->fxPeakEQ, &eq);

	// create 3rd band for treble
	eq.lBand = 2;
	eq.fCenter = fCenter_Treble;
	BASS_FXSetParameters(c->fxPeakEQ, &eq);
}
#endif //__BASS_FX__

void BASS_UpdatePosition ( int ch, qboolean IS_NEW_SOUND )
{// Update this channel's position, etc...
	qboolean	IS_LOCAL_SOUND = qfalse;
	Channel		*c = &SOUND_CHANNELS[ch];
	int			SOUND_ENTITY = -1;
	float		CHAN_VOLUME = 0.0;
	vec3_t		porg, corg;

	if (BASS_CheckSoundDisabled())
	{
		return;
	}

	if (!c) return; // should be impossible, but just in case...
	//if (!IS_NEW_SOUND && !c->isLooping) return; // We don't even need to update do we???

	float dist = (c->origin[0] == 0 && c->origin[1] == 0 && c->origin[2] == 0) ? 0.0 : Distance(cl.snap.ps.origin, c->origin);

	float CHAN_BASE_VOLUME = BASS_GetVolumeForChannel(c->entityChannel);

	if (CHAN_BASE_VOLUME <= 0)
	{
		BASS_StopChannel(ch);
//#ifdef __BASS_DEBUG__
//		Com_Printf("SOUND: Channel %i stopped. Channel's base volume is zero.\n", ch);
//#endif //__BASS_DEBUG__
		return;
	}

	SOUND_ENTITY = c->entityNum;
	CHAN_VOLUME = c->volume*CHAN_BASE_VOLUME;

	if (IS_NEW_SOUND && !(c->origin[0] == 0 && c->origin[1] == 0 && c->origin[2] == 0))
	{// New sound with an origin... Use the specified origin...

	}
	else if (SOUND_ENTITY == -1)
	{// Either a local sound, or we hopefully already have an origin...

	}
	else if (!c->isLooping
		&& !(s_entityPosition[SOUND_ENTITY][0] == 0 
		&& s_entityPosition[SOUND_ENTITY][1] == 0 
		&& s_entityPosition[SOUND_ENTITY][2] == 0))
	{// UPDATE POSITION - Primary - use s_entityPosition if we have one...
		VectorCopy(s_entityPosition[SOUND_ENTITY], c->origin);
	}
	else if (!c->isLooping
		&& !(cl.entityBaselines[SOUND_ENTITY].origin[0] == 0 
		&& cl.entityBaselines[SOUND_ENTITY].origin[1] == 0 
		&& cl.entityBaselines[SOUND_ENTITY].origin[2] == 0))
	{// UPDATE POSITION - Backup - use entity baseline origins...
		VectorCopy(cl.entityBaselines[SOUND_ENTITY].origin, c->origin);
	}
	
	if (c->origin[0] == 0 
		&& c->origin[1] == 0 
		&& c->origin[2] == 0)
	{// Must be a local sound...
		VectorSet(c->origin, 0, 0, 0);
		IS_LOCAL_SOUND = qtrue;
	}
	else if (c->entityChannel == CHAN_AMBIENT || c->entityChannel == CHAN_WEAPONLOCAL || c->entityChannel == CHAN_SABERLOCAL)
	{// Force all ambient sounds to local...
		VectorSet(c->origin, 0, 0, 0);
		IS_LOCAL_SOUND = qtrue;
	}
	else if (c->cullRange && dist > c->cullRange)
	{// Left sound range, remove it...
		BASS_StopChannel(ch);
#ifdef __BASS_DEBUG__
		Com_Printf("SOUND: Channel %i stopped. Out of cullRange %f > %f.\n", ch, dist, c->cullRange);
#endif //__BASS_DEBUG__
		return;
	}
	else if (c->cullRange)
	{// Update volume...
		float vol = 1.0 - Q_clamp(0.0, dist / c->cullRange, 1.0);
		CHAN_VOLUME = c->volume*vol*CHAN_BASE_VOLUME;
	}
	else if (dist > MAX_SOUND_RANGE)
	{// Left sound range, remove it...
		BASS_StopChannel(ch);
#ifdef __BASS_DEBUG__
		Com_Printf("SOUND: Channel %i stopped. Out of MAX_SOUND_RANGE %f > %f.\n", ch, dist, MAX_SOUND_RANGE);
#endif //__BASS_DEBUG__
		return;
	}
	else
	{// Update volume...
		float vol = 1.0 - Q_clamp(0.0, dist / MAX_SOUND_RANGE, 1.0);
		CHAN_VOLUME = c->volume*vol*CHAN_BASE_VOLUME;
	}

	if (CHAN_VOLUME <= 0.0)
	{
		BASS_StopChannel(ch);
#ifdef __BASS_DEBUG__
		Com_Printf("SOUND: Channel %i stopped. Volume is zero. CHAN_VOLUME %f. c->volume %f. CHAN_BASE_VOLUME %f. Dist %f. Cullrange %f.\n", ch, CHAN_VOLUME, c->volume, CHAN_BASE_VOLUME, dist, c->cullRange ? c->cullRange : MAX_SOUND_RANGE);
#endif //__BASS_DEBUG__
		return;
	}

	if (SOUND_ENTITY == cl.snap.ps.clientNum)
	{
		c->vel.x = cl.snap.ps.velocity[0];
		c->vel.y = cl.snap.ps.velocity[2];
		c->vel.z = cl.snap.ps.velocity[1];
		
		vec3_t fwd;
		AngleVectors(cl.snap.ps.viewangles, fwd, NULL, NULL);
		c->ang.x = fwd[0];
		c->ang.y = fwd[2];
		c->ang.z = fwd[1];
	}
	else if (SOUND_ENTITY >= 0 && SOUND_ENTITY < ENTITYNUM_MAX_NORMAL)
	{
		entityState_t *es = &cl.entityBaselines[SOUND_ENTITY];
		c->vel.x = es->pos.trDelta[0];
		c->vel.y = es->pos.trDelta[2];
		c->vel.z = es->pos.trDelta[1];

		vec3_t fwd;
		AngleVectors(es->angles, fwd, NULL, NULL);
		c->ang.x = fwd[0];
		c->ang.y = fwd[2];
		c->ang.z = fwd[1];
	}
	else
	{
		c->vel.x = 0;
		c->vel.y = 0;
		c->vel.z = 0;

		c->ang.x = 0;
		c->ang.y = 0;
		c->ang.z = 0;
	}

	VectorCopy(cl.snap.ps.origin, porg);
	BASS_D3DConversion(porg, porg);

	if (IS_LOCAL_SOUND)
	{
		// Set origin...
		c->pos.x = porg[0];
		c->pos.y = porg[1];
		c->pos.z = porg[2];

		BASS_ChannelSet3DPosition(c->channel, &c->pos, /*NULL*/&c->ang, &c->vel);

		//if (!BASS_ChannelIsSliding(c->channel, BASS_ATTRIB_VOL))
		{
			BASS_ChannelSet3DAttributes(c->channel, BASS_3DMODE_OFF, 1, 1, 1, 0, CHAN_VOLUME);
			BASS_ChannelSetAttribute(c->channel, BASS_ATTRIB_VOL, CHAN_VOLUME);
			//if (c->isLooping) BASS_ChannelFlags(c->channel, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);
		}
	}
	else
	{
		VectorCopy(c->origin, corg);
		BASS_D3DConversion(corg, corg);

		// Set origin...
		c->pos.x = corg[0];
		c->pos.y = corg[1];
		c->pos.z = corg[2];

		//Com_Printf("SOUND: Pos %f %f %f. Vel %f %f %f.\n", c->pos.x, c->pos.y, c->pos.z, c->vel.x, c->vel.y, c->vel.z);

		BASS_ChannelSet3DPosition(c->channel, &c->pos, /*NULL*/&c->ang, &c->vel);

		BASS_ChannelSet3DAttributes(c->channel, SOUND_3D_METHOD, c->cullRange > 0 ? c->cullRange : MAX_SOUND_RANGE, c->cullRange > 0 ? c->cullRange : MAX_SOUND_RANGE, 0, 0, 1);
		BASS_ChannelSetAttribute(c->channel, BASS_ATTRIB_VOL, CHAN_VOLUME);
		BASS_ChannelFlags(c->channel, BASS_SAMPLE_MUTEMAX, BASS_SAMPLE_MUTEMAX); // enable muting at the max distance

#ifdef __BASS_DEBUG__
		//Com_Printf("SOUND: Channel: %i. Porg: %f %f %f. Pos %f %f %f. Ch Volume: %f. Final Volume %f. EntNum: %i. EntChan: %i.\n", ch, porg[0], porg[2], porg[1], corg[0], corg[2], corg[1], c->volume, CHAN_VOLUME, c->entityNum, c->entityChannel);
#endif //__BASS_DEBUG__
	}

#ifdef __BASS_FX__
	UpdateFreeverbFX(ch);

	/*
	int   lBand;							// [0...............n] more bands means more memory & cpu usage
	float fBandwidth;						// [0.1...........<10] in octaves - fQ is not in use (Bandwidth has a priority over fQ)
	float fQ;								// [0...............1] the EE kinda definition (linear) (if Bandwidth is not in use)
	float fCenter;							// [1Hz..<info.freq/2] in Hz
	float fGain;							// [-15dB...0...+15dB] in dB (can be above/below these limits)
	int   lChannel;							// BASS_BFX_CHANxxx flag/s
	*/
	//SetDSP_EQ(0.0f, 2.5f, 0.0f, 125.0f, 1000.0f, 8000.0f);
	//UpdatePeakEQFX(ch, 15.0f, 2.5f, 0.0f, 125.0f, 1000.0f, 8000.0f);
#endif //__BASS_FX__
}

void BASS_ProcessStartRequest( int channel )
{
	Channel *c = &SOUND_CHANNELS[channel];

	if (BASS_CheckSoundDisabled())
	{
		BASS_InitChannel(c);
		return;
	}

	DWORD count = BASS_SampleGetChannels(c->originalChannel, NULL);
	if (count == -1) 
	{// fail to find a channel...
#ifdef __BASS_DEBUG__
		Com_Printf("SOUND: Channel %i failed to find an instance channel. Looping %i. OutChan %lu. OrigChan %lu. Volume %f.\n", channel, (int)c->isLooping, c->channel, c->originalChannel, c->volume);

		int error = BASS_ErrorGetCode();

		switch (error)
		{
		case BASS_ERROR_HANDLE:
			Com_Printf("SOUND: Handle is not a valid sample handle.\n");
			break;
		case BASS_ERROR_NOCHAN:
			Com_Printf("SOUND: The sample has no free channels.\n");
			break;
		case BASS_ERROR_TIMEOUT:
			Com_Printf("SOUND: The sample's minimum time gap (BASS_SAMPLE) has not yet passed since the last channel was created.\n");
			break;
		}
#endif //__BASS_DEBUG__

		c->startRequest = qfalse;
		c->isLooping = qfalse;
		c->isActive = qfalse;
		BASS_StopChannel(channel);
		return;
	}

	c->channel = BASS_SampleGetChannel(c->originalChannel, TRUE); // initialize sample channel

#ifdef __BASS_DEBUG__
	Com_Printf("SOUND: Channel %i. EntChannel: %i. Count: %i. Looping %i. OutChan %lu. Volume %f.\n", channel, c->entityChannel, count, (int)c->isLooping, c->channel, c->volume);
#endif //__BASS_DEBUG__

	//BASS_ChannelSlideAttribute(c->channel, BASS_ATTRIB_VOL, c->volume*BASS_GetVolumeForChannel(c->entityChannel), 1000); // fade-in over 100ms

	// Apply the 3D changes
	BASS_UpdatePosition(channel, qtrue);

#ifdef __BASS_FX__
	c->fxReverb = BASS_ChannelSetFX(c->channel, BASS_FX_BFX_FREEVERB, 0);

	if (!c->fxReverb)
	{
		int error = BASS_ErrorGetCode();

		switch (error)
		{
		case BASS_ERROR_HANDLE:
			Com_Printf("SOUND: Handle is not a channel.\n");
			break;
		case BASS_ERROR_ILLTYPE:
			Com_Printf("SOUND: type is invalid. Note BASS_FX must be loaded before these effects can be used.\n");
			break;
		case BASS_ERROR_FORMAT:
			Com_Printf("SOUND: The selected effect could be applied only on stereo or mono handle.\n");
			break;
		}
	}
#else //!__BASS_FX__
	//c->fxReverb = BASS_ChannelSetFX(c->channel, BASS_FX_DX8_REVERB, 0);
	//c->fxReverb = BASS_ChannelSetFX(c->channel, BASS_FX_DX8_I3DL2REVERB, 0);
#endif //__BASS_FX__

	//if (c->isLooping)
	//{
	//	BASS_ChannelFlags(c->channel, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP); // set the LOOP flag
	//}

	// Play
	BASS_ChannelPlay(c->channel, FALSE);

	BASS_Apply3D();

	c->startRequest = qfalse;
	c->isActive = qtrue;
}

void BASS_UpdateSoundsLoop ( void )
{
	vec3_t forward, right, up, porg;
	BASS_3DVECTOR pos, ang, top, vel;

	if (BASS_CheckSoundDisabled())
	{
		return;
	}

	VectorCopy(cl.snap.ps.origin, porg);

	BASS_D3DConversion(porg, porg);

	vel.x = cl.snap.ps.velocity[0];
	vel.y = cl.snap.ps.velocity[2];
	vel.z = cl.snap.ps.velocity[1];

	pos.x = porg[0];
	pos.y = porg[2];
	pos.z = porg[1];

	AngleVectors(cl.snap.ps.viewangles, forward, right, up);

	ang.x = forward[0];
	ang.y = forward[2];
	ang.z = forward[1];

	top.x = up[0];
	top.y = up[2];
	top.z = up[1];

	BASS_Set3DPosition(&pos, &vel, &ang, &top);

	BASS_ChannelSetAttribute(MUSIC_CHANNEL.channel, BASS_ATTRIB_VOL, MUSIC_CHANNEL.volume*BASS_GetVolumeForChannel(CHAN_MUSIC));

	for (int ch = 0; ch < MAX_BASS_CHANNELS; ch++) 
	{
		Channel *c = &SOUND_CHANNELS[ch];

		if (c->startRequest)
		{// Start any channels that have been requested...
			BASS_ProcessStartRequest(ch);
		}
		else if (c->isActive && BASS_ChannelIsActive(c->channel) == BASS_ACTIVE_PLAYING)
		{// If the channel's playing then update it's position
			BASS_UpdatePosition(ch, qfalse);
		}
		else
		{// Finished. Remove the channel...
			if (BASS_ChannelIsActive(c->channel) != BASS_ACTIVE_PLAYING)
			{
#ifdef __BASS_DEBUG__
				Com_Printf("SOUND: Channel %i (%lu) finished.\n", ch, c->channel);
#endif //__BASS_DEBUG__
				BASS_StopChannel(ch);
			}
		}
	}

	BASS_Apply3D();
}

void BASS_UpdateThread(void * aArg)
{
	while (!BASS_UPDATE_THREAD_STOP)
	{
		while (!FS_STARTUP_COMPLETE)
		{
			this_thread::sleep_for(chrono::milliseconds(1000));
			continue;
		}

		BASS_UpdateSoundsLoop();
		this_thread::sleep_for(chrono::milliseconds(2));

		if (BASS_CheckSoundDisabled())
		{
			break;
		}
	}

	BASS_UPDATE_THREAD_RUNNING = qfalse;
}

void BASS_UpdateSounds ( void )
{
	if (!BASS_INITIALIZED) return; // wait...

	if (thread::hardware_concurrency() <= 1)
	{// Only one CPU core to use. Don't thread...
		BASS_UpdateSoundsLoop();
		return;
	}

	if (!BASS_UPDATE_THREAD_RUNNING)
	{
		BASS_UPDATE_THREAD_RUNNING = qtrue;
		BASS_UPDATE_THREAD = new thread (BASS_UpdateThread, 0);
	}
}

//
// Effects...
//

void BASS_SetEAX_NORMAL ( void )
{
	if (BASS_CheckSoundDisabled()) return;
	if (!EAX_SUPPORTED) return;

	BASS_SetEAXParameters(EAX_PRESET_GENERIC);
}

void BASS_SetEAX_FOREST(void)
{
	if (BASS_CheckSoundDisabled()) return;
	if (!EAX_SUPPORTED) return;

	BASS_SetEAXParameters(EAX_PRESET_FOREST);
}

void BASS_SetEAX_MOUNTAINS(void)
{
	if (BASS_CheckSoundDisabled()) return;
	if (!EAX_SUPPORTED) return;

	BASS_SetEAXParameters(EAX_PRESET_MOUNTAINS);
}

void BASS_SetEAX_CAVE(void)
{
	if (BASS_CheckSoundDisabled()) return;
	if (!EAX_SUPPORTED) return;

	BASS_SetEAXParameters(EAX_PRESET_CAVE);
}

void BASS_SetEAX_UNDERWATER ( void )
{
	if (BASS_CheckSoundDisabled()) return;
	if (!EAX_SUPPORTED) return;

	BASS_SetEAXParameters(EAX_PRESET_UNDERWATER);
}

void BASS_SetRolloffFactor ( int factor )
{
	if (BASS_CheckSoundDisabled()) return;
	BASS_Set3DFactors(-1,pow(2,(factor-10)/5.0),-1);
}

void BASS_SetDopplerFactor ( int factor )
{
	if (BASS_CheckSoundDisabled()) return;
	BASS_Set3DFactors(-1,-1,pow(2,(factor-10)/5.0));
}

//
// Music Tracks...
//

qboolean BASS_MUSIC_STARTING = qfalse;

void BASS_StopMusic( DWORD samplechan )
{
	while (BASS_MUSIC_STARTING || MUSIC_LOADING)
	{
		Sleep(1);
	}

	// Free old samples...
	BASS_ChannelStop(MUSIC_CHANNEL.channel);
	BASS_SampleFree(MUSIC_CHANNEL.channel);
	BASS_MusicFree(MUSIC_CHANNEL.channel);
	BASS_StreamFree(MUSIC_CHANNEL.channel);
}

void BASS_StartMusic ( DWORD samplechan )
{
	if (BASS_CheckSoundDisabled()) return;
	if (!BASS_INITIALIZED) return;

	BASS_MUSIC_STARTING = qtrue;

	// Set new samples...
	MUSIC_CHANNEL.originalChannel=MUSIC_CHANNEL.channel = samplechan;
	MUSIC_CHANNEL.entityNum = -1;
	MUSIC_CHANNEL.entityChannel = CHAN_MUSIC;
	MUSIC_CHANNEL.volume = 1.0;

	BASS_SampleGetChannel(samplechan,FALSE); // initialize sample channel
	BASS_ChannelSetAttribute(samplechan, BASS_ATTRIB_VOL, MUSIC_CHANNEL.volume*BASS_GetVolumeForChannel(CHAN_MUSIC));

	// Play
	BASS_ChannelPlay(samplechan,FALSE);

	// Apply the 3D settings (music is always local)...
	// Set velocity to 0...
	MUSIC_CHANNEL.vel.x = 0;
	MUSIC_CHANNEL.vel.y = 0;
	MUSIC_CHANNEL.vel.z = 0;
		
	// Set origin to 0...
	MUSIC_CHANNEL.pos.x = 0;
	MUSIC_CHANNEL.pos.y = 0;
	MUSIC_CHANNEL.pos.z = 0;
	BASS_ChannelSet3DPosition(MUSIC_CHANNEL.channel, &MUSIC_CHANNEL.pos, NULL, &MUSIC_CHANNEL.vel);

	float CHAN_VOLUME = MUSIC_CHANNEL.volume*BASS_GetVolumeForChannel(MUSIC_CHANNEL.entityChannel);
	BASS_ChannelSet3DAttributes(MUSIC_CHANNEL.channel, BASS_3DMODE_OFF, 1, 1, 1, 0, CHAN_VOLUME);
	BASS_ChannelSetAttribute(MUSIC_CHANNEL.channel, BASS_ATTRIB_VOL, CHAN_VOLUME);
	BASS_Apply3D();

	BASS_MUSIC_STARTING = qfalse;
}

DWORD BASS_LoadMusicSample ( void *memory, int length )
{// Just load a sample into memory ready to play instantly...
	DWORD	newchan;
	int		flags = 0;

	if (BASS_CheckSoundDisabled()) return -1;
	if (!BASS_INITIALIZED) return -1;

	if (!s_allowDynamicMusic->integer)
	{
		flags = BASS_SAMPLE_LOOP;
	}

	MUSIC_LOADING = qtrue;

	// Try to load the sample with the highest quality options we support...
	if (newchan=BASS_SampleLoad(TRUE,memory,0,(DWORD)length,1,flags|BASS_SAMPLE_FLOAT))
	{
		MUSIC_LOADING = qfalse;
		return newchan;
	}
	else if (newchan=BASS_SampleLoad(TRUE,memory,0,(DWORD)length,1,flags))
	{
		MUSIC_LOADING = qfalse;
		return newchan;
	}

	MUSIC_LOADING = qfalse;
	return -1;
}

//
//
// New dynamic music system...
//
//

//
// JKA Tracks...
//

const char *JKA_TRACKS[] =
{
	"music/badsmall.mp3",
	"music/cinematic_1.mp3",
	"music/death_music.mp3",
	"music/endcredits.mp3",
	"music/goodsmall.mp3",
	"music/artus_topside/impbased_action.mp3",
	"music/bespin_streets/BespinA_Action.mp3",
	"music/hoth2/hoth2_action.mp3",
	"music/hoth2/hoth2_atr00.mp3",
	"music/hoth2/hoth2_atr01.mp3",
	"music/hoth2/hoth2_atr02.mp3",
	"music/hoth2/hoth2_atr03.mp3",
	"music/hoth2/hoth2_etr00.mp3",
	"music/hoth2/hoth2_etr01.mp3",
	"music/hoth2/hoth2_explore.mp3",
	"music/kor1/korrib_action.mp3",
	"music/kor_dark/final_battle.mp3",
	"music/kor_dark/korrib_action.mp3",
	"music/kor_dark/korrib_atr00.mp3",
	"music/kor_dark/korrib_atr01.mp3",
	"music/kor_dark/korrib_atr02.mp3",
	"music/kor_dark/korrib_atr03.mp3",
	"music/kor_dark/korrib_dark_etr00.mp3",
	"music/kor_dark/korrib_dark_etr01.mp3",
	"music/kor_dark/korrib_dark_explore.mp3",
	"music/kor_lite/final_battle.mp3",
	"music/kor_lite/korrib_action.mp3",
	"music/kor_lite/korrib_atr00.mp3",
	"music/kor_lite/korrib_atr01.mp3",
	"music/kor_lite/korrib_atr02.mp3",
	"music/kor_lite/korrib_atr03.mp3",
	"music/kor_lite/korrib_lite_etr00.mp3",
	"music/kor_lite/korrib_lite_etr01.mp3",
	"music/kor_lite/korrib_lite_explore.mp3",
	"music/mp/duel.mp3",
	"music/mp/mp_action1.mp3",
	"music/mp/mp_action2.mp3",
	"music/mp/mp_action3.mp3",
	"music/mp/MP_action4.mp3",
	"music/t1_fatal/tunnels_action.mp3",
	"music/t1_fatal/tunnels_atr00.mp3",
	"music/t1_fatal/tunnels_atr01.mp3",
	"music/t1_fatal/tunnels_atr02.mp3",
	"music/t1_fatal/tunnels_atr03.mp3",
	"music/t1_fatal/tunnels_etr00.mp3",
	"music/t1_fatal/tunnels_etr01.mp3",
	"music/t1_fatal/tunnels_explore.mp3",
	"music/t1_rail/rail_nowhere.mp3",
	"music/t1_sour/dealsour_action.mp3",
	"music/t1_sour/dealsour_atr00.mp3",
	"music/t1_sour/dealsour_atr01.mp3",
	"music/t1_sour/dealsour_atr02.mp3",
	"music/t1_sour/dealsour_etr00.mp3",
	"music/t1_sour/dealsour_etr01.mp3",
	"music/t1_sour/dealsour_explore.mp3",
	"music/t1_surprise/tusken_action.mp3",
	"music/t1_surprise/tusken_atr00.mp3",
	"music/t1_surprise/tusken_atr01.mp3",
	"music/t1_surprise/tusken_atr02.mp3",
	"music/t1_surprise/tusken_atr03.mp3",
	"music/t1_surprise/tusken_etr00.mp3",
	"music/t1_surprise/tusken_etr01.mp3",
	"music/t1_surprise/tusken_explore.mp3",
	"music/t2_dpred/impbaseb_action.mp3",
	"music/t2_dpred/impbaseb_atr00.mp3",
	"music/t2_dpred/impbaseb_atr01.mp3",
	"music/t2_dpred/impbaseb_atr02.mp3",
	"music/t2_dpred/impbaseb_atr03.mp3",
	"music/t2_dpred/impbaseb_etr00.mp3",
	"music/t2_dpred/impbaseb_etr01.mp3",
	"music/t2_dpred/impbaseb_explore.mp3",
	"music/t2_rancor/rancor_action.mp3",
	"music/t2_rancor/rancor_atr00.mp3",
	"music/t2_rancor/rancor_atr01.mp3",
	"music/t2_rancor/rancor_atr02.mp3",
	"music/t2_rancor/rancor_etr00.mp3",
	"music/t2_rancor/rancor_etr01.mp3",
	"music/t2_rancor/rancor_explore.mp3",
	"music/t2_rogue/narshaada_action.mp3",
	"music/t2_rogue/narshaada_atr00.mp3",
	"music/t2_rogue/narshaada_atr01.mp3",
	"music/t2_rogue/narshaada_atr02.mp3",
	"music/t2_rogue/narshaada_atr03.mp3",
	"music/t2_rogue/narshaada_etr00.mp3",
	"music/t2_rogue/narshaada_etr01.mp3",
	"music/t2_rogue/narshaada_explore.mp3",
	"music/t2_wedge/besplat_action.mp3",
	"music/t2_wedge/besplat_atr00.mp3",
	"music/t2_wedge/besplat_atr01.mp3",
	"music/t2_wedge/besplat_atr02.mp3",
	"music/t2_wedge/besplat_boss.mp3",
	"music/t2_wedge/besplat_etr00.mp3",
	"music/t2_wedge/besplat_etr01.mp3",
	"music/t2_wedge/besplat_explore.mp3",
	"music/t3_byss/alienhb_action.mp3",
	"music/t3_byss/alienhb_atr00.mp3",
	"music/t3_byss/alienhb_atr01.mp3",
	"music/t3_byss/alienhb_atr02.mp3",
	"music/t3_byss/alienhb_etr00.mp3",
	"music/t3_byss/alienhb_etr01.mp3",
	"music/t3_byss/alienhb_explore.mp3",
	"music/vjun2/impbasee_action.mp3",
	"music/vjun2/impbasee_atr00.mp3",
	"music/vjun2/impbasee_atr01.mp3",
	"music/vjun2/impbasee_atr02.mp3",
	"music/vjun2/impbasee_atr03.mp3",
	"music/vjun2/impbasee_etr00.mp3",
	"music/vjun2/impbasee_etr01.mp3",
	"music/vjun2/impbasee_explore.mp3",
	"music/vjun3/vjun3_action.mp3",
	"music/vjun3/vjun3_atr00.mp3",
	"music/vjun3/vjun3_atr01.mp3",
	"music/vjun3/vjun3_atr02.mp3",
	"music/vjun3/vjun3_etr00.mp3",
	"music/vjun3/vjun3_etr01.mp3",
	"music/vjun3/vjun3_explore.mp3",
	"music/yavin1/swamp_action.mp3",
	"music/yavin1/swamp_atr00.mp3",
	"music/yavin1/swamp_atr01.mp3",
	"music/yavin1/swamp_atr02.mp3",
	"music/yavin1/swamp_atr03.mp3",
	"music/yavin1/swamp_etr00.mp3",
	"music/yavin1/swamp_etr01.mp3",
	"music/yavin1/swamp_explore.mp3",
	"music/yavin2/yavtemp2_action.mp3",
	"music/yavin2/yavtemp2_atr00.mp3",
	"music/yavin2/yavtemp2_atr01.mp3",
	"music/yavin2/yavtemp2_atr02.mp3",
	"music/yavin2/yavtemp2_atr03.mp3",
	"music/yavin2/yavtemp2_etr00.mp3",
	"music/yavin2/yavtemp2_etr01.mp3",
	"music/yavin2/yavtemp2_explore.mp3",
	"music/yavin2_old/yavtemp_explore.mp3",
};

int GALACTIC_RADIO_TRACKS_NUM = 133;

// channel (sample/music) info structure
typedef struct {
	char name[260 + 1];
} radioMusicList_t;

//
// Galactic Radio (extra) Tracks...
//

qboolean GALACTIC_RADIO_EXTRA_TRACKS_LOADED = qfalse;

int GALACTIC_RADIO_EXTRA_TRACKS_NUM = 0;

radioMusicList_t GALACTIC_RADIO_EXTRA_TRACKS[512 + 1];

void BASS_GetGalacticExtraTracks(void)
{
	if (GALACTIC_RADIO_EXTRA_TRACKS_LOADED) return;

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir("warzone/music/galactic")) != NULL) {
		/* all the files and directories within psy directory */
		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_name[0] == '.') continue; // skip back directory...
			if (ent->d_namlen < 3) continue;

			sprintf(GALACTIC_RADIO_EXTRA_TRACKS[GALACTIC_RADIO_EXTRA_TRACKS_NUM].name, "music/galactic/%s", ent->d_name);
			//Com_Printf("Added relaxing track %s.\n", GALACTIC_RADIO_TRACKS[GALACTIC_RADIO_TRACKS_NUM].name);
			GALACTIC_RADIO_EXTRA_TRACKS_NUM++;
		}
		closedir(dir);
	}
	else {
		/* could not open directory */
		perror("");
	}

	GALACTIC_RADIO_EXTRA_TRACKS_LOADED = qtrue;
	//Com_Printf("^3BASS Sound System ^4- ^5Loaded ^7%i ^3Galactic Radio^5 extra music tracks.\n", GALACTIC_RADIO_EXTRA_TRACKS_NUM);
}

//
// Mind Worm Radio Tracks...
//

qboolean MINDWORM_TRACKS_LOADED = qfalse;

int MINDWORM_TRACKS_NUM = 0;

radioMusicList_t MINDWORM_TRACKS[512+1];

void BASS_GetMindWormTracks( void )
{
	if (MINDWORM_TRACKS_LOADED) return;

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir ("warzone/music/mindworm")) != NULL) {
		/* all the files and directories within psy directory */
		while ((ent = readdir (dir)) != NULL) {
			if (ent->d_name[0] == '.') continue; // skip back directory...
			if (ent->d_namlen < 3) continue;

			sprintf(MINDWORM_TRACKS[MINDWORM_TRACKS_NUM].name, "music/mindworm/%s", ent->d_name);
			//Com_Printf("Added mindworm track %s.\n", MINDWORM_TRACKS[MINDWORM_TRACKS_NUM].name);
			MINDWORM_TRACKS_NUM++;
		}
		closedir (dir);
	} else {
		/* could not open directory */
		perror ("");
	}

	MINDWORM_TRACKS_LOADED = qtrue;
	Com_Printf("^3BASS Sound System ^4- ^5Loaded ^7%i ^3Mindworm Radio^5 music tracks.\n", MINDWORM_TRACKS_NUM);
}

//
// Relaxing In The Rim Radio Tracks...
//

qboolean RELAXING_TRACKS_LOADED = qfalse;

int RELAXING_TRACKS_NUM = 0;

radioMusicList_t RELAXING_TRACKS[512 + 1];

void BASS_GetRelaxingTracks(void)
{
	if (RELAXING_TRACKS_LOADED) return;

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir("warzone/music/relaxing")) != NULL) {
		/* all the files and directories within psy directory */
		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_name[0] == '.') continue; // skip back directory...
			if (ent->d_namlen < 3) continue;

			sprintf(RELAXING_TRACKS[RELAXING_TRACKS_NUM].name, "music/relaxing/%s", ent->d_name);
			//Com_Printf("Added relaxing track %s.\n", RELAXING_TRACKS[MINDWORM_TRACKS_NUM].name);
			RELAXING_TRACKS_NUM++;
		}
		closedir(dir);
	}
	else {
		/* could not open directory */
		perror("");
	}

	RELAXING_TRACKS_LOADED = qtrue;
	Com_Printf("^3BASS Sound System ^4- ^5Loaded ^7%i ^3Relaxing In The Rim Radio^5 music tracks.\n", RELAXING_TRACKS_NUM);
}

// cl.mapname
//
// Map based radio station...
//

qboolean MAP_STATION_TRACKS_LOADED = qfalse;

char MAP_STATION_MAPNAME[64] = { 0 };
char MAP_STATION_NAME[512] = { 0 };
int MAP_STATION_TRACKS_NUM = 0;

radioMusicList_t MAP_STATION_TRACKS[512 + 1];

void BASS_GetMapStationTracks(void)
{
	if (strcmp(MAP_STATION_MAPNAME, cl.mapname))
	{// Mapname has changed... Init...
		memset(MAP_STATION_NAME, 0, sizeof(MAP_STATION_NAME));
		memset(MAP_STATION_MAPNAME, 0, sizeof(MAP_STATION_MAPNAME));
		MAP_STATION_TRACKS_NUM = 0;
		strcpy(MAP_STATION_MAPNAME, cl.mapname);
		MAP_STATION_TRACKS_LOADED = qfalse;

		if (strlen(MAP_STATION_MAPNAME) <= 0)
		{// No map loaded... Empty list...
			MAP_STATION_TRACKS_LOADED = qtrue;
			MAP_STATION_TRACKS_NUM = 0;
			return;
		}
	}

	if (MAP_STATION_TRACKS_LOADED || strlen(MAP_STATION_MAPNAME) <= 0) return;

	char radioIniName[512] = { 0 };
	char radioMapStrippedName[64] = { 0 };
	COM_StripExtension(MAP_STATION_MAPNAME, radioMapStrippedName, sizeof(radioMapStrippedName));
	sprintf(radioIniName, "%s.radio", radioMapStrippedName);

	strcpy(MAP_STATION_NAME, IniRead(radioIniName, "STATION", "NAME", ""));

	//Com_Printf("MAP_STATION_NAME: %s. Radio file: %s.\n", MAP_STATION_NAME, radioIniName);

	if (strlen(MAP_STATION_NAME) > 0)
	{
		for (int i = 0; i < 16; i++)
		{// Load music from up to 16 specified folder names...
			char MAP_STATION_FOLDER[512] = { 0 };
			memset(MAP_STATION_FOLDER, 0, sizeof(MAP_STATION_FOLDER));

			strcpy(MAP_STATION_FOLDER, IniRead(radioIniName, "STATION", va("MUSIC_FOLDER%i", i), ""));

			//Com_Printf("MAP_STATION_FOLDER %i: %s.\n", i, MAP_STATION_FOLDER);

			// Continue until we see an empty folder name...
			if (strlen(MAP_STATION_FOLDER) > 0)
			{
				DIR *dir;
				struct dirent *ent;
				if ((dir = opendir(va("warzone/music/%s", MAP_STATION_FOLDER))) != NULL) {
					/* all the files and directories within psy directory */
					while ((ent = readdir(dir)) != NULL) {
						if (ent->d_name[0] == '.') continue; // skip back directory...
						if (ent->d_namlen < 3) continue;

						sprintf(MAP_STATION_TRACKS[MAP_STATION_TRACKS_NUM].name, "music/%s/%s", MAP_STATION_FOLDER, ent->d_name);
						//Com_Printf("Added %s track %s.\n", MAP_STATION_FOLDER, MAP_STATION_TRACKS[MAP_STATION_TRACKS_NUM].name);
						MAP_STATION_TRACKS_NUM++;
					}
					closedir(dir);
				}
				else {
					/* could not open directory */
					perror("");
				}
			}
			else
			{
				break;
			}
		}

		for (int i = 0; i < 512; i++)
		{// Load music from up to 512 specified track names...
			char MUSIC_TRACK[512] = { 0 };
			memset(MUSIC_TRACK, 0, sizeof(MUSIC_TRACK));

			strcpy(MUSIC_TRACK, IniRead(radioIniName, "STATION", va("MUSIC_TRACK%i", i), ""));

			// Continue until we see an empty track slot...
			if (strlen(MUSIC_TRACK) > 0)
			{
				if (!strncmp(MUSIC_TRACK, "music/", 6) || !strncmp(MUSIC_TRACK, "http", 4))
				{// Already has music/ or is internet radio station...
					strcpy(MAP_STATION_TRACKS[MAP_STATION_TRACKS_NUM].name, MUSIC_TRACK);
				}
				else
				{// Add music/
					sprintf(MAP_STATION_TRACKS[MAP_STATION_TRACKS_NUM].name, "music/%s", MUSIC_TRACK);
				}

				MAP_STATION_TRACKS_NUM++;
			}
			else
			{
				break;
			}
		}

		MAP_STATION_TRACKS_LOADED = qtrue;
		Com_Printf("^3BASS Sound System ^4- ^5Loaded ^7%i ^3%s^5 music tracks.\n", MAP_STATION_TRACKS_NUM, MAP_STATION_NAME);
	}
	else
	{
		MAP_STATION_TRACKS_LOADED = qtrue;
		MAP_STATION_TRACKS_NUM = 0;
		Com_Printf("^3BASS Sound System ^4- ^5Map has no custom radio stations.\n");
	}
}

//
// Imperial News Tracks...
//

qboolean NEWS_IMP_TRACKS_LOADED = qfalse;

int NEWS_IMP_TRACKS_NUM = 0;

radioMusicList_t NEWS_IMP_TRACKS[512 + 1];

void BASS_GetImpNewsTracks(void)
{
	if (NEWS_IMP_TRACKS_LOADED) return;

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir("warzone/music/news-imperial")) != NULL) {
		/* all the files and directories within directory */
		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_name[0] == '.') continue; // skip back directory...
			if (ent->d_namlen < 3) continue;

			sprintf(NEWS_IMP_TRACKS[NEWS_IMP_TRACKS_NUM].name, "music/news-imperial/%s", ent->d_name);
			//Com_Printf("Added imperial news track %s.\n", NEWS_IMP_TRACKS[NEWS_IMP_TRACKS_NUM].name);
			NEWS_IMP_TRACKS_NUM++;
		}
		closedir(dir);
	}
	else {
		/* could not open directory */
		perror("");
	}

	NEWS_IMP_TRACKS_LOADED = qtrue;
	Com_Printf("^3BASS Sound System ^4- ^5Loaded ^7%i ^3Imperial^5 news reports.\n", NEWS_IMP_TRACKS_NUM);
}

//
// Generic News Tracks...
//

qboolean NEWS_GENERIC_TRACKS_LOADED = qfalse;

int NEWS_GENERIC_TRACKS_NUM = 0;

radioMusicList_t NEWS_GENERIC_TRACKS[512 + 1];

void BASS_GetGenericNewsTracks(void)
{
	if (NEWS_GENERIC_TRACKS_LOADED) return;

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir("warzone/music/news-generic")) != NULL) {
		/* all the files and directories within directory */
		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_name[0] == '.') continue; // skip back directory...
			if (ent->d_namlen < 3) continue;

			sprintf(NEWS_GENERIC_TRACKS[NEWS_GENERIC_TRACKS_NUM].name, "music/news-generic/%s", ent->d_name);
			//Com_Printf("Added generic news track %s.\n", NEWS_GENERIC_TRACKS[NEWS_GENERIC_TRACKS_NUM].name);
			NEWS_GENERIC_TRACKS_NUM++;
		}
		closedir(dir);
	}
	else {
		/* could not open directory */
		perror("");
	}

	NEWS_GENERIC_TRACKS_LOADED = qtrue;
	Com_Printf("^3BASS Sound System ^4- ^5Loaded ^7%i ^3Generic^5 news reports.\n", NEWS_GENERIC_TRACKS_NUM);
}


//
// Imperial Advertisement Tracks...
//

qboolean IMP_ADS_TRACKS_LOADED = qfalse;

int IMP_ADS_TRACKS_NUM = 0;

radioMusicList_t IMP_ADS_TRACKS[512 + 1];

void BASS_GetImpAdsTracks(void)
{
	if (IMP_ADS_TRACKS_LOADED) return;

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir("warzone/music/ads-imperial")) != NULL) {
		/* all the files and directories within psy directory */
		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_name[0] == '.') continue; // skip back directory...
			if (ent->d_namlen < 3) continue;

			sprintf(IMP_ADS_TRACKS[IMP_ADS_TRACKS_NUM].name, "music/ads-imperial/%s", ent->d_name);
			//Com_Printf("Added mindworm track %s.\n", IMP_ADS_TRACKS[IMP_ADS_TRACKS_NUM].name);
			IMP_ADS_TRACKS_NUM++;
		}
		closedir(dir);
	}
	else {
		/* could not open directory */
		perror("");
	}

	IMP_ADS_TRACKS_LOADED = qtrue;
	Com_Printf("^3BASS Sound System ^4- ^5Loaded ^7%i ^3Imperial^5 advertisements.\n", IMP_ADS_TRACKS_NUM);
}

//
// Generic Advertisement Tracks...
//

qboolean GENERIC_ADS_TRACKS_LOADED = qfalse;

int GENERIC_ADS_TRACKS_NUM = 0;

radioMusicList_t GENERIC_ADS_TRACKS[512 + 1];

void BASS_GetGenericAdsTracks(void)
{
	if (GENERIC_ADS_TRACKS_LOADED) return;

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir("warzone/music/ads-generic")) != NULL) {
		/* all the files and directories within psy directory */
		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_name[0] == '.') continue; // skip back directory...
			if (ent->d_namlen < 3) continue;

			sprintf(GENERIC_ADS_TRACKS[GENERIC_ADS_TRACKS_NUM].name, "music/ads-generic/%s", ent->d_name);
			//Com_Printf("Added mindworm track %s.\n", GENERIC_ADS_TRACKS[GENERIC_ADS_TRACKS_NUM].name);
			GENERIC_ADS_TRACKS_NUM++;
		}
		closedir(dir);
	}
	else {
		/* could not open directory */
		perror("");
	}

	GENERIC_ADS_TRACKS_LOADED = qtrue;
	Com_Printf("^3BASS Sound System ^4- ^5Loaded ^7%i ^3Generic^5 advertisements.\n", GENERIC_ADS_TRACKS_NUM);
}



//
// Custom Dynamic Tracks...
//

qboolean CUSTOM_TRACKS_LOADED = qfalse;

int CUSTOM_TRACKS_NUM = 0;

typedef struct {
	char name[260+1];
} customMusicList_t;

customMusicList_t CUSTOM_TRACKS[512+1];

void BASS_GetCustomTracks( void )
{
	if (CUSTOM_TRACKS_LOADED) return;

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir ("warzone/music/custom")) != NULL) {
		/* all the files and directories within psy directory */
		while ((ent = readdir (dir)) != NULL) {
			if (ent->d_name[0] == '.') continue; // skip back directory...
			if (ent->d_namlen < 3) continue;

			sprintf(CUSTOM_TRACKS[CUSTOM_TRACKS_NUM].name, "music/custom/%s", ent->d_name);
			//Com_Printf("Added custom track %s.\n", CUSTOM_TRACKS[CUSTOM_TRACKS_NUM].name);
			CUSTOM_TRACKS_NUM++;
		}
		closedir (dir);
	} else {
		/* could not open directory */
		perror ("");
	}

	CUSTOM_TRACKS_LOADED = qtrue;
	Com_Printf("^3BASS Sound System ^4- ^5Loaded ^7%i ^3CUSTOM^5 music tracks.\n", CUSTOM_TRACKS_NUM);
}

//
// Dynamic music system code...
//

#define				MAX_DYNAMIC_LIST 2048

qboolean			MUSIC_LIST_INITIALIZED = qfalse;
int					MUSIC_LIST_COUNT = 0;
radioMusicList_t	MUSIC_LIST[MAX_DYNAMIC_LIST+1];

// For multithreading...
qboolean			MUSIC_LIST_UPDATING = qfalse;

int					CURRENT_MUSIC_SELECTION = 0;

void BASS_AddDynamicTrack ( char *name )
{
	if (BASS_CheckSoundDisabled()) return;

	if (!MUSIC_LIST_INITIALIZED) 
	{// Don't add custom tracks until the JKA list has been initialized...
		return;
	}

	if (CURRENT_MUSIC_SELECTION >= 1)
	{// Don't add map tracks if we are using non-jka radio tracks...
		return;
	}

	if (MUSIC_LIST_UPDATING) return; // wait...

	if (MUSIC_LIST_COUNT >= MAX_DYNAMIC_LIST) return; // Hit MAX allowed number...

	int NUM_TRACKS = GALACTIC_RADIO_TRACKS_NUM;

	for (int i = MUSIC_LIST_COUNT-1; i >= NUM_TRACKS; i--)
	{// Reverse check because "extra" music will always be at the end of the list :)
		if (!strcmp(name, MUSIC_LIST[i].name)) return; // already in the list...
	}
	
	MUSIC_LIST_UPDATING = qtrue;
	strcpy(MUSIC_LIST[MUSIC_LIST_COUNT].name, name);
	MUSIC_LIST_COUNT++;
	MUSIC_LIST_UPDATING = qfalse;
}

//
// Recently played tracks list, to stop repeats...
//
#define MUSIC_PREVIOUS_LIST_SIZE 8

qboolean MUSIC_PREVIOUS_TRACKS_INITIALIZED = qfalse;

int MUSIC_PREVIOUS_TRACKS_ADDED = 0;
int MUSIC_PREVIOUS_TRACKS[MUSIC_PREVIOUS_LIST_SIZE] = { -1 };

void BASS_MusicPlayedInit(void)
{
	MUSIC_PREVIOUS_TRACKS_ADDED = 0;
	memset(&MUSIC_PREVIOUS_TRACKS, -1, sizeof(MUSIC_PREVIOUS_TRACKS));
}

qboolean BASS_MusicWasPreviouslyPlayed(int id)
{
	for (int i = 0; i < MUSIC_PREVIOUS_TRACKS_ADDED; i++)
	{
		if (id == MUSIC_PREVIOUS_TRACKS[i])
			return qtrue;
	}

	return qfalse;
}

void BASS_MusicInsertPlayed(int id)
{
	if (MUSIC_PREVIOUS_TRACKS_ADDED >= MUSIC_PREVIOUS_LIST_SIZE)
	{
		// Remove the oldest one, and sort the current list...
		for (int i = 0; i < MUSIC_PREVIOUS_LIST_SIZE - 1; i++)
		{
			MUSIC_PREVIOUS_TRACKS[i] = MUSIC_PREVIOUS_TRACKS[i + 1];
		}

		MUSIC_PREVIOUS_TRACKS[MUSIC_PREVIOUS_LIST_SIZE] = -1;
		MUSIC_PREVIOUS_TRACKS_ADDED--;
	}

	MUSIC_PREVIOUS_TRACKS[MUSIC_PREVIOUS_TRACKS_ADDED] = id;
	MUSIC_PREVIOUS_TRACKS_ADDED++;
}

//
// Initialization of music lists...
//
void BASS_InitDynamicList ( void )
{
	if (BASS_CheckSoundDisabled()) return;

	qboolean MUSIC_SELECTION_CHANGED = qfalse;
	qboolean MAP_STATION_UPDATE = qfalse;

	if (strlen(MAP_STATION_MAPNAME) > 0 && strlen(cl.mapname) > 0 && strcmp(MAP_STATION_MAPNAME, cl.mapname) && s_musicSelection->integer == 3)
		MAP_STATION_UPDATE = qtrue;

	if (s_musicSelection->integer != CURRENT_MUSIC_SELECTION || MAP_STATION_UPDATE)
	{// Player changed music selection (or map has changed)... Initialize the music list and reload...
		BASS_StopMusic(NULL);
		MUSIC_LIST_UPDATING = qtrue;
		MUSIC_LIST_INITIALIZED = qfalse;
		MUSIC_LIST_COUNT = 0;
		memset(MUSIC_LIST, 0, sizeof(MUSIC_LIST));
		CURRENT_MUSIC_SELECTION = s_musicSelection->integer;
		MUSIC_SELECTION_CHANGED = qtrue;
	}

	if (MUSIC_LIST_INITIALIZED) 
	{// Already done!
		return;
	}

	MUSIC_LIST_UPDATING = qtrue;

	memset(MUSIC_LIST, 0, sizeof(MUSIC_LIST));

	BASS_MusicPlayedInit();

	if (CURRENT_MUSIC_SELECTION == 1)
	{// Add all known "Mind Worm Radio" tracks to the list...
		BASS_GetMindWormTracks();

		for (int i = 0; i < MINDWORM_TRACKS_NUM; i++)
		{
			strcpy(MUSIC_LIST[MUSIC_LIST_COUNT].name, MINDWORM_TRACKS[i].name);
			MUSIC_LIST_COUNT++;
		}
	}
	else if (CURRENT_MUSIC_SELECTION == 2)
	{// Add all known "Relaxing In The Rim Radio" tracks to the list...
		BASS_GetRelaxingTracks();

		for (int i = 0; i < RELAXING_TRACKS_NUM; i++)
		{
			strcpy(MUSIC_LIST[MUSIC_LIST_COUNT].name, RELAXING_TRACKS[i].name);
			MUSIC_LIST_COUNT++;
		}
	}
	else if (CURRENT_MUSIC_SELECTION == 3)
	{// Add all map station tracks...
		BASS_GetMapStationTracks();

		for (int i = 0; i < MAP_STATION_TRACKS_NUM; i++)
		{
			strcpy(MUSIC_LIST[MUSIC_LIST_COUNT].name, MAP_STATION_TRACKS[i].name);
			MUSIC_LIST_COUNT++;
		}
	}
	else if (CURRENT_MUSIC_SELECTION == 4)
	{// Add all known CUSTOM tracks to the list...
		BASS_GetCustomTracks();

		for (int i = 0; i < CUSTOM_TRACKS_NUM; i++)
		{
			strcpy(MUSIC_LIST[MUSIC_LIST_COUNT].name, CUSTOM_TRACKS[i].name);
			MUSIC_LIST_COUNT++;
		}

		if (MUSIC_LIST_COUNT <= 0)
		{// No tracks in custom dir, turn the radio off...
			Cvar_Set("s_musicSelection", "5");
			MUSIC_LIST_INITIALIZED = qtrue;
			MUSIC_LIST_UPDATING = qfalse;
			return;
		}
	}
	else if (CURRENT_MUSIC_SELECTION == 5)
	{// Radio is off...
		MUSIC_LIST_INITIALIZED = qtrue;
		MUSIC_LIST_UPDATING = qfalse;
		return;
	}
	else
	{// Add all known "Galactic Radio" tracks to the list...
		BASS_GetGalacticExtraTracks();

		// Add all known JKA tracks to the list...
		for (int i = 0; i < GALACTIC_RADIO_TRACKS_NUM; i++)
		{
			strcpy(MUSIC_LIST[MUSIC_LIST_COUNT].name, (char *)JKA_TRACKS[i]);
			MUSIC_LIST_COUNT++;
		}

		// Add the extra tracks...
		for (int i = 0; i < GALACTIC_RADIO_EXTRA_TRACKS_NUM; i++)
		{
			strcpy(MUSIC_LIST[MUSIC_LIST_COUNT].name, GALACTIC_RADIO_EXTRA_TRACKS[i].name);
			MUSIC_LIST_COUNT++;
		}

		if (MUSIC_SELECTION_CHANGED) Com_Printf("^3BASS Sound System ^4- ^5Loaded ^7%i ^3Galactic Radio^5 music tracks.\n", MUSIC_LIST_COUNT);
	}

	//
	// TODO? Keep these separate, and play 3 or 4 in a row before returning to music?
	//
	if (MUSIC_LIST_COUNT > 0)
	{
		{// Add imperial ads to all stations...
			BASS_GetImpAdsTracks();

			// Add the imperial ads...
			for (int i = 0; i < IMP_ADS_TRACKS_NUM; i++)
			{
				strcpy(MUSIC_LIST[MUSIC_LIST_COUNT].name, IMP_ADS_TRACKS[i].name);
				MUSIC_LIST_COUNT++;
			}

			//Com_Printf("^3BASS Sound System ^4- ^5Added ^7%i ^3Imperial^5 advertisements.\n", IMP_ADS_TRACKS_NUM);
		}

		{// Add generic ads to all stations...
			BASS_GetGenericAdsTracks();

			// Add the imperial ads...
			for (int i = 0; i < GENERIC_ADS_TRACKS_NUM; i++)
			{
				strcpy(MUSIC_LIST[MUSIC_LIST_COUNT].name, GENERIC_ADS_TRACKS[i].name);
				MUSIC_LIST_COUNT++;
			}

			//Com_Printf("^3BASS Sound System ^4- ^5Added ^7%i ^3Generic^5 advertisements.\n", IMP_ADS_TRACKS_NUM);
		}

		{// Add imperial news to all stations...
			BASS_GetImpNewsTracks();

			// Add the imperial ads...
			for (int i = 0; i < NEWS_IMP_TRACKS_NUM; i++)
			{
				strcpy(MUSIC_LIST[MUSIC_LIST_COUNT].name, NEWS_IMP_TRACKS[i].name);
				MUSIC_LIST_COUNT++;
			}

			//Com_Printf("^3BASS Sound System ^4- ^5Added ^7%i ^3Imperial^5 news reports.\n", IMP_ADS_TRACKS_NUM);
		}

		{// Add generic news to all stations...
			BASS_GetGenericNewsTracks();

			// Add the imperial ads...
			for (int i = 0; i < NEWS_GENERIC_TRACKS_NUM; i++)
			{
				strcpy(MUSIC_LIST[MUSIC_LIST_COUNT].name, NEWS_GENERIC_TRACKS[i].name);
				MUSIC_LIST_COUNT++;
			}

			//Com_Printf("^3BASS Sound System ^4- ^5Added ^7%i ^3Generic^5 news reports.\n", IMP_ADS_TRACKS_NUM);
		}
	}

	MUSIC_LIST_INITIALIZED = qtrue;
	MUSIC_LIST_UPDATING = qfalse;
}

#ifdef __BASS_STREAM_MUSIC__
bool BASS_IsMusicInPK3(char * filename)
{
	/*if (!strncmp(filename, "music/galactic", 9))
	{
		return false;
	}

	if (!strncmp(filename, "music/mindworm", 9))
	{
		return false;
	}

	if (!strncmp(filename, "music/relaxing", 9))
	{
		return false;
	}*/

	if (!strncmp(filename, "http", 4))
	{
		return false;
	}

	/*if (StringContainsWord(filename, "music/custom"))
	{
		return false;
	}*/

	int nChkSum1 = 0;
	qboolean inPK3 = (qboolean)(FS_FileIsInPAK(filename, &nChkSum1) == 1);
	
	if (inPK3) return true;

	return false;
}

void CALLBACK BASS_StreamMusic_StatusProc(const void *buffer, DWORD length, void *user)
{
	//if (buffer && !length && (DWORD)user==req) // got HTTP/ICY tags, and this is still the current request
	//	MESS(32,WM_SETTEXT,0,buffer); // display status
}

void BASS_StartStreamingMusic(char *filename)
{
	DWORD newchan = 0, r = 0;

	if (BASS_CheckSoundDisabled()) return;

	if (CURRENT_MUSIC_SELECTION == 5) return;

	if (!strncmp(filename, "http", 4))
	{// http request...
	 //Com_Printf("Playing [HTTP] stream %s.\n", filename);
		newchan = BASS_StreamCreateURL(filename, 0, BASS_STREAM_BLOCK | BASS_STREAM_STATUS | BASS_STREAM_AUTOFREE, BASS_StreamMusic_StatusProc, (void*)r);
	}
	else
	{// local file...
	 //Com_Printf("Playing [LOCAL] stream %s.\n", filename);
		char fullPath[1024] = { 0 };
		sprintf_s(fullPath, "warzone/%s", filename);

		//Com_Printf("Playing stream %s.\n", fullPath);

		newchan = BASS_StreamCreateFile(FALSE, fullPath, 0, -1, BASS_STREAM_AUTOFREE);
	}

	// Load a music or sample from "file"
	if (newchan) {
		// Set new samples...
		MUSIC_CHANNEL.originalChannel = MUSIC_CHANNEL.channel = newchan;
		MUSIC_CHANNEL.entityNum = -1;
		MUSIC_CHANNEL.entityChannel = CHAN_MUSIC;
		MUSIC_CHANNEL.volume = 1.0;

		// Load a music or sample from "file" (memory)
		BASS_SampleGetChannel(MUSIC_CHANNEL.channel, FALSE); // initialize sample channel
		BASS_ChannelSetAttribute(MUSIC_CHANNEL.channel, BASS_ATTRIB_VOL, MUSIC_CHANNEL.volume*BASS_GetVolumeForChannel(CHAN_MUSIC));

		// Play
		BASS_ChannelPlay(MUSIC_CHANNEL.channel, FALSE);

		// Apply the 3D settings (music is always local)...
		BASS_ChannelSet3DAttributes(MUSIC_CHANNEL.channel, SOUND_3D_METHOD, 1, 1, 1, 0, MUSIC_CHANNEL.volume*BASS_GetVolumeForChannel(CHAN_MUSIC));
		BASS_Apply3D();
	}
	else {
		//Com_Printf("Can't load file (note samples must be mono)\n");
	}
}
#endif //__BASS_STREAM_MUSIC__

void BASS_MusicUpdateThread( void * aArg )
{
	if (!MUSIC_PREVIOUS_TRACKS_INITIALIZED)
	{
		BASS_MusicPlayedInit();
		MUSIC_PREVIOUS_TRACKS_INITIALIZED = qtrue;
	}

	while (!BASS_MUSIC_UPDATE_THREAD_STOP)
	{
		if (BASS_CheckSoundDisabled())
		{
			break;
		}

		if (!FS_STARTUP_COMPLETE || !s_soundStarted || !s_allowDynamicMusic->integer || MUSIC_LIST_UPDATING)
		{// wait...
			this_thread::sleep_for(chrono::milliseconds(1000));
			continue;
		}

		BASS_InitDynamicList(); // check if we have initialized the list yet...

		if (!MUSIC_LIST_INITIALIZED) return;

		if (CURRENT_MUSIC_SELECTION == 5)
		{// Radio is off. Turn off anything playing, and just wait...
			if (BASS_ChannelIsActive(MUSIC_CHANNEL.channel) == BASS_ACTIVE_PLAYING)
			{// Still playing a track...
				BASS_StopMusic(MUSIC_CHANNEL.channel);
			}

			this_thread::sleep_for(chrono::milliseconds(1000));
			continue;
		}

		if (s_volumeMusic->value <= 0)
		{// wait...
			if (BASS_ChannelIsActive(MUSIC_CHANNEL.channel) == BASS_ACTIVE_PLAYING)
			{// Still playing a track...
				BASS_StopMusic(MUSIC_CHANNEL.channel);
			}

			this_thread::sleep_for(chrono::milliseconds(1000));
			continue;
		}

		qboolean playingNews = qfalse;

		if (StringContainsWord(MUSIC_LIST[MUSIC_PREVIOUS_TRACKS[0]].name, "news-"))
		{// Check the previous played track to see if we are currently playing a news track, if so, don't fade.
			playingNews = qtrue;
		}

		// Do we need a new track yet???
		if (BASS_ChannelIsActive(MUSIC_CHANNEL.channel) == BASS_ACTIVE_PLAYING)
		{// Still playing a track...
			QWORD length = BASS_ChannelGetLength(MUSIC_CHANNEL.channel, BASS_POS_BYTE); // get channel's length
			QWORD fadelen = length - BASS_ChannelGetPosition(MUSIC_CHANNEL.channel, BASS_POS_BYTE); // get length remaining
			fadelen /= 1000;
			fadelen /= 1000;
			fadelen *= 3;
			// What we have left now in fadelen is roughly seconds (depends on encode)... Don't want to scan for exact times (only MP3 supported), so these rough times will do...

			//Com_Printf("%i seconds left on current music track...\n", (int)fadelen);

			if (playingNews && fadelen < 2)
			{// When playing news, let it finish before playing the next track...
				this_thread::sleep_for(chrono::milliseconds(1000));
				continue;
			}
			else if (fadelen < 2)
			{// Start playing next track at the same time... Copy the finish track to a new temp channel, then reuse the main channel for the new track...
				//Com_Printf("%i seconds left on current music track... Track is fading out on its own channel. Starting new track.\n", (int)fadelen);
				memcpy(&MUSIC_CHANNEL2, &MUSIC_CHANNEL, sizeof(MUSIC_CHANNEL));
				memset(&MUSIC_CHANNEL, 0, sizeof(MUSIC_CHANNEL));
			}
			else
			{// Wait...
				this_thread::sleep_for(chrono::milliseconds(1000));
				continue;
			}
		}

		if (BASS_MUSIC_UPDATE_THREAD_STOP)
			break;

		// Seems we need a new track... Select a random one and play it!
		int trackChoice = irand(0, MUSIC_LIST_COUNT-1);

		while (BASS_MusicWasPreviouslyPlayed(trackChoice))
			trackChoice = irand(0, MUSIC_LIST_COUNT - 1);

		if (!BASS_UPDATE_THREAD_STOP)
		{
			BASS_MusicInsertPlayed(trackChoice);

			//Com_Printf("Queue music tracks %s and %s.\n", MUSIC_LIST[trackChoice].name, MUSIC_LIST[trackChoice2].name);
#ifdef __BASS_STREAM_MUSIC__
			if (!BASS_IsMusicInPK3(MUSIC_LIST[trackChoice].name))
			{
				BASS_StartStreamingMusic(MUSIC_LIST[trackChoice].name);
			}
			else
			{
				int trackChoice2 = irand(0, MUSIC_LIST_COUNT - 1);

				while (MUSIC_LIST_COUNT > 1 && trackChoice2 == trackChoice) // Try again to pick a different one if we can...
					trackChoice2 = irand(0, MUSIC_LIST_COUNT - 1);

				BASS_MusicInsertPlayed(trackChoice2);

				BASS_StopMusic(MUSIC_CHANNEL.channel);
				S_StartBackgroundTrack_Actual(MUSIC_LIST[trackChoice].name, MUSIC_LIST[trackChoice2].name);
			}
#else //!__BASS_STREAM_MUSIC__
			BASS_StopMusic(MUSIC_CHANNEL.channel);
			S_StartBackgroundTrack_Actual( MUSIC_LIST[trackChoice].name, MUSIC_LIST[trackChoice2].name );
#endif //__BASS_STREAM_MUSIC__
		}

		if (BASS_MUSIC_UPDATE_THREAD_STOP)
			break;

		this_thread::sleep_for(chrono::milliseconds(1000));
	}

	BASS_MUSIC_UPDATE_THREAD_RUNNING = qfalse;
}

void BASS_UpdateDynamicMusic( void )
{
	if (!BASS_UPDATE_THREAD_RUNNING) return; // wait...
	if (!BASS_INITIALIZED) return; // wait...
	if (CURRENT_MUSIC_SELECTION == 5) return;

	if ( thread::hardware_concurrency() > 1 )
	{
		if (!BASS_MUSIC_UPDATE_THREAD_RUNNING && !BASS_MUSIC_UPDATE_THREAD_STOP && !(!FS_STARTUP_COMPLETE || !s_soundStarted || !s_allowDynamicMusic->integer || MUSIC_LIST_UPDATING))
		{
			BASS_MUSIC_UPDATE_THREAD_RUNNING = qtrue;
			BASS_MUSIC_UPDATE_THREAD = new thread (BASS_MusicUpdateThread, 0);
		}
	}
	else if (!BASS_MUSIC_UPDATE_THREAD_STOP)
	{
		if (BASS_CheckSoundDisabled() || !s_soundStarted || !s_allowDynamicMusic->integer || MUSIC_LIST_UPDATING)
		{// wait...
			return;
		}

		BASS_InitDynamicList(); // check if we have initialized the list yet...

		if (!MUSIC_LIST_INITIALIZED) return;

		// Do we need a new track yet???
		if (BASS_ChannelIsActive(MUSIC_CHANNEL.channel) == BASS_ACTIVE_PLAYING) return; // Still playing a track...

		// Seems we need a new track... Select a random one and play it!
#ifdef __BASS_STREAM_MUSIC__
		int selection = irand(0, MUSIC_LIST_COUNT);

		if (!BASS_IsMusicInPK3(MUSIC_LIST[selection].name))
		{
			BASS_StartStreamingMusic(MUSIC_LIST[selection].name);
		}
		else
		{
			BASS_StopMusic(MUSIC_CHANNEL.channel);
			S_StartBackgroundTrack_Actual(MUSIC_LIST[selection].name, "");
		}
#else //!__BASS_STREAM_MUSIC__
		BASS_StopMusic(MUSIC_CHANNEL.channel);
		S_StartBackgroundTrack_Actual( MUSIC_LIST[irand(0, MUSIC_LIST_COUNT)].name, "" );
#endif //__BASS_STREAM_MUSIC__

		this_thread::sleep_for(chrono::milliseconds(100));
	}
}


//
// Sounds...
//

void BASS_AddMemoryLoopChannel(DWORD samplechan, int entityNum, int entityChannel, vec3_t origin, float volume, char *filename);

qboolean BASS_EntityChannelHasSpecialCullrange(int entityChannel)
{
	switch (entityChannel)
	{
	case CHAN_CULLRANGE_8192:
		return qtrue;
		break;
	case CHAN_CULLRANGE_16384:
		return qtrue;
		break;
	case CHAN_CULLRANGE_32768:
		return qtrue;
		break;
	case CHAN_CULLRANGE_65536:
		return qtrue;
		break;
	case CHAN_CULLRANGE_131072:
		return qtrue;
		break;
	default:
		break;
	}

	return qfalse;
}

float BASS_CullRangeForEntityChannel(int entityChannel)
{
	switch (entityChannel)
	{
	case CHAN_CULLRANGE_8192:
		return 8192;
		break;
	case CHAN_CULLRANGE_16384:
		return 16384;
		break;
	case CHAN_CULLRANGE_32768:
		return 32768;
		break;
	case CHAN_CULLRANGE_65536:
		return 65536;
		break;
	case CHAN_CULLRANGE_131072:
		return 131072;
		break;
	default:
		break;
	}

	return MAX_SOUND_RANGE;
}

void BASS_AddMemoryChannel ( DWORD samplechan, int inEntityNum, int entityChannel, vec3_t origin, float volume, char *filename )
{
	int entityNum = (inEntityNum == -1 || inEntityNum >= ENTITYNUM_MAX_NORMAL) ? -1 : inEntityNum;

	if (entityChannel == CHAN_AMBIENT)
	{// Do it as a looping sound instead...
		BASS_AddMemoryLoopChannel(samplechan, entityNum, entityChannel, origin, volume, filename);
		return;
	}

	if (BASS_GetVolumeForChannel(entityChannel) <= 0)
	{// Ignore completely...
		return;
	}

#ifdef __BASS_DEBUG__
	Com_Printf("BASS DEBUG: BASS_AddMemoryChannel starting sound %s (samplechan %lu).\n", filename, samplechan);
#endif //__BASS_DEBUG__

	if (BASS_CheckSoundDisabled()) return;

	float cullRange = BASS_CullRangeForEntityChannel(entityChannel);

	if (origin && Distance(cl.snap.ps.origin, origin) > cullRange)
	{
#ifdef __BASS_DEBUG__
		Com_Printf("BASS DEBUG: BASS_AddMemoryChannel starting sound %s (samplechan %lu) is out of range.\n", filename, samplechan);
#endif //__BASS_DEBUG__
		return;
	}

	int chan = BASS_FindFreeChannel();

	if (chan < 0)
	{// No channel left to play on...
		Com_Printf("BASS: No free sound channels.\n");
		return;
	}

#ifdef __BASS_DEBUG__
	Com_Printf("BASS DEBUG: BASS_AddMemoryChannel starting sound %s on new channel %lu.\n", filename, chan);
#endif //__BASS_DEBUG__

	// Load a music or sample from "file" (memory)
	Channel *c = &SOUND_CHANNELS[chan];
	c->isActive = qfalse;
	c->channel = -1;
	c->originalChannel = samplechan;
	c->entityNum = entityNum;
	c->entityChannel = entityChannel;
	c->volume = (volume <= 1.0) ? volume : Q_clamp(0.0, volume / 255.0, 1.0); // In case someone uses a 255 based int volume somewhere, convert to 0-1

	if (origin) VectorCopy(origin, c->origin);
	else VectorSet(c->origin, 0, 0, 0);

	if (c->entityChannel == CHAN_AMBIENT || c->entityChannel == CHAN_WEAPONLOCAL || c->entityChannel == CHAN_SABERLOCAL)
	{// Force all ambient sounds to local...
		VectorSet(c->origin, 0, 0, 0);
	}

	c->cullRange = cullRange;
	c->isLooping = qfalse;
	c->startRequest = qtrue;
}

void BASS_AddEfxMemoryChannel(DWORD samplechan, int inEntityNum, int entityChannel, vec3_t origin, float volume, float cullRange, char *filename)
{
	if (BASS_CheckSoundDisabled()) return;

	if (BASS_GetVolumeForChannel(entityChannel) <= 0)
	{// Ignore completely...
		return;
	}

	int entityNum = ((inEntityNum == -1 || inEntityNum >= ENTITYNUM_MAX_NORMAL) && inEntityNum < 8192) ? -1 : inEntityNum; // 8192 is fake local cgame entities...

	if (origin && Distance(cl.snap.ps.origin, origin) > cullRange)
	{
#ifdef __BASS_DEBUG__
		Com_Printf("BASS DEBUG: BASS_AddEfxMemoryChannel starting sound %s (samplechan %lu) is out of range.\n", filename, samplechan);
#endif //__BASS_DEBUG__
		return;
	}

	//
	// UQ1: Since it seems these also re-call this function to update positions, etc, run a check first...
	//
	//if (origin)
	{// If there's no origin, surely this can't be an update...
		for (int ch = 0; ch < MAX_BASS_CHANNELS; ch++)
		{
			if (SOUND_CHANNELS[ch].isActive && SOUND_CHANNELS[ch].isLooping)
			{// This is active and looping...
				if (SOUND_CHANNELS[ch].entityChannel == entityChannel
					&& SOUND_CHANNELS[ch].entityNum == entityNum
					&& SOUND_CHANNELS[ch].originalChannel == samplechan)
				{// This is our sound! Just update it (and then return)...
					Channel *c = &SOUND_CHANNELS[ch];
					if (origin) VectorCopy(origin, c->origin);
					c->volume = volume;
					//Com_Printf("BASS DEBUG: Sound position (%f %f %f) and volume (%f) updated.\n", origin[0], origin[1], origin[2], volume);
					return;
				}
			}
		}
	}

#ifdef __BASS_DEBUG__
	Com_Printf("BASS DEBUG: BASS_AddEfxMemoryChannel starting sound %s (samplechan %lu). entityNum %i. entityChannel %i.\n", filename, samplechan, entityNum, entityChannel);
#endif //__BASS_DEBUG__

	/*if (origin)
	Com_Printf("BASS DEBUG: Sound %i for entity %i channel %i position (%f %f %f) and volume (%f) added.\n", (int)samplechan, entityNum, entityChannel, origin[0], origin[1], origin[2], volume);
	else
	Com_Printf("BASS DEBUG: Sound %i for entity %i channel %i position (%f %f %f) and volume (%f) added.\n", (int)samplechan, entityNum, entityChannel, 0, 0, 0, volume);*/

	int chan = BASS_FindFreeChannel();

	if (chan < 0)
	{// No channel left to play on...
		Com_Printf("BASS: No free sound channels.\n");
		return;
	}

#ifdef __BASS_DEBUG__
	Com_Printf("BASS DEBUG: BASS_AddEfxMemoryChannel starting sound %s on new channel %lu.\n", filename, chan);
#endif //__BASS_DEBUG__

	// Load a music or sample from "file" (memory)
	Channel *c = &SOUND_CHANNELS[chan];
	c->isActive = qfalse;
	c->channel = -1;
	c->originalChannel = samplechan;
	c->entityNum = entityNum;
	c->entityChannel = entityChannel;
	c->volume = volume;

	if (origin) VectorCopy(origin, c->origin);
	else VectorSet(c->origin, 0, 0, 0);

	if (c->entityChannel == CHAN_AMBIENT || c->entityChannel == CHAN_WEAPONLOCAL || c->entityChannel == CHAN_SABERLOCAL)
	{// Force all ambient sounds to local...
		VectorSet(c->origin, 0, 0, 0);
	}

	c->cullRange = cullRange;
	c->isLooping = qtrue;
	c->startRequest = qtrue;
}

void BASS_AddMemoryLoopChannel ( DWORD samplechan, int inEntityNum, int entityChannel, vec3_t origin, float volume, char *filename)
{
	if (BASS_CheckSoundDisabled()) return;

	/*
	// Will keep these loop channels, so that when they turn volume back up, the sound is playing...
	if (BASS_GetVolumeForChannel(entityChannel) <= 0)
	{// Ignore completely...
		return;
	}
	*/

	int entityNum = ((inEntityNum == -1 || inEntityNum >= ENTITYNUM_MAX_NORMAL) && inEntityNum < 8192) ? -1 : inEntityNum; // 8192 is fake local cgame entities...

	float cullRange = BASS_CullRangeForEntityChannel(entityChannel);

	if (origin && Distance(cl.snap.ps.origin, origin) > cullRange)
	{
#ifdef __BASS_DEBUG__
		Com_Printf("BASS DEBUG: BASS_AddMemoryLoopChannel starting sound %s (samplechan %lu) is out of range.\n", filename, samplechan);
#endif //__BASS_DEBUG__
		return;
	}

	//
	// UQ1: Since it seems these also re-call this function to update positions, etc, run a check first...
	//
	//if (origin)
	{// If there's no origin, surely this can't be an update...
		for (int ch = 0; ch < MAX_BASS_CHANNELS; ch++) 
		{
			if (SOUND_CHANNELS[ch].isActive && SOUND_CHANNELS[ch].isLooping)
			{// This is active and looping...
				if (SOUND_CHANNELS[ch].entityChannel == entityChannel 
					&& SOUND_CHANNELS[ch].entityNum == entityNum
					&& SOUND_CHANNELS[ch].originalChannel == samplechan)
				{// This is our sound! Just update it (and then return)...
					Channel *c = &SOUND_CHANNELS[ch];
					if (origin) VectorCopy(origin, c->origin);
					c->volume = volume;
					//Com_Printf("BASS DEBUG: Sound position (%f %f %f) and volume (%f) updated.\n", origin[0], origin[1], origin[2], volume);
					return;
				}
			}
		}
	}

#ifdef __BASS_DEBUG__
	Com_Printf("BASS DEBUG: BASS_AddMemoryLoopChannel starting sound %s (samplechan %lu). entityNum %i. entityChannel %i.\n", filename, samplechan, entityNum, entityChannel);
#endif //__BASS_DEBUG__

	/*if (origin)
		Com_Printf("BASS DEBUG: Sound %i for entity %i channel %i position (%f %f %f) and volume (%f) added.\n", (int)samplechan, entityNum, entityChannel, origin[0], origin[1], origin[2], volume);
	else
		Com_Printf("BASS DEBUG: Sound %i for entity %i channel %i position (%f %f %f) and volume (%f) added.\n", (int)samplechan, entityNum, entityChannel, 0, 0, 0, volume);*/

	int chan = BASS_FindFreeChannel();

	if (chan < 0)
	{// No channel left to play on...
		Com_Printf("BASS: No free sound channels.\n");
		return;
	}

#ifdef __BASS_DEBUG__
	Com_Printf("BASS DEBUG: BASS_AddMemoryLoopChannel starting sound %s on new channel %lu.\n", filename, chan);
#endif //__BASS_DEBUG__

	// Load a music or sample from "file" (memory)
	Channel *c = &SOUND_CHANNELS[chan];
	c->isActive = qfalse;
	c->channel = -1;
	c->originalChannel = samplechan;
	c->entityNum = entityNum;
	c->entityChannel = entityChannel;
	c->volume = volume;

	if (origin) VectorCopy(origin, c->origin);
	else VectorSet(c->origin, 0, 0, 0);

	if (c->entityChannel == CHAN_AMBIENT || c->entityChannel == CHAN_WEAPONLOCAL || c->entityChannel == CHAN_SABERLOCAL)
	{// Force all ambient sounds to local...
		VectorSet(c->origin, 0, 0, 0);
	}

	c->cullRange = cullRange;
	c->isLooping = qtrue;
	c->startRequest = qtrue;
}

DWORD BASS_LoadMemorySample ( void *memory, int length )
{// Just load a sample into memory ready to play instantly...
	DWORD newchan;

	if (BASS_CheckSoundDisabled()) return -1;

	// Try to load the sample with the highest quality options we support...
	if ((newchan=BASS_SampleLoad(TRUE, memory, 0, (DWORD)length, (DWORD)MAX_SAMPLE_CHANNELS, BASS_SAMPLE_3D | BASS_SAMPLE_MONO | BASS_SAMPLE_OVER_VOL | BASS_FLOAT_AVAILABLE)))
	{
		return newchan;
	}

	return -1;
}

void CALLBACK BASS_Stream_StatusProc(const void *buffer, DWORD length, void *user)
{
	//if (buffer && !length && (DWORD)user==req) // got HTTP/ICY tags, and this is still the current request
	//	MESS(32,WM_SETTEXT,0,buffer); // display status
}

void BASS_StartStreamingSound ( char *filename, int entityNum, int entityChannel, vec3_t origin )
{
	DWORD newchan = 0, r = 0;

	if (BASS_CheckSoundDisabled()) return;

	/*
	// Will keep these loop channels, so that when they turn volume back up, the sound is playing...
	if (BASS_GetVolumeForChannel(entityChannel) <= 0)
	{// Ignore completely...
	return;
	}
	*/

	if (!strncmp(filename, "http", 4))
	{// http request...
		//Com_Printf("Playing [HTTP] stream %s.\n", filename);
		newchan = BASS_StreamCreateURL(filename, 0, BASS_STREAM_BLOCK | BASS_STREAM_STATUS | BASS_STREAM_AUTOFREE, BASS_Stream_StatusProc, (void*)r);
	}
	else
	{// local file...
		//Com_Printf("Playing [LOCAL] stream %s.\n", filename);
		newchan = BASS_StreamCreateFile(FALSE, filename, 0, -1, BASS_STREAM_AUTOFREE);
	}

	// Load a music or sample from "file"
	if (newchan) {
		// Load a music or sample from "file" (memory)
		BASS_SampleGetChannel(newchan, FALSE); // initialize sample channel
		BASS_ChannelSetAttribute(newchan, BASS_ATTRIB_VOL, BASS_GetVolumeForChannel(CHAN_VOICE));

		// Play
		BASS_ChannelPlay(newchan, FALSE);

		// Apply the 3D settings (music is always local)...
		BASS_ChannelSet3DAttributes(newchan, SOUND_3D_METHOD, 1, 1, 1, 0, BASS_GetVolumeForChannel(CHAN_VOICE));
		BASS_Apply3D();
	} else {
		Com_Printf("Can't load file (note samples must be mono)\n");
	}
}

