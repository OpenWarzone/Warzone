// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "w_saber.h"
#include "qcommon/q_shared.h"
#include "jkg_damagetypes.h"

// Disable stupid warnings...
#pragma warning( disable : 4996 )

#define	MISSILE_PRESTEP_TIME	50

extern void laserTrapStick( gentity_t *ent, vec3_t endpos, vec3_t normal );
extern void Jedi_Decloak( gentity_t *self );

extern qboolean FighterIsLanded( Vehicle_t *pVeh, playerState_t *parentPS );

/*
================
G_ReflectMissile

  Reflect the missile roughly back at it's owner
================
*/
float RandFloat(float min, float max);
void G_ReflectMissile( gentity_t *ent, gentity_t *missile, vec3_t forward )
{
	vec3_t	bounce_dir;
	int		i;
	float	speed;
	int		isowner = 0;

	if (missile->r.ownerNum == ent->s.number)
	{ //the original owner is bouncing the missile, so don't try to bounce it back at him
		isowner = 1;
	}

	//save the original speed
	speed = VectorNormalize( missile->s.pos.trDelta );

	if ( &g_entities[missile->r.ownerNum] && missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART && !isowner )
	{//bounce back at them if you can
		VectorSubtract( g_entities[missile->r.ownerNum].r.currentOrigin, missile->r.currentOrigin, bounce_dir );
		VectorNormalize( bounce_dir );
	}
	else if (isowner)
	{ //in this case, actually push the missile away from me, and since we're giving boost to our own missile by pushing it, up the velocity
		vec3_t missile_dir;

		speed *= 1.5;

		VectorSubtract( missile->r.currentOrigin, ent->r.currentOrigin, missile_dir );
		VectorCopy( missile->s.pos.trDelta, bounce_dir );
		VectorScale( bounce_dir, DotProduct( forward, missile_dir ), bounce_dir );
		VectorNormalize( bounce_dir );
	}
	else
	{
		vec3_t missile_dir;

		VectorSubtract( ent->r.currentOrigin, missile->r.currentOrigin, missile_dir );
		VectorCopy( missile->s.pos.trDelta, bounce_dir );
		VectorScale( bounce_dir, DotProduct( forward, missile_dir ), bounce_dir );
		VectorNormalize( bounce_dir );
	}
	for ( i = 0; i < 3; i++ )
	{
		bounce_dir[i] += RandFloat( -0.2f, 0.2f );
	}

	VectorNormalize( bounce_dir );
	VectorScale( bounce_dir, speed, missile->s.pos.trDelta );
	missile->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( missile->r.currentOrigin, missile->s.pos.trBase );
	if ( missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART )
	{//you are mine, now!
		missile->r.ownerNum = ent->s.number;
	}
	/*if (missile->s.weapon == WP_ROCKET_LAUNCHER)
	{//stop homing
		missile->think = 0;
		missile->nextthink = 0;
	}*/
}

void G_DeflectMissile( gentity_t *ent, gentity_t *missile, vec3_t forward )
{
	vec3_t	bounce_dir;
	int		i;
	float	speed;
	vec3_t missile_dir;

	//save the original speed
	speed = VectorNormalize( missile->s.pos.trDelta );

	if (ent->client)
	{
		//VectorSubtract( ent->r.currentOrigin, missile->r.currentOrigin, missile_dir );
		AngleVectors(ent->client->ps.viewangles, missile_dir, 0, 0);
		VectorCopy(missile_dir, bounce_dir);
		//VectorCopy( missile->s.pos.trDelta, bounce_dir );
		VectorScale( bounce_dir, DotProduct( forward, missile_dir ), bounce_dir );
		VectorNormalize( bounce_dir );
	}
	else
	{
		VectorCopy(forward, bounce_dir);
		VectorNormalize(bounce_dir);
	}

	for ( i = 0; i < 3; i++ )
	{
		bounce_dir[i] += RandFloat( -1.0f, 1.0f );
	}

	VectorNormalize( bounce_dir );
	VectorScale( bounce_dir, speed, missile->s.pos.trDelta );
	missile->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( missile->r.currentOrigin, missile->s.pos.trBase );
	if ( missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART )
	{//you are mine, now!
		missile->r.ownerNum = ent->s.number;
	}
	/*if (missile->s.weapon == WP_ROCKET_LAUNCHER)
	{//stop homing
		missile->think = 0;
		missile->nextthink = 0;
	}*/
}

