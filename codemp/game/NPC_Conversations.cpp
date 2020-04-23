#include "g_local.h"
#include "ai_dominance_main.h"
#include "g_nav.h"
#include "anims.h"
#include "w_saber.h"
#include "bg_public.h"
#include "b_local.h"

#define __DOMINANCE_NPC__
#define __NPC_CONVERSATIONS__
#define __SHORT_STORMIE_CONVOS__
//#define __NPC_CONVERSATION_DEBUG__ // UQ1: Enable to list all npc conversations to the chat log...

#ifdef __DOMINANCE_NPC__

//Local Variables
extern npcStatic_t NPCS;

extern qboolean NPC_FacePosition(gentity_t *aiEnt, vec3_t position, qboolean doPitch );
extern qboolean Jedi_Move(gentity_t *aiEnt, gentity_t *goal, qboolean retreat );

int			NUM_REGISTERED_CONVO_FILES = 0;
char		REGISTERED_CONVO_FILES[1024][256];
qboolean	REGISTERED_CONVO_EXISTS[1024];

void NPC_EnforceConversationRange ( gentity_t *self )
{
	float dist = Distance(self->r.currentOrigin, self->NPC->conversationPartner->r.currentOrigin);
	gentity_t *aiEnt = self;

	if (!(aiEnt->client->pers.cmd.buttons & BUTTON_WALKING))
		aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;

	if (dist > 56)
	{// Too far, move forward...
		//trap->Print("Dist is %f. Moving closer.\n", dist);
		Jedi_Move(self, self->NPC->conversationPartner, qfalse);
	}
	else if (dist < 18 )
	{// Too close, move back...
		//trap->Print("Dist is %f. Moving back.\n", dist);
		Jedi_Move(self, self->NPC->conversationPartner, qtrue);
	}
	else
	{
		//trap->Print("Dist is %f. OK!\n", dist);
		aiEnt->client->pers.cmd.forwardmove = 0;
		aiEnt->client->pers.cmd.rightmove = 0;
		aiEnt->client->pers.cmd.upmove = 0;
	}
}

void G_NPCSound( gentity_t *ent, int channel, int soundIndex ) {
	ent->s.eventIndex = channel;
	G_AddEvent( ent, EV_ENTITY_SOUND, soundIndex );
}

qboolean G_ConversationExists ( char *filename )
{
	fileHandle_t	f;
	int				i;

	// Better then disk accessing...
	for (i = 0; i < NUM_REGISTERED_CONVO_FILES; i++)
	{
		if (!Q_stricmp(filename, REGISTERED_CONVO_FILES[i])) 
		{
			if (REGISTERED_CONVO_EXISTS[i])
			{
				//trap->Print("Convo %s exists.\n", filename);
				return qtrue;
			}
			else
			{
				//trap->Print("Convo %s does not exist.\n", filename);
				return qfalse;
			}
		}
	}

	trap->FS_Open( filename, &f, FS_READ );

	if ( !f )
	{
		trap->FS_Close( f );

		REGISTERED_CONVO_EXISTS[NUM_REGISTERED_CONVO_FILES] = qfalse;
		sprintf(REGISTERED_CONVO_FILES[NUM_REGISTERED_CONVO_FILES], filename);
		NUM_REGISTERED_CONVO_FILES++;

		//trap->Print("Convo %s set as not exist.\n", filename);
		return qfalse;
	}

	trap->FS_Close( f );

	REGISTERED_CONVO_EXISTS[NUM_REGISTERED_CONVO_FILES] = qtrue;
	sprintf(REGISTERED_CONVO_FILES[NUM_REGISTERED_CONVO_FILES], filename);
	NUM_REGISTERED_CONVO_FILES++;

	//trap->Print("Convo %s set as exist.\n", filename);
	return qtrue;
}

qboolean CONVO_SOUNDS_REGISTERED = qfalse;

