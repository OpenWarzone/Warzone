#include "b_local.h"
#include "g_nav.h"
#include "icarus/Q3_Interface.h"

extern qboolean NPC_SomeoneLookingAtMe(gentity_t *ent);

extern void Jedi_SetEnemyInfo(gentity_t *aiEnt, vec3_t enemy_dest, vec3_t enemy_dir, float *enemy_dist, vec3_t enemy_movedir, float *enemy_movespeed, int prediction );
extern void Jedi_EvasionSaber(gentity_t *aiEnt, vec3_t enemy_movedir, float enemy_dist, vec3_t enemy_dir );

qboolean NPC_IsHumanoid ( gentity_t *self )
{
	switch (self->client->NPC_class)
	{
	//case CLASS_ATST:
	case CLASS_BARTENDER:
	case CLASS_BESPIN_COP:		
	case CLASS_CLAW:
	case CLASS_COMMANDO:
	case CLASS_DESANN:		
	//case CLASS_FISH:
	//case CLASS_FLIER2:
	case CLASS_GALAK:
	//case CLASS_GLIDER:
	//case CLASS_GONK:				// droid
	case CLASS_GRAN:
	//case CLASS_HOWLER:
	case CLASS_IMPERIAL:
	case CLASS_IMPWORKER:
	//case CLASS_INTERROGATOR:		// droid 
	case CLASS_JAN:				
	case CLASS_JEDI:
	case CLASS_PADAWAN:
	case CLASS_HK51:
	case CLASS_NATIVE:
	case CLASS_NATIVE_GUNNER:
	case CLASS_KYLE:				
	case CLASS_LANDO:			
	//case CLASS_LIZARD:
	case CLASS_LUKE:				// UQ1: TODO - maybe should be allowed to switch to pistol/blaster???
	//case CLASS_MARK1:			// droid
	//case CLASS_MARK2:			// droid
	//case CLASS_GALAKMECH:		// droid
	//case CLASS_MINEMONSTER:
	case CLASS_MONMOTHA:			
	case CLASS_MORGANKATARN:
	//case CLASS_MOUSE:			// droid
	//case CLASS_MURJJ:
	case CLASS_PRISONER:
	//case CLASS_PROBE:			// droid
	case CLASS_PROTOCOL:			// droid
	//case CLASS_R2D2:				// droid
	//case CLASS_R5D2:				// droid
	case CLASS_REBEL:
	case CLASS_REBORN:
	case CLASS_REELO:
	//case CLASS_REMOTE:
	case CLASS_RODIAN:
	//case CLASS_SEEKER:			// droid
	//case CLASS_SENTRY:
	case CLASS_SHADOWTROOPER:
	case CLASS_STORMTROOPER:
	case CLASS_STORMTROOPER_ADVANCED:
	case CLASS_SWAMP:
	case CLASS_SWAMPTROOPER:
	case CLASS_TAVION:
	case CLASS_TRANDOSHAN:
	case CLASS_UGNAUGHT:
	case CLASS_JAWA:
	case CLASS_WEEQUAY:
	case CLASS_BOBAFETT:
	//case CLASS_VEHICLE:
	//case CLASS_RANCOR:
	//case CLASS_WAMPA:
	case CLASS_CIVILIAN:			// UQ1: Random civilian NPCs...
	case CLASS_GENERAL_VENDOR:
	case CLASS_WEAPONS_VENDOR:
	case CLASS_ARMOR_VENDOR:
	case CLASS_SUPPLIES_VENDOR:
	case CLASS_FOOD_VENDOR:
	case CLASS_MEDICAL_VENDOR:
	case CLASS_GAMBLER_VENDOR:
	case CLASS_TRADE_VENDOR:
	case CLASS_ODDITIES_VENDOR:
	case CLASS_DRUG_VENDOR:
	case CLASS_TRAVELLING_VENDOR:
		// Humanoid...
		return qtrue;
		break;
	default:
		// NOT Humanoid...
		break;
	}

	return qfalse;
}

qboolean NPC_IsJedi ( gentity_t *self )
{
	switch (self->client->NPC_class)
	{
	case CLASS_ALORA:
	case CLASS_DESANN:					
	case CLASS_JEDI:
	case CLASS_PADAWAN:
	case CLASS_KYLE:				
	case CLASS_LUKE:				// UQ1: TODO - maybe should be allowed to switch to pistol/blaster???			
	case CLASS_MORGANKATARN:
	case CLASS_REBORN:
	//case CLASS_SABER_DROID:
	case CLASS_SHADOWTROOPER:
	case CLASS_TAVION:
		// Is Jedi...
		return qtrue;
		break;
	default:
		// NOT Jedi...
		break;
	}

	return qfalse;
}

qboolean NPC_IsLightJedi ( gentity_t *self )
{
	switch (self->client->NPC_class)
	{
	case CLASS_JEDI:
	case CLASS_PADAWAN:
	case CLASS_KYLE:				
	case CLASS_LUKE:				// UQ1: TODO - maybe should be allowed to switch to pistol/blaster???
	case CLASS_MONMOTHA:			
	case CLASS_MORGANKATARN:
		// Is Jedi...
		return qtrue;
		break;
	default:
		// NOT Jedi...
		break;
	}

	return qfalse;
}

qboolean NPC_IsDarkJedi ( gentity_t *self )
{
	switch (self->client->NPC_class)
	{
	case CLASS_ALORA:
	case CLASS_DESANN:		
	case CLASS_REBORN:
	//case CLASS_SABER_DROID:
	case CLASS_SHADOWTROOPER:
	case CLASS_TAVION:
		// Is Jedi...
		return qtrue;
		break;
	default:
		// NOT Jedi...
		break;
	}

	return qfalse;
}

qboolean NPC_IsAnimalEnemyFaction ( gentity_t *self )
{
	switch (self->client->NPC_class)
	{
	//case CLASS_ATST:
	//case CLASS_BARTENDER:
	//case CLASS_BESPIN_COP:		
	case CLASS_CLAW:
	//case CLASS_COMMANDO:
	//case CLASS_DESANN:		
	//case CLASS_FISH:
	case CLASS_FLIER2:
	//case CLASS_GALAK:
	case CLASS_GLIDER:
	//case CLASS_GONK:				// droid
	//case CLASS_GRAN:
	case CLASS_HOWLER:
	case CLASS_REEK:
	case CLASS_NEXU:
	case CLASS_ACKLAY:
	//case CLASS_IMPERIAL:
	//case CLASS_IMPWORKER:
	//case CLASS_INTERROGATOR:		// droid 
	//case CLASS_JAN:				
	//case CLASS_JEDI:
	//case CLASS_PADAWAN:
	//case CLASS_KYLE:				
	//case CLASS_LANDO:			
	case CLASS_LIZARD:
	//case CLASS_LUKE:				// UQ1: TODO - maybe should be allowed to switch to pistol/blaster???
	//case CLASS_MARK1:			// droid
	//case CLASS_MARK2:			// droid
	//case CLASS_GALAKMECH:		// droid
	case CLASS_MINEMONSTER:
	//case CLASS_MONMOTHA:			
	//case CLASS_MORGANKATARN:
	//case CLASS_MOUSE:			// droid
	//case CLASS_MURJJ:
	//case CLASS_PRISONER:
	//case CLASS_PROBE:			// droid
	//case CLASS_PROTOCOL:			// droid
	//case CLASS_R2D2:				// droid
	//case CLASS_R5D2:				// droid
	//case CLASS_REBEL:
	//case CLASS_REBORN:
	//case CLASS_REELO:
	//case CLASS_REMOTE:
	//case CLASS_RODIAN:
	//case CLASS_SEEKER:			// droid
	//case CLASS_SENTRY:
	//case CLASS_SHADOWTROOPER:
	//case CLASS_STORMTROOPER:
	//case CLASS_SWAMP:
	//case CLASS_SWAMPTROOPER:
	//case CLASS_TAVION:
	//case CLASS_TRANDOSHAN:
	//case CLASS_UGNAUGHT:
	//case CLASS_JAWA:
	//case CLASS_WEEQUAY:
	//case CLASS_BOBAFETT:
	//case CLASS_VEHICLE:
	case CLASS_RANCOR:
	case CLASS_WAMPA:
		// Is Enemy Animal...
		return qtrue;
		break;
	default:
		// NOT Enemy Animal...
		break;
	}

	return qfalse;
}

