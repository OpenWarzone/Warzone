// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_weapons.c -- events and effects dealing with weapons
#include "cg_local.h"
#include "fx_local.h"
#include "ghoul2\G2.h"

/*
Ghoul2 Insert Start
*/
// set up the appropriate ghoul2 info to a refent
void CG_SetGhoul2InfoRef( refEntity_t *ent, refEntity_t	*s1)
{
	ent->ghoul2 = s1->ghoul2;
	VectorCopy( s1->modelScale, ent->modelScale);
	ent->radius = s1->radius;
	VectorCopy( s1->angles, ent->angles);
}

/*
Ghoul2 Insert End
*/

/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals( int itemNum ) {
	itemInfo_t		*itemInfo;
	gitem_t			*item;
	int				handle;

	if ( itemNum < 0 || itemNum >= bg_numItems ) {
		trap->Error( ERR_DROP, "CG_RegisterItemVisuals: itemNum %d out of range [0-%d]", itemNum, bg_numItems-1 );
	}

	itemInfo = &cg_items[ itemNum ];
	if ( itemInfo->registered ) {
		return;
	}

	item = &bg_itemlist[ itemNum ];

	memset( itemInfo, 0, sizeof( *itemInfo ) );
	itemInfo->registered = qtrue;

	if (item->giType == IT_TEAM &&
		(item->giTag == PW_REDFLAG || item->giTag == PW_BLUEFLAG) &&
		cgs.gametype == GT_CTY)
	{ //in CTY the flag model is different
		itemInfo->models[0] = trap->R_RegisterModel( item->world_model[1] );
	}
	else if (item->giType == IT_WEAPON &&
		(item->giTag == WP_THERMAL 
		|| item->giTag == WP_FRAG_GRENADE
		|| item->giTag == WP_TRIP_MINE 
		|| item->giTag == WP_DET_PACK))
	{
		itemInfo->models[0] = trap->R_RegisterModel( item->world_model[1] );
	}
	else
	{
		itemInfo->models[0] = trap->R_RegisterModel( item->world_model[0] );
	}
/*
Ghoul2 Insert Start
*/
	if (!Q_stricmp(&item->world_model[0][strlen(item->world_model[0]) - 4], ".glm"))
	{
		handle = trap->G2API_InitGhoul2Model(&itemInfo->g2Models[0], item->world_model[0], 0 , 0, 0, 0, 0);
		if (handle<0)
		{
			itemInfo->g2Models[0] = NULL;
		}
		else
		{
			itemInfo->radius[0] = 60;
		}
	}
	//could it becours i did somthig with the new gun ? now when i think of ithex edit? yeah i have hexed it but i can do a relook if it is
/*
Ghoul2 Insert End
*/
	if (item->icon)
	{
		if (item->giType == IT_HEALTH)
		{ //medpack gets nomip'd by the ui or something I guess.
			itemInfo->icon = trap->R_RegisterShaderNoMip( item->icon );
		}
		else
		{
			itemInfo->icon = trap->R_RegisterShader( item->icon );
		}
	}
	else
	{
		itemInfo->icon = 0;
	}

	if ( item->giType == IT_WEAPON ) 
	{
		CG_RegisterWeapon( item->giTag );
	}

	//
	// powerups have an accompanying ring or sphere
	//
	if ( item->giType == IT_POWERUP || item->giType == IT_HEALTH ||
		item->giType == IT_ARMOR || item->giType == IT_HOLDABLE ) {
		if ( item->world_model[1] ) {
			itemInfo->models[1] = trap->R_RegisterModel( item->world_model[1] );
		}
	}
}


/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

#define WEAPON_FORCE_BUSY_HOLSTER

#ifdef WEAPON_FORCE_BUSY_HOLSTER
//rww - this was done as a last resort. Forgive me.
static int cgWeapFrame = 0;
static int cgWeapFrameTime = 0;
#endif

/*
=================
CG_MapTorsoToWeaponFrame

=================
*/
static int CG_MapTorsoToWeaponFrame( clientInfo_t *ci, int frame, int animNum ) {
	animation_t *animations = bgHumanoidAnimations;
#ifdef WEAPON_FORCE_BUSY_HOLSTER
	if ( cg.snap->ps.forceHandExtend != HANDEXTEND_NONE || cgWeapFrameTime > cg.time ) {
		// the reason for the after delay is so that it doesn't snap the weapon frame to the "idle" (0) frame for a very quick moment
		if ( cgWeapFrame < 6 ) {
			cgWeapFrame = 6;
			cgWeapFrameTime = cg.time + 10;
		}

		else if ( cgWeapFrameTime < cg.time && cgWeapFrame < 10 ) {
			cgWeapFrame++;
			cgWeapFrameTime = cg.time + 10;
		}

		else if ( cg.snap->ps.forceHandExtend != HANDEXTEND_NONE && cgWeapFrame == 10 )
			cgWeapFrameTime = cg.time + 100;

		return cgWeapFrame;
	}
	else {
		cgWeapFrame = 0;
		cgWeapFrameTime = 0;
	}
#endif

	switch( animNum )
	{
	case TORSO_DROPWEAP1:
		if ( frame >= animations[animNum].firstFrame && frame < animations[animNum].firstFrame + 5 )
		{
			return frame - animations[animNum].firstFrame + 6;
		}
		break;

	case TORSO_RAISEWEAP1:
		if ( frame >= animations[animNum].firstFrame && frame < animations[animNum].firstFrame + 4 )
		{
			return frame - animations[animNum].firstFrame + 6 + 4;
		}
		break;
	case BOTH_ATTACK1:
	case BOTH_ATTACK2:
	case BOTH_ATTACK3:
	case BOTH_ATTACK4:
	case BOTH_ATTACK10:
	case BOTH_THERMAL_THROW:
		if ( frame >= animations[animNum].firstFrame && frame < animations[animNum].firstFrame + 6 )
		{
			return 1 + ( frame - animations[animNum].firstFrame );
		}

		break;
	}
	return -1;
}


/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition( vec3_t origin, vec3_t angles ) {
	float	scale;
	int		delta;
	float	fracsin;

	VectorCopy( cg.refdef.vieworg, origin );
	VectorCopy( cg.refdef.viewangles, angles );

	// on odd legs, invert some angles
	if ( cg.bobcycle & 1 ) {
		scale = -cg.xyspeed;
	} else {
		scale = cg.xyspeed;
	}

	if ( cg_weaponBob.value ) {
		// gun angles from bobbing
		angles[ROLL] += scale * cg.bobfracsin * 0.005;
		angles[YAW] += scale * cg.bobfracsin * 0.01;
		angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005;
	}

	if ( cg_fallingBob.value ) {
		// drop the weapon when landing
		delta = cg.time - cg.landTime;
		if ( delta < LAND_DEFLECT_TIME ) {
			origin[2] += cg.landChange*0.25 * delta / LAND_DEFLECT_TIME;
		} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
			origin[2] += cg.landChange*0.25 *
				(LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
		}
	}

#if 0
	// drop the weapon when stair climbing
	delta = cg.time - cg.stepTime;
	if ( delta < STEP_TIME/2 ) {
		origin[2] -= cg.stepChange*0.25 * delta / (STEP_TIME/2);
	} else if ( delta < STEP_TIME ) {
		origin[2] -= cg.stepChange*0.25 * (STEP_TIME - delta) / (STEP_TIME/2);
	}
#endif

	if ( cg_weaponBob.value ) {
		// idle drift
		scale = cg.xyspeed + 40;
		fracsin = sin( cg.time * 0.001 );
		angles[ROLL] += scale * fracsin * 0.01;
		angles[YAW] += scale * fracsin * 0.01;
		angles[PITCH] += scale * fracsin * 0.01;
	}
}


/*
===============
CG_LightningBolt

Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
The cent should be the non-predicted cent if it is from the player,
so the endpoint will reflect the simulated strike (lagging the predicted
angle)
===============
*/
static void CG_LightningBolt( centity_t *cent, vec3_t origin ) {

}


/*
========================
CG_AddWeaponWithPowerups
========================
*/
static void CG_AddWeaponWithPowerups( refEntity_t *gun, int powerups ) {
	// add powerup effects
	trap->R_AddRefEntityToScene(gun);

	if (cg.predictedPlayerState.electrifyTime > cg.time)
	{ //add electrocution shell
		int preShader = gun->customShader;
		if ( rand() & 1 )
		{
			gun->customShader = cgs.media.electricBodyShader;
		}
		else
		{
			gun->customShader = cgs.media.electricBody2Shader;
		}
		trap->R_AddRefEntityToScene( gun );
		gun->customShader = preShader; //set back just to be safe
	}
}

char *barrelTags[] = {
	"tag_barrel",
	"tag_barrel2",
	"tag_barrel3",
	"tag_barrel4"
};

void CG_AddDebugMuzzleLine( vec3_t from, vec3_t to )
{
	refEntity_t		re;

	memset( &re, 0, sizeof( re ) );

	re.reType = RT_LINE;
	re.radius = 1;

	re.shaderRGBA[0] = re.shaderRGBA[1] = re.shaderRGBA[2] = re.shaderRGBA[3] = 0xff;

	re.customShader = cgs.media.whiteShader;

	VectorCopy(from, re.origin);
	VectorCopy(to, re.oldorigin);

	AddRefEntityToScene( &re );
}

void CG_PositionRotatedEntityOnG2Bolt(refEntity_t *entity, void *g2parent, vec3_t parentOrigin, vec3_t parentAngles, char *boltName)
{
	int bolt = trap->G2API_AddBolt(g2parent, 0, boltName);

	assert(bolt != -1);

	if (bolt != -1)
	{
		//vec3_t boltOrg, boltAng;
		mdxaBone_t boltMatrix;
		vec3_t modelScale;

		VectorSet(modelScale, 1.0, 1.0, 1.0); // pass this through???? Needed??? Probably not...

		trap->G2API_GetBoltMatrix(g2parent, 0, bolt, &boltMatrix, parentAngles, parentOrigin, cg.time, /*cgs.gameModels*/NULL, modelScale);

#if 1
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, entity->origin);
		BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_Z/*NEGATIVE_Y*/, entity->angles);
		//VectorCopy(parentAngles, entity->angles);
		AnglesToAxis(entity->angles, entity->axis);
