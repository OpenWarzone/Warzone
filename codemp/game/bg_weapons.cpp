// Copyright (C) 2001-2002 Raven Software
//
// bg_weapons.c -- part of bg_pmove functionality

#include "qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

qboolean BG_HaveWeapon ( const playerState_t *ps, int weapon );

//ready for 2 handed pistols when ready for it. -- UQ1: Probably no longer needed - needs to be tested when we have 2 pistols...
vec3_t WP_FirstPistolMuzzle[WP_NUM_WEAPONS] = 
{//	Fwd,	right,	up.
	{ 12,	6,		0	},

};
vec3_t WP_SecondPistolMuzzle[WP_NUM_WEAPONS] = 
{//	Fwd,	right,	up.
	{ 12,  -6,		0	},

};

#define DEFAULT_DAMAGE				3
#define PISTOL_DMG					5//10
#define BLASTER_DAMAGE				10//20
#define ROCKET_DAMAGE				60
#define GRENADE_DAMAGE				70

#define DEFAULT_SPEED				3400//2300
#define PISTOL_SPEED				1600
#define BLASTER_SPEED				DEFAULT_SPEED
#define GRENADE_SPEED				900

#define DEFAULT_ACCURACY			1.0f
#define PISTOL_ACCURACY				1.0f
#define BLASTER_ACCURACY			1.6f

#define DEFAULT_BURST_SHOT			0
#define BURST_SLOW_LOW				2
#define BURST_SHOT_MID				3
#define BURST_SHOT_HIGH				5

#define DEFAULT_BURST_DELAY			0
#define LOW_BURST_DELAY				175	
#define MID_BURST_DELAY				150
#define HIGH_BURST_DELAY			100

//#define	WFT_NORMAL					(firingType_t)0
#define	WFT_AUTO					(firingType_t)0
#define	WFT_SEMI					(firingType_t)1
#define	WFT_BURST					(firingType_t)2


//New clean weapon table NOTE* need to remeber to put the WP_NAME same place as you have added it in BG_weapon.h else it gets messed up in the weapon table
weaponData_t weaponData[WP_NUM_WEAPONS] = {
	// char	classname[32];							fireTime	altFireTime		firingType			burstFireDelay			  shotsPerBurst			dmg				dmgAlt				boltSpeed			accuracy				splashDmg		splashRadius	chargeSubTime	altChargeSubTime	altChargeSub
	{ "No Weapon",									0,			0,				WFT_AUTO,			DEFAULT_BURST_DELAY,	  DEFAULT_BURST_SHOT,	0,				0,					0,					0,						0,				0,				0,				0,					0,			},

	{ "Emplaced Gun",								100,		100,			WFT_AUTO,			DEFAULT_BURST_DELAY,	  DEFAULT_BURST_SHOT,	BLASTER_DAMAGE, BLASTER_DAMAGE,		BLASTER_SPEED,		BLASTER_ACCURACY,		0,				0,				0,				0,					0,			},
	{ "Turret",										100,		100,			WFT_AUTO,			DEFAULT_BURST_DELAY,	  DEFAULT_BURST_SHOT,	BLASTER_DAMAGE, BLASTER_DAMAGE,		BLASTER_SPEED,		BLASTER_ACCURACY,		0,				0,				0,				0,					0,			},

	{ "Thermal Detonator",							800,		400,			WFT_AUTO,			DEFAULT_BURST_DELAY,	  DEFAULT_BURST_SHOT,	GRENADE_DAMAGE,	GRENADE_DAMAGE,		GRENADE_SPEED,		0,						90,				128,			0,				0,					0, },
	{ "Frag Grenade",								800,		400,			WFT_AUTO,			DEFAULT_BURST_DELAY,	  DEFAULT_BURST_SHOT,	GRENADE_DAMAGE,	GRENADE_DAMAGE,		GRENADE_SPEED,		0,						90,				128,			0,				0,					0, },
	{ "CryoBan Grenade",							800,		400,			WFT_AUTO,			DEFAULT_BURST_DELAY,	  DEFAULT_BURST_SHOT,	GRENADE_DAMAGE,	GRENADE_DAMAGE,		GRENADE_SPEED,		0,						90,				128,			0,				0,					0, },
	{ "Trip Mine",									800,		400,			WFT_AUTO,			DEFAULT_BURST_DELAY,	  DEFAULT_BURST_SHOT,	GRENADE_DAMAGE,	GRENADE_DAMAGE,		GRENADE_SPEED,		0,						0,				0,				0,				0,					0, },
	{ "Det Pack",									800,		400,			WFT_AUTO,			DEFAULT_BURST_DELAY,	  DEFAULT_BURST_SHOT,	GRENADE_DAMAGE,	GRENADE_DAMAGE,		GRENADE_SPEED,		0,						0,				0,				0,				0,					0, },

	{ "Melee",										400,		400,			WFT_AUTO,			DEFAULT_BURST_DELAY,	  DEFAULT_BURST_SHOT,	5,				5,					0,					0,						0,				0,				0,				0,					0, },

	{ "Light Saber",								100,		100,			WFT_AUTO,			DEFAULT_BURST_DELAY,	  DEFAULT_BURST_SHOT,	0,				0,					0,					0,						0,				0,				0,				0,					0, },
	{ "Modularised Weapon",							350,		250,			WFT_AUTO,			DEFAULT_BURST_DELAY,	  DEFAULT_BURST_SHOT,	BLASTER_DAMAGE, BLASTER_DAMAGE,		BLASTER_SPEED,		BLASTER_ACCURACY,		0,				0,				0,				200,				3, },
	
	};