void G_InitNPCConversationSounds ( void )
{
	int				section = 0;
	int				part = 0;
	char			filename[256];

	//
	// Stormies...
	//

	for (section = 1; section < 15; section++)
	{
		for (part = 1; part < 15; part++)
		{
			if (section < 10)
			{
				if (part < 10)
					sprintf(filename, "sound/conversation/stormtrooper/MST_0%iL0%i.mp3", section, part);
				else
					sprintf(filename, "sound/conversation/stormtrooper/MST_0%iL%i.mp3", section, part);
			}
			else
			{
				if (part < 10)
					sprintf(filename, "sound/conversation/stormtrooper/MST_%iL0%i.mp3", section, part);
				else
					sprintf(filename, "sound/conversation/stormtrooper/MST_%iL%i.mp3", section, part);
			}

			if ( !G_ConversationExists(filename) ) continue;

			G_SoundIndex( filename );
		}
	}

	//
	// Civilians...
	//

	for (part = 0; part < 50; part++)
	{
		if (part < 10)
			sprintf(filename, "sound/conversation/%s/conversation0%i.mp3", "civilian_gran", part);
		else
			sprintf(filename, "sound/conversation/%s/conversation%i.mp3", "civilian_gran", part);

		if ( !G_ConversationExists(filename) ) continue;

		G_SoundIndex( filename );
	}

	for (part = 0; part < 50; part++)
	{
		if (part < 10)
			sprintf(filename, "sound/conversation/%s/conversation0%i.mp3", "civilian_human_merc", part);
		else
			sprintf(filename, "sound/conversation/%s/conversation%i.mp3", "civilian_human_merc", part);

		if ( !G_ConversationExists(filename) ) continue;

		G_SoundIndex( filename );
	}

	for (part = 0; part < 50; part++)
	{
		if (part < 10)
			sprintf(filename, "sound/conversation/%s/conversation0%i.mp3", "civilian_human_merc2", part);
		else
			sprintf(filename, "sound/conversation/%s/conversation%i.mp3", "civilian_human_merc2", part);

		if ( !G_ConversationExists(filename) ) continue;

		G_SoundIndex( filename );
	}

	for (part = 0; part < 50; part++)
	{
		if (part < 10)
			sprintf(filename, "sound/conversation/%s/conversation0%i.mp3", "civilian_protocol", part);
		else
			sprintf(filename, "sound/conversation/%s/conversation%i.mp3", "civilian_protocol", part);

		if ( !G_ConversationExists(filename) ) continue;

		G_SoundIndex( filename );
	}

	for (part = 0; part < 50; part++)
	{
		if (part < 10)
			sprintf(filename, "sound/conversation/%s/conversation0%i.mp3", "civilian_r2d2", part);
		else
			sprintf(filename, "sound/conversation/%s/conversation%i.mp3", "civilian_r2d2", part);

		if ( !G_ConversationExists(filename) ) continue;

		G_SoundIndex( filename );
	}

	for (part = 0; part < 50; part++)
	{
		if (part < 10)
			sprintf(filename, "sound/conversation/%s/conversation0%i.mp3", "civilian_r5d2", part);
		else
			sprintf(filename, "sound/conversation/%s/conversation%i.mp3", "civilian_r5d2", part);

		if ( !G_ConversationExists(filename) ) continue;

		G_SoundIndex( filename );
	}

	for (part = 0; part < 50; part++)
	{
		if (part < 10)
			sprintf(filename, "sound/conversation/%s/conversation0%i.mp3", "civilian_rodian", part);
		else
			sprintf(filename, "sound/conversation/%s/conversation%i.mp3", "civilian_rodian", part);

		if ( !G_ConversationExists(filename) ) continue;

		G_SoundIndex( filename );
	}

	for (part = 0; part < 50; part++)
	{
		if (part < 10)
			sprintf(filename, "sound/conversation/%s/conversation0%i.mp3", "civilian_rodian2", part);
		else
			sprintf(filename, "sound/conversation/%s/conversation%i.mp3", "civilian_rodian2", part);

		if ( !G_ConversationExists(filename) ) continue;

		G_SoundIndex( filename );
	}

	for (part = 0; part < 50; part++)
	{
		if (part < 10)
			sprintf(filename, "sound/conversation/%s/conversation0%i.mp3", "civilian_trandoshan", part);
		else
			sprintf(filename, "sound/conversation/%s/conversation%i.mp3", "civilian_trandoshan", part);

		if ( !G_ConversationExists(filename) ) continue;

		G_SoundIndex( filename );
	}

	for (part = 0; part < 50; part++)
	{
		if (part < 10)
			sprintf(filename, "sound/conversation/%s/conversation0%i.mp3", "civilian_weequay", part);
		else
			sprintf(filename, "sound/conversation/%s/conversation%i.mp3", "civilian_weequay", part);

		if ( !G_ConversationExists(filename) ) continue;

		G_SoundIndex( filename );
	}

	for (part = 0; part < 50; part++)
	{
		if (part < 10)
			sprintf(filename, "sound/conversation/%s/conversation0%i.mp3", "civilian_weequay2", part);
		else
			sprintf(filename, "sound/conversation/%s/conversation%i.mp3", "civilian_weequay2", part);

		if ( !G_ConversationExists(filename) ) continue;

		G_SoundIndex( filename );
	}

	for (part = 0; part < 50; part++)
	{
		if (part < 10)
			sprintf(filename, "sound/conversation/%s/conversation0%i.mp3", "civilian_weequay3", part);
		else
			sprintf(filename, "sound/conversation/%s/conversation%i.mp3", "civilian_weequay3", part);

		if ( !G_ConversationExists(filename) ) continue;

		G_SoundIndex( filename );
	}

	for (part = 0; part < 50; part++)
	{
		if (part < 10)
			sprintf(filename, "sound/conversation/%s/conversation0%i.mp3", "civilian_weequay4", part);
		else
			sprintf(filename, "sound/conversation/%s/conversation%i.mp3", "civilian_weequay4", part);

		if ( !G_ConversationExists(filename) ) continue;

		G_SoundIndex( filename );
	}

	CONVO_SOUNDS_REGISTERED = qtrue;
}