/*
================
G_BounceMissile

================
*/
void G_BounceMissile( gentity_t *ent, trace_t *trace, qboolean HIT_TREE ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );

	if (HIT_TREE)
	{
		VectorCopy(velocity, trace->plane.normal);
		trace->plane.normal[0] += (irand(0,180) - 90);
		trace->plane.normal[1] += (irand(0,180) - 90);
		VectorNormalize(trace->plane.normal);
		dot = DotProduct( velocity, trace->plane.normal );
		VectorMA( velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta );
	}
	else
	{
		dot = DotProduct( velocity, trace->plane.normal );
		VectorMA( velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta );
	}


	if ( ent->flags & FL_BOUNCE_SHRAPNEL )
	{
		VectorScale( ent->s.pos.trDelta, 0.25f, ent->s.pos.trDelta );
		ent->s.pos.trType = TR_GRAVITY;

		// check for stop
		if ( trace->plane.normal[2] > 0.7 && ent->s.pos.trDelta[2] < 40 ) //this can happen even on very slightly sloped walls, so changed it from > 0 to > 0.7
		{
			G_SetOrigin( ent, trace->endpos );
			ent->nextthink = level.time + 100;
			return;
		}
	}
	else if ( ent->flags & FL_BOUNCE_HALF )
	{
		VectorScale( ent->s.pos.trDelta, 0.65f, ent->s.pos.trDelta );
		// check for stop
		if ( trace->plane.normal[2] > 0.2 && VectorLength( ent->s.pos.trDelta ) < 40 )
		{
			G_SetOrigin( ent, trace->endpos );
			return;
		}
	}

	if (ent->s.weapon == WP_THERMAL)
	{ //slight hack for hit sound
		G_Sound(ent, CHAN_BODY, G_SoundIndex(va("sound/weapons/thermal/bounce%i.wav", Q_irand(1, 2))));
	}
	else if (ent->s.weapon == WP_FRAG_GRENADE)
	{ //slight hack for hit sound
		G_Sound(ent, CHAN_BODY, G_SoundIndex(va("sound/weapons/thermal/bounce%i.wav", Q_irand(1, 2))));
	}
	else if (ent->s.weapon == WP_CYROBAN_GRENADE)
	{ //slight hack for hit sound
		G_Sound(ent, CHAN_BODY, G_SoundIndex(va("sound/weapons/thermal/bounce%i.wav", Q_irand(1, 2))));
	}
	else if (ent->s.weapon == WP_SABER)
	{
		G_Sound(ent, CHAN_SABER, G_SoundIndex(va("sound/weapons/saber/bounce%i.wav", Q_irand(1, 3))));
	}
	else if (ent->s.weapon == G2_MODEL_PART)
	{
		//Limb bounce sound?
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);

	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;

	if (ent->bounceCount != -5)
	{
		ent->bounceCount--;
	}
}


/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
void G_ExplodeMissile( gentity_t *ent ) {
	vec3_t		dir;
	vec3_t		origin;

	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
	SnapVector( origin );
	G_SetOrigin( ent, origin );

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	ent->s.eType = ET_GENERAL;
	G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( dir ) );

	ent->freeAfterEvent = qtrue;

	ent->takedamage = qfalse;
	// splash damage
	if ( ent->splashDamage ) 
	{
		if( G_RadiusDamage( ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent, ent, ent->splashMethodOfDeath ) )
		{
			if (ent->parent)
			{
				g_entities[ent->parent->s.number].client->accuracy_hits++;
			}
			else if (ent->activator)
			{
				g_entities[ent->activator->s.number].client->accuracy_hits++;
			}
		}
	}

	trap->LinkEntity( (sharedEntity_t *)ent );
}

void G_RunStuckMissile( gentity_t *ent )
{
	if ( ent->takedamage )
	{
		if ( ent->s.groundEntityNum >= 0 && ent->s.groundEntityNum < ENTITYNUM_WORLD )
		{
			gentity_t *other = &g_entities[ent->s.groundEntityNum];

			if ( (!VectorCompare( vec3_origin, other->s.pos.trDelta ) && other->s.pos.trType != TR_STATIONARY) ||
				(!VectorCompare( vec3_origin, other->s.apos.trDelta ) && other->s.apos.trType != TR_STATIONARY) )
			{//thing I stuck to is moving or rotating now, kill me
				G_Damage( ent, other, other, NULL, NULL, 99999, 0, MOD_CRUSH );
				return;
			}
		}
	}
	// check think function
	G_RunThink( ent, qfalse);
}

/*
================
G_BounceProjectile
================
*/
void G_BounceProjectile( vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout ) {
	vec3_t v, newv;
	float dot;

	VectorSubtract( impact, start, v );
	dot = DotProduct( v, dir );
	VectorMA( v, -2*dot, dir, newv );

	VectorNormalize(newv);
	VectorMA(impact, 8192, newv, endout);
}


//-----------------------------------------------------------------------------
gentity_t *CreateMissile( vec3_t org, vec3_t dir, float vel, int life,
							gentity_t *owner, qboolean altFire)
