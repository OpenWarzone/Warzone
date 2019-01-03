// DEMP2 Weapon

#include "cg_local.h"
#include "fx_local.h"

/*
---------------------------
FX_Clonepistol_ProjectileThink
---------------------------
*/
void FX_Clonepistol_ProjectileThink(centity_t *cent, const struct weaponInfo_s *weapon)
{
	vec3_t forward;
	int t;

	if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
	{
		forward[2] = 1.0f;
	}

	if (cent->currentState.generic1 == 6)
	{
		for (t = 1; t < (cent->currentState.generic1 - 1); t++)
		{
			PlayEffectID(weapon->missileRenderfx, cent->lerpOrigin, forward, -1, -1, qfalse);
		}
	}
	else
	{
		if (weapon->missileRenderfx)
		{
			PlayEffectID(weapon->missileRenderfx, cent->lerpOrigin, forward, -1, -1, qfalse);
		}
	}

	//AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1.0f, 1.0f, 1.0f );
}

/*
---------------------------
FX_Clonepistol_HitWall
---------------------------
*/
//i added this stuff here under but
void FX_Clonepistol_HitWall(vec3_t origin, vec3_t normal, int weapon, qboolean altFire)
{

	// Set fx to primary weapon fx.
	fxHandle_t fx = cg_weapons[weapon].missileWallImpactfx;

	if (!fx) {
		return;
	}

	if (altFire) {
		// If this is alt fire. Override all fx with alt fire fx...
		if (cg_weapons[weapon].altMissileWallImpactfx)
		{// We have alt fx for this weapon. Use it.
			fx = cg_weapons[weapon].altMissileWallImpactfx;
		}
	}

	if (fx)
	{// We have fx for this. Play it.
		PlayEffectID(fx, origin, normal, -1, -1, qfalse);
	}
}

/*
---------------------------
FX_Clonepistol_BounceWall
---------------------------
*///its like it is the same here just call with some othere cmd
void FX_Clonepistol_BounceWall(vec3_t origin, vec3_t normal, int weapon, qboolean altFire)
{

	// Set fx to primary weapon fx.
	fxHandle_t fx = cg_weapons[weapon].WallBounceEffectFX;

	if (!fx) {
		return;
	}

	if (altFire) {
		// If this is alt fire. Override all fx with alt fire fx...
		if (cg_weapons[weapon].altWallBounceEffectFX)
		{// We have alt fx for this weapon. Use it.
			fx = cg_weapons[weapon].altWallBounceEffectFX;
		}
	}

	if (fx)
	{// We have fx for this. Play it.
		PlayEffectID(fx, origin, normal, -1, -1, qfalse);
	}
}



/*
---------------------------
FX_Clonepistol_HitPlayer
---------------------------
*/

void FX_Clonepistol_HitPlayer(vec3_t origin, vec3_t normal, qboolean humanoid, int weapon, qboolean altFire)
{
	// Set fx to primary weapon fx.
	fxHandle_t fx = cg_weapons[weapon].fleshImpactEffect;

	if (!fx) {
		return;
	}

	if (altFire) {
		// If this is alt fire. Override all fx with alt fire fx...
		if (cg_weapons[weapon].altFleshImpactEffect)
		{// We have alt fx for this weapon. Use it.
			fx = cg_weapons[weapon].altFleshImpactEffect;
		}
	}

	if (fx)
	{// We have fx for this. Play it.
		PlayEffectID(fx, origin, normal, -1, -1, qfalse);
	}
}

/*
---------------------------
FX_DEMP2_ProjectileThink
---------------------------
*/

void FX_DEMP2_ProjectileThink(centity_t *cent, const struct weaponInfo_s *weapon)
{
	vec3_t forward;
	int t;

	if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
	{
		forward[2] = 1.0f;
	}

	if (cent->currentState.generic1 == 6)
	{
		for (t = 1; t < (cent->currentState.generic1 - 1); t++) 
		{
			PlayEffectID(weapon->missileRenderfx, cent->lerpOrigin, forward, -1, -1, qfalse);
		}
	}
	else
	{
		if (weapon->missileRenderfx)
		{
			PlayEffectID(weapon->missileRenderfx, cent->lerpOrigin, forward, -1, -1, qfalse);
		}
	}

	//AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1.0f, 1.0f, 1.0f );
}

