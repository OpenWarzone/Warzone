// Bryar Pistol Weapon Effects

#include "cg_local.h"
#include "fx_local.h"

extern void FX_WeaponBolt3D(vec3_t org, vec3_t fwd, float length, float radius, qhandle_t shader, qboolean addLight);

//
// UniqueOne's New - GENERIC - Weapon FX Code...
//

/*
-------------------------
FX_WeaponHitWall
-------------------------
*/
void FX_WeaponHitWall(vec3_t origin, vec3_t normal, int weapon, qboolean altFire)
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
-------------------------
FX_WeaponHitPlayer
-------------------------
*/
void FX_WeaponHitPlayer(vec3_t origin, vec3_t normal, qboolean humanoid, int weapon, qboolean altFire)
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
-------------------------
FX_WeaponProjectileThink
-------------------------
*/
void FX_WeaponProjectileThink(centity_t *cent, const struct weaponInfo_s *weapon)
{
	vec3_t forward;

	if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
	{
		forward[2] = 1.0f;
	}

	qhandle_t bolt3D = CG_Get3DWeaponBoltColor(weapon, qfalse);

	//trap->Print("bolt id is %i.\n", bolt3D);
	
	if (bolt3D > 0)
	{// New 3D bolt enabled...
		FX_WeaponBolt3D(cent->lerpOrigin, forward, CG_Get3DWeaponBoltLength(weapon, qfalse), CG_Get3DWeaponBoltWidth(weapon, qfalse), bolt3D, qtrue);
	}
	else if (weapon->missileRenderfx > 0)
	{// Old 2D system...
		PlayEffectID(weapon->missileRenderfx, cent->lerpOrigin, forward, -1, -1, qfalse);
	}

	//AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1.0f, 1.0f, 1.0f );
}

/*
-------------------------
FX_WeaponProjectileThink
-------------------------
*/
void FX_WeaponAltProjectileThink(centity_t *cent, const struct weaponInfo_s *weapon)
{
	vec3_t forward;

	if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
	{
		forward[2] = 1.0f;
	}
	
	qhandle_t bolt3D = CG_Get3DWeaponBoltColor(weapon, qtrue);

	if (bolt3D > 0)
	{// New 3D bolt enabled...
		FX_WeaponBolt3D(cent->lerpOrigin, forward, CG_Get3DWeaponBoltLength(weapon, qtrue), CG_Get3DWeaponBoltWidth(weapon, qtrue), bolt3D, qtrue);
	}
	else if (weapon->altMissileRenderfx > 0)
	{// Old 2D system...
		PlayEffectID(weapon->altMissileRenderfx, cent->lerpOrigin, forward, -1, -1, qfalse);
	}

	//AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1.0f, 1.0f, 1.0f );
}

void FX_ThermalProjectileThink(centity_t *cent, const struct weaponInfo_s *weapon)
{
	vec3_t forward;

	if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
	{
		forward[2] = 1.0f;
	}

	if (weapon->missileRenderfx)
	{
		PlayEffectID(weapon->missileRenderfx, cent->lerpOrigin, forward, -1, -1, qfalse);
	}
}


void FX_PulseGrenadeProjectileThink(centity_t *cent, const struct weaponInfo_s *weapon)
{
	vec3_t forward;

	if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
	{
		forward[2] = 1.0f;
	}

	if (weapon->missileRenderfx)
	{
		PlayEffectID(weapon->altMissileRenderfx, cent->lerpOrigin, forward, -1, -1, qfalse);
	}
}
