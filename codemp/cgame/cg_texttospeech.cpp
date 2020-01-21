#include "cg_local.h"

//#define __OTHER_LANGUAGES_SUPPORT__ // disable (comment out) this when your finished...

//
// TTS Voice Types Tracking Stuff...
//
typedef enum
{
	TTS_GENDER_NONE,
	TTS_GENDER_MALE,
	TTS_GENDER_FEMALE,
	TTS_GENDER_DROID,
	TTS_GENDER_YODA,
	TTS_GENDER_IMPERIAL_OFFICER,
	TTS_GENDER_BOUNTY_HUNTER_MALE,
	TTS_GENDER_BOUNTY_HUNTER_FEMALE,
	TTS_GENDER_EVIL_MALE,
	TTS_GENDER_EVIL_FEMALE,
	TTS_GENDER_MAX
} genders_t;

typedef enum
{
	TTS_AGE_NONE, // Only use to mark the 1st record in the list!
	TTS_AGE_ADULT,
	TTS_AGE_OLD,
	TTS_AGE_CHILD,
	TTS_AGE_MAX
} TTS_AGE_t;

typedef struct ttsVoiceData_s
{
	char	voicename[32+1];		// Voice Name
	char	description[128+1];	// Full Voice Description
	int		gender;
	int		age;
} ttsVoiceData_t;

//#define __OTHER_LANGUAGES_SUPPORT__ // UQ1: I can't be bothered with other languages. Too much work for now...

ttsVoiceData_t ttsVoiceData[] = {
	// char	voicename[32];	char description[128], char	gender[2], char	age[32]
	"unused", "unused voice", TTS_GENDER_NONE, TTS_AGE_NONE,
#ifdef __OTHER_LANGUAGES_SUPPORT__
	"leila22k", "Arabic (SA) - Leila", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"mehdi22k", "Arabic (SA) - Mehdi", TTS_GENDER_MALE, TTS_AGE_ADULT,
	"nizar22k", "Arabic (SA) - Nizar", TTS_GENDER_MALE, TTS_AGE_ADULT,
	"salma22k", "Arabic (SA) - Salma", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"laia22k", "Catalan - Laia", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"lulu22k", "Chinese (Mandarin) - Lulu", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"eliska22k", "Czech - Eliska", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"mette22k", "Danish - Mette", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"rasmus22k", "Danish - Rasmus", TTS_GENDER_MALE, TTS_AGE_ADULT,
	"jeroen22k", "Dutch (BE) - Jeroen", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"jeroenhappy22k", "Dutch (BE) - Jeroen (Happy)", TTS_GENDER_MALE, TTS_AGE_ADULT,
	"jeroensad22k", "Dutch (BE) - Jeroen (Sad)", TTS_GENDER_MALE, TTS_AGE_ADULT,
	"sofie22k", "Dutch (BE) - Sofie", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"zoe22k", "Dutch (BE) - Zoe", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"daan22k", "Dutch (NL) - Daan", TTS_GENDER_MALE, TTS_AGE_ADULT,
	"femke22k", "Dutch (NL) - Femke", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"jasmijn22k", "Dutch (NL) - Jasmijn", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"max22k", "Dutch (NL) - Max", TTS_GENDER_MALE, TTS_AGE_ADULT,
	"nizareng22k", "English (Arabic) - Nizar", TTS_GENDER_MALE, TTS_AGE_ADULT,
#endif //__OTHER_LANGUAGES_SUPPORT__
	"liam22k", "English (Australia) - Liam (Child) - Premium", TTS_GENDER_MALE, TTS_AGE_CHILD,
	"lisa22k", "English (Australia) - Lisa", TTS_GENDER_BOUNTY_HUNTER_FEMALE, TTS_AGE_ADULT,
	"olivia22k", "English (Australia) - Olivia (Child) - Premium", TTS_GENDER_FEMALE, TTS_AGE_CHILD,
	"tyler22k", "English (Australia) - Tyler", TTS_GENDER_BOUNTY_HUNTER_MALE, TTS_AGE_ADULT,
#ifdef __OTHER_LANGUAGES_SUPPORT__ // UQ1: Hmm... Might still use this one later...
	"deepa22k", "English (India) - Deepa", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
#endif //__OTHER_LANGUAGES_SUPPORT__
	"graham22k", "English (UK) - Graham", TTS_GENDER_IMPERIAL_OFFICER, TTS_AGE_ADULT,
	"harry22k", "English (UK) - Harry (Child) - Premium", TTS_GENDER_MALE, TTS_AGE_CHILD,
	"lucy22k", "English (UK) - Lucy", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"peter22k", "English (UK) - Peter", TTS_GENDER_IMPERIAL_OFFICER, TTS_AGE_ADULT,
	"peterhappy22k", "English (UK) - Peter (Happy)", TTS_GENDER_IMPERIAL_OFFICER, TTS_AGE_ADULT,
	"petersad22k", "English (UK) - Peter (Sad)", TTS_GENDER_IMPERIAL_OFFICER, TTS_AGE_ADULT,
	"queenelizabeth22k", "English (UK) - Queen Elizabeth", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"rachel22k", "English (UK) - Rachel", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"rosie22k", "English (UK) - Rosie (Child) - Premium", TTS_GENDER_FEMALE, TTS_AGE_CHILD,
	"ella22k", "English (US) - Ella (Child) - Premium", TTS_GENDER_FEMALE, TTS_AGE_CHILD,
	"heather22k", "English (US) - Heather", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"josh22k", "English (US) - Josh (Child) - Premium", TTS_GENDER_MALE, TTS_AGE_CHILD,
	"kenny22k", "English (US) - Kenny", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"laura22k", "English (US) - Laura", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	//"micah22k", "English (US) - Micah", TTS_GENDER_MALE, TTS_AGE_ADULT, // UQ1: Sounds too robotic...
	"nelly22k", "English (US) - Nelly", TTS_GENDER_FEMALE, TTS_AGE_CHILD,
	"rod22k", "English (US) - Rod", TTS_GENDER_MALE, TTS_AGE_ADULT,
	"ryan22k", "English (US) - Ryan", TTS_GENDER_MALE, TTS_AGE_ADULT,
	"saul22k", "English (US) - Saul", TTS_GENDER_MALE, TTS_AGE_ADULT,
	"scott22k", "English (US) - Scott (Child) - Premium", TTS_GENDER_MALE, TTS_AGE_CHILD,
	"tracy22k", "English (US) - Tracy", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"will22k", "English (US) - Will", TTS_GENDER_MALE, TTS_AGE_ADULT,
	"willbadguy22k", "English (US) - Will (BadGuy)", TTS_GENDER_EVIL_MALE, TTS_AGE_ADULT,
	"willfromafar22k", "English (US) - Will (FromAfar)", TTS_GENDER_MALE, TTS_AGE_ADULT,
	"willhappy22k", "English (US) - Will (Happy)", TTS_GENDER_MALE, TTS_AGE_ADULT,
	"willlittlecreature22k", "English (US) - Will (LittleCreature)", TTS_GENDER_YODA, TTS_AGE_ADULT,
	"willoldman22k", "English (US) - Will (Old Man)", TTS_GENDER_MALE, TTS_AGE_OLD,
	"willsad22k", "English (US) - Will (Sad)", TTS_GENDER_MALE, TTS_AGE_ADULT,
	"willupclose22k", "English (US) - Will (UpClose)", TTS_GENDER_MALE, TTS_AGE_ADULT,
#ifdef __OTHER_LANGUAGES_SUPPORT__
	"sanna22k", "Finnish - Sanna", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"justine22k", "French (BE) - Justine", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"louise22k", "French (Canada) - Louise", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"alice22k", "French - Alice", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"antoine22k", "French - Antoine", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"antoinefromafar22k", "French - Antoine (FromAfar)", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"antoinehappy22k", "French - Antoine (Happy)", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"antoinesad22k", "French - Antoine (Sad)", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"antoineupclose22k", "French - Antoine (UpClose)", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"bruno22k", "French - Bruno", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"claire22k", "French - Claire", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"julie22k", "French - Julie", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"margaux22k", "French - Margaux", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"margauxhappy22k", "French - Margaux (Happy)", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"margauxsad22k", "French - Margaux (Sad)", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"andreas22k", "German - Andreas", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"jonas22k", "German - Jonas (Child) - Premium", TTS_GENDER_FEMALE, TTS_AGE_CHILD,
	"julia22k", "German - Julia", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"klaus22k", "German - Klaus", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"lea22k", "German - Lea (Child) - Premium", TTS_GENDER_FEMALE, TTS_AGE_CHILD,
	"sarah22k", "German - Sarah", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"dimitris22k", "Greek - Dimitris", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"dimitrishappy22k", "Greek - Dimitris (Happy)", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"dimitrissad22k", "Greek - Dimitris (Sad)", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"chiara22k", "Italian - Chiara", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"fabiana22k", "Italian - Fabiana", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"vittorio22k", "Italian - Vittorio", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"sakura22k", "Japanese - Sakura", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"minji22k", "Korean - Minji", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"bente22k", "Norwegian - Bente", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"kari22k", "Norwegian - Kari", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"olav22k", "Norwegian - Olav", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"ania22k", "Polish - Ania", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"monika22k", "Polish - Monika", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"marcia22k", "Portuguese (Brazil) - Marcia", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"celia22k", "Portuguese - Celia", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"alyona22k", "Russian - Alyona", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"rodrigo22k", "Spanish (US) - Rodrigo", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"rosa22k", "Spanish (US) - Rosa", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"antonio22k", "Spanish - Antonio", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"ines22k", "Spanish - Ines", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"maria22k", "Spanish - Maria", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"samuel22k", "Swedish (Finland) - Samuel", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"kal22k", "Swedish (Gothenburg) - Kal", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"mia22k", "Swedish (Scania) - Mia", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"elin22k", "Swedish - Elin", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"emil22k", "Swedish - Emil", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"emma22k", "Swedish - Emma", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"erik22k", "Swedish - Erik", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
	"ipek22k", "Turkish - Ipek", TTS_GENDER_FEMALE, TTS_AGE_ADULT,
#endif //__OTHER_LANGUAGES_SUPPORT__
	"", "", TTS_GENDER_MAX, TTS_AGE_MAX // Marks the end of the list...
};

