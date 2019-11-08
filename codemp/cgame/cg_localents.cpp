// Copyright (C) 1999-2000 Id Software, Inc.
//

// cg_localents.c -- every frame, generate renderer commands for locally
// processed entities, like smoke puffs, gibs, shells, etc.

#include "cg_local.h"

int numLocalSoundEntities = 0;

#define	MAX_LOCAL_ENTITIES	2048 // 512
localEntity_t	cg_localEntities[MAX_LOCAL_ENTITIES];
localEntity_t	cg_activeLocalEntities;		// double linked list
localEntity_t	*cg_freeLocalEntities;		// single linked list

/*
===================
CG_InitLocalEntities

This is called at startup and for tournament restarts
===================
*/
void	CG_InitLocalEntities( void ) {
	int		i;

	memset( cg_localEntities, 0, sizeof( cg_localEntities ) );
	cg_activeLocalEntities.next = &cg_activeLocalEntities;
	cg_activeLocalEntities.prev = &cg_activeLocalEntities;
	cg_freeLocalEntities = cg_localEntities;
	for ( i = 0 ; i < MAX_LOCAL_ENTITIES - 1 ; i++ ) {
		cg_localEntities[i].next = &cg_localEntities[i+1];
	}
}


/*
==================
CG_FreeLocalEntity
==================
*/
void CG_FreeLocalEntity( localEntity_t *le ) {
	if ( !le->prev ) {
		trap->Error( ERR_DROP, "CG_FreeLocalEntity: not active" );
		return;
	}

	// remove from the doubly linked active list
	le->prev->next = le->next;
	le->next->prev = le->prev;

	// the free list is only singly linked
	le->next = cg_freeLocalEntities;
	cg_freeLocalEntities = le;
}

/*
===================
CG_AllocLocalEntity

Will allways succeed, even if it requires freeing an old active entity
===================
*/
localEntity_t	*CG_AllocLocalEntity( void ) {
	localEntity_t	*le;

	if ( !cg_freeLocalEntities ) {
		// no free entities, so free the one at the end of the chain
		// remove the oldest active entity
		CG_FreeLocalEntity( cg_activeLocalEntities.prev );
	}

	le = cg_freeLocalEntities;
	cg_freeLocalEntities = cg_freeLocalEntities->next;

	memset( le, 0, sizeof( *le ) );

	// link into the active list
	le->next = cg_activeLocalEntities.next;
	le->prev = &cg_activeLocalEntities;
	cg_activeLocalEntities.next->prev = le;
	cg_activeLocalEntities.next = le;
	return le;
}


/*
====================================================================================

FRAGMENT PROCESSING

A fragment localentity interacts with the environment in some way (hitting walls),
or generates more localentities along a trail.

====================================================================================
*/

/*
================
CG_BloodTrail

Leave expanding blood puffs behind gibs
================
*/
void CG_BloodTrail( localEntity_t *le ) {
	int		t;
	int		t2;
	int		step;
	vec3_t	newOrigin;
	localEntity_t	*blood;

	step = 150;
	t = step * ( (cg.time - cg.frametime + step ) / step );
	t2 = step * ( cg.time / step );

	for ( ; t <= t2; t += step ) {
		BG_EvaluateTrajectory( &le->pos, t, newOrigin );

		blood = CG_SmokePuff( newOrigin, vec3_origin,
					  20,		// radius
					  1, 1, 1, 1,	// color
					  2000,		// trailTime
					  t,		// startTime
					  0,		// fadeInTime
					  0,		// flags
					  /*cgs.media.bloodTrailShader*/0 );
		// use the optimized version
		blood->leType = LE_FALL_SCALE_FADE;
		// drop a total of 40 units over its lifetime
		blood->pos.trDelta[2] = 40;
	}
}


/*
================
CG_FragmentBounceMark
================
*/
void CG_FragmentBounceMark( localEntity_t *le, trace_t *trace ) {
//	int radius;

	if ( le->leMarkType == LEMT_BLOOD ) {
	//	radius = 16 + (rand()&31);
	//	CG_ImpactMark( cgs.media.bloodMarkShader, trace->endpos, trace->plane.normal, random()*360, 1,1,1,1, qtrue, radius, qfalse );
	} else if ( le->leMarkType == LEMT_BURN ) {
	//	radius = 8 + (rand()&15);
	//	CG_ImpactMark( cgs.media.burnMarkShader, trace->endpos, trace->plane.normal, random()*360, 1,1,1,1, qtrue, radius, qfalse );
	}

	// don't allow a fragment to make multiple marks, or they pile up while settling
	le->leMarkType = LEMT_NONE;
}

/*
================
CG_FragmentBounceSound
================
*/
void CG_FragmentBounceSound( localEntity_t *le, trace_t *trace ) {
	// half the fragments will make a bounce sounds
	if ( rand() & 1 )
	{
		sfxHandle_t	s = 0;

		switch( le->leBounceSoundType )
		{
		case LEBS_ROCK:
			s = cgs.media.rockBounceSound[Q_irand(0,1)];
			break;
		case LEBS_METAL:
			s = cgs.media.metalBounceSound[Q_irand(0,1)];// FIXME: make sure that this sound is registered properly...might still be rock bounce sound....
			break;
		default:
			return;
		}

		if ( s )
		{
			trap->S_StartSound( trace->endpos, ENTITYNUM_WORLD, CHAN_AUTO, s );
		}

		// bouncers only make the sound once...
		// FIXME: arbitrary...change if it bugs you
		le->leBounceSoundType = LEBS_NONE;
	}
	else if ( rand() & 1 )
	{
		// we may end up bouncing again, but each bounce reduces the chance of playing the sound again or they may make a lot of noise when they settle
		// FIXME: maybe just always do this??
		le->leBounceSoundType = LEBS_NONE;
	}
}


/*
================
CG_ReflectVelocity
================
*/
void CG_ReflectVelocity( localEntity_t *le, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = cg.time - cg.frametime + cg.frametime * trace->fraction;
	BG_EvaluateTrajectoryDelta( &le->pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, le->pos.trDelta );

	VectorScale( le->pos.trDelta, le->bounceFactor, le->pos.trDelta );

	VectorCopy( trace->endpos, le->pos.trBase );
	le->pos.trTime = cg.time;

	// check for stop, making sure that even on low FPS systems it doesn't bobble
	if ( trace->allsolid ||
		( trace->plane.normal[2] > 0 &&
		( le->pos.trDelta[2] < 40 || le->pos.trDelta[2] < -cg.frametime * le->pos.trDelta[2] ) ) ) {
		le->pos.trType = TR_STATIONARY;
	} else {

	}
}

/*
================
CG_AddFragment
================
*/
void CG_AddFragment( localEntity_t *le ) {
	vec3_t	newOrigin;
	trace_t	trace;

	if (le->forceAlpha)
	{
		le->refEntity.renderfx |= RF_FORCE_ENT_ALPHA;
		le->refEntity.shaderRGBA[3] = le->forceAlpha;
	}

	if ( le->pos.trType == TR_STATIONARY ) {
		// sink into the ground if near the removal time
		int		t;
		float	t_e;

		t = le->endTime - cg.time;
		if ( t < (SINK_TIME*2) ) {
			le->refEntity.renderfx |= RF_FORCE_ENT_ALPHA;
			t_e = (float)((float)(le->endTime - cg.time)/(SINK_TIME*2));
			t_e = (int)((t_e)*255);

			if (t_e > 255)
			{
				t_e = 255;
			}
			if (t_e < 1)
			{
				t_e = 1;
			}

			if (le->refEntity.shaderRGBA[3] && t_e > le->refEntity.shaderRGBA[3])
			{
				t_e = le->refEntity.shaderRGBA[3];
			}

			le->refEntity.shaderRGBA[3] = t_e;

			AddRefEntityToScene( &le->refEntity );
		} else {
			AddRefEntityToScene( &le->refEntity );
		}

		return;
	}

	// calculate new position
	BG_EvaluateTrajectory( &le->pos, cg.time, newOrigin );

	// trace a line from previous position to new position
	CG_Trace( &trace, le->refEntity.origin, NULL, NULL, newOrigin, -1, CONTENTS_SOLID );
	if ( trace.fraction == 1.0 ) {
		// still in free fall
		VectorCopy( newOrigin, le->refEntity.origin );

		if ( le->leFlags & LEF_TUMBLE ) {
			vec3_t angles;

			BG_EvaluateTrajectory( &le->angles, cg.time, angles );
			AnglesToAxis( angles, le->refEntity.axis );
			ScaleModelAxis(&le->refEntity);
		}

		AddRefEntityToScene( &le->refEntity );

		// add a blood trail
		if ( le->leBounceSoundType == LEBS_BLOOD ) {
			CG_BloodTrail( le );
		}

		return;
	}

	// if it is in a nodrop zone, remove it
	// this keeps gibs from waiting at the bottom of pits of death
	// and floating levels
	if ( CG_PointContents( trace.endpos, 0 ) & CONTENTS_NODROP ) {
		CG_FreeLocalEntity( le );
		return;
	}

	if (!trace.startsolid)
	{
		// leave a mark
		CG_FragmentBounceMark( le, &trace );

		// do a bouncy sound
		CG_FragmentBounceSound( le, &trace );

		if (le->bounceSound)
		{ //specified bounce sound (debris)
			trap->S_StartSound(le->pos.trBase, ENTITYNUM_WORLD, CHAN_AUTO, le->bounceSound);
		}

		// reflect the velocity on the trace plane
		CG_ReflectVelocity( le, &trace );

		AddRefEntityToScene( &le->refEntity );
	}
}