//-----------------------------------------------------------------------------
{
	gentity_t	*missile;

	missile = G_Spawn();

	missile->nextthink = level.time + life;
	missile->think = G_FreeEntity;
	missile->s.eType = ET_MISSILE;
	missile->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	missile->parent = owner;
	missile->r.ownerNum = owner->s.number;

	if (altFire)
	{
		missile->s.eFlags |= EF_ALT_FIRING;
	}

	missile->s.pos.trType = TR_LINEAR;
	missile->s.pos.trTime = level.time;// - MISSILE_PRESTEP_TIME;	// NOTENOTE This is a Quake 3 addition over JK2
	missile->target_ent = NULL;

	SnapVector(org);
	VectorCopy( org, missile->s.pos.trBase );
	VectorScale( dir, vel, missile->s.pos.trDelta );
	VectorCopy( org, missile->r.currentOrigin);
	SnapVector(missile->s.pos.trDelta);

	missile->s.primaryWeapon = (owner && owner->client) ? BG_EquippedWeaponVisualType3(&owner->client->ps) : WEAPON_STAT3_SHOT_DEFAULT;

	return missile;
}

//EFX Bounce impact code
//keeping this part of the code to add bounce impact efx later on for skills
void G_MissileBounceEffect( gentity_t *ent, vec3_t org, vec3_t dir )
{
	//FIXME: have an EV_BOUNCE_MISSILE event that checks the s.weapon and does the appropriate effect
	switch( ent->s.weapon )
	{
	case WP_MODULIZED_WEAPON:
		G_PlayEffectID( G_EffectIndex("blaster/deflect"), ent->r.currentOrigin, dir );
		break;
	default:
		{
			gentity_t *te = G_TempEntity( org, EV_SABER_BLOCK );
			VectorCopy(org, te->s.origin);
			VectorCopy(dir, te->s.angles);
			te->s.eventParm = 0;
			te->s.weapon = 0;//saberNum
			te->s.legsAnim = 0;//bladeNum
		}
		break;
	}
}