void NPC_EndConversation(gentity_t *aiEnt)
{
#ifdef __NPC_CONVERSATIONS__
	gentity_t	*NPC = aiEnt;

	NPC->NPC->conversationRole = 0;
	if (NPC->NPC->conversationPartner && NPC->NPC->conversationPartner->NPC) NPC->NPC->conversationPartner->NPC->conversationRole = 0;
	NPC->NPC->conversationSection = 1;
	if (NPC->NPC->conversationPartner && NPC->NPC->conversationPartner->NPC) NPC->NPC->conversationPartner->NPC->conversationSection = 1;
	NPC->NPC->conversationPart = 1;
	if (NPC->NPC->conversationPartner && NPC->NPC->conversationPartner->NPC) NPC->NPC->conversationPartner->NPC->conversationPart = 1;

	NPC->NPC->conversationReplyTime = level.time + 60000;
	if (NPC->NPC->conversationPartner && NPC->NPC->conversationPartner->NPC) NPC->NPC->conversationPartner->NPC->conversationReplyTime = level.time + 45000;

	if (NPC->NPC->conversationPartner && NPC->NPC->conversationPartner->NPC) NPC->NPC->conversationPartner->NPC->conversationPartner = NULL;
	NPC->NPC->conversationPartner = NULL;
#endif //__NPC_CONVERSATIONS__
}

void NPC_SetStormtrooperConversationReplyTimer(gentity_t *aiEnt)
{
#ifdef __NPC_CONVERSATIONS__
	gentity_t	*NPC = aiEnt;

	if (NPC->NPC->conversationPartner && NPC->NPC->conversationPartner->NPC)
	{
		NPC->NPC->conversationPart++;
		NPC->NPC->conversationPartner->NPC->conversationPart++;
		NPC->NPC->conversationReplyTime = level.time + 14000;
		NPC->NPC->conversationPartner->NPC->conversationReplyTime = level.time + 7000;
	}
	else
	{
		NPC_EndConversation(aiEnt);
	}
#endif //__NPC_CONVERSATIONS__
}

extern void NPC_SetAnim(gentity_t *ent, int setAnimParts, int anim, int setAnimFlags);


void NPC_ConversationAnimation(gentity_t *NPC)
{
#ifdef __NPC_CONVERSATIONS__
	int randAnim = irand(1,8);

	switch (randAnim)
	{
	case 1:
	case 2:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND1_TALK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		//trap->Print("BOTH_STAND1_TALK1\n");
		break;
	case 3:
	case 4:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND1_TALK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		//trap->Print("BOTH_STAND1_TALK2\n");
		break;
	case 5:
	case 8:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND1_TALK3, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		//trap->Print("BOTH_STAND1_TALK3\n");
		break;
	case 7:
	default:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_TALK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		//trap->Print("BOTH_TALK1\n");
		break;
	}
#endif //__NPC_CONVERSATIONS__
}