int TTS_VOICES_MAX = -1;

int GetTTSVoicesMax()
{
	int max = 0;

	if (TTS_VOICES_MAX != -1) return TTS_VOICES_MAX; // already set up...

	// We need to count them...
	while (ttsVoiceData[max].age != TTS_AGE_MAX)
	{
		max++;
	}

	TTS_VOICES_MAX = max;
	return max;
}

//
// Utility functions...
//
clientInfo_t *CG_GetClientInfoForEnt(centity_t *ent)
{
	clientInfo_t	*ci = NULL;

	if (ent->currentState.number < MAX_CLIENTS)
	{
		ci = &cgs.clientinfo[ent->currentState.number];
	}
	else
	{
		ci = ent->npcClient;
	}
	
	return ci;
}

//
// Stuff to select the best possible TTS voice types...
//
qboolean CG_IsSith(centity_t *ent, team_t team)
{
	clientInfo_t	*ci = CG_GetClientInfoForEnt(ent);

	if (team == FACTION_EMPIRE && ent->currentState.primaryWeapon == WP_SABER) 
		return qtrue;

	if (!ci) return qfalse;

	if (ci->modelName && ci->modelName[0])
	{
		if (StringContainsWord(ci->modelName, "sith"))
			return qtrue;
	}

	return qfalse;
}

qboolean CG_IsPadawan(centity_t *ent)
{
	clientInfo_t	*ci = CG_GetClientInfoForEnt(ent);

	if (ent->currentState.eType == ET_NPC)
	{// NPC handling...
		if (ent->currentState.NPC_class == CLASS_PADAWAN)
			return qtrue;
	}

	if (!ci) return qfalse;

	if (ci->modelName && ci->modelName[0])
	{
		if (StringContainsWord(ci->modelName, "padawan") || StringContainsWord(ci->modelName, "roxas"))
			return qtrue;

		if (StringContainsWord(ci->modelName, "youngling"))
			return qtrue;
	}

	return qfalse;
}

qboolean CG_IsImperialOfficer(centity_t *ent)
{
	clientInfo_t	*ci = CG_GetClientInfoForEnt(ent);

	if (ent->currentState.eType == ET_NPC)
	{// NPC handling...
		switch (ent->currentState.NPC_class)
		{
			case CLASS_IMPERIAL:
				return qtrue;
				break;
			default:
				break;
		}
	}

	if (!ci) return qfalse;

	if (ci->modelName && ci->modelName[0])
	{
		if (StringContainsWord(ci->modelName, "imperial"))
			return qtrue;
	}

	return qfalse;
}

