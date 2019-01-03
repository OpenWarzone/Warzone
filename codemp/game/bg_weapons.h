#pragma once

// Filename:-	bg_weapons.h
//
// This crosses both client and server.  It could all be crammed into bg_public, but isolation of this type of data is best.

#ifndef WEAPONS_H
#define WEAPONS_H

typedef enum {
	WP_NONE,

	// =================================================================
	//
	// Special weapons - do not add normal guns or melee weapons here...
	//
	// =================================================================

	WP_EMPLACED_GUN,
	WP_TURRET,

	// =================================================================
	//
	// Grenades... Unavailable for now - later on G key...
	//
	// =================================================================

	WP_THERMAL,
	WP_FRAG_GRENADE,
	WP_CYROBAN_GRENADE,
	WP_TRIP_MINE,
	WP_DET_PACK,

	// =================================================================
	//
	// Usable weapons...
	//
	// =================================================================

	WP_MELEE,
	WP_FIRST_USEABLE = WP_MELEE,

	WP_SABER,
	WP_NUM_MELEE_WEAPONS = WP_SABER,

	WP_MODULIZED_WEAPON,
	WP_NUM_USEABLE = WP_MODULIZED_WEAPON,

	WP_ALL_WEAPONS,
	WP_NUM_WEAPONS = WP_ALL_WEAPONS
} weapon_t;


typedef enum //# ammo_e
{
	AMMO_NONE,
	AMMO_FORCE,		// AMMO_PHASER
	AMMO_BLASTER,	// AMMO_STARFLEET,
	AMMO_POWERCELL,	// AMMO_ALIEN,
	AMMO_METAL_BOLTS,
	AMMO_ROCKETS,
	AMMO_EMPLACED,
	AMMO_THERMAL,
	AMMO_TRIPMINE,
	AMMO_DETPACK,
	AMMO_MAX
} ammo_t;

typedef enum firingType_e
{
	FT_AUTOMATIC,
	FT_SEMI,
	FT_BURST
} firingType_t;

typedef struct weaponData_s
{
	char	classname[32];		// Spawning name

	int		fireTime;			// Amount of time between firings
	int		altFireTime;		// Amount of time between alt-firings

	firingType_t    firingType;

	short   burstFireDelay;     // Delay between firing in a burst.
	
	char    shotsPerBurst;      // Shots per burst
					
	int		dmg;
	int		dmgAlt;

	int		boltSpeed;
	float	accuracy;

	int		splashDmg;
	int		splashRadius;

	int		chargeSubTime;		// ms interval for subtracting ammo during charge
	int		altChargeSubTime;	// above for secondary

	int		altChargeSub;		// above for secondary
} weaponData_t;


extern weaponData_t weaponData[WP_NUM_WEAPONS];


// Specific weapon information
#define DEFAULT_SHOTGUN_SPREAD	700
#define DEFAULT_SHOTGUN_COUNT	11

#define	LIGHTNING_RANGE		768

typedef enum { // UQ1: Used for hands model selection... Could be used for other stuff later I guess...
	WEAPONTYPE_NONE,
	WEAPONTYPE_PISTOL,
	WEAPONTYPE_BLASTER,
	WEAPONTYPE_SNIPER,
	WEAPONTYPE_ROCKET_LAUNCHER,
	WEAPONTYPE_GRENADE,
} weaponTypes_t;

typedef enum {
	SCOPE_NONE,
	SCOPE_BINOCULARS,
	SCOPE_IRONSIGHT_SHORT,
	SCOPE_IRONSIGHT_MID,
	SCOPE_DISRUPTOR,
	SCOPE_BOWCASTER, // these should be renamed to some "scope company/model names"
	SCOPE_EE3_BLASTTECH_SHORT, // rename this - eg: SCOPE_CLASS_BLASTECH_EE3_SHORT or whatever
	SCOPE_A280_BLASTTECH_LONG_SHORT, // rename this
	SCOPE_DLT20A_BLASTTECH_LONG_SHORT, // rename this
	SCOPE_ACP_SNIPER, // Add anything you want. rename as you want.
	SCOPE_BOWCASTER_CLASSIC,
	SCOPE_WOOKIE_BOWCASTER,
	SCOPE_BRYAR_RIFLE_SCOPE,
	SCOPE_DH_17_PISTOL,
	SCOPE_MAX_SCOPES
} scopeTypes_t;

typedef struct scopeData_s
{
	char	scopename[64];
	char	scopeModel[128];		// For the md3 we attach to tag_scope later
	char	scopeModelShader[128];	// For alternate shader to use on attached scope later
	char	gunMaskShader[128];
	char	maskShader[128];
	char	insertShader[128];
	char	lightShader[128];
	char	tickShader[128];
	char	chargeShader[128];
	int		scopeViewX;				// X position of the view inside the scope.
	int		scopeViewY;				// Y position of the view inside the scope.
	int		scopeViewW;				// Width of the view inside the scope.
	int		scopeViewH;				// Height of the view inside the scope.
	char	zoomStartSound[128];
	char	zoomEndSound[128];
	int		instantZoom;			// Should this scope instantly zoom in/out? really a qboolean but it looks like qboolean is not set here. int == qboolean anyway
	float	scopeZoomMin;			// Minimum Zoom Range.
	float	scopeZoomMax;			// Maximum Zoom Range.
	float	scopeZoomSpeed;			// How fast this scope zooms in/out.
} scopeData_t;

extern scopeData_t scopeData[];

/*

*/

#endif //WEAPONS_H
