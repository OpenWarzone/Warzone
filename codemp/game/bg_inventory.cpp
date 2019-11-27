#include "bg_inventory.h"


#ifdef _GAME
#include "g_local.h"
#endif

#ifdef _CGAME
#include "../cgame/cg_local.h"
#endif

#ifdef _GAME
//
// Global Stuff... This is for the server to keep track of unique item instances.
//
int					INVENTORY_ITEM_INSTANCES_COUNT = 0;	// TODO: This should be initialized to database's highest value at map load... Or even better, always store the max id to it's own DB field.
inventoryItem		*INVENTORY_ITEM_INSTANCES[8388608];
#endif

#if defined(rd_warzone_x86_EXPORTS)
#include "tr_local.h"
#endif


/*QUAKED item_***** ( 0 0 0 ) (-16 -16 -16) (16 16 16) suspended
DO NOT USE THIS CLASS, IT JUST HOLDS GENERAL INFORMATION.
The suspended flag will allow items to hang in the air, otherwise they are dropped to the next surface.

If an item is the target of another entity, it will not spawn in until fired.

An item fires all of its targets when it is picked up.  If the toucher can't carry it, the targets won't be fired.

"notfree" if set to 1, don't spawn in free for all games
"notteam" if set to 1, don't spawn in team games
"notsingle" if set to 1, don't spawn in single player games
"wait"	override the default wait before respawning.  -1 = never respawn automatically, which can be used with targeted spawning.
"random" random number of plus or minus seconds varied from the respawn time
"count" override quantity or duration on most items.
*/

