// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "ghoul2/G2.h"
#include "bg_saga.h"
#include "jkg_damagetypes.h"
#include "ai_dominance_main.h"

extern int DOM_GetNearWP(vec3_t org, int badwp);
extern void SP_NPC_spawner2( gentity_t *self);
extern qboolean NPC_IsAlive (gentity_t *self, gentity_t *NPC);
extern qboolean NPC_NeedPadawan_Spawn (gentity_t *player);
extern qboolean NPC_NeedFollower_Spawn(gentity_t *player);

// g_client.c -- client functions that don't happen every frame

static vec3_t	playerMins = {-15, -15, DEFAULT_MINS_2};
static vec3_t	playerMaxs = {15, 15, DEFAULT_MAXS_2};

extern model_scale_list_t model_scale_list[512];
extern int num_scale_models;
extern qboolean scale_models_loaded;

extern int g_siegeRespawnCheck;

void WP_SaberAddG2Model( gentity_t *saberent, const char *saberModel, qhandle_t saberSkin );
void WP_SaberRemoveG2Model( gentity_t *saberent );
extern qboolean WP_SaberStyleValidForSaber( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int saberAnimLevel );
extern qboolean WP_UseFirstValidSaberStyle( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int *saberAnimLevel );
forcedata_t Client_Force[MAX_CLIENTS];

/*QUAKED info_player_duel (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for duelists in duel.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_duel( gentity_t *ent )
{
	int		i;

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}

	ent->s.eType = ET_SPAWNPOINT;

	if (ent->noWaypointTime == 0)
	{
		trap->LinkEntity((sharedEntity_t*)ent);
	}
}

/*QUAKED info_player_duel1 (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for lone duelists in powerduel.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_duel1( gentity_t *ent )
{
	int		i;

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}

	ent->s.eType = ET_SPAWNPOINT;

	if (ent->noWaypointTime == 0)
	{
		trap->LinkEntity((sharedEntity_t*)ent);
	}
}

/*QUAKED info_player_duel2 (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for paired duelists in powerduel.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_duel2( gentity_t *ent )
{
	int		i;

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}

	ent->s.eType = ET_SPAWNPOINT;

	if (ent->noWaypointTime == 0)
	{
		trap->LinkEntity((sharedEntity_t*)ent);
	}
}

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for deathmatch games.
The first time a player enters the game, they will be at an 'initial' spot.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_deathmatch( gentity_t *ent ) {
	int		i;

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}

	ent->s.eType = ET_SPAWNPOINT;

	if (ent->noWaypointTime == 0)
	{
		trap->LinkEntity((sharedEntity_t*)ent);
	}
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
Targets will be fired when someone spawns in on them.
equivelant to info_player_deathmatch
*/
void SP_info_player_start(gentity_t *ent) {
	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch( ent );
}

/*QUAKED info_player_start_red (1 0 0) (-16 -16 -24) (16 16 32) INITIAL
For Red Team DM starts

Targets will be fired when someone spawns in on them.
equivalent to info_player_deathmatch

INITIAL - The first time a player enters the game, they will be at an 'initial' spot.

"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_start_red(gentity_t *ent) {
	SP_info_player_deathmatch( ent );
}

/*QUAKED info_player_start_blue (1 0 0) (-16 -16 -24) (16 16 32) INITIAL
For Blue Team DM starts

Targets will be fired when someone spawns in on them.
equivalent to info_player_deathmatch

INITIAL - The first time a player enters the game, they will be at an 'initial' spot.

"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_start_blue(gentity_t *ent) {
	SP_info_player_deathmatch( ent );
}

void SiegePointUse( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	//Toggle the point on/off
	if (self->genericValue1)
	{
		self->genericValue1 = 0;
	}
	else
	{
		self->genericValue1 = 1;
	}
}

/*QUAKED info_player_siegeteam1 (1 0 0) (-16 -16 -24) (16 16 32)
siege start point - team1
name and behavior of team1 depends on what is defined in the
.siege file for this level

startoff - if non-0 spawn point will be disabled until used
idealclass - if specified, this spawn point will be considered
"ideal" for players of this class name. Corresponds to the name
entry in the .scl (siege class) file.
Targets will be fired when someone spawns in on them.
*/
void SP_info_player_siegeteam1(gentity_t *ent) {
	int soff = 0;

	if (level.gametype != GT_SIEGE)
	{ //turn into a DM spawn if not in siege game mode
		ent->classname = "info_player_deathmatch";
		SP_info_player_deathmatch( ent );

		return;
	}

	G_SpawnInt("startoff", "0", &soff);

	if (soff)
	{ //start disabled
		ent->genericValue1 = 0;
	}
	else
	{
		ent->genericValue1 = 1;
	}

	ent->use = SiegePointUse;

	ent->s.eType = ET_SPAWNPOINT;

	if (ent->noWaypointTime == 0)
	{
		trap->LinkEntity((sharedEntity_t*)ent);
	}
}

/*QUAKED info_player_siegeteam2 (0 0 1) (-16 -16 -24) (16 16 32)
siege start point - team2
name and behavior of team2 depends on what is defined in the
.siege file for this level

startoff - if non-0 spawn point will be disabled until used
idealclass - if specified, this spawn point will be considered
"ideal" for players of this class name. Corresponds to the name
entry in the .scl (siege class) file.
Targets will be fired when someone spawns in on them.
*/
void SP_info_player_siegeteam2(gentity_t *ent) {
	int soff = 0;

	if (level.gametype != GT_SIEGE)
	{ //turn into a DM spawn if not in siege game mode
		ent->classname = "info_player_deathmatch";
		SP_info_player_deathmatch( ent );

		return;
	}

	G_SpawnInt("startoff", "0", &soff);

	if (soff)
	{ //start disabled
		ent->genericValue1 = 0;
	}
	else
	{
		ent->genericValue1 = 1;
	}

	ent->use = SiegePointUse;

	ent->s.eType = ET_SPAWNPOINT;

	if (ent->noWaypointTime == 0)
	{
		trap->LinkEntity((sharedEntity_t*)ent);
	}
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32) RED BLUE
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
RED - In a Siege game, the intermission will happen here if the Red (attacking) team wins
BLUE - In a Siege game, the intermission will happen here if the Blue (defending) team wins
*/
void SP_info_player_intermission( gentity_t *ent ) {

}

/*QUAKED info_player_intermission_red (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.

In a Siege game, the intermission will happen here if the Red (attacking) team wins
target - ent to look at
target2 - ents to use when this intermission point is chosen
*/
void SP_info_player_intermission_red( gentity_t *ent ) {

}

/*QUAKED info_player_intermission_blue (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.

In a Siege game, the intermission will happen here if the Blue (defending) team wins
target - ent to look at
target2 - ents to use when this intermission point is chosen
*/
void SP_info_player_intermission_blue( gentity_t *ent ) {

}

#define JMSABER_RESPAWN_TIME 20000 //in case it gets stuck somewhere no one can reach

void ThrowSaberToAttacker(gentity_t *self, gentity_t *attacker)
{
	gentity_t *ent = &g_entities[self->client->ps.saberIndex];
	vec3_t a;
	int altVelocity = 0;

	if (!ent || ent->enemy != self)
	{ //something has gone very wrong (this should never happen)
		//but in case it does.. find the saber manually
#ifdef _DEBUG
		Com_Printf("Lost the saber! Attempting to use global pointer..\n");
#endif
		ent = gJMSaberEnt;

		if (!ent)
		{
#ifdef _DEBUG
			Com_Printf("The global pointer was NULL. This is a bad thing.\n");
#endif
			return;
		}

#ifdef _DEBUG
		Com_Printf("Got it (%i). Setting enemy to client %i.\n", ent->s.number, self->s.number);
#endif

		ent->enemy = self;
		self->client->ps.saberIndex = ent->s.number;
	}

	trap->SetConfigstring ( CS_CLIENT_JEDIMASTER, "-1" );

	if (attacker && attacker->client && self->client->ps.saberInFlight)
	{ //someone killed us and we had the saber thrown, so actually move this saber to the saber location
	  //if we killed ourselves with saber thrown, however, same suicide rules of respawning at spawn spot still
	  //apply.
		gentity_t *flyingsaber = &g_entities[self->client->ps.saberEntityNum];

		if (flyingsaber && flyingsaber->inuse)
		{
			VectorCopy(flyingsaber->s.pos.trBase, ent->s.pos.trBase);
			VectorCopy(flyingsaber->s.pos.trDelta, ent->s.pos.trDelta);
			VectorCopy(flyingsaber->s.apos.trBase, ent->s.apos.trBase);
			VectorCopy(flyingsaber->s.apos.trDelta, ent->s.apos.trDelta);

			VectorCopy(flyingsaber->r.currentOrigin, ent->r.currentOrigin);
			VectorCopy(flyingsaber->r.currentAngles, ent->r.currentAngles);
			altVelocity = 1;
		}
	}

	self->client->ps.saberInFlight = qtrue; //say he threw it anyway in order to properly remove from dead body

	WP_SaberAddG2Model( ent, self->client->saber[0].model, self->client->saber[0].skin );

	ent->s.eFlags &= ~(EF_NODRAW);
	ent->s.modelGhoul2 = 1;
	ent->s.eType = ET_MISSILE;
	ent->enemy = NULL;

	if (!attacker || !attacker->client)
	{
		VectorCopy(ent->s.origin2, ent->s.pos.trBase);
		VectorCopy(ent->s.origin2, ent->s.origin);
		VectorCopy(ent->s.origin2, ent->r.currentOrigin);
		ent->pos2[0] = 0;
		trap->LinkEntity((sharedEntity_t *)ent);
		return;
	}

	if (!altVelocity)
	{
		VectorCopy(self->s.pos.trBase, ent->s.pos.trBase);
		VectorCopy(self->s.pos.trBase, ent->s.origin);
		VectorCopy(self->s.pos.trBase, ent->r.currentOrigin);

		VectorSubtract(attacker->client->ps.origin, ent->s.pos.trBase, a);

		VectorNormalize(a);

		ent->s.pos.trDelta[0] = a[0]*256;
		ent->s.pos.trDelta[1] = a[1]*256;
		ent->s.pos.trDelta[2] = 256;
	}

	trap->LinkEntity((sharedEntity_t *)ent);
}

void JMSaberThink(gentity_t *ent)
{
	gJMSaberEnt = ent;

	if (ent->enemy)
	{
		if (!ent->enemy->client || !ent->enemy->inuse)
		{ //disconnected?
			VectorCopy(ent->enemy->s.pos.trBase, ent->s.pos.trBase);
			VectorCopy(ent->enemy->s.pos.trBase, ent->s.origin);
			VectorCopy(ent->enemy->s.pos.trBase, ent->r.currentOrigin);
			ent->s.modelindex = G_ModelIndex("models/weapons2/saber/saber_w.glm");
			ent->s.eFlags &= ~(EF_NODRAW);
			ent->s.modelGhoul2 = 1;
			ent->s.eType = ET_MISSILE;
			ent->enemy = NULL;

			ent->pos2[0] = 1;
			ent->pos2[1] = 0; //respawn next think
			trap->LinkEntity((sharedEntity_t *)ent);
		}
		else
		{
			ent->pos2[1] = level.time + JMSABER_RESPAWN_TIME;
		}
	}
	else if (ent->pos2[0] && ent->pos2[1] < level.time)
	{
		VectorCopy(ent->s.origin2, ent->s.pos.trBase);
		VectorCopy(ent->s.origin2, ent->s.origin);
		VectorCopy(ent->s.origin2, ent->r.currentOrigin);
		ent->pos2[0] = 0;
		trap->LinkEntity((sharedEntity_t *)ent);
	}

	ent->nextthink = level.time + 50;
	G_RunObject(ent);
}

void JMSaberTouch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	int i = 0;
//	gentity_t *te;

	if (!other || !other->client || other->health < 1)
	{
		return;
	}

	if (self->enemy)
	{
		return;
	}

	if (!self->s.modelindex)
	{
		return;
	}

	if (HaveWeapon(&other->client->ps, WP_SABER))
	{
		return;
	}

	if (other->client->ps.isJediMaster)
	{
		return;
	}

	self->enemy = other;
	other->client->ps.temporaryWeapon = WP_SABER;
	other->client->ps.weapon = WP_SABER;
	other->s.weapon = WP_SABER;
	other->client->ps.scopeType = SCOPE_NONE;
	G_AddEvent(other, EV_BECOME_JEDIMASTER, 0);

	// Track the jedi master
	trap->SetConfigstring ( CS_CLIENT_JEDIMASTER, va("%i", other->s.number ) );

	if (g_spawnInvulnerability.integer)
	{
		other->client->ps.eFlags |= EF_INVULNERABLE;
		other->client->invulnerableTimer = level.time + g_spawnInvulnerability.integer;
	}

	trap->SendServerCommand( -1, va("cp \"%s %s\n\"", other->client->pers.netname, G_GetStringEdString("MP_SVGAME", "BECOMEJM")) );

	other->client->ps.isJediMaster = qtrue;
	other->client->ps.saberIndex = self->s.number;

	if (other->health < 200 && other->health > 0)
	{ //full health when you become the Jedi Master
		other->client->ps.stats[STAT_HEALTH] = other->health = 200;
	}

	if (other->client->ps.fd.forcePower < 100)
	{
		other->client->ps.fd.forcePower = 100;
	}

	while (i < NUM_FORCE_POWERS)
	{
		other->client->ps.fd.forcePowersKnown |= (1 << i);
		other->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_3;

		i++;
	}

	self->pos2[0] = 1;
	self->pos2[1] = level.time + JMSABER_RESPAWN_TIME;

	self->s.modelindex = 0;
	self->s.eFlags |= EF_NODRAW;
	self->s.modelGhoul2 = 0;
	self->s.eType = ET_GENERAL;

	/*
	te = G_TempEntity( vec3_origin, EV_DESTROY_GHOUL2_INSTANCE );
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = self->s.number;
	*/
	G_KillG2Queue(self->s.number);

	return;
}

gentity_t *gJMSaberEnt = NULL;

/*QUAKED info_jedimaster_start (1 0 0) (-16 -16 -24) (16 16 32)
"jedi master" saber spawn point
*/
void SP_info_jedimaster_start(gentity_t *ent)
{
	if (level.gametype != GT_JEDIMASTER)
	{
		gJMSaberEnt = NULL;
		G_FreeEntity(ent);
		return;
	}

	ent->enemy = NULL;

	ent->flags = FL_BOUNCE_HALF;

	ent->s.modelindex = G_ModelIndex("models/weapons2/saber/saber_w.glm");
	ent->s.modelGhoul2 = 1;
	ent->s.g2radius = 20;
	//ent->s.eType = ET_GENERAL;
	ent->s.eType = ET_MISSILE;
	ent->s.weapon = WP_SABER;
	ent->s.pos.trType = TR_GRAVITY;
	ent->s.pos.trTime = level.time;
	VectorSet( ent->r.maxs, 3, 3, 3 );
	VectorSet( ent->r.mins, -3, -3, -3 );
	ent->r.contents = CONTENTS_TRIGGER;
	ent->clipmask = MASK_SOLID;

	ent->isSaberEntity = qtrue;

	ent->bounceCount = -5;

	ent->physicsObject = qtrue;

	VectorCopy(ent->s.pos.trBase, ent->s.origin2); //remember the spawn spot

	ent->touch = JMSaberTouch;

	trap->LinkEntity((sharedEntity_t *)ent);

	ent->think = JMSaberThink;
	ent->nextthink = level.time + 50;
}

/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
SpotWouldTelefrag

================
*/
qboolean SpotWouldTelefrag( gentity_t *spot ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;

	VectorAdd( spot->s.origin, playerMins, mins );
	VectorAdd( spot->s.origin, playerMaxs, maxs );
	num = trap->EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) {
		hit = &g_entities[touch[i]];
		//if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
		if ( hit->client) {
			return qtrue;
		}

	}

	return qfalse;
}

qboolean SpotWouldTelefrag2( gentity_t *mover, vec3_t dest )
{
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;

	VectorAdd( dest, mover->r.mins, mins );
	VectorAdd( dest, mover->r.maxs, maxs );
	num = trap->EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++)
	{
		hit = &g_entities[touch[i]];
		if ( hit == mover )
		{
			continue;
		}

		if ( hit->r.contents & mover->r.contents )
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
================
SelectNearestDeathmatchSpawnPoint

Find the spot that we DON'T want to use
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectNearestDeathmatchSpawnPoint( vec3_t from ) {
	gentity_t	*spot;
	vec3_t		delta;
	float		dist, nearestDist;
	gentity_t	*nearestSpot;

	nearestDist = 999999;
	nearestSpot = NULL;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {

		VectorSubtract( spot->s.origin, from, delta );
		dist = VectorLength( delta );
		if ( dist < nearestDist ) {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}


/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectRandomDeathmatchSpawnPoint( qboolean isbot ) {
	gentity_t	*spot;
	int			count;
	int			selection;
	gentity_t	*spots[MAX_SPAWN_POINTS];

	count = 0;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL && count < MAX_SPAWN_POINTS) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}

		if(((spot->flags & FL_NO_BOTS) && isbot) ||
		   ((spot->flags & FL_NO_HUMANS) && !isbot))
		{
			// spot is not for this human/bot player
			continue;
		}

		spots[ count ] = spot;
		count++;
	}

	if ( !count ) {	// no spots that won't telefrag
		return G_Find( NULL, FOFS(classname), "info_player_deathmatch");
	}

	selection = rand() % count;
	return spots[ selection ];
}

