//
// cg_weaponinit.c -- events and effects dealing with weapons
#include "cg_local.h"
#include "fx_local.h"

extern void CG_RegisterDefaultBlasterShaders(void);

void CG_PrecacheScopes(void)
{
#if 0
	int sc = 0;

	for (sc = 0; sc < SCOPE_MAX_SCOPES; sc++) {
		if (strncmp(scopeData[sc].scopeModel, "", strlen(scopeData[sc].scopeModel))) trap->R_RegisterModel(scopeData[sc].scopeModel);
		if (strncmp(scopeData[sc].scopeModelShader, "", strlen(scopeData[sc].scopeModelShader))) trap->R_RegisterShader(scopeData[sc].scopeModelShader);
		if (strncmp(scopeData[sc].gunMaskShader, "", strlen(scopeData[sc].gunMaskShader))) trap->R_RegisterShader(scopeData[sc].gunMaskShader);
		if (strncmp(scopeData[sc].insertShader, "", strlen(scopeData[sc].insertShader))) trap->R_RegisterShader(scopeData[sc].insertShader);
		if (strncmp(scopeData[sc].maskShader, "", strlen(scopeData[sc].maskShader))) trap->R_RegisterShader(scopeData[sc].maskShader);
		if (strncmp(scopeData[sc].lightShader, "", strlen(scopeData[sc].lightShader))) trap->R_RegisterShader(scopeData[sc].lightShader);
		if (strncmp(scopeData[sc].tickShader, "", strlen(scopeData[sc].tickShader))) trap->R_RegisterShader(scopeData[sc].tickShader);
		if (strncmp(scopeData[sc].chargeShader, "", strlen(scopeData[sc].chargeShader))) trap->R_RegisterShader(scopeData[sc].chargeShader);
		if (strncmp(scopeData[sc].zoomStartSound, "", strlen(scopeData[sc].zoomStartSound))) trap->S_RegisterSound(scopeData[sc].zoomStartSound);
		if (strncmp(scopeData[sc].zoomEndSound, "", strlen(scopeData[sc].zoomEndSound))) trap->S_RegisterSound(scopeData[sc].zoomEndSound);
	}
#endif
}

static qboolean CG_IsGhoul2Model(const char *modelPath)
{
	size_t len = strlen(modelPath);
	if (len <= 4)
	{
		return qfalse;
	}

	if (Q_stricmp(modelPath + len - 4, ".glm") == 0)
	{
		return qtrue;
	}

	return qfalse;
}

static void CG_LoadViewWeapon(weaponInfo_t *weapon, const char *modelPath)
{
	char file[MAX_QPATH];
	char *slash;
	//int root;
	//int model;

	Q_strncpyz(file, modelPath, sizeof(file));
	slash = Q_strrchr(file, '/');

	Q_strncpyz(slash, "/model_default.skin", sizeof(file) - (slash - file));
	weapon->viewModelSkin = trap->R_RegisterSkin(file);

	trap->G2API_InitGhoul2Model(&weapon->g2ViewModel, modelPath, 0, weapon->viewModelSkin, 0, 0, 0);
	if (!trap->G2_HaveWeGhoul2Models(weapon->g2ViewModel))
	{
		return;
	}

	/*memset(weapon->viewModelAnims, 0, sizeof(weapon->viewModelAnims));
	trap->G2API_GetGLAName(weapon->g2ViewModel, 0, file);
	if (!file[0])
	{
		return;
	}

	slash = Q_strrchr(file, '/');

	Q_strncpyz(slash, "/animation.cfg", sizeof(file) - (slash - file));
	CG_LoadViewWeaponAnimations(weapon, file);*/
}