#else
		// set up the axis and origin we need for the actual effect spawning
		entity->origin[0] = boltMatrix.matrix[0][3];
		entity->origin[1] = boltMatrix.matrix[1][3];
		entity->origin[2] = boltMatrix.matrix[2][3];

		entity->axis[0][0] = boltMatrix.matrix[0][0];
		entity->axis[0][1] = boltMatrix.matrix[1][0];
		entity->axis[0][2] = boltMatrix.matrix[2][0];

		entity->axis[1][0] = boltMatrix.matrix[0][1];
		entity->axis[1][1] = boltMatrix.matrix[1][1];
		entity->axis[1][2] = boltMatrix.matrix[2][1];

		entity->axis[2][0] = boltMatrix.matrix[0][2];
		entity->axis[2][1] = boltMatrix.matrix[1][2];
		entity->axis[2][2] = boltMatrix.matrix[2][2];

		AxisToAngles(entity->axis, entity->angles);
#endif
		/*
		// FIXME: allow origin offsets along tag?
		VectorCopy(parentOrigin, entity->origin);
		for (int i = 0; i < 3; i++) {
			VectorMA(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);
		}

		// had to cast away the const to avoid compiler problems...
		MatrixMultiply(entity->axis, lerped.axis, tempAxis);
		MatrixMultiply(tempAxis, ((refEntity_t *)parent)->axis, entity->axis);
		*/

		trap->Print("CG_PositionRotatedEntityOnG2Bolt: boltname: %s. attached at %f %f %f. parent at %f %f %f.\n", boltName, entity->origin[0], entity->origin[1], entity->origin[2], parentOrigin[0], parentOrigin[1], parentOrigin[2]);
	}
	else
	{// Failed, return parent's data...
		VectorCopy(parentOrigin, entity->origin);
		VectorSet(entity->axis[0], 0.0, 0.0, 0.0);
		VectorSet(entity->axis[1], 0.0, 0.0, 0.0);
		VectorSet(entity->axis[2], 0.0, 0.0, 0.0);

		trap->Print("CG_PositionRotatedEntityOnG2Bolt: Failed to find bolt %s.\n", boltName);
	}
}

/*
=============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
=============
*/
void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent, int team, vec3_t newAngles, qboolean thirdPerson ) {
	refEntity_t				gun;
	refEntity_t				barrel;
	//vec3_t					angles;
	weapon_t				weaponNum;
	weaponInfo_t			*weapon;
	centity_t				*nonPredictedCent;
	refEntity_t				flash;
	int						dif = 0;
	int						shader = 0;
	float					val = 0.0f;
	float					scale = 1.0f;
	addspriteArgStruct_t	fxSArgs;
	vec3_t					flashorigin, flashdir;

	weaponNum = (weapon_t)cent->currentState.weapon;

	if (weaponNum == WP_EMPLACED_GUN)
	{
		return;
	}

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR &&
		cent->currentState.number == cg.predictedPlayerState.clientNum)
	{ //spectator mode, don't draw it...
		return;
	}

	if (cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson && thirdPerson/*!ps*/)
	{// Skip drawing the G2 weapon while in 1st person because we draw the feet/torso...
		return;
	}

	CG_RegisterWeapon( weaponNum );
	weapon = &cg_weapons[weaponNum];

	fxSArgs.shader = 0;

	if (weaponNum == WP_MODULIZED_WEAPON)
	{
		//
		// The new modular weapons system, add module models to a base model...
		//
	 
		// add the weapon
		memset(&gun, 0, sizeof(gun));

		VectorCopy(parent->lightingOrigin, gun.lightingOrigin);
		gun.shadowPlane = parent->shadowPlane;
		gun.renderfx = parent->renderfx;
		gun.hModel = trap->R_RegisterModel("models/wzweapons/pistol_1.md3");

		AnglesToAxis(vec3_origin, gun.axis);

		extern clientInfo_t *CG_GetClientInfoForEnt(centity_t *ent);
		clientInfo_t	*ci = CG_GetClientInfoForEnt(cent);


		mdxaBone_t 		boltMatrix;


		if (!trap->G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), 0))
		{ //it's quite possible that we may have have no weapon model and be in a valid state, so return here if this is the case
			return;
		}

		// go away and get me the bolt position for this frame please
		if (!(trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_rhand, &boltMatrix, newAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale)))
		{	// Couldn't find bolt point.
			return;
		}

		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, gun.origin);

		matrix3_t axis;

		//find bolt rotation unit vectors
		BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_X, axis[0]);  //left/right	
		BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Z, axis[1]);  //fwd/back
		BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_Y, axis[2]);  //up/down?!

																	  //rotational transitions
																	  //configure the initial rotational axis
		VectorCopy(axis[1], gun.axis[0]);
		VectorScale(axis[0], -1, gun.axis[1]); //reversed since this is a right hand rule system.
		VectorCopy(axis[2], gun.axis[2]);

		//debug/config rotation statement.
		extern void ApplyAxisRotation(vec3_t axis[3], int rotType, float value);
		ApplyAxisRotation(gun.axis, PITCH, 90.0);

		AxisToAngles(gun.axis, gun.angles);

		CG_AddWeaponWithPowerups(&gun, cent->currentState.powerups);

		//
		// Add the attachments...
		//

		{// Barrel...
			memset(&barrel, 0, sizeof(barrel));
			VectorCopy(parent->lightingOrigin, gun.lightingOrigin);
			barrel.shadowPlane = parent->shadowPlane;
			barrel.renderfx = parent->renderfx;
			barrel.hModel = trap->R_RegisterModel("models/wzweapons/barrel_1.md3");
			AnglesToAxis(vec3_origin, barrel.axis);
			CG_PositionRotatedEntityOnTag(&barrel, &gun, gun.hModel, "tag_barrel");
			CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups);
		}
		{// Clip...
			memset(&barrel, 0, sizeof(barrel));
			VectorCopy(parent->lightingOrigin, gun.lightingOrigin);
			barrel.shadowPlane = parent->shadowPlane;
			barrel.renderfx = parent->renderfx;
			barrel.hModel = trap->R_RegisterModel("models/wzweapons/clip_1.md3");
			AnglesToAxis(vec3_origin, barrel.axis);
			CG_PositionRotatedEntityOnTag(&barrel, &gun, gun.hModel, "tag_clip");
			CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups);
		}
		{// Scope...
			memset(&barrel, 0, sizeof(barrel));
			VectorCopy(parent->lightingOrigin, gun.lightingOrigin);
			barrel.shadowPlane = parent->shadowPlane;
			barrel.renderfx = parent->renderfx;
			barrel.hModel = trap->R_RegisterModel("models/wzweapons/scope_1.md3");
			AnglesToAxis(vec3_origin, barrel.axis);
			CG_PositionRotatedEntityOnTag(&barrel, &gun, gun.hModel, "tag_scope");
			CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups);
		}
		{// Stock...
			memset(&barrel, 0, sizeof(barrel));
			VectorCopy(parent->lightingOrigin, gun.lightingOrigin);
			barrel.shadowPlane = parent->shadowPlane;
			barrel.renderfx = parent->renderfx;
			barrel.hModel = trap->R_RegisterModel("models/wzweapons/stock_1.md3");
			AnglesToAxis(vec3_origin, barrel.axis);
			CG_PositionRotatedEntityOnTag(&barrel, &gun, gun.hModel, "tag_stock");
			CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups);
		}

		if ((cent->currentState.eFlags & EF_FIRING || ((ps) && ps->weaponstate == WEAPON_FIRING)))
		{
			if (weapon->isBlasterCanon)
			{
				trap->S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->spinSound);
			}
			else
			{
				trap->S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound);
			}
			cent->pe.lightningFiring = qtrue;
		}
		else
		{
			if (weapon->isBlasterCanon)
			{
				if (cent->pe.lightningFiring)
				{
					if (cent->currentState.clientNum == cg.clientNum)
						trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_WEAPONLOCAL, weapon->spindownSound);
					else
						trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_WEAPON, weapon->spindownSound);
				}
			}
			else if (weapon->readySound > 0)
			{
				if (cent->currentState.clientNum == cg.clientNum)
					trap->S_AddLoopingSound(cent->currentState.number, vec3_origin, vec3_origin, weapon->readySound);
				else
					trap->S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound);
			}
			cent->pe.lightningFiring = qfalse;
		}
	}