gitem_t	bg_itemlist[] =
{
	{
		NULL,				// classname
		NULL,				// pickup_sound
		{ NULL,			// world_model[0]
		NULL,			// world_model[1]
		0, 0 } ,			// world_model[2],[3]
		NULL,				// view_model
		/* icon */		NULL,		// icon
		/* pickup */	NULL,		// pickup_name
		0,					// quantity
		IT_BAD,				// giType (IT_*)
		0,					// giTag
		/* precache */ "",			// precaches
		/* sounds */ "",			// sounds
		"",					// description
		0,					// price
	},	// leave index 0 alone

		//
		// Pickups
		//

		/*QUAKED item_shield_sm_instant (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
		Instant shield pickup, restores 25
		*/
	{
		"item_shield_sm_instant",
		"sound/player/pickupshield.wav",
		{ "models/map_objects/mp/psd_sm.md3",
		0, 0, 0 },
		/* view */		NULL,
		/* icon */		"gfx/mp/small_shield",
		/* pickup */	"Shield Small",
		25,
		IT_ARMOR,
		1, //special for shield - max on pickup is maxhealth*tag, thus small shield goes up to 100 shield
		/* precache */ "",
		/* sounds */ "",
		"A small shield recharge battery.",					// description
		12,
	},

	/*QUAKED item_shield_lrg_instant (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	Instant shield pickup, restores 100
	*/
	{
		"item_shield_lrg_instant",
		"sound/player/pickupshield.wav",
		{ "models/map_objects/mp/psd.md3",
		0, 0, 0 },
		/* view */		NULL,
		/* icon */		"gfx/mp/large_shield",
		/* pickup */	"Shield Large",
		100,
		IT_ARMOR,
		2, //special for shield - max on pickup is maxhealth*tag, thus large shield goes up to 200 shield
		/* precache */ "",
		/* sounds */ "",
		"A large shield recharge battery.",					// description
		22,
	},

	/*QUAKED item_medpak_instant (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	Instant medpack pickup, heals 25
	*/
	{
		"item_medpak_instant",
		"sound/player/pickuphealth.wav",
		{ "models/map_objects/mp/medpac.md3",
		0, 0, 0 },
		/* view */		NULL,
		/* icon */		"gfx/hud/i_icon_medkit",
		/* pickup */	"Medpack",
		25,
		IT_HEALTH,
		0,
		/* precache */ "",
		/* sounds */ "",
		"A medical kit, used to heal light wounds.",					// description
		14,
	},


	//
	// ITEMS
	//

	/*QUAKED item_seeker (.3 .3 1) (-8 -8 -0) (8 8 16) suspended
	30 seconds of seeker drone
	*/
	{
		"item_seeker",
		"sound/weapons/w_pkup.wav",
		{ "models/items/remote.md3",
		0, 0, 0 } ,
		/* view */		NULL,
		/* icon */		"gfx/hud/i_icon_seeker",
		/* pickup */	"Seeker Drone",
		120,
		IT_HOLDABLE,
		HI_SEEKER,
		/* precache */ "",
		/* sounds */ "",
		"A seeker drone. Designed to protect it's owner until it is destroyed.",					// description
		80,
	},

	/*QUAKED item_shield (.3 .3 1) (-8 -8 -0) (8 8 16) suspended
	Portable shield
	*/
	{
		"item_shield",
		"sound/weapons/w_pkup.wav",
		{ "models/map_objects/mp/shield.md3",
		0, 0, 0 } ,
		/* view */		NULL,
		/* icon */		"gfx/hud/i_icon_shieldwall",
		/* pickup */	"Force Field",
		120,
		IT_HOLDABLE,
		HI_SHIELD,
		/* precache */ "",
		/* sounds */ "sound/weapons/detpack/stick.wav sound/movers/doors/forcefield_on.wav sound/movers/doors/forcefield_off.wav sound/movers/doors/forcefield_lp.wav sound/effects/bumpfield.wav",
		"A stationary shield device. Can be placed to create a temporary shield.",					// description
		50,
	},

	/*QUAKED item_medpac (.3 .3 1) (-8 -8 -0) (8 8 16) suspended
	Bacta canister pickup, heals 25 on use
	*/
	{
		"item_medpac",	//should be item_bacta
		"sound/weapons/w_pkup.wav",
		{ "models/map_objects/mp/bacta.md3",
		0, 0, 0 } ,
		/* view */		NULL,
		/* icon */		"gfx/hud/i_icon_bacta",
		/* pickup */	"Bacta Canister",
		25,
		IT_HOLDABLE,
		HI_MEDPAC,
		/* precache */ "",
		/* sounds */ "",
		"A bacta canister, used to heal bad wounds.",					// description
		26,
	},

	/*QUAKED item_medpac_big (.3 .3 1) (-8 -8 -0) (8 8 16) suspended
	Big bacta canister pickup, heals 50 on use
	*/
	{
		"item_medpac_big",	//should be item_bacta
		"sound/weapons/w_pkup.wav",
		{ "models/items/big_bacta.md3",
		0, 0, 0 } ,
		/* view */		NULL,
		/* icon */		"gfx/hud/i_icon_big_bacta",
		/* pickup */	"Bacta Canister",
		25,
		IT_HOLDABLE,
		HI_MEDPAC_BIG,
		/* precache */ "",
		/* sounds */ "",
		"A large medical kit, useful to heal the worst of wounds.",					// description
		38,
	},

	/*QUAKED item_binoculars (.3 .3 1) (-8 -8 -0) (8 8 16) suspended
	These will be standard equipment on the player - DO NOT PLACE
	*/
	{
		"item_binoculars",
		"sound/weapons/w_pkup.wav",
		{ "models/items/binoculars.md3",
		0, 0, 0 } ,
		/* view */		NULL,
		/* icon */		"gfx/hud/i_icon_zoom",
		/* pickup */	"Binoculars",
		60,
		IT_HOLDABLE,
		HI_BINOCULARS,
		/* precache */ "",
		/* sounds */ "",
		"Binoculars. Useful for scouting an area.",					// description
		200,
	},

	/*QUAKED item_sentry_gun (.3 .3 1) (-8 -8 -0) (8 8 16) suspended
	Sentry gun inventory pickup.
	*/
	{
		"item_sentry_gun",
		"sound/weapons/w_pkup.wav",
		{ "models/items/psgun.glm",
		0, 0, 0 } ,
		/* view */		NULL,
		/* icon */		"gfx/hud/i_icon_sentrygun",
		/* pickup */	"Sentry Gun",
		120,
		IT_HOLDABLE,
		HI_SENTRY_GUN,
		/* precache */ "",
		/* sounds */ "",
		"A stationary sentry gun. When droped in place, will shoot all enemies until it is destroyed.",					// description
		75,
	},

	/*QUAKED item_jetpack (.3 .3 1) (-8 -8 -0) (8 8 16) suspended
	Do not place.
	*/
	{
		"item_jetpack",
		"sound/weapons/w_pkup.wav",
		{ "models/items/psgun.glm", //FIXME: no model
		0, 0, 0 } ,
		/* view */		NULL,
		/* icon */		"gfx/hud/i_icon_jetpack",
		/* pickup */	"Jetpack",
		120,
		IT_HOLDABLE,
		HI_JETPACK,

		///* precache */ "effects/boba/jet.efx",
		///* sounds */ "sound/chars/boba/JETON.wav sound/chars/boba/JETHOVER.wav sound/effects/fire_lp.wav",
		/* precache */ "effects/Player/jetpack.efx",
		/* sounds */ "sound/jkg/jetpack/JETON.wav sound/jkg/jetpack/jetoff.wav sound/jkg/jetpack/jethover.wav sound/jkg/jetpack/jetlp.wav",
		"A jetpack device. Allows the wearer to fly.",					// description
		500,
	},

	/*QUAKED item_healthdisp (.3 .3 1) (-8 -8 -0) (8 8 16) suspended
	Do not place. For siege classes ONLY.
	*/
	{
		"item_healthdisp",
		"sound/weapons/w_pkup.wav",
		{ "models/map_objects/mp/bacta.md3", //replace me
		0, 0, 0 } ,
		/* view */		NULL,
		/* icon */		"gfx/hud/i_icon_healthdisp",
		/* pickup */	"Health Displenser",
		120,
		IT_HOLDABLE,
		HI_HEALTHDISP,
		/* precache */ "",
		/* sounds */ "",
		"A health dispenser. Use to regain health.",					// description
		0,
	},

	/*QUAKED item_ammodisp (.3 .3 1) (-8 -8 -0) (8 8 16) suspended
	Do not place. For siege classes ONLY.
	*/
	{
		"item_ammodisp",
		"sound/weapons/w_pkup.wav",
		{ "models/map_objects/mp/bacta.md3", //replace me
		0, 0, 0 } ,
		/* view */		NULL,
		/* icon */		"gfx/hud/i_icon_ammodisp",
		/* pickup */	"Ammo Displenser",
		120,
		IT_HOLDABLE,
		HI_AMMODISP,
		/* precache */ "",
		/* sounds */ "",
		"An ammo displenser. REPLACE ME!!!",					// description
		0,
	},

	/*QUAKED item_eweb_holdable (.3 .3 1) (-8 -8 -0) (8 8 16) suspended
	Do not place. For siege classes ONLY.
	*/
	{
		"item_eweb_holdable",
		"sound/interface/shieldcon_empty",
		{ "models/map_objects/hoth/eweb_model.glm",
		0, 0, 0 } ,
		/* view */		NULL,
		/* icon */		"gfx/hud/i_icon_eweb",
		/* pickup */	"E-WEB",
		120,
		IT_HOLDABLE,
		HI_EWEB,
		/* precache */ "",
		/* sounds */ "",
		"A portable, useable, turret device.",					// description
		300,
	},

	/*QUAKED item_seeker (.3 .3 1) (-8 -8 -0) (8 8 16) suspended
	30 seconds of seeker drone
	*/
	{
		"item_cloak",
		"sound/weapons/w_pkup.wav",
		{ "models/items/psgun.glm", //FIXME: no model
		0, 0, 0 } ,
		/* view */		NULL,
		/* icon */		"gfx/hud/i_icon_cloak",
		/* pickup */	"Cloaking Device",
		120,
		IT_HOLDABLE,
		HI_CLOAK,
		/* precache */ "",
		/* sounds */ "",
		"A cloaking device. Makes the user almost invisible to enemies.",					// description
		800,
	},

	/*QUAKED item_force_enlighten_light (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	Adds one rank to all Force powers temporarily. Only light jedi can use.
	*/
	{
		"item_force_enlighten_light",
		"sound/player/enlightenment.wav",
		{ "models/map_objects/mp/jedi_enlightenment.md3",
		0, 0, 0 } ,
		/* view */		NULL,
		/* icon */		"gfx/hud/mpi_jlight",
		/* pickup */	"Light Force Enlightenment",
		25,
		IT_POWERUP,
		PW_FORCE_ENLIGHTENED_LIGHT,
		/* precache */ "",
		/* sounds */ "",
		"",					// description
		0,
	},

	/*QUAKED item_force_enlighten_dark (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	Adds one rank to all Force powers temporarily. Only dark jedi can use.
	*/
	{
		"item_force_enlighten_dark",
		"sound/player/enlightenment.wav",
		{ "models/map_objects/mp/dk_enlightenment.md3",
		0, 0, 0 } ,
		/* view */		NULL,
		/* icon */		"gfx/hud/mpi_dklight",
		/* pickup */	"Dark Force Enlightenment",
		25,
		IT_POWERUP,
		PW_FORCE_ENLIGHTENED_DARK,
		/* precache */ "",
		/* sounds */ "",
		"",					// description
		0,
	},

	/*QUAKED item_force_boon (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	Unlimited Force Pool for a short time.
	*/
	{
		"item_force_boon",
		"sound/player/boon.wav",
		{ "models/map_objects/mp/force_boon.md3",
		0, 0, 0 } ,
		/* view */		NULL,
		/* icon */		"gfx/hud/mpi_fboon",
		/* pickup */	"Force Boon",
		25,
		IT_POWERUP,
		PW_FORCE_BOON,
		/* precache */ "",
		/* sounds */ "",
		"",					// description
		0,
	},

	/*QUAKED item_ysalimari (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	A small lizard carried on the player, which prevents the possessor from using any Force power.  However, he is unaffected by any Force power.
	*/
	{
		"item_ysalimari",
		"sound/player/ysalimari.wav",
		{ "models/map_objects/mp/ysalimari.md3",
		0, 0, 0 } ,
		/* view */		NULL,
		/* icon */		"gfx/hud/mpi_ysamari",
		/* pickup */	"Ysalamiri",
		25,
		IT_POWERUP,
		PW_YSALAMIRI,
		/* precache */ "",
		/* sounds */ "",
		"",					// description
		0,
	},

#ifndef __MMO__
	/*QUAKED ammo_thermal (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	*/
	{
		"ammo_thermal",
		"sound/weapons/w_pkup.wav",
		{ "models/weapons2/thermal/thermal_pu.md3",
		"models/weapons2/thermal/thermal_w.glm", 0, 0 },
		/* view */		"models/weapons2/thermal/thermal.md3",
		/* icon */		"gfx/hud/w_icon_thermal",
		/* pickup */	"Thermal Detonators",
		4,
		IT_AMMO,
		AMMO_THERMAL,
		/* precache */ "",
		/* sounds */ "",
		"@MENUS_THE_THERMAL_DETONATOR",					// description
		10,
	},

	/*QUAKED ammo_tripmine (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	*/
	{
		"ammo_tripmine",
		"sound/weapons/w_pkup.wav",
		{ "models/weapons2/laser_trap/laser_trap_pu.md3",
		"models/weapons2/laser_trap/laser_trap_w.glm", 0, 0 },
		/* view */		"models/weapons2/laser_trap/laser_trap.md3",
		/* icon */		"gfx/hud/w_icon_tripmine",
		/* pickup */	"Trip Mines",
		3,
		IT_AMMO,
		AMMO_TRIPMINE,
		/* precache */ "",
		/* sounds */ "",
		"@MENUS_TRIP_MINES_CONSIST_OF",					// description
		10,
	},

	/*QUAKED ammo_detpack (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	*/
	{
		"ammo_detpack",
		"sound/weapons/w_pkup.wav",
		{ "models/weapons2/detpack/det_pack_pu.md3", "models/weapons2/detpack/det_pack_proj.glm", "models/weapons2/detpack/det_pack_w.glm", 0 },
		/* view */		"models/weapons2/detpack/det_pack.md3",
		/* icon */		"gfx/hud/w_icon_detpack",
		/* pickup */	"Det Packs",
		3,
		IT_AMMO,
		AMMO_DETPACK,
		/* precache */ "",
		/* sounds */ "",
		"@MENUS_A_DETONATION_PACK_IS",					// description
		10,
	},
#endif //__MMO__

	/*QUAKED weapon_thermal (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	*/
	{
		"weapon_thermal",
		"sound/weapons/w_pkup.wav",
		{ "models/weapons/Grenade_Thermal/model.glm", "models/weapons/Grenade_Thermal/model_proj.md3",
		0, 0 },
		/* view */		"models/weapons/Grenade_Thermal/viewmodel.md3",
		/* icon */		"models/weapons/Grenade_Thermal/icon_default",
		/* pickup */	"Thermal Detonator",
		4,
		IT_WEAPON,
		WP_THERMAL,
		/* precache */ "",
		/* sounds */ "",
		"@MENUS_THE_THERMAL_DETONATOR",					// description
		860,
	},

	/*QUAKED weapon_fraggrenade (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	*/
	{
		"weapon_fraggrenade",
		"sound/weapons/w_pkup.wav",
		{ "models/weapons3/fraggrenade/thermal_w.glm", "models/weapons3/fraggrenade/thermal_pu.md3",
		0, 0 },
		/* view */		"models/weapons3/fraggrenade/thermal.md3",
		/* icon */		"gfx/hud/w_icon_fraggrenade",
		/* pickup */	"Thermal Detonator",
		4,
		IT_WEAPON,
		WP_FRAG_GRENADE,
		/* precache */ "",
		/* sounds */ "",
		"This grenade is designed to break into fragments during it's explosion, causing massive injuries over a large area.",					// description
		830,
	},

	/*QUAKED weapon_grenade_cryoban (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	*/
	{
		"weapon_grenade_cryoban ",
		"sound/weapons/w_pkup.wav",
		{ "models/weapons/Grenade_CryoBan/model.glm", "models/weapons/Grenade_CryoBan/model_proj.md3",
		0, 0 },
		/* view */		"models/weapons/Grenade_CryoBan/viewmodel.md3",
		/* icon */		"models/weapons/Grenade_CryoBan/icon_default",
		/* pickup */	"Cryoban Grenade",
		4,
		IT_WEAPON,
		WP_CYROBAN_GRENADE,
		/* precache */ "",
		/* sounds */ "",
		"Designed to deal massive, lingerring, cold damage over a large area.",					// description
		920,
	},

	/*QUAKED weapon_trip_mine (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	*/
	{
		"weapon_trip_mine",
		"sound/weapons/w_pkup.wav",
		{ "models/weapons/Mine_LaserTrip/model.glm",
		0, 0 },
		/* view */		"models/Weapons/Mine_LaserTrip/viewmodel.md3",
		/* icon */		"models/weapons/Mine_LaserTrip/icon_default",
		//        { "models/weapons2/laser_trap/laser_trap_w.glm", "models/weapons2/laser_trap/laser_trap_pu.md3",
		//		0, 0},
		///* view */		"models/weapons2/laser_trap/laser_trap.md3",
		///* icon */		"gfx/hud/w_icon_tripmine",
		/* pickup */	"Trip Mine",
		3,
		IT_WEAPON,
		WP_TRIP_MINE,
		/* precache */ "",
		/* sounds */ "",
		"@MENUS_TRIP_MINES_CONSIST_OF",					// description
		875,
	},

	/*QUAKED weapon_det_pack (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	*/
	{
		"weapon_det_pack",
		"sound/weapons/w_pkup.wav",
		{ "models/weapons2/detpack/det_pack_proj.glm", "models/weapons2/detpack/det_pack_pu.md3", "models/weapons2/detpack/det_pack_w.glm", 0 },
		/* view */		"models/weapons2/detpack/det_pack.md3",
		/* icon */		"gfx/hud/w_icon_detpack",
		/* pickup */	"Det Pack",
		3,
		IT_WEAPON,
		WP_DET_PACK,
		/* precache */ "",
		/* sounds */ "",
		"@MENUS_A_DETONATION_PACK_IS",					// description
		970,
	},

	/*QUAKED weapon_emplaced (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	*/
	{
		"weapon_emplaced",
		"sound/weapons/w_pkup.wav",
		{ "models/weapons2/blaster_r/blaster_w.glm",
		0, 0, 0 },
		/* view */		"models/weapons2/blaster_r/blaster.md3",
		/* icon */		"gfx/hud/w_icon_blaster",
		/* pickup */	"Emplaced Gun",
		50,
		IT_WEAPON,
		WP_EMPLACED_GUN,
		/* precache */ "",
		/* sounds */ "",
		"This gun is mountable, and capable of taking out the largest of enemy forces in a short time.",					// description
		1160,
	},


	//NOTE: This is to keep things from messing up because the turret weapon type isn't real
	{
		"weapon_turretwp",
		"sound/weapons/w_pkup.wav",
		{ "models/weapons2/blaster_r/blaster_w.glm",
		0, 0, 0 },
		/* view */		"models/weapons2/blaster_r/blaster.md3",
		/* icon */		"gfx/hud/w_icon_blaster",
		/* pickup */	"Turret Gun",
		50,
		IT_WEAPON,
		WP_TURRET,
		/* precache */ "",
		/* sounds */ "",
		"This gun is mountable, and capable of taking out the largest of enemy forces in a short time.",					// description
		1160,
	},

#ifndef __MMO__
	//
	// AMMO ITEMS
	//

	/*QUAKED ammo_force (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	Don't place this
	*/
	{
		"ammo_force",
		"sound/player/pickupenergy.wav",
		{ "models/items/energy_cell.md3",
		0, 0, 0 },
		/* view */		NULL,
		/* icon */		"gfx/hud/w_icon_blaster",
		/* pickup */	"Force??",
		100,
		IT_AMMO,
		AMMO_FORCE,
		/* precache */ "",
		/* sounds */ "",
		"",					// description
		0,
	},

	/*QUAKED ammo_blaster (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	Ammo for the Bryar and Blaster pistols.
	*/
	{
		"ammo_blaster",
		"sound/player/pickupenergy.wav",
		{ "models/items/energy_cell.md3",
		0, 0, 0 },
		/* view */		NULL,
		/* icon */		"gfx/hud/i_icon_battery",
		/* pickup */	"Blaster Pack",
		100,
		IT_AMMO,
		AMMO_BLASTER,
		/* precache */ "",
		/* sounds */ "",
		"",					// description
		0,
	},

	/*QUAKED ammo_powercell (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	Ammo for Tenloss Disruptor, Wookie Bowcaster, and the Destructive Electro Magnetic Pulse (demp2 ) guns
	*/
	{
		"ammo_powercell",
		"sound/player/pickupenergy.wav",
		{ "models/items/power_cell.md3",
		0, 0, 0 },
		/* view */		NULL,
		/* icon */		"gfx/mp/ammo_power_cell",
		/* pickup */	"Power Cell",
		100,
		IT_AMMO,
		AMMO_POWERCELL,
		/* precache */ "",
		/* sounds */ "",
		"",					// description
		0,
	},

	/*QUAKED ammo_metallic_bolts (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	Ammo for Imperial Heavy Repeater and the Golan Arms Flechette
	*/
	{
		"ammo_metallic_bolts",
		"sound/player/pickupenergy.wav",
		{ "models/items/metallic_bolts.md3",
		0, 0, 0 },
		/* view */		NULL,
		/* icon */		"gfx/mp/ammo_metallic_bolts",
		/* pickup */	"Metallic Bolts",
		100,
		IT_AMMO,
		AMMO_METAL_BOLTS,
		/* precache */ "",
		/* sounds */ "",
		"",					// description
		0,
	},

	/*QUAKED ammo_rockets (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	Ammo for Merr-Sonn portable missile launcher
	*/
	{
		"ammo_rockets",
		"sound/player/pickupenergy.wav",
		{ "models/items/rockets.md3",
		0, 0, 0 },
		/* view */		NULL,
		/* icon */		"gfx/mp/ammo_rockets",
		/* pickup */	"Rockets",
		3,
		IT_AMMO,
		AMMO_ROCKETS,
		/* precache */ "",
		/* sounds */ "",
		"",					// description
		0,
	},

	/*QUAKED ammo_all (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	DO NOT PLACE in a map, this is only for siege classes that have ammo
	dispensing ability
	*/
	{
		"ammo_all",
		"sound/player/pickupenergy.wav",
		{ "models/items/battery.md3",  //replace me
		0, 0, 0 },
		/* view */		NULL,
		/* icon */		"gfx/mp/ammo_rockets", //replace me
		/* pickup */	"Rockets",
		0,
		IT_AMMO,
		-1,
		/* precache */ "",
		/* sounds */ "",
		"",					// description
		0,
	},
#endif //__MMO__

	//
	// POWERUP ITEMS
	//
	/*QUAKED team_CTF_redflag (1 0 0) (-16 -16 -16) (16 16 16)
	Only in CTF games
	*/
	{
		"team_CTF_redflag",
		NULL,
		{ "models/flags/r_flag.md3",
		"models/flags/r_flag_ysal.md3", 0, 0 },
		/* view */		NULL,
		/* icon */		"gfx/hud/mpi_rflag",
		/* pickup */	"Red Flag",
		0,
		IT_TEAM,
		PW_REDFLAG,
		/* precache */ "",
		/* sounds */ "",
		"",					// description
		0,
	},

	/*QUAKED team_CTF_blueflag (0 0 1) (-16 -16 -16) (16 16 16)
	Only in CTF games
	*/
	{
		"team_CTF_blueflag",
		NULL,
		{ "models/flags/b_flag.md3",
		"models/flags/b_flag_ysal.md3", 0, 0 },
		/* view */		NULL,
		/* icon */		"gfx/hud/mpi_bflag",
		/* pickup */	"Blue Flag",
		0,
		IT_TEAM,
		PW_BLUEFLAG,
		/* precache */ "",
		/* sounds */ "",
		"",					// description
		0,
	},

	//
	// PERSISTANT POWERUP ITEMS
	//

	/*QUAKED team_CTF_neutralflag (0 0 1) (-16 -16 -16) (16 16 16)
	Only in One Flag CTF games
	*/
	{
		"team_CTF_neutralflag",
		NULL,
		{ "models/flags/n_flag.md3",
		0, 0, 0 },
		/* view */		NULL,
		/* icon */		"icons/iconf_neutral1",
		/* pickup */	"Neutral Flag",
		0,
		IT_TEAM,
		PW_NEUTRALFLAG,
		/* precache */ "",
		/* sounds */ "",
		"",					// description
		0,
	},

	{
		"item_redcube",
		"sound/player/pickupenergy.wav",
		{ "models/powerups/orb/r_orb.md3",
		0, 0, 0 },
		/* view */		NULL,
		/* icon */		"icons/iconh_rorb",
		/* pickup */	"Red Cube",
		0,
		IT_TEAM,
		0,
		/* precache */ "",
		/* sounds */ "",
		"",					// description
		0,
	},

	{
		"item_bluecube",
		"sound/player/pickupenergy.wav",
		{ "models/powerups/orb/b_orb.md3",
		0, 0, 0 },
		/* view */		NULL,
		/* icon */		"icons/iconh_borb",
		/* pickup */	"Blue Cube",
		0,
		IT_TEAM,
		0,
		/* precache */ "",
		/* sounds */ "",
		"",					// description
		0,
	},

	{
		"item_saber_crystal",
		"sound/player/pickupenergy.wav",
		{ "models/warzone/crystals/saber_crystal.md3",
		0, 0, 0 },
		/* view */		NULL,
		/* icon */		"icons/icon_saber_crystal",
		/* pickup */	"Khyber Crystal",
		0,
		IT_SABER_CRYSTAL,
		0,
		/* precache */ "",
		/* sounds */ "",
		"A Khyber Crystal. Usually uniquely attuned to the force user, this crystal is used in the crafting of Lightsaber weapons. Controls the color and bonus effects of the blade.",					// description
		400,
	},

	{
		"item_saber_modification",
		"sound/player/pickupenergy.wav",
		{ "models/warzone/modification/saber_modification.md3",
		0, 0, 0 },
		/* view */		NULL,
		/* icon */		"icons/icon_saber_modification",
		/* pickup */	"Saber Modification",
		0,
		IT_WEAPON_MODIFICATION,
		0,
		/* precache */ "",
		/* sounds */ "",
		"A Saber Modification. Used in the crafting of Lightsaber weapons. Adjusts the various strengths of the Lightsaber to suit it's wielder.",					// description
		300,
	},

	{
		"item_weapon_crystal",
		"sound/player/pickupenergy.wav",
		{ "models/warzone/crystals/weapon_crystal.md3",
		0, 0, 0 },
		/* view */		NULL,
		/* icon */		"icons/icon_weapon_crystal",
		/* pickup */	"Weapon Focusing Crystal",
		0,
		IT_WEAPON_CRYSTAL,
		0,
		/* precache */ "",
		/* sounds */ "",
		"A Weapon Crystal. Used to focus the beam in the crafting of guns weapons. Adjusts the damage types and color of the weapon's shots.",					// description
		400,
	},

	{
		"item_weapon_modification",
		"sound/player/pickupenergy.wav",
		{ "models/warzone/modification/weapon_modification.md3",
		0, 0, 0 },
		/* view */		NULL,
		/* icon */		"icons/icon_weapon_modification",
		/* pickup */	"Weapon Modification",
		0,
		IT_WEAPON_MODIFICATION,
		0,
		/* precache */ "",
		/* sounds */ "",
		"A Weapon Modification. Used in the crafting of guns. Adjusts the various strengths of the weapon to suit it's owner.",					// description
		300,
	},

	{
		"item_item_crystal",
		"sound/player/pickupenergy.wav",
		{ "models/warzone/crystals/item_crystal.md3",
		0, 0, 0 },
		/* view */		NULL,
		/* icon */		"icons/icon_item_crystal",
		/* pickup */	"Item Crystal",
		0,
		IT_ITEM_CRYSTAL,
		0,
		/* precache */ "",
		/* sounds */ "",
		"An Item Crystal. Used to focus the shielding in the crafting of wearable items. Adjusts the damage resistances of the item.",					// description
		400,
	},

	{
		"item_item_modification",
		"sound/player/pickupenergy.wav",
		{ "models/warzone/modification/item_modification.md3",
		0, 0, 0 },
		/* view */		NULL,
		/* icon */		"icons/icon_saber_crystal",
		/* pickup */	"Wearable Modification",
		0,
		IT_ITEM_MODIFICATION,
		0,
		/* precache */ "",
		/* sounds */ "",
		"A Wearable Modification. Used in the crafting of wearable items. Augments the item to suit it's owner.",					// description
		300,
	},

	//
	// WEAPONS
	//

	/*QUAKED weapon_melee (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	Don't place this
	*/
	{
		"weapon_melee",
		"sound/weapons/w_pkup.wav",
		{ "models/weapons2/stun_baton/baton_w.glm",
		0, 0, 0 },
		/* view */		"models/weapons2/stun_baton/baton.md3",
		/* icon */		"gfx/hud/w_icon_melee",
		/* pickup */	"Melee",
		100,
		IT_WEAPON,
		WP_MELEE,
		/* precache */ "",
		/* sounds */ "",
		"",					// description
		0,
	},

	/*QUAKED weapon_saber (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	Don't place this
	*/
	{
		"weapon_saber",
		"sound/weapons/w_pkup.wav",
		{ "models/weapons2/saber/saber_w.glm",
		0, 0, 0 },
		/* view */		"models/weapons2/saber/saber_w.md3",
		/* icon */		"gfx/hud/w_icon_lightsaber",
		/* pickup */	"Lightsaber",
		100,
		IT_WEAPON,
		WP_SABER,
		/* precache */ "",
		/* sounds */ "",
		"An elegant weapon from a more civilized age.",				// description
		1420,
	},

	/*QUAKED weapon_modular (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	*/
	{
		"weapon_modular",
		"sound/weapons/w_pkup.wav",
		{ "models/wzweapons/pistol_1.md3",
		0, 0, 0 },
		/* view */		"models/wzweapons/pistol_1.md3",
		/* icon */		"models/weapons/Bryar_Carbine/icon_default",
		/* pickup */	"Pistol",
		100,
		IT_WEAPON,
		WP_MODULIZED_WEAPON,
		/* precache */ "",
		/* sounds */ "",
		"This weapons looks like it was designed to be upgradable.",				// description
		530,
	},

	// end of list marker
	{ NULL }
};