/*
================
G_MissileImpact
================
*/
//[SaberSys]
#ifdef __MISSILES_AUTO_PARRY__
void WP_SaberBlockNonRandom(gentity_t *self, vec3_t hitloc, qboolean missileBlock);
#endif // __MISSILES_AUTO_PARRY__
//[/SaberSys]
void WP_flechette_alt_blow(gentity_t *ent);
void G_MissileImpact(gentity_t *ent, trace_t *trace, qboolean HIT_TREE) {
	gentity_t		*other;
	qboolean		hitClient = qfalse;
	qboolean		isKnockedSaber = qfalse;

	if (HIT_TREE)
	{
		//EFX Bounce impact code
		//keeping this part of the code to add bounce impact efx later on for skills
		// check for bounce
		if ((ent->bounceCount > 0 || ent->bounceCount == -5) &&
			(ent->flags & (FL_BOUNCE | FL_BOUNCE_HALF)))
		{
			// grenades would be handled by this.
			G_BounceMissile(ent, trace, HIT_TREE);
			G_AddEvent(ent, EV_GRENADE_BOUNCE, 0);
			return;
		}
		else if (ent->neverFree && ent->s.weapon == WP_SABER && (ent->flags & FL_BOUNCE_HALF))
		{ //this is a knocked-away saber
			if (ent->bounceCount > 0 || ent->bounceCount == -5)
			{
				G_BounceMissile(ent, trace, HIT_TREE);
				G_AddEvent(ent, EV_GRENADE_BOUNCE, 0);
				return;
			}

			isKnockedSaber = qtrue;
		}

		// I would glom onto the FL_BOUNCE code section above, but don't feel like risking breaking something else
		if (((ent->bounceCount > 0 || ent->bounceCount == -5) && (ent->flags&(FL_BOUNCE_SHRAPNEL)))
			|| ((trace->surfaceFlags&SURF_FORCEFIELD) && !ent->splashDamage && !ent->splashRadius && (ent->bounceCount > 0 || ent->bounceCount == -5)))
		{
			G_BounceMissile(ent, trace, HIT_TREE);

			if (ent->bounceCount < 1)
			{
				ent->flags &= ~FL_BOUNCE_SHRAPNEL;
			}
			return;
		}

		goto killProj;
	}

	other = &g_entities[trace->entityNum];

	//EFX Bounce impact code
	//keeping this part of the code to add bounce impact efx later on for skills
	// check for bounce
	if (!other->takedamage &&
		(ent->bounceCount > 0 || ent->bounceCount == -5) &&
		(ent->flags & (FL_BOUNCE | FL_BOUNCE_HALF)))
	{
		// grenades would be handled by this.
		G_BounceMissile(ent, trace, HIT_TREE);
		G_AddEvent(ent, EV_GRENADE_BOUNCE, 0);
		return;
	}
	else if (ent->neverFree && ent->s.weapon == WP_SABER && (ent->flags & FL_BOUNCE_HALF))
	{ //this is a knocked-away saber
		if (ent->bounceCount > 0 || ent->bounceCount == -5)
		{
			G_BounceMissile(ent, trace, HIT_TREE);
			G_AddEvent(ent, EV_GRENADE_BOUNCE, 0);
			return;
		}

		isKnockedSaber = qtrue;
	}

	// I would glom onto the FL_BOUNCE code section above, but don't feel like risking breaking something else
	if ((!other->takedamage && (ent->bounceCount > 0 || ent->bounceCount == -5) && (ent->flags&(FL_BOUNCE_SHRAPNEL))) || ((trace->surfaceFlags&SURF_FORCEFIELD) && !ent->splashDamage&&!ent->splashRadius && (ent->bounceCount > 0 || ent->bounceCount == -5)))
	{
		G_BounceMissile(ent, trace, HIT_TREE);

		if (ent->bounceCount < 1)
		{
			ent->flags &= ~FL_BOUNCE_SHRAPNEL;
		}
		return;
	}

	if ((other->r.contents & CONTENTS_LIGHTSABER) && !isKnockedSaber)
	{ //hit this person's saber, so..
		gentity_t *otherOwner = &g_entities[other->r.ownerNum];

		if (otherOwner->takedamage && otherOwner->client && otherOwner->client->ps.duelInProgress &&
			otherOwner->client->ps.duelIndex != ent->r.ownerNum)
		{
			goto killProj;
		}
	}
	else if (!isKnockedSaber)
	{
		if (other->takedamage && other->client && other->client->ps.duelInProgress &&
			other->client->ps.duelIndex != ent->r.ownerNum)
		{
			goto killProj;
		}
	}

	if (other->flags & FL_DMG_BY_HEAVY_WEAP_ONLY)
	{
		if (ent->methodOfDeath != MOD_REPEATER_ALT &&
			ent->methodOfDeath != MOD_ROCKET &&
			ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
			ent->methodOfDeath != MOD_ROCKET_HOMING &&
			ent->methodOfDeath != MOD_THERMAL &&
			ent->methodOfDeath != MOD_THERMAL_SPLASH &&
			ent->methodOfDeath != MOD_TRIP_MINE_SPLASH &&
			ent->methodOfDeath != MOD_TIMED_MINE_SPLASH &&
			ent->methodOfDeath != MOD_DET_PACK_SPLASH &&
			ent->methodOfDeath != MOD_VEHICLE &&
			ent->methodOfDeath != MOD_CONC &&
			ent->methodOfDeath != MOD_CONC_ALT &&
			ent->methodOfDeath != MOD_SABER &&
			ent->methodOfDeath != MOD_TURBLAST)
		{
			vec3_t fwd;

			if (trace)
			{
				VectorCopy(trace->plane.normal, fwd);
			}
			else
			{ //oh well
				AngleVectors(other->r.currentAngles, fwd, NULL, NULL);
			}

			G_DeflectMissile(other, ent, fwd);
			G_MissileBounceEffect(ent, ent->r.currentOrigin, fwd);
			return;
		}
	}

	if ((other->flags & FL_SHIELDED) &&
		ent->s.weapon != WP_FRAG_GRENADE &&
		ent->s.weapon != WP_CYROBAN_GRENADE &&
		ent->s.weapon != WP_THERMAL &&
		ent->s.weapon != WP_TRIP_MINE &&
		ent->s.weapon != WP_DET_PACK &&
		ent->s.weapon != WP_EMPLACED_GUN &&
		ent->methodOfDeath != MOD_REPEATER_ALT &&
		ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
		ent->methodOfDeath != MOD_TURBLAST &&
		ent->methodOfDeath != MOD_VEHICLE &&
		ent->methodOfDeath != MOD_CONC &&
		ent->methodOfDeath != MOD_CONC_ALT &&
		!(ent->dflags&DAMAGE_HEAVY_WEAP_CLASS))
	{
		vec3_t fwd;

		if (other->client)
		{
			AngleVectors(other->client->ps.viewangles, fwd, NULL, NULL);
		}
		else
		{
			AngleVectors(other->r.currentAngles, fwd, NULL, NULL);
		}

		G_DeflectMissile(other, ent, fwd);
		G_MissileBounceEffect(ent, ent->r.currentOrigin, fwd);
		return;
	}

	if (other->takedamage && other->client &&
		ent->s.weapon != WP_FRAG_GRENADE &&
		ent->s.weapon != WP_CYROBAN_GRENADE &&
		ent->s.weapon != WP_THERMAL &&
		ent->s.weapon != WP_TRIP_MINE &&
		ent->s.weapon != WP_DET_PACK &&
		ent->methodOfDeath != MOD_REPEATER_ALT &&
		ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
		ent->methodOfDeath != MOD_CONC &&
		ent->methodOfDeath != MOD_CONC_ALT &&
		other->client->ps.saberBlockTime < level.time &&
		!isKnockedSaber &&
		WP_SaberCanBlock(ent, other, ent->r.currentOrigin, ent->s.pos.trBase, 0, ent->methodOfDeath, qtrue, ent->damage))
	{ //only block one projectile per 200ms (to prevent giant swarms of projectiles being blocked)
		vec3_t fwd;
		gentity_t *te;
		int otherDefLevel = other->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE];

		te = G_TempEntity(ent->r.currentOrigin, EV_SABER_BLOCK);
		VectorCopy(ent->r.currentOrigin, te->s.origin);
		VectorCopy(trace->plane.normal, te->s.angles);
		te->s.eventParm = 0;
		te->s.weapon = 0;//saberNum
		te->s.legsAnim = 0;//bladeNum

		/*if (other->client->ps.velocity[2] > 0 ||
		other->client->pers.cmd.forwardmove ||
		other->client->pers.cmd.rightmove)
		*/
		if (other->client->ps.velocity[2] > 0 ||
			other->client->pers.cmd.forwardmove < 0) //now we only do it if jumping or running backward. Should be able to full-on charge.
		{
			otherDefLevel -= 1;
			if (otherDefLevel < 0)
			{
				otherDefLevel = 0;
			}
		}

		AngleVectors(other->client->ps.viewangles, fwd, NULL, NULL);
		if (otherDefLevel == FORCE_LEVEL_1)
		{
			//if def is only level 1, instead of deflecting the shot it should just die here
		}
		else if (otherDefLevel == FORCE_LEVEL_2)
		{
			G_DeflectMissile(other, ent, fwd);
		}
		else
		{
			G_ReflectMissile(other, ent, fwd);
		}
		other->client->ps.saberBlockTime = level.time + (350 - (otherDefLevel * 100)); //200;

		//For jedi AI
		other->client->ps.saberEventFlags |= SEF_DEFLECTED;

		if (otherDefLevel == FORCE_LEVEL_3)
		{
			other->client->ps.saberBlockTime = 0; //^_^
		}

		if (otherDefLevel == FORCE_LEVEL_1)
		{
			goto killProj;
		}
		return;
	}
	else if ((other->r.contents & CONTENTS_LIGHTSABER) && !isKnockedSaber)
	{ //hit this person's saber, so..
		gentity_t *otherOwner = &g_entities[other->r.ownerNum];

		if (otherOwner->takedamage && otherOwner->client &&
			ent->s.weapon != WP_FRAG_GRENADE &&
			ent->s.weapon != WP_CYROBAN_GRENADE &&
			ent->s.weapon != WP_THERMAL &&
			ent->s.weapon != WP_TRIP_MINE &&
			ent->s.weapon != WP_DET_PACK &&
			ent->methodOfDeath != MOD_REPEATER_ALT &&
			ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
			ent->methodOfDeath != MOD_CONC &&
			ent->methodOfDeath != MOD_CONC_ALT /*&&
											   otherOwner->client->ps.saberBlockTime < level.time*/)
		{ //for now still deflect even if saberBlockTime >= level.time because it hit the actual saber
			vec3_t fwd;
			gentity_t *te;
			int otherDefLevel = otherOwner->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE];

			//in this case, deflect it even if we can't actually block it because it hit our saber
			if (otherOwner->client && otherOwner->client->ps.weaponTime <= 0)
			{
				//[SaberSys]
#ifdef __MISSILES_AUTO_PARRY__
				WP_SaberBlockNonRandom(otherOwner, ent->r.currentOrigin, qtrue);
#endif //__MISSILES_AUTO_PARRY__
				//[/SaberSys]
			}

			te = G_TempEntity(ent->r.currentOrigin, EV_SABER_BLOCK);
			VectorCopy(ent->r.currentOrigin, te->s.origin);
			VectorCopy(trace->plane.normal, te->s.angles);
			te->s.eventParm = 0;
			te->s.weapon = 0;//saberNum
			te->s.legsAnim = 0;//bladeNum

			/*if (otherOwner->client->ps.velocity[2] > 0 ||
			otherOwner->client->pers.cmd.forwardmove ||
			otherOwner->client->pers.cmd.rightmove)*/
			if (otherOwner->client->ps.velocity[2] > 0 ||
				otherOwner->client->pers.cmd.forwardmove < 0) //now we only do it if jumping or running backward. Should be able to full-on charge.
			{
				otherDefLevel -= 1;
				if (otherDefLevel < 0)
				{
					otherDefLevel = 0;
				}
			}

			AngleVectors(otherOwner->client->ps.viewangles, fwd, NULL, NULL);

			if (otherDefLevel == FORCE_LEVEL_1)
			{
				//if def is only level 1, instead of deflecting the shot it should just die here
			}
			else if (otherDefLevel == FORCE_LEVEL_2)
			{
				G_DeflectMissile(otherOwner, ent, fwd);
			}
			else
			{
				G_ReflectMissile(otherOwner, ent, fwd);
			}
			otherOwner->client->ps.saberBlockTime = level.time + (350 - (otherDefLevel * 100));//200;

			//For jedi AI
			otherOwner->client->ps.saberEventFlags |= SEF_DEFLECTED;

			if (otherDefLevel == FORCE_LEVEL_3)
			{
				otherOwner->client->ps.saberBlockTime = 0; //^_^
			}

			if (otherDefLevel == FORCE_LEVEL_1)
			{
				goto killProj;
			}
			return;
		}
	}

	// check for sticking
	if (!other->takedamage && (ent->s.eFlags & EF_MISSILE_STICK))
	{
		laserTrapStick(ent, trace->endpos, trace->plane.normal);
		G_AddEvent(ent, EV_MISSILE_STICK, 0);
		return;
	}

	// impact damage
	if (other->takedamage && !isKnockedSaber) {
		// FIXME: wrong damage direction?
		if (ent->damage) {
			vec3_t	velocity;
			qboolean didDmg = qfalse;

			if (LogAccuracyHit(other, &g_entities[ent->r.ownerNum])) 
			{
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
				hitClient = qtrue;
			}
			BG_EvaluateTrajectoryDelta(&ent->s.pos, level.time, velocity);
			if (VectorLength(velocity) == 0) {
				velocity[2] = 1;	// stepped on a grenade
			}

			if (ent->s.weapon == WP_THERMAL && (ent->s.eFlags & EF_FIRING))
			{
				JKG_DoDamage(thermalDetDamageSettings, other, ent, g_entities + ent->r.ownerNum,
					velocity, ent->r.currentOrigin, 0, ent->methodOfDeath);
				didDmg = qtrue;
			}
			else
			{
				G_Damage(other, ent, &g_entities[ent->r.ownerNum], velocity, ent->r.currentOrigin, ent->damage, 0, ent->methodOfDeath);
				didDmg = qtrue;
			}
			
			if (ent->s.weapon == WP_CYROBAN_GRENADE && (ent->s.eFlags & EF_FIRING))
			{
				JKG_DoDamage(GrenadeCryoBanDamageSettings, other, ent, g_entities + ent->r.ownerNum,
					velocity, ent->r.currentOrigin, 0, ent->methodOfDeath);
				didDmg = qtrue;
			}
			else
			{
				G_Damage(other, ent, &g_entities[ent->r.ownerNum], velocity, ent->r.currentOrigin, ent->damage, 0, ent->methodOfDeath);
				didDmg = qtrue;
			}

			if (didDmg && other && other->client)
			{ //What I'm wondering is why this isn't in the NPC pain funcs. But this is what SP does, so whatever.
				class_t	npc_class = other->client->NPC_class;

				// If we are a robot and we aren't currently doing the full body electricity...
				if (npc_class == CLASS_SEEKER || npc_class == CLASS_PROBE || npc_class == CLASS_MOUSE ||
					npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_R5D2 || npc_class == CLASS_REMOTE ||
					npc_class == CLASS_MARK1 || npc_class == CLASS_MARK2 || //npc_class == CLASS_PROTOCOL ||//no protocol, looks odd
					npc_class == CLASS_INTERROGATOR || npc_class == CLASS_ATST || npc_class == CLASS_SENTRY)
				{
					// special droid only behaviors
					if (other->client->ps.electrifyTime < level.time + 100)
					{
						// ... do the effect for a split second for some more feedback
						other->client->ps.electrifyTime = level.time + 450;
					}
					//FIXME: throw some sparks off droids,too
				}
			}
		}

#if 0
		if (ent->s.weapon == WP_UNCLOAKER)
		{//a hit with demp2 decloaks people, disables ships
			if (other && other->client && other->client->NPC_class == CLASS_VEHICLE)
			{//hit a vehicle
				if (other->m_pVehicle //valid vehicle ent
					&& other->m_pVehicle->m_pVehicleInfo//valid stats
					&& (other->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER//always affect speeders
					|| (other->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER && ent->classname && Q_stricmp("vehicle_proj", ent->classname) == 0))//only vehicle ion weapons affect a fighter in this manner
					&& !FighterIsLanded(other->m_pVehicle, &other->client->ps)//not landed
					&& !(other->spawnflags & 2))//and not suspended
				{//vehicles hit by "ion cannons" lose control
					if (other->client->ps.electrifyTime > level.time)
					{//add onto it
						//FIXME: extern the length of the "out of control" time?
						other->client->ps.electrifyTime += Q_irand(200, 500);
						if (other->client->ps.electrifyTime > level.time + 4000)
						{//cap it
							other->client->ps.electrifyTime = level.time + 4000;
						}
					}
					else
					{//start it
						//FIXME: extern the length of the "out of control" time?
						other->client->ps.electrifyTime = level.time + Q_irand(200, 500);
					}
				}
			}
			else if (other && other->client && other->client->ps.powerups[PW_CLOAKED])
			{
				Jedi_Decloak(other);
				if (ent->methodOfDeath == MOD_DEMP2_ALT)
				{//direct hit with alt disables cloak forever
					//permanently disable the saboteur's cloak
					other->client->cloakToggleTime = Q3_INFINITE;
				}
				else
				{//temp disable
					other->client->cloakToggleTime = level.time + Q_irand(3000, 10000);
				}
			}
		}
#endif
	}