#if 0
	else
	{
		/*
		Ghoul2 Insert Start
		*/

		if ((cent->currentState.eFlags & EF_FIRING || ((ps) && ps->weaponstate == WEAPON_FIRING)))
		{
			if (weapon->isBlasterCanon)
			{
				trap->S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->spinSound);
			}
			else
			{
				trap->S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound);
			}
			cent->pe.lightningFiring = qtrue;
		}
		else
		{
			if (weapon->isBlasterCanon)
			{
				if (cent->pe.lightningFiring)
				{
					if (cent->currentState.clientNum == cg.clientNum)
						trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_WEAPONLOCAL, weapon->spindownSound);
					else
						trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_WEAPON, weapon->spindownSound);
				}
			}
			else if (weapon->readySound > 0)
			{
				if (cent->currentState.clientNum == cg.clientNum)
					trap->S_AddLoopingSound(cent->currentState.number, vec3_origin, vec3_origin, weapon->readySound);
				else
					trap->S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound);
			}
			cent->pe.lightningFiring = qfalse;
		}

		/*
		Ghoul2 Insert Start
		*/

		//need this for third and first person...
		if (weapon->isBlasterCanon)
		{
			dif = cg.time - cent->blastercannonBarrelRotationTime;

			if ((cent->currentState.eFlags & EF_FIRING) || (ps && ps->weaponstate == WEAPON_FIRING))
			{
				cent->blastercannonBarrelRotationTime = cg.time;
			}
		}

		memset(&gun, 0, sizeof(gun));


		// only do this if we are in first person, since world weapons are now handled on the server by Ghoul2
		if (!thirdPerson)
		{
			// add the weapon
			VectorCopy(parent->lightingOrigin, gun.lightingOrigin);
			gun.shadowPlane = parent->shadowPlane;
			gun.renderfx = parent->renderfx;

			if (ps)
			{	// this player, in first person view
				if (weapon->g2ViewModel)
				{
					gun.ghoul2 = weapon->g2ViewModel;
					gun.radius = 32.0f;
					if (!gun.ghoul2)
					{
						return;
					}
				}
				else
				{
					gun.hModel = weapon->viewModel;
					if (!gun.hModel)
					{
						return;
					}
				}
			}
			else
			{
				gun.hModel = weapon->weaponModel;

				if (!gun.hModel) {
					return;
				}
			}

			if (!gun.hModel) {
				return;
			}

			if (!ps) {
				// add weapon ready sound
				cent->pe.lightningFiring = qfalse;
				if ((cent->currentState.eFlags & EF_FIRING) && weapon->firingSound) {
					// lightning gun and gauntlet make a different sound when fire is held down
					trap->S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound);
					cent->pe.lightningFiring = qtrue;
				}
				else if (weapon->readySound) {
					trap->S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound);
				}
				else if ((cent->currentState.eFlags & EF_FIRING || ((ps) && ps->weaponstate == WEAPON_FIRING))) {
					//If we have a clone rifle, only play this sound if it is a minigun
					if (weapon->isBlasterCanon)
					{
						trap->S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->spinSound);
					}
					else
					{
						trap->S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound);
					}
					cent->pe.lightningFiring = qtrue;
				}
				else
				{
					if (weapon->isBlasterCanon)
					{
						if (cent->pe.lightningFiring)
						{
							if (cent->currentState.clientNum == cg.clientNum)
								trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_WEAPONLOCAL, weapon->spindownSound);
							else
								trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_WEAPON, weapon->spindownSound);
						}
					}
					else if (weapon->readySound > 0) { // Getting initalized to -1 sometimes... readySound isn't even referenced anywere else in code

						trap->S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound);
					}
					cent->pe.lightningFiring = qfalse;
				}
			}

			CG_PositionEntityOnTag(&gun, parent, parent->hModel, "tag_weapon");

			if (weaponNum == WP_MODULIZED_WEAPON)
			{// Test adding new module model to a base model... 1st person - TODO: Remove this crap...
				memset(&barrel, 0, sizeof(barrel));
				VectorCopy(parent->lightingOrigin, barrel.lightingOrigin);
				barrel.shadowPlane = parent->shadowPlane;
				barrel.renderfx = parent->renderfx;

				barrel.hModel = weapon->barrelModel;
				angles[YAW] = 0;
				angles[PITCH] = 0;
				angles[ROLL] = 0;

				AnglesToAxis(angles, barrel.axis);

				CG_PositionRotatedEntityOnTag(&barrel, &gun, trap->R_RegisterModel("models/wzweapons/barrel_1.md3"), "tag_barrel"); // barrel??
			}

			//this stuff under here
			// add the spinning barrel
			if (weapon->barrelModel)
			{
				memset(&barrel, 0, sizeof(barrel));
				VectorCopy(parent->lightingOrigin, barrel.lightingOrigin);
				barrel.shadowPlane = parent->shadowPlane;
				barrel.renderfx = parent->renderfx;

				barrel.hModel = weapon->barrelModel;
				angles[YAW] = 0;
				angles[PITCH] = 0;
				angles[ROLL] = 0;

				AnglesToAxis(angles, barrel.axis);

				//Z6 stuff.
				if (weapon->isBlasterCanon) {
					if (cent->currentState.eFlags & EF_FIRING || ((ps) && ps->weaponstate == WEAPON_FIRING))
					{
						RotateAroundDirection(barrel.axis, cg.time);
					}
					else if (dif > 0) {
						RotateAroundDirection(barrel.axis, (cg.time / (-dif / 5.0)));
					}
				}
				if (weapon->handsModel)
					CG_PositionRotatedEntityOnTag(&barrel, parent, /*gun*/
						weapon->handsModel, "tag_barrel");
				else
					CG_PositionRotatedEntityOnTag(&barrel, parent,/* gun*/
						weapon->weaponModel, "tag_barrel");
			}

			// Render hands with gun
			parent->hModel = weapon->handsModel;
			parent->renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;

			//if (!parent->hModel) trap->Print("Current weapon has no hands model\n");
			gun.renderfx = parent->renderfx;
			gun.hModel = weapon->viewModel;

			CG_PositionEntityOnTag(&gun, parent, parent->hModel, "tag_weapon");
			if (!CG_IsMindTricked(cent->currentState.trickedentindex,
				cent->currentState.trickedentindex2,
				cent->currentState.trickedentindex3,
				cent->currentState.trickedentindex4,
				cg.snap->ps.clientNum))
			{
				CG_AddWeaponWithPowerups(&gun, cent->currentState.powerups); //don't draw the weapon if the player is invisible
			}

			if (weapon->barrelCount > 0/*weapon->barrelModels[0]*/)
			{// Draw barrel models if any... Includes hand models if JKG weapon model...
				int i;

				for (i = 0; i < weapon->barrelCount/*4*/; i++)
				{
					if (weapon->barrelModels[i] == NULL_HANDLE) break;

					memset(&barrel, 0, sizeof(barrel));
					barrel.renderfx = parent->renderfx;
					barrel.hModel = weapon->barrelModels[i];

					AnglesToAxis(vec3_origin, barrel.axis);
					CG_PositionRotatedEntityOnTag(&barrel, parent, parent->hModel, barrelTags[i]);

					CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups);
				}
			}
			else if (weapon->barrelModel)
			{// add the spinning barrel

				memset(&barrel, 0, sizeof(barrel));
				VectorCopy(parent->lightingOrigin, barrel.lightingOrigin);
				barrel.shadowPlane = parent->shadowPlane;
				barrel.renderfx = parent->renderfx;

				barrel.hModel = weapon->barrelModel;
				angles[YAW] = 0;
				angles[PITCH] = 0;
				angles[ROLL] = 0;

				AnglesToAxis(angles, barrel.axis);

				CG_PositionRotatedEntityOnTag(&barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel");

				CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups);
			}
		}
	}
#else
	else
		return;
#endif

