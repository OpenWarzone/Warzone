#include "b_local.h"
#include "g_nav.h"
#include "anims.h"

extern int gWPNum;
extern wpobject_t *gWPArray[MAX_WPARRAY_SIZE];

extern int DOM_GetNearestWP(vec3_t org, int badwp);
extern int NPC_GetNextNode(gentity_t *NPC);
extern qboolean UQ1_UcmdMoveForDir ( gentity_t *self, usercmd_t *cmd, vec3_t dir, qboolean walk, vec3_t dest );

extern void ST_Speech( gentity_t *self, int speechType, float failChance );

qboolean NPC_FuturePathIsSafe( gentity_t *self )
{
	int wp = 0;

	if (!self) return qfalse;
	if (self->pathsize <= 0) return qfalse;
	if (self->wpCurrent < 0 || self->wpCurrent >= gWPNum) return qfalse;
	if (self->wpNext < 0 || self->wpNext >= gWPNum) return qfalse;

	for (wp = 0; wp < self->pathsize; wp++)
	{// See if our future path has no hostile NPCs/Bots around...
		int				i, num;
		int				touch[MAX_GENTITIES];
		gentity_t		*NPC;
		vec3_t			mins, maxs, position;
		static vec3_t	range = { 512, 512, 52 };
		qboolean		bad = qfalse;

		if (Distance(gWPArray[self->pathlist[wp]]->origin, self->r.currentOrigin) < 512) continue; // Too close to start point, ignore...

		// Ok, check that this position has no combatants nearby...
		VectorCopy(gWPArray[self->pathlist[wp]]->origin, position);

		VectorSubtract( position, range, mins );
		VectorAdd( position, range, maxs );

		num = trap->EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

		for ( i=0 ; i<num ; i++ ) {
			NPC = &g_entities[touch[i]];

			if ( !NPC ) {
				continue;
			}
			if ( NPC->s.eType != ET_NPC && NPC->s.eType != ET_PLAYER ) {
				continue;
			}
			if (NPC->s.eType == ET_NPC && NPC_IsCivilian(NPC)) {
				continue;
			}

			// Ok looks like this spot is bad... Combatants found here...
			bad = qtrue;
			break;
		}

		if (!bad) return qtrue; // We found a spot of safety...
	}

	return qfalse; // We checked their whole path, and no safe spot was found...
}

void NPC_CivilianCowerPoint( gentity_t *enemy, vec3_t position )
{
	int				i, num;
	int				touch[MAX_GENTITIES];
	gentity_t		*NPC;
	vec3_t			mins, maxs;
	static vec3_t	range = { 512, 512, 52 };

	VectorSubtract( position, range, mins );
	VectorAdd( position, range, maxs );

	num = trap->EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for ( i=0 ; i<num ; i++ ) {
		NPC = &g_entities[touch[i]];

		if ( !NPC ) {
			continue;
		}
		if ( NPC->s.eType != ET_NPC ) {
			continue;
		}
		if (!NPC_IsCivilian(NPC)) {
			continue;
		}

		//
		// This one should cower...
		//

		if (NPC->npc_cower_runaway || NPC_FuturePathIsSafe(NPC))
		{// Their future path is clear of combatants, make them run away...
			if ( NPC->npc_cower_time < level.time && TIMER_Done( NPC, "flee" ) && TIMER_Done( NPC, "panic" ) )
			{
				ST_Speech( NPC, SPEECH_COVER, 0 );//FIXME: flee sound?
			}

			NPC->npc_cower_runaway_anim = 0; // Select new cower animation...
			NPC->npc_cower_runaway = qtrue;
		}
		else if ( NPC->npc_cower_time < level.time && TIMER_Done( NPC, "flee" ) && TIMER_Done( NPC, "panic" ) )
		{
#if 0
			G_StartFlee( NPC, enemy, position, AEL_DANGER_GREAT, 3000, 5000 );
#endif
			ST_Speech( NPC, SPEECH_COVER, 0 );//FIXME: flee sound?
			NPC->npc_cower_runaway = qfalse;
		}

		NPC->npc_cower_time = level.time + 12000 + irand(0, 18000);
	}
}