qboolean CG_IsBountyHunter(centity_t *ent)
{
	clientInfo_t	*ci = CG_GetClientInfoForEnt(ent);

	if (ent->currentState.eType == ET_NPC)
	{// NPC handling...
		switch (ent->currentState.NPC_class)
		{
			case CLASS_BOBAFETT:
				return qtrue;
				break;
			default:
				break;
		}
	}

	if (!ci) return qfalse;

	if (ci->modelName && ci->modelName[0])
	{
		if (StringContainsWord(ci->modelName, "boba") || StringContainsWord(ci->modelName, "bounty"))
			return qtrue;
	}

	return qfalse;
}

qboolean CG_IsYoda(centity_t *ent)
{
	clientInfo_t	*ci = CG_GetClientInfoForEnt(ent);

	if (!ci) return qfalse;

	if (ci->modelName && ci->modelName[0])
	{
		if (StringContainsWord(ci->modelName, "yoda"))
			return qtrue;
	}

	return qfalse;
}

qboolean CG_TextToSpeechVoiceValid(centity_t *ent)
{// This checks if the current selected_voice is still valid for this NPC/Player/etc... Returns qfalse if it is not...
	if (ent->selected_voice)
	{
		clientInfo_t	*ci = CG_GetClientInfoForEnt(ent);
		int				SELECTED_GENDER = TTS_GENDER_NONE;
		int				SELECTED_AGE = TTS_AGE_NONE;

		// Select best gender for this entity...
		if (ent->playerState->extra_flags & EXF_GENDER_DROID)
		{
			SELECTED_GENDER = TTS_GENDER_DROID;
		}
		else if (CG_IsYoda(ent))
		{
			SELECTED_GENDER = TTS_GENDER_YODA;
		}
		else if (CG_IsImperialOfficer(ent))
		{// Special case... Use brittish voice for all imperial officers (like the movies - lol)... :)
			SELECTED_GENDER = TTS_GENDER_IMPERIAL_OFFICER; // All imperial officers are male. We don't have a female model, and i've never seen one anyway...
		}
		else if (CG_IsBountyHunter(ent))
		{// Use australian accent - closest thing to the new zealand accent from the movies...
			if (ci->gender == GENDER_FEMALE)
			{
				SELECTED_GENDER = TTS_GENDER_BOUNTY_HUNTER_FEMALE;
			}
			else
			{
				SELECTED_GENDER = TTS_GENDER_BOUNTY_HUNTER_MALE;
			}
		}
		else if (!ci)
		{// Will assume male...
			SELECTED_GENDER = TTS_GENDER_MALE;
		}
		else if (ci->gender == GENDER_MALE) 
		{
			if (CG_IsSith(ent, ci->team))
			{
				SELECTED_GENDER = TTS_GENDER_EVIL_MALE;
			}
			else
			{
				SELECTED_GENDER = TTS_GENDER_MALE;
			}
		}
		else if (ci->gender == GENDER_FEMALE) 
		{
			if (CG_IsSith(ent, ci->team))
			{
				SELECTED_GENDER = TTS_GENDER_EVIL_MALE;
			}
			else
			{
				SELECTED_GENDER = TTS_GENDER_FEMALE;
			}
		}
		else if (ci->gender == GENDER_NEUTER) 
		{
			SELECTED_GENDER = TTS_GENDER_DROID; // assume droid...
		}

		// Select the best age group for this entity... -- TODO: Old people npc class check??? Non-human (monster) check???
		if (CG_IsPadawan(ent))
		{
			SELECTED_AGE = TTS_AGE_CHILD;
		}
		else
		{
			SELECTED_AGE = TTS_AGE_ADULT;
		}

		if (ttsVoiceData[ent->selected_voice].age == SELECTED_AGE && ttsVoiceData[ent->selected_voice].gender == SELECTED_GENDER)
		{// Voice is valid...
			return qtrue;
		}
	}

	return qfalse;
}