/*
Ghoul2 Insert End
*/

	memset (&flash, 0, sizeof(flash));
	CG_PositionEntityOnTag( &flash, &gun, gun.hModel, "tag_flash");

	VectorCopy(flash.origin, cg.lastFPFlashPoint);

	// Do special charge bits
	//-----------------------
	//Make the guns do their charging visual in True View.
	if ((ps || cg.renderingThirdPerson || cg.predictedPlayerState.clientNum != cent->currentState.number) 
		&& ( cent->currentState.modelindex2 == WEAPON_CHARGING_ALT || cent->currentState.modelindex2 == WEAPON_CHARGING ))
	{
		if (!thirdPerson)
		{
			VectorCopy(flash.origin, flashorigin);
			VectorCopy(flash.axis[0], flashdir);
		}
		else
		{
			mdxaBone_t 		boltMatrix;

			if (!trap->G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), 1))
			{ //it's quite possible that we may have have no weapon model and be in a valid state, so return here if this is the case
				return;
			}

			// go away and get me the bolt position for this frame please
 			if (!(trap->G2API_GetBoltMatrix(cent->ghoul2, 1, 0, &boltMatrix, newAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale)))
			{	// Couldn't find bolt point.
				return;
			}

			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, flashorigin);
			BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_X, flashdir);
		}

		if ( val < 0.0f )
		{
			val = 0.0f;
		}
		else if ( val > 1.0f )
		{
			val = 1.0f;
			if (ps && cent->currentState.number == ps->clientNum)
			{
				CGCam_Shake( /*0.1f*/0.2f, 100 );
			}
		}
		else
		{
			if (ps && cent->currentState.number == ps->clientNum)
			{
				CGCam_Shake( val * val * /*0.3f*/0.6f, 100 );
			}
		}

		val += random() * 0.5f;
	}

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.clientNum];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on teh single player podiums), so
	// go ahead and use the cent
	if( ( nonPredictedCent - cg_entities ) != cent->currentState.clientNum ) {
		nonPredictedCent = cent;
	}

	if (ps 
		|| cg.renderingThirdPerson 
		|| cent->currentState.number != cg.predictedPlayerState.clientNum)
	{	// Make sure we don't do the thirdperson model effects for the local player if we're in first person
		refEntity_t	flash;

		memset (&flash, 0, sizeof(flash));

		if (!thirdPerson)
		{
			CG_PositionEntityOnTag( &flash, &gun, gun.hModel, "tag_flash");
			VectorCopy(flash.origin, flashorigin);
			VectorCopy(flash.axis[0], flashdir);
		}
		else
		{
			mdxaBone_t 		boltMatrix;

			if (!trap->G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), 1))
			{ //it's quite possible that we may have have no weapon model and be in a valid state, so return here if this is the case
				return;
			}

			// go away and get me the bolt position for this frame please
 			if (!(trap->G2API_GetBoltMatrix(cent->ghoul2, 1, 0, &boltMatrix, newAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale)))
			{	// Couldn't find bolt point.
				return;
			}

			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, flashorigin);
			BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_X, flashdir);
		}

		//if ( cg.time - cent->muzzleFlashTime <= MUZZLE_FLASH_TIME + 10 )// this need to be there or else it fucks up the muzzle flash efx by playing it all the time
		if (cent->currentState.modelindex2 == WEAPON_CHARGING_ALT)
		{	// Check the alt firing first.
			if (weapon->Altchargingfx)
			{
				if (!thirdPerson)
				{
					trap->FX_PlayEntityEffectID(weapon->Altchargingfx, flashorigin, flash.axis, -1, -1, -1, -1);
				}
				else
				{
					PlayEffectID(weapon->Altchargingfx, flashorigin, flashdir, -1, -1, qfalse);
				}
			}
			else if (fxSArgs.shader)
			{
				VectorCopy(flashorigin, fxSArgs.origin);
				VectorClear(fxSArgs.vel);
				VectorClear(fxSArgs.accel);
				fxSArgs.scale = 3.0f*val*scale;
				fxSArgs.dscale = 0.0f;
				fxSArgs.sAlpha = 0.7f;
				fxSArgs.eAlpha = 0.7f;
				fxSArgs.rotation = random() * 360;
				fxSArgs.bounce = 0.0f;
				fxSArgs.life = 1.0f;
				fxSArgs.shader = shader;
				fxSArgs.flags = 0x08000000;

				//FX_AddSprite( flash.origin, NULL, NULL, 3.0f * val, 0.0f, 0.7f, 0.7f, WHITE, WHITE, random() * 360, 0.0f, 1.0f, shader, FX_USE_ALPHA );
				trap->FX_AddSprite(&fxSArgs);
			}

			cent->altChargeTime = cg.time + 100;

			//trap->Print("ALT CHARGE\n");
		}
		else if (cent->currentState.modelindex2 == WEAPON_CHARGING)
		{	// Check the alt firing first.
			if (weapon->Chargingfx)
			{
				if (!thirdPerson)
				{
					trap->FX_PlayEntityEffectID(weapon->Chargingfx, flashorigin, flash.axis, -1, -1, -1, -1);
				}
				else
				{
					PlayEffectID(weapon->Chargingfx, flashorigin, flashdir, -1, -1, qfalse);
				}
			}
			else if (fxSArgs.shader)
			{
				VectorCopy(flashorigin, fxSArgs.origin);
				VectorClear(fxSArgs.vel);
				VectorClear(fxSArgs.accel);
				fxSArgs.scale = 3.0f*val*scale;
				fxSArgs.dscale = 0.0f;
				fxSArgs.sAlpha = 0.7f;
				fxSArgs.eAlpha = 0.7f;
				fxSArgs.rotation = random() * 360;
				fxSArgs.bounce = 0.0f;
				fxSArgs.life = 1.0f;
				fxSArgs.shader = shader;
				fxSArgs.flags = 0x08000000;

				//FX_AddSprite( flash.origin, NULL, NULL, 3.0f * val, 0.0f, 0.7f, 0.7f, WHITE, WHITE, random() * 360, 0.0f, 1.0f, shader, FX_USE_ALPHA );
				trap->FX_AddSprite(&fxSArgs);
			}

			cent->ChargeTime = cg.time + 100;

			//trap->Print("CHARGE\n");
		}
		else if (cent->altChargeTime > cg.time)
		{	// Check the alt firing first.
			if (weapon->altMuzzleEffect)
			{
				if (!thirdPerson)
				{
					trap->FX_PlayEntityEffectID(weapon->altMuzzleEffect, flashorigin, flash.axis, -1, -1, -1, -1);
				}
				else
				{
					PlayEffectID(weapon->altMuzzleEffect, flashorigin, flashdir, -1, -1, qfalse);
				}
			}
			else if (fxSArgs.shader)
			{
				VectorCopy(flashorigin, fxSArgs.origin);
				VectorClear(fxSArgs.vel);
				VectorClear(fxSArgs.accel);
				fxSArgs.scale = 3.0f*val*scale;
				fxSArgs.dscale = 0.0f;
				fxSArgs.sAlpha = 0.7f;
				fxSArgs.eAlpha = 0.7f;
				fxSArgs.rotation = random() * 360;
				fxSArgs.bounce = 0.0f;
				fxSArgs.life = 1.0f;
				fxSArgs.shader = shader;
				fxSArgs.flags = 0x08000000;

				//FX_AddSprite( flash.origin, NULL, NULL, 3.0f * val, 0.0f, 0.7f, 0.7f, WHITE, WHITE, random() * 360, 0.0f, 1.0f, shader, FX_USE_ALPHA );
				trap->FX_AddSprite(&fxSArgs);
			}

			//trap->Print("ALT FIRE\n");
		}
		else if (cent->ChargeTime > cg.time)
		{	// Check the alt firing first.
			if (weapon->muzzleEffect)
			{
				if (!thirdPerson)
				{
					trap->FX_PlayEntityEffectID(weapon->muzzleEffect, flashorigin, flash.axis, -1, -1, -1, -1);
				}
				else
				{
					PlayEffectID(weapon->muzzleEffect, flashorigin, flashdir, -1, -1, qfalse);
				}
			}
			else if (fxSArgs.shader)
			{
				VectorCopy(flashorigin, fxSArgs.origin);
				VectorClear(fxSArgs.vel);
				VectorClear(fxSArgs.accel);
				fxSArgs.scale = 3.0f*val*scale;
				fxSArgs.dscale = 0.0f;
				fxSArgs.sAlpha = 0.7f;
				fxSArgs.eAlpha = 0.7f;
				fxSArgs.rotation = random() * 360;
				fxSArgs.bounce = 0.0f;
				fxSArgs.life = 1.0f;
				fxSArgs.shader = shader;
				fxSArgs.flags = 0x08000000;

				//FX_AddSprite( flash.origin, NULL, NULL, 3.0f * val, 0.0f, 0.7f, 0.7f, WHITE, WHITE, random() * 360, 0.0f, 1.0f, shader, FX_USE_ALPHA );
				trap->FX_AddSprite(&fxSArgs);
			}

			//trap->Print("FIRE\n");
		}
		else if (fxSArgs.shader)
		{
			VectorCopy(flashorigin, fxSArgs.origin);
			VectorClear(fxSArgs.vel);
			VectorClear(fxSArgs.accel);
			fxSArgs.scale = 3.0f*val*scale;
			fxSArgs.dscale = 0.0f;
			fxSArgs.sAlpha = 0.7f;
			fxSArgs.eAlpha = 0.7f;
			fxSArgs.rotation = random() * 360;
			fxSArgs.bounce = 0.0f;
			fxSArgs.life = 1.0f;
			fxSArgs.shader = shader;
			fxSArgs.flags = 0x08000000;

			//FX_AddSprite( flash.origin, NULL, NULL, 3.0f * val, 0.0f, 0.7f, 0.7f, WHITE, WHITE, random() * 360, 0.0f, 1.0f, shader, FX_USE_ALPHA );
			trap->FX_AddSprite(&fxSArgs);

			//trap->Print("CHARGING SHADER\n");
		}
		else if ((cent->currentState.eFlags & EF_FIRING) && (cg.time - cent->muzzleFlashTime <= MUZZLE_FLASH_TIME + 10))
		{
			if (weapon->muzzleEffect)
			{
				if (!thirdPerson)
				{
					trap->FX_PlayEntityEffectID(weapon->muzzleEffect, flashorigin, flash.axis, -1, -1, -1, -1);
				}
				else
				{
					PlayEffectID(weapon->muzzleEffect, flashorigin, flashdir, -1, -1, qfalse);
				}
			}
			else
			{// No efx to use... Use shader...
				VectorCopy(flashorigin, fxSArgs.origin);
				VectorClear(fxSArgs.vel);
				VectorClear(fxSArgs.accel);
				fxSArgs.scale = 3.0f*val*scale;
				fxSArgs.dscale = 0.0f;
				fxSArgs.sAlpha = 0.7f;
				fxSArgs.eAlpha = 0.7f;
				fxSArgs.rotation = random() * 360;
				fxSArgs.bounce = 0.0f;
				fxSArgs.life = 1.0f;
				fxSArgs.shader = cgs.media.bryarFrontFlash; // FIXME: STOISS FIXME - weapon->muzzleShader ???
				fxSArgs.flags = 0x08000000;

				//FX_AddSprite( flash.origin, NULL, NULL, 3.0f * val, 0.0f, 0.7f, 0.7f, WHITE, WHITE, random() * 360, 0.0f, 1.0f, shader, FX_USE_ALPHA );
				trap->FX_AddSprite(&fxSArgs);
			}

			//trap->Print("FIRING2\n");
		}
		else if ((cent->currentState.eFlags & EF_ALT_FIRING) && (cg.time - cent->muzzleFlashTime <= MUZZLE_FLASH_TIME + 10))
		{
			if (weapon->muzzleEffect)
			{
				if (!thirdPerson)
				{
					trap->FX_PlayEntityEffectID(weapon->muzzleEffect, flashorigin, flash.axis, -1, -1, -1, -1);
				}
				else
				{
					PlayEffectID(weapon->muzzleEffect, flashorigin, flashdir, -1, -1, qfalse);
				}
			}
			else
			{// No efx to use... Use shader...
				VectorCopy(flashorigin, fxSArgs.origin);
				VectorClear(fxSArgs.vel);
				VectorClear(fxSArgs.accel);
				fxSArgs.scale = 3.0f*val*scale;
				fxSArgs.dscale = 0.0f;
				fxSArgs.sAlpha = 0.7f;
				fxSArgs.eAlpha = 0.7f;
				fxSArgs.rotation = random() * 360;
				fxSArgs.bounce = 0.0f;
				fxSArgs.life = 1.0f;
				fxSArgs.shader = cgs.media.bryarFrontFlash; // FIXME: STOISS FIXME - weapon->muzzleShader ???
				fxSArgs.flags = 0x08000000;

				//FX_AddSprite( flash.origin, NULL, NULL, 3.0f * val, 0.0f, 0.7f, 0.7f, WHITE, WHITE, random() * 360, 0.0f, 1.0f, shader, FX_USE_ALPHA );
				trap->FX_AddSprite(&fxSArgs);
			}

			//trap->Print("ALT FIRING2\n");
		}
		
		// add lightning bolt
		CG_LightningBolt( nonPredictedCent, flashorigin );

		if (weaponNum != WP_SABER)
		{// Saber doesn't have flash. This was causing there to be a light when it is off...
			if ( weapon->flashDlightColor[0] || weapon->flashDlightColor[1] || weapon->flashDlightColor[2] ) {
				AddLightToScene( flashorigin, 300 + (rand()&31), weapon->flashDlightColor[0],
					weapon->flashDlightColor[1], weapon->flashDlightColor[2] );
			}
		}
	}

	if (cg_debugMuzzle.integer)
	{
		vec3_t start, end, fdir;
		VectorCopy(flashorigin, start);
		VectorCopy(flashdir, fdir);
		VectorMA( start, 131072, fdir, end );
		CG_AddDebugMuzzleLine( start, end );
	}

	// UQ1: Record position to be used later in code instead of a stupid hard coded array...
	VectorCopy(flashorigin, cent->muzzlePoint);
	VectorCopy(flashdir, cent->muzzleDir);
}

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
void CG_AddViewWeapon( playerState_t *ps ) {
	refEntity_t	hand;
	centity_t	*cent;
	clientInfo_t	*ci;
	float		fovOffset;
	vec3_t		angles, gunPosition;
	weaponInfo_t	*weapon;
	float	cgFov = cg_fov.value;

	if (cgFov < 1)
	{
		cgFov = 1;
	}

	if ( ps->pm_type == PM_INTERMISSION ) {
		return;
	}

	// no gun if in third person view or a camera is active
	//if ( cg.renderingThirdPerson || cg.cameraMode) {
	if ( cg.renderingThirdPerson ) {
		return;
	}

	// allow the gun to be completely removed
	if ( !cg_drawGun.integer || cg.predictedPlayerState.scopeType) {
		vec3_t		origin;

		if ( cg.predictedPlayerState.eFlags & EF_FIRING ) {
			// special hack for lightning gun...
			VectorCopy( cg.refdef.vieworg, origin );
			VectorMA( origin, -8, cg.refdef.viewaxis[2], origin );
			CG_LightningBolt( &cg_entities[ps->clientNum], origin );
		}
		return;
	}

	// don't draw if testing a gun model
	if ( cg.testGun ) {
		return;
	}

	// drop gun lower at higher fov
	if ( cg_fovViewmodelAdjust.integer && cgFov > 90 )
		fovOffset = -0.2f * ( cgFov - 90 );
	else
		fovOffset = 0;

	cent = &cg_entities[cg.predictedPlayerState.clientNum];
	CG_RegisterWeapon( ps->weapon );
	weapon = &cg_weapons[ ps->weapon ];

	memset (&hand, 0, sizeof(hand));

	// set up gun position
	CG_CalculateWeaponPosition( hand.origin, angles );

	/*VectorMA( hand.origin, cg_gunX.value, cg.refdef.viewaxis[0], hand.origin );
	VectorMA( hand.origin, cg_gunY.value, cg.refdef.viewaxis[1], hand.origin );
	VectorMA( hand.origin, (cg_gunZ.value+fovOffset), cg.refdef.viewaxis[2], hand.origin );*/

	// Offset the gun if necessary
    gunPosition[0] = fabs (cg_gunX.value) > FLT_EPSILON ? cg_gunX.value : weapon->gunPosition[0];
    gunPosition[1] = fabs (cg_gunY.value) > FLT_EPSILON ? cg_gunY.value : weapon->gunPosition[1];
    gunPosition[2] = fabs (cg_gunZ.value) > FLT_EPSILON ? cg_gunZ.value : weapon->gunPosition[2];
    
	VectorMA (hand.origin, gunPosition[0], cg.refdef.viewaxis[0], hand.origin);
	VectorMA (hand.origin, gunPosition[1], cg.refdef.viewaxis[1], hand.origin);
	//VectorMA (hand.origin, gunPosition[2] + (fovOffset * (1 - phase)), cg.refdef.viewaxis[2], hand.origin);
	//VectorMA( hand.origin, (cg_gunZ.value+fovOffset), cg.refdef.viewaxis[2], hand.origin );
	VectorMA (hand.origin, gunPosition[2] + cg_gunZ.value + fovOffset, cg.refdef.viewaxis[2], hand.origin);

	AnglesToAxis( angles, hand.axis );

	if ( cg_fovViewmodel.integer )
	{
		float fracDistFOV = tanf( cg.refdef.fov_x * ( M_PI/180 ) * 0.5f );
		float fracWeapFOV = ( 1.0f / fracDistFOV ) * tanf( cgFov * ( M_PI/180 ) * 0.5f );
		VectorScale( hand.axis[0], fracWeapFOV, hand.axis[0] );
	}

	// map torso animations to weapon animations
	if ( cg_debugGun.integer ) {
		// development tool
		hand.frame = hand.oldframe = cg_debugGun.integer;
		hand.backlerp = 0;
	} else {
		float currentFrame;
		// get clientinfo for animation map
		if (cent->currentState.eType == ET_NPC)
		{
			if (!cent->npcClient)
			{
				return;
			}

			ci = cent->npcClient;
		}
		else
		{
			ci = &cgs.clientinfo[ cent->currentState.clientNum ];
		}

		trap->G2API_GetBoneFrame(cent->ghoul2, "lower_lumbar", cg.time, &currentFrame, cgs.gameModels, 0);
		hand.frame = CG_MapTorsoToWeaponFrame( ci, ceil( currentFrame ), ps->torsoAnim );
		hand.oldframe = CG_MapTorsoToWeaponFrame( ci, floor( currentFrame ), ps->torsoAnim );
		hand.backlerp = 1.0f - (currentFrame-floor(currentFrame));


		// Handle the fringe situation where oldframe is invalid
		if ( hand.frame == -1 )
		{
			hand.frame = 0;
			hand.oldframe = 0;
			hand.backlerp = 0;
		}
		else if ( hand.oldframe == -1 )
		{
			hand.oldframe = hand.frame;
			hand.backlerp = 0;
		}
	}

	// add everything onto the hand
	CG_AddPlayerWeapon( &hand, ps, &cg_entities[cg.predictedPlayerState.clientNum], ps->persistant[PERS_TEAM], angles, qfalse );
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/
#define ICON_WEAPONS	0
#define ICON_FORCE		1
#define ICON_INVENTORY	2


void CG_DrawIconBackground(void)
{
	int				/*height, xAdd, x2, y2,*/ t;
//	int				prongLeftX,prongRightX;
	float			inTime = cg.invenSelectTime+WEAPON_SELECT_TIME;
	float			wpTime = cg.weaponSelectTime+WEAPON_SELECT_TIME;
	float			fpTime = cg.forceSelectTime+WEAPON_SELECT_TIME;
//	int				drawType = cgs.media.weaponIconBackground;
//	int				yOffset = 0;

	// don't display if dead
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 )
	{
		return;
	}

	if (cg_hudFiles.integer)
	{ //simple hud
		return;
	}

//	x2 = 30;
//	y2 = SCREEN_HEIGHT-70;

	//prongLeftX =x2+37;
	//prongRightX =x2+544;

	if (inTime > wpTime)
	{
//		drawType = cgs.media.inventoryIconBackground;
		cg.iconSelectTime = cg.invenSelectTime;
	}
	else
	{
//		drawType = cgs.media.weaponIconBackground;
		cg.iconSelectTime = cg.weaponSelectTime;
	}

	if (fpTime > inTime && fpTime > wpTime)
	{
//		drawType = cgs.media.forceIconBackground;
		cg.iconSelectTime = cg.forceSelectTime;
	}

	if ((cg.iconSelectTime+WEAPON_SELECT_TIME)<cg.time)	// Time is up for the HUD to display
	{
		if (cg.iconHUDActive)		// The time is up, but we still need to move the prongs back to their original position
		{
			t =  cg.time - (cg.iconSelectTime+WEAPON_SELECT_TIME);
			cg.iconHUDPercent = t/ 130.0f;
			cg.iconHUDPercent = 1 - cg.iconHUDPercent;

			if (cg.iconHUDPercent<0)
			{
				cg.iconHUDActive = qfalse;
				cg.iconHUDPercent=0;
			}

		//	xAdd = (int) 8*cg.iconHUDPercent;

		//	height = (int) (60.0f*cg.iconHUDPercent);
			//CG_DrawPic( x2+60, y2+30+yOffset, 460, -height, drawType);	// Top half
			//CG_DrawPic( x2+60, y2+30-2+yOffset, 460, height, drawType);	// Bottom half

		}
		else
		{
		//	xAdd = 0;
		}

		return;
	}
	//prongLeftX =x2+37;
	//prongRightX =x2+544;

	if (!cg.iconHUDActive)
	{
		t = cg.time - cg.iconSelectTime;
		cg.iconHUDPercent = t/ 130.0f;

		// Calc how far into opening sequence we are
		if (cg.iconHUDPercent>1)
		{
			cg.iconHUDActive = qtrue;
			cg.iconHUDPercent=1;
		}
		else if (cg.iconHUDPercent<0)
		{
			cg.iconHUDPercent=0;
		}
	}
	else
	{
		cg.iconHUDPercent=1;
	}

	//trap->R_SetColor( colorTable[CT_WHITE] );
	//height = (int) (60.0f*cg.iconHUDPercent);
	//CG_DrawPic( x2+60, y2+30+yOffset, 460, -height, drawType);	// Top half
	//CG_DrawPic( x2+60, y2+30-2+yOffset, 460, height, drawType);	// Bottom half

	// And now for the prongs
/*	if ((cg.inventorySelectTime+WEAPON_SELECT_TIME)>cg.time)
	{
		cgs.media.currentBackground = ICON_INVENTORY;
		background = &cgs.media.inventoryProngsOn;
	}
	else if ((cg.weaponSelectTime+WEAPON_SELECT_TIME)>cg.time)
	{
		cgs.media.currentBackground = ICON_WEAPONS;
	}
	else
	{
		cgs.media.currentBackground = ICON_FORCE;
		background = &cgs.media.forceProngsOn;
	}
*/
	// Side Prongs
//	trap->R_SetColor( colorTable[CT_WHITE]);
//	xAdd = (int) 8*cg.iconHUDPercent;
//	CG_DrawPic( prongLeftX+xAdd, y2-10, 40, 80, background);
//	CG_DrawPic( prongRightX-xAdd, y2-10, -40, 80, background);

}