/*
===========
SelectRandomFurthestSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectRandomFurthestSpawnPoint ( vec3_t avoidPoint, vec3_t origin, vec3_t angles, team_t team, qboolean isbot ) {
	gentity_t	*spot;
	vec3_t		delta;
	float		dist;
	float		list_dist[MAX_SPAWN_POINTS];
	gentity_t	*list_spot[MAX_SPAWN_POINTS];
	int			numSpots, rnd, i, j;

	numSpots = 0;
	spot = NULL;

	//in Team DM, look for a team start spot first, if any
	if ( level.gametype == GT_TEAM
		&& team != FACTION_FREE
		&& team != FACTION_SPECTATOR )
	{
		char *classname = NULL;
		if ( team == FACTION_EMPIRE )
		{
			classname = "info_player_start_red";
		}
		else
		{
			classname = "info_player_start_blue";
		}
		while ((spot = G_Find (spot, FOFS(classname), classname)) != NULL) {
			if ( SpotWouldTelefrag( spot ) ) {
				continue;
			}

			if(((spot->flags & FL_NO_BOTS) && isbot) ||
			   ((spot->flags & FL_NO_HUMANS) && !isbot))
			{
				// spot is not for this human/bot player
				continue;
			}

			VectorSubtract( spot->s.origin, avoidPoint, delta );
			dist = VectorLength( delta );
			for (i = 0; i < numSpots; i++) {
				if ( dist > list_dist[i] ) {
					if ( numSpots >= MAX_SPAWN_POINTS )
						numSpots = MAX_SPAWN_POINTS-1;
					for (j = numSpots; j > i; j--) {
						list_dist[j] = list_dist[j-1];
						list_spot[j] = list_spot[j-1];
					}
					list_dist[i] = dist;
					list_spot[i] = spot;
					numSpots++;
					break;
				}
			}
			if (i >= numSpots && numSpots < MAX_SPAWN_POINTS) {
				list_dist[numSpots] = dist;
				list_spot[numSpots] = spot;
				numSpots++;
			}
		}
	}

	if ( !numSpots )
	{//couldn't find any of the above
		while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
			if ( SpotWouldTelefrag( spot ) ) {
				continue;
			}

			if(((spot->flags & FL_NO_BOTS) && isbot) ||
			   ((spot->flags & FL_NO_HUMANS) && !isbot))
			{
				// spot is not for this human/bot player
				continue;
			}

			VectorSubtract( spot->s.origin, avoidPoint, delta );
			dist = VectorLength( delta );
			for (i = 0; i < numSpots; i++) {
				if ( dist > list_dist[i] ) {
					if ( numSpots >= MAX_SPAWN_POINTS )
						numSpots = MAX_SPAWN_POINTS-1;
					for (j = numSpots; j > i; j--) {
						list_dist[j] = list_dist[j-1];
						list_spot[j] = list_spot[j-1];
					}
					list_dist[i] = dist;
					list_spot[i] = spot;
					numSpots++;
					break;
				}
			}
			if (i >= numSpots && numSpots < MAX_SPAWN_POINTS) {
				list_dist[numSpots] = dist;
				list_spot[numSpots] = spot;
				numSpots++;
			}
		}
		if (!numSpots) {
			spot = G_Find( NULL, FOFS(classname), "info_player_deathmatch");
			if (!spot)
				trap->Error( ERR_DROP, "Couldn't find a spawn point" );
			VectorCopy (spot->s.origin, origin);
			origin[2] += 9;
			VectorCopy (spot->s.angles, angles);
			return spot;
		}
	}

	// select a random spot from the spawn points furthest away
	rnd = random() * (numSpots / 2);

	VectorCopy (list_spot[rnd]->s.origin, origin);
	origin[2] += 9;
	VectorCopy (list_spot[rnd]->s.angles, angles);

	return list_spot[rnd];
}

gentity_t *SelectDuelSpawnPoint( int team, vec3_t avoidPoint, vec3_t origin, vec3_t angles, qboolean isbot )
{
	gentity_t	*spot;
	vec3_t		delta;
	float		dist;
	float		list_dist[MAX_SPAWN_POINTS];
	gentity_t	*list_spot[MAX_SPAWN_POINTS];
	int			numSpots, rnd, i, j;
	char		*spotName;

	if (team == DUELTEAM_LONE)
	{
		spotName = "info_player_duel1";
	}
	else if (team == DUELTEAM_DOUBLE)
	{
		spotName = "info_player_duel2";
	}
	else if (team == DUELTEAM_SINGLE)
	{
		spotName = "info_player_duel";
	}
	else
	{
		spotName = "info_player_deathmatch";
	}
tryAgain:

	numSpots = 0;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), spotName)) != NULL) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}

		if(((spot->flags & FL_NO_BOTS) && isbot) ||
			((spot->flags & FL_NO_HUMANS) && !isbot))
		{
			// spot is not for this human/bot player
			continue;
		}

		VectorSubtract( spot->s.origin, avoidPoint, delta );
		dist = VectorLength( delta );
		for (i = 0; i < numSpots; i++) {
			if ( dist > list_dist[i] ) {
				if ( numSpots >= MAX_SPAWN_POINTS )
					numSpots = MAX_SPAWN_POINTS-1;
				for (j = numSpots; j > i; j--) {
					list_dist[j] = list_dist[j-1];
					list_spot[j] = list_spot[j-1];
				}
				list_dist[i] = dist;
				list_spot[i] = spot;
				numSpots++;
				break;
			}
		}
		if (i >= numSpots && numSpots < MAX_SPAWN_POINTS) {
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}
	if (!numSpots)
	{
		if (Q_stricmp(spotName, "info_player_deathmatch"))
		{ //try the loop again with info_player_deathmatch as the target if we couldn't find a duel spot
			spotName = "info_player_deathmatch";
			goto tryAgain;
		}

		//If we got here we found no free duel or DM spots, just try the first DM spot
		spot = G_Find( NULL, FOFS(classname), "info_player_deathmatch");
		if (!spot)
			trap->Error( ERR_DROP, "Couldn't find a spawn point" );
		VectorCopy (spot->s.origin, origin);
		origin[2] += 9;
		VectorCopy (spot->s.angles, angles);
		return spot;
	}

	// select a random spot from the spawn points furthest away
	rnd = random() * (numSpots / 2);

	VectorCopy (list_spot[rnd]->s.origin, origin);
	origin[2] += 9;
	VectorCopy (list_spot[rnd]->s.angles, angles);

	return list_spot[rnd];
}

/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectSpawnPoint ( vec3_t avoidPoint, vec3_t origin, vec3_t angles, team_t team, qboolean isbot ) {
	return SelectRandomFurthestSpawnPoint( avoidPoint, origin, angles, team, isbot );

	/*
	gentity_t	*spot;
	gentity_t	*nearestSpot;

	nearestSpot = SelectNearestDeathmatchSpawnPoint( avoidPoint );

	spot = SelectRandomDeathmatchSpawnPoint ( qfalse );
	if ( spot == nearestSpot ) {
		// roll again if it would be real close to point of death
		spot = SelectRandomDeathmatchSpawnPoint ( qfalse );
		if ( spot == nearestSpot ) {
			// last try
			spot = SelectRandomDeathmatchSpawnPoint ( qfalse);
		}
	}

	// find a single player start spot
	if (!spot) {
		trap->Error( ERR_DROP, "Couldn't find a spawn point" );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
	*/
}

/*
===========
SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
gentity_t *SelectInitialSpawnPoint( vec3_t origin, vec3_t angles, team_t team, qboolean isbot ) {
	gentity_t	*spot;

	spot = NULL;
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if(((spot->flags & FL_NO_BOTS) && isbot) ||
		   ((spot->flags & FL_NO_HUMANS) && !isbot))
		{
			continue;
		}

		if ( spot->spawnflags & 1 ) {
			break;
		}
	}

	if ( !spot || SpotWouldTelefrag( spot ) ) {
		return SelectSpawnPoint( vec3_origin, origin, angles, team, isbot );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}

/*
===========
SelectSpectatorSpawnPoint

============
*/
gentity_t *SelectSpectatorSpawnPoint( vec3_t origin, vec3_t angles ) {
	FindIntermissionPoint();

	VectorCopy( level.intermission_origin, origin );
	VectorCopy( level.intermission_angle, angles );

	return NULL;
}

/*
=======================================================================

BODYQUE

=======================================================================
*/

/*
=======================================================================

BODYQUE

=======================================================================
*/

#define BODY_SINK_TIME		30000//45000