qboolean NPC_IsBountyHunter ( gentity_t *self )
{
	switch (self->client->NPC_class)
	{
	//case CLASS_ATST:
	//case CLASS_BARTENDER:
	//case CLASS_BESPIN_COP:		
	//case CLASS_CLAW:
	//case CLASS_COMMANDO:
	//case CLASS_DESANN:		
	//case CLASS_FISH:
	//case CLASS_FLIER2:
	//case CLASS_GALAK:
	//case CLASS_GLIDER:
	//case CLASS_GONK:				// droid
	//case CLASS_GRAN:
	//case CLASS_HOWLER:
	//case CLASS_IMPERIAL:
	//case CLASS_IMPWORKER:
	//case CLASS_INTERROGATOR:		// droid 
	//case CLASS_JAN:				
	//case CLASS_JEDI:
	//case CLASS_PADAWAN:
	//case CLASS_KYLE:				
	//case CLASS_LANDO:			
	//case CLASS_LIZARD:
	//case CLASS_LUKE:				// UQ1: TODO - maybe should be allowed to switch to pistol/blaster???
	//case CLASS_MARK1:			// droid
	//case CLASS_MARK2:			// droid
	//case CLASS_GALAKMECH:		// droid
	//case CLASS_MINEMONSTER:
	//case CLASS_MONMOTHA:			
	//case CLASS_MORGANKATARN:
	//case CLASS_MOUSE:			// droid
	//case CLASS_MURJJ:
	//case CLASS_PRISONER:
	//case CLASS_PROBE:			// droid
	//case CLASS_PROTOCOL:			// droid
	//case CLASS_R2D2:				// droid
	//case CLASS_R5D2:				// droid
	//case CLASS_REBEL:
	//case CLASS_REBORN:
	//case CLASS_REELO:
	//case CLASS_REMOTE:
	//case CLASS_RODIAN:
	//case CLASS_SEEKER:			// droid
	//case CLASS_SENTRY:
	//case CLASS_SHADOWTROOPER:
	//case CLASS_STORMTROOPER:
	//case CLASS_SWAMP:
	//case CLASS_SWAMPTROOPER:
	//case CLASS_TAVION:
	//case CLASS_TRANDOSHAN:
	//case CLASS_UGNAUGHT:
	//case CLASS_JAWA:
	//case CLASS_WEEQUAY:
	case CLASS_BOBAFETT:
	//case CLASS_VEHICLE:
	//case CLASS_RANCOR:
	//case CLASS_WAMPA:
		// Is Bounty Hunter...
		return qtrue;
		break;
	default:
		// NOT Bounty Hunter...
		break;
	}

	return qfalse;
}

qboolean NPC_IsCommando ( gentity_t *self )
{
	switch (self->client->NPC_class)
	{
	//case CLASS_ATST:
	//case CLASS_BARTENDER:
	//case CLASS_BESPIN_COP:		
	//case CLASS_CLAW:
	case CLASS_COMMANDO:
	//case CLASS_DESANN:		
	//case CLASS_FISH:
	//case CLASS_FLIER2:
	//case CLASS_GALAK:
	//case CLASS_GLIDER:
	//case CLASS_GONK:				// droid
	//case CLASS_GRAN:
	//case CLASS_HOWLER:
	case CLASS_MERC:
	//case CLASS_IMPERIAL:
	//case CLASS_IMPWORKER:
	//case CLASS_INTERROGATOR:		// droid 
	case CLASS_JAN:				
	//case CLASS_JEDI:
	//case CLASS_PADAWAN:
	//case CLASS_KYLE:				
	//case CLASS_LANDO:			
	//case CLASS_LIZARD:
	//case CLASS_LUKE:				// UQ1: TODO - maybe should be allowed to switch to pistol/blaster???
	//case CLASS_MARK1:			// droid
	//case CLASS_MARK2:			// droid
	//case CLASS_GALAKMECH:		// droid
	//case CLASS_MINEMONSTER:
	//case CLASS_MONMOTHA:			
	//case CLASS_MORGANKATARN:
	//case CLASS_MOUSE:			// droid
	//case CLASS_MURJJ:
	//case CLASS_PRISONER:
	//case CLASS_PROBE:			// droid
	//case CLASS_PROTOCOL:			// droid
	//case CLASS_R2D2:				// droid
	//case CLASS_R5D2:				// droid
	//case CLASS_REBEL:
	//case CLASS_REBORN:
	//case CLASS_REELO:
	//case CLASS_REMOTE:
	//case CLASS_RODIAN:
	//case CLASS_SEEKER:			// droid
	//case CLASS_SENTRY:
	//case CLASS_SHADOWTROOPER:
	//case CLASS_STORMTROOPER:
	//case CLASS_SWAMP:
	//case CLASS_SWAMPTROOPER:
	//case CLASS_TAVION:
	//case CLASS_TRANDOSHAN:
	//case CLASS_UGNAUGHT:
	//case CLASS_JAWA:
	//case CLASS_WEEQUAY:
	//case CLASS_BOBAFETT:
	//case CLASS_VEHICLE:
	//case CLASS_RANCOR:
	//case CLASS_WAMPA:
		// Is Commando...
		return qtrue;
		break;
	default:
		// NOT Commando...
		break;
	}

	return qfalse;
}

qboolean NPC_IsAdvancedGunner ( gentity_t *self )
{
	switch (self->client->NPC_class)
	{
	//case CLASS_ATST:
	//case CLASS_BARTENDER:
	case CLASS_BESPIN_COP:		
	//case CLASS_CLAW:
	//case CLASS_COMMANDO:
	//case CLASS_DESANN:		
	//case CLASS_FISH:
	//case CLASS_FLIER2:
	//case CLASS_GALAK:
	//case CLASS_GLIDER:
	//case CLASS_GONK:				// droid
	//case CLASS_GRAN:
	//case CLASS_HOWLER:
	//case CLASS_IMPERIAL:
	//case CLASS_IMPWORKER:
	//case CLASS_INTERROGATOR:		// droid 
	//case CLASS_JAN:				
	//case CLASS_JEDI:
	//case CLASS_PADAWAN:
	//case CLASS_KYLE:				
	case CLASS_LANDO:			
	//case CLASS_LIZARD:
	//case CLASS_LUKE:				// UQ1: TODO - maybe should be allowed to switch to pistol/blaster???
	//case CLASS_MARK1:			// droid
	//case CLASS_MARK2:			// droid
	//case CLASS_GALAKMECH:		// droid
	//case CLASS_MINEMONSTER:
	//case CLASS_MONMOTHA:			
	//case CLASS_MORGANKATARN:
	//case CLASS_MOUSE:			// droid
	//case CLASS_MURJJ:
	//case CLASS_PRISONER:
	//case CLASS_PROBE:			// droid
	//case CLASS_PROTOCOL:			// droid
	//case CLASS_R2D2:				// droid
	//case CLASS_R5D2:				// droid
	//case CLASS_REBEL:
	//case CLASS_REBORN:
	//case CLASS_REELO:
	//case CLASS_REMOTE:
	//case CLASS_RODIAN:
	//case CLASS_SEEKER:			// droid
	//case CLASS_SENTRY:
	//case CLASS_SHADOWTROOPER:
	//case CLASS_STORMTROOPER:
	case CLASS_STORMTROOPER_ADVANCED:
	//case CLASS_SWAMP:
	//case CLASS_SWAMPTROOPER:
	//case CLASS_TAVION:
	//case CLASS_TRANDOSHAN:
	//case CLASS_UGNAUGHT:
	//case CLASS_JAWA:
	//case CLASS_WEEQUAY:
	//case CLASS_BOBAFETT:
	//case CLASS_VEHICLE:
	//case CLASS_RANCOR:
	//case CLASS_WAMPA:
		// Is Commando...
		return qtrue;
		break;
	default:
		// NOT Commando...
		break;
	}

	return qfalse;
}