qboolean CG_WeaponCheck(int weap)
{
	return qtrue;
}

/*
===============
CG_WeaponSelectable
===============
*/
static qboolean CG_WeaponSelectable( int i ) {
	if (!i)
	{
		return qfalse;
	}

	if (!HaveWeapon(&cg.predictedPlayerState, i))
	{
		return qfalse;
	}

	return qtrue;
}

/*
===================
CG_DrawWeaponSelect
===================
*/
void CG_DrawWeaponSelect( void ) {
	int				i;
	int				count;
	int				smallIconSize,bigIconSize;
	int				holdX,x,y,pad;
	int				sideLeftIconCnt,sideRightIconCnt;
	int				sideMax,holdCount,iconCnt;
	int				yOffset = 0;

	if (cg.predictedPlayerState.emplacedIndex)
	{ //can't cycle when on a weapon
		cg.weaponSelectTime = 0;
	}

	if ((cg.weaponSelectTime+WEAPON_SELECT_TIME) < cg.time)	// Time is up for the HUD to display
	{
		return;
	}

	// don't display if dead
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 )
	{
		return;
	}

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	// count the number of weapons owned
	count = 0;

	for ( i = 1 ; i < WP_NUM_WEAPONS ; i++ )
	{
		if (HaveWeapon(&cg.predictedPlayerState, i))
		{
			count++;
		}
	}

	if (count == 0)	// If no weapons, don't display
	{
		return;
	}

	sideMax = 3;	// Max number of icons on the side

	// Calculate how many icons will appear to either side of the center one
	holdCount = count - 1;	// -1 for the center icon

	if (holdCount == 0)			// No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > (2*sideMax))	// Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else							// Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount/2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	i = cg.weaponSelect - 1;

	if ( i < WP_FIRST_USEABLE )
	{
		i = WP_NUM_USEABLE;
	}

	smallIconSize = 40;
	bigIconSize = 80;
	pad = 12;

	x = 320;
	y = 410;

	// Background
