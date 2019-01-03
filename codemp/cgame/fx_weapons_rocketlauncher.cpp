// Rocket Launcher Weapon

#include "cg_local.h"

void FX_RocketAddLight(vec3_t org, int weapon)
{// should really do light colors too.. hmm.. i will do the other weap colors another time too slow over tv, okay
	vec4_t color = { 0.9f, 0.9f, 0.2f, 80.0f }; // r, g, b, intensity

	if (cg_weapons[weapon].missileDlightColor[0] || cg_weapons[weapon].missileDlightColor[1] || cg_weapons[weapon].missileDlightColor[2])
		VectorCopy(cg_weapons[weapon].missileDlightColor, color);

	AddLightToScene(org, color[3], color[0], color[1], color[2]);
}

/*
---------------------------
FX_RocketProjectileThink
---------------------------
*/

void FX_RocketProjectileThink(centity_t *cent, const struct weaponInfo_s *weapon)
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

	FX_RocketAddLight(cent->lerpOrigin, cent->currentState.weapon);
}

/*
---------------------------
FX_RocketHitWall
---------------------------
*/

void FX_RocketHitWall(vec3_t origin, vec3_t normal, int weapon, qboolean altFire)
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

void FX_PulseRocketHitWall(vec3_t origin, vec3_t normal, int weapon, qboolean altFire)
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
FX_RocketHitPlayer
---------------------------
*/

void FX_RocketHitPlayer(vec3_t origin, vec3_t normal, qboolean humanoid, int weapon, qboolean altFire)
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

		if (!fx) {
			// If there is no primary (missileWallImpactfx) fx. Use original blaster fx.
			return;
		}
	}

	if (fx)
	{// We have fx for this. Play it.
		//trap->Print("We have fx\n");
		PlayEffectID(fx, origin, normal, -1, -1, qfalse);
	}
}

void FX_PulseRocketHitPlayer(vec3_t origin, vec3_t normal, qboolean humanoid, int weapon, qboolean altFire)
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
FX_RocketAltProjectileThink
---------------------------
*/

void FX_RocketAltProjectileThink(centity_t *cent, const struct weaponInfo_s *weapon)
{
	vec3_t forward;
	fxHandle_t fx = cg_weapons[cent->currentState.weapon].fleshImpactEffect;

	if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
	{
		forward[2] = 1.0f;
	}

	if (fx)
		PlayEffectID(fx, cent->lerpOrigin, forward, -1, -1, qfalse);

	FX_RocketAddLight(cent->lerpOrigin, cent->currentState.weapon);
}

void FX_PulseRocketAltProjectileThink(centity_t *cent, const struct weaponInfo_s *weapon)
{
	vec3_t forward;

	if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
	{
		forward[2] = 1.0f;
	}

	if (weapon->altMissileRenderfx)
	{
		PlayEffectID(weapon->altMissileRenderfx, cent->lerpOrigin, forward, -1, -1, qfalse);
	}
}