char *CG_GetTextToSpeechVoiceForEntity(centity_t *ent)
{
	if (CG_TextToSpeechVoiceValid(ent))
	{// Already have a voice... Use it...
		return ttsVoiceData[ent->selected_voice].voicename;
	}
	else
	{// Find a new voice for this character...
		clientInfo_t	*ci = CG_GetClientInfoForEnt(ent);
		int				SELECTED_GENDER = TTS_GENDER_NONE;
		int				SELECTED_AGE = TTS_AGE_NONE;
		int				BEST_VOICES_NUM = 0;
		int				BEST_VOICES[128];
		int				i;

		// Init anyway in case we somehow fail...
		ent->selected_voice = 0;

		// Select best gender for this entity...
		if (ci->gender == GENDER_DROID)
		{
			SELECTED_GENDER = TTS_GENDER_DROID;
		}
		else if (CG_IsYoda(ent))
		{
			SELECTED_GENDER = TTS_GENDER_YODA;
		}
		else if (CG_IsImperialOfficer(ent))
		{// Special case... Use brittish voice for all imperial officers (like the movies - lol)... :)
			SELECTED_GENDER = TTS_GENDER_IMPERIAL_OFFICER; // All imperial officers are male. We don't have a female model, and i've never seen one anyway...
		}
		else if (CG_IsBountyHunter(ent))
		{// Use australian accent - closest thing to the new zealand accent from the movies...
			if (ci->gender == GENDER_FEMALE)
			{
				SELECTED_GENDER = TTS_GENDER_BOUNTY_HUNTER_FEMALE;
			}
			else
			{
				SELECTED_GENDER = TTS_GENDER_BOUNTY_HUNTER_MALE;
			}
		}
		else if (!ci)
		{// Will assume male...
			SELECTED_GENDER = TTS_GENDER_MALE;
		}
		else if (ci->gender == GENDER_MALE) 
		{
			if (CG_IsSith(ent, ci->team))
			{
				SELECTED_GENDER = TTS_GENDER_EVIL_MALE;
			}
			else
			{
				SELECTED_GENDER = TTS_GENDER_MALE;
			}
		}
		else if (ci->gender == GENDER_FEMALE) 
		{
			if (CG_IsSith(ent, ci->team))
			{
				SELECTED_GENDER = TTS_GENDER_EVIL_MALE;
			}
			else
			{
				SELECTED_GENDER = TTS_GENDER_FEMALE;
			}
		}
		else if (ci->gender == GENDER_NEUTER) 
		{
			SELECTED_GENDER = TTS_GENDER_DROID; // assume droid...
		}

		// Select the best age group for this entity... -- TODO: Old people npc class check??? Non-human (monster) check???
		if (CG_IsPadawan(ent))
		{
			SELECTED_AGE = TTS_AGE_CHILD;
		}
		else
		{
			SELECTED_AGE = TTS_AGE_ADULT;
		}

		// Now that we have found a gender and age to use, go through the full voices list and make a short list to select a voice from...
		for (i = 0; i < GetTTSVoicesMax(); i++)
		{// Create BEST voices list...
			if (ttsVoiceData[i].gender == SELECTED_GENDER && ttsVoiceData[i].age == SELECTED_AGE)
			{// Perfect... Add to the BEST list...
				BEST_VOICES[BEST_VOICES_NUM] = i;
				BEST_VOICES_NUM++;
			}
		}

		if (BEST_VOICES_NUM > 0)
		{// Found some, select one at random...
			ent->selected_voice = BEST_VOICES[irand(0, BEST_VOICES_NUM-1)];
		}
		else
		{
			ent->selected_voice = 0;
		}

		/*
		if (ci && ent->selected_voice)
		{
			trap->Print("%s selected TTS voice %s.\n", ci->name, ttsVoiceData[ent->selected_voice].voicename);
		}
		else if (ent->selected_voice)
		{
			trap->Print("(unknown) selected TTS voice %s.\n", ttsVoiceData[ent->selected_voice].voicename);
		}
		else if (ci && !ent->selected_voice)
		{
			trap->Print("%s could not find a TTS voice for age %i gender %i.. Using default.\n", ci->name, SELECTED_AGE, SELECTED_GENDER);
			ent->selected_voice = 1;
		}
		else if (!ent->selected_voice)
		{
			trap->Print("(unknown) could not find a TTS voice for age %i gender %i. Using default.\n", ci->name, SELECTED_AGE, SELECTED_GENDER);
			ent->selected_voice = 1;
		}
		*/
	}

	// Failed to find a matching voice... No TTS support!
	if (!ent->selected_voice) return NULL;
	
	// All good... Return the voice...
	return ttsVoiceData[ent->selected_voice].voicename;
}

//
// TTS Generic functions...
//
void TextToSpeech( const char *text, const char *voice, int entityNum, vec3_t origin )
{// UQ1: Now uses a trap call to do all the good stuff in engine (client) code...
	// Note text send starting with the character ! will never be cached/saved...
	trap->S_TextToSpeech(text, voice, entityNum, (float *)origin);
}

extern void CG_ChatBox_AddString(char *chatStr);

void CHATTER_TextToSpeech( const char *text, const char *voice, int entityNum, vec3_t origin, qboolean add_chat_text )
{
	clientInfo_t	*ci = CG_GetClientInfoForEnt(&cg_entities[entityNum]);
	char			chatline_text[MAX_SAY_TEXT] = {0};

	if (text[0] == '\0') return; // hmm somehow this can happen... no point wasting time...

	if (ci && add_chat_text)
	{
		Com_sprintf( chatline_text, sizeof( chatline_text ), "^7%s: ^2%s.", ci->name, text );
		CG_ChatBox_AddString( (char *)chatline_text );
		trap->Print( "*%s\n", chatline_text );
	}

	TextToSpeech(text, voice, entityNum, origin);
}

void CG_SaySillyTextTest ( void )
{
#ifdef _WIN32
	int choice = irand(0,10);

	switch (choice)
	{
	case 1:
		CHATTER_TextToSpeech("!What the are you doing???", CG_GetTextToSpeechVoiceForEntity(&cg_entities[cg.clientNum]), cg.clientNum, cg.snap->ps.origin, qfalse);
		break;
	case 2:
		CHATTER_TextToSpeech("!Stop that!", CG_GetTextToSpeechVoiceForEntity(&cg_entities[cg.clientNum]), cg.clientNum, cg.snap->ps.origin, qfalse);
		break;
	case 3:
		CHATTER_TextToSpeech("!Hay, stop it!", CG_GetTextToSpeechVoiceForEntity(&cg_entities[cg.clientNum]), cg.clientNum, cg.snap->ps.origin, qfalse);
		break;
	case 4:
		CHATTER_TextToSpeech("!Get away from me!", CG_GetTextToSpeechVoiceForEntity(&cg_entities[cg.clientNum]), cg.clientNum, cg.snap->ps.origin, qfalse);
		break;
	case 5:
		CHATTER_TextToSpeech("!How much wood wood a wood chuck chuck if a wood chuck could chuck wood?", CG_GetTextToSpeechVoiceForEntity(&cg_entities[cg.clientNum]), cg.clientNum, cg.snap->ps.origin, qfalse);
		break;
	case 6:
		CHATTER_TextToSpeech("!What are you doing?", CG_GetTextToSpeechVoiceForEntity(&cg_entities[cg.clientNum]), cg.clientNum, cg.snap->ps.origin, qfalse);
		break;
	case 7:
		CHATTER_TextToSpeech("!Dont talk to me.", CG_GetTextToSpeechVoiceForEntity(&cg_entities[cg.clientNum]), cg.clientNum, cg.snap->ps.origin, qfalse);
		break;
	case 8:
		CHATTER_TextToSpeech("!Go away!", CG_GetTextToSpeechVoiceForEntity(&cg_entities[cg.clientNum]), cg.clientNum, cg.snap->ps.origin, qfalse);
		break;
	case 9:
		CHATTER_TextToSpeech("!Ouch! That hurt!", CG_GetTextToSpeechVoiceForEntity(&cg_entities[cg.clientNum]), cg.clientNum, cg.snap->ps.origin, qfalse);
		break;
	default:
		CHATTER_TextToSpeech("!Oh meye!", CG_GetTextToSpeechVoiceForEntity(&cg_entities[cg.clientNum]), cg.clientNum, cg.snap->ps.origin, qfalse);
		break;
	}
#endif //_WIN32
}