/*
===============
InitBodyQue
===============
*/
void InitBodyQue (void) {
	int		i;
	gentity_t	*ent;

	level.bodyQueIndex = 0;
	for (i=0; i<BODY_QUEUE_SIZE ; i++) {
		ent = G_Spawn();
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}

/*
=============
BodySink

After sitting around for five seconds, fall into the ground and disappear
=============
*/
void BodySink( gentity_t *ent ) {
	if ( level.time - ent->timestamp > BODY_SINK_TIME + 2500 ) {
		// the body ques are never actually freed, they are just unlinked
		trap->UnlinkEntity( (sharedEntity_t *)ent );
		ent->physicsObject = qfalse;
		return;
	}
//	ent->nextthink = level.time + 100;
//	ent->s.pos.trBase[2] -= 1;

	G_AddEvent(ent, EV_BODYFADE, 0);
	ent->nextthink = level.time + 18000;
	ent->takedamage = qfalse;
}

/*
=============
CopyToBodyQue

A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.
=============
*/
static qboolean CopyToBodyQue( gentity_t *ent ) {
	gentity_t		*body;
	int			contents;
	int			islight = 0;

	if (level.intermissiontime)
	{
		return qfalse;
	}

	trap->UnlinkEntity ((sharedEntity_t *)ent);

	// if client is in a nodrop area, don't leave the body
	contents = trap->PointContents( ent->s.origin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		return qfalse;
	}

	if (ent->client && (ent->client->ps.eFlags & EF_DISINTEGRATION))
	{ //for now, just don't spawn a body if you got disint'd
		return qfalse;
	}

	// grab a body que and cycle to the next one
	body = level.bodyQue[ level.bodyQueIndex ];
	level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;

	trap->UnlinkEntity ((sharedEntity_t *)body);
	body->s = ent->s;

	//avoid oddly angled corpses floating around
	body->s.angles[PITCH] = body->s.angles[ROLL] = body->s.apos.trBase[PITCH] = body->s.apos.trBase[ROLL] = 0;

	body->s.g2radius = 100;

	body->s.eType = ET_BODY;
	body->s.eFlags = EF_DEAD;		// clear EF_TALK, etc

	if (ent->client && (ent->client->ps.eFlags & EF_DISINTEGRATION))
	{
		body->s.eFlags |= EF_DISINTEGRATION;
	}

	VectorCopy(ent->client->ps.lastHitLoc, body->s.origin2);

	body->s.powerups = 0;	// clear powerups
	body->s.loopSound = 0;	// clear lava burning
	body->s.loopIsSoundset = qfalse;
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;		// don't bounce
	if ( body->s.groundEntityNum == ENTITYNUM_NONE ) {
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy( ent->client->ps.velocity, body->s.pos.trDelta );
	} else {
		body->s.pos.trType = TR_STATIONARY;
	}
	body->s.event = 0;

	body->s.weapon = ent->s.bolt2;

	if (body->s.weapon == WP_SABER && ent->client->ps.saberInFlight)
	{
		body->s.weapon = WP_MODULIZED_WEAPON; //lie to keep from putting a saber on the corpse, because it was thrown at death
	}

	//G_AddEvent(body, EV_BODY_QUEUE_COPY, ent->s.clientNum);
	//Now doing this through a modified version of the rcg reliable command.
	if (ent->client && ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
	{
		islight = 1;
	}
	trap->SendServerCommand(-1, va("ircg %i %i %i %i", ent->s.number, body->s.number, body->s.weapon, islight));

	body->r.svFlags = ent->r.svFlags | SVF_BROADCAST;
	VectorCopy (ent->r.mins, body->r.mins);
	VectorCopy (ent->r.maxs, body->r.maxs);
	VectorCopy (ent->r.absmin, body->r.absmin);
	VectorCopy (ent->r.absmax, body->r.absmax);

	body->s.torsoAnim = body->s.legsAnim = ent->client->ps.legsAnim;

	body->s.customRGBA[0] = ent->client->ps.customRGBA[0];
	body->s.customRGBA[1] = ent->client->ps.customRGBA[1];
	body->s.customRGBA[2] = ent->client->ps.customRGBA[2];
	body->s.customRGBA[3] = ent->client->ps.customRGBA[3];

#ifndef __DISABLE_PLAYERCLIP__
	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
#else //__DISABLE_PLAYERCLIP__
	body->clipmask = CONTENTS_SOLID;
#endif //__DISABLE_PLAYERCLIP__
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->s.number;

	body->nextthink = level.time + BODY_SINK_TIME;
	body->think = BodySink;

	body->die = body_die;

	// don't take more damage if already gibbed
	if ( ent->health <= GIB_HEALTH ) {
		body->takedamage = qfalse;
	} else {
		body->takedamage = qtrue;
	}

	VectorCopy ( body->s.pos.trBase, body->r.currentOrigin );
	trap->LinkEntity ((sharedEntity_t *)body);

	return qtrue;
}

//======================================================================


/*
==================
SetClientViewAngle

==================
*/
void SetClientViewAngle( gentity_t *ent, vec3_t angle ) {
	int			i;

	// set the delta angle
	for (i=0 ; i<3 ; i++) {
		int		cmdAngle;

		cmdAngle = ANGLE2SHORT(angle[i]);
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy( angle, ent->s.angles );
	VectorCopy (ent->s.angles, ent->client->ps.viewangles);
}

void MaintainBodyQueue(gentity_t *ent)
{ //do whatever should be done taking ragdoll and dismemberment states into account.
	qboolean doRCG = qfalse;

	assert(ent && ent->client);
	if (ent->client->tempSpectate >= level.time ||
		(ent->client->ps.eFlags2 & EF2_SHIP_DEATH))
	{
		ent->client->noCorpse = qtrue;
	}

	if (!ent->client->noCorpse && !ent->client->ps.fallingToDeath)
	{
		if (!CopyToBodyQue (ent))
		{
			doRCG = qtrue;
		}
	}
	else
	{
		ent->client->noCorpse = qfalse; //clear it for next time
		ent->client->ps.fallingToDeath = qfalse;
		doRCG = qtrue;
	}

	if (doRCG)
	{ //bodyque func didn't manage to call ircg so call this to assure our limbs and ragdoll states are proper on the client.
		trap->SendServerCommand(-1, va("rcg %i", ent->s.clientNum));
	}
}

/*
================
ClientRespawn
================
*/
void SiegeRespawn(gentity_t *ent);
void ClientRespawn( gentity_t *ent ) {
	MaintainBodyQueue(ent);

	if (gEscaping || level.gametype == GT_POWERDUEL)
	{
		ent->client->sess.sessionTeam = FACTION_SPECTATOR;
		ent->client->sess.spectatorState = SPECTATOR_FREE;
		ent->client->sess.spectatorClient = 0;

		ent->client->pers.teamState.state = TEAM_BEGIN;
		AddTournamentQueue(ent->client);
		ClientSpawn(ent);
		ent->client->iAmALoser = qtrue;
		return;
	}

	trap->UnlinkEntity ((sharedEntity_t *)ent);

	if (level.gametype == GT_SIEGE)
	{
		if (g_siegeRespawn.integer)
		{
			if (ent->client->tempSpectate < level.time)
			{
				int minDel = g_siegeRespawn.integer* 2000;
				if (minDel < 20000)
				{
					minDel = 20000;
				}
				ent->client->tempSpectate = level.time + minDel;
				ent->health = ent->client->ps.stats[STAT_HEALTH] = 1;
				ent->waterlevel = ent->watertype = 0;
				ent->client->ps.weapon = WP_NONE;
				ent->client->ps.primaryWeapon = 0;
				ent->client->ps.secondaryWeapon = 0;
				ent->client->ps.temporaryWeapon = 0;
				ent->client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
				ent->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
				ent->takedamage = qfalse;
				trap->LinkEntity((sharedEntity_t *)ent);

				// Respawn time.
				if ( ent->s.number < MAX_CLIENTS )
				{
					gentity_t *te = G_TempEntity( ent->client->ps.origin, EV_SIEGESPEC );
					te->s.time = g_siegeRespawnCheck;
					te->s.owner = ent->s.number;
				}

				return;
			}
		}
		SiegeRespawn(ent);
	}
	else
	{
		ClientSpawn(ent);
	}
}

/*
================
TeamCount

Returns number of players on a team
================
*/
int TeamCount( int ignoreClientNum, team_t team ) {
	int		i;
	int		count = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( i == ignoreClientNum ) {
			continue;
		}
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( level.clients[i].sess.sessionTeam == team ) {
			count++;
		}
		else if (level.gametype == GT_SIEGE &&
            level.clients[i].sess.siegeDesiredTeam == team)
		{
			count++;
		}
	}

	return count;
}

/*
================
TeamLeader

Returns the client number of the team leader
================
*/
int TeamLeader( int team ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( level.clients[i].sess.sessionTeam == team ) {
			if ( level.clients[i].sess.teamLeader )
				return i;
		}
	}

	return -1;
}


/*
================
PickTeam

================
*/
team_t PickTeam( int ignoreClientNum ) {
	int		counts[FACTION_NUM_FACTIONS];

	counts[FACTION_REBEL] = TeamCount( ignoreClientNum, FACTION_REBEL );
	counts[FACTION_EMPIRE] = TeamCount( ignoreClientNum, FACTION_EMPIRE );

	if ( counts[FACTION_REBEL] > counts[FACTION_EMPIRE] ) {
		return FACTION_EMPIRE;
	}
	if ( counts[FACTION_EMPIRE] > counts[FACTION_REBEL] ) {
		return FACTION_REBEL;
	}
	// equal team count, so join the team with the lowest score
	if ( level.teamScores[FACTION_REBEL] > level.teamScores[FACTION_EMPIRE] ) {
		return FACTION_EMPIRE;
	}
	return FACTION_REBEL;
}

/*
===========
ForceClientSkin

Forces a client's skin (for teamplay)
===========
*/
/*
static void ForceClientSkin( gclient_t *client, char *model, const char *skin ) {
	char *p;

	if ((p = Q_strrchr(model, '/')) != 0) {
		*p = 0;
	}

	Q_strcat(model, MAX_QPATH, "/");
	Q_strcat(model, MAX_QPATH, skin);
}
*/

/*
===========
ClientCheckName
============
*/
static void ClientCleanName( const char *in, char *out, int outSize )
{
	int outpos = 0, colorlessLen = 0, spaces = 0, ats = 0;

	// discard leading spaces
	for ( ; *in == ' '; in++);

	// discard leading asterisk's (fail raven for using * as a skipnotify)
	// apparently .* causes the issue too so... derp
	//for(; *in == '*'; in++);

	for(; *in && outpos < outSize - 1; in++)
	{
		out[outpos] = *in;

		if ( *in == ' ' )
		{// don't allow too many consecutive spaces
			if ( spaces > 2 )
				continue;

			spaces++;
		}
		else if ( *in == '@' )
		{// don't allow too many consecutive at signs
			if ( ++ats > 2 ) {
				outpos -= 2;
				ats = 0;
				continue;
			}
		}
		else if ( (byte)*in < 0x20
				|| (byte)*in == 0x81 || (byte)*in == 0x8D || (byte)*in == 0x8F || (byte)*in == 0x90 || (byte)*in == 0x9D
				|| (byte)*in == 0xA0 || (byte)*in == 0xAD )
		{
			continue;
		}
		else if ( outpos > 0 && out[outpos-1] == Q_COLOR_ESCAPE )
		{
			if ( Q_IsColorStringExt( &out[outpos-1] ) )
			{
				colorlessLen--;

#if 0
				if ( ColorIndex( *in ) == 0 )
				{// Disallow color black in names to prevent players from getting advantage playing in front of black backgrounds
					outpos--;
					continue;
				}
#endif
			}
			else
			{
				spaces = ats = 0;
				colorlessLen++;
			}
		}
		else
		{
			spaces = ats = 0;
			colorlessLen++;
		}

		outpos++;
	}

	out[outpos] = '\0';

	// don't allow empty names
	if ( *out == '\0' || colorlessLen == 0 )
		Q_strncpyz( out, "Padawan", outSize );
}

#ifdef _DEBUG
void G_DebugWrite(const char *path, const char *text)
{
	fileHandle_t f;

	trap->FS_Open( path, &f, FS_APPEND );
	trap->FS_Write(text, strlen(text), f);
	trap->FS_Close(f);
}
#endif

qboolean G_SaberModelSetup(gentity_t *ent)
{
	int i = 0;
	qboolean fallbackForSaber = qtrue;

	while (i < MAX_SABERS)
	{
		if (ent->client->saber[i].model[0])
		{
			//first kill it off if we've already got it
			if (ent->client->weaponGhoul2[i])
			{
				trap->G2API_CleanGhoul2Models(&(ent->client->weaponGhoul2[i]));
			}
			trap->G2API_InitGhoul2Model(&ent->client->weaponGhoul2[i], ent->client->saber[i].model, 0, 0, -20, 0, 0);

			if (ent->client->weaponGhoul2[i])
			{
				int j = 0;
				char *tagName;
				int tagBolt;

				if (ent->client->saber[i].skin)
				{
					trap->G2API_SetSkin(ent->client->weaponGhoul2[i], 0, ent->client->saber[i].skin, ent->client->saber[i].skin);
				}

				if (ent->client->saber[i].saberFlags & SFL_BOLT_TO_WRIST)
				{
					trap->G2API_SetBoltInfo(ent->client->weaponGhoul2[i], 0, 3+i);
				}
				else
				{ // bolt to right hand for 0, or left hand for 1
					trap->G2API_SetBoltInfo(ent->client->weaponGhoul2[i], 0, i);
				}

				//Add all the bolt points
				while (j < ent->client->saber[i].numBlades)
				{
					tagName = va("*blade%i", j+1);
					tagBolt = trap->G2API_AddBolt(ent->client->weaponGhoul2[i], 0, tagName);

					if (tagBolt == -1)
					{
						if (j == 0)
						{ //guess this is an 0ldsk3wl saber
							tagBolt = trap->G2API_AddBolt(ent->client->weaponGhoul2[i], 0, "*flash");
							fallbackForSaber = qfalse;
							break;
						}

						if (tagBolt == -1)
						{
							assert(0);
							break;

						}
					}
					j++;

					fallbackForSaber = qfalse; //got at least one custom saber so don't need default
				}

				//Copy it into the main instance
				trap->G2API_CopySpecificGhoul2Model(ent->client->weaponGhoul2[i], 0, ent->ghoul2, i+1);
			}
		}
		else
		{
			break;
		}

		i++;
	}

	return fallbackForSaber;
}

/*
===========
SetupGameGhoul2Model

There are two ghoul2 model instances per player (actually three).  One is on the clientinfo (the base for the client side
player, and copied for player spawns and for corpses).  One is attached to the centity itself, which is the model acutally
animated and rendered by the system.  The final is the game ghoul2 model.  This is animated by pmove on the server, and
is used for determining where the lightsaber should be, and for per-poly collision tests.
===========
*/
void *g2SaberInstance = NULL;

qboolean BG_IsValidCharacterModel(const char *modelName, const char *skinName);
qboolean BG_ValidateSkinForTeam( const char *modelName, char *skinName, int team, float *colors );
void BG_GetVehicleModelName(char *modelName, size_t len);

void SetupGameGhoul2Model(gentity_t *ent, char *modelname, char *skinName)
{
	int handle;
	char		afilename[MAX_QPATH];
#if 0
	char		/**GLAName,*/ *slash;
#endif
	char		GLAName[MAX_QPATH];
	vec3_t	tempVec = {0,0,0};

	if (strlen(modelname) >= MAX_QPATH )
	{
		Com_Error( ERR_FATAL, "SetupGameGhoul2Model(%s): modelname exceeds MAX_QPATH.\n", modelname );
	}
	if (skinName && strlen(skinName) >= MAX_QPATH )
	{
		Com_Error( ERR_FATAL, "SetupGameGhoul2Model(%s): skinName exceeds MAX_QPATH.\n", skinName );
	}

	// First things first.  If this is a ghoul2 model, then let's make sure we demolish this first.
	if (ent->ghoul2 && trap->G2API_HaveWeGhoul2Models(ent->ghoul2))
	{
		trap->G2API_CleanGhoul2Models(&(ent->ghoul2));
	}

	//rww - just load the "standard" model for the server"
	if (!precachedKyle)
	{
		int defSkin;

		Com_sprintf( afilename, sizeof( afilename ), "models/players/kyle/model.glm" );
		handle = trap->G2API_InitGhoul2Model(&precachedKyle, afilename, 0, 0, -20, 0, 0);

		if (handle<0)
		{
			return;
		}

		defSkin = trap->R_RegisterSkin("models/players/kyle/model_default.skin");
		trap->G2API_SetSkin(precachedKyle, 0, defSkin, defSkin);
	}

	if (precachedKyle && trap->G2API_HaveWeGhoul2Models(precachedKyle))
	{
		if (d_perPlayerGhoul2.integer || ent->s.number >= MAX_CLIENTS ||
			G_PlayerHasCustomSkeleton(ent))
		{ //rww - allow option for perplayer models on server for collision and bolt stuff.
			char modelFullPath[MAX_QPATH];
			char truncModelName[MAX_QPATH];
			char skin[MAX_QPATH];
			char vehicleName[MAX_QPATH];
			int skinHandle = 0;
			int i = 0;
			char *p;

			// If this is a vehicle, get it's model name.
			if ( ent->client->NPC_class == CLASS_VEHICLE )
			{
				char realModelName[MAX_QPATH];

				Q_strncpyz( vehicleName, modelname, sizeof( vehicleName ) );
				strcpy(realModelName, vehicleName);
				BG_GetVehicleModelName(realModelName, sizeof( realModelName ));
				strcpy(truncModelName, realModelName);
				skin[0] = 0;
				if ( ent->m_pVehicle
					&& ent->m_pVehicle->m_pVehicleInfo
					&& ent->m_pVehicle->m_pVehicleInfo->skin
					&& ent->m_pVehicle->m_pVehicleInfo->skin[0] )
				{
					skinHandle = trap->R_RegisterSkin(va("models/players/%s/model_%s.skin", realModelName, ent->m_pVehicle->m_pVehicleInfo->skin));
				}
				else
				{
					skinHandle = trap->R_RegisterSkin(va("models/players/%s/model_default.skin", realModelName));
				}
			}
			else
			{
				if (skinName && skinName[0])
				{
					strcpy(skin, skinName);
					strcpy(truncModelName, modelname);
				}
				else
				{
					strcpy(skin, "default");

					strcpy(truncModelName, modelname);
					p = Q_strrchr(truncModelName, '/');

					if (p)
					{
						*p = 0;
						p++;

						while (p && *p)
						{
							skin[i] = *p;
							i++;
							p++;
						}
						skin[i] = 0;
						i = 0;
					}

					if (!BG_IsValidCharacterModel(truncModelName, skin))
					{
						strcpy(truncModelName, DEFAULT_MODEL);
						strcpy(skin, "default");
					}

					if ( level.gametype >= GT_TEAM && level.gametype != GT_SIEGE && !g_jediVmerc.integer )
					{
						float colorOverride[3];

						colorOverride[0] = colorOverride[1] = colorOverride[2] = 0.0f;

						BG_ValidateSkinForTeam( truncModelName, skin, ent->client->sess.sessionTeam, colorOverride);
						if (colorOverride[0] != 0.0f ||
							colorOverride[1] != 0.0f ||
							colorOverride[2] != 0.0f)
						{
							ent->client->ps.customRGBA[0] = colorOverride[0]*255.0f;
							ent->client->ps.customRGBA[1] = colorOverride[1]*255.0f;
							ent->client->ps.customRGBA[2] = colorOverride[2]*255.0f;
						}

						//BG_ValidateSkinForTeam( truncModelName, skin, ent->client->sess.sessionTeam, NULL );
					}
					else if (level.gametype == GT_SIEGE)
					{ //force skin for class if appropriate
						if (ent->client->siegeClass != -1)
						{
							siegeClass_t *scl = &bgSiegeClasses[ent->client->siegeClass];
							if (scl->forcedSkin[0])
							{
								Q_strncpyz( skin, scl->forcedSkin, sizeof( skin ) );
							}
						}
					}
				}
			}

			if (skin[0])
			{
				char *useSkinName;

				if (strchr(skin, '|'))
				{//three part skin
					useSkinName = va("models/players/%s/|%s", truncModelName, skin);
				}
				else
				{
					useSkinName = va("models/players/%s/model_%s.skin", truncModelName, skin);
				}

				skinHandle = trap->R_RegisterSkin(useSkinName);
			}

			strcpy(modelFullPath, va("models/players/%s/model.glm", truncModelName));
			handle = trap->G2API_InitGhoul2Model(&ent->ghoul2, modelFullPath, 0, skinHandle, -20, 0, 0);

			if (handle<0)
			{ //Huh. Guess we don't have this model. Use the default.

				if (ent->ghoul2 && trap->G2API_HaveWeGhoul2Models(ent->ghoul2))
				{
					trap->G2API_CleanGhoul2Models(&(ent->ghoul2));
				}
				ent->ghoul2 = NULL;
				trap->G2API_DuplicateGhoul2Instance(precachedKyle, &ent->ghoul2);
			}
			else
			{
				trap->G2API_SetSkin(ent->ghoul2, 0, skinHandle, skinHandle);

				GLAName[0] = 0;
				trap->G2API_GetGLAName( ent->ghoul2, 0, GLAName);

				if (!GLAName[0] || (!strstr(GLAName, "players/_humanoid/") && ent->s.number < MAX_CLIENTS && !G_PlayerHasCustomSkeleton(ent)))
				{ //a bad model
					trap->G2API_CleanGhoul2Models(&(ent->ghoul2));
					ent->ghoul2 = NULL;
					trap->G2API_DuplicateGhoul2Instance(precachedKyle, &ent->ghoul2);
				}

				if (ent->s.number >= MAX_CLIENTS)
				{
					ent->s.modelGhoul2 = 1; //so we know to free it on the client when we're removed.

					if (skin[0])
					{ //append it after a *
						strcat( modelFullPath, va("*%s", skin) );
					}

					if ( ent->client->NPC_class == CLASS_VEHICLE )
					{ //vehicles are tricky and send over their vehicle names as the model (the model is then retrieved based on the vehicle name)
						ent->s.modelindex = G_ModelIndex(vehicleName);
					}
					else
					{
						ent->s.modelindex = G_ModelIndex(modelFullPath);
					}
				}
			}
		}
		else
		{
			trap->G2API_DuplicateGhoul2Instance(precachedKyle, &ent->ghoul2);
		}
	}
	else
	{
		return;
	}

	//Attach the instance to this entity num so we can make use of client-server
	//shared operations if possible.
	trap->G2API_AttachInstanceToEntNum(ent->ghoul2, ent->s.number, qtrue);

	// The model is now loaded.

	GLAName[0] = 0;

	if (!BGPAFtextLoaded)
	{
		if (BG_ParseAnimationFile("models/players/_humanoid/animation.cfg", bgHumanoidAnimations, qtrue) == -1)
		{
			Com_Printf( "Failed to load humanoid animation file\n");
			return;
		}
	}

	if (ent->s.number >= MAX_CLIENTS || G_PlayerHasCustomSkeleton(ent))
	{
		ent->localAnimIndex = -1;

		GLAName[0] = 0;
		trap->G2API_GetGLAName(ent->ghoul2, 0, GLAName);

		if (GLAName[0] &&
			!strstr(GLAName, "players/_humanoid/") /*&&
			!strstr(GLAName, "players/rockettrooper/")*/)
		{ //it doesn't use humanoid anims.
			char *slash = Q_strrchr( GLAName, '/' );
			if ( slash )
			{
				strcpy(slash, "/animation.cfg");

				ent->localAnimIndex = BG_ParseAnimationFile(GLAName, NULL, qfalse);
			}
		}
		else
		{ //humanoid index.
			if (strstr(GLAName, "players/rockettrooper/"))
			{
				ent->localAnimIndex = 1;
			}
			else
			{
				ent->localAnimIndex = 0;
			}
		}

		if (ent->localAnimIndex == -1)
		{
			Com_Error(ERR_DROP, "NPC had an invalid GLA\n");
		}
	}
	else
	{
		GLAName[0] = 0;
		trap->G2API_GetGLAName(ent->ghoul2, 0, GLAName);

		if (strstr(GLAName, "players/rockettrooper/"))
		{
			//assert(!"Should not have gotten in here with rockettrooper skel");
			ent->localAnimIndex = 1;
		}
		else
		{
			ent->localAnimIndex = 0;
		}
	}

	if (ent->s.NPC_class == CLASS_VEHICLE &&
		ent->m_pVehicle)
	{ //do special vehicle stuff
		char strTemp[128];
		int i;

		// Setup the default first bolt
		i = trap->G2API_AddBolt( ent->ghoul2, 0, "model_root" );

		// Setup the droid unit.
		ent->m_pVehicle->m_iDroidUnitTag = trap->G2API_AddBolt( ent->ghoul2, 0, "*droidunit" );

		// Setup the Exhausts.
		for ( i = 0; i < MAX_VEHICLE_EXHAUSTS; i++ )
		{
			Com_sprintf( strTemp, 128, "*exhaust%i", i + 1 );
			ent->m_pVehicle->m_iExhaustTag[i] = trap->G2API_AddBolt( ent->ghoul2, 0, strTemp );
		}

		// Setup the Muzzles.
		for ( i = 0; i < MAX_VEHICLE_MUZZLES; i++ )
		{
			Com_sprintf( strTemp, 128, "*muzzle%i", i + 1 );
			ent->m_pVehicle->m_iMuzzleTag[i] = trap->G2API_AddBolt( ent->ghoul2, 0, strTemp );
			if ( ent->m_pVehicle->m_iMuzzleTag[i] == -1 )
			{//ergh, try *flash?
				Com_sprintf( strTemp, 128, "*flash%i", i + 1 );
				ent->m_pVehicle->m_iMuzzleTag[i] = trap->G2API_AddBolt( ent->ghoul2, 0, strTemp );
			}
		}

		// Setup the Turrets.
		for ( i = 0; i < MAX_VEHICLE_TURRET_MUZZLES; i++ )
		{
			if ( ent->m_pVehicle->m_pVehicleInfo->turret[i].gunnerViewTag )
			{
				ent->m_pVehicle->m_iGunnerViewTag[i] = trap->G2API_AddBolt( ent->ghoul2, 0, ent->m_pVehicle->m_pVehicleInfo->turret[i].gunnerViewTag );
			}
			else
			{
				ent->m_pVehicle->m_iGunnerViewTag[i] = -1;
			}
		}
	}

	if (ent->client->ps.weapon == WP_SABER || ent->s.number < MAX_CLIENTS || (ent->NPC && ent->client->NPC_class != CLASS_VEHICLE))
	{ //a player or NPC saber user
		trap->G2API_AddBolt(ent->ghoul2, 0, "*r_hand");
		trap->G2API_AddBolt(ent->ghoul2, 0, "*l_hand");

		//rhand must always be first bolt. lhand always second. Whichever you want the
		//jetpack bolted to must always be third.
		trap->G2API_AddBolt(ent->ghoul2, 0, "*chestg");

		//claw bolts
		trap->G2API_AddBolt(ent->ghoul2, 0, "*r_hand_cap_r_arm");
		trap->G2API_AddBolt(ent->ghoul2, 0, "*l_hand_cap_l_arm");

		trap->G2API_SetBoneAnim(ent->ghoul2, 0, "model_root", 0, 12, BONE_ANIM_OVERRIDE_LOOP, 1.0f, level.time, -1, -1);
		trap->G2API_SetBoneAngles(ent->ghoul2, 0, "upper_lumbar", tempVec, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, level.time);
		trap->G2API_SetBoneAngles(ent->ghoul2, 0, "cranium", tempVec, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_Y, POSITIVE_X, NULL, 0, level.time);

		if (!g2SaberInstance)
		{
			trap->G2API_InitGhoul2Model(&g2SaberInstance, "models/weapons2/saber/saber_w.glm", 0, 0, -20, 0, 0);

			if (g2SaberInstance)
			{
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				trap->G2API_SetBoltInfo(g2SaberInstance, 0, 0);
				// now set up the gun bolt on it
				trap->G2API_AddBolt(g2SaberInstance, 0, "*blade1");
			}
		}

		if (G_SaberModelSetup(ent))
		{
			if (g2SaberInstance)
			{
				trap->G2API_CopySpecificGhoul2Model(g2SaberInstance, 0, ent->ghoul2, 1);
			}
		}
	}

	if (ent->s.number >= MAX_CLIENTS && !(ent->NPC && ent->client->NPC_class != CLASS_VEHICLE))
	{ //some extra NPC stuff
		if (trap->G2API_AddBolt(ent->ghoul2, 0, "lower_lumbar") == -1)
		{ //check now to see if we have this bone for setting anims and such
			ent->noLumbar = qtrue;
		}
	}
}




/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap->SetUserinfo
if desired.
============
*/

qboolean G_SetSaber(gentity_t *ent, int saberNum, char *saberName, qboolean siegeOverride);
void G_ValidateSiegeClassForTeam(gentity_t *ent, int team);

typedef struct userinfoValidate_s {
	const char		*field, *fieldClean;
	unsigned int	minCount, maxCount;
} userinfoValidate_t;

#define UIF( x, _min, _max ) { STRING(\\) #x STRING(\\), STRING( x ), _min, _max }
static userinfoValidate_t userinfoFields[] = {
	UIF( cl_guid,			0, 0 ), // not allowed, q3fill protection
	UIF( cl_punkbuster,		0, 0 ), // not allowed, q3fill protection
	UIF( ip,				0, 1 ), // engine adds this at the end
	UIF( name,				1, 1 ),
	UIF( rate,				1, 1 ),
	UIF( snaps,				1, 1 ),
	UIF( model,				1, 1 ),
	UIF( forcepowers,		1, 1 ),
	UIF( color1,			1, 1 ),
	UIF( color2,			1, 1 ),
	UIF( handicap,			1, 1 ),
	UIF( sex,				0, 1 ),
	UIF( cg_predictItems,	1, 1 ),
	UIF( saber1,			1, 1 ),
	UIF( saber2,			1, 1 ),
	UIF( char_color_red,	1, 1 ),
	UIF( char_color_green,	1, 1 ),
	UIF( char_color_blue,	1, 1 ),
	UIF( teamtask,			0, 1 ), // optional
	UIF( password,			0, 1 ), // optional
	UIF( teamoverlay,		0, 1 ), // only registered in cgame, not sent when connecting
};
static const size_t numUserinfoFields = ARRAY_LEN( userinfoFields );

static const char *userinfoValidateExtra[USERINFO_VALIDATION_MAX] = {
	"Size",					// USERINFO_VALIDATION_SIZE
	"# of slashes",			// USERINFO_VALIDATION_SLASH
	"Extended ascii",		// USERINFO_VALIDATION_EXTASCII
	"Control characters",	// USERINFO_VALIDATION_CONTROLCHARS
};

void Svcmd_ToggleUserinfoValidation_f( void ) {
	if ( trap->Argc() == 1 ) {
		int i=0;
		for ( i=0; i<numUserinfoFields; i++ ) {
			if ( (g_userinfoValidate.integer & (1<<i)) )	trap->Print( "%2d [X] %s\n", i, userinfoFields[i].fieldClean );
			else											trap->Print( "%2d [ ] %s\n", i, userinfoFields[i].fieldClean );
		}
		for ( ; i<numUserinfoFields+USERINFO_VALIDATION_MAX; i++ ) {
			if ( (g_userinfoValidate.integer & (1<<i)) )	trap->Print( "%2d [X] %s\n", i, userinfoValidateExtra[i-numUserinfoFields] );
			else											trap->Print( "%2d [ ] %s\n", i, userinfoValidateExtra[i-numUserinfoFields] );
		}
		return;
	}
	else {
		char arg[8]={0};
		int index;

		trap->Argv( 1, arg, sizeof( arg ) );
		index = atoi( arg );

		if ( index < 0 || index > numUserinfoFields+USERINFO_VALIDATION_MAX-1 ) {
			Com_Printf( "ToggleUserinfoValidation: Invalid range: %i [0, %i]\n", index, numUserinfoFields+USERINFO_VALIDATION_MAX-1 );
			return;
		}

		trap->Cvar_Set( "g_userinfoValidate", va( "%i", (1<<index) ^ g_userinfoValidate.integer ) );
		trap->Cvar_Update( &g_userinfoValidate );

		if ( index < numUserinfoFields )	Com_Printf( "%s %s\n", userinfoFields[index].fieldClean,				((g_userinfoValidate.integer & (1<<index)) ? "Validated" : "Ignored") );
		else								Com_Printf( "%s %s\n", userinfoValidateExtra[index-numUserinfoFields],	((g_userinfoValidate.integer & (1<<index)) ? "Validated" : "Ignored") );
	}
}

char *G_ValidateUserinfo( const char *userinfo ) {
	unsigned int		i=0, count=0;
	size_t				length = strlen( userinfo );
	userinfoValidate_t	*info = NULL;
	char				key[BIG_INFO_KEY], value[BIG_INFO_VALUE];
	const char			*s;
	unsigned int		fieldCount[ARRAY_LEN( userinfoFields )];

	memset( fieldCount, 0, sizeof( fieldCount ) );

	// size checks
	if ( g_userinfoValidate.integer & (1<<(numUserinfoFields+USERINFO_VALIDATION_SIZE)) ) {
		if ( length < 1 )
			return "Userinfo too short";
		else if ( length >= MAX_INFO_STRING )
			return "Userinfo too long";
	}

	// slash checks
	if ( g_userinfoValidate.integer & (1<<(numUserinfoFields+USERINFO_VALIDATION_SLASH)) ) {
		// there must be a leading slash
		if ( userinfo[0] != '\\' )
			return "Missing leading slash";

		// no trailing slashes allowed, engine will append ip\\ip:port
		if ( userinfo[length-1] == '\\' )
			return "Trailing slash";

		// format for userinfo field is: \\key\\value
		// so there must be an even amount of slashes
		for ( i=0, count=0; i<length; i++ ) {
			if ( userinfo[i] == '\\' )
				count++;
		}
		if ( (count&1) ) // odd
			return "Bad number of slashes";
	}

	// extended characters are impossible to type, may want to disable
	if ( g_userinfoValidate.integer & (1<<(numUserinfoFields+USERINFO_VALIDATION_EXTASCII)) ) {
		for ( i=0, count=0; i<length; i++ ) {
			if ( userinfo[i] < 0 )
				count++;
		}
		if ( count )
			return "Extended ASCII characters found";
	}

	// disallow \n \r ; and \"
	if ( g_userinfoValidate.integer & (1<<(numUserinfoFields+USERINFO_VALIDATION_CONTROLCHARS)) ) {
		if ( Q_strchrs( userinfo, "\n\r;\"" ) )
			return "Invalid characters found";
	}

	s = userinfo;
	while ( s ) {
		Info_NextPair( &s, key, value );

		if ( !key[0] )
			break;

		for ( i=0; i<numUserinfoFields; i++ ) {
			if ( !Q_stricmp( key, userinfoFields[i].fieldClean ) )
				fieldCount[i]++;
		}
	}

	// count the number of fields
	for ( i=0, info=userinfoFields; i<numUserinfoFields; i++, info++ ) {
		if ( g_userinfoValidate.integer & (1<<i) ) {
			if ( info->minCount && !fieldCount[i] )
				return va( "%s field not found", info->fieldClean );
			else if ( fieldCount[i] > info->maxCount )
				return va( "Too many %s fields (%i/%i)", info->fieldClean, fieldCount[i], info->maxCount );
		}
	}

	return NULL;
}

extern void DOM_SetFakeNPCName(gentity_t *ent);
extern void NPC_Precache ( gentity_t *spawner );

void StripModelName( const char *in, char *out, int destsize )
{
	const char *slash = strrchr(in, '/');
	if (slash)
		destsize = (destsize < slash-in+1 ? destsize : slash-in+1);

	if ( in == out && destsize > 1 )
		out[destsize-1] = '\0';
	else
		Q_strncpyz(out, in, destsize);
}

extern qboolean WP_SaberParseParms(const char *saberName, saberInfo_t *saber);

void G_CheckSaber(gentity_t *ent)
{
	if (!ent || !ent->client)
	{
		return;
	}

	if (ent->s.number >= MAX_CLIENTS && ent->s.eType != ET_NPC)
	{
		return;
	}

	qboolean changed = qfalse;
	inventoryItem *saber = BG_EquippedWeapon(&ent->client->ps);

	if (saber && saber->getBaseItem()->giTag == WP_SABER)
	{
		if (saber->getIsTwoHanded())
		{
			if (saber->getModelType() == SABER_MODELTYPE_ELECTROSTAFF)
			{
				if (Q_stricmp(ent->client->pers.saber1, "electrostaff"))
				{
					if (ent->s.eType == ET_NPC)
					{
						WP_SaberParseParms("electrostaff", &ent->client->saber[0]);
						ent->s.npcSaber1 = G_ModelIndex(va("@%s", "electrostaff"));
						ent->s.npcSaber2 = G_ModelIndex(va("@%s", "none"));
						Q_strncpyz(ent->client->pers.saber1, "electrostaff", sizeof(ent->client->pers.saber1));
					}
					else
					{
						G_SetSaber(ent, 0, "electrostaff", qtrue);
						G_SetSaber(ent, 1, "none", qtrue);
						changed = qtrue;
					}
				}
			}
			else
			{
				if (Q_stricmp(ent->client->pers.saber1, "dual_1"))
				{
					if (ent->s.eType == ET_NPC)
					{
						WP_SaberParseParms("dual_1", &ent->client->saber[0]);
						ent->s.npcSaber1 = G_ModelIndex(va("@%s", "dual_1"));
						ent->s.npcSaber2 = G_ModelIndex(va("@%s", "none"));
						Q_strncpyz(ent->client->pers.saber1, "dual_1", sizeof(ent->client->pers.saber1));
					}
					else
					{
						G_SetSaber(ent, 0, "dual_1", qtrue);
						G_SetSaber(ent, 1, "none", qtrue);
						changed = qtrue;
					}
				}
			}
		}
		else
		{
			if (Q_stricmp(ent->client->pers.saber1, "single_16"))
			{
				if (ent->s.eType == ET_NPC)
				{
					WP_SaberParseParms("single_16", &ent->client->saber[0]);
					ent->s.npcSaber1 = G_ModelIndex(va("@%s", "single_16"));
					ent->s.npcSaber2 = G_ModelIndex(va("@%s", "none"));
					Q_strncpyz(ent->client->pers.saber1, "single_16", sizeof(ent->client->pers.saber1));
				}
				else
				{
					G_SetSaber(ent, 0, "single_16", qtrue);
					G_SetSaber(ent, 1, "none", qtrue);
					changed = qtrue;
				}
			}
		}
	}
	else
	{
		if (Q_stricmp(ent->client->pers.saber1, "single_16"))
		{
			if (ent->s.eType == ET_NPC)
			{
				WP_SaberParseParms("single_16", &ent->client->saber[0]);
				ent->s.npcSaber1 = G_ModelIndex(va("@%s", "single_16"));
				ent->s.npcSaber2 = G_ModelIndex(va("@%s", "none"));
				Q_strncpyz(ent->client->pers.saber1, "single_16", sizeof(ent->client->pers.saber1));
			}
			else
			{
				G_SetSaber(ent, 0, "single_16", qtrue);
				G_SetSaber(ent, 1, "none", qtrue);
				changed = qtrue;
			}
		}
	}

	if (changed)
	{
		ClientUserinfoChanged(ent->s.number);
	}
}

qboolean ClientUserinfoChanged( int clientNum ) {
	gentity_t *ent = g_entities + clientNum;
	gclient_t *client = ent->client;
	int team=FACTION_FREE, health=100, maxHealth=100, teamLeader;
	const char *s=NULL;
	char *value=NULL, userinfo[MAX_INFO_STRING], buf[MAX_INFO_STRING], oldClientinfo[MAX_INFO_STRING], model[MAX_QPATH],
		forcePowers[DEFAULT_FORCEPOWERS_LEN], oldname[MAX_NETNAME], className[MAX_QPATH], color1[16], color2[16];
	qboolean modelChanged = qfalse;
	//gender_t gender = GENDER_MALE;

	//[RGBSabers]
	char	rgb1[MAX_INFO_STRING];
	char	rgb2[MAX_INFO_STRING];
	char	script1[MAX_INFO_STRING];
	char	script2[MAX_INFO_STRING];
	//[/RGBSabers]

	trap->GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check for malformed or illegal info strings
	s = G_ValidateUserinfo( userinfo );
	if ( s && *s ) {
		G_SecurityLogPrintf( "Client %d (%s) failed userinfo validation: %s [IP: %s]\n", clientNum, ent->client->pers.netname, s, client->sess.IP );
		trap->DropClient( clientNum, va( "Failed userinfo validation: %s", s ) );
		G_LogPrintf( "Userinfo: %s\n", userinfo );
		return qfalse;
	}

	// check for local client
	s = Info_ValueForKey( userinfo, "ip" );
	if ( !strcmp( s, "localhost" ) && !(ent->r.svFlags & SVF_BOT) )
		client->pers.localClient = qtrue;

	// check the item prediction
	s = Info_ValueForKey( userinfo, "cg_predictItems" );
	if ( !atoi( s ) )	client->pers.predictItemPickup = qfalse;
	else				client->pers.predictItemPickup = qtrue;

	// set name

	if (ent->s.eFlags & EF_FAKE_NPC_BOT)
	{
		s = Info_ValueForKey( userinfo, "name" );
		ClientCleanName( s, oldname, sizeof( oldname ) );
		ClientCleanName( client->pers.netname, client->pers.netname_nocolor, sizeof( client->pers.netname_nocolor ) );
		Q_strncpyz( client->pers.netname_nocolor, client->pers.netname, sizeof( client->pers.netname_nocolor ) );
		Q_StripColor( client->pers.netname_nocolor );
	}
	else
	{
		Q_strncpyz( oldname, client->pers.netname, sizeof( oldname ) );
		s = Info_ValueForKey( userinfo, "name" );
		ClientCleanName( s, client->pers.netname, sizeof( client->pers.netname ) );
		Q_strncpyz( client->pers.netname_nocolor, client->pers.netname, sizeof( client->pers.netname_nocolor ) );
		Q_StripColor( client->pers.netname_nocolor );
	}

	if ( client->sess.sessionTeam == FACTION_SPECTATOR && client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
		Q_strncpyz( client->pers.netname, "scoreboard", sizeof( client->pers.netname ) );
		Q_strncpyz( client->pers.netname_nocolor, "scoreboard", sizeof( client->pers.netname_nocolor ) );
	}

	if ( client->pers.connected == CON_CONNECTED && strcmp( oldname, client->pers.netname ) ) {
		if (ent->s.eFlags & EF_FAKE_NPC_BOT)
		{
			s = Info_ValueForKey( userinfo, "name" );
			ClientCleanName( s, oldname, sizeof( oldname ) );
			ClientCleanName( client->pers.netname, client->pers.netname_nocolor, sizeof( client->pers.netname_nocolor ) );
			Q_strncpyz( client->pers.netname_nocolor, client->pers.netname, sizeof( client->pers.netname_nocolor ) );
			Q_StripColor( client->pers.netname_nocolor );

			Info_SetValueForKey( userinfo, "name", client->pers.netname );
			trap->SetUserinfo( clientNum, userinfo );

			trap->SendServerCommand( -1, va( "print \"%s" S_COLOR_WHITE " %s %s\n\"", oldname, G_GetStringEdString( "MP_SVGAME", "PLRENAME" ), client->pers.netname ) );
			G_LogPrintf( "ClientRename: %i [%s] (%s) \"%s^7\" -> \"%s^7\"\n", clientNum, ent->client->sess.IP, ent->client->pers.guid, oldname, ent->client->pers.netname );
			client->pers.netnameTime = level.time + 5000;
		}
		else
		{
			if ( client->pers.netnameTime > level.time ) {
				trap->SendServerCommand( clientNum, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NONAMECHANGE" ) ) );

				Info_SetValueForKey( userinfo, "name", oldname );
				trap->SetUserinfo( clientNum, userinfo );
				Q_strncpyz( client->pers.netname, oldname, sizeof( client->pers.netname ) );
				Q_strncpyz( client->pers.netname_nocolor, oldname, sizeof( client->pers.netname_nocolor ) );
				Q_StripColor( client->pers.netname_nocolor );
			}
			else {
				trap->SendServerCommand( -1, va( "print \"%s" S_COLOR_WHITE " %s %s\n\"", oldname, G_GetStringEdString( "MP_SVGAME", "PLRENAME" ), client->pers.netname ) );
				G_LogPrintf( "ClientRename: %i [%s] (%s) \"%s^7\" -> \"%s^7\"\n", clientNum, ent->client->sess.IP, ent->client->pers.guid, oldname, ent->client->pers.netname );
				client->pers.netnameTime = level.time + 5000;
			}
		}
	}

	// set model
	Q_strncpyz( model, Info_ValueForKey( userinfo, "model" ), sizeof( model ) );

	if ( d_perPlayerGhoul2.integer&& Q_stricmp( model, client->modelname ) ) {
		Q_strncpyz( client->modelname, model, sizeof( client->modelname ) );
		modelChanged = qtrue;
	}

	
	//if (!(ent->s.eFlags & EF_FAKE_NPC_BOT))
	{// UQ1: Register NPC sounds for this model... This should work with most models...
		int i;

		// Initialize sounds before we begin...
		ent->client->ps.csSounds_Std = ent->s.csSounds_Std = 0;
		ent->client->ps.csSounds_Combat = ent->s.csSounds_Combat = 0;
		ent->client->ps.csSounds_Extra = ent->s.csSounds_Extra = 0;
		ent->client->ps.csSounds_Jedi = ent->s.csSounds_Jedi = 0;

		if (!ent->NPC_type || !ent->NPC_type[0]) ent->NPC_type = G_NewString(model);
		StripModelName(model, ent->NPC_type, sizeof(model));
		ent->NPC_type = Q_strlwr( ent->NPC_type );
	
		// Convert the spaces in the bot name to _ to match npc names...
		for (i = 0; i < strlen(ent->NPC_type); i++)
		{
			if (ent->NPC_type[i] == ' ') 
				ent->NPC_type[i] = '_';
		}

#if 0
		//trap->Print("Model is %s.\n", ent->NPC_type);

		// Try using model name as sound name... Should work most of the time...
		ent->s.csSounds_Std = G_SoundIndex( va("*$%s", ent->NPC_type) );
		ent->s.csSounds_Combat = G_SoundIndex( va("*$%s", ent->NPC_type) );
		ent->s.csSounds_Extra = G_SoundIndex( va("*$%s", ent->NPC_type) );
		ent->s.csSounds_Jedi = G_SoundIndex( va("*$%s", ent->NPC_type) );

		if (!(ent->s.csSounds_Std || ent->s.csSounds_Combat || ent->s.csSounds_Extra || ent->s.csSounds_Jedi))
		{// Still failed to find sounds... Try using NPC Type as sound name but with variation 1...
			ent->s.csSounds_Std = G_SoundIndex( va("*$%s1", ent->NPC_type) );
			ent->s.csSounds_Combat = G_SoundIndex( va("*$%s1", ent->NPC_type) );
			ent->s.csSounds_Extra = G_SoundIndex( va("*$%s1", ent->NPC_type) );
			ent->s.csSounds_Jedi = G_SoundIndex( va("*$%s1", ent->NPC_type) );
		}

		if (!(ent->s.csSounds_Std || ent->s.csSounds_Combat || ent->s.csSounds_Extra || ent->s.csSounds_Jedi))
		{// Still failed to find sounds... Try using NPC Type as sound name but with variation 2...
			ent->s.csSounds_Std = G_SoundIndex( va("*$%s2", ent->NPC_type) );
			ent->s.csSounds_Combat = G_SoundIndex( va("*$%s2", ent->NPC_type) );
			ent->s.csSounds_Extra = G_SoundIndex( va("*$%s2", ent->NPC_type) );
			ent->s.csSounds_Jedi = G_SoundIndex( va("*$%s2", ent->NPC_type) );
		}

		if (!(ent->s.csSounds_Std || ent->s.csSounds_Combat || ent->s.csSounds_Extra || ent->s.csSounds_Jedi))
		{// Still failed to find sounds... Try using model name for sounds...
			ent->s.csSounds_Std = G_SoundIndex( va("*$%s", ent->client->modelname) );
			ent->s.csSounds_Combat = G_SoundIndex( va("*$%s", ent->client->modelname) );
			ent->s.csSounds_Extra = G_SoundIndex( va("*$%s", ent->client->modelname) );
			ent->s.csSounds_Jedi = G_SoundIndex( va("*$%s", ent->client->modelname) );
		}

		if (!(ent->s.csSounds_Std || ent->s.csSounds_Combat || ent->s.csSounds_Extra || ent->s.csSounds_Jedi))
		{// Still failed to find sounds... Try using model name as sound name but with variation 1...
			ent->s.csSounds_Std = G_SoundIndex( va("*$%s1", ent->client->modelname) );
			ent->s.csSounds_Combat = G_SoundIndex( va("*$%s1", ent->client->modelname) );
			ent->s.csSounds_Extra = G_SoundIndex( va("*$%s1", ent->client->modelname) );
			ent->s.csSounds_Jedi = G_SoundIndex( va("*$%s1", ent->client->modelname) );
		}

		if (!(ent->s.csSounds_Std || ent->s.csSounds_Combat || ent->s.csSounds_Extra || ent->s.csSounds_Jedi))
		{// Still failed to find sounds... Try using model name as sound name but with variation 2...
			ent->s.csSounds_Std = G_SoundIndex( va("*$%s2", ent->client->modelname) );
			ent->s.csSounds_Combat = G_SoundIndex( va("*$%s2", ent->client->modelname) );
			ent->s.csSounds_Extra = G_SoundIndex( va("*$%s2", ent->client->modelname) );
			ent->s.csSounds_Jedi = G_SoundIndex( va("*$%s2", ent->client->modelname) );
		}
		
		if (!(ent->s.csSounds_Std || ent->s.csSounds_Combat || ent->s.csSounds_Extra || ent->s.csSounds_Jedi))
		{// Failed to find sounds... Try NPC sound precache...
			NPC_Precache(ent);
		}
#else
		{
			gentity_t *newent = ent;
			int SOUND_INDEX = 0;

			// Try to precache just like an npc does...
			NPC_Precache(newent);

			SOUND_INDEX = newent->s.csSounds_Std;

			if (!SOUND_INDEX)
			{// Still failed to find sounds... Try using NPC Type as sound name but with variation 1...
				SOUND_INDEX = G_SoundIndex( va("*$%s", newent->NPC_type) );
			}

			if (!SOUND_INDEX)
			{// Still failed to find sounds... Try using NPC Type as sound name but with variation 1...
				SOUND_INDEX = G_SoundIndex( va("*$%s1", newent->NPC_type) );
			}

			if (!SOUND_INDEX)
			{// Still failed to find sounds... Try using NPC Type as sound name but with variation 2...
				SOUND_INDEX = G_SoundIndex( va("*$%s2", newent->NPC_type) );
			}

			if (!SOUND_INDEX)
			{// Still failed to find sounds... Try using model name for sounds...
				SOUND_INDEX = G_SoundIndex( va("*$%s", newent->client->modelname) );
			}

			//trap->Print("%i given sound index %i.\n", newent->s.number, SOUND_INDEX);

			// Try using NPC Type as sound name... Should work most of the time...
			newent->s.csSounds_Std = SOUND_INDEX;
			newent->s.csSounds_Combat = SOUND_INDEX;
			newent->s.csSounds_Extra = SOUND_INDEX;
			newent->s.csSounds_Jedi = SOUND_INDEX;
		}
#endif

		ent->client->ps.csSounds_Std = ent->s.csSounds_Std;
		ent->client->ps.csSounds_Combat = ent->s.csSounds_Combat;
		ent->client->ps.csSounds_Extra = ent->s.csSounds_Extra;
		ent->client->ps.csSounds_Jedi = ent->s.csSounds_Jedi;
		//trap->Print("SERVER: %i %i %i %i.\n", ent->client->ps.csSounds_Std, ent->client->ps.csSounds_Combat, ent->client->ps.csSounds_Extra, ent->client->ps.csSounds_Jedi);
	}
	

	client->ps.customRGBA[0] = (value=Info_ValueForKey( userinfo, "char_color_red" ))	? Com_Clampi( 0, 255, atoi( value ) ) : 255;
	client->ps.customRGBA[1] = (value=Info_ValueForKey( userinfo, "char_color_green" ))	? Com_Clampi( 0, 255, atoi( value ) ) : 255;
	client->ps.customRGBA[2] = (value=Info_ValueForKey( userinfo, "char_color_blue" ))	? Com_Clampi( 0, 255, atoi( value ) ) : 255;

	//Prevent skins being too dark
	if ( g_charRestrictRGB.integer && ((client->ps.customRGBA[0]+client->ps.customRGBA[1]+client->ps.customRGBA[2]) < 100) )
		client->ps.customRGBA[0] = client->ps.customRGBA[1] = client->ps.customRGBA[2] = 255;

	client->ps.customRGBA[3]=255;

#ifdef __STANDARDIZED_MODEL_SCALING__
	vec3_t		playerMaxs = { 15, 15, DEFAULT_MAXS_2 };

	if (StringContainsWord(model, "padawan") || StringContainsWord(model, "roxas"))
	{// Padawan/child model is scale 0.88.
		ent->modelScale[0] = ent->modelScale[1] = ent->modelScale[2] = 0.88f;
		client->ps.iModelScale = ent->modelScale[0] * 88;
		ent->s.iModelScale = ent->modelScale[0] * 88;

		ent->r.maxs[0] = playerMaxs[0] * ent->modelScale[0];
		ent->r.maxs[1] = playerMaxs[1] * ent->modelScale[1];
		ent->r.maxs[2] = playerMaxs[2] * ent->modelScale[2];

		ent->client->ps.standheight = DEFAULT_MAXS_2 * ent->modelScale[2];
		ent->client->ps.crouchheight = CROUCH_MAXS_2 * ent->modelScale[2];
	}
	else
	{// Standard player is scale 1.0.
		ent->modelScale[0] = ent->modelScale[1] = ent->modelScale[2] = 1.0f;
		client->ps.iModelScale = ent->modelScale[0] * 100;
		ent->s.iModelScale = ent->modelScale[0] * 100;

		ent->r.maxs[0] = playerMaxs[0] * ent->modelScale[0];
		ent->r.maxs[1] = playerMaxs[1] * ent->modelScale[1];
		ent->r.maxs[2] = playerMaxs[2] * ent->modelScale[2];

		ent->client->ps.standheight = DEFAULT_MAXS_2 * ent->modelScale[2];
		ent->client->ps.crouchheight = CROUCH_MAXS_2 * ent->modelScale[2];
	}
#else //!__STANDARDIZED_MODEL_SCALING__
	if (scale_models_loaded)
	{
		int			loop = 0;
		qboolean	found = qfalse;
		vec3_t		playerMaxs = {15, 15, DEFAULT_MAXS_2};

		/*
		if ( Q_stricmp( model, client->modelname ) ) {
			Q_strncpyz( client->modelname, model, sizeof( client->modelname ) );
			modelChanged = qtrue;
		}
		*/

		for (loop = 0; loop < num_scale_models; loop++)
		{
			if (!Q_strncmp(model_scale_list[loop].botName, model, strlen(model_scale_list[loop].botName)))
			{// A match! Set the scale!
				if (ent->modelScale[2] != model_scale_list[loop].scale / 100.0f)
				{
					ent->modelScale[0] = ent->modelScale[1] = ent->modelScale[2] = model_scale_list[loop].scale / 100.0f;
					client->ps.iModelScale = model_scale_list[loop].scale;
					ent->s.iModelScale = model_scale_list[loop].scale;

					ent->r.maxs[0] = playerMaxs[0] * ent->modelScale[0];
					ent->r.maxs[1] = playerMaxs[1] * ent->modelScale[1];
					ent->r.maxs[2] = playerMaxs[2] * ent->modelScale[2];

					ent->client->ps.standheight = DEFAULT_MAXS_2 * ent->modelScale[2];
					ent->client->ps.crouchheight = CROUCH_MAXS_2 * ent->modelScale[2];

					Com_Printf("^1*** ^3MODEL-SCALE^5: Scale set to ^7%fx^5 normal for model %s.\n", model_scale_list[loop].scale / 100.0f, model);
				}

				found = qtrue;
				break;
			}
		}

		if (!found)
		{
			ent->modelScale[0] = ent->modelScale[1] = ent->modelScale[2] = 1.0f;
			client->ps.iModelScale = ent->modelScale[0] * 100;
			ent->s.iModelScale = ent->modelScale[0] * 100;

			ent->r.maxs[0] = playerMaxs[0] * ent->modelScale[0];
			ent->r.maxs[1] = playerMaxs[1] * ent->modelScale[1];
			ent->r.maxs[2] = playerMaxs[2] * ent->modelScale[2];

			ent->client->ps.standheight = DEFAULT_MAXS_2 * ent->modelScale[2];
			ent->client->ps.crouchheight = CROUCH_MAXS_2 * ent->modelScale[2];

			Com_Printf("^1*** ^3MODEL-SCALE^5: Scale set to ^7%fx^5 normal for UNKNOWN model %s.\n", 1.0f, model);
		}
	}
#endif //__STANDARDIZED_MODEL_SCALING__

	Q_strncpyz( forcePowers, Info_ValueForKey( userinfo, "forcepowers" ), sizeof( forcePowers ) );

#ifdef __FORCED_TEAM_COLORS__
	// update our customRGBA for team colors.
	if ( level.gametype >= GT_TEAM && level.gametype != GT_SIEGE && !g_jediVmerc.integer ) {
		char skin[MAX_QPATH] = {0};
		vec3_t colorOverride = {0.0f};

		VectorClear( colorOverride );

		BG_ValidateSkinForTeam( model, skin, client->sess.sessionTeam, colorOverride );
		if ( colorOverride[0] != 0.0f || colorOverride[1] != 0.0f || colorOverride[2] != 0.0f )
			VectorScaleM( colorOverride, 255.0f, client->ps.customRGBA );
	}
#endif //__FORCED_TEAM_COLORS__

	// bots set their team a few frames later
	if ( level.gametype >= GT_TEAM && g_entities[clientNum].r.svFlags & SVF_BOT ) {
		s = Info_ValueForKey( userinfo, "team" );
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) )
			team = FACTION_EMPIRE;
		else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) )
			team = FACTION_REBEL;
		else
			team = PickTeam( clientNum ); // pick the team with the least number of players
	}
	else
		team = client->sess.sessionTeam;

	//Testing to see if this fixes the problem with a bot's team getting set incorrectly.
	team = client->sess.sessionTeam;

	//Set the siege class
	if (qtrue /*level.gametype == GT_SIEGE*/) {
		Q_strncpyz( className, client->sess.siegeClass, sizeof( className ) );

		//Now that the team is legal for sure, we'll go ahead and get an index for it.
		client->siegeClass = BG_SiegeFindClassIndexByName( className );
		if ( client->siegeClass == -1 ) {
			// ok, get the first valid class for the team you're on then, I guess.
			BG_SiegeCheckClassLegality( team, className );
			Q_strncpyz( client->sess.siegeClass, className, sizeof( client->sess.siegeClass ) );
			client->siegeClass = BG_SiegeFindClassIndexByName( className );
		}
		else {
			// otherwise, make sure the class we are using is legal.
			G_ValidateSiegeClassForTeam( ent, team );
			Q_strncpyz( className, client->sess.siegeClass, sizeof( className ) );
		}

		if ( client->siegeClass != -1 ) {
			// Set the sabers if the class dictates
			siegeClass_t *scl = &bgSiegeClasses[client->siegeClass];

#if 0
			G_SetSaber( ent, 0, scl->saber1[0] ? scl->saber1 : DEFAULT_SABER, qtrue );
			G_SetSaber( ent, 1, scl->saber2[0] ? scl->saber2 : "none", qtrue );
#endif

			//make sure the saber models are updated
			G_SaberModelSetup( ent );

			if ( scl->forcedModel[0] ) {
				// be sure to override the model we actually use
				Q_strncpyz( model, scl->forcedModel, sizeof( model ) );
				if ( d_perPlayerGhoul2.integer && Q_stricmp( model, client->modelname ) ) {
					Q_strncpyz( client->modelname, model, sizeof( client->modelname ) );
					modelChanged = qtrue;
				}
			}

			if ( G_PlayerHasCustomSkeleton( ent ) )
			{//force them to use their class model on the server, if the class dictates
				if ( Q_stricmp( model, client->modelname ) || ent->localAnimIndex == 0 )
				{
					Q_strncpyz( client->modelname, model, sizeof( client->modelname ) );
					modelChanged = qtrue;
				}
			}
		}
	}
	else
		Q_strncpyz( className, "none", sizeof( className ) );

