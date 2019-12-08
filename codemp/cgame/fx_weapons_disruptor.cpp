// Disruptor Weapon

#include "cg_local.h"
#include "fx_local.h"

extern void FX_WeaponBolt3D(vec3_t org, vec3_t fwd, float length, float radius, qhandle_t shader, qboolean addLight);
extern qboolean CG_CalcMuzzlePoint(int entityNum, vec3_t muzzle);

/*
---------------------------
FX_DisruptorMainShot
---------------------------
*/
static vec3_t WHITE={1.0f,1.0f,1.0f};

void FX_DisruptorMainShot( centity_t *cent, vec3_t start, vec3_t end )
{
#if 0
//	vec3_t	dir;
//	float	len;

	trap->FX_AddLine( start, end, 0.1f, 6.0f, 0.0f,
							1.0f, 0.0f, 0.0f,
							WHITE, WHITE, 0.0f,
							150, trap->R_RegisterShader( "gfx/effects/redLine" ),
							FX_SIZE_LINEAR | FX_ALPHA_LINEAR );

//	VectorSubtract( end, start, dir );
//	len = VectorNormalize( dir );

//	FX_AddCylinder( start, dir, 5.0f, 5.0f, 0.0f,
//								5.0f, 5.0f, 0.0f,
//								len, len, 0.0f,
//								1.0f, 1.0f, 0.0f,
//								WHITE, WHITE, 0.0f,
//								400, cgi_R_RegisterShader( "gfx/effects/spiral" ), 0 );
#else
	vec3_t forward, midPoint;

	float dist = Distance(start, end);
	VectorSubtract(end, start, forward);
	VectorNormalize(forward);

	midPoint[0] = (start[0] + end[0]) * 0.5;
	midPoint[1] = (start[1] + end[1]) * 0.5;
	midPoint[2] = (start[2] + end[2]) * 0.5;

	qhandle_t bolt3D = 0;

	switch (cent->currentState.temporaryWeapon)
	{// Were we sent a specific color to use???
	case ITEM_CRYSTAL_RED:				// Bonus Heat Damage/Resistance
		bolt3D = cgs.media.redBlasterShot;
		break;
	case ITEM_CRYSTAL_GREEN:				// Bonus Kinetic (force) Damage/Resistance
		bolt3D = cgs.media.greenBlasterShot;
		break;
	case ITEM_CRYSTAL_BLUE:				// Bonus Electric Damage/Resistance
		bolt3D = cgs.media.blueBlasterShot;
		break;
	case ITEM_CRYSTAL_WHITE:				// Bonus Cold Damage/Resistance
		bolt3D = cgs.media.whiteBlasterShot;
		break;
	case ITEM_CRYSTAL_YELLOW:			// Bonus 1/2 Heat + 1/2 Kinetic Damage/Resistance
		bolt3D = cgs.media.yellowBlasterShot;
		break;
	case ITEM_CRYSTAL_PURPLE:			// Bonus 1/2 Electric + 1/2 Heat Damage/Resistance
		bolt3D = cgs.media.PurpleBlasterShot;
		break;
	case ITEM_CRYSTAL_ORANGE:			// Bonus 1/2 Cold + 1/2 Kinetic Damage/Resistance
		bolt3D = cgs.media.orangeBlasterShot;
		break;
	case ITEM_CRYSTAL_PINK:				// Bonus 1/2 Electric + 1/2 Cold Damage/Resistance
		bolt3D = cgs.media.BluePurpleBlasterShot; // TODO: Add actual pink...
		break;
	case ITEM_CRYSTAL_DEFAULT:			// GREY shots/blade? No special damage/resistance type...
	default:
		// Nope...
		bolt3D = cgs.media.whiteBlasterShot;
		break;
	}

	//trap->Print("bolt id is %i.\n", bolt3D);

	if (bolt3D > 0)
	{// New 3D bolt enabled...
		FX_WeaponBolt3D(cent->lerpOrigin, forward, dist / 16.47f, 1.0, bolt3D, qtrue);
	}
#endif
}