void TTS_SayText ( void )
{
	char	str[MAX_TOKEN_CHARS];
	char	str2[MAX_TOKEN_CHARS];
	
	if ( trap->Cmd_Argc() < 2 )
	{
		trap->Print( "^4*** ^3TTS^4: ^7Usage:\n" );
		trap->Print( "^4*** ^3TTS^4: ^3/tts \"text\"^5.\n" );
		return;
	}

	trap->Cmd_Argv( 1, str, sizeof(str) );

	sprintf(str2, "!%s", str); // do not cache...

	CHATTER_TextToSpeech(str2, CG_GetTextToSpeechVoiceForEntity(&cg_entities[cg.clientNum]), cg.clientNum, cg.snap->ps.origin, qtrue);
}

//
// Padawan Combat Chatters...
//
const char *PADAWAN_COMBAT_TALKS[] =
{
	// These ones are pre-recorded emotions...
	"This is fun!",
	"#LAUGH01#",
	"#LAUGH02#",
	"#LAUGH03#",
	"#LION#",
	"#WOLF#",
	"#BREATH01#",
	"#BREATH02#",
	"#BREATH03#",
	"#AARGH01#",
	"#AARGH02#",
	"#AARGH03#",
	"#PIG#",
	"Whoa!",
	"Stupid!",
	"Shit!",
	"Piss off!",
	"Loser!",
	"Leave me alone!",
	"Jerk!",
	"Impressive!",
	"Idiot!",
	"You Idiot!",
	"I like you!",
	//"I hate you!",
	"Hurry up!",
	"Hurry up! I'm busting to pee!",
	"Hilarious!",
	"Go away!",
	"Bugger off!",
	"Bravo!",
	"Wow!",
	"Whatever!",
	"What!",
	"Wait a minute!",
	"Uh oh!",
	"That is so funny!",
	"Surprise!",
	"Stop!",
	"Stop it!",
	"Seriously!",
	"Go away!",
	"Ouch!",
	"Ouch! It really hurts!",
	"",
};

int PADAWAN_COMBAT_TALKS_MAX = -1;

int GetPadawanCombatChattersMax()
{
	int total = 0;

	if (PADAWAN_COMBAT_TALKS_MAX != -1) return PADAWAN_COMBAT_TALKS_MAX; // already set up...

	// We need to count them...
	while (strncmp(PADAWAN_COMBAT_TALKS[total], "", strlen(PADAWAN_COMBAT_TALKS[total])))
	{
		total++;
	}

	PADAWAN_COMBAT_TALKS_MAX = total;
	return total;
}

void CG_PadawanCombatChatter ( int entityNum )
{
	int choice = irand(0,GetPadawanCombatChattersMax());
	centity_t *ent = &cg_entities[entityNum];

	if (!ent) return;
	if (ent->currentState.eType != ET_NPC) return;
	if (ent->currentState.NPC_class != CLASS_PADAWAN) return;

	CHATTER_TextToSpeech(PADAWAN_COMBAT_TALKS[choice], CG_GetTextToSpeechVoiceForEntity(ent), entityNum, ent->lerpOrigin, (qboolean)(Q_stricmpn(PADAWAN_COMBAT_TALKS[choice], "#", 1)));
}

//
// Padawan Combat Kill Chatters...
//
const char *PADAWAN_COMBAT_KILL_TALKS[] =
{
	// These ones are pre-recorded emotions...
	"#LAUGH01#",
	"#LAUGH02#",
	"#LAUGH03#",
	"#LION#",
	"#BREATH01#",
	"#BREATH02#",
	"#BREATH03#",
	"#AARGH01#",
	"#AARGH02#",
	"#AARGH03#",
	"Whoa!",
	"Ta da!",
	"Stupid!",
	"See you!",
	"See you later!",
	"Screw you!",
	"Shit!",
	"Piss off!",
	"Perfect!",
	"Oops!",
	"Not again!",
	"Loser!",
	"Later, alligator!",
	"Jerk!",
	"It's about time!",
	"Incredible!",
	"I win!",
	"Hooray!",
	"Have a nice day!",
	"Ha!",
	"Haha!",
	"Good night!",
	"Damn!",
	"Cool!",
	"Bye Bye!",
	"Bingo!",
	"Amazing!",
	"Ah hah!",
	"You're welcome!",
	"You lose!",
	"Yes!",
	"Yeah!",
	"Yahoo!",
	"Wow!",
	"Wonderful!",
	"Wicked!",
	"Whoops!",
	"Whatever!",
	"Well!",
	"That is so funny!",
	"Thanks!",
	"Thank you!",
	"Surprise!",
	"Sorry!",
	"",
};

int PADAWAN_COMBAT_KILL_TALKS_MAX = -1;

int GetPadawanCombatKillChattersMax()
{
	int total = 0;

	if (PADAWAN_COMBAT_KILL_TALKS_MAX != -1) return PADAWAN_COMBAT_KILL_TALKS_MAX; // already set up...

	// We need to count them...
	while (strncmp(PADAWAN_COMBAT_KILL_TALKS[total], "", strlen(PADAWAN_COMBAT_KILL_TALKS[total])))
	{
		total++;
	}

	PADAWAN_COMBAT_KILL_TALKS_MAX = total;
	return total;
}

void CG_PadawanCombatKillChatter ( int entityNum )
{
	int choice = irand(0,GetPadawanCombatKillChattersMax());
	centity_t *ent = &cg_entities[entityNum];

	if (!ent) return;
	if (ent->currentState.eType != ET_NPC) return;
	if (ent->currentState.NPC_class != CLASS_PADAWAN) return;

	CHATTER_TextToSpeech(PADAWAN_COMBAT_KILL_TALKS[choice], CG_GetTextToSpeechVoiceForEntity(ent), entityNum, ent->lerpOrigin, (qboolean)(Q_stricmpn(PADAWAN_COMBAT_KILL_TALKS[choice], "#", 1)));
}

//
// Padawan No-Reply Chatters...
//
const char *PADAWAN_NO_REPLY_CHATTERS[] =
{
	// These ones are pre-recorded emotions...
	"This is fun!",
	"#LAUGH01#",
	"#LAUGH02#",
	"#LAUGH03#",
	"#YAWN01#",
	"#YAWN02#",
	"#COUGH01#",
	"#COUGH02#",
	"#SLEEP01#",
	"#SLEEP02#",
	"#SNEEZE01#",
	"#SNEEZE02#",
	"#THROAT01#",
	"#THROAT02#",
	"#THROAT03#",
	"#WHISTLE01#",
	"#WHISTLE02#",
	"Hurry up!",
	"",
};