void NPC_StormTrooperConversation(gentity_t *aiEnt)
{
#ifdef __NPC_CONVERSATIONS__
	gentity_t		*NPC = aiEnt;
	int				role = NPC->NPC->conversationRole;
	int				section = NPC->NPC->conversationSection;
	int				part = NPC->NPC->conversationPart;
	vec3_t			origin, angles;
	char			filename[256];

	if (NPC->enemy || !NPC->NPC->conversationPartner || NPC->NPC->conversationPartner->enemy || !NPC->NPC->conversationPartner->NPC)
	{// Exit early if they get a target...
		NPC_EndConversation(aiEnt);
		return;
	}

	// Look at our partner...
	VectorCopy(NPC->NPC->conversationPartner->r.currentOrigin, origin);
	VectorSubtract( origin, NPC->r.currentOrigin , NPC->move_vector );
	vectoangles( NPC->move_vector, angles );
	G_SetAngles(NPC, angles);
	VectorCopy(angles, NPC->client->ps.viewangles);
	NPC_FacePosition( NPC, NPC->NPC->conversationPartner->r.currentOrigin, qfalse );

	if (NPC->NPC->conversationReplyTime > level.time)
		return; // Wait...

	if (section < 10)
	{
		if (part < 10)
			sprintf(filename, "sound/conversation/stormtrooper/MST_0%iL0%i.mp3", section, part);
		else
			sprintf(filename, "sound/conversation/stormtrooper/MST_0%iL%i.mp3", section, part);
	}
	else
	{
		if (part < 10)
			sprintf(filename, "sound/conversation/stormtrooper/MST_%iL0%i.mp3", section, part);
		else
			sprintf(filename, "sound/conversation/stormtrooper/MST_%iL%i.mp3", section, part);
	}

	if ( !G_ConversationExists(filename) )
	{
		//trap->Print("File %s does not exist.\n", filename);

#ifdef __SHORT_STORMIE_CONVOS__
		// Short convo's only play 1 random section...
		NPC_EndConversation(aiEnt);
		return;
#else //!__SHORT_STORMIE_CONVOS__
		NPC->NPC->conversationSection++;
		NPC->NPC->conversationPart = 1;

		if (NPC->NPC->conversationPartner->NPC)
		{
			NPC->NPC->conversationPartner->NPC->conversationSection++;
			NPC->NPC->conversationPartner->NPC->conversationPart = 1;
		}
		else
		{
			NPC_EndConversation();
			return;
		}

		if (section < 10)
		{
			if (part < 10)
				sprintf(filename, "sound/conversation/stormtrooper/MST_0%iL0%i.mp3", section, part);
			else
				sprintf(filename, "sound/conversation/stormtrooper/MST_0%iL%i.mp3", section, part);
		}
		else
		{
			if (part < 10)
				sprintf(filename, "sound/conversation/stormtrooper/MST_%iL0%i.mp3", section, part);
			else
				sprintf(filename, "sound/conversation/stormtrooper/MST_%iL%i.mp3", section, part);
		}

		if ( !G_ConversationExists(filename) )
		{// End of conversation...
			//trap->Print("File %s does not exist. Aborting conversation.\n", filename);

			if (NPC->NPC->conversationSection > 15)
				NPC_EndConversation();

			return;
		}
#endif //__SHORT_STORMIE_CONVOS__
	}
	//CHAN_VOICE

	//trap->Print("NPC %i playing sound file %s.\n", NPC->s.number, filename);

	G_NPCSound( NPC, CHAN_VOICE/*CHAN_AUTO*/, G_SoundIndex(filename));
	NPC_SetStormtrooperConversationReplyTimer(aiEnt);
	NPC_ConversationAnimation(NPC);
#endif //__NPC_CONVERSATIONS__
}

#ifdef __SHORT_STORMIE_CONVOS__
int NPC_StormtrooperFindConversationMaxSections( void )
{
	int				section = 1;
	int				part = 1;
	char			filename[256];

	for (section = 1; section <= 15; section++)
	{
		if (section < 10)
		{
			sprintf(filename, "sound/conversation/stormtrooper/MST_0%iL0%i.mp3", section, part);
		}
		else
		{
			sprintf(filename, "sound/conversation/stormtrooper/MST_%iL0%i.mp3", section, part);
		}

		if ( !G_ConversationExists(filename) )
		{// We found the first section with no sound... This is the max...
			return section;
		}
	}

	// Never found an end... Just return the value of section...
	return section;
}
#endif //__SHORT_STORMIE_CONVOS__