//	memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));
//	calcColor[3] = .35f;
//	trap->R_SetColor( calcColor);

	// Left side ICONS
	trap->R_SetColor(colorTable[CT_WHITE]);
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize);

	for (iconCnt = 1; iconCnt < (sideLeftIconCnt +1 ); i--)
	{
		if ( i < WP_FIRST_USEABLE )
		{
			i = WP_NUM_USEABLE;
		}

		if ( !HaveWeapon(&cg.predictedPlayerState, i) )	// Does he have this weapon?
		{
			continue;
		}

		iconCnt++;					// Good icon

		if (cgs.media.weaponIcons[i])
		{
			CG_RegisterWeapon( i );

			trap->R_SetColor(colorTable[CT_WHITE]);

			if (!CG_WeaponCheck(i))
			{
				CG_DrawPic( holdX, y+10+yOffset, smallIconSize, smallIconSize, cgs.media.weaponIcons_NA[i] );
			}
			else
			{
				CG_DrawPic( holdX, y+10+yOffset, smallIconSize, smallIconSize, cgs.media.weaponIcons[i] );
			}

			holdX -= (smallIconSize+pad);
		}
	}

	// Current Center Icon
	if (cgs.media.weaponIcons[cg.weaponSelect])
	{
		CG_RegisterWeapon( cg.weaponSelect );

		trap->R_SetColor( colorTable[CT_WHITE]);

		if (!CG_WeaponCheck(cg.weaponSelect))
		{
			CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10+yOffset, bigIconSize, bigIconSize, cgs.media.weaponIcons_NA[cg.weaponSelect] );
		}
		else
		{
			CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10+yOffset, bigIconSize, bigIconSize, cgs.media.weaponIcons[cg.weaponSelect] );
		}
	}

	i = cg.weaponSelect + 1;

	if ( i > WP_NUM_USEABLE)
	{
		i = WP_FIRST_USEABLE;
	}

	// Right side ICONS
	// Work forwards from current icon
	holdX = x + (bigIconSize/2) + pad;

	for (iconCnt = 1; iconCnt < (sideRightIconCnt +1 ); i++)
	{
		if ( i > WP_NUM_USEABLE)
		{
			i = WP_FIRST_USEABLE;
		}

		if ( !HaveWeapon(&cg.predictedPlayerState, i) )	// Does he have this weapon?
		{
			continue;
		}

		iconCnt++;					// Good icon

		if (cgs.media.weaponIcons[i])
		{
			CG_RegisterWeapon( i );

			// No ammo for this weapon?
			trap->R_SetColor( colorTable[CT_WHITE]);

			if (!CG_WeaponCheck(i))
			{
				CG_DrawPic( holdX, y+10+yOffset, smallIconSize, smallIconSize, cgs.media.weaponIcons_NA[i] );
			}
			else
			{
				CG_DrawPic( holdX, y+10+yOffset, smallIconSize, smallIconSize, cgs.media.weaponIcons[i] );
			}


			holdX += (smallIconSize+pad);
		}
	}

	// draw the selected name
	if ( cg_weapons[ cg.weaponSelect ].item )
	{
		vec4_t		textColor = { .875f, .718f, .121f, 1.0f };
		char		text[1024];
		char		upperKey[1024];

		strcpy(upperKey, cg_weapons[ cg.weaponSelect ].item->classname);

		if ( trap->SE_GetStringTextString( va("SP_INGAME_%s", Q_strupr(upperKey)), text, sizeof( text )))
		{
			CG_DrawProportionalString(320, y+45+yOffset, text, UI_CENTER|UI_SMALLFONT, textColor);
		}
		else
		{
			CG_DrawProportionalString(320, y+45+yOffset, cg_weapons[ cg.weaponSelect ].item->classname, UI_CENTER|UI_SMALLFONT, textColor);
		}

#ifdef _WIN32
		{
			// UQ1: Just testing...
			char text[1024] = { 0 };

			if ( !trap->SE_GetStringTextString( va("SP_INGAME_%s", Q_strupr(upperKey)), text, sizeof( text )))
			{
				strcpy(text, cg_weapons[ cg.weaponSelect ].item->classname);
			}

			if (cg_ttsPlayerVoice.integer >= 2) 
				TextToSpeech( text, CG_GetTextToSpeechVoiceForEntity(&cg_entities[cg.clientNum]), cg.clientNum, cg.snap->ps.origin );
		}
#endif //_WIN32
	}

	trap->R_SetColor( NULL );
}


/*
===============
CG_NextWeapon_f
===============
*/
void CG_NextWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.emplacedIndex)
	{
		return;
	}

	if (cg.predictedPlayerState.scopeType >= SCOPE_BINOCULARS)
	{
		CG_ZoomIn_f();
		return;
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i = 0 ; i < WP_NUM_WEAPONS ; i++ ) {
		//*SIGH*... Hack to put concussion rifle before rocketlauncher
		cg.weaponSelect++;

		if ( cg.weaponSelect > WP_NUM_USEABLE ) {
			cg.weaponSelect = WP_FIRST_USEABLE;
		}
		if ( CG_WeaponSelectable( cg.weaponSelect ) ) {
			break;
		}
	}
	if ( i == WP_NUM_WEAPONS ) {
		cg.weaponSelect = original;
	}
	else
	{
		trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPON);
		trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPONLOCAL);
	}
}

/*
===============
CG_PrevWeapon_f
===============
*/
void CG_PrevWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.emplacedIndex)
	{
		return;
	}

	if (cg.predictedPlayerState.scopeType >= SCOPE_BINOCULARS)
	{
		CG_ZoomOut_f();
		return;
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i = 0 ; i < WP_NUM_WEAPONS ; i++ ) {
		cg.weaponSelect--;
		
		if ( cg.weaponSelect < WP_FIRST_USEABLE) {
			cg.weaponSelect = WP_NUM_USEABLE;
		}

		if ( CG_WeaponSelectable( cg.weaponSelect ) ) {
			break;
		}
	}
	if ( i == WP_NUM_WEAPONS ) {
		cg.weaponSelect = original;
	}
	else
	{
		trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPON);
		trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPONLOCAL);
	}
}

/*
===============
CG_Weapon_f
===============
*/
void CG_Weapon_f( void ) {
	int		num;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	if (cg.snap->ps.emplacedIndex)
	{
		return;
	}

	num = atoi( CG_Argv( 1 ) );

	if ( num < 1 || num > WP_NUM_USEABLE ) {
		return;
	}

	//if (num == 1 && cg.snap->ps.weapon == WP_SABER)
	//{
	//	if (cg.predictedPlayerState.weaponTime < 1)
	////	if (cg.snap->ps.weaponTime < 1)
	//	{
	//		trap->SendConsoleCommand("sv_saberswitch\n");
	//	}
	//	return;
	//}
	if (num == 1 && cg.snap->ps.weapon == WP_SABER)
	{
		//[MELEE]
		//Switch to melee when blade is toggled if ojp_sabermelee is on
		if (cg.snap->ps.weaponTime < 1)
		{
			if (ojp_sabermelee.integer && !cg.snap->ps.saberHolstered && CG_WeaponSelectable(WP_MELEE))
			{
				num = WP_MELEE;

				cg.weaponSelectTime = cg.time;

				if (cg.weaponSelect != num)
				{
					trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPON);
					trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPONLOCAL);
				}

				cg.weaponSelect = num;
			}
			else
			{
				trap->SendConsoleCommand("sv_saberswitch\n");
			}
		}
	}

	//rww - hack to make weapon numbers same as single player
	if (num > WP_FIRST_USEABLE)
	{
		num++;
	}
	else
	{
		if (HaveWeapon(&cg.snap->ps, WP_SABER))
		{
			num = WP_SABER;
		}
		else
		{
			num = WP_MELEE;
		}
	}

	if (num > WP_NUM_USEABLE+1)
	{ //other weapons are off limits due to not actually being weapon weapons
		return;
	}

	if (num >= WP_THERMAL && num <= WP_DET_PACK)
	{
		int weap, i = 0;

		if (cg.snap->ps.weapon >= WP_THERMAL && cg.snap->ps.weapon >= WP_FRAG_GRENADE 
			&& cg.snap->ps.weapon >= WP_CYROBAN_GRENADE
			&& cg.snap->ps.weapon <= WP_DET_PACK)
		{
			// already in cycle range so start with next cycle item
			weap = cg.snap->ps.weapon + 1;
		}
		else
		{
			// not in cycle range, so start with thermal detonator
			weap = WP_THERMAL;
			//weap = WP_FRAG_GRENADE;
		}

		// prevent an endless loop
		while ( i <= 4 )
		{
			if (weap > WP_DET_PACK)
			{
				weap = WP_THERMAL;
				//weap = WP_FRAG_GRENADE;
			}

			if (CG_WeaponSelectable(weap))
			{
				num = weap;
				break;
			}

			weap++;
			i++;
		}
	}

	if (!CG_WeaponSelectable(num))
	{
		return;
	}

	cg.weaponSelectTime = cg.time;

	if (!HaveWeapon(&cg.snap->ps, num))
	{
		if (num == WP_SABER)
		{ //don't have saber, try melee on the same slot
			num = WP_MELEE;

			if (!HaveWeapon(&cg.snap->ps, num))
			{
				return;
			}
		}
		else
		{
			return;		// don't have the weapon
		}
	}

	if (cg.weaponSelect != num)
	{
		trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPON);
		trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPONLOCAL);
	}

	cg.weaponSelect = num;
}


//Version of the above which doesn't add +2 to a weapon.  The above can't
//triger WP_MELEE.  Derogatory comments go here.
void CG_WeaponClean_f( void ) {
	int		num;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	if (cg.snap->ps.emplacedIndex)
	{
		return;
	}

	num = atoi( CG_Argv( 1 ) );

	if ( num < 1 || num > WP_NUM_USEABLE ) {
		return;
	}

	if (num == 1 && cg.snap->ps.weapon == WP_SABER)
	{
		if (cg.snap->ps.weaponTime < 1)
		{
			trap->SendConsoleCommand("sv_saberswitch\n");
		}
		return;
	}

	if (num == WP_FIRST_USEABLE)
	{
		if (HaveWeapon(&cg.snap->ps, WP_SABER))
		{
			num = WP_SABER;
		}
		else
		{
			num = WP_MELEE;
		}
	}

	if (num > WP_NUM_USEABLE+1)
	{ //other weapons are off limits due to not actually being weapon weapons
		return;
	}

	if (num >= WP_THERMAL && num <= WP_DET_PACK)
	{
		int weap, i = 0;

		if (cg.snap->ps.weapon >= WP_THERMAL && cg.snap->ps.weapon >= WP_FRAG_GRENADE 
			&& cg.snap->ps.weapon >= WP_CYROBAN_GRENADE
			&& cg.snap->ps.weapon <= WP_DET_PACK)
		{
			// already in cycle range so start with next cycle item
			weap = cg.snap->ps.weapon + 1;
		}
		else
		{
			// not in cycle range, so start with thermal detonator
			weap = WP_THERMAL;
			//weap = WP_FRAG_GRENADE;
		}

		// prevent an endless loop
		while ( i <= 4 )
		{
			if (weap > WP_DET_PACK)
			{
				weap = WP_THERMAL;
				//weap = WP_FRAG_GRENADE;
			}

			if (CG_WeaponSelectable(weap))
			{
				num = weap;
				break;
			}

			weap++;
			i++;
		}
	}

	if (!CG_WeaponSelectable(num))
	{
		return;
	}

	cg.weaponSelectTime = cg.time;

	if (HaveWeapon(&cg.snap->ps, num))
	{
		if (num == WP_SABER)
		{ //don't have saber, try melee on the same slot
			num = WP_MELEE;

			if (!HaveWeapon(&cg.snap->ps, num))
			{
				return;
			}
		}
		else
		{
			return;		// don't have the weapon
		}
	}

	if (cg.weaponSelect != num)
	{
		trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPON);
		trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPONLOCAL);
	}

	cg.weaponSelect = num;
}