int		bg_numItems = sizeof(bg_itemlist) / sizeof(bg_itemlist[0]) - 1;

//
// ALL INVENTORY POSSIBILITIES...
//
int				allInventoryItemsCount = 0;
int				allInventorySabersStart = 0;
int				allInventorySabersEnd = 0;
int				allInventorySaberModsStart = 0;
int				allInventorySaberModsEnd = 0;
int				allInventorySaberCrystalsStart = 0;
int				allInventorySaberCrystalsEnd = 0;
int				allInventoryWeaponsStart = 0;
int				allInventoryWeaponsEnd = 0;
int				allInventoryWeaponModsStart = 0;
int				allInventoryWeaponModsEnd = 0;
int				allInventoryWeaponCrystalsStart = 0;
int				allInventoryWeaponCrystalsEnd = 0;
inventoryItem	*allInventoryItems[1048576];

//
//
//
float StatRollForQuality(int quality)
{
	return 0.01 * pow(float(quality + 1), 2.0);
}

void GenerateAllInventoryItems(void)
{// TODO: Share between CGAME and RENDERER...
#if defined(_UI)
	// Don't need any of this in old ui code...
	return;
#endif //defined(_UI)

	/*
	for (int i = 0; i < bg_numItems; i++)
	{
		Com_Printf("%i - %s.\n", i, (bg_itemlist[i].name && bg_itemlist[i].name[0]) ? bg_itemlist[i].name : "NONE");
	}
	return;
	*/

	//
	// Sabers...
	//
	int bgItemID = 38;

	allInventorySabersStart = 0;

	// Sabers...
	for (int quality = QUALITY_GREY; quality <= QUALITY_GOLD; quality++)
	{
		float roll = StatRollForQuality(quality);

		for (int crystal = ITEM_CRYSTAL_RED; crystal < ITEM_CRYSTAL_MAX; crystal++)
		{
			for (int stat1 = SABER_STAT1_DEFAULT; stat1 < SABER_STAT1_MAX; stat1++)
			{
				for (int stat2 = SABER_STAT2_DEFAULT; stat2 < SABER_STAT2_MAX; stat2++)
				{
					for (int stat3 = SABER_STAT3_DEFAULT; stat3 < SABER_STAT3_MAX; stat3++)
					{
						// Never add stat slots not available at this quality level...
						if (quality <= QUALITY_WHITE && (stat1 > 0 || stat2 > 0 || stat3 > 0)) continue;
						if (quality <= QUALITY_GREEN && (stat2 > 0 || stat3 > 0)) continue;
						if (quality <= QUALITY_BLUE && (stat3 > 0)) continue;

						// Always fill required slotes for stats...
						if (quality > QUALITY_BLUE && !(stat1 > 0 && stat2 > 0 && stat3 > 0)) continue;
						if (quality > QUALITY_GREEN && !(stat1 > 0 && stat2 > 0)) continue;
						if (quality > QUALITY_WHITE && !(stat1 > 0)) continue;
						if (!(crystal > 0)) continue;

						inventoryItem *item = new inventoryItem(allInventoryItemsCount, bgItemID, (itemQuality_t)quality, 1, -1);

						item->setItemID(allInventoryItemsCount);
						item->setBaseItem(bgItemID);
						item->setQuality((itemQuality_t)quality);
						item->setCrystal((itemPowerCrystal_t)crystal);

						item->setStat1(stat1, roll);
						item->setStat2(stat2, roll);
						item->setStat3(stat3, roll);

						item->setQuantity(1);

//#if defined(_GAME)
//						trap->Print("^1*** ^3INVENTORY-GAME^5: Saber %i. Name: %s\n", allInventoryItemsCount, item->getName());
//#endif

						allInventoryItems[allInventoryItemsCount] = item;
						allInventoryItemsCount++;
					}
				}
			}
		}
	}

	allInventorySabersEnd = allInventoryItemsCount - 1;

	int numSabers = allInventorySabersEnd - allInventorySabersStart;


	//
	// Weapons...
	//

	allInventoryWeaponsStart = allInventoryItemsCount;

	// Modulated weapon...
	bgItemID = 39;

	for (int quality = QUALITY_GREY; quality <= QUALITY_GOLD; quality++)
	{
		float roll = StatRollForQuality(quality);

		for (int crystal = ITEM_CRYSTAL_DEFAULT; crystal < ITEM_CRYSTAL_MAX; crystal++)
		{
			for (int stat1 = WEAPON_STAT1_DEFAULT; stat1 < WEAPON_STAT1_MAX; stat1++)
			{
				for (int stat2 = WEAPON_STAT2_DEFAULT; stat2 < WEAPON_STAT2_MAX; stat2++)
				{
					for (int stat3 = WEAPON_STAT3_SHOT_DEFAULT; stat3 < WEAPON_STAT3_MAX; stat3++)
					{
						// Never add stat slots not available at this quality level...
						if (quality <= QUALITY_GREY && (stat1 > 0 || stat2 > 0 || stat3 > 0 || crystal > 0)) continue;
						if (quality <= QUALITY_WHITE && (stat1 > 0 || stat2 > 0 || stat3 > 0)) continue;
						if (quality <= QUALITY_GREEN && (stat2 > 0 || stat3 > 0)) continue;
						if (quality <= QUALITY_BLUE && (stat3 > 0)) continue;

						// Always fill required slotes for stats...
						if (quality > QUALITY_BLUE && !(stat1 > 0 && stat2 > 0 && stat3 > 0)) continue;
						if (quality > QUALITY_GREEN && !(stat1 > 0 && stat2 > 0)) continue;
						if (quality > QUALITY_WHITE && !(stat1 > 0)) continue;
						if (quality > QUALITY_GREY && !(crystal > 0)) continue;

						inventoryItem *item = new inventoryItem(allInventoryItemsCount, bgItemID, (itemQuality_t)quality, 1, -1);

						item->setItemID(allInventoryItemsCount);
						item->setBaseItem(bgItemID);
						item->setQuality((itemQuality_t)quality);
						item->setCrystal((itemPowerCrystal_t)crystal);

						item->setStat1(stat1, roll);
						item->setStat2(stat2, roll);
						item->setStat3(stat3, roll);

						item->setQuantity(1);

//#if defined(_GAME)
//						trap->Print("^1*** ^3INVENTORY-GAME^5: Weapon %i. Name: %s\n", allInventoryItemsCount, item->getName());
//#endif

						allInventoryItems[allInventoryItemsCount] = item;
						allInventoryItemsCount++;
					}
				}
			}
		}
	}

	allInventoryWeaponsEnd = allInventoryItemsCount - 1;

	int numWeapons = allInventoryWeaponsEnd - allInventoryWeaponsStart;

	//
	// Saber Mods...
	//

	bgItemID = 32;

	allInventorySaberModsStart = allInventoryItemsCount;

	for (int quality = QUALITY_GREEN; quality <= QUALITY_GOLD; quality++)
	{
		float roll = StatRollForQuality(quality);

		for (int stat1 = SABER_STAT1_MELEE_BLOCKING; stat1 < SABER_STAT1_MAX; stat1++)
		{
			inventoryItem *item = new inventoryItem(allInventoryItemsCount, bgItemID, (itemQuality_t)quality, 1, -1);

			item->setItemID(allInventoryItemsCount);
			item->setBaseItem(bgItemID);
			item->setQuality((itemQuality_t)quality);
			item->setCrystal((itemPowerCrystal_t)0);

			item->setStat1(stat1, roll);

			item->setQuantity(1);

//#if defined(_GAME)
//			trap->Print("^1*** ^3INVENTORY-GAME^5: Saber Mod %i. Name: %s\n", allInventoryItemsCount, item->getName());
//#endif

			allInventoryItems[allInventoryItemsCount] = item;
			allInventoryItemsCount++;
		}

		for (int stat2 = SABER_STAT2_DAMAGE_MODIFIER; stat2 < SABER_STAT2_MAX; stat2++)
		{
			inventoryItem *item = new inventoryItem(allInventoryItemsCount, bgItemID, (itemQuality_t)quality, 1, -1);

			item->setItemID(allInventoryItemsCount);
			item->setBaseItem(bgItemID);
			item->setQuality((itemQuality_t)quality);
			item->setCrystal((itemPowerCrystal_t)0);

			item->setStat2(stat2, roll);

			item->setQuantity(1);

//#if defined(_GAME)
//			trap->Print("^1*** ^3INVENTORY-GAME^5: Saber Mod %i. Name: %s\n", allInventoryItemsCount, item->getName());
//#endif

			allInventoryItems[allInventoryItemsCount] = item;
			allInventoryItemsCount++;
		}

		for (int stat3 = SABER_STAT3_LENGTH_MODIFIER; stat3 < SABER_STAT3_MAX; stat3++)
		{
			inventoryItem *item = new inventoryItem(allInventoryItemsCount, bgItemID, (itemQuality_t)quality, 1, -1);

			item->setItemID(allInventoryItemsCount);
			item->setBaseItem(bgItemID);
			item->setQuality((itemQuality_t)quality);
			item->setCrystal((itemPowerCrystal_t)0);

			item->setStat3(stat3, roll);

			item->setQuantity(1);

//#if defined(_GAME)
//			trap->Print("^1*** ^3INVENTORY-GAME^5: Saber Mod %i. Name: %s\n", allInventoryItemsCount, item->getName());
//#endif

			allInventoryItems[allInventoryItemsCount] = item;
			allInventoryItemsCount++;
		}
	}

	allInventorySaberModsEnd = allInventoryItemsCount - 1;

	int numSaberMods = allInventorySaberModsEnd - allInventorySaberModsStart;


	//
	// Saber Crystals...
	//

	bgItemID = 31;

	allInventorySaberCrystalsStart = allInventoryItemsCount;

	for (int quality = QUALITY_GREEN; quality <= QUALITY_GOLD; quality++)
	{
		float roll = StatRollForQuality(quality);

		for (int crystal = ITEM_CRYSTAL_RED; crystal < ITEM_CRYSTAL_MAX; crystal++)
		{
			inventoryItem *item = new inventoryItem(allInventoryItemsCount, bgItemID, (itemQuality_t)quality, 1, -1);

			item->setItemID(allInventoryItemsCount);
			item->setBaseItem(bgItemID);
			item->setQuality((itemQuality_t)quality);
			item->setCrystal((itemPowerCrystal_t)crystal);

			item->setQuantity(1);

			//#if defined(_GAME)
			//			trap->Print("^1*** ^3INVENTORY-GAME^5: Saber Crystal %i. Name: %s\n", allInventoryItemsCount, item->getName());
			//#endif

			allInventoryItems[allInventoryItemsCount] = item;
			allInventoryItemsCount++;
		}
	}

	allInventorySaberCrystalsEnd = allInventoryItemsCount - 1;

	int numSaberCrystals = allInventorySaberCrystalsEnd - allInventorySaberCrystalsStart;


	//
	// Weapon Mods...
	//

	bgItemID = 34;

	allInventoryWeaponModsStart = allInventoryItemsCount;

	for (int quality = QUALITY_GREEN; quality <= QUALITY_GOLD; quality++)
	{
		float roll = StatRollForQuality(quality);

		for (int stat1 = WEAPON_STAT1_FIRE_ACCURACY_MODIFIER; stat1 < WEAPON_STAT1_MAX; stat1++)
		{
			inventoryItem *item = new inventoryItem(allInventoryItemsCount, bgItemID, (itemQuality_t)quality, 1, -1);

			item->setItemID(allInventoryItemsCount);
			item->setBaseItem(bgItemID);
			item->setQuality((itemQuality_t)quality);
			item->setCrystal((itemPowerCrystal_t)0);

			item->setStat1(stat1, roll);

			item->setQuantity(1);

//#if defined(_GAME)
//			trap->Print("^1*** ^3INVENTORY-GAME^5: Weapon Mod %i. Name: %s\n", allInventoryItemsCount, item->getName());
//#endif

			allInventoryItems[allInventoryItemsCount] = item;
			allInventoryItemsCount++;
		}

		for (int stat2 = WEAPON_STAT2_FIRE_DAMAGE_MODIFIER; stat2 < WEAPON_STAT2_MAX; stat2++)
		{
			inventoryItem *item = new inventoryItem(allInventoryItemsCount, bgItemID, (itemQuality_t)quality, 1, -1);

			item->setItemID(allInventoryItemsCount);
			item->setBaseItem(bgItemID);
			item->setQuality((itemQuality_t)quality);
			item->setCrystal((itemPowerCrystal_t)0);

			item->setStat2(stat2, roll);

			item->setQuantity(1);

//#if defined(_GAME)
//			trap->Print("^1*** ^3INVENTORY-GAME^5: Weapon Mod %i. Name: %s\n", allInventoryItemsCount, item->getName());
//#endif

			allInventoryItems[allInventoryItemsCount] = item;
			allInventoryItemsCount++;
		}

		for (int stat3 = WEAPON_STAT3_SHOT_BOUNCE; stat3 < WEAPON_STAT3_MAX; stat3++)
		{
			inventoryItem *item = new inventoryItem(allInventoryItemsCount, bgItemID, (itemQuality_t)quality, 1, -1);

			item->setItemID(allInventoryItemsCount);
			item->setBaseItem(bgItemID);
			item->setQuality((itemQuality_t)quality);
			item->setCrystal((itemPowerCrystal_t)0);

			item->setStat3(stat3, roll);

			item->setQuantity(1);

//#if defined(_GAME)
//			trap->Print("^1*** ^3INVENTORY-GAME^5: Weapon Mod %i. Name: %s\n", allInventoryItemsCount, item->getName());
//#endif

			allInventoryItems[allInventoryItemsCount] = item;
			allInventoryItemsCount++;
		}
	}


	allInventoryWeaponModsEnd = allInventoryItemsCount - 1;

	int numWeaponMods = allInventoryWeaponModsEnd - allInventoryWeaponModsStart;


	//
	// Weapon Crystals...
	//

	bgItemID = 33;

	allInventoryWeaponCrystalsStart = allInventoryItemsCount;

	for (int quality = QUALITY_GREEN; quality <= QUALITY_GOLD; quality++)
	{
		float roll = StatRollForQuality(quality);

		for (int crystal = ITEM_CRYSTAL_RED; crystal < ITEM_CRYSTAL_MAX; crystal++)
		{
			inventoryItem *item = new inventoryItem(allInventoryItemsCount, bgItemID, (itemQuality_t)quality, 1, -1);

			item->setItemID(allInventoryItemsCount);
			item->setBaseItem(bgItemID);
			item->setQuality((itemQuality_t)quality);
			item->setCrystal((itemPowerCrystal_t)crystal);

			item->setQuantity(1);

			//#if defined(_GAME)
			//			trap->Print("^1*** ^3INVENTORY-GAME^5: Weapon Crystal %i. Name: %s\n", allInventoryItemsCount, item->getName());
			//#endif

			allInventoryItems[allInventoryItemsCount] = item;
			allInventoryItemsCount++;
		}
	}

	allInventoryWeaponCrystalsEnd = allInventoryItemsCount - 1;

	int numWeaponCrystals = allInventoryWeaponCrystalsEnd - allInventoryWeaponCrystalsStart;

	//
	//
	//

#if defined(rd_warzone_x86_EXPORTS)
	ri->Printf(PRINT_ALL, "^1*** ^3INVENTORY-GUI^5: Generated %i total inventory possibilities.\n", allInventoryItemsCount);
	ri->Printf(PRINT_ALL, "^1*** ^3INVENTORY-GUI^5: %i base sabers possible (plus %i saber mods and %i saber crystals).\n", numSabers, numSaberMods, numSaberCrystals);
	ri->Printf(PRINT_ALL, "^1*** ^3INVENTORY-GUI^5: %i base weapons possible (plus %i weapon mods and %i weapon crystals).\n", numWeapons, numWeaponMods, numWeaponCrystals);
#elif defined(_CGAME) || defined(_GAME)
	trap->Print("^1*** ^3INVENTORY^5: Generated %i total inventory possibilities.\n", allInventoryItemsCount);
	trap->Print("^1*** ^3INVENTORY^5: %i base sabers possible (plus %i saber mods and %i saber crystals).\n", numSabers, numSaberMods, numSaberCrystals);
	trap->Print("^1*** ^3INVENTORY^5: %i base weapons possible (plus %i weapon mods and %i weapon crystals).\n", numWeapons, numWeaponMods, numWeaponCrystals);
#else //!defined(rd_warzone_x86_EXPORTS)
	// Probably UI, don't need this there...
#endif //defined(rd_warzone_x86_EXPORTS)
}