void NPC_StormtrooperFindConversationPartner(gentity_t *aiEnt)
{
#ifdef __NPC_CONVERSATIONS__
	gentity_t	*NPC = aiEnt;

	if (NPC->client->NPC_class != CLASS_STORMTROOPER
		&& NPC->client->NPC_class != CLASS_STORMTROOPER_ADVANCED 
		&& NPC->client->NPC_class != CLASS_STORMTROOPER_ATST_PILOT
		&& NPC->client->NPC_class != CLASS_STORMTROOPER_ATAT_PILOT) return;

	//if (VectorLength(NPC->client->ps.velocity) <= 16)
	{// I'm not mooving... Conversaion possible...
		int i = 0;
		gentity_t *partner = NULL;

		// Make sure there are no other conversations going on...
		for (i = MAX_CLIENTS; i < MAX_GENTITIES; i++)
		{
			partner = &g_entities[i];

			if (!partner) continue;
			if (partner == NPC) continue;
			if (partner->s.eType != ET_NPC) continue;
			if (!partner->client) continue;
			if (!partner->NPC) continue;
			if (partner->client->NPC_class != CLASS_STORMTROOPER 
				&& partner->client->NPC_class != CLASS_STORMTROOPER_ADVANCED 
				&& partner->client->NPC_class != CLASS_STORMTROOPER_ATST_PILOT
				&& partner->client->NPC_class != CLASS_STORMTROOPER_ATAT_PILOT)
				continue;
			if (partner->NPC->conversationPartner || partner->NPC->conversationReplyTime > level.time)
#ifdef __SHORT_STORMIE_CONVOS__
				// Short convos only play one section... So we should be able to use more at a time (and closer together)...
				if (Distance(partner->r.currentOrigin, NPC->r.currentOrigin) < 512)
					return; // We don't want them talking too close to others having the same conversations :)
#else //!__SHORT_STORMIE_CONVOS__
				if (Distance(partner->r.currentOrigin, NPC->r.currentOrigin) < 2048)
					return; // We don't want them talking too close to others having the same conversations :)
#endif //__SHORT_STORMIE_CONVOS__
		}

		for (i = MAX_CLIENTS; i < MAX_GENTITIES; i++)
		{
			partner = &g_entities[i];

			if (!partner) continue;
			if (partner == NPC) continue;
			if (partner->s.eType != ET_NPC) continue;
			if (!partner->client) continue;
			if (partner->client->NPC_class != CLASS_STORMTROOPER 
				&& partner->client->NPC_class != CLASS_STORMTROOPER_ADVANCED 
				&& partner->client->NPC_class != CLASS_STORMTROOPER_ATST_PILOT
				&& partner->client->NPC_class != CLASS_STORMTROOPER_ATAT_PILOT)
				continue;
			if (VectorLength(partner->client->ps.velocity) > 16 && Distance(partner->r.currentOrigin, NPC->r.currentOrigin) > 96) continue;
			if (!partner->NPC) continue;

			if (partner->NPC->conversationPartner || partner->NPC->conversationReplyTime > level.time)
			{// this one already in a convo...
#ifdef __SHORT_STORMIE_CONVOS__
				// Short convos only play one section... So we should be able to use more at a time (and closer together)...
				if (Distance(partner->r.currentOrigin, NPC->r.currentOrigin) < 512)
					return; // We don't want them talking too close to others having the same conversations :)
#else //!__SHORT_STORMIE_CONVOS__
				if (Distance(partner->r.currentOrigin, NPC->r.currentOrigin) < 2048)
					return; // We don't want them talking too close to others having the same conversations :)
#endif //__SHORT_STORMIE_CONVOS__

				continue;
			}

			if (Distance(partner->r.currentOrigin, NPC->r.currentOrigin) > 128) continue;

#ifdef __SHORT_STORMIE_CONVOS__
			// Looks good! Start a convo... Short conversations only play 1 section...
			NPC->NPC->conversationSection = irand(1, NPC_StormtrooperFindConversationMaxSections()-1);
			NPC->NPC->conversationRole = 1;
			NPC->NPC->conversationPart = 1;

			if (partner->NPC)
				NPC->NPC->conversationPartner = partner;

			if (NPC->NPC->conversationPartner->NPC)
			{
				NPC->NPC->conversationPartner->NPC->conversationPartner = NPC;
				NPC->NPC->conversationPartner->NPC->conversationRole = 2;
				NPC->NPC->conversationPartner->NPC->conversationSection = NPC->NPC->conversationSection;
				NPC->NPC->conversationPartner->NPC->conversationPart = 1;
				NPC->NPC->conversationPartner->NPC->conversationReplyTime = level.time + 8000;

#ifdef __NPC_CONVERSATION_DEBUG__
				trap->Print(">> NPC %i (%s) enterred a conversation with NPC %i.\n", NPC->s.number, NPC->NPC_type, NPC->NPC->conversationPartner->s.number);
#endif //__NPC_CONVERSATION_DEBUG__
				NPC_StormTrooperConversation(aiEnt);
			}
			else
			{
				NPC_EndConversation(aiEnt);
				continue;
			}
			return;
#else //!__SHORT_STORMIE_CONVOS__
			// Looks good! Start a convo...
			NPC->NPC->conversationSection = 1;
			NPC->NPC->conversationRole = 1;
			NPC->NPC->conversationPart = 1;

			if (partner->NPC)
				NPC->NPC->conversationPartner = partner;

			if (NPC->NPC->conversationPartner->NPC)
			{
				NPC->NPC->conversationPartner->NPC->conversationPartner = NPC;
				NPC->NPC->conversationPartner->NPC->conversationRole = 2;
				NPC->NPC->conversationPartner->NPC->conversationSection = 1;
				NPC->NPC->conversationPartner->NPC->conversationPart = 1;
				NPC->NPC->conversationPartner->NPC->conversationReplyTime = level.time + 8000;

#ifdef __NPC_CONVERSATION_DEBUG__
				trap->Print(">> NPC %i (%s) enterred a conversation with NPC %i.\n", NPC->s.number, NPC->NPC_type, NPC->NPC->conversationPartner->s.number);
#endif //__NPC_CONVERSATION_DEBUG__
				NPC_StormTrooperConversation();
			}
			else
			{
				NPC_EndConversation();
				continue;
			}
			return;
#endif //__SHORT_STORMIE_CONVOS__
		}
	}
#endif //__NPC_CONVERSATIONS__
}