killProj:
	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if (!HIT_TREE && other->takedamage && other->client && !isKnockedSaber) 
	{
		G_AddEvent(ent, EV_MISSILE_HIT, DirToByte(trace->plane.normal));
		ent->s.otherEntityNum = other->s.number;
	}
	else if (!HIT_TREE && trace->surfaceFlags & SURF_METALSTEPS) 
	{
		G_AddEvent(ent, EV_MISSILE_MISS_METAL, DirToByte(trace->plane.normal));
	}
	else if (ent->s.weapon != G2_MODEL_PART && !isKnockedSaber) 
	{
		G_AddEvent(ent, EV_MISSILE_MISS, DirToByte(trace->plane.normal));
	}

	if (!isKnockedSaber)
	{
		ent->freeAfterEvent = qtrue;

		// change over to a normal entity right at the point of impact
		ent->s.eType = ET_GENERAL;
	}

	SnapVectorTowards(trace->endpos, ent->s.pos.trBase);	// save net bandwidth

	G_SetOrigin(ent, trace->endpos);

	ent->takedamage = qfalse;

	// splash damage (doesn't apply to person directly hit)
	if (ent->splashDamage && ent->splashRadius)
	{
		G_RadiusDamage(trace->endpos, ent->parent, ent->splashDamage, ent->splashRadius, other, ent, ent->splashMethodOfDeath);
	}

	if (ent->s.weapon == G2_MODEL_PART)
	{
		ent->freeAfterEvent = qfalse; //it will free itself
	}

	trap->LinkEntity((sharedEntity_t *)ent);
}