/*
=====================================================================

TRIVIAL LOCAL ENTITIES

These only do simple scaling or modulation before passing to the renderer
=====================================================================
*/

/*
====================
CG_AddFadeRGB
====================
*/
void CG_AddFadeRGB( localEntity_t *le ) {
	refEntity_t *re;
	float c;

	re = &le->refEntity;

	c = ( le->endTime - cg.time ) * le->lifeRate;
	c *= 0xff;

	re->shaderRGBA[0] = le->color[0] * c;
	re->shaderRGBA[1] = le->color[1] * c;
	re->shaderRGBA[2] = le->color[2] * c;
	re->shaderRGBA[3] = le->color[3] * c;

	AddRefEntityToScene( re );
}

static void CG_AddFadeScaleModel( localEntity_t *le )
{
	refEntity_t	*ent = &le->refEntity;

	float frac = ( cg.time - le->startTime )/((float)( le->endTime - le->startTime ));

	frac *= frac * frac; // yes, this is completely ridiculous...but it causes the shell to grow slowly then "explode" at the end

	ent->nonNormalizedAxes = qtrue;

	AxisCopy( axisDefault, ent->axis );

	VectorScale( ent->axis[0], le->radius * frac, ent->axis[0] );
	VectorScale( ent->axis[1], le->radius * frac, ent->axis[1] );
	VectorScale( ent->axis[2], le->radius * 0.5f * frac, ent->axis[2] );

	frac = 1.0f - frac;

	ent->shaderRGBA[0] = le->color[0] * frac;
	ent->shaderRGBA[1] = le->color[1] * frac;
	ent->shaderRGBA[2] = le->color[2] * frac;
	ent->shaderRGBA[3] = le->color[3] * frac;

	// add the entity
	AddRefEntityToScene( ent );
}

/*
==================
CG_AddMoveScaleFade
==================
*/
static void CG_AddMoveScaleFade( localEntity_t *le ) {
	refEntity_t	*re;
	float		c;
	vec3_t		delta;
	float		len;

	re = &le->refEntity;

	if ( le->fadeInTime > le->startTime && cg.time < le->fadeInTime ) {
		// fade / grow time
		c = 1.0 - (float) ( le->fadeInTime - cg.time ) / ( le->fadeInTime - le->startTime );
	}
	else {
		// fade / grow time
		c = ( le->endTime - cg.time ) * le->lifeRate;
	}

	re->shaderRGBA[3] = 0xff * c * le->color[3];

	if ( !( le->leFlags & LEF_PUFF_DONT_SCALE ) ) {
		re->radius = le->radius * ( 1.0 - c ) + 8;
	}

	BG_EvaluateTrajectory( &le->pos, cg.time, re->origin );

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < le->radius ) {
		CG_FreeLocalEntity( le );
		return;
	}

	AddRefEntityToScene( re );
}

/*
==================
CG_AddPuff
==================
*/
static void CG_AddPuff( localEntity_t *le ) {
	refEntity_t	*re;
	float		c;
	vec3_t		delta;
	float		len;

	re = &le->refEntity;

	// fade / grow time
	c = ( le->endTime - cg.time ) / (float)( le->endTime - le->startTime );

	re->shaderRGBA[0] = le->color[0] * c;
	re->shaderRGBA[1] = le->color[1] * c;
	re->shaderRGBA[2] = le->color[2] * c;

	if ( !( le->leFlags & LEF_PUFF_DONT_SCALE ) ) {
		re->radius = le->radius * ( 1.0 - c ) + 8;
	}

	BG_EvaluateTrajectory( &le->pos, cg.time, re->origin );

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < le->radius ) {
		CG_FreeLocalEntity( le );
		return;
	}

	AddRefEntityToScene( re );
}

/*
===================
CG_AddScaleFade

For rocket smokes that hang in place, fade out, and are
removed if the view passes through them.
There are often many of these, so it needs to be simple.
===================
*/
static void CG_AddScaleFade( localEntity_t *le ) {
	refEntity_t	*re;
	float		c;
	vec3_t		delta;
	float		len;

	re = &le->refEntity;

	// fade / grow time
	c = ( le->endTime - cg.time ) * le->lifeRate;

	re->shaderRGBA[3] = 0xff * c * le->color[3];
	re->radius = le->radius * ( 1.0 - c ) + 8;

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < le->radius ) {
		CG_FreeLocalEntity( le );
		return;
	}

	AddRefEntityToScene( re );
}


/*
=================
CG_AddFallScaleFade

This is just an optimized CG_AddMoveScaleFade
For blood mists that drift down, fade out, and are
removed if the view passes through them.
There are often 100+ of these, so it needs to be simple.
=================
*/
static void CG_AddFallScaleFade( localEntity_t *le ) {
	refEntity_t	*re;
	float		c;
	vec3_t		delta;
	float		len;

	re = &le->refEntity;

	// fade time
	c = ( le->endTime - cg.time ) * le->lifeRate;

	re->shaderRGBA[3] = 0xff * c * le->color[3];

	re->origin[2] = le->pos.trBase[2] - ( 1.0 - c ) * le->pos.trDelta[2];

	re->radius = le->radius * ( 1.0 - c ) + 16;

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < le->radius ) {
		CG_FreeLocalEntity( le );
		return;
	}

	AddRefEntityToScene( re );
}



/*
================
CG_AddExplosion
================
*/
static void CG_AddExplosion( localEntity_t *ex ) {
	refEntity_t	*ent;

	ent = &ex->refEntity;

	// add the entity
	AddRefEntityToScene(ent);

	// add the dlight
	if ( ex->light ) {
		float		light;

		light = (float)( cg.time - ex->startTime ) / ( ex->endTime - ex->startTime );
		if ( light < 0.5 ) {
			light = 1.0;
		} else {
			light = 1.0 - ( light - 0.5 ) * 2;
		}
		light = ex->light * light;
		AddLightToScene(ent->origin, light, ex->lightColor[0], ex->lightColor[1], ex->lightColor[2] );
	}
}

/*
================
CG_AddSpriteExplosion
================
*/
static void CG_AddSpriteExplosion( localEntity_t *le ) {
	refEntity_t	re;
	float c;

	re = le->refEntity;

	c = ( le->endTime - cg.time ) / ( float ) ( le->endTime - le->startTime );
	if ( c > 1 ) {
		c = 1.0;	// can happen during connection problems
	}

	re.shaderRGBA[0] = 0xff;
	re.shaderRGBA[1] = 0xff;
	re.shaderRGBA[2] = 0xff;
	re.shaderRGBA[3] = 0xff * c * 0.33;

	re.reType = RT_SPRITE;
	re.radius = 42 * ( 1.0 - c ) + 30;

	AddRefEntityToScene( &re );

	// add the dlight
	if ( le->light ) {
		float		light;

		light = (float)( cg.time - le->startTime ) / ( le->endTime - le->startTime );
		if ( light < 0.5 ) {
			light = 1.0;
		} else {
			light = 1.0 - ( light - 0.5 ) * 2;
		}
		light = le->light * light;
		AddLightToScene(re.origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2] );
	}
}