qboolean WeaponIsGrenade(int weapon)
{
	if (weapon >= WP_THERMAL && weapon <= WP_DET_PACK)
	{
		return qtrue;
	}

	return qfalse;
}

qboolean WeaponIsRocketLauncher(int weapon)
{
	return qfalse;
}

qboolean WeaponIsPistol(int weapon)
{
	switch (weapon)
	{
	case WP_THERMAL:
	case WP_FRAG_GRENADE:
	case WP_CYROBAN_GRENADE:
		return qtrue;
	default:
		break;
	}

	return qfalse;
}

qboolean IsRollWithPistols(int weapon)
{
	switch (weapon)
	{
	case WP_THERMAL:
	case WP_FRAG_GRENADE:
	case WP_CYROBAN_GRENADE:
		return qtrue;
	default:
		break;
	}

	return qfalse;
}
qboolean WeaponIsSniper(int weapon)
{
	return qfalse;
}
//theses weapons have charge option if they not are listed under the WeaponIsSniperNoCharge function.
qboolean WeaponIsSniperCharge(int weapon)
{
	return qfalse;
}
// theses weapons only have scopes with no charge options.
qboolean WeaponIsSniperNoCharge ( int weapon )
{
	return qtrue;
}

qboolean BG_HaveWeapon ( const playerState_t *ps, int weapon )
{
	if (ps->primaryWeapon == weapon && weapon <= WP_NUM_USEABLE) return qtrue;
	if (ps->secondaryWeapon == weapon && weapon <= WP_NUM_USEABLE) return qtrue;
	if (ps->temporaryWeapon == weapon && weapon <= WP_NUM_USEABLE) return qtrue;
	if (ps->temporaryWeapon == WP_ALL_WEAPONS && weapon <= WP_NUM_USEABLE) return qtrue;
	if (weapon == WP_MELEE) return qtrue; // Everyone now gets melee... for now...

	return qfalse;
}

qboolean HaveWeapon ( playerState_t *ps, int weapon )
{
	if (ps->primaryWeapon == weapon && weapon <= WP_NUM_USEABLE) return qtrue;
	if (ps->secondaryWeapon == weapon && weapon <= WP_NUM_USEABLE) return qtrue;
	if (ps->temporaryWeapon == weapon && weapon <= WP_NUM_USEABLE) return qtrue;
	if (ps->temporaryWeapon == WP_ALL_WEAPONS && weapon <= WP_NUM_USEABLE) return qtrue;
	if (weapon == WP_MELEE) return qtrue; // Everyone now gets melee... for now...

	return qfalse;
}

