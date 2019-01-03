#include "client.h"
#include "snd_local.h"

#ifdef _WIN32
#include <ole2.h>
extern size_t GetHttpPostData(char *address, char *poststr, char *recvdata);
extern void GetHttpDownload(char *address, char *out_file);

qboolean TTS_FileExists( const char *file )
{
#if 0
	fileHandle_t hFile;
	FS_FOpenFileRead(file, &hFile, qfalse);
	
	if (hFile) {
		FS_FCloseFile(hFile);
		return qtrue;
	}

	return qfalse;
#else
	return FS_FileExists(file);
#endif
}

extern sfx_t		s_knownSfx[MAX_SFX];

void DoTextToSpeech (char* text, char *voice, int entityNum, vec3_t origin)
{
	int			i = 0;
	size_t		size = 0;
	char		USE_VOICE[64];
	char		RESPONSE[16550+1];
	char		POST_DATA[16550+1];
	qboolean	FOUND_HTTPS = qfalse;
	char		MP3_ADDRESS[512];
	int			MP3_ADDRESS_LENGTH = 0;
	qboolean	IS_CHAT = qfalse;
	qboolean	IS_CHAT2 = qfalse;
	qboolean	FILENAME_TOO_LONG = qfalse;
	char		filename[512];
	char		filename2[512];

	if (text[0] == '\0') return;

	memset(USE_VOICE, 0, sizeof(USE_VOICE));
	memset(RESPONSE, 0, sizeof(RESPONSE));
	memset(POST_DATA, 0, sizeof(POST_DATA));
	memset(MP3_ADDRESS, 0, sizeof(MP3_ADDRESS));
	memset(filename, 0, sizeof(filename));
	memset(filename2, 0, sizeof(filename2));

	sprintf(USE_VOICE, voice);

	{// Shorten the text to fit a decent filename length...
		char	SHORTENED_TEXT[64] = { 0 };

		COM_StripExtension( text, SHORTENED_TEXT, sizeof( SHORTENED_TEXT ) );
		sprintf(filename, "warzone/sound/tts/%s/%s.mp3", voice, SHORTENED_TEXT);
		sprintf(filename2, "sound/tts/%s/%s.mp3", voice, SHORTENED_TEXT);
	}

	if ( !strcmp("chatvoice", voice) ) IS_CHAT = qtrue;
	if ( text[0] == '!' ) IS_CHAT2 = qtrue;

	if ( IS_CHAT || IS_CHAT2 )
	{

	}
	else if ( TTS_FileExists( filename ) )
	{// We have a local file...
		sfxHandle_t sfxHandle = S_RegisterSound(filename);

		if (!s_knownSfx[sfxHandle].bInMemory)
			S_memoryLoad(&s_knownSfx[sfxHandle]);

		if (s_knownSfx[ sfxHandle ].bassSampleID < 0) return;

		if (S_ShouldCull((float *)origin, qfalse, entityNum))
			BASS_AddMemoryChannel(s_knownSfx[ sfxHandle ].bassSampleID, entityNum, CHAN_VOICE, (float *)origin, 0.25);
		else
			BASS_AddMemoryChannel(s_knownSfx[ sfxHandle ].bassSampleID, entityNum, CHAN_VOICE, (float *)origin, 1.0);

		return;
	}
	else if ( TTS_FileExists( filename2 ) )
	{// We have a local file...
		sfxHandle_t sfxHandle = S_RegisterSound(filename2);

		if (!s_knownSfx[sfxHandle].bInMemory)
			S_memoryLoad(&s_knownSfx[sfxHandle]);

		if (s_knownSfx[ sfxHandle ].bassSampleID < 0) return;

		if (S_ShouldCull((float *)origin, qfalse, entityNum))
			BASS_AddMemoryChannel(s_knownSfx[ sfxHandle ].bassSampleID, entityNum, CHAN_VOICE, (float *)origin, 0.25);
		else
			BASS_AddMemoryChannel(s_knownSfx[ sfxHandle ].bassSampleID, entityNum, CHAN_VOICE, (float *)origin, 1.0);

		return;
	}

#ifdef __TTS_DEVELOPER__
	if (IS_CHAT) 
		sprintf(USE_VOICE, "ryan22k"); // override the voice var with "ryan22k". So we can transmit "chatvoice" to the engine and ignore saving random chat...
	else 
		sprintf(USE_VOICE, voice);

	//Com_Printf("Voice: %s. Text: %s.\n", voice, text);

	// url: acapela_tts, snd_url: '', voice: options.avoice, listen: '1', format: 'MP3', codecMP3: '1', spd: '180', vct: '100', text: '\\vct=100\\ \\spd=180\\ ' + text
	sprintf(POST_DATA, "&voice=%s&listen=1&format=MP3&codecMP3=1&spd=180&vct=100&text=\\vct=100\\ \\spd=180\\  %s", USE_VOICE, text);
	size = GetHttpPostData("https://acapela-box.com/AcaBox/dovaas.php", POST_DATA, RESPONSE);

	//Com_Printf("Response the was:\n%s.\n", RESPONSE);

	for (i = 0; i < size; i++)
	{
		if (FOUND_HTTPS)
		{// Adding characters to address...
			if (RESPONSE[i] == '"') break; // finished address string...

			if (RESPONSE[i] == '\\') continue; // never add a \

			MP3_ADDRESS[MP3_ADDRESS_LENGTH] = RESPONSE[i];
			MP3_ADDRESS_LENGTH++;
		}
		else
		{// Looking for start of the text we want...
			if (RESPONSE[i] == 'h' && RESPONSE[i+1] == 't' && RESPONSE[i+2] == 't' && RESPONSE[i+3] == 'p' && RESPONSE[i+4] == 's')
			{
				FOUND_HTTPS = qtrue;
				MP3_ADDRESS[MP3_ADDRESS_LENGTH] = RESPONSE[i];
				MP3_ADDRESS_LENGTH++;
			}
		}
	}

	//Com_Printf("MP3 https address is: %s.\n", MP3_ADDRESS);
	
	if (!IS_CHAT && !IS_CHAT2)
	{// Download and play the new file from the new local copy...
		GetHttpDownload(MP3_ADDRESS, filename);

		if ( TTS_FileExists( filename ) )
		{// We have a local file...
			sfxHandle_t sfxHandle = S_RegisterSound(filename);

			if (!s_knownSfx[sfxHandle].bInMemory)
				S_memoryLoad(&s_knownSfx[sfxHandle]);

			if (s_knownSfx[ sfxHandle ].bassSampleID < 0) return;

			//Com_Printf("Playing TTS %s at %f %f %f.\n", filename, 

			if (S_ShouldCull((float *)origin, qfalse, entityNum))
				BASS_AddMemoryChannel(s_knownSfx[ sfxHandle ].bassSampleID, entityNum, CHAN_VOICE, (float *)origin, 0.25);
			else
				BASS_AddMemoryChannel(s_knownSfx[ sfxHandle ].bassSampleID, entityNum, CHAN_VOICE, (float *)origin, 1.0);

			return;
		}
		else if ( TTS_FileExists( filename2 ) )
		{// We have a local file...
			sfxHandle_t sfxHandle = S_RegisterSound(filename2);

			if (!s_knownSfx[sfxHandle].bInMemory)
				S_memoryLoad(&s_knownSfx[sfxHandle]);

			if (s_knownSfx[ sfxHandle ].bassSampleID < 0) return;

			if (S_ShouldCull((float *)origin, qfalse, entityNum))
				BASS_AddMemoryChannel(s_knownSfx[ sfxHandle ].bassSampleID, entityNum, CHAN_VOICE, (float *)origin, 0.25);
			else
				BASS_AddMemoryChannel(s_knownSfx[ sfxHandle ].bassSampleID, entityNum, CHAN_VOICE, (float *)origin, 1.0);

			return;
		}
	}
	else
	{// Stream chat - direct from the server...
		BASS_StartStreamingSound( MP3_ADDRESS, -1, CHAN_LOCAL, NULL );
	}
#endif //__TTS_DEVELOPER__
}