/*
===================
CG_AddRefEntity
===================
*/
void CG_AddRefEntity( localEntity_t *le ) {
	if (le->endTime < cg.time) {
		CG_FreeLocalEntity( le );
		return;
	}
	AddRefEntityToScene( &le->refEntity );
}

/*
===================
CG_AddScorePlum
===================
*/
#define NUMBER_SIZE		8

void CG_AddScorePlum( localEntity_t *le ) {
	refEntity_t	*re;
	vec3_t		origin, delta, dir, vec, up = {0, 0, 1};
	float		c, len;
	int			i, score, digits[10], numdigits, negative;

	re = &le->refEntity;

	c = ( le->endTime - cg.time ) * le->lifeRate;

	score = le->radius;
	if (score < 0) {
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0x11;
		re->shaderRGBA[2] = 0x11;
	}
	else {
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		if (score >= 50) {
			re->shaderRGBA[1] = 0;
		} else if (score >= 20) {
			re->shaderRGBA[0] = re->shaderRGBA[1] = 0;
		} else if (score >= 10) {
			re->shaderRGBA[2] = 0;
		} else if (score >= 2) {
			re->shaderRGBA[0] = re->shaderRGBA[2] = 0;
		}

	}
	if (c < 0.25)
		re->shaderRGBA[3] = 0xff * 4 * c;
	else
		re->shaderRGBA[3] = 0xff;

	re->radius = NUMBER_SIZE / 2;

	VectorCopy(le->pos.trBase, origin);
	origin[2] += 110 - c * 100;

	VectorSubtract(cg.refdef.vieworg, origin, dir);
	CrossProduct(dir, up, vec);
	VectorNormalize(vec);

	VectorMA(origin, -10 + 20 * sin(c * 2 * M_PI), vec, origin);

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < 20 ) {
		CG_FreeLocalEntity( le );
		return;
	}

	negative = qfalse;
	if (score < 0) {
		negative = qtrue;
		score = -score;
	}

	for (numdigits = 0; !(numdigits && !score); numdigits++) {
		digits[numdigits] = score % 10;
		score = score / 10;
	}

	if (negative) {
		digits[numdigits] = 10;
		numdigits++;
	}

	for (i = 0; i < numdigits; i++) {
		VectorMA(origin, (float) (((float) numdigits / 2) - i) * NUMBER_SIZE, vec, re->origin);
		re->customShader = cgs.media.numberShaders[digits[numdigits-1-i]];
		AddRefEntityToScene( re );
	}
}

/*
===================
CG_AddOLine

For forcefields/other rectangular things
===================
*/
void CG_AddOLine( localEntity_t *le )
{
	refEntity_t	*re;
	float		frac, alpha;

	re = &le->refEntity;

	frac = (cg.time - le->startTime) / ( float ) ( le->endTime - le->startTime );
	if ( frac > 1 )
		frac = 1.0;	// can happen during connection problems
	else if (frac < 0)
		frac = 0.0;

	// Use the liferate to set the scale over time.
	re->data.line.width = le->data.line.width + (le->data.line.dwidth * frac);
	if (re->data.line.width <= 0)
	{
		CG_FreeLocalEntity( le );
		return;
	}

	// We will assume here that we want additive transparency effects.
	alpha = le->alpha + (le->dalpha * frac);
	re->shaderRGBA[0] = 0xff * alpha;
	re->shaderRGBA[1] = 0xff * alpha;
	re->shaderRGBA[2] = 0xff * alpha;
	re->shaderRGBA[3] = 0xff * alpha;	// Yes, we could apply c to this too, but fading the color is better for lines.

	re->shaderTexCoord[0] = 1;
	re->shaderTexCoord[1] = 1;

	re->rotation = 90;

	re->reType = RT_ORIENTEDLINE;

	AddRefEntityToScene( re );
}

/*
===================
CG_AddLine

for beams and the like.
===================
*/
void CG_AddLine( localEntity_t *le )
{
	refEntity_t	*re;

	re = &le->refEntity;

	re->reType = RT_LINE;

	AddRefEntityToScene( re );
}

float mix(float x, float y, float a)
{
	return x * (1.0 - a) + y * a;
}

/*
===================
CG_MapBirds

for warzone flying birds...
===================
*/
extern qboolean		BIRDS_ENABLED;
extern int			BIRDS_COUNT;
extern float		MAP_WATER_LEVEL;
extern vec3_t		MAP_INFO_MINS;
extern vec3_t		MAP_INFO_MAXS;

#define MAX_MAP_BIRDS				512
#define BIRD_MAX_DRAW_DISTANCE		65536.0
#define BIRD_RETURN_HOME_DISTANCE	32768.0
#define BIRD_MAX_CIRCLE_SIZE		8
#define BIRD_CIRCLE_CHANGE_RATE		0.1

typedef struct mapBirds_s {
	vec3_t		origin;
	vec3_t		dir;
	vec3_t		startOrigin;
	float		rotate;
	float		circleSize;
} mapBirds_t;

qboolean		mapBirdsLoaded = qfalse;
int				numMapBirds = 0;
mapBirds_t		mapBirds[MAX_MAP_BIRDS];
qhandle_t		birdModel = -1;

void CG_DrawBird(mapBirds_t *bird)
{
	if (Distance(bird->origin, cg.refdef.vieworg) > BIRD_MAX_DRAW_DISTANCE)
	{// Early cull...
		return;
	}

	refEntity_t ent;

	// Draw the bolt core...
	memset(&ent, 0, sizeof(refEntity_t));
	ent.reType = RT_MODEL;

	ent.modelScale[0] = 100.0;
	ent.modelScale[1] = 100.0;
	ent.modelScale[2] = 100.0;

	VectorCopy(bird->origin, ent.origin);
	vectoangles(bird->dir, ent.angles);
	AnglesToAxis(ent.angles, ent.axis);
	ScaleModelAxis(&ent);

	ent.hModel = birdModel;

	AddRefEntityToScene(&ent);

	// Add sounds as well???
}