#if 0
	// only set the saber name on the first connect.
	//	it will be read from userinfo on ClientSpawn and stored in client->pers.saber1/2
	if ( !VALIDSTRING( client->pers.saber1 ) || !VALIDSTRING( client->pers.saber2 ) ) {
		G_SetSaber( ent, 0, Info_ValueForKey( userinfo, "saber1" ), qfalse );
		G_SetSaber( ent, 1, Info_ValueForKey( userinfo, "saber2" ), qfalse );
	}
#else
	inventoryItem *saber = BG_EquippedWeapon(&ent->client->ps);

	if (saber && saber->getBaseItem()->giTag == WP_SABER)
	{
		if (saber->getIsTwoHanded())
		{
			if (saber->getModelType() == SABER_MODELTYPE_ELECTROSTAFF)
			{
				if (Q_stricmp(ent->client->saber[0].model, "electrostaff"))
				{
					G_SetSaber(ent, 0, "electrostaff", qtrue);
					G_SetSaber(ent, 1, "none", qtrue);

					Info_SetValueForKey(userinfo, "saber1", "electrostaff");
					Info_SetValueForKey(userinfo, "saber2", "none");
				}
			}
			else
			{
				if (Q_stricmp(ent->client->saber[0].model, "dual_1"))
				{
					G_SetSaber(ent, 0, "dual_1", qtrue);
					G_SetSaber(ent, 1, "none", qtrue);

					Info_SetValueForKey(userinfo, "saber1", "dual_1");
					Info_SetValueForKey(userinfo, "saber2", "none");
				}
			}
		}
		else
		{
			if (Q_stricmp(ent->client->saber[0].model, "single_16"))
			{
				G_SetSaber(ent, 0, "single_16", qtrue);
				G_SetSaber(ent, 1, "none", qtrue);

				Info_SetValueForKey(userinfo, "saber1", "single_16");
				Info_SetValueForKey(userinfo, "saber2", "none");
			}
		}
	}