qboolean NPC_IsGunner(gentity_t *self)
{
	switch (self->client->NPC_class)
	{
	case CLASS_NATIVE_GUNNER:
		// Is Gunner...
		return qtrue;
		break;
	default:
		// NOT Commando...
		break;
	}

	return qfalse;
}

qboolean NPC_IsNative(gentity_t *self)
{
	switch (self->client->NPC_class)
	{
	case CLASS_NATIVE:
	case CLASS_NATIVE_GUNNER:
		// Is Gunner...
		return qtrue;
		break;
	default:
		// NOT Commando...
		break;
	}

	return qfalse;
}

qboolean NPC_IsFollowerGunner(gentity_t *self)
{
	switch (self->client->NPC_class)
	{
	case CLASS_HK51:
		// Is Gunner...
		return qtrue;
		break;
	default:
		// NOT Commando...
		break;
	}

	return qfalse;
}

qboolean NPC_IsStormtrooper ( gentity_t *self )
{
	switch (self->client->NPC_class)
	{
	//case CLASS_ATST:
	//case CLASS_BARTENDER:
	//case CLASS_BESPIN_COP:		
	//case CLASS_CLAW:
	//case CLASS_COMMANDO:
	//case CLASS_DESANN:		
	//case CLASS_FISH:
	//case CLASS_FLIER2:
	//case CLASS_GALAK:
	//case CLASS_GLIDER:
	//case CLASS_GONK:				// droid
	//case CLASS_GRAN:
	//case CLASS_HOWLER:
	//case CLASS_IMPERIAL:
	case CLASS_IMPWORKER:
	//case CLASS_INTERROGATOR:		// droid 
	//case CLASS_JAN:				
	//case CLASS_JEDI:
	//case CLASS_PADAWAN:
	//case CLASS_KYLE:				
	//case CLASS_LANDO:			
	//case CLASS_LIZARD:
	//case CLASS_LUKE:				// UQ1: TODO - maybe should be allowed to switch to pistol/blaster???
	//case CLASS_MARK1:			// droid
	//case CLASS_MARK2:			// droid
	//case CLASS_GALAKMECH:		// droid
	//case CLASS_MINEMONSTER:
	//case CLASS_MONMOTHA:			
	//case CLASS_MORGANKATARN:
	//case CLASS_MOUSE:			// droid
	//case CLASS_MURJJ:
	//case CLASS_PRISONER:
	//case CLASS_PROBE:			// droid
	//case CLASS_PROTOCOL:			// droid
	//case CLASS_R2D2:				// droid
	//case CLASS_R5D2:				// droid
	//case CLASS_REBEL:
	//case CLASS_REBORN:
	//case CLASS_REELO:
	//case CLASS_REMOTE:
	//case CLASS_RODIAN:
	//case CLASS_SEEKER:			// droid
	//case CLASS_SENTRY:
	//case CLASS_SHADOWTROOPER:
	case CLASS_STORMTROOPER:
	case CLASS_STORMTROOPER_ADVANCED:
	//case CLASS_SWAMP:
	case CLASS_SWAMPTROOPER:
	//case CLASS_TAVION:
	//case CLASS_TRANDOSHAN:
	//case CLASS_UGNAUGHT:
	//case CLASS_JAWA:
	//case CLASS_WEEQUAY:
	//case CLASS_BOBAFETT:
	//case CLASS_VEHICLE:
	//case CLASS_RANCOR:
	//case CLASS_WAMPA:
		// Is Commando...
		return qtrue;
		break;
	default:
		// NOT Commando...
		break;
	}

	return qfalse;
}

qboolean NPC_HasGrenades ( gentity_t *self )
{
	switch (self->client->NPC_class)
	{
	//case CLASS_ATST:
	//case CLASS_BARTENDER:
	case CLASS_BESPIN_COP:		
	//case CLASS_CLAW:
	case CLASS_COMMANDO:
	//case CLASS_DESANN:		
	//case CLASS_FISH:
	//case CLASS_FLIER2:
	//case CLASS_GALAK:
	//case CLASS_GLIDER:
	//case CLASS_GONK:				// droid
	//case CLASS_GRAN:
	//case CLASS_HOWLER:
	case CLASS_IMPERIAL:
	//case CLASS_IMPWORKER:
	//case CLASS_INTERROGATOR:		// droid 
	case CLASS_JAN:				
	//case CLASS_JEDI:
	//case CLASS_PADAWAN:
	//case CLASS_KYLE:				
	case CLASS_LANDO:			
	//case CLASS_LIZARD:
	//case CLASS_LUKE:				// UQ1: TODO - maybe should be allowed to switch to pistol/blaster???
	//case CLASS_MARK1:			// droid
	//case CLASS_MARK2:			// droid
	//case CLASS_GALAKMECH:		// droid
	//case CLASS_MINEMONSTER:
	//case CLASS_MONMOTHA:			
	case CLASS_MORGANKATARN:
	//case CLASS_MOUSE:			// droid
	//case CLASS_MURJJ:
	case CLASS_PRISONER:
	//case CLASS_PROBE:			// droid
	//case CLASS_PROTOCOL:			// droid
	//case CLASS_R2D2:				// droid
	//case CLASS_R5D2:				// droid
	case CLASS_REBEL:
	//case CLASS_REBORN:
	//case CLASS_REELO:
	//case CLASS_REMOTE:
	//case CLASS_RODIAN:
	//case CLASS_SEEKER:			// droid
	//case CLASS_SENTRY:
	//case CLASS_SHADOWTROOPER:
	//case CLASS_STORMTROOPER:
	case CLASS_STORMTROOPER_ADVANCED:
	//case CLASS_SWAMP:
	case CLASS_SWAMPTROOPER:
	//case CLASS_TAVION:
	case CLASS_TRANDOSHAN:
	//case CLASS_UGNAUGHT:
	//case CLASS_JAWA:
	case CLASS_WEEQUAY:
	case CLASS_BOBAFETT:
	//case CLASS_VEHICLE:
	//case CLASS_RANCOR:
	//case CLASS_WAMPA:
	case CLASS_HK51:
	case CLASS_NATIVE:
	case CLASS_NATIVE_GUNNER:
		// Has Grenades...
		return qtrue;
		break;
	default:
		// Does not have grenades...
		break;
	}

	return qfalse;
}