void CG_Birds(void)
{
	if (!BIRDS_ENABLED || !BIRDS_COUNT)
	{
		return;
	}

	if (!mapBirdsLoaded)
	{
		memset(mapBirds, 0, sizeof(mapBirds));

		vec3_t MAP_SIZE;
		MAP_SIZE[0] = MAP_INFO_MAXS[0] - MAP_INFO_MINS[0];
		MAP_SIZE[1] = MAP_INFO_MAXS[1] - MAP_INFO_MINS[1];
		MAP_SIZE[2] = MAP_INFO_MAXS[2] - MAP_INFO_MINS[2];

		//trap->Print("CGAME-BIRDS: MAP BOUNDS: %f %f %f x %f %f %f\n", MAP_INFO_MINS[0], MAP_INFO_MINS[1], MAP_INFO_MINS[2], MAP_INFO_MAXS[0], MAP_INFO_MAXS[1], MAP_INFO_MAXS[2]);
		//trap->Print("CGAME-BIRDS: MAP SIZE: %f %f %f\n", MAP_SIZE[0], MAP_SIZE[1], MAP_SIZE[2]);

		for (int i = 0; i < BIRDS_COUNT && i < MAX_MAP_BIRDS; i++)
		{
			mapBirds_t *bird = &mapBirds[i];

			trace_t tr;
			vec3_t downPos;

			bird->origin[0] = MAP_INFO_MINS[0] + irand_big(0, MAP_SIZE[0]);
			bird->origin[1] = MAP_INFO_MINS[0] + irand_big(0, MAP_SIZE[1]);
			bird->origin[2] = MAP_INFO_MAXS[2] - 256.0;

			VectorCopy(bird->origin, downPos);
			downPos[2] = MAP_WATER_LEVEL;

			trap->CM_Trace(&tr, bird->origin, downPos, NULL, NULL, 0, MASK_SOLID, 0);
			bird->origin[2] = tr.endpos[2] + (8192.0 + (8192.0 * random()));

			bird->rotate = 1.0;

			if (irand(0, 1) == 1)
			{// This one flies in counter clockwise...
				bird->rotate = -1.0;
			}

			bird->circleSize = irand(1, BIRD_MAX_CIRCLE_SIZE);

			float a = ((float(cg.time) / 1000.0) / bird->circleSize + i) * bird->rotate;
			float b = 0.0;
			bird->dir[0] = cos(a) * cos(b);
			bird->dir[1] = sin(a) * cos(b);
			bird->dir[2] = sin(b);

			VectorCopy(bird->origin, bird->startOrigin);

			//trap->Print("Bird %i spawn at %f %f %f. Dir %f.\n", i, bird->origin[0], bird->origin[1], bird->origin[2], bird->dir[YAW]);
		}

		birdModel = trap->R_RegisterModel("models/warzone/birds/bird.3ds");

		mapBirdsLoaded = qtrue;
	}

	if (birdModel <= 0)
	{// In case asset is somehow missing...
		return;
	}

	// Update and draw birds...
	for (int i = 0; i < BIRDS_COUNT && i < MAX_MAP_BIRDS; i++)
	{
		mapBirds_t *bird = &mapBirds[i];

		// Randomly adjust the bird's circle size over time...
		if (irand(0, 1) == 1)
		{
			bird->circleSize += BIRD_CIRCLE_CHANGE_RATE;
		}
		else
		{
			bird->circleSize -= BIRD_CIRCLE_CHANGE_RATE;
		}

		bird->circleSize = Q_clampi(1, bird->circleSize, BIRD_MAX_CIRCLE_SIZE);

		float a = ((float(cg.time) / 1000.0) / bird->circleSize + i) * bird->rotate;
		float b = 0.0;
		bird->dir[0] = cos(a) * cos(b);
		bird->dir[1] = sin(a) * cos(b);
		bird->dir[2] = sin(b);


		float distFromStart = DistanceHorizontal(bird->origin, bird->startOrigin);

		// Stay near our start position...
		vec3_t wantedDir;
		VectorSubtract(bird->startOrigin, bird->origin, wantedDir);
		VectorNormalize(wantedDir);

		float returnStrength = pow(Q_clamp(0.0, distFromStart / BIRD_RETURN_HOME_DISTANCE, 1.0), 3.0);

		bird->dir[0] = mix(bird->dir[0], wantedDir[0], returnStrength);
		bird->dir[1] = mix(bird->dir[1], wantedDir[1], returnStrength);
		bird->dir[2] = sin(b);
		VectorNormalize(bird->dir);

		VectorMA(bird->origin, 72.0, bird->dir, bird->origin);

		CG_DrawBird(bird);
	}
}


/*
===================
CG_ShipLaser*

for warzone event ships shooting at each other...
===================
*/
qboolean CG_IsEnemyShip(int myTeam, int otherTeam)
{
	if (myTeam != otherTeam
		&& otherTeam != FACTION_WILDLIFE
		&& otherTeam != FACTION_SPECTATOR)
		return qtrue;

	return qfalse;
}

centity_t *CG_FindEnemyShip(centity_t *myShip)
{
	centity_t *enemyShip = NULL;
	float enemyShipDistance = 131072.0;

	for (int i = 0; i < MAX_GENTITIES; i++)
	{
		centity_t *cent = &cg_entities[i];

		if (!cent) continue;
		if (cent->currentState.eType != ET_SERVERMODEL) continue;
		if (!CG_IsEnemyShip(myShip->currentState.teamowner, cent->currentState.teamowner)) continue;

		// Found one?
		float dist = Distance(myShip->lerpOrigin, cent->lerpOrigin);

		if (dist < enemyShipDistance)
		{
			enemyShip = cent;
			enemyShipDistance = dist;
		}
	}

	return enemyShip;
}

extern void FX_WeaponBolt3D(vec3_t org, vec3_t fwd, float length, float radius, qhandle_t shader);

#define MAX_SHIP_LASERS 512//256
#define MAX_SHIP_LASER_DRAW_DISTANCE 131072.0

typedef struct shipLasers_s {
	vec3_t		origin;
	vec3_t		dir;
	vec3_t		endOrigin;
	int			team;
	float		size;
	qboolean	active;
} shipLasers_t;

shipLasers_t	shipLasers[MAX_SHIP_LASERS];

void CG_ShipLasersThink(void)
{
	for (int i = 0; i < MAX_SHIP_LASERS; i++)
	{
		shipLasers_t *laser = &shipLasers[i];

		if (laser && laser->active)
		{
			qhandle_t bolt3D = NULL;

			switch (laser->team)
			{
			case FACTION_WILDLIFE:
			{// Wildlife is native and does not spawn from a ship...
				continue;
			}
			break;
			case FACTION_EMPIRE:
			{
				bolt3D = cgs.media.redBlasterShot;
			}
			break;
			case FACTION_REBEL:
			{
				bolt3D = cgs.media.blueBlasterShot;
			}
			break;
			case FACTION_MANDALORIAN:
			{
				bolt3D = cgs.media.greenBlasterShot;
			}
			break;
			case FACTION_MERC:
			{
				bolt3D = cgs.media.yellowBlasterShot;
			}
			break;
			case FACTION_PIRATES:
			{
				bolt3D = cgs.media.orangeBlasterShot;
			}
			break;
			default:
				continue;
				break;
			}

			VectorMA(laser->origin, 96.0, laser->dir, laser->origin);

			if (Distance(laser->origin, cg.refdef.vieworg) > MAX_SHIP_LASER_DRAW_DISTANCE)
			{
				laser->active = qfalse;
				return;
			}

			if (Distance(laser->origin, laser->endOrigin) <= 96.0)
			{// Explode and free this bolt...
				laser->active = qfalse;
				CG_SurfaceExplosion(laser->origin, laser->dir, 128.0, 0.5, qtrue);
				return;
			}

			if (bolt3D > 0)
			{// New 3D bolt enabled...
				FX_WeaponBolt3D(laser->origin, laser->dir, laser->size, laser->size, bolt3D);
			}
		}
	}
}

int CG_ShipLaserFindFreeSlot(void)
{
	for (int i = 0; i < MAX_SHIP_LASERS; i++)
	{
		shipLasers_t *laser = &shipLasers[i];

		if (!laser->active)
		{
			return i;
		}
	}

	return -1;
}

void CG_ShipLaserCreate(centity_t *myShip, centity_t *enemyShip)
{
	int laserSlotID = CG_ShipLaserFindFreeSlot();

	if (laserSlotID >= 0)
	{
		vec3_t forward, endOrigin, startOrigin, enemyOrigin, fwd;

//#define SHIP_HALF_SIZE 16384 // TODO: Send real sizes with the game entity... This is capital size...
#define MY_SHIP_HALF_SIZE int(16384.0 * (float(myShip->currentState.iModelScale) / 112.0))
#define ENEMY_SHIP_HALF_SIZE int(16384.0 * (float(enemyShip->currentState.iModelScale) / 112.0))
		
		// Assuming long ships, since we have no tags to use atm...
		AngleVectors(myShip->lerpAngles, NULL, fwd, NULL); // FIXME: ship models are rotated 90...
		if (irand(0,1) == 0)
			VectorMA(myShip->lerpOrigin, irand(0, MY_SHIP_HALF_SIZE), fwd, startOrigin);
		else
			VectorMA(myShip->lerpOrigin, -irand(0, MY_SHIP_HALF_SIZE), fwd, startOrigin);

		// Assuming long ships, since we have no tags to use atm...
		AngleVectors(enemyShip->lerpAngles, NULL, fwd, NULL); // FIXME: ship models are rotated 90...
		if (irand(0, 1) == 0)
			VectorMA(enemyShip->lerpOrigin, irand(0, ENEMY_SHIP_HALF_SIZE), fwd, enemyOrigin);
		else
			VectorMA(enemyShip->lerpOrigin, -irand(0, ENEMY_SHIP_HALF_SIZE), fwd, enemyOrigin);

		vec3_t offsetShot;
		offsetShot[0] = irand(0, 512) - 256;
		offsetShot[1] = irand(0, 512) - 256;
		offsetShot[2] = irand(0, 512) - 256;

		float boltAccuracy = 1.0 - (VectorLength(offsetShot) / 768.0);

		VectorSubtract(enemyOrigin, startOrigin, forward);
		forward[0] += offsetShot[0];
		forward[1] += offsetShot[1];
		forward[2] += offsetShot[2];
		VectorNormalize(forward);

		float dist = Distance(startOrigin, enemyOrigin);

		VectorMA(startOrigin, dist + ((1.0 - boltAccuracy) * 2048.0), forward, endOrigin);

		shipLasers_t *laser = &shipLasers[laserSlotID];
		VectorCopy(startOrigin, laser->origin);
		VectorCopy(endOrigin, laser->endOrigin);
		VectorCopy(forward, laser->dir);
		laser->team = myShip->currentState.teamowner;
		laser->size = 16.0;
		laser->active = qtrue;

		/*trap->Print("Ship shooting laser at another ship. Start %i %i %i. End %i %i %i. Dir %f %f %f.\n"
			, int(startOrigin[0]), int(startOrigin[1]), int(startOrigin[2])
			, int(endOrigin[0]), int(endOrigin[1]), int(endOrigin[2])
			, laser->dir[0], laser->dir[1], laser->dir[2]);*/
	}
}