qboolean NPC_HasConversationSounds(gentity_t *conversationalist)
{
#ifdef __NPC_CONVERSATIONS__
	char			filename[256];

	// For faster checking without FS wear...
	if (conversationalist->NPC->conversationAvailable == 1) return qfalse;
	if (conversationalist->NPC->conversationAvailable == 2) return qtrue;

	//trap->Print("Testing %s for conversation sounds.\n", conversationalist->NPC_type);

	sprintf(filename, "sound/conversation/%s/conversation00.mp3", conversationalist->NPC_type);

	if ( !G_ConversationExists(filename) )
	{// End of conversation...
		conversationalist->NPC->conversationAvailable = 1; // checked but has none!
		//trap->Print("%s has NO conversation sounds.\n", conversationalist->NPC_type);
		return qfalse;
	}

	conversationalist->NPC->conversationAvailable = 2; // checked and has some!
	//trap->Print("%s has conversation sounds.\n", conversationalist->NPC_type);
#endif //__NPC_CONVERSATIONS__
	return qtrue;
}

qboolean NPC_VendorHasConversationSounds(gentity_t *conversationalist)
{
#ifdef __NPC_CONVERSATIONS__
	char			filename[256];

	// For faster checking without FS wear...
	if (conversationalist->NPC->conversationAvailable == 1) return qfalse;
	if (conversationalist->NPC->conversationAvailable == 2) return qtrue;

	//trap->Print("Testing %s for conversation sounds.\n", conversationalist->NPC_type);

	sprintf(filename, "sound/conversation/civilian_%s/conversation00.mp3", conversationalist->NPC_type);

	if ( !G_ConversationExists(filename) )
	{// End of conversation...
		conversationalist->NPC->conversationAvailable = 1; // checked but has none!
		//trap->Print("%s has NO conversation sounds.\n", conversationalist->NPC_type);
		return qfalse;
	}

	conversationalist->NPC->conversationAvailable = 2; // checked and has some!
	//trap->Print("%s has conversation sounds.\n", conversationalist->NPC_type);
#endif //__NPC_CONVERSATIONS__
	return qtrue;
}