void NPC_CheckEvasion(gentity_t *aiEnt)
{
	vec3_t		enemy_dir, enemy_movedir, enemy_dest;
	float		enemy_dist, enemy_movespeed;
	gentity_t	*NPC = aiEnt;

	if ( !NPC->enemy || !NPC->enemy->inuse || (NPC->enemy->NPC && NPC->enemy->health <= 0) )
	{
		return;
	}

	// Who can evade???
	switch (NPC->client->NPC_class)
	{
	//case CLASS_ATST:
	case CLASS_BARTENDER:
	case CLASS_BESPIN_COP:		
	case CLASS_CLAW:
	case CLASS_COMMANDO:
	case CLASS_DESANN:		
	//case CLASS_FISH:
	//case CLASS_FLIER2:
	case CLASS_GALAK:
	//case CLASS_GLIDER:
	//case CLASS_GONK:				// droid
	case CLASS_GRAN:
	//case CLASS_HOWLER:
	case CLASS_IMPERIAL:
	case CLASS_IMPWORKER:
	//case CLASS_INTERROGATOR:		// droid 
	case CLASS_JAN:				
	case CLASS_JEDI:
	case CLASS_PADAWAN:
	case CLASS_HK51:
	case CLASS_NATIVE:
	case CLASS_NATIVE_GUNNER:
	case CLASS_KYLE:				
	case CLASS_LANDO:			
	//case CLASS_LIZARD:
	case CLASS_LUKE:				
	case CLASS_MARK1:			// droid
	case CLASS_MARK2:			// droid
	//case CLASS_GALAKMECH:		// droid
	//case CLASS_MINEMONSTER:
	case CLASS_MONMOTHA:			
	case CLASS_MORGANKATARN:
	//case CLASS_MOUSE:			// droid
	case CLASS_MURJJ:
	case CLASS_PRISONER:
	//case CLASS_PROBE:			// droid
	case CLASS_PROTOCOL:			// droid
	//case CLASS_R2D2:				// droid
	//case CLASS_R5D2:				// droid
	case CLASS_REBEL:
	case CLASS_REBORN:
	case CLASS_REELO:
	//case CLASS_REMOTE:
	case CLASS_RODIAN:
	//case CLASS_SEEKER:			// droid
	//case CLASS_SENTRY:
	case CLASS_SHADOWTROOPER:
	case CLASS_STORMTROOPER:
	case CLASS_STORMTROOPER_ADVANCED:
	case CLASS_SWAMP:
	case CLASS_SWAMPTROOPER:
	case CLASS_TAVION:
	case CLASS_TRANDOSHAN:
	case CLASS_UGNAUGHT:
	case CLASS_JAWA:
	case CLASS_WEEQUAY:
	case CLASS_BOBAFETT:
	//case CLASS_VEHICLE:
	//case CLASS_RANCOR:
	//case CLASS_WAMPA:
		// OK... EVADE AWAY!!!
		break;
	default:
		// NOT OK...
		return;
		break;
	}

	//See where enemy will be 300 ms from now
	Jedi_SetEnemyInfo( NPC, enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed, 300 );

	if ( NPC->enemy->s.weapon == WP_SABER )
	{
		Jedi_EvasionSaber( NPC, enemy_movedir, enemy_dist, enemy_dir );
	}
	else
	{//do we need to do any evasion for other kinds of enemies?
		if (NPC->enemy->client)
		{
			vec3_t shotDir, ang;

			VectorSubtract(NPC->r.currentOrigin, NPC->enemy->r.currentOrigin, shotDir);
			vectoangles(shotDir, ang);

			if (NPC->enemy->client->ps.rocketLockTime != 0 && InFieldOfVision(NPC->enemy->client->ps.viewangles, /*60*/90, ang))
			{// They are about to shoot a rocket at us! Evade!!!
				Jedi_EvasionSaber( NPC, enemy_movedir, enemy_dist, enemy_dir );
			}
			else if (NPC->enemy->client->ps.weaponstate == WEAPON_FIRING && InFieldOfVision(NPC->enemy->client->ps.viewangles, /*60*/90, ang))
			{// They are shooting at us. Evade!!!
				Jedi_EvasionSaber(NPC, enemy_movedir, enemy_dist, enemy_dir );
			}
			else if (/*irand(0,100) <= 50 &&*/ InFieldOfVision(NPC->enemy->client->ps.viewangles, 60/*40*/, ang))
			{// Randomly (when they are targetting us)... Evade!!!
				Jedi_EvasionSaber(NPC, enemy_movedir, enemy_dist, enemy_dir );
			}
			else
			{// Check for nearby missiles/grenades to evade...
				int i;
				int entities[MAX_GENTITIES];
				int numEnts = 0;//trap_EntitiesInBox(ent->r.absmin, ent->r.absmax, entities, MAX_GENTITIES);

				vec3_t mins;
				vec3_t maxs;
				int e = 0;

				for ( e = 0 ; e < 3 ; e++ ) 
				{
					mins[e] = NPC->r.currentOrigin[e] - 256;
					maxs[e] = NPC->r.currentOrigin[e] + 256;
				}

				numEnts = trap->EntitiesInBox(mins, maxs, entities, MAX_GENTITIES);

				for (i = 0; i < numEnts; i++)
				{
					gentity_t * missile = &g_entities[i];

					if (!missile) continue;
					if (!missile->inuse) continue;

					if (missile->s.eType == ET_MISSILE)
					{// Missile incoming!!! Evade!!!
						Jedi_EvasionSaber(NPC, enemy_movedir, enemy_dist, enemy_dir );
						return;
					}
				}
			}
		}
	}
}

/*
void NPC_LostEnemyDecideChase(void)

  We lost our enemy and want to drop him but see if we should chase him if we are in the proper bState
*/

void NPC_LostEnemyDecideChase(gentity_t *aiEnt)
{
	switch( aiEnt->NPC->behaviorState )
	{
	case BS_HUNT_AND_KILL:
		//We were chasing him and lost him, so try to find him
		if ( aiEnt->enemy == aiEnt->NPC->goalEntity && aiEnt->enemy->lastWaypoint != WAYPOINT_NONE )
		{//Remember his last valid Wp, then check it out
			//FIXME: Should we only do this if there's no other enemies or we've got LOCKED_ENEMY on?
			NPC_BSSearchStart( aiEnt, aiEnt->enemy->lastWaypoint, BS_SEARCH );
		}
		//If he's not our goalEntity, we're running somewhere else, so lose him
		break;
	default:
		break;
	}
	G_ClearEnemy( aiEnt );
}
/*
-------------------------
NPC_StandIdle
-------------------------
*/

void NPC_StandIdle( void )
{
/*
	//Must be done with any other animations
	if ( NPC->client->ps.legsAnimTimer != 0 )
		return;

	//Not ready to do another one
	if ( TIMER_Done( NPC, "idleAnim" ) == false )
		return;

	int anim = NPC->client->ps.legsAnim;

	if ( anim != BOTH_STAND1 && anim != BOTH_STAND2 )
		return;

	//FIXME: Account for STAND1 or STAND2 here and set the base anim accordingly
	int	baseSeq = ( anim == BOTH_STAND1 ) ? BOTH_STAND1_RANDOM1 : BOTH_STAND2_RANDOM1;

	//Must have at least one random idle animation
	//NOTENOTE: This relies on proper ordering of animations, which SHOULD be okay
	if ( PM_HasAnimation( NPC, baseSeq ) == false )
		return;

	int	newIdle = Q_irand( 0, MAX_IDLE_ANIMS-1 );

	//FIXME: Technically this could never complete.. but that's not really too likely
	while( 1 )
	{
		if ( PM_HasAnimation( NPC, baseSeq + newIdle ) )
			break;

		newIdle = Q_irand( 0, MAX_IDLE_ANIMS );
	}

	//Start that animation going
	NPC_SetAnim( NPC, SETANIM_BOTH, baseSeq + newIdle, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );

	int newTime = PM_AnimLength( NPC->client->clientInfo.animFileIndex, (animNumber_t) (baseSeq + newIdle) );

	//Don't do this again for a random amount of time
	TIMER_Set( NPC, "idleAnim", newTime + Q_irand( 2000, 10000 ) );
*/
}

qboolean NPC_StandTrackAndShoot (gentity_t *NPC, qboolean canDuck)
{
	qboolean	attack_ok = qfalse;
	qboolean	duck_ok = qfalse;
	qboolean	faced = qfalse;
	float		attack_scale = 1.0;
	gentity_t	*aiEnt = NPC;

	//First see if we're hurt bad- if so, duck
	//FIXME: if even when ducked, we can shoot someone, we should.
	//Maybe is can be shot even when ducked, we should run away to the nearest cover?
	if ( canDuck )
	{
		if ( NPC->health < 20 )
		{
		//	if( NPC->svFlags&SVF_HEALING || random() )
			if( random() )
			{
				duck_ok = qtrue;
			}
		}
		else if ( NPC->health < 40 )
		{
//			if ( NPC->svFlags&SVF_HEALING )
//			{//Medic is on the way, get down!
//				duck_ok = qtrue;
//			}
			// no more borg
///			if ( NPC->client->playerTeam!= TEAM_BORG )
//			{//Borg don't care if they're about to die
				//attack_scale will be a max of .66
//				attack_scale = NPC->health/60;
//			}
		}
	}

	//NPC_CheckEnemy( qtrue, qfalse, qtrue );

	if ( !duck_ok )
	{//made this whole part a function call
		attack_ok = NPC_CheckCanAttack( aiEnt, attack_scale, qtrue );
		faced = qtrue;
	}

	if ( canDuck && (duck_ok || (!attack_ok && aiEnt->client->ps.weaponTime <= 0)) && aiEnt->client->pers.cmd.upmove != -127 )
	{//if we didn't attack check to duck if we're not already
		if( !duck_ok )
		{
			if ( NPC->enemy->client )
			{
				if ( NPC->enemy->enemy == NPC )
				{
					if ( NPC->enemy->client->buttons & BUTTON_ATTACK )
					{//FIXME: determine if enemy fire angles would hit me or get close
						if ( NPC_CheckDefend(aiEnt, 1.0 ) )//FIXME: Check self-preservation?  Health?
						{
							duck_ok = qtrue;
						}
					}
				}
			}
		}

		if ( duck_ok )
		{//duck and don't shoot
			aiEnt->client->pers.cmd.upmove = -127;
			aiEnt->NPC->duckDebounceTime = level.time + 1000;//duck for a full second
		}
	}

	return faced;
}