void CG_ShootAtEnemyShips(centity_t *myShip)
{
	if (Distance(myShip->lerpOrigin, cg.refdef.vieworg) > MAX_SHIP_LASER_DRAW_DISTANCE)
	{
		return;
	}

#define SHIP_LASER_OUTPUT int(20.0 * (float(myShip->currentState.iModelScale) / 112.0))

	if (irand(0, 40 - SHIP_LASER_OUTPUT) != 0) return;

	centity_t *enemyShip = CG_FindEnemyShip(myShip);

	if (enemyShip != NULL)
	{
		CG_ShipLaserCreate(myShip, enemyShip);
	}
}

/*
===================
CG_ShipFighters

for warzone event fighters... A reworked version of birds...
===================
*/

#define FIGHTERS_MAX_DRAW_DISTANCE			131072.0
#define FIGHTERS_MAX_LASER_DRAW_DISTANCE	65536.0
#define FIGHTERS_MAX_CIRCLE_SIZE			512
#define FIGHTERS_CIRCLE_CHANGE_RATE			0.999
#define FIGHTERS_RETURN_HOME_DISTANCE		65536.0
#define FIGHTERS_RETURN_HOME_POWER			16.0
#define FIGHTER_ATTACK_RUN_DIR_CLOSENESS	0.5
#define FIGHTER_LASER_CHANCE				70
#define FIGHTER_CHASE_CHANCE				200
#define FIGHTER_CHASE_MAX_DISTANCE			12288.0

qboolean		eventFightersLoaded = qfalse;
qhandle_t		fighterModels[2] = { { -1 } };
qhandle_t		fighterSounds[2] = { { -1 } };

int				numMapFighters = 0;
mapFighters_t	*mapFighters[1024] = { { NULL } };

void CG_DrawFighter(mapFighters_t *fighter, qhandle_t fighterModel)
{
	float dist = Distance(fighter->origin, cg.refdef.vieworg);

	if (dist > FIGHTERS_MAX_DRAW_DISTANCE)
	{// Early cull...
		return;
	}

	refEntity_t ent;

	// Draw the bolt core...
	memset(&ent, 0, sizeof(refEntity_t));
	ent.reType = RT_MODEL;

	ent.modelScale[0] = 1.0;
	ent.modelScale[1] = 1.0;
	ent.modelScale[2] = 1.0;

	VectorCopy(fighter->origin, ent.origin);
	vectoangles(fighter->dir, ent.angles);
	AnglesToAxis(ent.angles, ent.axis);
	ScaleModelAxis(&ent);

	ent.hModel = fighterModel;

	AddRefEntityToScene(&ent);

	if (dist < 16384)
	{// Add sounds as well??? + 8192 because that marks local entities in the sound system for tracking...
		trap->S_AddLoopingSound(fighter->localSoundEntityNum + 8192, ent.origin, vec3_origin, fighter->loopSound, CHAN_CULLRANGE_16384);
	}
}

extern qboolean CG_InFOV(vec3_t spot, vec3_t from, vec3_t fromAngles, int hFOV, int vFOV);