qboolean NPC_VendorHasVendorSound(gentity_t *conversationalist, char *name)
{
#ifdef __NPC_CONVERSATIONS__
	char			filename[256];

	sprintf(filename, va("sound/vendor/%s/%s.mp3", conversationalist->NPC_type, name));

	if ( !G_ConversationExists(filename) )
	{// End of conversation...
		return qfalse;
	}

#endif //__NPC_CONVERSATIONS__
	return qtrue;
}

void NPC_SetConversationReplyTimer(gentity_t *aiEnt)
{
#ifdef __NPC_CONVERSATIONS__
	gentity_t	*NPC = aiEnt;

	NPC->NPC->conversationPart++;
	NPC->NPC->conversationReplyTime = level.time + 10000;//18000;

	if (NPC->NPC->conversationPartner->NPC)
	{
		NPC->NPC->conversationPartner->NPC->conversationReplyTime = level.time + 5000;//8000;
	//	NPC->NPC->conversationPartner->NPC->conversationPart++;
	}
#endif //__NPC_CONVERSATIONS__
}

void NPC_NPCConversation(gentity_t *aiEnt)
{
#ifdef __NPC_CONVERSATIONS__
	gentity_t	*NPC = aiEnt;

	int				part = NPC->NPC->conversationPart-1;
//	vec3_t			origin, angles;
	char			filename[256];

	if (NPC->client->NPC_class == CLASS_STORMTROOPER 
		|| NPC->client->NPC_class == CLASS_STORMTROOPER_ADVANCED 
		|| NPC->client->NPC_class == CLASS_STORMTROOPER_ATST_PILOT 
		|| NPC->client->NPC_class == CLASS_STORMTROOPER_ATAT_PILOT)
	{
		NPC_StormTrooperConversation(aiEnt);
		return;
	}

	if (NPC->enemy || NPC->NPC->conversationPartner->enemy || !NPC->NPC->conversationPartner->NPC)
	{// Exit early if they get a target...
		NPC_EndConversation(aiEnt);
		return;
	}

	// Look at our partner...
	NPC_FacePosition( NPC, NPC->NPC->conversationPartner->r.currentOrigin, qfalse );

	if (NPC->NPC->conversationReplyTime > level.time)
		return; // Wait...

	if (part < 10)
		sprintf(filename, "sound/conversation/%s/conversation0%i.mp3", NPC->NPC_type, part);
	else
		sprintf(filename, "sound/conversation/%s/conversation%i.mp3", NPC->NPC_type, part);

	if ( !G_ConversationExists(filename) )
	{
		//trap->Print("File %s does not exist. Aborting conversation.\n", filename);

		NPC_EndConversation(aiEnt);

		return;
	}

	//trap->Print("NPC %i (%s) playing sound file %s (index %i).\n", NPC->s.number, NPC->NPC_type, filename, G_SoundIndex(filename));

	G_NPCSound( NPC, CHAN_VOICE/*CHAN_AUTO*/, G_SoundIndex(filename));
	NPC_SetConversationReplyTimer(aiEnt);
	NPC_ConversationAnimation(NPC);

	if (NPC->NPC->conversationPart > 50)
		NPC_EndConversation(aiEnt);
#endif //__NPC_CONVERSATIONS__
}