int PADAWAN_NO_REPLY_CHATTERS_MAX = -1;

int GetPadawanNoReplyChattersMax()
{
	int total = 0;

	if (PADAWAN_NO_REPLY_CHATTERS_MAX != -1) return PADAWAN_NO_REPLY_CHATTERS_MAX; // already set up...

	// We need to count them...
	while (strncmp(PADAWAN_NO_REPLY_CHATTERS[total], "", strlen(PADAWAN_NO_REPLY_CHATTERS[total])))
	{
		total++;
	}

	PADAWAN_NO_REPLY_CHATTERS_MAX = total;
	return total;
}

void CG_PadawanIdleNoReplyChatter ( int entityNum )
{
	int choice = irand(0,GetPadawanNoReplyChattersMax());
	centity_t *ent = &cg_entities[entityNum];

	if (!ent) return;
	if (ent->currentState.eType != ET_NPC) return;
	if (ent->currentState.NPC_class != CLASS_PADAWAN) return;

	CHATTER_TextToSpeech(PADAWAN_NO_REPLY_CHATTERS[choice], CG_GetTextToSpeechVoiceForEntity(ent), entityNum, ent->lerpOrigin, (qboolean)(Q_stricmpn(PADAWAN_NO_REPLY_CHATTERS[choice], "#", 1)));
}

//
// Padawan Chatters...
//
const char *PADAWAN_CHATTERS[] =
{
	//"I think I might retire here",
	//"Give me the strength to change the things I can, the grace to accept the things I can not, and an account full of galactic credits",
	//"Democracy is a beautiful thing, except for that part about letting just any idiot vote",
	//"Home is where your house is",
	//"When I die, I want to see my old master again. But he better have lost the nose hair and the old-man smell",
	//"Love is that first feeling you feel before all the bad stuff",
	//"I wanna get chocolate wasted",
	//"I see dead people",
	//"I think I'm gonna like it here",
	//"If you fight with the god of death, I can't guarantee your safety",
	//"Can I shoot them now",
	//"I like using two pistols. There is symetry to wielding two",
	"Man That guy was a special kind of stupid",
	"Man that guy was stupid, ugly and a bad shooter",
	//"I'm gonna keep fighting til this world is the way it should be. Full of candy",
	//"When hunting a legendary light saber. Finding a fairy or two shouldn't be surprising",
	//"You can't trust a rancor to watch your food",
	//"You shouldn't laugh at your master, when he's mad or screaming at you",
	//"I want a wampa pet. Can I have one, master",
	//"Are we there yet",
	"Master, I need to pee",
	//"Master, where does light go when you turn it off",
	//"Master, do blind people feel love at first sight",
	//"Master, if you try to fail, and succeed, which have you done",
	//"Master, if you tell a joke in the forest, but nobody laughs, was it a joke",
	//"Master, if I save time, when do I get it back",
	"Master, before drawing boards were invented, where did one go back to",
	"Master, can a stupid person be a smart ass",
	//"Master, do imperial pilots take crash courses",
	"Master, does killing time damage eternity",
	"Master, how many weeks are there in a light year",
	"Master, what is the speed of dark",
	//"Master, if someone with multiple personalities threatens to kill himself, is it considered a hostage situation",
	"Master, I know what the f word means. Its like sex, but you dont love the other person",
	// UQ1: This work but are too long for reply timer... Need to keep these under 10 seconds to make it not stupid...
	////"Master, As you make your way through this hectic world, set aside a few minutes each day. At the end of the year, you will have a couple of days saved up",
	////"Master, If we could just get everyone to close his or her eyes and visualize galactic peace for an hour, imagine how serene and quiet it would be until the looting started",
	////"Master, I like to go to the greysor pound and pretend I found my greysor, but then tell them to kill it anyway because I already sold all his stuff. Pound people have no sense of humor",
	////"Master, I believe you should live each day as if it is your last, which is why I dont have any clean laundry, because who wants to wash clothes on the last day of their life",
	// These ones are pre-recorded emotions...
	"",
};

int PADAWAN_CHATTERS_MAX = -1;

int GetPadawanChattersMax()
{
	int total = 0;

	if (PADAWAN_CHATTERS_MAX != -1) return PADAWAN_CHATTERS_MAX; // already set up...

	// We need to count them...
	while (strncmp(PADAWAN_CHATTERS[total], "", strlen(PADAWAN_CHATTERS[total])))
	{
		total++;
	}

	PADAWAN_CHATTERS_MAX = total;
	return total;
}

void CG_PadawanIdleChatter ( int entityNum )
{
	int choice = irand(0,GetPadawanChattersMax());
	centity_t *ent = &cg_entities[entityNum];

	if (!ent) return;
	if (ent->currentState.eType != ET_NPC) return;
	if (ent->currentState.NPC_class != CLASS_PADAWAN) return;

	CHATTER_TextToSpeech(PADAWAN_CHATTERS[choice], CG_GetTextToSpeechVoiceForEntity(ent), entityNum, ent->lerpOrigin, qtrue);
}

//
// Padawan Reply Chatters...
//
const char *PADAWAN_REPLY_CHATTERS[] =
{
	//"ummmmm. yeah",
	//"yeah. ok",
	//"i guess so",
	//"ummmmm. why not",
	//"right",
	//"ok",
	//"okay",
	//"ummmmm",
	//"that is a stupid question",
	//"well, that is a stupid question",
	//"that was a stupid question",
	//"well, that was a stupid question",
	// These are pre-recorded emotional samples...
	//"Idiot!",
	//"Maybe!",
	// Pre-recorded sample ones...
	"Not again!",
	"Oh my god! Not again!",
	"Ridiculous!",
	"Stop bugging me!",
	"Stop it!",
	"Stop!",
	"Stupid!",
	//"That's silly!",
	"Whatever!",
	"I don't care!",
	"",
};

int PADAWAN_REPLY_CHATTERS_MAX = -1;