void CG_Fighters(centity_t *myShip)
{
	if (!eventFightersLoaded)
	{
		fighterModels[0] = trap->R_RegisterModel("models/map_objects/ships/tie_fighter.md3"); // imp fighter model
		fighterSounds[0] = trap->S_RegisterSound("sound/vehicles/tie/loop.wav");

		fighterModels[1] = trap->R_RegisterModel("models/map_objects/ships/x_wing_nogear.md3"); // rebel fighter model
		fighterSounds[1] = trap->S_RegisterSound("sound/vehicles/x-wing/loop.wav");

		eventFightersLoaded = qtrue;
	}

	if (myShip->currentState.teamowner != FACTION_EMPIRE && myShip->currentState.teamowner != FACTION_REBEL)
	{// Team has no fighters (for now)...
		return;
	}

	int FIGHTERS_COUNT = 0;
	int ENEMY_FIGHTERS_COUNT = 0;

	if (float(myShip->currentState.iModelScale) / 112.0 > 0.9)
	{// Giant capital ships... (eg: Super Star Destroyers)...
		FIGHTERS_COUNT = 16;
		ENEMY_FIGHTERS_COUNT = 8;
	}
	else if (float(myShip->currentState.iModelScale) / 112.0 >= 0.5)
	{// Capital ships... (eg: Star Destoyers or Calamari Cruisers)...
		FIGHTERS_COUNT = 8;
		ENEMY_FIGHTERS_COUNT = 4;
	}

	if (FIGHTERS_COUNT <= 0)
	{// Ship is too small to have fighters...
		return;
	}

	if (myShip->numFighters <= 0)
	{
		memset(myShip->fighters, 0, sizeof(myShip->fighters));

		for (int i = 0; i < FIGHTERS_COUNT && i < 16; i++)
		{
			mapFighters_t *fighter = &myShip->fighters[i];

			// In random positions near the ship...
			fighter->origin[0] = myShip->lerpOrigin[0] + irand(0, 32768) - 16386;
			fighter->origin[1] = myShip->lerpOrigin[1] + irand(0, 32768) - 16386;
			fighter->origin[2] = myShip->lerpOrigin[2] - irand(4096, 8192);

			fighter->rotate = 1.0;

			if (irand(0, 1) == 1)
			{// This one flies in counter clockwise...
				fighter->rotate = -1.0;
			}

			fighter->circleSize = irand(1, FIGHTERS_MAX_CIRCLE_SIZE);

			float a = ((float(cg.time) / 1000.0) / fighter->circleSize + i) * fighter->rotate;
			float b = 0.0;
			fighter->dir[0] = cos(a) * cos(b);
			fighter->dir[1] = sin(a) * cos(b);
			fighter->dir[2] = 0.0;// sin(b);

			VectorCopy(fighter->origin, fighter->startOrigin);

			//trap->Print("Fighter %i spawn at %f %f %f. Dir %f.\n", i, fighter->origin[0], fighter->origin[1], fighter->origin[2], fighter->dir[YAW]);

			fighter->loopSound = fighterSounds[(myShip->currentState.teamowner == FACTION_EMPIRE) ? 0 : 1];
			
			fighter->team = myShip->currentState.teamowner;

			// Allocate a local sound entity number so sound system can track the sound...
			fighter->localSoundEntityNum = numLocalSoundEntities;
			numLocalSoundEntities++;

			myShip->numFighters++;

			// Add it to the global fighter list as well, so we can do fighter vs fighter weapon fx...
			mapFighters[numMapFighters] = fighter;
			numMapFighters++;
		}

		trap->Print("Event ship %i given %i fighter escorts...\n", myShip->currentState.number, FIGHTERS_COUNT);

		//
		// Also assign enemy attacking fighters... Hmm, maybe only when we are within range of an enemy capital (or shooting at one)?
		//
		memset(myShip->enemyFighters, 0, sizeof(myShip->enemyFighters));

		for (int i = 0; i < ENEMY_FIGHTERS_COUNT && i < 16; i++)
		{
			mapFighters_t *fighter = &myShip->enemyFighters[i];

			// In random positions near the ship...
			fighter->origin[0] = myShip->lerpOrigin[0] + irand(0, 32768) - 16386;
			fighter->origin[1] = myShip->lerpOrigin[1] + irand(0, 32768) - 16386;
			fighter->origin[2] = myShip->lerpOrigin[2] - irand(4096, 8192);

			fighter->rotate = 1.0;

			if (irand(0, 1) == 1)
			{// This one flies in counter clockwise...
				fighter->rotate = -1.0;
			}

			fighter->circleSize = irand(1, FIGHTERS_MAX_CIRCLE_SIZE);

			float a = ((float(cg.time) / 1000.0) / fighter->circleSize + i) * fighter->rotate;
			float b = 0.0;
			fighter->dir[0] = cos(a) * cos(b);
			fighter->dir[1] = sin(a) * cos(b);
			fighter->dir[2] = 0.0;// sin(b);

			VectorCopy(fighter->origin, fighter->startOrigin);

			//trap->Print("Enemy fighter %i spawn at %f %f %f. Dir %f.\n", i, fighter->origin[0], fighter->origin[1], fighter->origin[2], fighter->dir[YAW]);

			fighter->loopSound = fighterSounds[(myShip->currentState.teamowner == FACTION_EMPIRE) ? 1 : 0];

			fighter->team = (myShip->currentState.teamowner == FACTION_EMPIRE) ? FACTION_REBEL : FACTION_EMPIRE;

			// Allocate a local sound entity number so sound system can track the sound...
			fighter->localSoundEntityNum = numLocalSoundEntities;
			numLocalSoundEntities++;

			myShip->numEnemyFighters++;

			// Add it to the global fighter list as well, so we can do fighter vs fighter weapon fx...
			mapFighters[numMapFighters] = fighter;
			numMapFighters++;
		}

		trap->Print("Event ship %i given %i enemy fighter attackers...\n", myShip->currentState.number, ENEMY_FIGHTERS_COUNT);
	}


	//
	// Draw all escort fighters...
	//
	qhandle_t fighterModel = fighterModels[(myShip->currentState.teamowner == FACTION_EMPIRE) ? 0 : 1];

	if (fighterModel <= 0)
	{// In case asset is somehow missing...
		return;
	}

	// Update and draw escort fighters...
	for (int i = 0; i < myShip->numFighters && i < 16; i++)
	{
		mapFighters_t *fighter = &myShip->fighters[i];

		if (myShip->numEnemyFighters > 0 && fighter->chaseTime < cg.time && irand(0, FIGHTER_CHASE_CHANCE) == 0)
		{
			vec3_t myAngles;
			vectoangles(fighter->dir, myAngles);
			int closestTarget = -1;
			float closestDistance = FIGHTER_CHASE_MAX_DISTANCE;

			for (int f = 0; f < myShip->numEnemyFighters; f++)
			{// Hmm, mapFighters may not be required now that I assigned enemy fighters to each ship as well...
				mapFighters_t *thisFighter = &myShip->enemyFighters[f];

				float enemyDist = Distance(thisFighter->origin, fighter->origin);

				if (enemyDist < closestDistance && CG_InFOV(thisFighter->origin, fighter->origin, myAngles, 120, 120))
				{
					closestTarget = f;
					closestDistance = enemyDist;
				}
			}

			if (closestTarget != -1)
			{
				fighter->chaseTime = cg.time + 10000 + irand(5000, 15000); // 15->25 secs?
				fighter->chaseTarget = closestTarget;
			}
		}

		if (fighter->chaseTime > cg.time && fighter->chaseTarget)
		{// Chase the enemy for a while...
			mapFighters_t *wantedEnemy = &myShip->enemyFighters[fighter->chaseTarget];

			vec3_t wantedDir, enemyOrg;

			// Adjust the target direction, and slow the following fighter down as they get close...
			float wantedDist = Distance(wantedEnemy->origin, fighter->origin);
			float wantedOffsetStrength = Q_clamp(0.0, wantedDist / 8192.0, 1.0);
			float wantedSpeedStrength = Q_clamp(0.0, wantedDist / 2048.0, 1.0);

			// Randomize target direction a bit, so fighters don't get too close, and don't go through eachother too often...
			VectorSubtract(wantedEnemy->origin, fighter->origin, wantedDir);
			wantedDir[0] += (irand(0, 256) - 128) * wantedOffsetStrength;
			wantedDir[1] += (irand(0, 256) - 128) * wantedOffsetStrength;
			wantedDir[2] += (irand(0, 256) - 128) * wantedOffsetStrength;
			VectorNormalize(wantedDir);

			float wantedStrength = Q_clamp(0.0, 0.75, 1.0);
			fighter->dir[0] = mix(fighter->dir[0], wantedDir[0], wantedStrength);
			fighter->dir[1] = mix(fighter->dir[1], wantedDir[1], wantedStrength);
			fighter->dir[2] = mix(fighter->dir[2], wantedDir[2], wantedStrength);

			VectorMA(fighter->origin, mix(64.0, 72.0, wantedSpeedStrength), fighter->dir, fighter->origin);
		}
		else
		{// Randomly fly around the event capital ship...
			fighter->chaseTarget = 0;
			fighter->chaseTime = 0;

			// Randomly adjust the fighter's circle size over time...
			if (irand(0, 1) == 1)
			{
				fighter->circleSize += FIGHTERS_CIRCLE_CHANGE_RATE;
			}
			else
			{
				fighter->circleSize -= FIGHTERS_CIRCLE_CHANGE_RATE;
			}

			fighter->circleSize = Q_clampi(1, fighter->circleSize, FIGHTERS_MAX_CIRCLE_SIZE);

			if (irand(0, 100) == 0)
			{// Randomly switch rotation directions...
				fighter->rotate *= -1.0;
			}

			float a = ((float(cg.time) / 1000.0) / fighter->circleSize + i) * fighter->rotate;
			float b = 1000.0;
			vec3_t newdir;
			newdir[0] = cos(a) * cos(b);
			newdir[1] = sin(a) * cos(b);
			newdir[2] = 0.0;// sin(b);

			// Adjust direction over time to compensate for random rotation direction switching...
			fighter->dir[0] = mix(fighter->dir[0], newdir[0], 0.01);
			fighter->dir[1] = mix(fighter->dir[1], newdir[1], 0.01);
			fighter->dir[2] = 0.0;
			VectorNormalize(fighter->dir);

			float distFromStart = DistanceHorizontal(fighter->origin, fighter->startOrigin);

			// Stay near our start position...
			vec3_t wantedDir;
			VectorSubtract(fighter->startOrigin, fighter->origin, wantedDir);
			VectorNormalize(wantedDir);

			float returnStrength = pow(Q_clamp(0.0, distFromStart / FIGHTERS_RETURN_HOME_DISTANCE, 1.0), FIGHTERS_RETURN_HOME_POWER);

			fighter->dir[0] = mix(fighter->dir[0], wantedDir[0], returnStrength);
			fighter->dir[1] = mix(fighter->dir[1], wantedDir[1], returnStrength);
			fighter->dir[2] = mix(fighter->dir[2], wantedDir[2], returnStrength); //0.0;// sin(b);
			VectorNormalize(fighter->dir);

			VectorMA(fighter->origin, 72.0, fighter->dir, fighter->origin);
		}

		CG_DrawFighter(fighter, fighterModel);

		if (Distance(fighter->origin, cg.refdef.vieworg) < FIGHTERS_MAX_LASER_DRAW_DISTANCE && irand(0, FIGHTER_LASER_CHANCE) == 0)
		{// Check for an enemy fighter or cap ship to shoot at...
			qboolean	shouldFire = qfalse;
			qboolean	fighterTarget = qfalse;
			vec3_t		enemyPos;
			float		enemyDistance;
			float		enemyDirDistance;

			for (int f = 0; f < numMapFighters; f++)
			{// Hmm, mapFighters may not be required now that I assigned enemy fighters to each ship as well...
				mapFighters_t *thisFighter = mapFighters[f];

				if (thisFighter->team != fighter->team)
				{// An enemy fighter, check distance...
					float enemyDist = Distance(thisFighter->origin, fighter->origin);

					if (enemyDist < 8192.0)
					{// Within shooting range, check direction...
						vec3_t fighterDir;
						VectorSubtract(thisFighter->origin, fighter->origin, fighterDir);
						VectorNormalize(fighterDir);

						float directionCloseness = Distance(fighter->dir, fighterDir);

						if (directionCloseness <= FIGHTER_ATTACK_RUN_DIR_CLOSENESS)
						{
							shouldFire = qtrue;
							fighterTarget = qtrue;
							VectorCopy(thisFighter->origin, enemyPos);
							enemyDistance = enemyDist;
							enemyDirDistance = (directionCloseness / FIGHTER_ATTACK_RUN_DIR_CLOSENESS);
							break;
						}
					}
				}
			}

			if (shouldFire)
			{
				int laserSlotID = CG_ShipLaserFindFreeSlot();

				if (laserSlotID >= 0)
				{
					vec3_t endOrigin;
					float distanceModifier = 1.0 - Q_clamp(0.0, enemyDistance / 8192.0, 1.0);
					
					if (!fighterTarget && distanceModifier <= 0.01 && enemyDirDistance >= 0.01)
					{// Probably gonna actually hit them, so set exact enemy pos for bolt end point...
						VectorCopy(enemyPos, endOrigin);
					}
					else
					{
						VectorMA(fighter->origin, enemyDistance * 4.0, fighter->dir, endOrigin);
					}

					shipLasers_t *laser = &shipLasers[laserSlotID];
					VectorCopy(fighter->origin, laser->origin);
					VectorCopy(endOrigin, laser->endOrigin);
					VectorCopy(fighter->dir, laser->dir);
					laser->team = myShip->currentState.teamowner;
					laser->size = 8.0;
					laser->active = qtrue;
				}
			}
		}
	}

	//
	// Draw all enemy fighters...
	//
	fighterModel = fighterModels[(myShip->currentState.teamowner == FACTION_EMPIRE) ? 1 : 0];

	if (fighterModel <= 0)
	{// In case asset is somehow missing...
		return;
	}

	// Update and draw fighters...
	for (int i = 0; i < myShip->numEnemyFighters && i < 8; i++)
	{
		mapFighters_t *fighter = &myShip->enemyFighters[i];

		// Randomly adjust the fighter's circle size over time...
		if (irand(0, 1) == 1)
		{
			fighter->circleSize += FIGHTERS_CIRCLE_CHANGE_RATE;
		}
		else
		{
			fighter->circleSize -= FIGHTERS_CIRCLE_CHANGE_RATE;
		}

		fighter->circleSize = Q_clampi(1, fighter->circleSize, FIGHTERS_MAX_CIRCLE_SIZE);

		if (irand(0, 100) == 0)
		{// Randomly switch rotation directions...
			fighter->rotate *= -1.0;
		}

		float a = ((float(cg.time) / 1000.0) / fighter->circleSize + i) * fighter->rotate;
		float b = 1000.0;
		vec3_t newdir;
		newdir[0] = cos(a) * cos(b);
		newdir[1] = sin(a) * cos(b);
		newdir[2] = 0.0;// sin(b);

		// Always move back toward our start height (for after attack runs)...
		vec3_t homeDir;
		VectorSubtract(fighter->startOrigin, fighter->origin, homeDir);
		VectorNormalize(homeDir);

		// Adjust direction over time to compensate for random rotation direction switching...
		fighter->dir[0] = mix(fighter->dir[0], newdir[0], 0.01);
		fighter->dir[1] = mix(fighter->dir[1], newdir[1], 0.01);
		fighter->dir[2] = mix(fighter->dir[2], homeDir[2], 0.01);
		VectorNormalize(fighter->dir);

		float distFromStart = DistanceHorizontal(fighter->origin, fighter->startOrigin);

		// Stay near our start position...
		vec3_t wantedDir;
		VectorSubtract(fighter->startOrigin, fighter->origin, wantedDir);
		VectorNormalize(wantedDir);

		float returnStrength = pow(Q_clamp(0.0, distFromStart / FIGHTERS_RETURN_HOME_DISTANCE, 1.0), FIGHTERS_RETURN_HOME_POWER);

		fighter->dir[0] = mix(fighter->dir[0], wantedDir[0], returnStrength);
		fighter->dir[1] = mix(fighter->dir[1], wantedDir[1], returnStrength);
		fighter->dir[2] = mix(fighter->dir[2], wantedDir[2], returnStrength); //0.0;// sin(b);
		VectorNormalize(fighter->dir);


		float capitalDistance = Distance(myShip->lerpOrigin, fighter->origin);

		if (capitalDistance >= 12288.0 && capitalDistance <= 32768.0)
		{// Check if we should move the fighter's direction up/down toward the cap ship to do an attack run...
			vec3_t upDownDir, enemyUpDownDir, attackRunDir;
			VectorCopy(fighter->dir, upDownDir);
			upDownDir[2] = 0;

			VectorSubtract(myShip->lerpOrigin, fighter->origin, enemyUpDownDir);
			VectorNormalize(enemyUpDownDir);
			VectorCopy(enemyUpDownDir, attackRunDir);
			enemyUpDownDir[2] = 0;

			float directionCloseness = Distance(upDownDir, enemyUpDownDir);

			if (directionCloseness <= FIGHTER_ATTACK_RUN_DIR_CLOSENESS)
			{// Seems we are pointing roughly at the capital ship, and in range, so adjust angles for the attack run!
				directionCloseness = 1.0 - (directionCloseness / FIGHTER_ATTACK_RUN_DIR_CLOSENESS);
				//trap->Print("Fighter %i attack run directionCloseness %f. Distance %f.\n", fighter->localSoundEntityNum, directionCloseness, capitalDistance);
				fighter->dir[0] = mix(fighter->dir[0], attackRunDir[0], directionCloseness);
				fighter->dir[1] = mix(fighter->dir[1], attackRunDir[1], directionCloseness);
				fighter->dir[2] = mix(fighter->dir[2], attackRunDir[2], directionCloseness);
				VectorNormalize(fighter->dir);
			}
		}

		VectorMA(fighter->origin, 72.0, fighter->dir, fighter->origin);

		CG_DrawFighter(fighter, fighterModel);

		if (Distance(fighter->origin, cg.refdef.vieworg) < FIGHTERS_MAX_LASER_DRAW_DISTANCE && irand(0, FIGHTER_LASER_CHANCE) == 0)
		{// Check for an enemy fighter or cap ship to shoot at...
			qboolean	shouldFire = qfalse;
			qboolean	fighterTarget = qfalse;
			vec3_t		enemyPos;
			float		enemyDistance;
			float		enemyDirDistance;

			if (capitalDistance <= 24576.0)
			{// First check if we should fire at the capital ship...
				vec3_t capitalDir;
				VectorSubtract(myShip->lerpOrigin, fighter->origin, capitalDir);
				VectorNormalize(capitalDir);

				float directionCloseness = Distance(fighter->dir, capitalDir);

				if (directionCloseness <= FIGHTER_ATTACK_RUN_DIR_CLOSENESS)
				{// Seems we are pointing roughly at the capital ship, and in range, so shoot away!
					shouldFire = qtrue;
					VectorCopy(myShip->lerpOrigin, enemyPos);
					enemyDistance = capitalDistance;
					enemyDirDistance = (directionCloseness / FIGHTER_ATTACK_RUN_DIR_CLOSENESS);
				}
			}
			
			if (!shouldFire)
			{
				for (int f = 0; f < numMapFighters; f++)
				{// Hmm, mapFighters may not be required now that I assigned enemy fighters to each ship as well...
					mapFighters_t *thisFighter = mapFighters[f];

					if (thisFighter->team != fighter->team)
					{// An enemy fighter, check distance...
						float enemyDist = Distance(thisFighter->origin, fighter->origin);

						if (enemyDist < 8192.0)
						{// Within shooting range, check direction...
							vec3_t fighterDir;
							VectorSubtract(thisFighter->origin, fighter->origin, fighterDir);
							VectorNormalize(fighterDir);

							float directionCloseness = Distance(fighter->dir, fighterDir);

							if (directionCloseness <= FIGHTER_ATTACK_RUN_DIR_CLOSENESS)
							{
								shouldFire = qtrue;
								fighterTarget = qtrue;
								VectorCopy(thisFighter->origin, enemyPos);
								enemyDistance = enemyDist;
								enemyDirDistance = (directionCloseness / FIGHTER_ATTACK_RUN_DIR_CLOSENESS);
								break;
							}
						}
					}
				}
			}

			if (shouldFire)
			{
				int laserSlotID = CG_ShipLaserFindFreeSlot();

				if (laserSlotID >= 0)
				{
					vec3_t endOrigin;
					float distanceModifier = 1.0 - Q_clamp(0.0, enemyDistance / 8192.0, 1.0);
					
					if (!fighterTarget && distanceModifier <= 0.01 && enemyDirDistance >= 0.01)
					{// Probably gonna actually hit them, so set exact enemy pos for bolt end point...
						VectorCopy(enemyPos, endOrigin);
					}
					else
					{
						VectorMA(fighter->origin, enemyDistance * 4.0, fighter->dir, endOrigin);
					}

					shipLasers_t *laser = &shipLasers[laserSlotID];
					VectorCopy(fighter->origin, laser->origin);
					VectorCopy(endOrigin, laser->endOrigin);
					VectorCopy(fighter->dir, laser->dir);
					laser->team = fighter->team;
					laser->size = 8.0;
					laser->active = qtrue;
				}
			}
		}
	}
}