void NPC_FindConversationPartner(gentity_t *aiEnt)
{
#ifdef __NPC_CONVERSATIONS__
	gentity_t	*NPC = aiEnt;

	//if (!CONVO_SOUNDS_REGISTERED) G_InitNPCConversationSounds();

	if (NPC->NPC->conversationSearchTime > level.time) return;

	NPC->NPC->conversationSearchTime = level.time + 5000 + irand(0, 10000);

	if (NPC->client->NPC_class == CLASS_STORMTROOPER 
		|| NPC->client->NPC_class == CLASS_STORMTROOPER_ADVANCED 
		|| NPC->client->NPC_class == CLASS_STORMTROOPER_ATST_PILOT
		|| NPC->client->NPC_class == CLASS_STORMTROOPER_ATAT_PILOT)
	{
		NPC_StormtrooperFindConversationPartner(aiEnt);
		return;
	}

	if (!NPC_HasConversationSounds(NPC)) 
		return;

	//if (VectorLength(NPC->client->ps.velocity) <= 16)
	{// I'm not mooving... Conversaion possible...
		int i = 0;
		gentity_t *partner = NULL;

		// Make sure there are no other conversations going on...
		for (i = MAX_CLIENTS; i < MAX_GENTITIES; i++)
		{
			partner = &g_entities[i];

			if (!partner) continue;
			if (partner == NPC) continue;
			if (partner->s.eType != ET_NPC) continue;
			if (!partner->client) continue;
			if (!partner->NPC) continue;
			if (partner->client->NPC_class == CLASS_STORMTROOPER 
				|| partner->client->NPC_class == CLASS_STORMTROOPER_ADVANCED 
				|| partner->client->NPC_class == CLASS_STORMTROOPER_ATST_PILOT
				|| partner->client->NPC_class == CLASS_STORMTROOPER_ATAT_PILOT)
				continue;
			if (partner->client->NPC_class == NPC->client->NPC_class) continue;
			//if (!Q_stricmpn(partner->NPC_type, NPC->NPC_type, strlen(partner->NPC_type)-1)) continue; // never talk to the same race... (they would repeat eachother)
			if (partner->NPC->conversationPartner || partner->NPC->conversationReplyTime > level.time)
				if (Distance(partner->r.currentOrigin, NPC->r.currentOrigin) < 512/*1024*/)//2048)
					return; // We don't want them talking too close to others having the same conversations :)
		}

		for (i = MAX_CLIENTS; i < MAX_GENTITIES; i++)
		{
			partner = &g_entities[i];

			if (!partner) continue;
			if (partner == NPC) continue;
			if (partner->s.eType != ET_NPC) continue;
			if (!partner->client) continue;
			if (!partner->NPC) continue;
			if (partner->client->NPC_class == CLASS_STORMTROOPER 
				|| partner->client->NPC_class == CLASS_STORMTROOPER_ADVANCED 
				|| partner->client->NPC_class == CLASS_STORMTROOPER_ATST_PILOT
				|| partner->client->NPC_class == CLASS_STORMTROOPER_ATAT_PILOT)
				continue;
			if (partner->client->NPC_class == NPC->client->NPC_class) continue;
			//if (!Q_stricmpn(partner->NPC_type, NPC->NPC_type, strlen(partner->NPC_type)-1)) continue; // never talk to the same race... (they would repeat eachother)
			if (VectorLength(partner->client->ps.velocity) > 16 && Distance(partner->r.currentOrigin, NPC->r.currentOrigin) > 96) continue;
			if (!NPC_HasConversationSounds(partner)) continue;

			if (partner->NPC->conversationPartner || partner->NPC->conversationReplyTime > level.time)
			{// this one already in a convo...
				if (Distance(partner->r.currentOrigin, NPC->r.currentOrigin) < 512/*1024*/)//2048)
					return; // We don't want them talking too close to others having the same conversations :)

				continue;
			}

			if (Distance(partner->r.currentOrigin, NPC->r.currentOrigin) > 128) continue;

			// Looks good! Start a convo...
			NPC->NPC->conversationSection = 1;
			NPC->NPC->conversationRole = 1;
			NPC->NPC->conversationPart = 1;

			if (partner->NPC)
				NPC->NPC->conversationPartner = partner;

			if (NPC->NPC->conversationPartner->NPC)
			{
				NPC->NPC->conversationPartner->NPC->conversationPartner = NPC;
				NPC->NPC->conversationPartner->NPC->conversationRole = 2;
				NPC->NPC->conversationPartner->NPC->conversationSection = 1;
				NPC->NPC->conversationPartner->NPC->conversationPart = 1;
				NPC->NPC->conversationPartner->NPC->conversationReplyTime = level.time + 8000;

#ifdef __NPC_CONVERSATION_DEBUG__
				trap->Print(">> NPC %i (%s) enterred a conversation with NPC %i (%s).\n", NPC->s.number, NPC->NPC_type, NPC->NPC->conversationPartner->s.number, NPC->NPC->conversationPartner->NPC_type);
#endif //__NPC_CONVERSATION_DEBUG__
				NPC_NPCConversation(aiEnt);
			}
			else
			{
				NPC_EndConversation(aiEnt);
				continue;
			}
			return;
		}
	}
#endif //__NPC_CONVERSATIONS__
}

#endif //__DOMINANCE_NPC__