#endif

#if 0
	// set max health
	if ( level.gametype == GT_SIEGE && client->siegeClass != -1 ) {
		siegeClass_t *scl = &bgSiegeClasses[client->siegeClass];

		if ( scl->maxhealth )
			maxHealth = scl->maxhealth;

		health = maxHealth;
	}
	else
		health = Com_Clampi( 1, 100, atoi( Info_ValueForKey( userinfo, "handicap" ) ) );
#else
	health = 1000;
#endif

	client->pers.maxHealth = health;
	if ( client->pers.maxHealth < 1 || client->pers.maxHealth > maxHealth )
#if 0
		client->pers.maxHealth = 100;
#else
		client->pers.maxHealth = 1000;
#endif
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;

	if ( level.gametype >= GT_TEAM )
		client->pers.teamInfo = qtrue;
	else {
		s = Info_ValueForKey( userinfo, "teamoverlay" );
		if ( !*s || atoi( s ) != 0 )
			client->pers.teamInfo = qtrue;
		else
			client->pers.teamInfo = qfalse;
	}

	// team task (0 = none, 1 = offence, 2 = defence)
//	teamTask = atoi(Info_ValueForKey(userinfo, "teamtask"));
	// team Leader (1 = leader, 0 is normal player)
	teamLeader = client->sess.teamLeader;

	// colors
	Q_strncpyz( color1, Info_ValueForKey( userinfo, "color1" ), sizeof( color1 ) );
	Q_strncpyz( color2, Info_ValueForKey( userinfo, "color2" ), sizeof( color2 ) );

	//[RGBSabers]
	Q_strncpyz(rgb1, Info_ValueForKey(userinfo, "rgb_saber1"), sizeof(rgb1));
	Q_strncpyz(rgb2, Info_ValueForKey(userinfo, "rgb_saber2"), sizeof(rgb2));

	Q_strncpyz(script1, Info_ValueForKey(userinfo, "rgb_script1"), sizeof(script1));
	Q_strncpyz(script2, Info_ValueForKey(userinfo, "rgb_script2"), sizeof(script2));


	//	Com_Printf("game > newinfo update > sab1 \"%s\" sab2 \"%s\" \n",rgb1,rgb2);

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	//[/RGBSabers]

	/*
	UQ1: Ummm... How about we actually look at sounds.cfg and set gender info???
	*/

	{
		char			*modelname = Info_ValueForKey( userinfo, "model" );
		const char		*dir = modelname;
		char			use_dir[64];
		fileHandle_t	f;
		int				fLen = 0;
		char			*buf;
		int				num = 0;
		int				NUM_LINES = 0;

		StripModelName( dir, use_dir, 64 );

		//try default sounds.cfg first
		fLen = trap->FS_Open(va("models/players/%s/sounds.cfg", use_dir), &f, FS_READ);

		if ( !f )
		{//no?  Look for _default sounds.cfg
			//trap->Print("%s does not exist.\n", va("models/players/%s/sounds.cfg", use_dir));

			fLen = trap->FS_Open(va("models/players/%s/sounds_default.cfg", use_dir), &f, FS_READ);

			/*if ( !f )
			{
				trap->Print("%s does not exist.\n", va("models/players/%s/sounds_default.cfg", use_dir));
			}
			else
			{
				trap->Print("%s exists.\n", va("models/players/%s/sounds_default.cfg", use_dir));
			}*/
		}
		/*else
		{
			trap->Print("%s exists.\n", va("models/players/%s/sounds.cfg", use_dir));
		}*/

		if ( f )
		{// Seems we have a sounds.cfg file... Read/Use it...
			if ( (buf = (char *)malloc( fLen + 1)) != 0 )
			{//alloc memory for buffer
				char			*s, *t;

				// Read in the whole file...
				trap->FS_Read( buf, fLen, f );
				buf[fLen] = 0;
				trap->FS_Close( f );

				// 
				for ( t = s = buf; *t; /* */ )
				{
					char sex[1];
					sex[0] = s[0];

					num++;
					s = strchr( s, '\n' );

					if ( !s && *t )
					{// UQ1: pff no newline on the sex value in the cfg... Hopefully this is all we need...
						//trap->Print("%s sex is %s.\n", modelname, sex);
						ent->s.extra_flags &= ~EXF_GENDER_MALE;
						ent->s.extra_flags &= ~EXF_GENDER_FEMALE;
						ent->s.extra_flags &= ~EXF_GENDER_DROID;

						// Set sex...
						if (sex[0] == 'm') {
							Info_SetValueForKey( userinfo, "sex", "male" );
							ent->s.extra_flags |= EXF_GENDER_MALE;
							//trap->Print("%s sex set to MALE.\n", modelname);
							break;
						}
						else if (sex[0] == 'f') {
							Info_SetValueForKey( userinfo, "sex", "female" );
							ent->s.extra_flags |= EXF_GENDER_FEMALE;
							//trap->Print("%s sex set to FEMALE.\n", modelname);
							break;
						}
						else if (sex[0] == 'd') {
							Info_SetValueForKey( userinfo, "sex", "droid" );
							ent->s.extra_flags |= EXF_GENDER_DROID;
							//trap->Print("%s sex set to DROID.\n", modelname);
							break;
						}
						else if (sex[0] == 'n') {
							Info_SetValueForKey( userinfo, "sex", "neuter" );
							//trap->Print("%s sex set to NEUTER.\n", modelname);
							break;
						}
						else {
							Info_SetValueForKey( userinfo, "sex", "male" );
							ent->s.extra_flags |= EXF_GENDER_MALE;
							//trap->Print("%s sex set to MALE (default).\n", modelname);
							break;
						}
					}

					if ( !s || num > fLen )
					{
						break;
					}

					while ( *s == '\n' )
					{
						*s++ = 0;
					}

					if ( *t )
					{
						if ( t[0] != '\0' && Q_strncmp( "//", va( "%s", t), 2) )
						{	// Not a comment either... Record it in our list...
							if (NUM_LINES < 1)
							{// Sound dir...
								//strcpy(OUT, t);
							}
							else
							{// line 1 is gender "m" or "f" or "n".
								ent->s.extra_flags &= ~EXF_GENDER_MALE;
								ent->s.extra_flags &= ~EXF_GENDER_FEMALE;
								ent->s.extra_flags &= ~EXF_GENDER_DROID;

								// Set sex...
								if (sex[0] == 'm') {
									Info_SetValueForKey( userinfo, "sex", "male" );
									ent->s.extra_flags |= EXF_GENDER_MALE;
									break;
								}
								else if (sex[0] == 'f') {
									Info_SetValueForKey( userinfo, "sex", "female" );
									ent->s.extra_flags |= EXF_GENDER_FEMALE;
									break;
								}
								else if (sex[0] == 'd') {
									Info_SetValueForKey( userinfo, "sex", "droid" );
									ent->s.extra_flags |= EXF_GENDER_DROID;
									break;
								}
								else if (sex[0] == 'n') {
									Info_SetValueForKey( userinfo, "sex", "neuter" );
									break;
								}
								else {
									Info_SetValueForKey( userinfo, "sex", "male" );
									ent->s.extra_flags |= EXF_GENDER_MALE;
									break;
								}
							}

							NUM_LINES++;
						}
					}

					t = s;
				}

				free(buf);
			}

			trap->FS_Close( f );
		}
	}

	/*
	// gender hints
	s = Info_ValueForKey( userinfo, "sex" );
	if ( !Q_stricmp( s, "male" ) )
	{
		gender = GENDER_MALE;
		
	}
	else if ( !Q_stricmp( s, "female" ) )
	{
		gender = GENDER_FEMALE;
	}
	else if ( !Q_stricmp( s, "droid" ) )
	{
		gender = GENDER_DROID;
	}
	else
	{
		gender = GENDER_NEUTER;
	}
	*/

	s = Info_ValueForKey( userinfo, "snaps" );
	if ( atoi( s ) < sv_fps.integer )
		trap->SendServerCommand( clientNum, va( "print \"" S_COLOR_YELLOW"Recommend setting /snaps %d or higher to match this server's sv_fps\n\"", sv_fps.integer ) );

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	buf[0] = '\0';
	Q_strcat( buf, sizeof( buf ), va( "n\\%s\\", client->pers.netname ) );
	Q_strcat( buf, sizeof( buf ), va( "t\\%i\\", client->sess.sessionTeam ) );
	Q_strcat( buf, sizeof( buf ), va( "model\\%s\\", model ) );
	//	 if ( gender == GENDER_MALE )	Q_strcat( buf, sizeof( buf ), va( "ds\\%c\\", 'm' ) );
	//else if ( gender == GENDER_FEMALE )	Q_strcat( buf, sizeof( buf ), va( "ds\\%c\\", 'f' ) );
	//else								Q_strcat( buf, sizeof( buf ), va( "ds\\%c\\", 'n' ) );
	Q_strcat( buf, sizeof( buf ), va( "st\\%s\\", client->pers.saber1 ) );
	Q_strcat( buf, sizeof( buf ), va( "st2\\%s\\", client->pers.saber2 ) );
	Q_strcat( buf, sizeof( buf ), va( "c1\\%s\\", color1 ) );
	Q_strcat( buf, sizeof( buf ), va( "c2\\%s\\", color2 ) );
	Q_strcat( buf, sizeof( buf ), va( "hc\\%i\\", client->pers.maxHealth ) );
	if ( ent->r.svFlags & SVF_BOT )
		Q_strcat( buf, sizeof( buf ), va( "skill\\%s\\", Info_ValueForKey( userinfo, "skill" ) ) );
	if ( level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL ) {
		Q_strcat( buf, sizeof( buf ), va( "w\\%i\\", client->sess.wins ) );
		Q_strcat( buf, sizeof( buf ), va( "l\\%i\\", client->sess.losses ) );
	}
	if ( level.gametype == GT_POWERDUEL )
		Q_strcat( buf, sizeof( buf ), va( "dt\\%i\\", client->sess.duelTeam ) );
	if ( level.gametype >= GT_TEAM ) {
	//	Q_strcat( buf, sizeof( buf ), va( "tt\\%d\\", teamTask ) );
		Q_strcat( buf, sizeof( buf ), va( "tl\\%d\\", teamLeader ) );
	}
	if ( level.gametype == GT_SIEGE ) {
		Q_strcat( buf, sizeof( buf ), va( "siegeclass\\%s\\", className ) );
		Q_strcat( buf, sizeof( buf ), va( "sdt\\%i\\", className ) );
	}

	trap->GetConfigstring( CS_PLAYERS+clientNum, oldClientinfo, sizeof( oldClientinfo ) );
	if (strcmp( oldClientinfo, buf )) // UQ1: Only if it actually changed...
		trap->SetConfigstring( CS_PLAYERS+clientNum, buf );

	// only going to be true for allowable server-side custom skeleton cases
	if ( modelChanged ) {
		// update the server g2 instance if appropriate
		char *modelname = Info_ValueForKey( userinfo, "model" );
		SetupGameGhoul2Model( ent, modelname, NULL );

		if ( ent->ghoul2 && ent->client )
			ent->client->renderInfo.lastG2 = NULL; //update the renderinfo bolts next update.

		client->torsoAnimExecute = client->legsAnimExecute = -1;
		client->torsoLastFlip = client->legsLastFlip = qfalse;
	}

	if ( g_logClientInfo.integer ) {
		if ( strcmp( oldClientinfo, buf ) )
			G_LogPrintf( "ClientUserinfoChanged: %i %s\n", clientNum, buf );
		else
			G_LogPrintf( "ClientUserinfoChanged: %i <no change>\n", clientNum );
	}

	return qtrue;
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournament restart.

The session information will be valid after exit.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournament
restarts.
============
*/

static qboolean CompareIPs( const char *ip1, const char *ip2 )
{
	while ( 1 ) {
		if ( *ip1 != *ip2 )
			return qfalse;
		if ( !*ip1 || *ip1 == ':' )
			break;
		ip1++;
		ip2++;
	}

	return qtrue;
}