void NPC_BSIdle( gentity_t *aiEnt )
{
	//FIXME if there is no nav data, we need to do something else
	// if we're stuck, try to move around it
	if ( UpdateGoal(aiEnt) )
	{
		NPC_MoveToGoal(aiEnt, qtrue );
	}

	if ( ( aiEnt->client->pers.cmd.forwardmove == 0 ) && ( aiEnt->client->pers.cmd.rightmove == 0 ) && ( aiEnt->client->pers.cmd.upmove == 0 ) )
	{
//		NPC_StandIdle(aiEnt);
	}

	NPC_UpdateAngles( aiEnt, qtrue, qtrue );
	aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
}

void NPC_BSRun (gentity_t *aiEnt)
{
	//FIXME if there is no nav data, we need to do something else
	// if we're stuck, try to move around it
	if ( UpdateGoal(aiEnt) )
	{
		NPC_MoveToGoal(aiEnt, qtrue );
	}

	NPC_UpdateAngles(aiEnt, qtrue, qtrue );
}

void NPC_BSStandGuard (gentity_t *aiEnt)
{
	//FIXME: Use Snapshot info
	if ( aiEnt->enemy == NULL )
	{//Possible to pick one up by being shot
		if( random() < 0.5 )
		{
			if(aiEnt->client->enemyTeam)
			{
				gentity_t *newenemy = NPC_PickEnemy(aiEnt, aiEnt, aiEnt->client->enemyTeam, (qboolean)(aiEnt->cantHitEnemyCounter < 10), (qboolean)(aiEnt->client->enemyTeam == NPCTEAM_PLAYER), qtrue);
				//only checks for vis if couldn't hit last enemy
				if(newenemy)
				{
					G_SetEnemy( aiEnt, newenemy );
				}
			}
		}
	}

	if ( aiEnt->enemy != NULL )
	{
		if( aiEnt->NPC->tempBehavior == BS_STAND_GUARD )
		{
			aiEnt->NPC->tempBehavior = BS_DEFAULT;
		}

		if( aiEnt->NPC->behaviorState == BS_STAND_GUARD )
		{
			aiEnt->NPC->behaviorState = BS_STAND_AND_SHOOT;
		}
	}

	NPC_UpdateAngles(aiEnt, qtrue, qtrue );
}

/*
-------------------------
NPC_BSHuntAndKill
-------------------------
*/

void NPC_BSHuntAndKill( gentity_t *aiEnt )
{
	qboolean	turned = qfalse;
	vec3_t		vec;
	float		enemyDist;
	visibility_t	oEVis;
	int			curAnim;

	// UQ1: Added evasion...
	NPC_CheckEvasion(aiEnt);

	NPC_CheckEnemy(aiEnt, (qboolean)(aiEnt->NPC->tempBehavior != BS_HUNT_AND_KILL), qfalse, qtrue );//don't find new enemy if this is tempbehav

	if ( aiEnt->enemy )
	{
		oEVis = NPCS.enemyVisibility = NPC_CheckVisibility (aiEnt, aiEnt->enemy, CHECK_FOV|CHECK_SHOOT );//CHECK_360|//CHECK_PVS|
		if(NPCS.enemyVisibility > VIS_PVS)
		{
			if ( !NPC_EnemyTooFar(aiEnt, aiEnt->enemy, 0, qtrue ) )
			{//Enemy is close enough to shoot - FIXME: this next func does this also, but need to know here for info on whether ot not to turn later
				NPC_CheckCanAttack(aiEnt, 1.0, qfalse );
				turned = qtrue;
			}
		}

		curAnim = aiEnt->client->ps.legsAnim;
		if(curAnim != BOTH_ATTACK1 && curAnim != BOTH_ATTACK2 && curAnim != BOTH_ATTACK3 && curAnim != BOTH_MELEE1 && curAnim != BOTH_MELEE2 )
		{//Don't move toward enemy if we're in a full-body attack anim
			//FIXME, use IdealDistance to determin if we need to close distance
			VectorSubtract(aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin, vec);
			enemyDist = VectorLength(vec);
			if( enemyDist > 48 && ((enemyDist*1.5)*(enemyDist*1.5) >= NPC_MaxDistSquaredForWeapon(aiEnt) ||
				oEVis != VIS_SHOOT ||
				//!(ucmd.buttons & BUTTON_ATTACK) ||
				enemyDist > IdealDistance(aiEnt)*3 ) )
			{//We should close in?
				aiEnt->NPC->goalEntity = aiEnt->enemy;

				NPC_MoveToGoal(aiEnt, qtrue );
			}
			else if(enemyDist < IdealDistance(aiEnt))
			{//We should back off?
				//if(ucmd.buttons & BUTTON_ATTACK)
				{
					aiEnt->NPC->goalEntity = aiEnt->enemy;
					aiEnt->NPC->goalRadius = 12;
					NPC_MoveToGoal(aiEnt, qtrue );

					aiEnt->client->pers.cmd.forwardmove *= -1;
					aiEnt->client->pers.cmd.rightmove *= -1;
					VectorScale( aiEnt->client->ps.moveDir, -1, aiEnt->client->ps.moveDir );

					aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
				}
			}//otherwise, stay where we are
		}
	}
	else
	{//ok, stand guard until we find an enemy
		if( aiEnt->NPC->tempBehavior == BS_HUNT_AND_KILL )
		{
			aiEnt->NPC->tempBehavior = BS_DEFAULT;
		}
		else
		{
			aiEnt->NPC->tempBehavior = BS_STAND_GUARD;
			NPC_BSStandGuard(aiEnt);
		}
		return;
	}

	if(!turned)
	{
		NPC_UpdateAngles(aiEnt, qtrue, qtrue);
	}
}