void ShutdownTextToSpeechThread ( void )
{

}

typedef struct ttsData_s
{
	char	voicename[32];		// Voice Name...
	char	text[1024];			// Text...
	int		entityNum;			// Sound entity number...
	vec3_t	origin;				// Sound origin...
} ttsData_t;

DWORD WINAPI ThreadFunc(void *ttsInfoVoid) 
{
	ttsData_t *ttsInfo = (ttsData_t*)ttsInfoVoid;
	DoTextToSpeech(ttsInfo->text, ttsInfo->voicename, ttsInfo->entityNum, ttsInfo->origin);
	free(ttsInfoVoid);
	return 1;
}

void RemoveColorEscapeSequences( char *text ) {
	int i, l;

	l = 0;
	for ( i = 0; text[i]; i++ ) {
		if (Q_IsColorStringExt(&text[i])) {
			i++;
			continue;
		}
		if (text[i] > 0x7E)
			continue;
		text[l++] = text[i];
	}
	text[l] = '\0';
}

char PREVIOUS_TALK_TEXT[16550+1];
int  PREVIOUS_TALK_TIME = 0;

void S_TextToSpeech( const char *text, const char *voice, int entityNum, float *origin )
{
	ttsData_t *ttsInfo;

	if (!voice) return; // Not initialized yet.. wait...
	if (cl.serverTime != 0 && PREVIOUS_TALK_TIME >= cl.serverTime - 1000) return;

	ttsInfo = (ttsData_t *)malloc(sizeof(ttsData_t)+1);

	// Remove color codes...
	memset(ttsInfo->voicename, '\0', sizeof(char)*32);
	strcpy(ttsInfo->voicename, voice);

	memset(ttsInfo->text, '\0', sizeof(char)*1024);
	strcpy(ttsInfo->text, text);
	RemoveColorEscapeSequences(ttsInfo->text);

	ttsInfo->entityNum = entityNum;
	VectorCopy(origin, ttsInfo->origin);

	if (strcmp(ttsInfo->text, PREVIOUS_TALK_TEXT))
	{// Never repeat...
		HANDLE thread;
		memset(PREVIOUS_TALK_TEXT, '\0', sizeof(char)*16551);
		strcpy(PREVIOUS_TALK_TEXT, ttsInfo->text);
		PREVIOUS_TALK_TIME = cl.serverTime;
		thread = CreateThread(NULL, 0, ThreadFunc, ttsInfo, 0, NULL);
	}
	else
	{
		free(ttsInfo);
	}
}