#if defined(_GAME)
inventoryItem *BG_FindBaseInventoryItem(int bgItemID, int quality, int crystal, int stat1, int stat2, int stat3)
{
	for (int i = 0; i < allInventoryItemsCount; i++)
	{
		inventoryItem *item = allInventoryItems[i];
		
		if (item->getBaseItemID() != bgItemID) continue;
		if (item->getQuality() != quality) continue;
		if (item->getCrystal() != crystal) continue;
		if (item->getBasicStat1() != stat1) continue;
		if (item->getBasicStat2() != stat2) continue;
		if (item->getBasicStat3() != stat3) continue;

		return item;
	}

	return NULL;
}

void BG_CreatePlayerInventoryItem(playerState_t *ps, int psSlot, int bgItemID, int quality, int crystal, int stat1, int stat2, int stat3)
{
	if (!ps) return;
	if (psSlot > 63) return;

	if (psSlot < 0)
	{// Find a slot...
		for (int i = 0; i < 64; i++)
		{
			if (ps->inventoryItems[i] < 0)
			{
				psSlot = i;
				break;
			}
		}
	}

	inventoryItem *item = BG_FindBaseInventoryItem(bgItemID, quality, crystal, stat1, stat2, stat3);

	if (item)
	{
		ps->inventoryItems[psSlot] = item->getItemID();
	}
}
#endif //defined(_GAME)