int GetPadawanReplyChattersMax()
{
	int total = 0;

	if (PADAWAN_REPLY_CHATTERS_MAX != -1) return PADAWAN_REPLY_CHATTERS_MAX; // already set up...

	// We need to count them...
	while (strncmp(PADAWAN_REPLY_CHATTERS[total], "", strlen(PADAWAN_REPLY_CHATTERS[total])))
	{
		total++;
	}

	PADAWAN_REPLY_CHATTERS_MAX = total;
	return total;
}

const char *PADAWAN_REPLY_YODA_CHATTERS[] =
{
	/*"ummmmm. yeah",
	"yeah. ok",
	"i guess so",
	"ummmmm. why not",
	"right",
	"ok",
	"okay",
	"ummmmm",
	"a stupid question, that is",
	"well, a stupid question, that is",
	"a stupid question, that was",
	"well, a stupid question, that was",*/
	// Pre-recorded sample ones...
	"Not again!",
	"Oh my god! Not again!",
	"Ridiculous!",
	"Stop bugging me!",
	"Stop it!",
	"Stop!",
	"Stupid!",
	//"That's silly!",
	"Whatever!",
	"I don't care!",
	"",
};

int PADAWAN_REPLY_YODA_CHATTERS_MAX = -1;

int GetPadawanReplyYodaChattersMax()
{
	int total = 0;

	if (PADAWAN_REPLY_YODA_CHATTERS_MAX != -1) return PADAWAN_REPLY_YODA_CHATTERS_MAX; // already set up...

	// We need to count them...
	while (strncmp(PADAWAN_REPLY_YODA_CHATTERS[total], "", strlen(PADAWAN_REPLY_YODA_CHATTERS[total])))
	{
		total++;
	}

	PADAWAN_REPLY_YODA_CHATTERS_MAX = total;
	return total;
}

void CG_PadawanIdleReplyChatter ( int entityNum )
{
	int			choice = irand(0,GetPadawanReplyChattersMax());
	centity_t	*ent = &cg_entities[entityNum];
	qboolean	isYoda = CG_IsYoda(ent);

	if (isYoda) choice = irand(0,GetPadawanReplyYodaChattersMax());

	if (!ent) return;
	//if (ent->currentState.eType != ET_NPC) return;

	if (isYoda) 
		CHATTER_TextToSpeech(PADAWAN_REPLY_YODA_CHATTERS[choice], CG_GetTextToSpeechVoiceForEntity(ent), entityNum, ent->lerpOrigin, qtrue);
	else
		CHATTER_TextToSpeech(PADAWAN_REPLY_CHATTERS[choice], CG_GetTextToSpeechVoiceForEntity(ent), entityNum, ent->lerpOrigin, qtrue);
}

//
// Backup Combat Sounds...
//

const char *COMBAT_BACKUP_SOUNDS[] =
{
	// Pre-recorded sample ones...
	"#AARGH01#",
	"#AARGH02#",
	"#AARGH03#",
	"#LAUGH01#",
	"#LAUGH02#",
	"#LAUGH03#",
	"#YAWN01#",
	"#YAWN02#",
	"Loser!",
	"Stop it!",
	"Stop!",
	"Uh oh!",
	"You lose!",
	"I win!",
	"Yes!",
	"Impressive!",
	"",
};

int COMBAT_BACKUP_SOUNDS_MAX = -1;

int GetCombatSoundsMax()
{
	int total = 0;

	if (COMBAT_BACKUP_SOUNDS_MAX != -1) return COMBAT_BACKUP_SOUNDS_MAX; // already set up...

	// We need to count them...
	while (strncmp(COMBAT_BACKUP_SOUNDS[total], "", strlen(COMBAT_BACKUP_SOUNDS[total])))
	{
		total++;
	}

	COMBAT_BACKUP_SOUNDS_MAX = total;
	return total;
}

//
// Pre-downloading all chats for all voices stuff...
//
extern char *showPowersName[];