char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot ) {
	char		*value = NULL;
	gentity_t	*ent = NULL, *te = NULL;
	gclient_t	*client;
	char		userinfo[MAX_INFO_STRING] = {0},
				tmpIP[NET_ADDRSTRMAXLEN] = {0},
				guid[33] = {0};

	ent = &g_entities[ clientNum ];

	ent->s.number = clientNum;
	ent->classname = "connecting";

	{// Free any NPC data they have...
		ent->s.NPC_NAME_ID = 0; // Init the type...
	}

	trap->GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	value = Info_ValueForKey( userinfo, "ja_guid" );
	if( value[0] )
		Q_strncpyz( guid, value, sizeof( guid ) );
	else if( isBot )
		Q_strncpyz( guid, "BOT", sizeof( guid ) );
	else
		Q_strncpyz( guid, "NOGUID", sizeof( guid ) );

	// check to see if they are on the banned IP list
	value = Info_ValueForKey (userinfo, "ip");
	Q_strncpyz( tmpIP, isBot ? "Bot" : value, sizeof( tmpIP ) );
	if ( G_FilterPacket( value ) ) {
		return "Banned.";
	}

	if ( !isBot && g_needpass.integer ) {
		// check for a password
		value = Info_ValueForKey (userinfo, "password");
		if ( g_password.string[0] && Q_stricmp( g_password.string, "none" ) &&
			strcmp( g_password.string, value) != 0) {
			static char sTemp[1024];
			Q_strncpyz(sTemp, G_GetStringEdString("MP_SVGAME","INVALID_ESCAPE_TO_MAIN"), sizeof (sTemp) );
			return sTemp;// return "Invalid password";
		}
	}

	if ( !isBot && firstTime )
	{
		if ( g_antiFakePlayer.integer )
		{// patched, check for > g_maxConnPerIP connections from same IP
			int count=0, i=0;

			for ( i=0; i<sv_maxclients.integer; i++ )
			{
				#if 0
					if ( level.clients[i].pers.connected != CON_DISCONNECTED && i != clientNum )
					{
						if ( CompareIPs( clientNum, i ) )
						{
							if ( !level.security.clientConnectionActive[i] )
							{//This IP has a dead connection pending, wait for it to time out
							//	client->pers.connected = CON_DISCONNECTED;
								return "Please wait, another connection from this IP is still pending...";
							}
						}
					}
				#else
					if ( CompareIPs( tmpIP, level.clients[i].sess.IP ) )
						count++;
				#endif
			}
			if ( count > g_maxConnPerIP.integer )
			{
			//	client->pers.connected = CON_DISCONNECTED;
				return "Too many connections from the same IP";
			}
		}
	}

	if ( ent->inuse )
	{// if a player reconnects quickly after a disconnect, the client disconnect may never be called, thus flag can get lost in the ether
		G_LogPrintf( "Forcing disconnect on active client: %i\n", clientNum );
		// so lets just fix up anything that should happen on a disconnect
		ClientDisconnect( clientNum );
	}

	// they can connect
	client = &level.clients[ clientNum ];
	ent->client = client;

	//assign the pointer for bg entity access
	ent->playerState = &ent->client->ps;

	memset( client, 0, sizeof(*client) );

	Q_strncpyz( client->pers.guid, guid, sizeof( client->pers.guid ) );

	client->pers.connected = CON_CONNECTING;
	client->pers.connectTime = level.time;

	// read or initialize the session data
	if ( firstTime || level.newSession ) {
		G_InitSessionData( client, userinfo, isBot );
	}
	G_ReadSessionData( client );

	if (level.gametype == GT_SIEGE &&
		(firstTime || level.newSession))
	{ //if this is the first time then auto-assign a desired siege team and show briefing for that team
		client->sess.siegeDesiredTeam = 0;//PickTeam(ent->s.number);
		/*
		trap->SendServerCommand(ent->s.number, va("sb %i", client->sess.siegeDesiredTeam));
		*/
		//don't just show it - they'll see it if they switch to a team on purpose.
	}


	if (level.gametype == GT_SIEGE && client->sess.sessionTeam != FACTION_SPECTATOR)
	{
		if (firstTime || level.newSession)
		{ //start as spec
			client->sess.siegeDesiredTeam = client->sess.sessionTeam;
			client->sess.sessionTeam = FACTION_SPECTATOR;
		}
	}
	else if (level.gametype == GT_POWERDUEL && client->sess.sessionTeam != FACTION_SPECTATOR)
	{
		client->sess.sessionTeam = FACTION_SPECTATOR;
	}

	if( isBot && ent->s.eType != ET_NPC) {
		ent->r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
		if( !G_BotConnect( clientNum, (qboolean)!firstTime ) ) {
			return "BotConnectfailed";
		}
	}

	// get and distribute relevent paramters
	if ( !ClientUserinfoChanged( clientNum ) )
		return "Failed userinfo validation";

	if ( !isBot && firstTime )
	{
		if ( !tmpIP[0] )
		{//No IP sent when connecting, probably an unban hack attempt
			client->pers.connected = CON_DISCONNECTED;
			G_SecurityLogPrintf( "Client %i (%s) sent no IP when connecting.\n", clientNum, client->pers.netname );
			return "Invalid userinfo detected";
		}
	}

	if ( firstTime )
		Q_strncpyz( client->sess.IP, tmpIP, sizeof( client->sess.IP ) );

	G_LogPrintf( "ClientConnect: %i [%s] (%s) \"%s^7\"\n", clientNum, tmpIP, guid, client->pers.netname );

#ifndef __MMO__
	// don't do the "xxx connected" messages if they were caried over from previous level
	if ( firstTime ) {
		trap->SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s\n\"", client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLCONNECT")) );
	}
#endif //__MMO__

	if ( level.gametype >= GT_TEAM &&
		client->sess.sessionTeam != FACTION_SPECTATOR ) {
		BroadcastTeamChange( client, -1 );
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	te = G_TempEntity( vec3_origin, EV_CLIENTJOIN );
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = clientNum;

	// for statistics
//	client->areabits = areabits;
//	if ( !client->areabits )
//		client->areabits = G_Alloc( (trap->AAS_PointReachabilityAreaIndex( NULL ) + 7) / 8 );

	return NULL;
}

void G_WriteClientSessionData( gclient_t *client );

void WP_SetSaber( int entNum, saberInfo_t *sabers, int saberNum, const char *saberName );

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
extern qboolean	gSiegeRoundBegun;
extern qboolean	gSiegeRoundEnded;
extern qboolean g_dontPenalizeTeam; //g_cmds.c
void SetTeamQuick(gentity_t *ent, int team, qboolean doBegin);
void ClientBegin( int clientNum, qboolean allowTeamReset ) {
	gentity_t	*ent;
	gclient_t	*client;
	int			flags, i;
	char		userinfo[MAX_INFO_VALUE], *modelname;
	int			spawnCount;

	ent = g_entities + clientNum;

	if ((ent->r.svFlags & SVF_BOT) && level.gametype >= GT_TEAM)
	{
		if (allowTeamReset)
		{
			const char *team = "Red";
			int preSess;

			//SetTeam(ent, "");
			ent->client->sess.sessionTeam = PickTeam(-1);
			trap->GetUserinfo(clientNum, userinfo, MAX_INFO_STRING);

			if (ent->client->sess.sessionTeam == FACTION_SPECTATOR)
			{
				ent->client->sess.sessionTeam = FACTION_EMPIRE;
			}

			if (ent->client->sess.sessionTeam == FACTION_EMPIRE)
			{
				team = "Red";
			}
			else
			{
				team = "Blue";
			}

			Info_SetValueForKey( userinfo, "team", team );

			trap->SetUserinfo( clientNum, userinfo );

			ent->client->ps.persistant[ PERS_TEAM ] = ent->client->sess.sessionTeam;

			preSess = ent->client->sess.sessionTeam;
			G_ReadSessionData( ent->client );
			ent->client->sess.sessionTeam = (team_t)preSess;
			G_WriteClientSessionData(ent->client);
			if ( !ClientUserinfoChanged( clientNum ) )
				return;
			ClientBegin(clientNum, qfalse);
			return;
		}
	}

	client = level.clients + clientNum;

	if ( ent->r.linked ) {
		trap->UnlinkEntity( (sharedEntity_t *)ent );
	}
	G_InitGentity( ent );
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	//assign the pointer for bg entity access
	ent->playerState = &ent->client->ps;

	client->pers.connected = CON_CONNECTED;
	client->pers.enterTime = level.time;
	client->pers.teamState.state = TEAM_BEGIN;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	flags = client->ps.eFlags;
	spawnCount = client->ps.persistant[PERS_SPAWN_COUNT];

	i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		if (ent->client->ps.fd.forcePowersActive & (1 << i))
		{
			WP_ForcePowerStop(ent, (forcePowers_t)i);
		}
		i++;
	}

	i = TRACK_CHANNEL_1;

	while (i < NUM_TRACK_CHANNELS)
	{
		if (ent->client->ps.fd.killSoundEntIndex[i-50] && ent->client->ps.fd.killSoundEntIndex[i-50] < MAX_GENTITIES && ent->client->ps.fd.killSoundEntIndex[i-50] > 0)
		{
			G_MuteSound(ent->client->ps.fd.killSoundEntIndex[i-50], CHAN_VOICE);
		}
		i++;
	}
	i = 0;

	memset( &client->ps, 0, sizeof( client->ps ) );
	client->ps.eFlags = flags;
	client->ps.persistant[PERS_SPAWN_COUNT] = spawnCount;

	client->ps.hasDetPackPlanted = qfalse;

	//first-time force power initialization
	WP_InitForcePowers( ent );

	//init saber ent
	WP_SaberInitBladeData( ent );

	// First time model setup for that player.
	trap->GetUserinfo( clientNum, userinfo, sizeof(userinfo) );
	modelname = Info_ValueForKey (userinfo, "model");
	SetupGameGhoul2Model(ent, modelname, NULL);

	if ( ent->ghoul2 && ent->client )
		ent->client->renderInfo.lastG2 = NULL; //update the renderinfo bolts next update.

	if ( level.gametype == GT_POWERDUEL && client->sess.sessionTeam != FACTION_SPECTATOR && client->sess.duelTeam == DUELTEAM_FREE )
		SetTeam( ent, "s" );
	else
	{
		if ( level.gametype == GT_SIEGE && (!gSiegeRoundBegun || gSiegeRoundEnded) )
			SetTeamQuick( ent, FACTION_SPECTATOR, qfalse );

		// locate ent at a spawn point
		ClientSpawn( ent );
	}

#ifndef __MMO__
	if ( client->sess.sessionTeam != FACTION_SPECTATOR ) {
		if ( level.gametype != GT_DUEL || level.gametype == GT_POWERDUEL ) {
			trap->SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s\n\"", client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLENTER")) );
		}
	}
#endif //__MMO__

	G_LogPrintf( "ClientBegin: %i\n", clientNum );

	// count current clients and rank for scoreboard
	CalculateRanks();
}

static qboolean AllForceDisabled(int force)
{
	int i;

	if (force)
	{
		for (i=0;i<NUM_FORCE_POWERS;i++)
		{
			if (!(force & (1<<i)))
			{
				return qfalse;
			}
		}

		return qtrue;
	}

	return qfalse;
}

//Convenient interface to set all my limb breakage stuff up -rww
void G_BreakArm(gentity_t *ent, int arm)
{
	int anim = -1;

	assert(ent && ent->client);

	if (ent->s.NPC_class == CLASS_VEHICLE || ent->localAnimIndex > 1)
	{ //no broken limbs for vehicles and non-humanoids
		return;
	}

	if (!arm)
	{ //repair him
		ent->client->ps.brokenLimbs = 0;
		return;
	}

	if (ent->client->ps.fd.saberAnimLevel == SS_STAFF)
	{ //I'm too lazy to deal with this as well for now.
		return;
	}

	if (arm == BROKENLIMB_LARM)
	{
		if (ent->client->saber[1].model[0] &&
			ent->client->ps.weapon == WP_SABER &&
			!ent->client->ps.saberHolstered &&
			ent->client->saber[1].soundOff)
		{ //the left arm shuts off its saber upon being broken
			G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
		}
	}

	ent->client->ps.brokenLimbs = 0; //make sure it's cleared out
	ent->client->ps.brokenLimbs |= (1 << arm); //this arm is now marked as broken

	//Do a pain anim based on the side. Since getting your arm broken does tend to hurt.
	if (arm == BROKENLIMB_LARM)
	{
		anim = BOTH_PAIN2;
	}
	else if (arm == BROKENLIMB_RARM)
	{
		anim = BOTH_PAIN3;
	}

	if (anim == -1)
	{
		return;
	}

	G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);

	//This could be combined into a single event. But I guess limbs don't break often enough to
	//worry about it.
	G_EntitySound( ent, CHAN_VOICE, G_SoundIndex("*pain25.wav") );
	//FIXME: A nice bone snapping sound instead if possible
	G_Sound(ent, CHAN_AUTO, G_SoundIndex( va("sound/player/bodyfall_human%i.wav", Q_irand(1, 3)) ));
}