void CG_SetWeaponHandModel(weaponInfo_t    *weaponInfo, int weaponType)
{
	if (weaponType == WEAPONTYPE_PISTOL)
		weaponInfo->handsModel = trap->R_RegisterModel("models/weapons2/briar_pistol/briar_pistol_hand.md3");
	else if (weaponType == WEAPONTYPE_BLASTER || weaponType == WEAPONTYPE_NONE)
		weaponInfo->handsModel = trap->R_RegisterModel("models/weapons2/blaster_r/blaster_hand.md3");
	else if (weaponType == WEAPONTYPE_SNIPER)
		weaponInfo->handsModel = trap->R_RegisterModel("models/weapons2/disruptor/disruptor_hand.md3");
	else if (weaponType == WEAPONTYPE_ROCKET_LAUNCHER)
		weaponInfo->handsModel = trap->R_RegisterModel("models/weapons2/merr_sonn/merr_sonn_hand.md3");
	else if (weaponType == WEAPONTYPE_GRENADE)
		weaponInfo->handsModel = trap->R_RegisterModel("models/weapons2/thermal/thermal_hand.md3");
}

/*
=================
CG_RegisterWeapon

The server says this item is used on this level
=================
*/
void CG_RegisterWeapon( int weaponNum) {
	weaponInfo_t	*weaponInfo;
	gitem_t			*item, *ammo;
	char			path[MAX_QPATH];
	vec3_t			mins, maxs;
	int				i;
	char			extensionlessModel[MAX_QPATH];

	if (weaponNum < WP_NONE) 
	{// Missing SP weapons...
		return;
	}

	// Always register all the blaster bolt colors...
	CG_RegisterDefaultBlasterShaders();

	weaponInfo = &cg_weapons[weaponNum];

	if ( weaponNum == 0 ) {
		return;
	}

	if ( weaponInfo->registered ) {
		return;
	}

	memset( weaponInfo, 0, sizeof( *weaponInfo ) );
	weaponInfo->registered = qtrue;

	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( item->giType == IT_WEAPON && item->giTag == weaponNum ) {
			weaponInfo->item = item;
			break;
		}
	}

	if ( !item->classname ) {
		trap->Error( ERR_DROP, "Couldn't find weapon %i", weaponNum );
		return;
	}

	CG_RegisterItemVisuals( item - bg_itemlist );

	// UQ1: Set all names in the bg_weapons.c struct and copy it here... Don't set them below...
	if (weaponInfo->item)
		weaponInfo->item->classname = weaponData[weaponNum].classname;

	// Clear gunPosition...
	VectorClear(weaponInfo->gunPosition);

	weaponInfo->viewModel = NULL_HANDLE;
	if (weaponInfo->g2ViewModel)
	{
		trap->G2API_CleanGhoul2Models(&weaponInfo->g2ViewModel);
		weaponInfo->g2ViewModel = NULL;
	}

	//trap->Print("^3Loading viewModel %s.\n", item->view_model);
	if (CG_IsGhoul2Model(item->view_model))
	{
		//trap->Print("^2Loaded glm viewModel for %s.\n", item->view_model);
		CG_LoadViewWeapon(weaponInfo, item->view_model);
	}
	else
	{
		// load in-view model also
		weaponInfo->viewModel = trap->R_RegisterModel(item->view_model);
	}
	
	// load cmodel before model so filecache works
	weaponInfo->weaponModel = trap->R_RegisterModel(item->world_model[0]);


	memset (weaponInfo->barrelModels, NULL_HANDLE, sizeof (weaponInfo->barrelModels));
    COM_StripExtension (item->view_model, extensionlessModel, sizeof(extensionlessModel));
    
    for ( i = 0; i < 4; i++ )
    {
        const char *barrelModel;
        int len;
        qhandle_t barrel;

		if( i == 0 )
			barrelModel = va("%s_barrel.md3", extensionlessModel);
		else
			barrelModel = va("%s_barrel%i.md3", extensionlessModel, i+1);

		len = strlen( barrelModel );
        
        if ( (len + 1) > MAX_QPATH )
        {
			trap->Print(S_COLOR_YELLOW "Warning: barrel model path %s is too long (%d chars). Max length is 63.\n", barrelModel, len);
            break;
        }
        
        barrel = trap->R_RegisterModel (barrelModel);

        if ( barrel == NULL_HANDLE )
        {
            break;
        }
        
        weaponInfo->barrelModels[i] = barrel;
		weaponInfo->barrelCount++;
    }
	

	// calc midpoint for rotation
	trap->R_ModelBounds( weaponInfo->weaponModel, mins, maxs );
	for ( i = 0 ; i < 3 ; i++ ) {
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * ( maxs[i] - mins[i] );
	}

	weaponInfo->weaponIcon = trap->R_RegisterShader( item->icon );
	weaponInfo->ammoIcon = trap->R_RegisterShader( item->icon );

	for ( ammo = bg_itemlist + 1 ; ammo->classname ; ammo++ ) {
		if ( ammo->giType == IT_AMMO && ammo->giTag == weaponNum ) {
			break;
		}
	}
	if ( ammo->classname && ammo->world_model[0] ) {
		weaponInfo->ammoModel = trap->R_RegisterModel( ammo->world_model[0] );
	}