/*
===================
CG_OutOfAmmoChange

The current weapon has just run out of ammo
===================
*/
void CG_OutOfAmmoChange( int oldWeapon )
{
#if 0
	int		i;

	cg.weaponSelectTime = cg.time;

	for ( i = WP_NUM_USEABLE ; i > 0 ; i-- )	//We don't want the emplaced or turret
	{
		if ( CG_WeaponSelectable( i ) )
		{
			//rww - Don't we want to make sure i != one of these if autoswitch is 1 (safe)?
			if (cg_autoSwitch.integer != 1 || (i != WP_TRIP_MINE && i != WP_DET_PACK && i != WP_THERMAL && i != WP_ROCKET && i != WP_FRAG_GRENADE))
			{
				if (i != oldWeapon)
				{ //don't even do anything if we're just selecting the weapon we already have/had
					cg.weaponSelect = i;
					break;
				}
			}
		}
	}

	trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPON);
	trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPONLOCAL);
#endif
}



/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

void CG_GetClientWeaponMuzzleBoltPoint(int clIndex, vec3_t to)
{
	centity_t *cent;
	mdxaBone_t	boltMatrix;

	if (clIndex < 0 || clIndex >= MAX_CLIENTS)
	{
		return;
	}

	cent = &cg_entities[clIndex];

	if (!cent || !cent->ghoul2 || !trap->G2_HaveWeGhoul2Models(cent->ghoul2) ||
		!trap->G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), 1))
	{
		return;
	}

	trap->G2API_GetBoltMatrix(cent->ghoul2, 1, 0, &boltMatrix, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, to);
}

/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event
================
*/
void CG_FireWeapon( centity_t *cent, qboolean altFire ) {
	entityState_t *ent;
	int				c;
	weaponInfo_t	*weap;

	ent = &cent->currentState;
	if ( ent->weapon == WP_NONE ) {
		return;
	}
	if ( ent->weapon >= WP_NUM_WEAPONS ) {
		trap->Error( ERR_DROP, "CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS" );
		return;
	}
	weap = &cg_weapons[ ent->weapon ];

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleFlashTime = cg.time;

	#ifdef BASE_COMPAT
		// play quad sound if needed
		if ( cent->currentState.powerups & ( 1 << PW_QUAD ) ) {
			//trap->S_StartSound (NULL, cent->currentState.number, CHAN_ITEM, cgs.media.quadSound );
		}
	#endif // BASE_COMPAT

	// play a sound
	if (altFire)
	{
		// play a sound
		for ( c = 0 ; c < 4 ; c++ ) {
			if ( !weap->altFlashSound[c] ) {
				break;
			}
		}
		if ( c > 0 ) {
			c = rand() % c;
			if ( weap->altFlashSound[c] )
			{
				if (ent->number == cg.clientNum)
					trap->S_StartSound(NULL, ent->number, CHAN_WEAPONLOCAL, weap->altFlashSound[c]);
				else
					trap->S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->altFlashSound[c] );
			}
		}
//		if ( weap->altFlashSnd )
//		{
//			trap->S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->altFlashSnd );
//		}
	}
	else
	{
		// play a sound
		for ( c = 0 ; c < 4 ; c++ ) {
			if ( !weap->flashSound[c] ) {
				break;
			}
		}
		if ( c > 0 ) {
			c = rand() % c;
			if ( weap->flashSound[c] )
			{
				if (ent->number == cg.clientNum)
					trap->S_StartSound(NULL, ent->number, CHAN_WEAPONLOCAL, weap->flashSound[c]);
				else
					trap->S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->flashSound[c] );
			}
		}
	}
}

qboolean CG_VehicleWeaponImpact( centity_t *cent )
{//see if this is a missile entity that's owned by a vehicle and should do a special, overridden impact effect
	if ((cent->currentState.eFlags&EF_JETPACK_ACTIVE)//hack so we know we're a vehicle Weapon shot
		&& cent->currentState.otherEntityNum2
		&& g_vehWeaponInfo[cent->currentState.otherEntityNum2].iImpactFX)
	{//missile is from a special vehWeapon
		vec3_t normal;
		ByteToDir( cent->currentState.eventParm, normal );

		PlayEffectID( g_vehWeaponInfo[cent->currentState.otherEntityNum2].iImpactFX, cent->lerpOrigin, normal, -1, -1, qfalse );
		return qtrue;
	}
	return qfalse;
}

/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
void CG_MissileHitWall(int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType, qboolean altFire, int charge)
{
	FX_WeaponHitWall(origin, dir, weapon, altFire);
}


/*
=================
CG_MissileHitPlayer
=================
*/
void CG_MissileHitPlayer(int weapon, vec3_t origin, vec3_t dir, int entityNum, qboolean altFire)
{
	qboolean	humanoid = qtrue;
	FX_WeaponHitPlayer(origin, dir, humanoid, weapon, altFire);
}


/*
============================================================================

BULLETS

============================================================================
*/


/*
======================
CG_CalcMuzzlePoint
======================
*/
qboolean CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle ) {
#if 0 // UQ: Since we already stored it when drawing it...
	vec3_t		forward, right;
	vec3_t		gunpoint;
	int			anim;
#endif
	centity_t	*cent;

	if ( entityNum == cg.snap->ps.clientNum )
	{ //I'm not exactly sure why we'd be rendering someone else's crosshair, but hey.
#if 0 // UQ: Since we already stored it when drawing it...
		int weapontype = cg.snap->ps.weapon;
		vec3_t weaponMuzzle;
		centity_t *pEnt = &cg_entities[cg.predictedPlayerState.clientNum];

		if (cg_muzzleX.value != 0.0 || cg_muzzleY.value != 0.0 || cg_muzzleZ.value != 0.0)
		{
			VectorSet(weaponMuzzle, cg_muzzleX.value != 0.0, cg_muzzleY.value, cg_muzzleZ.value);
		}
		else
		{
			VectorCopy(WP_MuzzlePoint[weapontype], weaponMuzzle);
		}

		if (cg.renderingThirdPerson)
		{
			VectorCopy( pEnt->lerpOrigin, gunpoint );
			AngleVectors( pEnt->lerpAngles, forward, right, NULL );
		}
		else
		{
			VectorCopy( cg.refdef.vieworg, gunpoint );
			AngleVectors( cg.refdef.viewangles, forward, right, NULL );
		}

		if (weapontype == WP_EMPLACED_GUN && cg.snap->ps.emplacedIndex)
		{
			centity_t *gunEnt = &cg_entities[cg.snap->ps.emplacedIndex];

			if (gunEnt)
			{
				vec3_t pitchConstraint;

				VectorCopy(gunEnt->lerpOrigin, gunpoint);
				gunpoint[2] += 46;

				if (cg.renderingThirdPerson)
				{
					VectorCopy(pEnt->lerpAngles, pitchConstraint);
				}
				else
				{
					VectorCopy(cg.refdef.viewangles, pitchConstraint);
				}

				if (pitchConstraint[PITCH] > 40)
				{
					pitchConstraint[PITCH] = 40;
				}
				AngleVectors( pitchConstraint, forward, right, NULL );
			}
		}

		VectorCopy(gunpoint, muzzle);

		VectorMA(muzzle, weaponMuzzle[0], forward, muzzle);
		VectorMA(muzzle, weaponMuzzle[1], right, muzzle);

		if (weapontype == WP_EMPLACED_GUN && cg.snap->ps.emplacedIndex)
		{
			//Do nothing
		}
		else if (cg.renderingThirdPerson)
		{
			muzzle[2] += (cg.snap->ps.viewheight + weaponMuzzle[2]) * (pEnt->currentState.iModelScale/100.0f);
		}
		else
		{
			muzzle[2] += weaponMuzzle[2] * (pEnt->currentState.iModelScale/100.0f);
		}
#else
		centity_t *pEnt = &cg_entities[cg.predictedPlayerState.clientNum];
		VectorCopy(pEnt->muzzlePoint, muzzle);
#endif

		return qtrue;
	}

	cent = &cg_entities[entityNum];

	if ( !cent->currentValid ) {
		return qfalse;
	}

#if 0 // UQ: Since we already stored it when drawing it...
	VectorCopy( cent->currentState.pos.trBase, muzzle );

	AngleVectors( cent->currentState.apos.trBase, forward, NULL, NULL );
	anim = cent->currentState.legsAnim;
	if ( anim == BOTH_CROUCH1WALK || anim == BOTH_CROUCH1IDLE ) {
		muzzle[2] += (CROUCH_VIEWHEIGHT * (cent->currentState.iModelScale/100.0f));
	} else {
		muzzle[2] += (DEFAULT_VIEWHEIGHT * (cent->currentState.iModelScale/100.0f));
	}

	VectorMA( muzzle, 14, forward, muzzle );
#else
	VectorCopy(cent->muzzlePoint, muzzle);
#endif

	return qtrue;

}



/*
Ghoul2 Insert Start
*/

