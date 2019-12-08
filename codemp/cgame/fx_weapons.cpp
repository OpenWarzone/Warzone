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
		bolt3D = CG_Get3DWeaponBoltColor(weapon, qfalse);
		break;
	}

#ifdef __SEND_FULL_WEAPON_INFO_WITH_BOLT__
	// Also grab all the original weapon info...
	inventoryItem *gun = BG_GetInventoryItemByID(cent->currentState.boneIndex1);
	inventoryItem *mod1 = BG_GetInventoryItemByID(cent->currentState.boneIndex2);
	inventoryItem *mod2 = BG_GetInventoryItemByID(cent->currentState.boneIndex3);
	inventoryItem *mod3 = BG_GetInventoryItemByID(cent->currentState.boneIndex4);
#endif //__SEND_FULL_WEAPON_INFO_WITH_BOLT__

	if (bolt3D > 0)
	{// New 3D bolt enabled...
		switch (cent->currentState.primaryWeapon)
		{
		case WEAPON_STAT3_SHOT_REPEATING:
			FX_WeaponBolt3D(cent->lerpOrigin, forward, 1.0f / 16.47f, 1.0f, bolt3D, qtrue);
			break;
		default:
			FX_WeaponBolt3D(cent->lerpOrigin, forward, 1.0f, 1.0f, bolt3D, qtrue);
			break;
		}
	}
	else if (weapon->missileRenderfx > 0)
	{// Old 2D system...
		PlayEffectID(weapon->missileRenderfx, cent->lerpOrigin, forward, -1, -1, qfalse);
	}
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
		bolt3D = CG_Get3DWeaponBoltColor(weapon, qtrue);
		break;
	}

#ifdef __SEND_FULL_WEAPON_INFO_WITH_BOLT__
	// Also grab all the original weapon info...
	inventoryItem *gun = BG_GetInventoryItemByID(cent->currentState.boneIndex1);
	inventoryItem *mod1 = BG_GetInventoryItemByID(cent->currentState.boneIndex2);
	inventoryItem *mod2 = BG_GetInventoryItemByID(cent->currentState.boneIndex3);
	inventoryItem *mod3 = BG_GetInventoryItemByID(cent->currentState.boneIndex4);
#endif //__SEND_FULL_WEAPON_INFO_WITH_BOLT__

	if (bolt3D > 0)
	{// New 3D bolt enabled...
		switch (cent->currentState.primaryWeapon)
		{
		case WEAPON_STAT3_SHOT_REPEATING:
			FX_WeaponBolt3D(cent->lerpOrigin, forward, 1.25f / 16.47f, 1.25f, bolt3D, qtrue);
			break;
		default:
			FX_WeaponBolt3D(cent->lerpOrigin, forward, 1.25f, 1.25f, bolt3D, qtrue);
			break;
		}
	}
	else if (weapon->altMissileRenderfx > 0)
	{// Old 2D system...
		PlayEffectID(weapon->altMissileRenderfx, cent->lerpOrigin, forward, -1, -1, qfalse);
	}
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