//	strcpy( path, item->view_model );
//	COM_StripExtension( path, path );
//	strcat( path, "_flash.md3" );
	weaponInfo->flashModel = NULL;
	weaponInfo->barrelModel = 0;

	if (weaponNum != WP_SABER)
	{
		Q_strncpyz( path, item->view_model, sizeof(path) );
		COM_StripExtension( path, path, sizeof( path ) );
		Q_strcat( path, sizeof(path), "_hand.md3" );
		weaponInfo->handsModel = trap->R_RegisterModel( path );

#if 0
		if (!weaponInfo->handsModel)
		{
			if (WeaponIsGrenade(weaponNum))
			{
				//CG_SetWeaponHandModel(weaponInfo, WEAPONTYPE_GRENADE);
				weaponInfo->handsModel = NULL;
			}
			else if (WeaponIsRocketLauncher(weaponNum))
			{
				//CG_SetWeaponHandModel(weaponInfo, WEAPONTYPE_ROCKET_LAUNCHER);
				weaponInfo->handsModel = NULL;
			}
			else if (WeaponIsSniper(weaponNum))
			{
				CG_SetWeaponHandModel(weaponInfo, WEAPONTYPE_SNIPER);
			}
			else if (WeaponIsPistol(weaponNum))
			{
				//CG_SetWeaponHandModel(weaponInfo, WEAPONTYPE_PISTOL);
				weaponInfo->handsModel = NULL;
			}
			else
			{
				CG_SetWeaponHandModel(weaponInfo, WEAPONTYPE_BLASTER);
			}
		}
#endif
	}
	else
	{
		weaponInfo->handsModel = NULL;
	}

	/*
	// Debugging...
	weaponInfo->bolt3DShader = cgs.media.redBlasterShot; // Setting this enables 3D bolts for this gun, using this color shader...
	weaponInfo->bolt3DShaderAlt = cgs.media.redBlasterShot; // Setting this enables 3D bolts for this gun's alt fire, using this color shader...
	weaponInfo->bolt3DLength = 1.0; // If not set, 1.0 is the default length.
	weaponInfo->bolt3DLengthAlt = 1.25; // If not set, 1.0 is the default length.
	weaponInfo->bolt3DWidth = 1.0; // If not set, 1.0 is the default length.
	weaponInfo->bolt3DWidthAlt = 1.25; // If not set, 1.0 is the default length.
	*/

	// Init this weapon's bolt 3d... So it falls back to old efx if not set below...
	weaponInfo->bolt3DShader = -1; // Setting this enables 3D bolts for this gun, using this color shader...
	weaponInfo->bolt3DShaderAlt = -1; // Setting this enables 3D bolts for this gun's alt fire, using this color shader...
	weaponInfo->bolt3DLength = 0; // If not set, 1.0 is the default length.
	weaponInfo->bolt3DLengthAlt = 0; // If not set, 1.0 is the default length.
	weaponInfo->bolt3DWidth = 0; // If not set, 1.0 is the default length.
	weaponInfo->bolt3DWidthAlt = 0; // If not set, 1.0 is the default length.

	switch ( weaponNum ) {
	case WP_MELEE:
		trap->FX_RegisterEffect( "stunBaton/flesh_impact" );
		break;
	case WP_SABER:
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->firingSound = trap->S_RegisterSound( "sound/weapons/saber/saberhum1.wav" );
		weaponInfo->missileModel		= trap->R_RegisterModel( "models/weapons2/saber/saber_w.glm" );
		break;

	case WP_FRAG_GRENADE:
		weaponInfo->selectSound = trap->S_RegisterSound("sound/weapons/select_grenade.mp3");
		weaponInfo->flashSound[0] = trap->S_RegisterSound("sound/weapons/thermal/fire.wav");
		weaponInfo->firingSound = NULL_SOUND;
		weaponInfo->chargeSound = trap->S_RegisterSound("sound/weapons/grenade_cook.mp3");
		weaponInfo->muzzleEffect = NULL_FX;
		weaponInfo->missileModel = trap->R_RegisterModel("models/weapons3/fraggrenade/thermal_proj.md3");
		weaponInfo->missileSound = NULL_SOUND;
		weaponInfo->missileDlight = 0;
		weaponInfo->missileHitSound = NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_ThermalProjectileThink;
		weaponInfo->altFlashSound[0] = trap->S_RegisterSound("sound/weapons/thermal/fire.wav");
		weaponInfo->altFiringSound = NULL_SOUND;
		weaponInfo->altChargeSound = trap->S_RegisterSound("sound/weapons/grenade_cook.mp3");
		weaponInfo->altMuzzleEffect = NULL_FX;
		weaponInfo->altMissileModel = trap->R_RegisterModel("models/weapons3/fraggrenade/thermal_proj.md3");
		weaponInfo->altMissileSound = NULL_SOUND;
		weaponInfo->altMissileDlight = 0;
		weaponInfo->altMissileHitSound = NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_PulseGrenadeProjectileThink;
										//  X,	 ,Y	   ,Z	
		VectorSet(weaponInfo->gunPosition, 8.0, 10.0, -5.5);
		weaponInfo->missileWallImpactfx = trap->FX_RegisterEffect("weapons/grenaderibbon_red");
		weaponInfo->altMissileWallImpactfx = trap->FX_RegisterEffect("weapons/grenaderibbon_red");

		weaponInfo->fleshImpactEffect					= trap->FX_RegisterEffect("explosives/fragmedium");
		weaponInfo->altFleshImpactEffect				= trap->FX_RegisterEffect("explosives/fragmedium");
		weaponInfo->missileWallImpactfx					= trap->FX_RegisterEffect("explosives/fragmedium");
		weaponInfo->altMissileWallImpactfx				= trap->FX_RegisterEffect("explosives/fragmedium");
		

		cgs.media.grenadeBounce1 = trap->S_RegisterSound("sound/weapons/grenade_bounce1.mp3");
		cgs.media.grenadeBounce2 = trap->S_RegisterSound("sound/weapons/grenade_bounce2.mp3");
		trap->S_RegisterSound("sound/weapons/grenade_thermdetloop.mp3");
		break;

	case WP_MODULIZED_WEAPON:
		weaponInfo->bolt3DShader = cgs.media.redBlasterShot; // Setting this enables 3D bolts for this gun, using this color shader...
		weaponInfo->bolt3DShaderAlt = cgs.media.redBlasterShot; // Setting this enables 3D bolts for this gun's alt fire, using this color shader...
		weaponInfo->bolt3DLength = 1.0; // If not set, 1.0 is the default length.
		weaponInfo->bolt3DLengthAlt = 1.25; // If not set, 1.0 is the default length.
		weaponInfo->bolt3DWidth = 1.0; // If not set, 1.0 is the default length.
		weaponInfo->bolt3DWidthAlt = 1.25; // If not set, 1.0 is the default length.

		weaponInfo->selectSound = trap->S_RegisterSound("sound/weapons/select_carbine.mp3");
		weaponInfo->flashSound[0] = trap->S_RegisterSound("sound/weapons/blasters/bryar_rifle1.mp3");
		weaponInfo->flashSound[1] = trap->S_RegisterSound("sound/weapons/blasters/bryar_rifle2.mp3");
		weaponInfo->flashSound[2] = trap->S_RegisterSound("sound/weapons/blasters/bryar_rifle3.mp3");
		weaponInfo->flashSound[3] = trap->S_RegisterSound("sound/weapons/blasters/bryar_rifle4.mp3");
		weaponInfo->firingSound = NULL_SOUND;
		weaponInfo->chargeSound = NULL_SOUND;
		weaponInfo->muzzleEffect = trap->FX_RegisterEffect("blasters/muzzleflash2_Red_medium");
		weaponInfo->missileModel = NULL_HANDLE;
		weaponInfo->missileSound = NULL_SOUND;
		weaponInfo->missileDlight = 0;
		weaponInfo->missileHitSound = NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_WeaponProjectileThink;
		weaponInfo->powerupShotRenderfx = NULL_FX;
		weaponInfo->altFlashSound[0] = trap->S_RegisterSound("sound/weapons/blasters/bryar_rifle1.mp3");
		weaponInfo->altFlashSound[1] = trap->S_RegisterSound("sound/weapons/blasters/bryar_rifle2.mp3");
		weaponInfo->altFlashSound[2] = trap->S_RegisterSound("sound/weapons/blasters/bryar_rifle3.mp3");
		weaponInfo->altFlashSound[3] = trap->S_RegisterSound("sound/weapons/blasters/bryar_rifle4.mp3");
		weaponInfo->altFiringSound = NULL_SOUND;
		weaponInfo->altChargeSound = NULL_SOUND;
		weaponInfo->altMuzzleEffect = trap->FX_RegisterEffect("blasters/muzzleflash2_Red_medium");
		weaponInfo->altMissileModel = NULL_HANDLE;
		weaponInfo->altMissileSound = NULL_SOUND;
		weaponInfo->altMissileDlight = 0;
		weaponInfo->altMissileHitSound = NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_WeaponAltProjectileThink;
										//  X,	 ,Y	   ,Z	
		VectorSet(weaponInfo->gunPosition, 10.0, - 3.0, - 5.4);

		weaponInfo->missileRenderfx = trap->FX_RegisterEffect("blasters/shot_redorange_medium");
		weaponInfo->altMissileRenderfx = trap->FX_RegisterEffect("blasters/shot_redorange_medium");

		weaponInfo->fleshImpactEffect					= trap->FX_RegisterEffect("warzone_modular_weapons/blaster_impact_flesh");
		weaponInfo->altFleshImpactEffect				= trap->FX_RegisterEffect("warzone_modular_weapons/blaster_impact_flesh");
		weaponInfo->missileWallImpactfx					= trap->FX_RegisterEffect("warzone_modular_weapons/blaster_impact");
		weaponInfo->altMissileWallImpactfx				= trap->FX_RegisterEffect("warzone_modular_weapons/blaster_impact");


		//trap->FX_RegisterEffect("warzone_modular_weapons/blaster_deflect");
		break;

	case WP_CYROBAN_GRENADE:
		weaponInfo->selectSound = trap->S_RegisterSound("sound/weapons/thermal/select.wav");
		weaponInfo->flashSound[0] = trap->S_RegisterSound("sound/weapons/melee/swing1.mp3");
		weaponInfo->flashSound[1] = trap->S_RegisterSound("sound/weapons/melee/swing2.mp3");
		weaponInfo->flashSound[2] = trap->S_RegisterSound("sound/weapons/melee/swing3.mp3");
		weaponInfo->flashSound[3] = trap->S_RegisterSound("sound/weapons/melee/swing4.mp3");
		weaponInfo->firingSound = NULL_SOUND;
		weaponInfo->chargeSound = trap->S_RegisterSound("sound/weapons/thermal/charge.wav");
		weaponInfo->muzzleEffect = NULL_FX;
		weaponInfo->missileModel = trap->R_RegisterModel("models/weapons/Grenade_CryoBan/model_proj.md3");
		weaponInfo->missileSound = NULL_SOUND;
		weaponInfo->missileDlight = 0;
		weaponInfo->missileHitSound = NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_ThermalProjectileThink;;
		weaponInfo->altFlashSound[0] = trap->S_RegisterSound("sound/weapons/melee/swing1.mp3");
		weaponInfo->altFlashSound[1] = trap->S_RegisterSound("sound/weapons/melee/swing2.mp3");
		weaponInfo->altFlashSound[2] = trap->S_RegisterSound("sound/weapons/melee/swing3.mp3");
		weaponInfo->altFlashSound[3] = trap->S_RegisterSound("sound/weapons/melee/swing4.mp3");
		weaponInfo->altFiringSound = NULL_SOUND;
		weaponInfo->altChargeSound = trap->S_RegisterSound("sound/weapons/grenade_cook.wav");
		weaponInfo->altMuzzleEffect = NULL_FX;
		weaponInfo->altMissileModel = trap->R_RegisterModel("models/weapons/Grenade_CryoBan/model_proj.md3");
		weaponInfo->altMissileSound = NULL_SOUND;
		weaponInfo->altMissileDlight = 0;
		weaponInfo->altMissileHitSound = NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_PulseGrenadeProjectileThink;
										//  X,	 ,Y	   ,Z	
		VectorSet(weaponInfo->gunPosition, 8.0, - 2.0, - 3.0);

		weaponInfo->missileWallImpactfx = trap->FX_RegisterEffect("weapons/grenaderibbon_red");
		weaponInfo->altMissileWallImpactfx = trap->FX_RegisterEffect("weapons/grenaderibbon_red");

		weaponInfo->fleshImpactEffect					= trap->FX_RegisterEffect("explosives/cryobanmedium");
		weaponInfo->altFleshImpactEffect				= trap->FX_RegisterEffect("explosives/cryobanmedium");
		weaponInfo->missileWallImpactfx					= trap->FX_RegisterEffect("explosives/cryobanmedium");
		weaponInfo->altMissileWallImpactfx				= trap->FX_RegisterEffect("explosives/cryobanmedium");


		cgs.media.grenadeBounce1 = trap->S_RegisterSound("sound/weapons/grenade_bounce1.mp3");
		cgs.media.grenadeBounce2 = trap->S_RegisterSound("sound/weapons/grenade_bounce2.mp3");
		trap->S_RegisterSound("sound/weapons/grenade_thermdetloop.mp3");
		trap->S_RegisterSound("sound/weapons/thermal/warning.wav");
		break;

	case WP_THERMAL:
		weaponInfo->selectSound			= trap->S_RegisterSound("sound/weapons/thermal/select.wav");
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/melee/swing1.mp3");
		weaponInfo->flashSound[1]		= trap->S_RegisterSound("sound/weapons/melee/swing2.mp3");
		weaponInfo->flashSound[2]		= trap->S_RegisterSound("sound/weapons/melee/swing3.mp3");
		weaponInfo->flashSound[3]		= trap->S_RegisterSound("sound/weapons/melee/swing4.mp3");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= trap->S_RegisterSound( "sound/weapons/thermal/charge.wav");
		weaponInfo->muzzleEffect		= NULL_FX;
		weaponInfo->missileModel		= trap->R_RegisterModel( "models/weapons/Grenade_Thermal/model_proj.md3" );
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_ThermalProjectileThink;;
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/melee/swing1.mp3");
		weaponInfo->altFlashSound[1]	= trap->S_RegisterSound("sound/weapons/melee/swing2.mp3");
		weaponInfo->altFlashSound[2]	= trap->S_RegisterSound("sound/weapons/melee/swing3.mp3");
		weaponInfo->altFlashSound[3]	= trap->S_RegisterSound("sound/weapons/melee/swing4.mp3");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= trap->S_RegisterSound( "sound/weapons/grenade_cook.wav");
		weaponInfo->altMuzzleEffect		= NULL_FX;
		weaponInfo->altMissileModel		= trap->R_RegisterModel( "models/weapons/Grenade_Thermal/model_proj.md3" );
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_PulseGrenadeProjectileThink;
										//  X,	 ,Y	   ,Z	
		VectorSet(weaponInfo->gunPosition, 8.0, - 2.0, - 3.0);

		weaponInfo->missileWallImpactfx = trap->FX_RegisterEffect("weapons/grenaderibbon_red");
		weaponInfo->altMissileWallImpactfx = trap->FX_RegisterEffect("weapons/grenaderibbon_red");

		weaponInfo->fleshImpactEffect					= trap->FX_RegisterEffect("explosives/baradium_class-e");
		weaponInfo->altFleshImpactEffect				= trap->FX_RegisterEffect("explosives/shockwave");
		weaponInfo->missileWallImpactfx					= trap->FX_RegisterEffect("explosives/baradium_class-e");
		weaponInfo->altMissileWallImpactfx				= trap->FX_RegisterEffect("explosives/shockwave");

		cgs.media.grenadeBounce1 = trap->S_RegisterSound("sound/weapons/grenade_bounce1.mp3");
		cgs.media.grenadeBounce2 = trap->S_RegisterSound("sound/weapons/grenade_bounce2.mp3");
		trap->S_RegisterSound("sound/weapons/grenade_thermdetloop.mp3");
		trap->S_RegisterSound( "sound/weapons/thermal/warning.wav" );
		break;

	case WP_TRIP_MINE:
		weaponInfo->selectSound			= trap->S_RegisterSound("sound/weapons/detpack/select.wav");
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/laser_trap/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= NULL_FX;
		weaponInfo->missileModel		= trap->R_RegisterModel("models/Weapons/Mine_LaserTrip/projectile.md3");
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= 0;
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/laser_trap/fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= NULL_FX;
		weaponInfo->altMissileModel		= trap->R_RegisterModel("models/Weapons/Mine_LaserTrip/projectile.md3");
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = 0;
										//  X,	 ,Y	   ,Z	
		VectorSet(weaponInfo->gunPosition, 8.0, - 2.0, - 3.0);

		cgs.effects.tripmineLaserFX = trap->FX_RegisterEffect("weapons/laser_blue.efx");
		cgs.effects.tripmineGlowFX = trap->FX_RegisterEffect("tripMine/glowbit.efx");
		trap->FX_RegisterEffect( "explosives/demosmall" );
		// NOTENOTE temp stuff
		trap->S_RegisterSound( "sound/weapons/laser_trap/stick.wav" );
		trap->S_RegisterSound( "sound/weapons/laser_trap/warning.wav" );
		break;

	case WP_DET_PACK:
		weaponInfo->selectSound			= trap->S_RegisterSound("sound/weapons/detpack/select.wav");
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/detpack/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= NULL_FX;
		weaponInfo->missileModel		= trap->R_RegisterModel( "models/weapons2/detpack/det_pack.md3" );
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= 0;
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/detpack/fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= NULL_FX;
		weaponInfo->altMissileModel		= trap->R_RegisterModel( "models/weapons2/detpack/det_pack.md3" );
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = 0;
										//  X,	 ,Y	   ,Z	
		VectorSet(weaponInfo->gunPosition, 10.0, -2.0, -3.4);
		trap->R_RegisterModel( "models/weapons2/detpack/det_pack.md3" );
		trap->S_RegisterSound( "sound/weapons/detpack/stick.wav" );
		trap->S_RegisterSound( "sound/weapons/detpack/warning.wav" );
		trap->S_RegisterSound( "sound/weapons/explosions/explode5.wav" );
		break;

	case WP_TURRET:
		weaponInfo->flashSound[0]		= NULL_SOUND;
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= NULL_HANDLE;
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_WeaponProjectileThink;
		

		weaponInfo->missileRenderfx = trap->FX_RegisterEffect("blaster/shot");
		weaponInfo->altMissileRenderfx = trap->FX_RegisterEffect("blaster/shot");

		weaponInfo->fleshImpactEffect = trap->FX_RegisterEffect("warzone_modular_weapons/blaster_impact_flesh");
		weaponInfo->altFleshImpactEffect = trap->FX_RegisterEffect("warzone_modular_weapons/blaster_impact_flesh");
		weaponInfo->missileWallImpactfx = trap->FX_RegisterEffect("warzone_modular_weapons/blaster_impact");
		weaponInfo->altMissileWallImpactfx = trap->FX_RegisterEffect("warzone_modular_weapons/blaster_impact");

		//trap->FX_RegisterEffect("warzone_modular_weapons/blaster_deflect");
		break;
	 default:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 1 );
		weaponInfo->flashSound[0] = trap->S_RegisterSound( "sound/weapons/rocket/rocklf1a.wav" );
		break;
	}
}