// create one instance of all the weapons we are going to use so we can just copy this info into each clients gun ghoul2 object in fast way
static void *g2WeaponInstances[WP_NUM_WEAPONS];
//[VisualWeapons]
void *g2HolsterWeaponInstances[WP_NUM_WEAPONS];
//[/VisualWeapons]
void CG_InitG2Weapons(void)
{
	int i = 0;
	gitem_t		*item;
	memset(g2WeaponInstances, 0, sizeof(g2WeaponInstances));
	for ( item = bg_itemlist + 1 ; item->classname ; item++ )
	{
		if ( item->giType == IT_WEAPON )
		{
			assert(item->giTag < WP_NUM_WEAPONS);

			//CG_LoadingString(CG_GetStringEdString("SP_INGAME", Q_strupr(item->classname))); // FIXME: Add the names to the strings file and enable this one!
			CG_LoadingString(va("%s (G2 model)", item->classname));

			// initialise model
			trap->G2API_InitGhoul2Model(&g2WeaponInstances[/*i*/item->giTag], item->world_model[0], 0, 0, 0, 0, 0);
			//[VisualWeapons]
			//init holster models at the same time.
			trap->G2API_InitGhoul2Model(&g2HolsterWeaponInstances[item->giTag], item->world_model[0], 0, 0, 0, 0, 0);
			//[/VisualWeapons
//			trap->G2API_InitGhoul2Model(&g2WeaponInstances[i], item->world_model[0],G_ModelIndex( item->world_model[0] ) , 0, 0, 0, 0);
			if (g2WeaponInstances[/*i*/item->giTag])
			{
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				trap->G2API_SetBoltInfo(g2WeaponInstances[/*i*/item->giTag], 0, 0);
				// now set up the gun bolt on it
				if (item->giTag == WP_SABER)
				{
					trap->G2API_AddBolt(g2WeaponInstances[/*i*/item->giTag], 0, "*blade1");
					trap->G2API_AddBolt(g2WeaponInstances[/*i*/item->giTag], 0, "*blade2");
				}
				else
				{
					trap->G2API_AddBolt(g2WeaponInstances[/*i*/item->giTag], 0, "*flash");
				}
				i++;
			}
			if (i == WP_NUM_WEAPONS)
			{
				assert(0);
				break;
			}

		}
	}
}

// clean out any g2 models we instanciated for copying purposes
void CG_ShutDownG2Weapons(void)
{
	int i;
	for (i = 0; i < WP_NUM_WEAPONS; i++)
	{
		weaponInfo_t *weaponInfo = NULL;

		trap->G2API_CleanGhoul2Models(&g2WeaponInstances[i]);
		//[VisualWeapons]
		trap->G2API_CleanGhoul2Models(&g2HolsterWeaponInstances[i]);
		//[/VisualWeapons]

		weaponInfo = &cg_weapons[i];

		if (weaponInfo != NULL)
		{
			//trap->G2API_CleanGhoul2Models(&weaponInfo->g2WorldModel);
			trap->G2API_CleanGhoul2Models(&weaponInfo->g2ViewModel);
		}
	}
}

void *CG_G2WeaponInstance(centity_t *cent, int weapon)
{
	clientInfo_t *ci = NULL;

	if (weapon != WP_SABER)
	{
		return g2WeaponInstances[weapon];
	}

	if (cent->currentState.eType != ET_PLAYER &&
		cent->currentState.eType != ET_NPC)
	{
		return g2WeaponInstances[weapon];
	}

	if (cent->currentState.eType == ET_NPC)
	{
		ci = cent->npcClient;
	}
	else
	{
		ci = &cgs.clientinfo[cent->currentState.number];
	}

	if (!ci)
	{
		return g2WeaponInstances[weapon];
	}

	//Try to return the custom saber instance if we can.
	if (ci->saber[0].model[0] &&
		ci->ghoul2Weapons[0])
	{
		return ci->ghoul2Weapons[0];
	}

	//If no custom then just use the default.
	return g2WeaponInstances[weapon];
}

//[VisualWeapons]
void *CG_G2HolsterWeaponInstance(centity_t *cent, int weapon, qboolean secondSaber)
{
	clientInfo_t *ci = NULL;

	if (weapon != WP_SABER)
	{
		return g2HolsterWeaponInstances[weapon];
	}

	if (cent->currentState.eType != ET_PLAYER &&
		cent->currentState.eType != ET_NPC)
	{
		return g2HolsterWeaponInstances[weapon];
	}

	if (cent->currentState.eType == ET_NPC)
	{
		ci = cent->npcClient;
	}
	else
	{
		ci = &cgs.clientinfo[cent->currentState.number];
	}

	if (!ci)
	{
		return g2HolsterWeaponInstances[weapon];
	}

	//Try to return the custom saber instance if we can.
	if(secondSaber)
	{//return secondSaber instance
		if (ci->saber[1].model[0] &&
			ci->ghoul2HolsterWeapons[1])
		{
			return ci->ghoul2HolsterWeapons[1];
		}
	}
	else
	{//return first saber instance
		if (ci->saber[0].model[0] &&
			ci->ghoul2HolsterWeapons[0])
		{
			return ci->ghoul2HolsterWeapons[0];
		}
	}

	//If no custom then just use the default.
	return g2HolsterWeaponInstances[weapon];
}
//[/VisualWeapons]

// what ghoul2 model do we want to copy ?
void CG_CopyG2WeaponInstance(centity_t *cent, int weaponNum, void *toGhoul2)
{
	//rww - the -1 is because there is no "weapon" for WP_NONE
	assert(weaponNum < WP_NUM_WEAPONS);
	if (CG_G2WeaponInstance(cent, weaponNum/*-1*/))
	{
		if (weaponNum == WP_SABER)
		{
			clientInfo_t *ci = NULL;

			if (cent->currentState.eType == ET_NPC)
			{
				ci = cent->npcClient;
			}
			else
			{
				ci = &cgs.clientinfo[cent->currentState.number];
			}

			if (!ci)
			{
				trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, weaponNum/*-1*/), 0, toGhoul2, 1);
			}
			else
			{ //Try both the left hand saber and the right hand saber
				int i = 0;

				while (i < MAX_SABERS)
				{
					if (ci->saber[i].model[0] &&
						ci->ghoul2Weapons[i])
					{
						trap->G2API_CopySpecificGhoul2Model(ci->ghoul2Weapons[i], 0, toGhoul2, i+1);
					}
					else if (ci->ghoul2Weapons[i])
					{ //if the second saber has been removed, then be sure to remove it and free the instance.
						qboolean g2HasSecondSaber = trap->G2API_HasGhoul2ModelOnIndex(&(toGhoul2), 2);

						if (g2HasSecondSaber)
						{ //remove it now since we're switching away from sabers
							trap->G2API_RemoveGhoul2Model(&(toGhoul2), 2);
						}
						trap->G2API_CleanGhoul2Models(&ci->ghoul2Weapons[i]);
					}

					i++;
				}
			}
		}
		else
		{
			qboolean g2HasSecondSaber = trap->G2API_HasGhoul2ModelOnIndex(&(toGhoul2), 2);

			if (g2HasSecondSaber)
			{ //remove it now since we're switching away from sabers
				trap->G2API_RemoveGhoul2Model(&(toGhoul2), 2);
			}

			if (weaponNum == WP_EMPLACED_GUN)
			{ //a bit of a hack to remove gun model when using an emplaced weap
				if (trap->G2API_HasGhoul2ModelOnIndex(&(toGhoul2), 1))
				{
					trap->G2API_RemoveGhoul2Model(&(toGhoul2), 1);
				}
			}
			else if (weaponNum == WP_MELEE)
			{ //don't want a weapon on the model for this one
				if (trap->G2API_HasGhoul2ModelOnIndex(&(toGhoul2), 1))
				{
					trap->G2API_RemoveGhoul2Model(&(toGhoul2), 1);
				}
			}
			else
			{
				trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, weaponNum/*-1*/), 0, toGhoul2, 1);
			}
		}
	}
}

void CG_CheckPlayerG2Weapons(playerState_t *ps, centity_t *cent)
{
	if (!ps)
	{
		assert(0);
		return;
	}

	if (ps->pm_flags & PMF_FOLLOW)
	{
		return;
	}

	if (cent->currentState.eType == ET_NPC)
	{
		assert(0);
		return;
	}

	// should we change the gun model on this player?
	if (cent->currentState.saberInFlight)
	{
		cent->ghoul2weapon = CG_G2WeaponInstance(cent, WP_SABER);
	}

	if (cent->currentState.eFlags & EF_DEAD)
	{ //no updating weapons when dead
		cent->ghoul2weapon = NULL;
		return;
	}

	if (cent->torsoBolt)
	{ //got our limb cut off, no updating weapons until it's restored
		cent->ghoul2weapon = NULL;
		return;
	}

	if (cgs.clientinfo[ps->clientNum].team == FACTION_SPECTATOR ||
		ps->persistant[PERS_TEAM] == FACTION_SPECTATOR)
	{
		cent->ghoul2weapon = cg_entities[ps->clientNum].ghoul2weapon = NULL;
		cent->weapon = cg_entities[ps->clientNum].weapon = 0;
		return;
	}

	if (cent->ghoul2 && cent->ghoul2weapon != CG_G2WeaponInstance(cent, ps->weapon) &&
		ps->clientNum == cent->currentState.number) //don't want spectator mode forcing one client's weapon instance over another's
	{
		CG_CopyG2WeaponInstance(cent, ps->weapon, cent->ghoul2);
		cent->ghoul2weapon = CG_G2WeaponInstance(cent, ps->weapon);
		if (cent->weapon == WP_SABER && cent->weapon != ps->weapon && !ps->saberHolstered)
		{ //switching away from the saber
			//trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, trap->S_RegisterSound( "sound/weapons/saber/saberoffquick.wav" ));
			if (cgs.clientinfo[ps->clientNum].saber[0].soundOff && !ps->saberHolstered)
			{
				trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, cgs.clientinfo[ps->clientNum].saber[0].soundOff);
			}

			if (cgs.clientinfo[ps->clientNum].saber[1].soundOff &&
				cgs.clientinfo[ps->clientNum].saber[1].model[0] &&
				!ps->saberHolstered)
			{
				trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, cgs.clientinfo[ps->clientNum].saber[1].soundOff);
			}
		}
		else if (ps->weapon == WP_SABER && cent->weapon != ps->weapon && !cent->saberWasInFlight)
		{ //switching to the saber
			//trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, trap->S_RegisterSound( "sound/weapons/saber/saberon.wav" ));
			if (cgs.clientinfo[ps->clientNum].saber[0].soundOn)
			{
				trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, cgs.clientinfo[ps->clientNum].saber[0].soundOn);
			}

			if (cgs.clientinfo[ps->clientNum].saber[1].soundOn)
			{
				trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, cgs.clientinfo[ps->clientNum].saber[1].soundOn);
			}

			BG_SI_SetDesiredLength(&cgs.clientinfo[ps->clientNum].saber[0], 0, -1);
			BG_SI_SetDesiredLength(&cgs.clientinfo[ps->clientNum].saber[1], 0, -1);
		}
		cent->weapon = ps->weapon;
	}
}


/*
Ghoul2 Insert End
*/