void FX_DEMP2_AltProjectileThink(centity_t *cent, const struct weaponInfo_s *weapon)
{
	vec3_t forward;
	int t;

	if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
	{
		forward[2] = 1.0f;
	}

	if (cent->currentState.generic1 == 6)
	{
		for (t = 1; t < (cent->currentState.generic1 - 1); t++)
		{
			PlayEffectID(weapon->altMissileRenderfx, cent->lerpOrigin, forward, -1, -1, qfalse);
		}
	}
	else
	{
		if (weapon->altMissileRenderfx)
		{
			PlayEffectID(weapon->altMissileRenderfx, cent->lerpOrigin, forward, -1, -1, qfalse);
		}
	}

	//AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1.0f, 1.0f, 1.0f );
}

/*
---------------------------
FX_DEMP2_HitWall
---------------------------
*/

void FX_DEMP2_HitWall(vec3_t origin, vec3_t normal, int weapon, qboolean altFire)
{
	// Set fx to primary weapon fx.
	fxHandle_t fx = cg_weapons[weapon].missileWallImpactfx;

	if (!fx) {
		return;
	}

	if (altFire) {
		// If this is alt fire. Override all fx with alt fire fx...
		if (cg_weapons[weapon].altMissileWallImpactfx)
		{// We have alt fx for this weapon. Use it.
			fx = cg_weapons[weapon].altMissileWallImpactfx;
		}
	}

	if (fx)
	{// We have fx for this. Play it.
		PlayEffectID(fx, origin, normal, -1, -1, qfalse);
	}
}

/*
---------------------------
FX_DEMP2_HitPlayer
---------------------------
*/

void FX_DEMP2_HitPlayer(vec3_t origin, vec3_t normal, qboolean humanoid, int weapon, qboolean altFire)
{
	// Set fx to primary weapon fx.
	fxHandle_t fx = cg_weapons[weapon].fleshImpactEffect;

	if (!fx) {
		return;
	}

	if (altFire) {
		// If this is alt fire. Override all fx with alt fire fx...
		if (cg_weapons[weapon].altFleshImpactEffect)
		{// We have alt fx for this weapon. Use it.
			fx = cg_weapons[weapon].altFleshImpactEffect;
		}
	}

	if (fx)
	{// We have fx for this. Play it.
		PlayEffectID(fx, origin, normal, -1, -1, qfalse);
	}
}
/*
---------------------------
FX_DEMP2_AltBeam
---------------------------
*/
void FX_DEMP2_AltBeam( vec3_t start, vec3_t end, vec3_t normal, //qboolean spark,
								vec3_t targ1, vec3_t targ2 )
{
	static vec3_t WHITE	={1.0f,1.0f,1.0f};
	static vec3_t BRIGHT={0.75f,0.5f,1.0f};

	// UQ1: Let's at least give it something...
	//"concussion/beam"
	trap->FX_AddLine( start, end, 0.3f, 15.0f, 0.0f,
							1.0f, 0.0f, 0.0f,
							WHITE, WHITE, 0.0f,
							175, trap->R_RegisterShader( "gfx/misc/lightningFlash"/*"gfx/effects/blueLine"*/ ),
							FX_SIZE_LINEAR | FX_ALPHA_LINEAR );

	// add some beef
	trap->FX_AddLine( start, end, 0.3f, 11.0f, 0.0f,
						1.0f, 0.0f, 0.0f,
						BRIGHT, BRIGHT, 0.0f,
						150, trap->R_RegisterShader( "gfx/misc/electric2"/*"gfx/misc/whiteline2"*/ ),
						FX_SIZE_LINEAR | FX_ALPHA_LINEAR );
}

//---------------------------------------------
void FX_DEMP2_AltDetonate( vec3_t org, float size )
{
	localEntity_t	*ex;

	ex = CG_AllocLocalEntity();
	ex->leType = LE_FADE_SCALE_MODEL;
	memset( &ex->refEntity, 0, sizeof( refEntity_t ));

	ex->refEntity.renderfx |= RF_VOLUMETRIC;

	ex->startTime = cg.time;
	ex->endTime = ex->startTime + 800;//1600;

	ex->radius = size;
	ex->refEntity.customShader = cgs.media.demp2ShellShader;
	ex->refEntity.hModel = cgs.media.demp2Shell;
	VectorCopy( org, ex->refEntity.origin );

	ex->color[0] = ex->color[1] = ex->color[2] = 255.0f;
}