/*
---------------------------
FX_DisruptorAltShot
---------------------------
*/
void FX_DisruptorAltShot( centity_t *cent, vec3_t start, vec3_t end, qboolean fullCharge )
{
#if 0
	trap->FX_AddLine( start, end, 0.1f, 10.0f, 0.0f,
							1.0f, 0.0f, 0.0f,
							WHITE, WHITE, 0.0f,
							175, trap->R_RegisterShader( "gfx/effects/redLine" ),
							FX_SIZE_LINEAR | FX_ALPHA_LINEAR );

	if ( fullCharge )
	{
		vec3_t	YELLER={0.8f,0.7f,0.0f};

		// add some beef
		trap->FX_AddLine( start, end, 0.1f, 7.0f, 0.0f,
							1.0f, 0.0f, 0.0f,
							YELLER, YELLER, 0.0f,
							150, trap->R_RegisterShader( "gfx/misc/whiteline2" ),
							FX_SIZE_LINEAR | FX_ALPHA_LINEAR );
	}
#else
	vec3_t forward, midPoint;

	float dist = Distance(start, end);
	VectorSubtract(end, start, forward);
	VectorNormalize(forward);

	midPoint[0] = (start[0] + end[0]) * 0.5;
	midPoint[1] = (start[1] + end[1]) * 0.5;
	midPoint[2] = (start[2] + end[2]) * 0.5;

	qhandle_t bolt3D = 0;

	switch (cent->currentState.temporaryWeapon)
	{// Were we sent a specific color to use???
	case ITEM_CRYSTAL_RED:				// Bonus Heat Damage/Resistance
		bolt3D = cgs.media.redBlasterShot;
		break;
	case ITEM_CRYSTAL_GREEN:				// Bonus Kinetic (force) Damage/Resistance
		bolt3D = cgs.media.greenBlasterShot;
		break;
	case ITEM_CRYSTAL_BLUE:				// Bonus Electric Damage/Resistance
		bolt3D = cgs.media.blueBlasterShot;
		break;
	case ITEM_CRYSTAL_WHITE:				// Bonus Cold Damage/Resistance
		bolt3D = cgs.media.whiteBlasterShot;
		break;
	case ITEM_CRYSTAL_YELLOW:			// Bonus 1/2 Heat + 1/2 Kinetic Damage/Resistance
		bolt3D = cgs.media.yellowBlasterShot;
		break;
	case ITEM_CRYSTAL_PURPLE:			// Bonus 1/2 Electric + 1/2 Heat Damage/Resistance
		bolt3D = cgs.media.PurpleBlasterShot;
		break;
	case ITEM_CRYSTAL_ORANGE:			// Bonus 1/2 Cold + 1/2 Kinetic Damage/Resistance
		bolt3D = cgs.media.orangeBlasterShot;
		break;
	case ITEM_CRYSTAL_PINK:				// Bonus 1/2 Electric + 1/2 Cold Damage/Resistance
		bolt3D = cgs.media.BluePurpleBlasterShot; // TODO: Add actual pink...
		break;
	case ITEM_CRYSTAL_DEFAULT:			// GREY shots/blade? No special damage/resistance type...
	default:
		// Nope...
		bolt3D = cgs.media.whiteBlasterShot;
		break;
	}

	if (bolt3D > 0)
	{// New 3D bolt enabled...
		FX_WeaponBolt3D(cent->lerpOrigin, forward, dist / 16.47f, 1.25, bolt3D, qtrue);
	}
#endif
}


/*
---------------------------
FX_DisruptorAltMiss
---------------------------
*/
#define FX_ALPHA_WAVE		0x00000008

void FX_DisruptorAltMiss( vec3_t origin, vec3_t normal, int weapon, qboolean altFire )
{// do this later
	vec3_t pos, c1, c2;
	addbezierArgStruct_t b;

	VectorMA( origin, 4.0f, normal, c1 );
	VectorCopy( c1, c2 );
	c1[2] += 4;
	c2[2] += 12;

	VectorAdd( origin, normal, pos );
	pos[2] += 28;

	/*
	FX_AddBezier( origin, pos, c1, vec3_origin, c2, vec3_origin, 6.0f, 6.0f, 0.0f, 0.0f, 0.2f, 0.5f,
	WHITE, WHITE, 0.0f, 4000, trap->R_RegisterShader( "gfx/effects/smokeTrail" ), FX_ALPHA_WAVE );
	*/

	VectorCopy(origin, b.start);
	VectorCopy(pos, b.end);
	VectorCopy(c1, b.control1);
	VectorCopy(vec3_origin, b.control1Vel);
	VectorCopy(c2, b.control2);
	VectorCopy(vec3_origin, b.control2Vel);

	b.size1 = 6.0f;
	b.size2 = 6.0f;
	b.sizeParm = 0.0f;
	b.alpha1 = 0.0f;
	b.alpha2 = 0.2f;
	b.alphaParm = 0.5f;

	VectorCopy(WHITE, b.sRGB);
	VectorCopy(WHITE, b.eRGB);

	b.rgbParm = 0.0f;
	b.killTime = 4000;
	b.shader = trap->R_RegisterShader( "gfx/effects/smokeTrail" );
	b.flags = FX_ALPHA_WAVE;

	trap->FX_AddBezier(&b);

	PlayEffectID( cgs.effects.disruptorAltMissEffect, origin, normal, -1, -1, qfalse );
}

/*
---------------------------
FX_DisruptorAltHit
---------------------------
*/

void FX_DisruptorAltHit( vec3_t origin, vec3_t normal, int weapon, qboolean altFire )
{
	fxHandle_t fx = cg_weapons[weapon].altMissileWallImpactfx;

	if (fx)
		PlayEffectID(fx, origin, normal, -1, -1, qfalse);
}



/*
---------------------------
FX_DisruptorHitWall
---------------------------
*/

void FX_DisruptorHitWall(vec3_t origin, vec3_t normal, int weapon, qboolean altFire)
{
	fxHandle_t fx = cg_weapons[weapon].missileWallImpactfx;
	if (altFire) fx = cg_weapons[weapon].altMissileWallImpactfx;

	if (fx)
		PlayEffectID(fx, origin, normal, -1, -1, qfalse);
}

/*
---------------------------
FX_DisruptorHitPlayer
---------------------------
*/

void FX_DisruptorHitPlayer(vec3_t origin, vec3_t normal, qboolean humanoid, int weapon, qboolean altFire)
{
	fxHandle_t fx = cg_weapons[weapon].fleshImpactEffect;
	if (altFire) fx = cg_weapons[weapon].altFleshImpactEffect;

	if (fx)
		PlayEffectID(fx, origin, normal, -1, -1, qfalse);
}