/*
===================
CG_ForceField*

for warzone force field testing...
===================
*/


#define MAX_FORCEFIELDS 256

typedef struct forcefield_s {
	qboolean	active;
	vec3_t		origin;
	vec3_t		radius;
	int			team;
	int			health;
	qboolean	isTown;
} forcefield_t;

forcefield_t	forcefields[MAX_FORCEFIELDS];

qhandle_t forcefieldModel = -1;
qhandle_t forcefieldShader = -1;

void CG_ForcefieldDraw(vec3_t origin, vec3_t radius)
{
	if (forcefieldModel == -1)
	{
		forcefieldModel = trap->R_RegisterModel("models/warzone/shield.md3");
		forcefieldShader = trap->R_RegisterShader("models/warzone/shield");
	}

	refEntity_t ent;

	// Draw the bolt core...
	memset(&ent, 0, sizeof(refEntity_t));
	ent.reType = RT_MODEL;

	vec3_t scale;
	scale[0] = (radius[0] / 128.0); // full size of sphere model is 64.
	scale[1] = (radius[1] / 128.0); // full size of sphere model is 64.
	scale[2] = (radius[2] / 128.0); // full size of sphere model is 64.
	ent.modelScale[0] = scale[0];
	ent.modelScale[1] = scale[1];
	ent.modelScale[2] = scale[2];

	VectorCopy(origin, ent.origin);
	VectorSet(ent.angles, 0, 0, 0);
	AnglesToAxis(ent.angles, ent.axis);
	ScaleModelAxis(&ent);

	ent.customShader = forcefieldShader;

	ent.hModel = forcefieldModel;

	AddRefEntityToScene(&ent);
}