/*
================
G_RunMissile
================
*/
void G_RunMissile(gentity_t *ent) {
	vec3_t		origin, groundSpot;
	trace_t		tr;
	int			passent;
	qboolean	isKnockedSaber = qfalse;
	qboolean	HIT_TREE = qfalse;

	if (ent->neverFree && ent->s.weapon == WP_SABER && (ent->flags & FL_BOUNCE_HALF))
	{
		isKnockedSaber = qtrue;
		ent->s.pos.trType = TR_GRAVITY;
	}

	// get current position
	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);

	// if this missile bounced off an invulnerability sphere
	if (ent->target_ent) {
		passent = ent->target_ent->s.number;
	}
	else {
		// ignore interactions with the missile owner
		if ((ent->r.svFlags&SVF_OWNERNOTSHARED)
			&& (ent->s.eFlags&EF_JETPACK_ACTIVE))
		{//A vehicle missile that should be solid to its owner
			//I don't care about hitting my owner
			passent = ent->s.number;
		}
		else
		{
			passent = ent->r.ownerNum;
		}
	}
	// trace a line from the previous position to the current position
	if (d_projectileGhoul2Collision.integer)
	{
		trap->Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask, qfalse, G2TRFLAG_DOGHOULTRACE | G2TRFLAG_GETSURFINDEX | G2TRFLAG_THICK | G2TRFLAG_HITCORPSES, g_g2TraceLod.integer);

		if (tr.fraction != 1.0 && tr.entityNum < ENTITYNUM_WORLD)
		{
			gentity_t *g2Hit = &g_entities[tr.entityNum];

			if (g2Hit->inuse && g2Hit->client && g2Hit->ghoul2)
			{ //since we used G2TRFLAG_GETSURFINDEX, tr.surfaceFlags will actually contain the index of the surface on the ghoul2 model we collided with.
				g2Hit->client->g2LastSurfaceHit = tr.surfaceFlags;
				g2Hit->client->g2LastSurfaceTime = level.time;
			}

			if (g2Hit->ghoul2)
			{
				tr.surfaceFlags = 0; //clear the surface flags after, since we actually care about them in here.
			}
		}
	}
	else
	{
		trap->Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask, qfalse, 0, 0);
	}

	HIT_TREE = FOLIAGE_TreeSolidBlocking(ent, origin);

	if (HIT_TREE)
	{

	}
	else if (tr.startsolid || tr.allsolid) {
		// make sure the tr.entityNum is set to the entity we're stuck in
		trap->Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, passent, ent->clipmask, qfalse, 0, 0);
		tr.fraction = 0;
	}
	else {
		VectorCopy(tr.endpos, ent->r.currentOrigin);
	}

	if (ent->passThroughNum && tr.entityNum == (ent->passThroughNum - 1))
	{
		VectorCopy(origin, ent->r.currentOrigin);
		trap->LinkEntity((sharedEntity_t *)ent);
		goto passthrough;
	}

	trap->LinkEntity((sharedEntity_t *)ent);

	if (ent->s.weapon == G2_MODEL_PART && !ent->bounceCount)
	{
		vec3_t lowerOrg;
		trace_t trG;

		VectorCopy(ent->r.currentOrigin, lowerOrg);
		lowerOrg[2] -= 1;
		trap->Trace(&trG, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, lowerOrg, passent, ent->clipmask, qfalse, 0, 0);

		VectorCopy(trG.endpos, groundSpot);

		if (!trG.startsolid && !trG.allsolid && trG.entityNum == ENTITYNUM_WORLD)
		{
			ent->s.groundEntityNum = trG.entityNum;
		}
		else
		{
			ent->s.groundEntityNum = ENTITYNUM_NONE;
		}
	}

	if (tr.fraction != 1 || HIT_TREE) {
		// never explode or bounce on sky
		if (!HIT_TREE && (tr.surfaceFlags & SURF_NOIMPACT)) {
			// If grapple, reset owner
			if (ent->parent && ent->parent->client && ent->parent->client->hook == ent) {
				ent->parent->client->hook = NULL;
			}

			if ((ent->s.weapon == WP_SABER && ent->isSaberEntity) || isKnockedSaber)
			{
				G_RunThink(ent, qfalse);
				return;
			}
			else if (ent->s.weapon != G2_MODEL_PART)
			{
				G_FreeEntity(ent);
				return;
			}
		}

		if (!HIT_TREE
			&& ent->s.weapon > WP_NONE 
			&& ent->s.weapon < WP_NUM_WEAPONS 
			&& (tr.entityNum < MAX_CLIENTS || g_entities[tr.entityNum].s.eType == ET_NPC))
		{ //player or NPC, try making a mark on him
			//copy current pos to s.origin, and current projected traj to origin2
			VectorCopy(ent->r.currentOrigin, ent->s.origin);
			BG_EvaluateTrajectory(&ent->s.pos, level.time, ent->s.origin2);

			if (VectorCompare(ent->s.origin, ent->s.origin2))
			{
				ent->s.origin2[2] += 2.0f; //whatever, at least it won't mess up.
			}
		}

		G_MissileImpact(ent, &tr, HIT_TREE);

		if (HIT_TREE || tr.entityNum == ent->s.otherEntityNum)
		{ //if the impact event other and the trace ent match then it's ok to do the g2 mark
			ent->s.trickedentindex = 1;
		}

		if (ent->s.eType != ET_MISSILE && ent->s.weapon != G2_MODEL_PART)
		{
			return;		// exploded
		}
	}

passthrough:
	if (ent->s.pos.trType == TR_STATIONARY && (ent->s.eFlags&EF_MISSILE_STICK))
	{//stuck missiles should check some special stuff
		G_RunStuckMissile(ent);
		return;
	}

	if (ent->s.weapon == G2_MODEL_PART)
	{
		if (ent->s.groundEntityNum == ENTITYNUM_WORLD)
		{
			ent->s.pos.trType = TR_LINEAR;
			VectorClear(ent->s.pos.trDelta);
			ent->s.pos.trTime = level.time;

			VectorCopy(groundSpot, ent->s.pos.trBase);
			VectorCopy(groundSpot, ent->r.currentOrigin);

			if (ent->s.apos.trType != TR_STATIONARY)
			{
				ent->s.apos.trType = TR_STATIONARY;
				ent->s.apos.trTime = level.time;

				ent->s.apos.trBase[ROLL] = 0;
				ent->s.apos.trBase[PITCH] = 0;
			}
		}
	}

	// check think function after bouncing
	G_RunThink(ent, qfalse);
}


//=============================================================================