//void FX_Lightning_AltBeam(vec3_t org, float size)
//{
//	localEntity_t	*ex;
//
//	ex = CG_AllocLocalEntity();
//	ex->leType = LE_FADE_SCALE_MODEL;
//	memset(&ex->refEntity, 0, sizeof(refEntity_t));
//
//	ex->refEntity.renderfx |= RF_VOLUMETRIC;
//
//	ex->startTime = cg.time;
//	ex->endTime = ex->startTime + 800;//1600;
//
//	ex->radius = size;
//	ex->refEntity.customShader = cgs.media.LightningtradeShader;
//	ex->refEntity.hModel = cgs.media.LightningtradeShader2;
//	VectorCopy(org, ex->refEntity.origin);
//
//	ex->color[0] = ex->color[1] = ex->color[2] = 255.0f;
//}

extern qboolean CG_CalcMuzzlePoint(int entityNum, vec3_t muzzle);

void FX_Lightning_AltBeam(centity_t *shotby, vec3_t end, qboolean hit)
{// "hit" is only used when hitting target. set to qfalse to shoot normal.

	// When you want to draw the beam
	vec3_t muzzlePos, hitPos;
	int i = 0;
	static vec3_t white = { 1.0f, 1.0f, 1.0f }; // will need to be adjusted to match gun height
	float minimumBeamThickness = 2.0;
	float maximumBeamThickness = 5.0f;//28.0f;
	float lifeTimeOfBeamInMilliseconds = 300;

	if (hit)
	{
		VectorCopy(end, hitPos);
		//VectorAdd(WP_MuzzlePoint[WP_ARC_CASTER_IMPERIAL], shotby->lerpOrigin, muzzlePos);
		CG_CalcMuzzlePoint(shotby->currentState.number, muzzlePos);

		//trap->Print("Hit beam between %f %f %f and %f %f %f.\n", muzzlePos[0], muzzlePos[1], muzzlePos[2], hitPos[0], hitPos[1], hitPos[2]);
	}
	else
	{
		VectorCopy(end, hitPos);
		CG_CalcMuzzlePoint(shotby->currentState.number, muzzlePos);

		//trap->Print("Miss beam between %f %f %f and %f %f %f.\n", muzzlePos[0], muzzlePos[1], muzzlePos[2], hitPos[0], hitPos[1], hitPos[2]);
	}

	trap->FX_AddLine(
		muzzlePos, hitPos,
		minimumBeamThickness,
		maximumBeamThickness,
		0.0f,
		1.0f, 0.0f, 0.0f,
		white,
		white,
		0.0f,
		lifeTimeOfBeamInMilliseconds,
		trap->R_RegisterShader("gfx/electricity/electricity_deform"),
		FX_SIZE_LINEAR |
		FX_ALPHA_LINEAR);

	// Add fx all the way along the line... Poor FPS...
	{
		vec3_t currentPos, fxDir;
		float dist = Distance(muzzlePos, hitPos);
		float fxAt = 0;
		float nextFxAt = 8.0;

		weaponInfo_t	*weapon = &cg_weapons[shotby->currentState.weapon];

		VectorCopy(muzzlePos, currentPos);
		VectorSubtract(hitPos, currentPos, fxDir);
		vectoangles(fxDir, fxDir);
		AngleVectors(fxDir, fxDir, NULL, NULL);

		for (fxAt = nextFxAt; fxAt < dist; fxAt += nextFxAt)
		{// Draw a bunch of efx along this line...
			VectorMA(currentPos, nextFxAt, fxDir, currentPos);
			//nextFxAt *= 1.25;
			//trap->Print("FX drawn at range %f pos %f %f %f.\n", fxAt, currentPos[0], currentPos[1], currentPos[2]);
			PlayEffectID(weapon->altMuzzleEffect, currentPos, fxDir, -1, -1, qfalse);
		}
	}
}