//Update the ghoul2 instance anims based on the playerstate values
qboolean BG_SaberStanceAnim( int anim );
qboolean PM_RunningAnim( int anim );
void G_UpdateClientAnims(gentity_t *self, float animSpeedScale)
{
	static int f;
	static int torsoAnim;
	static int legsAnim;
	static int firstFrame, lastFrame;
	static int aFlags;
	static float animSpeed, lAnimSpeedScale;
	qboolean setTorso = qfalse;

	torsoAnim = (self->client->ps.torsoAnim);
	legsAnim = (self->client->ps.legsAnim);

	//if (torsoAnim > 32768) return; // obviously bad...
	//if (legsAnim > 32768) return; // obviously bad...

	if (self->client->ps.saberLockFrame)
	{
		trap->G2API_SetBoneAnim(self->ghoul2, 0, "model_root", self->client->ps.saberLockFrame, self->client->ps.saberLockFrame+1, BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND, animSpeedScale, level.time, -1, 150);
		trap->G2API_SetBoneAnim(self->ghoul2, 0, "lower_lumbar", self->client->ps.saberLockFrame, self->client->ps.saberLockFrame+1, BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND, animSpeedScale, level.time, -1, 150);
		trap->G2API_SetBoneAnim(self->ghoul2, 0, "Motion", self->client->ps.saberLockFrame, self->client->ps.saberLockFrame+1, BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND, animSpeedScale, level.time, -1, 150);
		return;
	}

	// JKG: Freezing/stun
	if (JKG_DamageTypeFreezes((const damageType_t)self->client->ps.damageTypeFlags))
	{
		const animation_t *torsoAnimData = &bgAllAnims[self->localAnimIndex].anims[self->client->ps.freezeTorsoAnim];
		const animation_t *legsAnimData = &bgAllAnims[self->localAnimIndex].anims[self->client->ps.freezeLegsAnim];
		int legsAnimFrame = legsAnimData->firstFrame + legsAnimData->numFrames;
		int torsoAnimFrame = torsoAnimData->firstFrame + torsoAnimData->numFrames;

		trap->G2API_SetBoneAnim(self->ghoul2, 0, "model_root", legsAnimFrame, legsAnimFrame, BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND, animSpeedScale, level.time, -1, 150);
		trap->G2API_SetBoneAnim(self->ghoul2, 0, "lower_lumbar", torsoAnimFrame, torsoAnimFrame, BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND, animSpeedScale, level.time, -1, 150);

		return;
	}

	if (self->localAnimIndex >= NUM_RESERVED_ANIMSETS &&
		bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame == 0 &&
		bgAllAnims[self->localAnimIndex].anims[legsAnim].numFrames == 0)
	{ //We'll allow this for non-humanoids.
		goto tryTorso;
	}

	if (self->client->legsAnimExecute != legsAnim || self->client->legsLastFlip != self->client->ps.legsFlip)
	{
		animSpeed = 50.0f / bgAllAnims[self->localAnimIndex].anims[legsAnim].frameLerp;
		lAnimSpeedScale = (animSpeed *= animSpeedScale);

		if (bgAllAnims[self->localAnimIndex].anims[legsAnim].loopFrames != -1)
		{
			aFlags = BONE_ANIM_OVERRIDE_LOOP;
		}
		else
		{
			aFlags = BONE_ANIM_OVERRIDE_FREEZE;
		}

		if (animSpeed < 0)
		{
			lastFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame;
			firstFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame + bgAllAnims[self->localAnimIndex].anims[legsAnim].numFrames;
		}
		else
		{
			firstFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame;
			lastFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame + bgAllAnims[self->localAnimIndex].anims[legsAnim].numFrames;
		}

		aFlags |= BONE_ANIM_BLEND; //since client defaults to blend. Not sure if this will make much difference if any on server position, but it's here just for the sake of matching them.

		trap->G2API_SetBoneAnim(self->ghoul2, 0, "model_root", firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
		self->client->legsAnimExecute = legsAnim;
		self->client->legsLastFlip = self->client->ps.legsFlip;
	}

tryTorso:
	if (self->localAnimIndex > 1 &&
		bgAllAnims[self->localAnimIndex].anims[torsoAnim].firstFrame == 0 &&
		bgAllAnims[self->localAnimIndex].anims[torsoAnim].numFrames == 0)

	{ //If this fails as well just return.
		return;
	}
	else if (self->s.number >= MAX_CLIENTS &&
		self->s.NPC_class == CLASS_VEHICLE)
	{ //we only want to set the root bone for vehicles
		return;
	}

	if ((self->client->torsoAnimExecute != torsoAnim || self->client->torsoLastFlip != self->client->ps.torsoFlip) &&
		!self->noLumbar)
	{
		aFlags = 0;
		animSpeed = 0;

		f = torsoAnim;

		BG_SaberStartTransAnim(self->s.number, &self->client->ps, self->client->ps.fd.saberAnimLevel, self->client->ps.fd.saberAnimLevelBase, self->client->ps.weapon, f, &animSpeedScale, self->client->ps.brokenLimbs);


		animSpeed = 50.0f / bgAllAnims[self->localAnimIndex].anims[f].frameLerp;
		lAnimSpeedScale = (animSpeed *= animSpeedScale);

		if (bgAllAnims[self->localAnimIndex].anims[f].loopFrames != -1)
		{
			aFlags = BONE_ANIM_OVERRIDE_LOOP;
		}
		else
		{
			aFlags = BONE_ANIM_OVERRIDE_FREEZE;
		}

		aFlags |= BONE_ANIM_BLEND; //since client defaults to blend. Not sure if this will make much difference if any on client position, but it's here just for the sake of matching them.

		if (animSpeed < 0)
		{
			lastFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame;
			firstFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame + bgAllAnims[self->localAnimIndex].anims[f].numFrames;
		}
		else
		{
			firstFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame;
			lastFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame + bgAllAnims[self->localAnimIndex].anims[f].numFrames;
		}

		trap->G2API_SetBoneAnim(self->ghoul2, 0, "lower_lumbar", firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, /*firstFrame why was it this before?*/-1, 150);

		self->client->torsoAnimExecute = torsoAnim;
		self->client->torsoLastFlip = self->client->ps.torsoFlip;

		setTorso = qtrue;
	}

	if (setTorso &&
		self->localAnimIndex <= 1)
	{ //only set the motion bone for humanoids.
		trap->G2API_SetBoneAnim(self->ghoul2, 0, "Motion", firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
	}

#if 0 //disabled for now
	if (self->client->ps.brokenLimbs != self->client->brokenLimbs ||
		setTorso)
	{
		if (self->localAnimIndex <= 1 && self->client->ps.brokenLimbs &&
			(self->client->ps.brokenLimbs & (1 << BROKENLIMB_LARM)))
		{ //broken left arm
			char *brokenBone = "lhumerus";
			animation_t *armAnim;
			int armFirstFrame;
			int armLastFrame;
			int armFlags = 0;
			float armAnimSpeed;

			armAnim = &bgAllAnims[self->localAnimIndex].anims[ BOTH_DEAD21 ];
			self->client->brokenLimbs = self->client->ps.brokenLimbs;

			armFirstFrame = armAnim->firstFrame;
			armLastFrame = armAnim->firstFrame+armAnim->numFrames;
			armAnimSpeed = 50.0f / armAnim->frameLerp;
			armFlags = (BONE_ANIM_OVERRIDE_LOOP|BONE_ANIM_BLEND);

			trap->G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, level.time, -1, 150);
		}
		else if (self->localAnimIndex <= 1 && self->client->ps.brokenLimbs &&
			(self->client->ps.brokenLimbs & (1 << BROKENLIMB_RARM)))
		{ //broken right arm
			char *brokenBone = "rhumerus";
			char *supportBone = "lhumerus";

			self->client->brokenLimbs = self->client->ps.brokenLimbs;

			//Only put the arm in a broken pose if the anim is such that we
			//want to allow it.
			if ((//self->client->ps.weapon == WP_MELEE ||
				self->client->ps.weapon != WP_SABER ||
				BG_SaberStanceAnim(self->client->ps.torsoAnim) ||
				PM_RunningAnim(self->client->ps.torsoAnim)) &&
				(!self->client->saber[1].model[0] || self->client->ps.weapon != WP_SABER))
			{
				int armFirstFrame;
				int armLastFrame;
				int armFlags = 0;
				float armAnimSpeed;
				animation_t *armAnim;

				if (self->client->ps.weapon == WP_MELEE ||
					self->client->ps.weapon == WP_SABER ||
					self->client->ps.weapon == WP_MODULIZED_WEAPON)
				{ //don't affect this arm if holding a gun, just make the other arm support it
					armAnim = &bgAllAnims[self->localAnimIndex].anims[ BOTH_ATTACK2 ];

					//armFirstFrame = armAnim->firstFrame;
					armFirstFrame = armAnim->firstFrame+armAnim->numFrames;
					armLastFrame = armAnim->firstFrame+armAnim->numFrames;
					armAnimSpeed = 50.0f / armAnim->frameLerp;
					armFlags = (BONE_ANIM_OVERRIDE_LOOP|BONE_ANIM_BLEND);

					trap->G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, level.time, -1, 150);
				}
				else
				{ //we want to keep the broken bone updated for some cases
					trap->G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
				}

				if (self->client->ps.torsoAnim != BOTH_MELEE1 &&
					self->client->ps.torsoAnim != BOTH_MELEE2 &&
					(self->client->ps.torsoAnim == TORSO_WEAPONREADY2 || self->client->ps.torsoAnim == BOTH_ATTACK2 || self->client->ps.weapon < WP_MODULIZED_WEAPON))
				{
					//Now set the left arm to "support" the right one
					armAnim = &bgAllAnims[self->localAnimIndex].anims[ BOTH_STAND2 ];
					armFirstFrame = armAnim->firstFrame;
					armLastFrame = armAnim->firstFrame+armAnim->numFrames;
					armAnimSpeed = 50.0f / armAnim->frameLerp;
					armFlags = (BONE_ANIM_OVERRIDE_LOOP|BONE_ANIM_BLEND);

					trap->G2API_SetBoneAnim(self->ghoul2, 0, supportBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, level.time, -1, 150);
				}
				else
				{ //we want to keep the support bone updated for some cases
					trap->G2API_SetBoneAnim(self->ghoul2, 0, supportBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
				}
			}
			else
			{ //otherwise, keep it set to the same as the torso
				trap->G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
				trap->G2API_SetBoneAnim(self->ghoul2, 0, supportBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
			}
		}
		else if (self->client->brokenLimbs)
		{ //remove the bone now so it can be set again
			char *brokenBone = NULL;
			int broken = 0;

			//Warning: Don't remove bones that you've added as bolts unless you want to invalidate your bolt index
			//(well, in theory, I haven't actually run into the problem)
			if (self->client->brokenLimbs & (1<<BROKENLIMB_LARM))
			{
				brokenBone = "lhumerus";
				broken |= (1<<BROKENLIMB_LARM);
			}
			else if (self->client->brokenLimbs & (1<<BROKENLIMB_RARM))
			{ //can only have one arm broken at once.
				brokenBone = "rhumerus";
				broken |= (1<<BROKENLIMB_RARM);

				//want to remove the support bone too then
				trap->G2API_SetBoneAnim(self->ghoul2, 0, "lhumerus", 0, 1, 0, 0, level.time, -1, 0);
				trap->G2API_RemoveBone(self->ghoul2, "lhumerus", 0);
			}

			assert(brokenBone);

			//Set the flags and stuff to 0, so that the remove will succeed
			trap->G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, 0, 1, 0, 0, level.time, -1, 0);

			//Now remove it
			trap->G2API_RemoveBone(self->ghoul2, brokenBone, 0);
			self->client->brokenLimbs &= ~broken;
		}
	}
#endif
}

/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/

vmCvar_t	mapname;
extern qboolean WP_HasForcePowers( const playerState_t *ps );
void ClientSpawn(gentity_t *ent) {
	int					i = 0, index = 0, saveSaberNum = ENTITYNUM_NONE, wDisable = 0, savedSiegeIndex = 0, maxHealth = 100;
	vec3_t				spawn_origin, spawn_angles;
	gentity_t			*spawnPoint = NULL;
#if 0 // UQ1: Nope. Waste of bandwidth...
	gentity_t			*tent = NULL;
#endif //0
	gclient_t			*client = NULL;
	clientPersistant_t	saved;
	clientSession_t		savedSess;
	forcedata_t			savedForce;
	saberInfo_t			saberSaved[MAX_SABERS];
	int					persistant[MAX_PERSISTANT] = {0};
	int					flags, gameFlags, savedPing, accuracy_hits, accuracy_shots, eventSequence;
	void				*g2WeaponPtrs[MAX_SABERS];
	char				userinfo[MAX_INFO_STRING] = {0}, *key = NULL, *value = NULL, *saber = NULL;
	qboolean			changedSaber = qfalse, inSiegeWithClass = qfalse;

	index = ent - g_entities;
	client = ent->client;

	if (ent->padawan)
	{
		ent->padawan->parent = NULL;
	}

	ent->parent = NULL;
	ent->padawan = NULL;

	//first we want the userinfo so we can see if we should update this client's saber -rww
	trap->GetUserinfo( index, userinfo, sizeof( userinfo ) );

	Cmd_SaberAttackCycle_f(ent);

	for ( i=0; i<MAX_SABERS; i++ )
	{
		saber = (i&1) ? ent->client->pers.saber2 : ent->client->pers.saber1;
		value = Info_ValueForKey( userinfo, va( "saber%i", i+1 ) );
		if ( saber && value &&
			(Q_stricmp( value, saber ) || !saber[0] || !ent->client->saber[0].model[0]) )
		{ //doesn't match up (or our saber is BS), we want to try setting it
			if ( G_SetSaber( ent, i, value, qfalse ) )
				changedSaber = qtrue;

			//Well, we still want to say they changed then (it means this is siege and we have some overrides)
			else if ( !saber[0] || !ent->client->saber[0].model[0] )
				changedSaber = qtrue;
		}
	}

	if ( changedSaber )
	{ //make sure our new info is sent out to all the other clients, and give us a valid stance
		if ( !ClientUserinfoChanged( ent->s.number ) )
			return;

		//make sure the saber models are updated
		G_SaberModelSetup( ent );

		for ( i=0; i<MAX_SABERS; i++ )
		{
			saber = (i&1) ? ent->client->pers.saber2 : ent->client->pers.saber1;
			key = va( "saber%d", i+1 );
			value = Info_ValueForKey( userinfo, key );
			if ( Q_stricmp( value, saber ) )
			{// they don't match up, force the user info
				Info_SetValueForKey( userinfo, key, saber );
				trap->SetUserinfo( ent->s.number, userinfo );
			}
		}

		if ( ent->client->saber[0].model[0] && ent->client->saber[1].model[0] )
		{ //dual
			ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = SS_DUAL;
		}
		else if ( (ent->client->saber[0].saberFlags&SFL_TWO_HANDED) )
		{ //staff
			ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = SS_STAFF;
		}
		else
		{
			//ent->client->sess.saberLevel = Com_Clampi( SS_FAST, SS_STRONG, ent->client->sess.saberLevel );
			Cmd_SaberAttackCycle_f(ent);
			ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel;

			// limit our saber style to our force points allocated to saber offense
			//if ( level.gametype != GT_SIEGE && ent->client->ps.fd.saberAnimLevel > ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] )
			//	ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel = ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE];
		}
		if ( level.gametype != GT_SIEGE )
		{// let's just make sure the styles we chose are cool
			if ( !WP_SaberStyleValidForSaber( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, ent->client->ps.fd.saberAnimLevel ) )
			{
				WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &ent->client->ps.fd.saberAnimLevel );
				ent->client->ps.fd.saberAnimLevelBase = ent->client->saberCycleQueue = ent->client->ps.fd.saberAnimLevel;
			}
		}

		G_CheckSaberStanceValidity(ent);
	}

	if (client->ps.fd.forceDoInit)
	{ //force a reread of force powers
		WP_InitForcePowers( ent );
		client->ps.fd.forceDoInit = 0;
	}

	if (ent->client->ps.fd.saberAnimLevel != SS_STAFF && ent->client->ps.fd.saberAnimLevel != SS_DUAL &&
		ent->client->ps.fd.saberAnimLevel == ent->client->ps.fd.saberDrawAnimLevel &&
		ent->client->ps.fd.saberAnimLevel == ent->client->sess.saberLevel)
	{
		//ent->client->sess.saberLevel = Com_Clampi( SS_FAST, SS_STRONG, ent->client->sess.saberLevel );
		Cmd_SaberAttackCycle_f(ent);
		ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel;

		// limit our saber style to our force points allocated to saber offense
		//if ( level.gametype != GT_SIEGE && ent->client->ps.fd.saberAnimLevel > ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] )
		//	ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel = ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE];
	}

	trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
	
	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if ( client->sess.sessionTeam == FACTION_SPECTATOR ) {
			spawnPoint = SelectSpectatorSpawnPoint (spawn_origin, spawn_angles);
	} else if (level.gametype == GT_CTF || level.gametype == GT_CTY) {
		// all base oriented team games use the CTF spawn points
		spawnPoint = SelectCTFSpawnPoint (
						client->sess.sessionTeam,
						client->pers.teamState.state,
						spawn_origin, spawn_angles, (qboolean)!(ent->r.svFlags & SVF_BOT));
	} else if (level.gametype < GT_TEAM 
		&& (!Q_stricmpn(mapname.string, "ctf", 3) || !Q_stricmpn(mapname.string, "mp/ctf", 6) || !Q_stricmpn(mapname.string, "mp\\ctf", 6))) {
		// UQ1: this is a CTF map. always use CTF spawn points... - FFA compatibility!
		spawnPoint = SelectCTFSpawnPoint (
						(team_t)irand(1, 2),
						client->pers.teamState.state,
						spawn_origin, spawn_angles, (qboolean)!(ent->r.svFlags & SVF_BOT));
	}
	else if (level.gametype == GT_SIEGE)
	{
		spawnPoint = SelectSiegeSpawnPoint (
						client->siegeClass,
						client->sess.sessionTeam,
						client->pers.teamState.state,
						spawn_origin, spawn_angles, (qboolean)!(ent->r.svFlags & SVF_BOT));
	}
	else if (level.gametype >= GT_TEAM) {
		// all base oriented team games use the CTF spawn points
		spawnPoint = SelectCTFSpawnPoint (
						client->sess.sessionTeam,
						client->pers.teamState.state,
						spawn_origin, spawn_angles, (qboolean)!(ent->r.svFlags & SVF_BOT));
	}
	else {
		if (level.gametype == GT_POWERDUEL)
		{
			spawnPoint = SelectDuelSpawnPoint(client->sess.duelTeam, client->ps.origin, spawn_origin, spawn_angles, (qboolean)(ent->r.svFlags & SVF_BOT));
		}
		else if (level.gametype == GT_DUEL)
		{	// duel
			spawnPoint = SelectDuelSpawnPoint(DUELTEAM_SINGLE, client->ps.origin, spawn_origin, spawn_angles, (qboolean)(ent->r.svFlags & SVF_BOT));
		}
		else
		{
			// the first spawn should be at a good looking spot
			if ( !client->pers.initialSpawn && client->pers.localClient ) {
				client->pers.initialSpawn = qtrue;
				spawnPoint = SelectInitialSpawnPoint( spawn_origin, spawn_angles, client->sess.sessionTeam, (qboolean)(ent->r.svFlags & SVF_BOT) );
			} else {
				// don't spawn near existing origin if possible
				spawnPoint = SelectSpawnPoint (
					client->ps.origin,
					spawn_origin, spawn_angles, client->sess.sessionTeam, (qboolean)(ent->r.svFlags & SVF_BOT) );
			}
		}
	}
	client->pers.teamState.state = TEAM_ACTIVE;

	// toggle the teleport bit so the client knows to not lerp
	// and never clear the voted flag
	flags = ent->client->ps.eFlags & (EF_TELEPORT_BIT);
	flags ^= EF_TELEPORT_BIT;
	gameFlags = ent->client->mGameFlags & ( PSG_VOTED | PSG_TEAMVOTED);

	// clear everything but the persistant data

	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
//	savedAreaBits = client->areabits;
	accuracy_hits = client->accuracy_hits;
	accuracy_shots = client->accuracy_shots;
	for ( i=0; i<MAX_PERSISTANT; i++ )
		persistant[i] = client->ps.persistant[i];

	eventSequence = client->ps.eventSequence;

	savedForce = client->ps.fd;

	saveSaberNum = client->ps.saberEntityNum;

	savedSiegeIndex = client->siegeClass;

	for ( i=0; i<MAX_SABERS; i++ )
	{
		saberSaved[i] = client->saber[i];
		g2WeaponPtrs[i] = client->weaponGhoul2[i];
	}

	for ( i=0; i<HL_MAX; i++ )
		ent->locationDamage[i] = 0;


	//
	// Inventory Data...
	//
	extern int BG_CountInventoryItems(playerState_t *ps);
	
	qboolean		inventorySaved = qfalse;

	int				savedInventoryBlank[16] = { { -1 } };
	uint16_t		savedInventoryItems[64] = { { 0 } }; // Need to keep values < 65536 for transmission
	uint16_t		savedInventoryMod1[64] = { { 0 } }; // Need to keep values < 65536 for transmission
	uint16_t		savedInventoryMod2[64] = { { 0 } }; // Need to keep values < 65536 for transmission
	uint16_t		savedInventoryMod3[64] = { { 0 } }; // Need to keep values < 65536 for transmission
	int				savedInventoryEquipped[16] = { { -1 } }; // Inventory Slot ID of the item...

	memset(&savedInventoryBlank, 0, sizeof(savedInventoryBlank));
	memset(&savedInventoryItems, 0, sizeof(savedInventoryItems));
	memset(&savedInventoryMod1, 0, sizeof(savedInventoryMod1));
	memset(&savedInventoryMod2, 0, sizeof(savedInventoryMod2));
	memset(&savedInventoryMod3, 0, sizeof(savedInventoryMod3));
	memset(&savedInventoryEquipped, 0, sizeof(savedInventoryEquipped));

	if (BG_CountInventoryItems(&client->ps) > 0)
	{
		inventorySaved = qtrue;

		memcpy(&savedInventoryBlank, client->ps.inventoryBlank, sizeof(client->ps.inventoryBlank));
		memcpy(&savedInventoryItems, client->ps.inventoryItems, sizeof(client->ps.inventoryItems));
		memcpy(&savedInventoryMod1, client->ps.inventoryMod1, sizeof(client->ps.inventoryMod1));
		memcpy(&savedInventoryMod2, client->ps.inventoryMod2, sizeof(client->ps.inventoryMod2));
		memcpy(&savedInventoryMod3, client->ps.inventoryMod3, sizeof(client->ps.inventoryMod3));
		memcpy(&savedInventoryEquipped, client->ps.inventoryEquipped, sizeof(client->ps.inventoryEquipped));
	}

	//
	//
	//

	memset( client, 0, sizeof( *client ) ); // bk FIXME: Com_Memset?
	client->bodyGrabIndex = ENTITYNUM_NONE;

	//Get the skin RGB based on his userinfo
	client->ps.customRGBA[0] = (value=Info_ValueForKey( userinfo, "char_color_red" ))	? Com_Clampi( 0, 255, atoi( value ) ) : 255;
	client->ps.customRGBA[1] = (value=Info_ValueForKey( userinfo, "char_color_green" ))	? Com_Clampi( 0, 255, atoi( value ) ) : 255;
	client->ps.customRGBA[2] = (value=Info_ValueForKey( userinfo, "char_color_blue" ))	? Com_Clampi( 0, 255, atoi( value ) ) : 255;

	//Prevent skins being too dark
	if ( g_charRestrictRGB.integer && ((client->ps.customRGBA[0]+client->ps.customRGBA[1]+client->ps.customRGBA[2]) < 100) )
		client->ps.customRGBA[0] = client->ps.customRGBA[1] = client->ps.customRGBA[2] = 255;

	client->ps.customRGBA[3]=255;

#if 0 // UQ1: Don't need colored skins, etc... We have name tags...
	if ( level.gametype >= GT_TEAM && level.gametype != GT_SIEGE && !g_jediVmerc.integer )
	{
		char skin[MAX_QPATH] = {0}, model[MAX_QPATH] = {0};
		vec3_t colorOverride = {0.0f};

		VectorClear( colorOverride );
		Q_strncpyz( model, Info_ValueForKey( userinfo, "model" ), sizeof( model ) );

		BG_ValidateSkinForTeam( model, skin, savedSess.sessionTeam, colorOverride );
		if ( colorOverride[0] != 0.0f || colorOverride[1] != 0.0f || colorOverride[2] != 0.0f )
			VectorScaleM( colorOverride, 255.0f, client->ps.customRGBA );
	}
#endif //0

	client->siegeClass = savedSiegeIndex;

	for ( i=0; i<MAX_SABERS; i++ )
	{
		client->saber[i] = saberSaved[i];
		client->weaponGhoul2[i] = g2WeaponPtrs[i];
	}

	//or the saber ent num
	client->ps.saberEntityNum = saveSaberNum;
	client->saberStoredIndex = saveSaberNum;

	client->ps.fd = savedForce;

	client->ps.duelIndex = ENTITYNUM_NONE;

	//spawn with 100
	client->ps.jetpackFuel = 100;
	client->ps.cloakFuel = 100;
	client->manualblockdeflect = 0;
	client->manualblockdeflectCD = 0;

	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;
//	client->areabits = savedAreaBits;
	client->accuracy_hits = accuracy_hits;
	client->accuracy_shots = accuracy_shots;
	client->lastkilled_client = -1;

	for ( i=0; i<MAX_PERSISTANT; i++ )
		client->ps.persistant[i] = persistant[i];

	client->ps.eventSequence = eventSequence;
	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;

	client->airOutTime = level.time + 12000;


	//
	// Inventory Data...
	//
	if (inventorySaved)
	{
		memcpy(&client->ps.inventoryBlank, &savedInventoryBlank, sizeof(client->ps.inventoryBlank));
		memcpy(&client->ps.inventoryItems, &savedInventoryItems, sizeof(client->ps.inventoryItems));
		memcpy(&client->ps.inventoryMod1, &savedInventoryMod1, sizeof(client->ps.inventoryMod1));
		memcpy(&client->ps.inventoryMod2, &savedInventoryMod2, sizeof(client->ps.inventoryMod2));
		memcpy(&client->ps.inventoryMod3, &savedInventoryMod3, sizeof(client->ps.inventoryMod3));
		memcpy(&client->ps.inventoryEquipped, &savedInventoryEquipped, sizeof(client->ps.inventoryEquipped));
	}

#if 0
	// set max health
	if (/*level.gametype == GT_SIEGE &&*/ client->siegeClass != -1)
	{
		siegeClass_t *scl = &bgSiegeClasses[client->siegeClass];
		maxHealth = 100;

		if (scl->maxhealth)
		{
			maxHealth = scl->maxhealth;
		}
	}
	else
	{
		maxHealth = Com_Clampi( 1, 100, atoi( Info_ValueForKey( userinfo, "handicap" ) ) );
	}
#else
	maxHealth = 1000;
#endif
	client->pers.maxHealth = maxHealth;//atoi( Info_ValueForKey( userinfo, "handicap" ) );
	if ( client->pers.maxHealth < 1 || client->pers.maxHealth > maxHealth ) {
#if 0
		client->pers.maxHealth = 100;
#else
		client->pers.maxHealth = 1000;
#endif
	}
	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags = flags;
	client->mGameFlags = gameFlags;

	client->ps.groundEntityNum = ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->playerState = &ent->client->ps;
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;

	VectorCopy (playerMins, ent->r.mins);
	VectorCopy (playerMaxs, ent->r.maxs);
	client->ps.crouchheight = CROUCH_MAXS_2;
	client->ps.standheight = DEFAULT_MAXS_2;

	client->ps.clientNum = index;

	if ((ent->client->saber[0].saberFlags&SFL_TWO_HANDED))
	{// Dualblade.
		if ( !WP_SaberStyleValidForSaber( &client->saber[0], &client->saber[1], client->ps.saberHolstered, client->ps.fd.saberAnimLevel ) )
		{//only use dual style if the style we're trying to use isn't valid
			client->ps.fd.saberAnimLevelBase = client->ps.fd.saberAnimLevel = SS_CROWD_CONTROL;//SS_STAFF;
		}
		client->ps.fd.saberAnimLevelBase = client->ps.fd.saberAnimLevel = SS_CROWD_CONTROL;// SS_STAFF;

		G_CheckSaberStanceValidity(ent);
	}

	if (ent->client->saber[0].model[0] &&
		ent->client->saber[1].model[0])
	{ //dual saber
		client->ps.saberHolstered = 0;
		client->ps.fd.saberAnimLevelBase = client->ps.fd.saberAnimLevel = client->ps.fd.saberDrawAnimLevel = SS_DUAL;

		G_CheckSaberStanceValidity(ent);
	}

	//give default weapons
	client->ps.primaryWeapon = WP_NONE;
	client->ps.secondaryWeapon = WP_NONE;
	client->ps.temporaryWeapon = WP_NONE;

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		wDisable = g_duelWeaponDisable.integer;
	}
	else
	{
		wDisable = g_weaponDisable.integer;
	}

	if ( level.gametype != GT_HOLOCRON
		&& level.gametype != GT_JEDIMASTER
		&& !HasSetSaberOnly()
		&& !AllForceDisabled( g_forcePowerDisable.integer )
		&& g_jediVmerc.integer )
	{
		if ( level.gametype >= GT_TEAM && (client->sess.sessionTeam == FACTION_REBEL || client->sess.sessionTeam == FACTION_EMPIRE) )
		{//In Team games, force one side to be merc and other to be jedi
			if ( level.numPlayingClients > 0 )
			{//already someone in the game
				int forceTeam = FACTION_SPECTATOR;
				for ( i = 0 ; i < level.maxclients ; i++ )
				{
					if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
						continue;
					}
					if ( level.clients[i].sess.sessionTeam == FACTION_REBEL || level.clients[i].sess.sessionTeam == FACTION_EMPIRE )
					{//in-game
						if ( WP_HasForcePowers( &level.clients[i].ps ) )
						{//this side is using force
							forceTeam = level.clients[i].sess.sessionTeam;
						}
						else
						{//other team is using force
							if ( level.clients[i].sess.sessionTeam == FACTION_REBEL )
							{
								forceTeam = FACTION_EMPIRE;
							}
							else
							{
								forceTeam = FACTION_REBEL;
							}
						}
						break;
					}
				}
				if ( WP_HasForcePowers( &client->ps ) && client->sess.sessionTeam != forceTeam )
				{//using force but not on right team, switch him over
					const char *teamName = TeamName( forceTeam );
					//client->sess.sessionTeam = forceTeam;
					SetTeam( ent, (char *)teamName );
					return;
				}
			}
		}

		if ( WP_HasForcePowers( &client->ps ) )
		{
			client->ps.trueNonJedi = qfalse;
			client->ps.trueJedi = qtrue;
			//make sure they only use the saber
			client->ps.weapon = WP_SABER;
			client->ps.primaryWeapon = WP_SABER;
			client->ps.secondaryWeapon = WP_MODULIZED_WEAPON;
			BG_CreatePlayerDefaultJediInventory(ent, &client->ps, client->sess.sessionTeam);
		}
		else
		{//no force powers set
			client->ps.trueNonJedi = qtrue;
			client->ps.trueJedi = qfalse;
			client->ps.primaryWeapon = WP_MELEE;
			client->ps.secondaryWeapon = WP_MODULIZED_WEAPON;
			BG_CreatePlayerDefaultGunnerInventory(&client->ps, client->sess.sessionTeam);
		}
	}
	else
	{//jediVmerc is incompatible with this gametype, turn it off!
		trap->Cvar_Set( "g_jediVmerc", "0" );
		if (level.gametype == GT_HOLOCRON)
		{
			//always get free saber level 1 in holocron
			client->ps.primaryWeapon = WP_SABER;
			BG_CreatePlayerDefaultJediInventory(ent, &client->ps, client->sess.sessionTeam);
		}
		else
		{
			if (client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE])
			{
				client->ps.primaryWeapon = WP_SABER;
				BG_CreatePlayerDefaultJediInventory(ent, &client->ps, client->sess.sessionTeam);
			}
			else
			{ //if you don't have saber attack rank then you don't get a saber
				client->ps.primaryWeapon = WP_MELEE;
			}
		}

		if (level.gametype != GT_SIEGE)
		{
			client->ps.secondaryWeapon = WP_MODULIZED_WEAPON;
			BG_CreatePlayerDefaultGunnerInventory(&client->ps, client->sess.sessionTeam);
		}

		if (level.gametype == GT_JEDIMASTER)
		{
			client->ps.primaryWeapon = WP_MELEE;
			BG_CreatePlayerDefaultGunnerInventory(&client->ps, client->sess.sessionTeam);
		}

		if (HaveWeapon(&client->ps, WP_SABER))
		{
			client->ps.weapon = WP_SABER;
			BG_CreatePlayerDefaultJediInventory(ent, &client->ps, client->sess.sessionTeam);
		}
		else if (HaveWeapon(&client->ps, WP_MODULIZED_WEAPON))
		{
			client->ps.weapon = WP_MODULIZED_WEAPON;
			BG_CreatePlayerDefaultGunnerInventory(&client->ps, client->sess.sessionTeam);
		}
		else
		{
			client->ps.weapon = WP_MELEE;
			BG_CreatePlayerDefaultGunnerInventory(&client->ps, client->sess.sessionTeam);
		}
	}
	//[SaberSys] Melee To FFA