// This stuff will need to be moved to a game entity at some point, but for now it's just visuals...
int CG_ForcefieldFindFreeSlot(void)
{
	for (int i = 0; i < MAX_FORCEFIELDS; i++)
	{
		forcefield_t *forcefield = &forcefields[i];

		if (!forcefield->active)
		{
			return i;
		}
	}

	return -1;
}

void CG_ForcefieldDropTownShield(void)
{
	for (int i = 0; i < MAX_FORCEFIELDS; i++)
	{
		forcefield_t *forcefield = &forcefields[i];

		if (forcefield->active && forcefield->isTown)
		{
			forcefield->active = qfalse;
			forcefield->isTown = qfalse;
		}
	}
}

void CG_ForcefieldCreate(vec3_t origin, vec3_t radius, int team, int health, qboolean isTown)
{
	int id = CG_ForcefieldFindFreeSlot();

	if (id >= 0)
	{
		forcefield_t *forcefield = &forcefields[id];
		forcefield->active = qtrue;
		VectorCopy(origin, forcefield->origin);
		VectorCopy(radius, forcefield->radius);
		forcefield->health = health;
		forcefield->isTown = isTown;
	}
}

void CG_ForcefieldThink(void)
{
	for (int i = 0; i < MAX_FORCEFIELDS; i++)
	{
		forcefield_t *forcefield = &forcefields[i];

		if (forcefield->active)
		{
			if (forcefield->health > -999990.0 && forcefield->health <= 0)
			{// Collapsed... -99999.9 marks infinite strengths....
				forcefield->active = qfalse;
				forcefield->isTown = qfalse;

				/*trap->Print("Forcefield dropped at %i %i %i radius %i %i %i. health %f.\n"
					, int(forcefield->origin[0]), int(forcefield->origin[1]), int(forcefield->origin[2])
					, int(forcefield->radius[0]), int(forcefield->radius[1]), int(forcefield->radius[2])
					, int(forcefield->health));*/
			}
			else
			{
				CG_ForcefieldDraw(forcefield->origin, forcefield->radius);
				
				/*trap->Print("Forcefield drawn at %i %i %i radius %i %i %i. health %f.\n"
					, int(forcefield->origin[0]), int(forcefield->origin[1]), int(forcefield->origin[2])
					, int(forcefield->radius[0]), int(forcefield->radius[1]), int(forcefield->radius[2])
					, int(forcefield->health));*/
			}
		}
	}
}

//==============================================================================

/*
===================
CG_AddLocalEntities

===================
*/
void CG_AddLocalEntities( void ) {
	localEntity_t	*le, *next;

	// walk the list backwards, so any new local entities generated
	// (trails, marks, etc) will be present this frame
	le = cg_activeLocalEntities.prev;
	for ( ; le != &cg_activeLocalEntities ; le = next ) {
		// grab next now, so if the local entity is freed we
		// still have it
		next = le->prev;

		if ( cg.time >= le->endTime ) {
			CG_FreeLocalEntity( le );
			continue;
		}
		switch ( le->leType ) {
		default:
			trap->Error( ERR_DROP, "Bad leType: %i", le->leType );
			break;

		case LE_MARK:
			break;

		case LE_SPRITE_EXPLOSION:
			CG_AddSpriteExplosion( le );
			break;

		case LE_EXPLOSION:
			CG_AddExplosion( le );
			break;

		case LE_FADE_SCALE_MODEL:
			CG_AddFadeScaleModel( le );
			break;

		case LE_FRAGMENT:			// gibs and brass
			CG_AddFragment( le );
			break;

		case LE_PUFF:
			CG_AddPuff( le );
			break;

		case LE_MOVE_SCALE_FADE:		// water bubbles
			CG_AddMoveScaleFade( le );
			break;

		case LE_FADE_RGB:				// teleporters, railtrails
			CG_AddFadeRGB( le );
			break;

		case LE_FALL_SCALE_FADE: // gib blood trails
			CG_AddFallScaleFade( le );
			break;

		case LE_SCALE_FADE:		// rocket trails
			CG_AddScaleFade( le );
			break;

		case LE_SCOREPLUM:
			CG_AddScorePlum( le );
			break;

		case LE_OLINE:
			CG_AddOLine( le );
			break;

		case LE_SHOWREFENTITY:
			CG_AddRefEntity( le );
			break;

		case LE_LINE:					// oriented lines for FX
			CG_AddLine( le );
			break;
		}
	}

	// Warzone flying birds...
	CG_Birds();

	// Warzone event ships shooting at each other...
	CG_ShipLasersThink();

	// Warzone shields/forcefields...
	CG_ForcefieldThink();
}