// NOTE: "" means unused/ignore
scopeData_t scopeData[] = {
	// char	scopename[64],							char scopeModel[128],							char scopeModelShader[128],				char gunMaskShader[128],								char	maskShader[128],								char	insertShader[128],					char	lightShader[128],						char	tickShader[128],				char	chargeShader[128],				int scopeViewX,	int scopeViewY,	int scopeViewW,	int scopeViewH,		char	zoomStartSound[128],				char	zoomEndSound[128]					qboolean instantZoom,	float scopeZoomMin	float scopeZoomMax	float scopeZoomSpeed							
	"No Scope",										"",												"",										"",														"",														"",											"",												"",										"",										0,				0,				640,			480,				"",											"",											qtrue,					1.0f,				1.0f,				0.0f,							
	"Binoculars",									"",												"",										"",														"",														"",											"",												"",										"",										0,				0,				640,			480,				"sound/interface/zoomstart.wav",			"sound/interface/zoomend.wav",				qfalse,					1.5f,				3.0f,				0.1f,							
	"Short Range Viewfinder",						"",												"",										"",														"",														"",											"",												"",										"",										0,				0,				640,			480,				"",											"",											qtrue,					1.2f,				2.0f,				0.0f,							
	"Mid Range Viewfinder",							"",												"",										"",														"",														"",											"",												"",										"",										0,				0,				640,			480,				"",											"",											qtrue,					1.5f,				5.0f,				0.0f,							
	"Tenloss Disruptor Scope",						"",												"",										"",  													"gfx/2d/cropCircle2",									"gfx/2d/cropCircle",						"gfx/2d/cropCircleGlow",						"gfx/2d/insertTick",					"gfx/2d/crop_charge",					128,			50,				382,			382,				"sound/weapons/disruptor/zoomstart.wav",	"sound/weapons/disruptor/zoomend.wav",		qfalse,					3.0f,				3.0f,				0.2f,							
	"Blastech Bowcaster Scope",						"",												"",										"",														"gfx/2d/bowMask",										"gfx/2d/bowInsert",							"",												"",										"",										0,				0,				0,				0,					"sound/weapons/disruptor/zoomstart.wav",	"sound/weapons/disruptor/zoomend.wav",		qfalse,					1.2f,				1.2f,				0.4f,							
	"Blastech EE3 Scope",							"",												"",										"",														"gfx/2d/fett/cropCircle2",								"gfx/2d/fett/cropCircle",					"gfx/2d/fett/cropCircleGlow",					"",										"gfx/2d/fett/crop_charge",				0,				0,				640,			480,				"sound/weapons/disruptor/zoomstart.wav",	"sound/weapons/disruptor/zoomend.wav",		qfalse,					1.5f,				1.5f,				0.3f,							
	"Blastech A280 Scope",							"",												"",										"gfx/2D/arcMask",										"gfx/2d/a280cropCircle2",								"gfx/2d/a280cropCircle",					"",												"",										"",										134,			56,				364,			364,				"sound/weapons/disruptor/zoomstart.wav",	"sound/weapons/disruptor/zoomend.wav",		qfalse,					1.5f,				1.5f,				0.2f,							
	"Blastech DLT 19 Scope",						"",												"",										"gfx/2d/DLT-19_HeavyBlaster/scope_mask_overlay",		"gfx/2d/DLT-19_HeavyBlaster/scope_mask",				"gfx/2d/a280cropCircle",					"",												"",										"",										134,			56,				364,			364,				"sound/weapons/disruptor/zoomstart.wav",	"sound/weapons/disruptor/zoomend.wav",		qfalse,					2.5f,				2.5f,				0.2f,							
	"BlastTech ACP HAMaR Scope",					"",												"",										"gfx/2d/acp_sniperrifle/scope_mask_overlay",			"gfx/2d/acp_sniperrifle/scope_mask",					"",											"",												"",										"",										162,			20,				316,			440,				"sound/weapons/disruptor/zoomstart.wav",	"sound/weapons/disruptor/zoomend.wav",		qfalse,					1.5f,				5.0f,				0.2f,							
	"BlastTech Rifle Bowcaster Scope",				"",												"",										"",														"gfx/2d/Bowcaster/lensmask",							"gfx/2d/Bowcaster/lensmask_zoom",			"",												"",										"",										0,				0,				640,			480,				"sound/weapons/disruptor/zoomstart.wav",	"sound/weapons/disruptor/zoomend.wav",		qtrue,					1.2f,				1.2f,				0.0f,							
	"BlastTech Rifle Heavy Bowcaster Scope",		"",												"",										"gfx/2d/Bowcaster_Heavy/scope_mask_overlay",			"gfx/2d/Bowcaster_Heavy/scope_mask",					"",											"",												"",										"",										0,				0,				640,			480,				"sound/weapons/disruptor/zoomstart.wav",	"sound/weapons/disruptor/zoomend.wav",		qtrue,					1.2f,				1.2f,				0.0f,							
	"BlastTech Bryar Sniper Scope",					"",												"",										"",														"gfx/2d/Bryar_Rifle/scope_mask",						"",											"",												"",										"",										112,			36,				414,			414,				"sound/weapons/disruptor/zoomstart.wav",	"sound/weapons/disruptor/zoomend.wav",		qfalse,					1.5f,				1.5f,				0.2f,							
	"BlastTech DH-17 Pistol Scope",					"",												"",										"gfx/2d/DH-17_Pistol/scope_mask_overlay",				"gfx/2d/DH-17_Pistol/scope_mask",						"",											"",												"",										"",										0,				0,				640,			480,				"sound/weapons/disruptor/zoomstart.wav",	"sound/weapons/disruptor/zoomend.wav",		qtrue,					1.0f,				1.0f,				0.0f,							
};