void NPC_BSStandAndShoot (gentity_t *aiEnt)
{
	//FIXME:
	//When our numbers outnumber enemies 3 to 1, or only one of them,
	//go into hunt and kill mode

	//FIXME:
	//When they're all dead, go to some script or wander off to sickbay?

	// UQ1: Added evasion...
	NPC_CheckEvasion(aiEnt);

	if(aiEnt->client->playerTeam && aiEnt->client->enemyTeam)
	{
		//FIXME: don't realize this right away- or else enemies show up and we're standing around
		/*
		if( teamNumbers[NPC->enemyTeam] == 0 )
		{//ok, stand guard until we find another enemy
			//reset our rush counter
			teamCounter[NPC->playerTeam] = 0;
			NPCInfo->tempBehavior = BS_STAND_GUARD;
			NPC_BSStandGuard();
			return;
		}*/
		/*
		//FIXME: whether to do this or not should be settable
		else if( NPC->playerTeam != TEAM_BORG )//Borg don't rush
		{
		//FIXME: In case reinforcements show up, we should wait a few seconds
		//and keep checking before rushing!
		//Also: what if not everyone on our team is going after playerTeam?
		//Also: our team count includes medics!
			if(NPC->health > 25)
			{//Can we rush the enemy?
				if(teamNumbers[NPC->enemyTeam] == 1 ||
					teamNumbers[NPC->playerTeam] >= teamNumbers[NPC->enemyTeam]*3)
				{//Only one of them or we outnumber 3 to 1
					if(teamStrength[NPC->playerTeam] >= 75 ||
						(teamStrength[NPC->playerTeam] >= 50 && teamStrength[NPC->playerTeam] > teamStrength[NPC->enemyTeam]))
					{//Our team is strong enough to rush
						teamCounter[NPC->playerTeam]++;
						if(teamNumbers[NPC->playerTeam] * 17 <= teamCounter[NPC->playerTeam])
						{//ok, we waited 1.7 think cycles on average and everyone is go, let's do it!
							//FIXME: Should we do this to everyone on our team?
							NPCInfo->behaviorState = BS_HUNT_AND_KILL;
							//FIXME: if the tide changes, we should retreat!
							//FIXME: when do we reset the counter?
							NPC_BSHuntAndKill ();
							return;
						}
					}
					else//Oops!  Something's wrong, reset the counter to rush
						teamCounter[NPC->playerTeam] = 0;
				}
				else//Oops!  Something's wrong, reset the counter to rush
					teamCounter[NPC->playerTeam] = 0;
			}
		}
		*/
	}

	NPC_CheckEnemy(aiEnt, qtrue, qfalse, qtrue);

	if(aiEnt->NPC->duckDebounceTime > level.time && aiEnt->client->ps.weapon != WP_SABER )
	{
		aiEnt->client->pers.cmd.upmove = -127;
		if(aiEnt->enemy)
		{
			NPC_CheckCanAttack(aiEnt, 1.0, qtrue);
		}
		return;
	}

	if(aiEnt->enemy)
	{
		if(!NPC_StandTrackAndShoot( aiEnt, qtrue ))
		{//That func didn't update our angles
			aiEnt->NPC->desiredYaw = aiEnt->client->ps.viewangles[YAW];
			aiEnt->NPC->desiredPitch = aiEnt->client->ps.viewangles[PITCH];
			NPC_UpdateAngles(aiEnt, qtrue, qtrue);
		}
	}
	else
	{
		aiEnt->NPC->desiredYaw = aiEnt->client->ps.viewangles[YAW];
		aiEnt->NPC->desiredPitch = aiEnt->client->ps.viewangles[PITCH];
		NPC_UpdateAngles(aiEnt, qtrue, qtrue);
//		NPC_BSIdle();//only moves if we have a goal
	}
}

void NPC_BSRunAndShoot (gentity_t *aiEnt)
{
	/*if(NPC->playerTeam && NPC->enemyTeam)
	{
		//FIXME: don't realize this right away- or else enemies show up and we're standing around
		if( teamNumbers[NPC->enemyTeam] == 0 )
		{//ok, stand guard until we find another enemy
			//reset our rush counter
			teamCounter[NPC->playerTeam] = 0;
			NPCInfo->tempBehavior = BS_STAND_GUARD;
			NPC_BSStandGuard();
			return;
		}
	}*/

	//NOTE: are we sure we want ALL run and shoot people to move this way?
	//Shouldn't it check to see if we have an enemy and our enemy is our goal?!
	//Moved that check into NPC_MoveToGoal
	//NPCInfo->combatMove = qtrue;

	// UQ1: Added evasion...
	NPC_CheckEvasion(aiEnt);

	NPC_CheckEnemy(aiEnt, qtrue, qfalse, qtrue );

	if ( aiEnt->NPC->duckDebounceTime > level.time ) // && NPCInfo->hidingGoal )
	{
		aiEnt->client->pers.cmd.upmove = -127;
		if ( aiEnt->enemy )
		{
			NPC_CheckCanAttack(aiEnt, 1.0, qfalse );
		}
		return;
	}

	if ( aiEnt->enemy )
	{
		int monitor = aiEnt->cantHitEnemyCounter;
		NPC_StandTrackAndShoot( aiEnt, qfalse );//(NPCInfo->hidingGoal != NULL) );

		if ( !(aiEnt->client->pers.cmd.buttons & BUTTON_ATTACK) && aiEnt->client->pers.cmd.upmove >= 0 && aiEnt->cantHitEnemyCounter > monitor )
		{//not crouching and not firing
			vec3_t	vec;

			VectorSubtract( aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin, vec );
			vec[2] = 0;
			if ( VectorLength( vec ) > 128 || aiEnt->cantHitEnemyCounter >= 10 )
			{//run at enemy if too far away
				//The cantHitEnemyCounter getting high has other repercussions
				//100 (10 seconds) will make you try to pick a new enemy...
				//But we're chasing, so we clamp it at 50 here
				if ( aiEnt->cantHitEnemyCounter > 60 )
				{
					aiEnt->cantHitEnemyCounter = 60;
				}

				if ( aiEnt->cantHitEnemyCounter >= (aiEnt->NPC->stats.aggression+1) * 10 )
				{
					NPC_LostEnemyDecideChase(aiEnt);
				}

				//chase and face
				aiEnt->client->pers.cmd.angles[YAW] = 0;
				aiEnt->client->pers.cmd.angles[PITCH] = 0;
				aiEnt->NPC->goalEntity = aiEnt->enemy;
				aiEnt->NPC->goalRadius = 12;
				//NAV_ClearLastRoute(NPC);
				NPC_MoveToGoal(aiEnt, qtrue );
				NPC_UpdateAngles(aiEnt, qtrue, qtrue);
			}
			else
			{
				//FIXME: this could happen if they're just on the other side
				//of a thin wall or something else blocking out shot.  That
				//would make us just stand there and not go around it...
				//but maybe it's okay- might look like we're waiting for
				//him to come out...?
				//Current solution: runs around if cantHitEnemyCounter gets
				//to 10 (1 second).
			}
		}
		else
		{//Clear the can't hit enemy counter here
			aiEnt->cantHitEnemyCounter = 0;
		}
	}
	else
	{
		if ( aiEnt->NPC->tempBehavior == BS_HUNT_AND_KILL )
		{//lost him, go back to what we were doing before
			aiEnt->NPC->tempBehavior = BS_DEFAULT;
			return;
		}

//		NPC_BSRun();//only moves if we have a goal
	}
}

//Simply turn until facing desired angles
void NPC_BSFace (gentity_t *aiEnt)
{
	//FIXME: once you stop sending turning info, they reset to whatever their delta_angles was last????
	//Once this is over, it snaps back to what it was facing before- WHY???
	if( NPC_UpdateAngles (aiEnt, qtrue, qtrue ) )
	{
#ifndef __NO_ICARUS__
		trap->ICARUS_TaskIDComplete( (sharedEntity_t *)aiEnt, TID_BSTATE );
#endif //__NO_ICARUS__

		aiEnt->NPC->desiredYaw = aiEnt->client->ps.viewangles[YAW];
		aiEnt->NPC->desiredPitch = aiEnt->client->ps.viewangles[PITCH];

		aiEnt->NPC->aimTime = 0;//ok to turn normally now
	}
}