#if 0 // UQ1: Let's not override this :)
	if (level.gametype == GT_FFA)
	{
		client->ps.primaryWeapon = WP_MELEE;
	}
#endif //0

	if (HaveWeapon(&client->ps, WP_SABER))
	{
		client->ps.weapon = WP_SABER;
		BG_CreatePlayerDefaultJediInventory(ent, &client->ps, client->sess.sessionTeam);
	}
	else if (HaveWeapon(&client->ps, WP_MELEE))
	{
		client->ps.weapon = WP_MELEE;
	}
	//[/SaberSys] Melee To FFA
	/*
	client->ps.stats[STAT_HOLDABLE_ITEMS] |= ( 1 << HI_BINOCULARS );
	client->ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_BINOCULARS, IT_HOLDABLE);
	*/

	if (/*level.gametype == GT_SIEGE &&*/ client->siegeClass != -1 &&
		client->sess.sessionTeam != FACTION_SPECTATOR)
	{ //well then, we will use a custom weaponset for our class
		int m = 0;

		// UQ1: TODO: Fix class weapons... --- bgSiegeClasses[client->siegeClass].weapons;
		client->ps.primaryWeapon = WP_SABER; // temporary
		client->ps.secondaryWeapon = WP_MODULIZED_WEAPON; // temporary

		if (HaveWeapon(&client->ps, WP_SABER))
		{
			client->ps.weapon = WP_SABER;
			BG_CreatePlayerDefaultJediInventory(ent, &client->ps, client->sess.sessionTeam);
		}
		else if (HaveWeapon(&client->ps, WP_MODULIZED_WEAPON))
		{
			client->ps.weapon = WP_MODULIZED_WEAPON;
			BG_CreatePlayerDefaultGunnerInventory(&client->ps, client->sess.sessionTeam);
		}
		else
		{
			client->ps.weapon = WP_MELEE;
		}

		inSiegeWithClass = qtrue;

		while (m < WP_NUM_WEAPONS)
		{
			if (HaveWeapon(&client->ps, m))
			{
				if (client->ps.weapon != WP_SABER)
				{ //try to find the highest ranking weapon we have
					if (m > client->ps.weapon)
					{
						client->ps.weapon = m;
					}
				}
			}
			m++;
		}
	}

	if (/*level.gametype == GT_SIEGE &&*/
		client->siegeClass != -1 &&
		client->sess.sessionTeam != FACTION_SPECTATOR)
	{ //use class-specified inventory
		client->ps.stats[STAT_HOLDABLE_ITEMS] = bgSiegeClasses[client->siegeClass].invenItems;
		client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
	}
	else
	{
		client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
		client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
	}

	if (/*level.gametype == GT_SIEGE &&*/
		client->siegeClass != -1 &&
		bgSiegeClasses[client->siegeClass].powerups &&
		client->sess.sessionTeam != FACTION_SPECTATOR)
	{ //this class has some start powerups
		i = 0;
		while (i < PW_NUM_POWERUPS)
		{
			if (bgSiegeClasses[client->siegeClass].powerups & (1 << i))
			{
				client->ps.powerups[i] = Q3_INFINITE;
			}
			i++;
		}
	}

	if ( client->sess.sessionTeam == FACTION_SPECTATOR )
	{
		client->ps.primaryWeapon = 0;
		client->ps.secondaryWeapon = 0;
		client->ps.temporaryWeapon = 0;
		client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
		client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
	}

// nmckenzie: DESERT_SIEGE... or well, siege generally.  This was over-writing the max value, which was NOT good for siege.

	client->ps.rocketLockIndex = ENTITYNUM_NONE;
	client->ps.rocketLockTime = 0;

	//rww - Set here to initialize the circling seeker drone to off.
	//A quick note about this so I don't forget how it works again:
	//ps.genericEnemyIndex is kept in sync between the server and client.
	//When it gets set then an entitystate value of the same name gets
	//set along with an entitystate flag in the shared bg code. Which
	//is why a value needs to be both on the player state and entity state.
	//(it doesn't seem to just carry over the entitystate value automatically
	//because entity state value is derived from player state data or some
	//such)
	client->ps.genericEnemyIndex = -1;

	client->ps.isJediMaster = qfalse;

	if (client->ps.fallingToDeath)
	{
		client->ps.fallingToDeath = 0;
		client->noCorpse = qtrue;
	}

	//Do per-spawn force power initialization
	WP_SpawnInitForcePowers( ent );

	// health will count down towards max_health
	if (/*level.gametype == GT_SIEGE &&*/
		client->siegeClass != -1 &&
		bgSiegeClasses[client->siegeClass].starthealth)
	{ //class specifies a start health, so use it
		ent->health = client->ps.stats[STAT_HEALTH] = bgSiegeClasses[client->siegeClass].starthealth;
	}
	else if ( level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL )
	{//only start with 100 health in Duel
		if ( level.gametype == GT_POWERDUEL && client->sess.duelTeam == DUELTEAM_LONE )
		{
			if ( duel_fraglimit.integer )
			{

				ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] =
					g_powerDuelStartHealth.integer - ((g_powerDuelStartHealth.integer - g_powerDuelEndHealth.integer) * (float)client->sess.wins / (float)duel_fraglimit.integer);
			}
			else
			{
				ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] = 150;
			}
		}
		else
		{
			ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] = 100;
		}
	}
	else if (client->ps.stats[STAT_MAX_HEALTH] <= 100)
	{
		ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] * 1.25;
	}
	else if (client->ps.stats[STAT_MAX_HEALTH] < 125)
	{
		ent->health = client->ps.stats[STAT_HEALTH] = 125;
	}
	else
	{
		ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH];
	}

	// Start with a small amount of armor as well.
	if (/*level.gametype == GT_SIEGE &&*/
		client->siegeClass != -1 /*&&
		bgSiegeClasses[client->siegeClass].startarmor*/)
	{ //class specifies a start armor amount, so use it
		client->ps.stats[STAT_ARMOR] = bgSiegeClasses[client->siegeClass].startarmor;
	}
	else if ( level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL )
	{//no armor in duel
		client->ps.stats[STAT_ARMOR] = 0;
	}
	else
	{
		client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_HEALTH] * 0.25;
	}


	G_SetOrigin( ent, spawn_origin );
	VectorCopy( spawn_origin, client->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	trap->GetUsercmd( client - level.clients, &ent->client->pers.cmd );
	SetClientViewAngle( ent, spawn_angles );
	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->latched_buttons = 0;

	if (!level.intermissiontime) {
		if (ent->client->sess.sessionTeam != FACTION_SPECTATOR) {
			G_KillBox(ent);
			// force the base weapon up
			if (client->ps.weapon <= WP_NONE)
			{
				client->ps.weapon = WP_MODULIZED_WEAPON;
			}

			client->ps.torsoTimer = client->ps.legsTimer = 0;

			if (client->ps.weapon == WP_SABER)
			{
				G_SetAnim(ent, NULL, SETANIM_BOTH, BOTH_STAND1TO2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS, 0);
			}
			else
			{
				G_SetAnim(ent, NULL, SETANIM_TORSO, TORSO_RAISEWEAP1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS, 0);
				client->ps.legsAnim = WeaponReadyAnim[client->ps.weapon];
			}
			client->ps.weaponstate = WEAPON_RAISING;
			client->ps.weaponTime = client->ps.torsoTimer;

			if (g_spawnInvulnerability.integer)
			{
				ent->client->ps.eFlags |= EF_INVULNERABLE;
				ent->client->invulnerableTimer = level.time + g_spawnInvulnerability.integer;
			}

			// fire the targets of the spawn point
			G_UseTargets(spawnPoint, ent);

			// positively link the client, even if the command times are weird
			VectorCopy(ent->client->ps.origin, ent->r.currentOrigin);

#if 0 // UQ1: Nope. Waste of bandwidth...
			tent = G_TempEntity(ent->client->ps.origin, EV_PLAYER_TELEPORT_IN);
			tent->s.clientNum = ent->s.clientNum;
#endif

			trap->LinkEntity ((sharedEntity_t *)ent);
		}
	} else {
		// move players to intermission
		MoveClientToIntermission(ent);
	}

	//set teams for NPCs to recognize
	if (/*level.gametype == GT_SIEGE*/qtrue)
	{ //Imperial (team1) team is allied with "enemy" NPCs in this mode
		if (client->sess.sessionTeam == SIEGETEAM_TEAM1)
		{
			client->playerTeam = NPCTEAM_ENEMY;
			ent->s.teamowner = NPCTEAM_ENEMY;
			client->enemyTeam = NPCTEAM_PLAYER;
		}
		else
		{
			client->playerTeam = NPCTEAM_PLAYER;
			ent->s.teamowner = NPCTEAM_PLAYER;
			client->enemyTeam = NPCTEAM_ENEMY;
		}
	}
	else
	{
		client->playerTeam = NPCTEAM_PLAYER;
		ent->s.teamowner = NPCTEAM_PLAYER;
		client->enemyTeam = NPCTEAM_ENEMY;
	}

	/*
	//scaling for the power duel opponent
	if (level.gametype == GT_POWERDUEL &&
		client->sess.duelTeam == DUELTEAM_LONE)
	{
		client->ps.iModelScale = 125;
		VectorSet(ent->modelScale, 1.25f, 1.25f, 1.25f);
	}
	*/
	//Disabled. At least for now. Not sure if I'll want to do it or not eventually.

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink( ent-g_entities, NULL );

	// run the presend to set anything else, follow spectators wait
	// until all clients have been reconnected after map_restart
	if ( ent->client->sess.spectatorState != SPECTATOR_FOLLOW )
		ClientEndFrame( ent );

	// clear entity state values
	BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );

#ifndef __NO_ICARUS__
	//rww - make sure client has a valid icarus instance
	trap->ICARUS_FreeEnt( (sharedEntity_t *)ent );
	trap->ICARUS_InitEnt( (sharedEntity_t *)ent );
#endif //__NO_ICARUS__

	extern vmCvar_t npc_followers;

	if (npc_followers.integer && (g_gametype.integer == GT_WARZONE || g_gametype.integer == GT_INSTANCE) && ent->s.primaryWeapon == WP_SABER)
	{
		if (ent->client->sess.sessionTeam == FACTION_REBEL)
		{// Spawn a padawan for this jedi player...
			if (NPC_NeedPadawan_Spawn(ent))
			{// Only if we do not already have a padawan...
				gentity_t *padawan = G_Spawn();

#ifdef __USE_NAVLIB_SPAWNPOINTS__
				if (G_NavmeshIsLoaded())
				{
					vec3_t position;
#pragma omp critical
					{
						FindRandomNavmeshPointInRadius(-1, ent->r.currentOrigin, position, 1024.0/*2048.0*/);
					}

					position[2] += 32.0;

					//trap->Print("padawan spawnpoint %f %f %f\n", position[0], position[1], position[2]);

					padawan->NPC_type = "padawan";

					trap->Print("Spawning \"%s\" for player %s.\n", padawan->NPC_type, ent->client->pers.netname);

					padawan->s.teamowner = ent->client->sess.sessionTeam;
					padawan->padawanSaberType = irand(1, 12);
					VectorCopy(position, padawan->s.origin);
					SP_NPC_spawner2(padawan);
				}
#else //!__USE_NAVLIB_SPAWNPOINTS__
				int waypoint = DOM_GetNearWP(ent->r.currentOrigin, ent->wpCurrent);

				if (waypoint >= 0 && waypoint < gWPNum)
				{
					padawan->NPC_type = "padawan";

					trap->Print("Spawning \"%s\" for player %s.\n", padawan->NPC_type, ent->client->pers.netname);

					padawan->s.teamowner = ent->client->sess.sessionTeam;
					padawan->padawanSaberType = irand(1, 12);
					VectorCopy(gWPArray[waypoint]->origin, padawan->s.origin);
					SP_NPC_spawner2(padawan);
				}
#endif //__USE_NAVLIB_SPAWNPOINTS__
			}
		}
		else if (ent->client->sess.sessionTeam == FACTION_EMPIRE)
		{// Spawn a padawan for this jedi player...
			if (NPC_NeedFollower_Spawn(ent))
			{// Only if we do not already have a padawan...
				gentity_t *padawan = G_Spawn();

#ifdef __USE_NAVLIB_SPAWNPOINTS__
				if (G_NavmeshIsLoaded())
				{
					vec3_t position;
#pragma omp critical
					{
						FindRandomNavmeshPointInRadius(-1, ent->r.currentOrigin, position, 1024.0/*2048.0*/);
					}

					position[2] += 32.0;

					//trap->Print("hk51 spawnpoint %f %f %f\n", position[0], position[1], position[2]);

					char name[64] = "hk51";
					padawan->NPC_type = Q_strlwr(G_NewString("hk51"));

					trap->Print("Spawning \"%s\" for player %s.\n", padawan->NPC_type, ent->client->pers.netname);

					padawan->s.teamowner = ent->client->sess.sessionTeam;
					VectorCopy(position, padawan->s.origin);
					SP_NPC_spawner2(padawan);
				}
#else //!__USE_NAVLIB_SPAWNPOINTS__
				int waypoint = DOM_GetNearWP(ent->r.currentOrigin, ent->wpCurrent);

				if (waypoint >= 0 && waypoint < gWPNum)
				{
					char name[64] = "hk51";
					padawan->NPC_type = Q_strlwr(G_NewString("hk51"));

					trap->Print("Spawning \"%s\" for player %s.\n", padawan->NPC_type, ent->client->pers.netname);

					padawan->s.teamowner = ent->client->sess.sessionTeam;
					VectorCopy(gWPArray[waypoint]->origin, padawan->s.origin);
					SP_NPC_spawner2(padawan);
				}
#endif //__USE_NAVLIB_SPAWNPOINTS__
			}
		}
	}

	ent->health = client->ps.stats[STAT_HEALTH];
	ent->maxHealth = client->ps.stats[STAT_MAX_HEALTH];
}


/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap->DropClient(), which will call this and do
server system housekeeping.
============
*/
extern void G_LeaveVehicle( gentity_t* ent, qboolean ConCheck );

void G_ClearVote( gentity_t *ent ) {
	if ( level.voteTime ) {
		if ( ent->client->mGameFlags & PSG_VOTED ) {
			if ( ent->client->pers.vote == 1 ) {
				level.voteYes--;
				trap->SetConfigstring( CS_VOTE_YES, va( "%i", level.voteYes ) );
			}
			else if ( ent->client->pers.vote == 2 ) {
				level.voteNo--;
				trap->SetConfigstring( CS_VOTE_NO, va( "%i", level.voteNo ) );
			}
		}
		ent->client->mGameFlags &= ~(PSG_VOTED);
		ent->client->pers.vote = 0;
	}
}
void G_ClearTeamVote( gentity_t *ent, int team ) {
	int voteteam;

		 if ( team == FACTION_EMPIRE )	voteteam = 0;
	else if ( team == FACTION_REBEL )	voteteam = 1;
	else							return;

	if ( level.teamVoteTime[voteteam] ) {
		if ( ent->client->mGameFlags & PSG_TEAMVOTED ) {
			if ( ent->client->pers.teamvote == 1 ) {
				level.teamVoteYes[voteteam]--;
				trap->SetConfigstring( CS_TEAMVOTE_YES, va( "%i", level.teamVoteYes[voteteam] ) );
			}
			else if ( ent->client->pers.teamvote == 2 ) {
				level.teamVoteNo[voteteam]--;
				trap->SetConfigstring( CS_TEAMVOTE_NO, va( "%i", level.teamVoteNo[voteteam] ) );
			}
		}
		ent->client->mGameFlags &= ~(PSG_TEAMVOTED);
		ent->client->pers.teamvote = 0;
	}
}

void ClientDisconnect( int clientNum ) {
	gentity_t	*ent;
	gentity_t	*tent;
	int			i;
	
	// cleanup if we are kicking a bot that
	// hasn't spawned yet
	G_RemoveQueuedBotBegin( clientNum );

	ent = g_entities + clientNum;

	ent->s.eFlags |= EF_DEAD;
	if (ent->client) 
	{
		ent->client->ps.pm_type = PM_DEAD;
		ent->client->ps.persistant[PERS_TEAM] = FACTION_SPECTATOR;
	}

	if ( !ent->client || ent->client->pers.connected == CON_DISCONNECTED ) {
		return;
	}

	{// Free any NPC data they have...
		ent->s.NPC_NAME_ID = 0; // Init the type...
	}

	i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		if (ent->client->ps.fd.forcePowersActive & (1 << i))
		{
			WP_ForcePowerStop(ent, (forcePowers_t)i);
		}
		i++;
	}

	i = TRACK_CHANNEL_1;

	while (i < NUM_TRACK_CHANNELS)
	{
		if (ent->client->ps.fd.killSoundEntIndex[i-50] && ent->client->ps.fd.killSoundEntIndex[i-50] < MAX_GENTITIES && ent->client->ps.fd.killSoundEntIndex[i-50] > 0)
		{
			G_MuteSound(ent->client->ps.fd.killSoundEntIndex[i-50], CHAN_VOICE);
		}
		i++;
	}
	i = 0;

	G_LeaveVehicle( ent, qtrue );

	// stop any following clients
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].sess.sessionTeam == FACTION_SPECTATOR
			&& level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW
			&& level.clients[i].sess.spectatorClient == clientNum ) {
			StopFollowing( &g_entities[i] );
		}
	}

	// send effect if they were completely connected
	if ( ent->client->pers.connected == CON_CONNECTED
		&& ent->client->sess.sessionTeam != FACTION_SPECTATOR ) {
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = ent->s.clientNum;

		// They don't get to take powerups with them!
		// Especially important for stuff like CTF flags
		TossClientItems( ent );
	}

	G_LogPrintf( "ClientDisconnect: %i [%s] (%s) \"%s^7\"\n", clientNum, ent->client->sess.IP, ent->client->pers.guid, ent->client->pers.netname );

	// if we are playing in tourney mode, give a win to the other player and clear his frags for this round
	if ( level.gametype == GT_DUEL && !level.intermissiontime && !level.warmupTime ) {
		if ( level.sortedClients[1] == clientNum ) {
			level.clients[ level.sortedClients[0] ].ps.persistant[PERS_SCORE] = 0;
			level.clients[ level.sortedClients[0] ].sess.wins++;
			ClientUserinfoChanged( level.sortedClients[0] );
		}
		else if ( level.sortedClients[0] == clientNum ) {
			level.clients[ level.sortedClients[1] ].ps.persistant[PERS_SCORE] = 0;
			level.clients[ level.sortedClients[1] ].sess.wins++;
			ClientUserinfoChanged( level.sortedClients[1] );
		}
	}

	if ( level.gametype == GT_DUEL && ent->client->sess.sessionTeam == FACTION_FREE && level.intermissiontime ) {
		trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
		level.restarted = qtrue;
		level.changemap = NULL;
		level.intermissiontime = 0;
	}

	if (ent->ghoul2 && trap->G2API_HaveWeGhoul2Models(ent->ghoul2))
	{
		trap->G2API_CleanGhoul2Models(&ent->ghoul2);
	}
	i = 0;
	while (i < MAX_SABERS)
	{
		if (ent->client->weaponGhoul2[i] && trap->G2API_HaveWeGhoul2Models(ent->client->weaponGhoul2[i]))
		{
			trap->G2API_CleanGhoul2Models(&ent->client->weaponGhoul2[i]);
		}
		i++;
	}

	G_ClearVote( ent );
	G_ClearTeamVote( ent, ent->client->sess.sessionTeam );

	trap->UnlinkEntity ((sharedEntity_t *)ent);
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = FACTION_FREE;
	ent->client->sess.sessionTeam = FACTION_FREE;
	ent->r.contents = 0;

	if (ent->client->holdingObjectiveItem > 0)
	{ //carrying a siege objective item - make sure it updates and removes itself from us now in case this is an instant death-respawn situation
		gentity_t *objectiveItem = &g_entities[ent->client->holdingObjectiveItem];

		if (objectiveItem->inuse && objectiveItem->think)
		{
            objectiveItem->think(objectiveItem);
		}
	}

	trap->SetConfigstring( CS_PLAYERS + clientNum, "");

	CalculateRanks();

	if ( ent->r.svFlags & SVF_BOT ) {
		BotAIShutdownClient( clientNum, qfalse );
	}
}