void CG_DownloadAllTextToSpeechSounds ( void )
{
	int		voice_num = 0;
	char	voice[64];
	int		wait_time = 0;

	for (voice_num = 0; voice_num < GetTTSVoicesMax() && ttsVoiceData[voice_num].age != TTS_AGE_MAX; voice_num++)
	{
		int		weaponNum = 0;
		int		forcePowerNum = 0;
		int		soundNum = 0;

		if (ttsVoiceData[voice_num].age == TTS_AGE_NONE) continue;

		strcpy(voice, ttsVoiceData[voice_num].voicename);

		// First get the backup combat sounds...
		
		for (soundNum = 0; soundNum < GetCombatSoundsMax(); soundNum++)
		{
			trap->Print("Generating backup combat sound %s for voice %s.\n", COMBAT_BACKUP_SOUNDS[soundNum], voice);

			while (!trap->S_DownloadVoice(COMBAT_BACKUP_SOUNDS[soundNum], voice))
			{// Wait and retry...
				trap->Print("Failed. Waiting a moment before continuing.\n");

				for (wait_time = 0; wait_time < 500; wait_time++)
				{// Do some random silly stuff as we have no sleep() function;
					int ran = irand(0,100);
					trap->UpdateScreen();
				}
			}

			trap->UpdateScreen();
		}

		// Download all weapon sounds...
		for (weaponNum = 0; weaponNum < WP_NUM_WEAPONS; weaponNum++)
		{
			if (weaponData[weaponNum].classname && weaponData[weaponNum].classname[0])
			{
				trap->Print("Generating TTS weapon %s sound for voice %s.\n", weaponData[weaponNum].classname, voice);

				while (!trap->S_DownloadVoice(weaponData[weaponNum].classname, voice))
				{// Wait and retry...
					trap->Print("Failed. Waiting a moment before continuing.\n");

					for (wait_time = 0; wait_time < 500; wait_time++)
					{// Do some random silly stuff as we have no sleep() function;
						int ran = irand(0,100);
						trap->UpdateScreen();
					}
				}
			}

			// Wait at least 1 sec between server connects...
			/*wait_time = cg.time + 1000;

			for (wait_time = 0; wait_time < 100; wait_time++)
			{// Do some random silly stuff as we have no sleep() function;
				int ran = irand(0,100);
				trap->UpdateScreen();
			}*/
			trap->UpdateScreen();
		}

		for (forcePowerNum = 0; forcePowerNum < NUM_FORCE_POWERS; forcePowerNum++)
		{
			if ((char *)CG_GetStringEdString("SP_INGAME", showPowersName[forcePowerNum])[0])
			{
				trap->Print("Generating TTS force power %s sound for voice %s.\n", (char *)CG_GetStringEdString("SP_INGAME", showPowersName[forcePowerNum]), voice);

				while (!trap->S_DownloadVoice((char *)CG_GetStringEdString("SP_INGAME", showPowersName[forcePowerNum]), voice))
				{// Wait and retry...
					trap->Print("Failed. Waiting a moment before continuing.\n");

					for (wait_time = 0; wait_time < 500; wait_time++)
					{// Do some random silly stuff as we have no sleep() function;
						int ran = irand(0,100);
						trap->UpdateScreen();
					}
				}
			}

			// Wait at least 1 sec between server connects...
			/*for (wait_time = 0; wait_time < 100; wait_time++)
			{// Do some random silly stuff as we have no sleep() function;
				int ran = irand(0,100);
				trap->UpdateScreen();
			}*/
			trap->UpdateScreen();
		}

		if (ttsVoiceData[voice_num].age == TTS_AGE_CHILD)
		{// Do padawan chatters...
			int padawan_chatter = 0;

			for (padawan_chatter = 0; padawan_chatter < GetPadawanChattersMax(); padawan_chatter++)
			{
				trap->Print("Generating TTS padawan chatter %i sound for voice %s.\n", padawan_chatter, voice);
				
				while (!trap->S_DownloadVoice(PADAWAN_CHATTERS[padawan_chatter], voice))
				{// Wait and retry...
					trap->Print("Failed. Waiting a moment before continuing.\n");
					trap->Print("Text was %s.", PADAWAN_CHATTERS[padawan_chatter]);

					for (wait_time = 0; wait_time < 500; wait_time++)
					{// Do some random silly stuff as we have no sleep() function;
						int ran = irand(0,100);
						trap->UpdateScreen();
					}
				}

				trap->UpdateScreen();
			}

			for (padawan_chatter = 0; padawan_chatter < GetPadawanNoReplyChattersMax(); padawan_chatter++)
			{
				trap->Print("Generating TTS padawan no-reply chatter %i sound for voice %s.\n", padawan_chatter, voice);
				
				while (!trap->S_DownloadVoice(PADAWAN_NO_REPLY_CHATTERS[padawan_chatter], voice))
				{// Wait and retry...
					trap->Print("Failed. Waiting a moment before continuing.\n");

					for (wait_time = 0; wait_time < 500; wait_time++)
					{// Do some random silly stuff as we have no sleep() function;
						int ran = irand(0,100);
						trap->UpdateScreen();
					}
				}

				trap->UpdateScreen();
			}

			for (padawan_chatter = 0; padawan_chatter < GetPadawanCombatChattersMax(); padawan_chatter++)
			{
				trap->Print("Generating TTS padawan combat chatter %i sound for voice %s.\n", padawan_chatter, voice);
				
				while (!trap->S_DownloadVoice(PADAWAN_COMBAT_TALKS[padawan_chatter], voice))
				{// Wait and retry...
					trap->Print("Failed. Waiting a moment before continuing.\n");

					for (wait_time = 0; wait_time < 500; wait_time++)
					{// Do some random silly stuff as we have no sleep() function;
						int ran = irand(0,100);
						trap->UpdateScreen();
					}
				}

				trap->UpdateScreen();
			}

			for (padawan_chatter = 0; padawan_chatter < GetPadawanCombatKillChattersMax(); padawan_chatter++)
			{
				trap->Print("Generating TTS padawan combat kill chatter %i sound for voice %s.\n", padawan_chatter, voice);
				
				while (!trap->S_DownloadVoice(PADAWAN_COMBAT_KILL_TALKS[padawan_chatter], voice))
				{// Wait and retry...
					trap->Print("Failed. Waiting a moment before continuing.\n");

					for (wait_time = 0; wait_time < 500; wait_time++)
					{// Do some random silly stuff as we have no sleep() function;
						int ran = irand(0,100);
						trap->UpdateScreen();
					}
				}

				trap->UpdateScreen();
			}
		}

		if (ttsVoiceData[voice_num].gender == TTS_GENDER_YODA)
		{// Do padawan reply chatters... Yoda has special ones...
			int padawan_chatter = 0;

			for (padawan_chatter = 0; padawan_chatter < GetPadawanReplyYodaChattersMax(); padawan_chatter++)
			{
				trap->Print("Generating TTS padawan reply chatter %i sound for voice %s.\n", padawan_chatter, voice);
				
				while (!trap->S_DownloadVoice(PADAWAN_REPLY_YODA_CHATTERS[padawan_chatter], voice))
				{// Wait and retry...
					trap->Print("Failed. Waiting a moment before continuing.\n");

					for (wait_time = 0; wait_time < 500; wait_time++)
					{// Do some random silly stuff as we have no sleep() function;
						int ran = irand(0,100);
						trap->UpdateScreen();
					}
				}

				trap->UpdateScreen();
			}
		}
		else if (ttsVoiceData[voice_num].age != TTS_AGE_CHILD)
		{// Do padawan reply chatters...
			int padawan_chatter = 0;

			for (padawan_chatter = 0; padawan_chatter < GetPadawanReplyChattersMax(); padawan_chatter++)
			{
				trap->Print("Generating TTS padawan reply chatter %i sound for voice %s.\n", padawan_chatter, voice);
				
				while (!trap->S_DownloadVoice(PADAWAN_REPLY_CHATTERS[padawan_chatter], voice))
				{// Wait and retry...
					trap->Print("Failed. Waiting a moment before continuing.\n");

					for (wait_time = 0; wait_time < 500; wait_time++)
					{// Do some random silly stuff as we have no sleep() function;
						int ran = irand(0,100);
						trap->UpdateScreen();
					}
				}

				trap->UpdateScreen();
			}
		}

		trap->Print("Generating TTS \"My health is low.\" sound for voice %s.\n", voice);

		while (!trap->S_DownloadVoice("My health is low.", voice))
		{// Wait and retry...
			trap->Print("Failed. Waiting a moment before continuing.\n");

			for (wait_time = 0; wait_time < 500; wait_time++)
			{// Do some random silly stuff as we have no sleep() function;
				int ran = irand(0,100);
				trap->UpdateScreen();
			}
		}
	}
}