void NPC_BSPointShoot (gentity_t *aiEnt, qboolean shoot)
{//FIXME: doesn't check for clear shot...
	vec3_t	muzzle, dir, angles, org;

	if ( !aiEnt->enemy || !aiEnt->enemy->inuse || (aiEnt->enemy->NPC && aiEnt->enemy->health <= 0) )
	{//FIXME: should still keep shooting for a second or two after they actually die...
#ifndef __NO_ICARUS__
		trap->ICARUS_TaskIDComplete( (sharedEntity_t *)aiEnt, TID_BSTATE );
#endif //__NO_ICARUS__
		goto finished;
		return;
	}

	// UQ1: Added evasion...
	NPC_CheckEvasion(aiEnt);

	CalcEntitySpot(aiEnt, SPOT_WEAPON, muzzle);
	CalcEntitySpot(aiEnt->enemy, SPOT_HEAD, org);//Was spot_org
	//Head is a little high, so let's aim for the chest:
	if ( aiEnt->enemy->client )
	{
		org[2] -= 12;//NOTE: is this enough?
	}

	VectorSubtract(org, muzzle, dir);
	vectoangles(dir, angles);

	switch( aiEnt->client->ps.weapon )
	{
	case WP_NONE:
	case WP_SABER:
		//don't do any pitch change if not holding a firing weapon
		break;
	default:
		aiEnt->NPC->desiredPitch = aiEnt->NPC->lockedDesiredPitch = AngleNormalize360(angles[PITCH]);
		break;
	}

	aiEnt->NPC->desiredYaw = aiEnt->NPC->lockedDesiredYaw = AngleNormalize360(angles[YAW]);

	if ( NPC_UpdateAngles (aiEnt, qtrue, qtrue ) )
	{//FIXME: if angles clamped, this may never work!
		//NPCInfo->shotTime = NPC->attackDebounceTime = 0;

		if ( shoot )
		{//FIXME: needs to hold this down if using a weapon that requires it, like phaser...
			aiEnt->client->pers.cmd.buttons |= BUTTON_ATTACK;
		}

		//if ( !shoot || !(NPC->svFlags & SVF_LOCKEDENEMY) )
		if (1)
		{//If locked_enemy is on, dont complete until it is destroyed...
#ifndef __NO_ICARUS__
			trap->ICARUS_TaskIDComplete( (sharedEntity_t *)aiEnt, TID_BSTATE );
#endif //__NO_ICARUS__
			goto finished;
		}
	}
	//else if ( shoot && (NPC->svFlags & SVF_LOCKEDENEMY) )
	if (0)
	{//shooting them till their dead, not aiming right at them yet...
		/*
		qboolean movingTarget = qfalse;

		if ( NPC->enemy->client )
		{
			if ( VectorLengthSquared( NPC->enemy->client->ps.velocity ) )
			{
				movingTarget = qtrue;
			}
		}
		else if ( VectorLengthSquared( NPC->enemy->s.pos.trDelta ) )
		{
			movingTarget = qtrue;
		}

		if (movingTarget )
		*/
		{
			float	dist = VectorLength( dir );
			float	yawMiss, yawMissAllow = aiEnt->enemy->r.maxs[0];
			float	pitchMiss, pitchMissAllow = (aiEnt->enemy->r.maxs[2] - aiEnt->enemy->r.mins[2])/2;

			if ( yawMissAllow < 8.0f )
			{
				yawMissAllow = 8.0f;
			}

			if ( pitchMissAllow < 8.0f )
			{
				pitchMissAllow = 8.0f;
			}

			yawMiss = tan(DEG2RAD(AngleDelta ( aiEnt->client->ps.viewangles[YAW], aiEnt->NPC->desiredYaw ))) * dist;
			pitchMiss = tan(DEG2RAD(AngleDelta ( aiEnt->client->ps.viewangles[PITCH], aiEnt->NPC->desiredPitch))) * dist;

			if ( yawMissAllow >= yawMiss && pitchMissAllow > pitchMiss )
			{
				aiEnt->client->pers.cmd.buttons |= BUTTON_ATTACK;
			}
		}
	}

	return;

finished:
	aiEnt->NPC->desiredYaw = aiEnt->client->ps.viewangles[YAW];
	aiEnt->NPC->desiredPitch = aiEnt->client->ps.viewangles[PITCH];

	aiEnt->NPC->aimTime = 0;//ok to turn normally now
}

/*
void NPC_BSMove(void)
Move in a direction, face another
*/
void NPC_BSMove(gentity_t *aiEnt)
{
	gentity_t	*goal = NULL;

	// UQ1: Added evasion...
	NPC_CheckEvasion(aiEnt);

	NPC_CheckEnemy(aiEnt, qtrue, qfalse, qtrue);

	if(aiEnt->enemy)
	{
		NPC_CheckCanAttack(aiEnt, 1.0, qfalse);
	}
	else
	{
		NPC_UpdateAngles(aiEnt, qtrue, qtrue);
	}

	goal = UpdateGoal(aiEnt);
	if(goal)
	{
//		NPCInfo->moveToGoalMod = 1.0;

		NPC_SlideMoveToGoal(aiEnt);
	}
}

/*
void NPC_BSShoot(void)
Move in a direction, face another
*/

void NPC_BSShoot(gentity_t *aiEnt)
{
//	NPC_BSMove();

	// UQ1: Added evasion...
	NPC_CheckEvasion(aiEnt);

	NPCS.enemyVisibility = VIS_SHOOT;

	if ( aiEnt->client->ps.weaponstate != WEAPON_READY && aiEnt->client->ps.weaponstate != WEAPON_FIRING )
	{
		aiEnt->client->ps.weaponstate = WEAPON_READY;
	}

	WeaponThink(aiEnt, qtrue);
}

/*
void NPC_BSPatrol( void )

  Same as idle, but you look for enemies every "vigilance"
  using your angles, HFOV, VFOV and visrange, and listen for sounds within earshot...
*/
void NPC_BSPatrol( gentity_t *aiEnt)
{
	//int	alertEventNum;

	if(level.time > aiEnt->NPC->enemyCheckDebounceTime)
	{
		aiEnt->NPC->enemyCheckDebounceTime = level.time + (aiEnt->NPC->stats.vigilance * 1000);
		NPC_CheckEnemy(aiEnt, qtrue, qfalse, qtrue);
		if(aiEnt->enemy)
		{//FIXME: do anger script
			aiEnt->NPC->behaviorState = BS_HUNT_AND_KILL;
			//NPC_AngerSound(aiEnt);
			return;
		}
	}

	//FIXME: Implement generic sound alerts
	/*
	alertEventNum = NPC_CheckAlertEvents( aiEnt, qtrue, qtrue );
	if( alertEventNum != -1 )
	{//If we heard something, see if we should check it out
		if ( NPC_CheckInvestigate( aiEnt, alertEventNum ) )
		{
			return;
		}
	}
	*/

	aiEnt->NPC->investigateSoundDebounceTime = 0;
	//FIXME if there is no nav data, we need to do something else
	// if we're stuck, try to move around it
	if ( UpdateGoal(aiEnt) )
	{
		NPC_MoveToGoal(aiEnt, qtrue );
	}

	NPC_UpdateAngles(aiEnt, qtrue, qtrue );

	aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
}

/*
void NPC_BSDefault(void)
	uses various scriptflags to determine how an npc should behave
*/
extern void NPC_CheckGetNewWeapon(gentity_t *aiEnt);
extern void NPC_BSST_Attack(gentity_t *aiEnt);