qboolean S_DownloadVoice( const char *text, const char *voice )
{
	int			i = 0;
	size_t		size = 0;
	char		USE_VOICE[64];
	char		RESPONSE[16550+1];
	char		POST_DATA[16550+1];
	qboolean	FOUND_HTTPS = qfalse;
	char		MP3_ADDRESS[512];
	int			MP3_ADDRESS_LENGTH = 0;
	qboolean	IS_CHAT = qfalse;
	qboolean	FILENAME_TOO_LONG = qfalse;
	char		filename[512];
	char		filename2[512];

	memset(USE_VOICE, 0, sizeof(USE_VOICE));
	memset(RESPONSE, 0, sizeof(RESPONSE));
	memset(POST_DATA, 0, sizeof(POST_DATA));
	memset(MP3_ADDRESS, 0, sizeof(MP3_ADDRESS));
	memset(filename, 0, sizeof(filename));
	memset(filename2, 0, sizeof(filename2));

	sprintf(USE_VOICE, voice);

	{// Shorten the text to fit a decent filename length...
		char	SHORTENED_TEXT[64] = { 0 };

		COM_StripExtension( text, SHORTENED_TEXT, sizeof( SHORTENED_TEXT ) );
		sprintf(filename, "warzone/sound/tts/%s/%s.mp3", voice, SHORTENED_TEXT);
		sprintf(filename2, "sound/tts/%s/%s.mp3", voice, SHORTENED_TEXT);
	}

	if ( TTS_FileExists( filename ) )
	{// We have a local file...
		return qtrue;
	}
	else if ( TTS_FileExists( filename2 ) )
	{// We have a local file...
		return qtrue;
	}

#ifdef __TTS_DEVELOPER__
	// url: acapela_tts, snd_url: '', voice: options.avoice, listen: '1', format: 'MP3', codecMP3: '1', spd: '180', vct: '100', text: '\\vct=100\\ \\spd=180\\ ' + text
	sprintf(POST_DATA, "&voice=%s&listen=1&format=MP3&codecMP3=1&spd=180&vct=100&text=\\vct=100\\ \\spd=180\\  %s", USE_VOICE, text);
	//Com_Printf("Filename: %s. PostData: %s.\n", filename2, POST_DATA);

	size = GetHttpPostData("https://acapela-box.com/AcaBox/dovaas.php", POST_DATA, RESPONSE);

	//Com_Printf("Response the was:\n%s.\n", RESPONSE);

	for (i = 0; i < size; i++)
	{
		if (FOUND_HTTPS)
		{// Adding characters to address...
			if (RESPONSE[i] == '"') break; // finished address string...

			if (RESPONSE[i] == '\\') continue; // never add a \

			MP3_ADDRESS[MP3_ADDRESS_LENGTH] = RESPONSE[i];
			MP3_ADDRESS_LENGTH++;
		}
		else
		{// Looking for start of the text we want...
			if (RESPONSE[i] == 'h' && RESPONSE[i+1] == 't' && RESPONSE[i+2] == 't' && RESPONSE[i+3] == 'p' && RESPONSE[i+4] == 's')
			{
				FOUND_HTTPS = qtrue;
				MP3_ADDRESS[MP3_ADDRESS_LENGTH] = RESPONSE[i];
				MP3_ADDRESS_LENGTH++;
			}
		}
	}

	//Com_Printf("MP3 https address is: %s.\n", MP3_ADDRESS);
	
	// Download the new file...
	GetHttpDownload(MP3_ADDRESS, filename);

	if ( TTS_FileExists( filename ) || TTS_FileExists( filename2 ) ) 
		return qtrue; // it worked...
#endif //__TTS_DEVELOPER__

	return qfalse; // it failed...
}

#endif //_WIN32