void NPC_BSDefault( gentity_t *aiEnt)
{
//	vec3_t		enemyDir;
//	float		enemyDist;
//	float		shootDist;
//	qboolean	enemyFOV = qfalse;
//	qboolean	enemyShotFOV = qfalse;
//	qboolean	enemyPVS = qfalse;
//	vec3_t		enemyHead;
//	vec3_t		muzzle;
//	qboolean	enemyLOS = qfalse;
//	qboolean	enemyCS = qfalse;
	qboolean	move = qtrue;
//	qboolean	shoot = qfalse;


	if( aiEnt->NPC->scriptFlags & SCF_FIRE_WEAPON )
	{
		WeaponThink(aiEnt, qtrue );
	}

	if ( aiEnt->NPC->scriptFlags & SCF_FORCED_MARCH )
	{//being forced to walk
		if( aiEnt->client->ps.torsoAnim != TORSO_SURRENDER_START )
		{
			NPC_SetAnim( aiEnt, SETANIM_TORSO, TORSO_SURRENDER_START, SETANIM_FLAG_HOLD );
		}
	}
	//look for a new enemy if don't have one and are allowed to look, validate current enemy if have one
	NPC_CheckEnemy(aiEnt, (qboolean)(aiEnt->NPC->scriptFlags&SCF_LOOK_FOR_ENEMIES), qfalse, qtrue );
	if ( !aiEnt->enemy )
	{//still don't have an enemy
		if ( !(aiEnt->NPC->scriptFlags&SCF_IGNORE_ALERTS) )
		{//check for alert events
			//FIXME: Check Alert events, see if we should investigate or just look at it
			int alertEvent = NPC_CheckAlertEvents(aiEnt, qtrue, qtrue, -1, qtrue, AEL_DISCOVERED );

			//There is an event to look at
			if ( alertEvent >= 0 && level.alertEvents[alertEvent].ID != aiEnt->NPC->lastAlertID )
			{//heard/saw something
				if ( level.alertEvents[alertEvent].level >= AEL_DISCOVERED && (aiEnt->NPC->scriptFlags&SCF_LOOK_FOR_ENEMIES) )
				{//was a big event
					if ( level.alertEvents[alertEvent].owner && level.alertEvents[alertEvent].owner->client && level.alertEvents[alertEvent].owner->health >= 0 && level.alertEvents[alertEvent].owner->client->playerTeam == aiEnt->client->enemyTeam )
					{//an enemy
						G_SetEnemy( aiEnt, level.alertEvents[alertEvent].owner );
					}
				}
				else
				{//FIXME: investigate lesser events
				}
			}
			//FIXME: also check our allies' condition?
		}
	}

	if ( aiEnt->enemy && !(aiEnt->NPC->scriptFlags&SCF_FORCED_MARCH) )
	{
		// just use the stormtrooper attack AI...
		NPC_CheckGetNewWeapon(aiEnt);
		if ( aiEnt->client->leader
			&& aiEnt->NPC->goalEntity == aiEnt->client->leader
#ifndef __NO_ICARUS__
			&& !trap->ICARUS_TaskIDPending( (sharedEntity_t *)aiEnt, TID_MOVE_NAV ) 
#endif //__NO_ICARUS__
			)
		{
			NPC_ClearGoal(aiEnt);
		}
			NPC_BSST_Attack(aiEnt);
		return;
/*
		//have an enemy
		//FIXME: if one of these fails, meaning we can't shoot, do we really need to do the rest?
		VectorSubtract( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, enemyDir );
		enemyDist = VectorNormalize( enemyDir );
		enemyDist *= enemyDist;
		shootDist = NPC_MaxDistSquaredForWeapon(aiEnt);

		enemyFOV = InFOV( NPC->enemy, NPC, NPCInfo->stats.hfov, NPCInfo->stats.vfov );
		enemyShotFOV = InFOV( NPC->enemy, NPC, 20, 20 );
		enemyPVS = trap->inPVS( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin );

		if ( enemyPVS )
		{//in the pvs
			trace_t	tr;

			CalcEntitySpot( NPC->enemy, SPOT_HEAD, enemyHead );
			enemyHead[2] -= Q_flrand( 0.0f, NPC->enemy->maxs[2]*0.5f );
			CalcEntitySpot( NPC, SPOT_WEAPON, muzzle );
			enemyLOS = NPC_ClearLOS( muzzle, enemyHead );

			trap->trace ( &tr, muzzle, vec3_origin, vec3_origin, enemyHead, NPC->s.number, MASK_SHOT );
			enemyCS = NPC_EvaluateShot( tr.entityNum, qtrue );
		}
		else
		{//skip thr 2 traces since they would have to fail
			enemyLOS = qfalse;
			enemyCS = qfalse;
		}

		if ( enemyCS && enemyShotFOV )
		{//can hit enemy if we want
			NPC->cantHitEnemyCounter = 0;
		}
		else
		{//can't hit
			NPC->cantHitEnemyCounter++;
		}

		if ( enemyCS && enemyShotFOV && enemyDist < shootDist )
		{//can shoot
			shoot = qtrue;
			if ( NPCInfo->goalEntity == NPC->enemy )
			{//my goal is my enemy and I have a clear shot, no need to chase right now
				move = qfalse;
			}
		}
		else
		{//don't shoot yet, keep chasing
			shoot = qfalse;
			move = qtrue;
		}

		//shoot decision
		if ( !(NPCInfo->scriptFlags&SCF_DONT_FIRE) )
		{//try to shoot
			if ( NPC->enemy )
			{
				if ( shoot )
				{
					if( !(NPCInfo->scriptFlags & SCF_FIRE_WEAPON) ) // we've already fired, no need to do it again here
					{
						WeaponThink( qtrue );
					}
				}
			}
		}

		//chase decision
		if ( NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
		{//go after him
			NPCInfo->goalEntity = NPC->enemy;
			//FIXME: don't need to chase when have a clear shot and in range?
			if ( !enemyCS && NPC->cantHitEnemyCounter > 60 )
			{//haven't been able to shoot enemy for about 6 seconds, need to do something
				//FIXME: combat points?  Just chase?
				if ( enemyPVS )
				{//in my PVS, just pick a combat point
					//FIXME: implement
				}
				else
				{//just chase him
				}
			}
			//FIXME: in normal behavior, should we use combat Points?  Do we care?  Is anyone actually going to ever use this AI?
		}
		else if ( NPC->cantHitEnemyCounter > 60 )
		{//pick a new one
			NPC_CheckEnemy( qtrue, qfalse, qtrue );
		}

		if ( enemyPVS && enemyLOS )//&& !enemyShotFOV )
		{//have a clear LOS to him//, but not looking at him
			//Find the desired angles
			vec3_t	angles;

			GetAnglesForDirection( muzzle, enemyHead, angles );

			NPCInfo->desiredYaw		= AngleNormalize180( angles[YAW] );
			NPCInfo->desiredPitch	= AngleNormalize180( angles[PITCH] );
		}
		*/
	}

	if ( UpdateGoal(aiEnt) )
	{//have a goal
		if ( !aiEnt->enemy
			&& aiEnt->client->leader
			&& aiEnt->NPC->goalEntity == aiEnt->client->leader
#ifndef __NO_ICARUS__
			&& !trap->ICARUS_TaskIDPending( (sharedEntity_t *)aiEnt, TID_MOVE_NAV ) 
#endif //__NO_ICARUS__
			)
		{
			NPC_BSFollowLeader(aiEnt);
		}
		else
		{
			//set angles
			if ( (aiEnt->NPC->scriptFlags & SCF_FACE_MOVE_DIR) || aiEnt->NPC->goalEntity != aiEnt->enemy )
			{//face direction of movement, NOTE: default behavior when not chasing enemy
				aiEnt->NPC->combatMove = qfalse;
			}
			else
			{//face goal.. FIXME: what if have a navgoal but want to face enemy while moving?  Will this do that?
				vec3_t	dir, angles;

				aiEnt->NPC->combatMove = qfalse;

				VectorSubtract( aiEnt->NPC->goalEntity->r.currentOrigin, aiEnt->r.currentOrigin, dir );
				vectoangles( dir, angles );
				aiEnt->NPC->desiredYaw = angles[YAW];
				if ( aiEnt->NPC->goalEntity == aiEnt->enemy )
				{
					aiEnt->NPC->desiredPitch = angles[PITCH];
				}
			}

			//set movement
			//override default walk/run behavior
			//NOTE: redundant, done in NPC_ApplyScriptFlags
			if ( aiEnt->NPC->scriptFlags & SCF_RUNNING )
			{
				aiEnt->client->pers.cmd.buttons &= ~BUTTON_WALKING;
			}
			else if ( aiEnt->NPC->scriptFlags & SCF_WALKING )
			{
				aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
			}
			else if ( aiEnt->NPC->goalEntity == aiEnt->enemy )
			{
				aiEnt->client->pers.cmd.buttons &= ~BUTTON_WALKING;
			}
			else
			{
				aiEnt->client->pers.cmd.buttons |= BUTTON_WALKING;
			}

			if ( aiEnt->NPC->scriptFlags & SCF_FORCED_MARCH )
			{//being forced to walk
				//if ( g_crosshairEntNum != NPC->s.number )
				if (!NPC_SomeoneLookingAtMe(aiEnt))
				{//don't walk if player isn't aiming at me
					move = qfalse;
				}
			}

			if ( move )
			{
				//move toward goal
				NPC_MoveToGoal(aiEnt, qtrue );
			}
		}
	}
	else if ( !aiEnt->enemy && aiEnt->client->leader )
	{
		NPC_BSFollowLeader(aiEnt);
	}

	//update angles
	NPC_UpdateAngles(aiEnt, qtrue, qtrue );
}
