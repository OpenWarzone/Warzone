// cmodel.c -- model loading
#include "cm_local.h"
#include "qcommon/qfiles.h"

#ifdef _WIN32
#define __BSP_USE_SHARED_MEMORY__

#include "../SharedMemory/sharedMemory.h"
#endif //_WIN32

void *ShaderData;
uint32_t ShaderDataCount;
void *LeafsData;
uint32_t LeafsDataCount;
void *LeafBrushesData;
uint32_t LeafBrushesDataCount;
void *LeafSurfacesData;
uint32_t LeafSurfacesDataCount;
void *PlanesData;
uint32_t PlanesDataCount;
void *BrushSidesData;
uint32_t BrushSidesDataCount;
void *BrushesData;
uint32_t BrushesDataCount;
void *SubmodelsData;
uint32_t SubmodelsDataCount;
void *NodesData;
uint32_t NodesDataCount;
void *EntityStringData;
uint32_t EntityStringDataCount;
void *VisibilityData;
uint32_t VisibilityDataClusterCount;
uint32_t VisibilityDataClusterBytesCount;
void *PatchesData;
uint32_t PatchesDataCount;

//#define __RANDOM_TERRAIN_GENERATOR__

#ifdef __RANDOM_TERRAIN_GENERATOR__
#include "../terrainGenerator/rfHillTerrain.h"

using namespace RobotFrog; // the class is in this namespace
void Terrain(void)
{
	HillTerrain *terrain = new RobotFrog::HillTerrain/*terrain()*/; // you can also specify terrain params here

	terrain->SetIsland(true);
	terrain->Generate();
	
	for (int x = 0; x < terrain->GetSize(); ++x)
	{
		for (int y = 0; y < terrain->GetSize(); ++y)
		{
			float z = terrain->GetCell(x, y);
			// do whatever with the z values here to draw your terrain
		}
	}
}
#endif //__RANDOM_TERRAIN_GENERATOR__

#ifdef BSPC

#include "../bspc/l_qfiles.h"

void SetPlaneSignbits (cplane_t *out) {
	int	bits, j;

	// for fast box on planeside test
	bits = 0;
	for (j=0 ; j<3 ; j++) {
		if (out->normal[j] < 0) {
			bits |= 1<<j;
		}
	}
	out->signbits = bits;
}
#endif //BSPC

// to allow boxes to be treated as brush models, we allocate
// some extra indexes along with those needed by the map
#define	BOX_BRUSHES		1
#define	BOX_SIDES		6
#define	BOX_LEAFS		2
#define	BOX_PLANES		12

#define	LL(x) x=LittleLong(x)


clipMap_t	cmg; //rwwRMG - changed from cm
int			c_pointcontents;
int			c_traces, c_brush_traces, c_patch_traces;


byte		*cmod_base;

#ifndef BSPC
cvar_t		*cm_noAreas;
cvar_t		*cm_noCurves;
cvar_t		*cm_playerCurveClip;
cvar_t		*cm_extraVerbose;
#endif

cmodel_t	box_model;
cplane_t	*box_planes;
cbrush_t	*box_brush;



void	CM_InitBoxHull (void);
void	CM_FloodAreaConnections (clipMap_t &cm);

//rwwRMG - added:
clipMap_t	SubBSP[MAX_SUB_BSP];
int			NumSubBSP, TotalSubModels;

/*
===============================================================================

					MAP LOADING

===============================================================================
*/

qboolean StringsContainWord ( const char *heystack, const char *heystack2,  char *needle )
{
	if (StringContainsWord(heystack, needle)) return qtrue;
	if (StringContainsWord(heystack2, needle)) return qtrue;
	return qfalse;
}

qboolean IsKnownShinyMap2 ( const char *heystack )
{
	if (StringContainsWord(heystack, "/players/")) return qfalse;
	if (StringContainsWord(heystack, "/bespin/")) return qtrue;
	if (StringContainsWord(heystack, "/cloudcity/")) return qtrue;
	if (StringContainsWord(heystack, "/byss/")) return qtrue;
	if (StringContainsWord(heystack, "/cairn/")) return qtrue;
	if (StringContainsWord(heystack, "/doomgiver/")) return qtrue;
	if (StringContainsWord(heystack, "/factory/")) return qtrue;
	if (StringContainsWord(heystack, "/hoth/")) return qtrue;
	if (StringContainsWord(heystack, "/impdetention/")) return qtrue;
	if (StringContainsWord(heystack, "/imperial/")) return qtrue;
	if (StringContainsWord(heystack, "/impgarrison/")) return qtrue;
	if (StringContainsWord(heystack, "/kejim/")) return qtrue;
	if (StringContainsWord(heystack, "/nar_hideout/")) return qtrue;
	if (StringContainsWord(heystack, "/nar_streets/")) return qtrue;
	if (StringContainsWord(heystack, "/narshaddaa/")) return qtrue;
	if (StringContainsWord(heystack, "/rail/")) return qtrue;
	if (StringContainsWord(heystack, "/rooftop/")) return qtrue;
	if (StringContainsWord(heystack, "/taspir/")) return qtrue; // lots of metal... will try this
	if (StringContainsWord(heystack, "/vjun/")) return qtrue;
	if (StringContainsWord(heystack, "/wedge/")) return qtrue;
	
	// MB2 Maps...
	if (StringContainsWord(heystack, "/epiii_boc/")) return qtrue;
	if (StringContainsWord(heystack, "/amace_cc/")) return qtrue;
	if (StringContainsWord(heystack, "/bespinsp/")) return qtrue;
	if (StringContainsWord(heystack, "/imm_cc/")) return qtrue;
	if (StringContainsWord(heystack, "/com_tower/")) return qtrue;
	if (StringContainsWord(heystack, "/imperial_tram/")) return qtrue;
	if (StringContainsWord(heystack, "/corellia/")) return qtrue;
	if (StringContainsWord(heystack, "/mb2_outlander/")) return qtrue;
	if (StringContainsWord(heystack, "/falcon_sjc/")) return qtrue;
	if (StringContainsWord(heystack, "/second-deathstar/")) return qtrue;
	if (StringContainsWord(heystack, "/thedeathstar/")) return qtrue;
	if (StringContainsWord(heystack, "/casa_del_paria/")) return qtrue;
	if (StringContainsWord(heystack, "/evil3_")) return qtrue;
	if (StringContainsWord(heystack, "/hanger/")) return qtrue;
	if (StringContainsWord(heystack, "/naboo/")) return qtrue;
	if (StringContainsWord(heystack, "/shinfl/")) return qtrue;
	if (StringContainsWord(heystack, "/hangar/")) return qtrue;
	if (StringContainsWord(heystack, "/lab/")) return qtrue;
	if (StringContainsWord(heystack, "/mainhall/")) return qtrue;
	if (StringContainsWord(heystack, "/amace_kamino/")) return qtrue;
	if (StringContainsWord(heystack, "/kamino/")) return qtrue;
	if (StringContainsWord(heystack, "/mb2_kamino/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_Kamino/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_Mustafar/")) return qtrue; // hmm... maybe???
	if (StringContainsWord(heystack, "/mygeeto1a/")) return qtrue;
	if (StringContainsWord(heystack, "/mygeeto1c/")) return qtrue;
	if (StringContainsWord(heystack, "/ddee_hangarc/")) return qtrue;
	if (StringContainsWord(heystack, "/droidee/")) return qtrue;
	if (StringContainsWord(heystack, "/amace_detention/")) return qtrue;
	if (StringContainsWord(heystack, "/amace_leviathan/")) return qtrue;
	if (StringContainsWord(heystack, "/amace_reactor/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_battle_over/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_palp/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_starship/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_utapua/")) return qtrue;
	if (StringContainsWord(heystack, "/mace_dark/")) return qtrue;
	if (StringContainsWord(heystack, "/mace_hanger/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_TantIV/")) return qtrue;
	if (StringContainsWord(heystack, "/ship/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_Exec/")) return qtrue;
	if (StringContainsWord(heystack, "/tantive/")) return qtrue;
	if (StringContainsWord(heystack, "/tantive1/")) return qtrue;
	if (StringContainsWord(heystack, "/MMT/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_Hangar/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_TFed/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_TFedTOO/")) return qtrue;
	if (StringContainsWord(heystack, "/plasma_TradeFed/")) return qtrue;

	// Misc Maps...
	if (StringContainsWord(heystack, "/atlantica/")) return qtrue;
	if (StringContainsWord(heystack, "/Carida/")) return qtrue;
	if (StringContainsWord(heystack, "/bunker/")) return qtrue;
	if (StringContainsWord(heystack, "/DF/")) return qtrue;
	if (StringContainsWord(heystack, "/bespinnew/")) return qtrue;
	if (StringContainsWord(heystack, "/cloudcity/")) return qtrue;
	if (StringContainsWord(heystack, "/coruscantsjc/")) return qtrue;
	if (StringContainsWord(heystack, "/ffawedge/")) return qtrue;
	if (StringContainsWord(heystack, "/jenshotel/")) return qtrue;
	if (StringContainsWord(heystack, "/CoruscantStreets/")) return qtrue;
	if (StringContainsWord(heystack, "/ctf_fighterbays/")) return qtrue;
	if (StringContainsWord(heystack, "/e3sjc/")) return qtrue;
	if (StringContainsWord(heystack, "/mustafar_sjc/")) return qtrue;
	if (StringContainsWord(heystack, "/fearis/")) return qtrue;
	if (StringContainsWord(heystack, "/kotor_dantooine/")) return qtrue;
	if (StringContainsWord(heystack, "/kotor_ebon_hawk/")) return qtrue;
	if (StringContainsWord(heystack, "/deltaphantom/")) return qtrue;
	if (StringContainsWord(heystack, "/pass_me_around/")) return qtrue;
	if (StringContainsWord(heystack, "/AMegaCity/")) return qtrue;
	if (StringContainsWord(heystack, "/mantell")) return qtrue;
	if (StringContainsWord(heystack, "/rcruiser/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_padme/")) return qtrue;
	if (StringContainsWord(heystack, "/Anaboo/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_imort/")) return qtrue;
	if (StringContainsWord(heystack, "/Asjc_mygeeto/")) return qtrue;
	if (StringContainsWord(heystack, "/ACrimeHutt/")) return qtrue;
	if (StringContainsWord(heystack, "/ASenateBase/")) return qtrue;

	// Warzone
	if (StringContainsWord(heystack, "/impfact/")) return qtrue;
	
	return qfalse;
}

qboolean IsKnownShinyMap ( const char *heystack )
{
	if (IsKnownShinyMap2( heystack ))
	{
		return qtrue;
	}
	
	return qfalse;
}

qboolean HaveSurfaceType( int materialType)
{
	switch(materialType)
	{
	case MATERIAL_WATER:			// 13			// light covering of water on a surface
	case MATERIAL_SHORTGRASS:		// 5			// manicured lawn
	case MATERIAL_LONGGRASS:		// 6			// long jungle grass
	case MATERIAL_SAND:				// 8			// sandy beach
	case MATERIAL_CARPET:			// 27			// lush carpet
	case MATERIAL_GRAVEL:			// 9			// lots of small stones
	case MATERIAL_ROCK:				// 23			//
	case MATERIAL_TILES:			// 26			// tiled floor
	case MATERIAL_SOLIDWOOD:		// 1			// freshly cut timber
	case MATERIAL_HOLLOWWOOD:		// 2			// termite infested creaky wood
	case MATERIAL_POLISHEDWOOD:
	case MATERIAL_SOLIDMETAL:		// 3			// solid girders
	case MATERIAL_HOLLOWMETAL:		// 4			// hollow metal machines
	case MATERIAL_DRYLEAVES:		// 19			// dried up leaves on the floor
	case MATERIAL_GREENLEAVES:		// 20			// fresh leaves still on a tree
	case MATERIAL_FABRIC:			// 21			// Cotton sheets
	case MATERIAL_CANVAS:			// 22			// tent material
	case MATERIAL_MARBLE:			// 12			// marble floors
	case MATERIAL_SNOW:				// 14			// freshly laid snow
	case MATERIAL_MUD:				// 17			// wet soil
	case MATERIAL_DIRT:				// 7			// hard mud
	case MATERIAL_CONCRETE:			// 11			// hardened concrete pavement
	case MATERIAL_FLESH:			// 16			// hung meat, corpses in the world
	case MATERIAL_RUBBER:			// 24			// hard tire like rubber
	case MATERIAL_PLASTIC:			// 25			//
	case MATERIAL_PLASTER:			// 28			// drywall style plaster
	case MATERIAL_SHATTERGLASS:		// 29			// glass with the Crisis Zone style shattering
	case MATERIAL_ARMOR:			// 30			// body armor
	case MATERIAL_ICE:				// 15			// packed snow/solid ice
	case MATERIAL_GLASS:			// 10			//
	case MATERIAL_BPGLASS:			// 18			// bulletproof glass
	case MATERIAL_COMPUTER:			// 31			// computers/electronic equipment
	case MATERIAL_PUDDLE:
	case MATERIAL_LAVA:
	case MATERIAL_TREEBARK:
	case MATERIAL_STONE:
	case MATERIAL_EFX:
	case MATERIAL_BLASTERBOLT:
	case MATERIAL_FIRE:
	case MATERIAL_SMOKE:
	case MATERIAL_MAGIC_PARTICLES:
	case MATERIAL_MAGIC_PARTICLES_TREE:
	case MATERIAL_FIREFLIES:
	case MATERIAL_PORTAL:
	case MATERIAL_MENU_BACKGROUND:
	case MATERIAL_SKYSCRAPER:
	case MATERIAL_DISTORTEDGLASS:
	case MATERIAL_DISTORTEDPUSH:
	case MATERIAL_DISTORTEDPULL:
	case MATERIAL_CLOAK:
	case MATERIAL_FORCEFIELD:
	case MATERIAL_BIRD:
		return qtrue;
		break;
	default:
		break;
	}

	return qfalse;
}

int DetectMaterialType(const char *name)
{
	if (StringContainsWord(name, "gfx/water"))
		return MATERIAL_NONE;
	if (StringContainsWord(name, "gfx/atmospheric"))
		return MATERIAL_NONE;
	if (StringContainsWord(name, "warzone/plant"))
		return MATERIAL_GREENLEAVES;
	else if ((StringContainsWord(name, "yavin/tree2b") || StringContainsWord(name, "yavin/tree05") || StringContainsWord(name, "yavin/tree06"))
		&& !(StringContainsWord(name, "yavin/tree05_vines") || StringContainsWord(name, "yavin/tree06b")))
		return MATERIAL_TREEBARK;
	else if ((StringContainsWord(name, "yavin/tree08") || StringContainsWord(name, "yavin/tree09"))
		&& !(StringContainsWord(name, "yavin/tree08b") || StringContainsWord(name, "yavin/tree09_vines") || StringContainsWord(name, "yavin/tree09a") || StringContainsWord(name, "yavin/tree09b") || StringContainsWord(name, "yavin/tree09d")))
		return MATERIAL_TREEBARK;

	//
	// Special cases - where we are pretty sure we want lots of specular and reflection...
	//
	else if (StringContainsWord(name, "jetpack"))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "plastic") || StringContainsWord(name, "stormtrooper") || StringContainsWord(name, "snowtrooper") || StringContainsWord(name, "medpac") || StringContainsWord(name, "bacta") || StringContainsWord(name, "helmet") || StringContainsWord(name, "feather"))
		return MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "/ships/") || StringContainsWord(name, "engine") || StringContainsWord(name, "mp/flag"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "wing") || StringContainsWord(name, "xwbody") || StringContainsWord(name, "tie_"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "ship") || StringContainsWord(name, "shuttle") || StringContainsWord(name, "falcon"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "freight") || StringContainsWord(name, "transport") || StringContainsWord(name, "crate"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "container") || StringContainsWord(name, "barrel") || StringContainsWord(name, "train"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "crane") || StringContainsWord(name, "plate") || StringContainsWord(name, "cargo"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "ship_"))
		return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
	else if (!StringContainsWord(name, "trainer") && StringContainsWord(name, "train"))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "reborn") || StringContainsWord(name, "trooper"))
		return MATERIAL_ARMOR;
	else if (StringContainsWord(name, "boba") || StringContainsWord(name, "pilot"))
		return MATERIAL_ARMOR;
	else if (StringContainsWord(name, "water") && !StringContainsWord(name, "splash") && !StringContainsWord(name, "drip") && !StringContainsWord(name, "ripple") && !StringContainsWord(name, "bubble") && !StringContainsWord(name, "woosh") && !StringContainsWord(name, "underwater") && !StringContainsWord(name, "bottom"))
	{
		return MATERIAL_WATER;
	}
	else if (StringContainsWord(name, "grass") || StringContainsWord(name, "foliage") || StringContainsWord(name, "yavin/ground") || StringContainsWord(name, "mp/s_ground") || StringContainsWord(name, "volcano/terrain") || StringContainsWord(name, "bay/terrain") || StringContainsWord(name, "towers/terrain") || StringContainsWord(name, "yavinassault/terrain"))
		return MATERIAL_SHORTGRASS;
	else if (StringContainsWord(name, "vj4")) // special case for vjun rock...
		return MATERIAL_ROCK;

	//
	// Player model stuff overrides
	//
	else if (StringContainsWord(name, "players") && StringContainsWord(name, "eye") || StringContainsWord(name, "goggles"))
		return MATERIAL_GLASS;
	else if (StringContainsWord(name, "bespin/bench") && StringContainsWord(name, "bespin/light"))
		return MATERIAL_HOLLOWMETAL;
	else if (!StringContainsWord(name, "players") && StringContainsWord(name, "bespin"))
		return MATERIAL_MARBLE;
	else if (StringContainsWord(name, "players") && !StringContainsWord(name, "glass") && (StringContainsWord(name, "sithsoldier") || StringContainsWord(name, "sith_assassin") || StringContainsWord(name, "r2d2") || StringContainsWord(name, "protocol") || StringContainsWord(name, "r5d2") || StringContainsWord(name, "c3po") || StringContainsWord(name, "hk4") || StringContainsWord(name, "hk5") || StringContainsWord(name, "droid") || StringContainsWord(name, "shadowtrooper")))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "players") && StringContainsWord(name, "shadowtrooper"))
		return MATERIAL_SOLIDMETAL; // dunno about this one.. looks good as armor...
	else if (StringContainsWord(name, "players") && (StringContainsWord(name, "mandalore") || StringContainsWord(name, "mandalorian") || StringContainsWord(name, "sith_warrior/body")))
		return MATERIAL_ARMOR;
	else if (StringContainsWord(name, "players") && (StringContainsWord(name, "hood") || StringContainsWord(name, "robe") || StringContainsWord(name, "cloth") || StringContainsWord(name, "pants") || StringContainsWord(name, "sith_warrior/bandon_body")))
		return MATERIAL_FABRIC;
	else if (StringContainsWord(name, "players") && (StringContainsWord(name, "hair") || StringContainsWord(name, "chewbacca"))) // use carpet
		return MATERIAL_FABRIC;//MATERIAL_CARPET; Just because it has a bit of parallax and suitable specular...
	else if (StringContainsWord(name, "players") && (StringContainsWord(name, "flesh") || StringContainsWord(name, "body") || StringContainsWord(name, "leg") || StringContainsWord(name, "hand") || StringContainsWord(name, "head") || StringContainsWord(name, "hips") || StringContainsWord(name, "torso") || StringContainsWord(name, "tentacles") || StringContainsWord(name, "face") || StringContainsWord(name, "arms") || StringContainsWord(name, "sith_warrior/head") || StringContainsWord(name, "sith_warrior/bandon_head")))
		return MATERIAL_FLESH;
	else if (StringContainsWord(name, "players") && (StringContainsWord(name, "arm") || StringContainsWord(name, "foot") || StringContainsWord(name, "neck")))
		return MATERIAL_FLESH;
	else if (StringContainsWord(name, "players") && (StringContainsWord(name, "skirt") || StringContainsWord(name, "boots") || StringContainsWord(name, "accesories") || StringContainsWord(name, "accessories") || StringContainsWord(name, "vest") || StringContainsWord(name, "holster") || StringContainsWord(name, "cap") || StringContainsWord(name, "collar")))
		return MATERIAL_FABRIC;
	else if (!StringContainsWord(name, "players") && StringContainsWord(name, "_cc"))
		return MATERIAL_MARBLE;
	//
	// If player model material not found above, use defaults...
	//

	//
	// Stuff we can be pretty sure of...
	//
	else if (StringContainsWord(name, "concrete"))
		return MATERIAL_CONCRETE;
	else if (!StringContainsWord(name, "glow") && !StringContainsWord(name, "glw") && StringContainsWord(name, "models/weapon") && StringContainsWord(name, "saber") && StringContainsWord(name, "bespin/bench") && StringContainsWord(name, "bespin/light"))
		return MATERIAL_HOLLOWMETAL; // UQ1: Using hollowmetal for weapons to force low parallax setting...
	else if (!StringContainsWord(name, "glow") && !StringContainsWord(name, "glw") && (StringContainsWord(name, "models/weapon") || StringContainsWord(name, "scope") || StringContainsWord(name, "blaster") || StringContainsWord(name, "pistol") || StringContainsWord(name, "thermal") || StringContainsWord(name, "bowcaster") || StringContainsWord(name, "cannon") || StringContainsWord(name, "saber") || StringContainsWord(name, "rifle") || StringContainsWord(name, "rocket")))
		return MATERIAL_HOLLOWMETAL; // UQ1: Using hollowmetal for weapons to force low parallax setting...
	else if (StringContainsWord(name, "metal") || StringContainsWord(name, "pipe") || StringContainsWord(name, "shaft") || StringContainsWord(name, "jetpack") || StringContainsWord(name, "antenna") || StringContainsWord(name, "xwing") || StringContainsWord(name, "tie_") || StringContainsWord(name, "raven") || StringContainsWord(name, "falcon") || StringContainsWord(name, "engine") || StringContainsWord(name, "elevator") || StringContainsWord(name, "evaporator") || StringContainsWord(name, "airpur") || StringContainsWord(name, "gonk") || StringContainsWord(name, "droid") || StringContainsWord(name, "cart") || StringContainsWord(name, "vent") || StringContainsWord(name, "tank") || StringContainsWord(name, "transformer") || StringContainsWord(name, "generator") || StringContainsWord(name, "grate") || StringContainsWord(name, "rack") || StringContainsWord(name, "mech") || StringContainsWord(name, "turbolift") || StringContainsWord(name, "tube") || StringContainsWord(name, "coil") || StringContainsWord(name, "vader_trim") || StringContainsWord(name, "newfloor_vjun") || StringContainsWord(name, "bay_beam"))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "armor") || StringContainsWord(name, "armour"))
		return MATERIAL_ARMOR;
	else if (StringContainsWord(name, "textures/byss/") && !StringContainsWord(name, "glow") && !StringContainsWord(name, "glw") && !StringContainsWord(name, "static") && !StringContainsWord(name, "isd") && !StringContainsWord(name, "power") && !StringContainsWord(name, "env_") && !StringContainsWord(name, "byss_switch"))
		return MATERIAL_SOLIDMETAL; // special for byss shiny
	else if (StringContainsWord(name, "textures/vjun/") && !StringContainsWord(name, "glow") && !StringContainsWord(name, "glw") && !StringContainsWord(name, "static") && !StringContainsWord(name, "light") && !StringContainsWord(name, "env_") && !StringContainsWord(name, "_env") && !StringContainsWord(name, "switch_off") && !StringContainsWord(name, "switch_on") && !StringContainsWord(name, "screen") && !StringContainsWord(name, "blend") && !StringContainsWord(name, "o_ground") && !StringContainsWord(name, "_onoffg") && !StringContainsWord(name, "_onoffr") && !StringContainsWord(name, "console"))
		return MATERIAL_SOLIDMETAL; // special for vjun shiny
	else if (StringContainsWord(name, "sand"))
		return MATERIAL_SAND;
	else if (StringContainsWord(name, "gravel"))
		return MATERIAL_GRAVEL;
	else if ((StringContainsWord(name, "dirt") || StringContainsWord(name, "ground")) && !StringContainsWord(name, "menus/main_background"))
		return MATERIAL_DIRT;
	else if (IsKnownShinyMap(name) && StringContainsWord(name, "stucco"))
		return MATERIAL_TILES;
	else if (StringContainsWord(name, "rift") && StringContainsWord(name, "piller"))
		return MATERIAL_MARBLE;
	else if (StringContainsWord(name, "stucco") || StringContainsWord(name, "piller") || StringContainsWord(name, "sith_jp"))
		return MATERIAL_CONCRETE;
	else if (StringContainsWord(name, "marbl") || StringContainsWord(name, "teeth"))
		return MATERIAL_MARBLE;
	else if (StringContainsWord(name, "snow"))
		return MATERIAL_SNOW;
	else if (StringContainsWord(name, "canvas"))
		return MATERIAL_CANVAS;
	else if (StringContainsWord(name, "rock"))
		return MATERIAL_ROCK;
	else if (StringContainsWord(name, "rubber"))
		return MATERIAL_RUBBER;
	else if (StringContainsWord(name, "carpet"))
		return MATERIAL_CARPET;
	else if (StringContainsWord(name, "plaster"))
		return MATERIAL_PLASTER;
	else if (StringContainsWord(name, "computer") || StringContainsWord(name, "console") || StringContainsWord(name, "button") || StringContainsWord(name, "terminal") || StringContainsWord(name, "switch") || StringContainsWord(name, "panel") || StringContainsWord(name, "control"))
		return MATERIAL_COMPUTER;
	else if (StringContainsWord(name, "fabric"))
		return MATERIAL_FABRIC;
	else if (StringContainsWord(name, "tree") || StringContainsWord(name, "leaf") || StringContainsWord(name, "leaves") || StringContainsWord(name, "fern") || StringContainsWord(name, "vine"))
		return MATERIAL_GREENLEAVES;
	else if (StringContainsWord(name, "bamboo"))
		return MATERIAL_TREEBARK;
	else if (StringContainsWord(name, "wood") && !StringContainsWord(name, "street"))
		return MATERIAL_SOLIDWOOD;
	else if (StringContainsWord(name, "mud"))
		return MATERIAL_MUD;
	else if (StringContainsWord(name, "ice"))
		return MATERIAL_ICE;
	else if ((StringContainsWord(name, "grass") || StringContainsWord(name, "foliage")) && (StringContainsWord(name, "long") || StringContainsWord(name, "tall") || StringContainsWord(name, "thick")))
		return MATERIAL_LONGGRASS;
	else if (StringContainsWord(name, "grass") || StringContainsWord(name, "foliage"))
		return MATERIAL_SHORTGRASS;
	else if (IsKnownShinyMap(name) && StringContainsWord(name, "floor"))
		return MATERIAL_TILES;
	else if (StringContainsWord(name, "floor"))
		return MATERIAL_CONCRETE;
	else if (StringContainsWord(name, "textures/mp/") && !StringContainsWord(name, "glow") && !StringContainsWord(name, "glw") && !StringContainsWord(name, "static") && !StringContainsWord(name, "light") && !StringContainsWord(name, "env_") && !StringContainsWord(name, "_env") && !StringContainsWord(name, "underside") && !StringContainsWord(name, "blend") && !StringContainsWord(name, "t_pit") && !StringContainsWord(name, "desert") && !StringContainsWord(name, "cliff"))
		return MATERIAL_SOLIDMETAL; // special for mp shiny
	else if (IsKnownShinyMap(name) && StringContainsWord(name, "frame"))
		return MATERIAL_SOLIDMETAL;
	else if (IsKnownShinyMap(name) && StringContainsWord(name, "wall"))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "wall") || StringContainsWord(name, "underside"))
		return MATERIAL_CONCRETE;
	else if (IsKnownShinyMap(name) && StringContainsWord(name, "door"))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "door"))
		return MATERIAL_CONCRETE;
	else if (IsKnownShinyMap(name) && StringContainsWord(name, "ground"))
		return MATERIAL_TILES; // dunno about this one
	else if (StringContainsWord(name, "ground"))
		return MATERIAL_CONCRETE;
	else if (StringContainsWord(name, "desert"))
		return MATERIAL_CONCRETE;
	else if (IsKnownShinyMap(name) && (StringContainsWord(name, "tile") || StringContainsWord(name, "lift")))
		return MATERIAL_SOLIDMETAL;
	else if (StringContainsWord(name, "tile") || StringContainsWord(name, "lift"))
		return MATERIAL_TILES;
	else if (StringContainsWord(name, "glass") || StringContainsWord(name, "light") || StringContainsWord(name, "screen") || StringContainsWord(name, "lamp") || StringContainsWord(name, "crystal"))
		return MATERIAL_GLASS;
	else if (StringContainsWord(name, "flag"))
		return MATERIAL_FABRIC;
	else if (StringContainsWord(name, "column") || StringContainsWord(name, "stone") || StringContainsWord(name, "statue"))
		return MATERIAL_MARBLE;
	// Extra backup - backup stuff. Used when nothing better found...
	else if (StringContainsWord(name, "red") || StringContainsWord(name, "blue") || StringContainsWord(name, "yellow") || StringContainsWord(name, "white") || StringContainsWord(name, "monitor"))
		return MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "yavin") && (StringContainsWord(name, "trim") || StringContainsWord(name, "step") || StringContainsWord(name, "pad")))
		return MATERIAL_STONE;
	else if (!StringContainsWord(name, "players") && (StringContainsWord(name, "coruscant") || StringContainsWord(name, "/rooftop/") || StringContainsWord(name, "/nar_") || StringContainsWord(name, "/imperial/")))
		return MATERIAL_TILES;
	else if (!StringContainsWord(name, "players") && (StringContainsWord(name, "deathstar") || StringContainsWord(name, "imperial") || StringContainsWord(name, "shuttle") || StringContainsWord(name, "destroyer")))
		return MATERIAL_TILES;
	else if (!StringContainsWord(name, "players") && StringContainsWord(name, "dantooine"))
		return MATERIAL_MARBLE;
	else if (StringContainsWord(name, "outside"))
		return MATERIAL_CONCRETE; // Outside, assume concrete...
	else if (StringContainsWord(name, "out") && (StringContainsWord(name, "trim") || StringContainsWord(name, "step") || StringContainsWord(name, "pad")))
		return MATERIAL_CONCRETE; // Outside, assume concrete...
	else if (StringContainsWord(name, "out") && (StringContainsWord(name, "frame") || StringContainsWord(name, "wall") || StringContainsWord(name, "round") || StringContainsWord(name, "crate") || StringContainsWord(name, "trim") || StringContainsWord(name, "support") || StringContainsWord(name, "step") || StringContainsWord(name, "pad") || StringContainsWord(name, "weapon") || StringContainsWord(name, "gun")))
		return MATERIAL_CONCRETE; // Outside, assume concrete...
	else if (StringContainsWord(name, "frame") || StringContainsWord(name, "wall") || StringContainsWord(name, "round") || StringContainsWord(name, "crate") || StringContainsWord(name, "trim") || StringContainsWord(name, "support") || StringContainsWord(name, "step") || StringContainsWord(name, "pad") || StringContainsWord(name, "weapon") || StringContainsWord(name, "gun"))
		return MATERIAL_CONCRETE;
	else if (StringContainsWord(name, "yavin"))
		return MATERIAL_STONE; // On yavin maps, assume rock for anything else...
	else if (StringContainsWord(name, "black") || StringContainsWord(name, "boon") || StringContainsWord(name, "items") || StringContainsWord(name, "shield"))
		return MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "refract") || StringContainsWord(name, "reflect"))
		return MATERIAL_PLASTIC;
	else if (StringContainsWord(name, "map_objects") || StringContainsWord(name, "key"))
		return MATERIAL_SOLIDMETAL; // hmmm, maybe... testing...
	else if (StringContainsWord(name, "rodian"))
		return MATERIAL_FLESH;
	else if (StringContainsWord(name, "players")) // Fall back to flesh on anything not caught above...
		return MATERIAL_FLESH;
	else if (IsKnownShinyMap(name)) // Chances are it's shiny...
		return MATERIAL_TILES;

	return MATERIAL_CONCRETE;// MATERIAL_NONE;
}

int GetMaterialType ( const char *name, int materialType )
{
	if (StringContainsWord(name, "gfx/2d")
		|| StringContainsWord(name, "gfx/console")
		|| StringContainsWord(name, "gfx/colors")
		|| StringContainsWord(name, "gfx/digits")
		|| StringContainsWord(name, "gfx/hud")
		|| StringContainsWord(name, "gfx/jkg")
		|| StringContainsWord(name, "gfx/menu")) 
		return MATERIAL_EFX;

	if (!HaveSurfaceType(materialType))
	{
		int material = DetectMaterialType(name);

		if (material)
			return material;
	}
	else
	{
		if (StringContainsWord(name, "gfx/water"))
			return MATERIAL_EFX;
		else if (StringContainsWord(name, "gfx/atmospheric"))
			return MATERIAL_EFX;
		else if (StringContainsWord(name, "warzone/plant"))
			return MATERIAL_GREENLEAVES;
		else if ((StringContainsWord(name, "yavin/tree2b") || StringContainsWord(name, "yavin/tree05") || StringContainsWord(name, "yavin/tree06"))
			&& !(StringContainsWord(name, "yavin/tree05_vines") || StringContainsWord(name, "yavin/tree06b")))
			return MATERIAL_TREEBARK;
		else if ((StringContainsWord(name, "yavin/tree08") || StringContainsWord(name, "yavin/tree09"))
			&& !(StringContainsWord(name, "yavin/tree08b") || StringContainsWord(name, "yavin/tree09_vines") || StringContainsWord(name, "yavin/tree09a") || StringContainsWord(name, "yavin/tree09b") || StringContainsWord(name, "yavin/tree09d")))
			return MATERIAL_TREEBARK;

		//
		// Special cases - where we are pretty sure we want lots of specular and reflection... Override!
		//
		else if (StringContainsWord(name, "vj4")) // special case for vjun rock...
			return MATERIAL_ROCK;
		else if (StringContainsWord(name, "plastic") || StringContainsWord(name, "stormtrooper") || StringContainsWord(name, "snowtrooper") || StringContainsWord(name, "medpac") || StringContainsWord(name, "bacta") || StringContainsWord(name, "helmet"))
			return MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "/ships/") || StringContainsWord(name, "engine") || StringContainsWord(name, "mp/flag"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "wing") || StringContainsWord(name, "xwbody") || StringContainsWord(name, "tie_"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "ship") || StringContainsWord(name, "shuttle") || StringContainsWord(name, "falcon"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "freight") || StringContainsWord(name, "transport") || StringContainsWord(name, "crate"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "container") || StringContainsWord(name, "barrel") || StringContainsWord(name, "train"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "crane") || StringContainsWord(name, "plate") || StringContainsWord(name, "cargo"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "ship_"))
			return MATERIAL_SOLIDMETAL;//MATERIAL_PLASTIC;
		else if (StringContainsWord(name, "models/weapon") && StringContainsWord(name, "saber") && !StringContainsWord(name, "glow") && StringContainsWord(name, "bespin/bench") && StringContainsWord(name, "bespin/light"))
			return MATERIAL_HOLLOWMETAL; // UQ1: Using hollowmetal for weapons to force low parallax setting...
		else if (StringContainsWord(name, "reborn") || StringContainsWord(name, "trooper"))
			return MATERIAL_ARMOR;
		else if (StringContainsWord(name, "boba") || StringContainsWord(name, "pilot"))
			return MATERIAL_ARMOR;
		else if (StringContainsWord(name, "grass") || (StringContainsWord(name, "foliage") && !StringContainsWord(name, "billboard")) || StringContainsWord(name, "yavin/ground") || StringContainsWord(name, "mp/s_ground") || StringContainsWord(name, "yavinassault/terrain"))
			return MATERIAL_SHORTGRASS;

		//
		// Player model stuff overrides
		//
		else if (StringContainsWord(name, "players") && StringContainsWord(name, "eye"))
			return MATERIAL_GLASS;
		else if (StringContainsWord(name, "bespin/bench") && StringContainsWord(name, "bespin/light"))
			return MATERIAL_HOLLOWMETAL;
		else if (!StringContainsWord(name, "players") && StringContainsWord(name, "bespin"))
			return MATERIAL_MARBLE;
		else if (StringContainsWord(name, "players") && (StringContainsWord(name, "sithsoldier") || StringContainsWord(name, "r2d2") || StringContainsWord(name, "protocol") || StringContainsWord(name, "r5d2") || StringContainsWord(name, "c3po")))
			return MATERIAL_SOLIDMETAL;
		else if (StringContainsWord(name, "players") && (StringContainsWord(name, "hood") || StringContainsWord(name, "robe") || StringContainsWord(name, "cloth") || StringContainsWord(name, "pants")))
			return MATERIAL_FABRIC;
		else if (StringContainsWord(name, "players") && (StringContainsWord(name, "hair") || StringContainsWord(name, "chewbacca"))) // use carpet
			return MATERIAL_FABRIC;//MATERIAL_CARPET; Just because it has a bit of parallax and suitable specular...
		else if (StringContainsWord(name, "players") && (StringContainsWord(name, "flesh") || StringContainsWord(name, "body") || StringContainsWord(name, "leg") || StringContainsWord(name, "hand") || StringContainsWord(name, "head") || StringContainsWord(name, "hips") || StringContainsWord(name, "torso") || StringContainsWord(name, "tentacles") || StringContainsWord(name, "face") || StringContainsWord(name, "arms")))
			return MATERIAL_FLESH;
		else if (StringContainsWord(name, "players") && (StringContainsWord(name, "arm") || StringContainsWord(name, "foot") || StringContainsWord(name, "neck")))
			return MATERIAL_FLESH;
		else if (StringContainsWord(name, "players") && (StringContainsWord(name, "skirt") || StringContainsWord(name, "boots") || StringContainsWord(name, "accesories") || StringContainsWord(name, "accessories") || StringContainsWord(name, "vest") || StringContainsWord(name, "holster") || StringContainsWord(name, "cap") || StringContainsWord(name, "collar")))
			return MATERIAL_FABRIC;
		else if (!StringContainsWord(name, "players") && StringContainsWord(name, "_cc"))
			return MATERIAL_MARBLE;
		//
		// If player model material not found above, use defaults...
		//
	}

	if (StringContainsWord(name, "gfx/water"))
		return MATERIAL_EFX;
	else if (StringContainsWord(name, "gfx/atmospheric"))
		return MATERIAL_EFX;
	else if (StringContainsWord(name, "common/water") && !StringContainsWord(name, "splash") && !StringContainsWord(name, "drip") && !StringContainsWord(name, "ripple") && !StringContainsWord(name, "bubble") && !StringContainsWord(name, "woosh") && !StringContainsWord(name, "underwater") && !StringContainsWord(name, "bottom"))
	{
		return MATERIAL_WATER;
	}
	else if (StringContainsWord(name, "vj4"))
	{// special case for vjun rock...
		return MATERIAL_ROCK;
	}

	return MATERIAL_CONCRETE;
}


/*
=================
CMod_LoadShaders
=================
*/
static void CMod_LoadShaders( lump_t *l, clipMap_t &cm )
{
	dshader_t	*in;
	int			i, count;
	CCMShader	*out;

	in = (dshader_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in)) {
		Com_Error (ERR_DROP, "CMod_LoadShaders: funny lump size");
	}
	count = l->filelen / sizeof(*in);

	if (count < 1) {
		Com_Error (ERR_DROP, "Map with no shaders");
	}
	cm.shaders = (CCMShader *)Hunk_Alloc( (1+count) * sizeof( *cm.shaders ), h_high );
	cm.numShaders = count;

	ShaderData = cm.shaders;
	ShaderDataCount = cm.numShaders;

	out = cm.shaders;
	for ( i = 0; i < count; i++, in++, out++ )
	{
		Q_strncpyz(out->shader, in->shader, MAX_QPATH);
		out->contentFlags = LittleLong( in->contentFlags );
		out->surfaceFlags = LittleLong( in->surfaceFlags );
		//out->materialType = LittleLong( in->materialType );
		out->materialType = LittleLong(in->surfaceFlags) & MATERIAL_MASK;
		
		if (in->shader && ( StringContainsWord(in->shader, "warzone/tree") || StringContainsWord(in->shader, "warzone\\tree")))
		{// LOL WTF HAX!!! :)
			if (StringContainsWord(in->shader, "bark") 
				|| StringContainsWord(in->shader, "trunk") 
				|| StringContainsWord(in->shader, "giant_tree") 
				|| StringContainsWord(in->shader, "vine01"))
			{
				in->contentFlags |= (CONTENTS_SOLID | CONTENTS_OPAQUE);
				out->contentFlags = LittleLong( in->contentFlags );
				out->surfaceFlags = LittleLong( in->surfaceFlags );
			}
		}

		if (!HaveSurfaceType(out->materialType))
			out->materialType = LittleLong( GetMaterialType(in->shader, /*in*/out->materialType) );

		if (in->shader && ( StringContainsWord(in->shader, "skies/") || (StringContainsWord(in->shader, "sky") && !StringContainsWord(in->shader, "skyscraper"))))
		{// LOL WTF HAX!!! :)
			in->contentFlags |= (CONTENTS_SOLID | CONTENTS_OPAQUE);
			in->surfaceFlags |= SURF_SKY;
			out->contentFlags = LittleLong( in->contentFlags );
			out->surfaceFlags = LittleLong( in->surfaceFlags );
			Com_Printf("Set SURF_SKY for %s.\n", in->shader);
		}
	}
}


/*
=================
CMod_LoadSubmodels
=================
*/
void CMod_LoadSubmodels( lump_t *l, clipMap_t &cm ) {
	dmodel_t	*in;
	cmodel_t	*out;
	int			i, j, count;
	int			*indexes;

	in = (dmodel_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "CMod_LoadSubmodels: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error (ERR_DROP, "Map with no models");
	cm.cmodels = (struct cmodel_s *)Hunk_Alloc( count * sizeof( *cm.cmodels ), h_high );
	cm.numSubModels = count;

	SubmodelsData = cm.cmodels;
	SubmodelsDataCount = cm.numSubModels;

	if ( count > MAX_SUBMODELS ) {
		Com_Error( ERR_DROP, "MAX_SUBMODELS exceeded" );
	}

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out = &cm.cmodels[i];

		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
		}

		//rwwRMG - sof2 doesn't have to add this &cm == &cmg check.
		//Are they getting leaf data elsewhere? (the reason this needs to be done is
		//in sub bsp instances the first brush model isn't necessary a world model and might be
		//real architecture)
		if ( i == 0 && &cm == &cmg ) {
			out->firstNode = 0;
			continue;	// world model doesn't need other info
		}

		// make a "leaf" just to hold the model's brushes and surfaces
		out->firstNode = -1;

		// make a "leaf" just to hold the model's brushes and surfaces
		out->leaf.numLeafBrushes = LittleLong( in->numBrushes );
		indexes = (int *)Hunk_Alloc( out->leaf.numLeafBrushes * 4, h_high );
		out->leaf.firstLeafBrush = indexes - cm.leafbrushes;
		for ( j = 0 ; j < out->leaf.numLeafBrushes ; j++ ) {
			indexes[j] = LittleLong( in->firstBrush ) + j;
		}

		out->leaf.numLeafSurfaces = LittleLong( in->numSurfaces );
		indexes = (int *)Hunk_Alloc( out->leaf.numLeafSurfaces * 4, h_high );
		out->leaf.firstLeafSurface = indexes - cm.leafsurfaces;
		for ( j = 0 ; j < out->leaf.numLeafSurfaces ; j++ ) {
			indexes[j] = LittleLong( in->firstSurface ) + j;
		}
	}
}


/*
=================
CMod_LoadNodes

=================
*/
void CMod_LoadNodes( lump_t *l, clipMap_t &cm, bool subBSP ) {
	dnode_t		*in;
	int			child;
	cNode_t		*out;
	int			i, j, count;

	in = (dnode_t *)(cmod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");

	count = l->filelen / sizeof(*in);

	if (!subBSP && count < 1)
		Com_Error (ERR_DROP, "Map has no nodes");

	int originalNumNodes = cm.numNodes;
	
	if (subBSP)
	{
		cNode_t *oldNodes = cm.nodes;

		cm.nodes = (cNode_t *)Hunk_Alloc((count + originalNumNodes) * sizeof(*cm.nodes), h_high);
		cm.numNodes = originalNumNodes + count;

		for (int i = 0; i < originalNumNodes; i++)
		{
			cNode_t *oldn = oldNodes + i;
			cNode_t *newn = cm.nodes + i;

			newn->plane = oldn->plane;

			for (j = 0; j<2; j++)
			{
				Com_Printf("ORIGINAL: node %i. child %i is %i.\n", i, j, oldn->children[j]);
				newn->children[j] = oldn->children[j];
			}
		}

		Z_Free(oldNodes);

		NodesData = cm.nodes;
		NodesDataCount = cm.numNodes;

		out = cm.nodes + originalNumNodes;
	}
	else
	{
		cm.nodes = (cNode_t *)Hunk_Alloc(count * sizeof(*cm.nodes), h_high);
		cm.numNodes = count;

		NodesData = cm.nodes;
		NodesDataCount = cm.numNodes;

		out = cm.nodes;
	}

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->plane = cm.planes + LittleLong( in->planeNum );

		for (j=0 ; j<2 ; j++)
		{
			child = LittleLong (in->children[j]);

			if (subBSP && in->children[j] < 0) child -= originalNumNodes;
			if (subBSP && in->children[j] >= 0) child += originalNumNodes;
			out->children[j] = child;

			if (subBSP) Com_Printf("NEW: node %i. child %i is %i.\n", i + originalNumNodes, j, out->children[j]);
		}
	}
}

/*
=================
CM_BoundBrush

=================
*/
void CM_BoundBrush( cbrush_t *b ) {
	b->bounds[0][0] = -b->sides[0].plane->dist;
	b->bounds[1][0] = b->sides[1].plane->dist;

	b->bounds[0][1] = -b->sides[2].plane->dist;
	b->bounds[1][1] = b->sides[3].plane->dist;

	b->bounds[0][2] = -b->sides[4].plane->dist;
	b->bounds[1][2] = b->sides[5].plane->dist;
}


/*
=================
CMod_LoadBrushes

=================
*/
void CMod_LoadBrushes( lump_t *l, clipMap_t	&cm ) {
	dbrush_t	*in;
	cbrush_t	*out;
	int			i, count;

	in = (dbrush_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in)) {
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	}
	count = l->filelen / sizeof(*in);

	cm.brushes = (cbrush_t *)Hunk_Alloc( ( BOX_BRUSHES + count ) * sizeof( *cm.brushes ), h_high );
	cm.numBrushes = count;

	BrushesData = cm.brushes;
	BrushesDataCount = cm.numBrushes;

	out = cm.brushes;

	for ( i=0 ; i<count ; i++, out++, in++ ) {
		out->sides = cm.brushsides + LittleLong(in->firstSide);
		out->numsides = LittleLong(in->numSides);

		out->shaderNum = LittleLong( in->shaderNum );
		if ( out->shaderNum < 0 || out->shaderNum >= cm.numShaders ) {
			Com_Error( ERR_DROP, "CMod_LoadBrushes: bad shaderNum: %i", out->shaderNum );
		}
		out->contents = cm.shaders[out->shaderNum].contentFlags;

		CM_BoundBrush( out );
	}

}

/*
=================
CMod_LoadLeafs
=================
*/
void CMod_LoadLeafs (lump_t *l, clipMap_t &cm)
{
	int			i;
	cLeaf_t		*out;
	dleaf_t 	*in;
	int			count;

	in = (dleaf_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error (ERR_DROP, "Map with no leafs");

	cm.leafs = (cLeaf_t *)Hunk_Alloc( ( BOX_LEAFS + count ) * sizeof( *cm.leafs ), h_high );
	cm.numLeafs = count;

	LeafsData = cm.leafs;
	LeafsDataCount = cm.numLeafs;

	out = cm.leafs;
	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->cluster = LittleLong (in->cluster);
		out->area = LittleLong (in->area);
		out->firstLeafBrush = LittleLong (in->firstLeafBrush);
		out->numLeafBrushes = LittleLong (in->numLeafBrushes);
		out->firstLeafSurface = LittleLong (in->firstLeafSurface);
		out->numLeafSurfaces = LittleLong (in->numLeafSurfaces);

		if (out->cluster >= cm.numClusters)
			cm.numClusters = out->cluster + 1;
		if (out->area >= cm.numAreas)
			cm.numAreas = out->area + 1;
	}

	cm.areas = (cArea_t *)Hunk_Alloc( cm.numAreas * sizeof( *cm.areas ), h_high );
	cm.areaPortals = (int *)Hunk_Alloc( cm.numAreas * cm.numAreas * sizeof( *cm.areaPortals ), h_high );
}

/*
=================
CMod_LoadPlanes
=================
*/
void CMod_LoadPlanes (lump_t *l, clipMap_t &cm, bool subBSP)
{
	int			i, j;
	cplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;

	in = (dplane_t *)(cmod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");

	count = l->filelen / sizeof(*in);

	if (!subBSP && count < 1)
		Com_Error (ERR_DROP, "Map with no planes");

	if (subBSP)
	{
		int originalNumPlanes = cm.numPlanes;

		cplane_t *oldPlanes = cm.planes;

		cm.planes = (struct cplane_s *)Hunk_Alloc((BOX_PLANES + count + originalNumPlanes) * sizeof(*cm.planes), h_high);
		cm.numPlanes = originalNumPlanes + count;

		for (int i = 0; i < originalNumPlanes; i++)
		{
			cplane_t *oldp = oldPlanes + i;
			cplane_t *newp = cm.planes + i;
			
			for (j = 0; j<3; j++)
			{
				newp->normal[j] = oldp->normal[j];
			}

			newp->dist = oldp->dist;
			newp->type = oldp->type;
			newp->signbits = oldp->signbits;
		}

		Z_Free(oldPlanes);

		PlanesData = cm.planes;
		PlanesDataCount = cm.numPlanes;

		out = cm.planes + originalNumPlanes;
	}
	else
	{
		cm.numPlanes = count;
/*#ifdef __BSP_USE_SHARED_MEMORY__
		hSharedMemory *sharedMemoryPointer = OpenSharedMemory("BSPSurfacePlanes", "BSPSurfacePlanesMutex", (BOX_PLANES + count) * sizeof(*cm.planes));
		if (sharedMemoryPointer)
		{
			cm.planes = (struct cplane_s *)sharedMemoryPointer->hFileView;
			cm.planesAreSharedMemory = qtrue;
		}
		else
		{
			cm.planes = (struct cplane_s *)Hunk_Alloc((BOX_PLANES + count) * sizeof(*cm.planes), h_high);
			cm.planesAreSharedMemory = qfalse;
		}
#else //!__BSP_USE_SHARED_MEMORY__*/
		cm.planes = (struct cplane_s *)Hunk_Alloc((BOX_PLANES + count) * sizeof(*cm.planes), h_high);
//#endif //__BSP_USE_SHARED_MEMORY__

		PlanesData = cm.planes;
		PlanesDataCount = cm.numPlanes;

		out = cm.planes;
	}

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		bits = 0;
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat (in->dist);
		out->type = PlaneTypeForNormal( out->normal );
		out->signbits = bits;
	}
}

/*
=================
CMod_LoadLeafBrushes
=================
*/
void CMod_LoadLeafBrushes (lump_t *l, clipMap_t	&cm)
{
	int			i;
	int			*out;
	int		 	*in;
	int			count;

	in = (int *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	cm.leafbrushes = (int *)Hunk_Alloc( (count + BOX_BRUSHES) * sizeof( *cm.leafbrushes ), h_high );
	cm.numLeafBrushes = count;

	LeafBrushesData = cm.leafbrushes;
	LeafBrushesDataCount = cm.numLeafBrushes;

	out = cm.leafbrushes;

	for ( i=0 ; i<count ; i++, in++, out++) {
		*out = LittleLong (*in);
	}
}

/*
=================
CMod_LoadLeafSurfaces
=================
*/
void CMod_LoadLeafSurfaces( lump_t *l, clipMap_t &cm )
{
	int			i;
	int			*out;
	int		 	*in;
	int			count;

	in = (int *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	cm.leafsurfaces = (int *)Hunk_Alloc( count * sizeof( *cm.leafsurfaces ), h_high );
	cm.numLeafSurfaces = count;

	LeafSurfacesData = cm.leafsurfaces;
	LeafSurfacesDataCount = cm.numLeafSurfaces;

	out = cm.leafsurfaces;

	for ( i=0 ; i<count ; i++, in++, out++) {
		*out = LittleLong (*in);
	}
}

/*
=================
CMod_LoadBrushSides
=================
*/
void CMod_LoadBrushSides (lump_t *l, clipMap_t &cm)
{
	int				i;
	cbrushside_t	*out;
	dbrushside_t 	*in;
	int				count;
	int				num;

	in = (dbrushside_t *)(cmod_base + l->fileofs);
	if ( l->filelen % sizeof(*in) ) {
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	}
	count = l->filelen / sizeof(*in);

	cm.brushsides = (cbrushside_t *)Hunk_Alloc( ( BOX_SIDES + count ) * sizeof( *cm.brushsides ), h_high );
	cm.numBrushSides = count;

	BrushSidesData = cm.brushsides;
	BrushSidesDataCount = cm.numBrushSides;

	out = cm.brushsides;

	for ( i=0 ; i<count ; i++, in++, out++) {
		num = LittleLong( in->planeNum );
		out->plane = &cm.planes[num];
		out->shaderNum = LittleLong( in->shaderNum );
		if ( out->shaderNum < 0 || out->shaderNum >= cm.numShaders ) {
			Com_Error( ERR_DROP, "CMod_LoadBrushSides: bad shaderNum: %i", out->shaderNum );
		}
	}
}


/*
=================
CMod_LoadEntityString
=================
*/
void CMod_LoadEntityString( lump_t *l, clipMap_t &cm ) {
	cm.entityString = (char *)Hunk_Alloc( l->filelen, h_high );
	cm.numEntityChars = l->filelen;
	Com_Memcpy (cm.entityString, cmod_base + l->fileofs, l->filelen);

	EntityStringData = cm.entityString;
	EntityStringDataCount = cm.numEntityChars;
}

/*
=================
CMod_LoadVisibility
=================
*/
#define	VIS_HEADER	8
void CMod_LoadVisibility( lump_t *l, clipMap_t &cm ) {
	int		len;
	byte	*buf;

    len = l->filelen;
	if ( !len ) {
		cm.clusterBytes = ( cm.numClusters + 31 ) & ~31;
		cm.visibility = (unsigned char *)Hunk_Alloc( cm.clusterBytes, h_high );
		Com_Memset( cm.visibility, 255, cm.clusterBytes );
		return;
	}
	buf = cmod_base + l->fileofs;

	cm.vised = qtrue;
	cm.visibility = (unsigned char *)Hunk_Alloc( len, h_high );
	cm.numClusters = LittleLong( ((int *)buf)[0] );
	cm.clusterBytes = LittleLong( ((int *)buf)[1] );
	Com_Memcpy (cm.visibility, buf + VIS_HEADER, len - VIS_HEADER );

	VisibilityData = cm.visibility;
	VisibilityDataClusterCount = cm.numClusters;
	VisibilityDataClusterBytesCount = cm.clusterBytes;
}

//==================================================================


/*
=================
CMod_LoadPatches
=================
*/
#define	MAX_PATCH_VERTS		1024
void CMod_LoadPatches( lump_t *surfs, lump_t *verts, clipMap_t &cm ) {
	drawVert_t	*dv, *dv_p;
	dsurface_t	*in;
	int			count;
	int			i, j;
	int			c;
	cPatch_t	*patch;
	vec3_t		points[MAX_PATCH_VERTS];
	int			width, height;
	int			shaderNum;

	in = (dsurface_t *)(cmod_base + surfs->fileofs);
	if (surfs->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	cm.numSurfaces = count = surfs->filelen / sizeof(*in);
	cm.surfaces = (cPatch_t ** )Hunk_Alloc( cm.numSurfaces * sizeof( cm.surfaces[0] ), h_high );

	PatchesData = cm.surfaces;
	PatchesDataCount = cm.numSurfaces;

	dv = (drawVert_t *)(cmod_base + verts->fileofs);
	if (verts->filelen % sizeof(*dv))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");

	// scan through all the surfaces, but only load patches,
	// not planar faces
	for ( i = 0 ; i < count ; i++, in++ ) {
		if ( LittleLong( in->surfaceType ) != MST_PATCH ) {
			continue;		// ignore other surfaces
		}
		// FIXME: check for non-colliding patches

		cm.surfaces[ i ] = patch = (cPatch_t *)Hunk_Alloc( sizeof( *patch ), h_high );

		// load the full drawverts onto the stack
		width = LittleLong( in->patchWidth );
		height = LittleLong( in->patchHeight );
		c = width * height;
		if ( c > MAX_PATCH_VERTS ) {
			Com_Error( ERR_DROP, "ParseMesh: MAX_PATCH_VERTS" );
		}

		dv_p = dv + LittleLong( in->firstVert );
		for ( j = 0 ; j < c ; j++, dv_p++ ) {
			points[j][0] = LittleFloat( dv_p->xyz[0] );
			points[j][1] = LittleFloat( dv_p->xyz[1] );
			points[j][2] = LittleFloat( dv_p->xyz[2] );
		}

		shaderNum = LittleLong( in->shaderNum );
		patch->contents = cm.shaders[shaderNum].contentFlags;
		patch->surfaceFlags = cm.shaders[shaderNum].surfaceFlags;
		patch->materialType = cm.shaders[shaderNum].materialType;

		if (!HaveSurfaceType(patch->materialType))
			patch->surfaceFlags = LittleLong(GetMaterialType(cm.shaders[shaderNum].GetName(), patch->materialType));

		// create the internal facet structure
		patch->pc = CM_GeneratePatchCollide( width, height, points );
	}
}

//==================================================================

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
void *gpvCachedMapDiskImage = NULL;
char  gsCachedMapDiskImage[MAX_QPATH];
qboolean gbUsingCachedMapDataRightNow = qfalse;	// if true, signifies that you can't delete this at the moment!! (used during z_malloc()-fail recovery attempt)

// called in response to a "devmapbsp blah" or "devmapall blah" command, do NOT use inside CM_Load unless you pass in qtrue
//
// new bool return used to see if anything was freed, used during z_malloc failure re-try
//
qboolean CM_DeleteCachedMap(qboolean bGuaranteedOkToDelete)
{
	qboolean bActuallyFreedSomething = qfalse;

	if (bGuaranteedOkToDelete || !gbUsingCachedMapDataRightNow)
	{
		// dump cached disk image...
		//
		if (gpvCachedMapDiskImage)
		{
			Z_Free(	gpvCachedMapDiskImage );
					gpvCachedMapDiskImage = NULL;

			bActuallyFreedSomething = qtrue;
		}
		gsCachedMapDiskImage[0] = '\0';

		// force map loader to ignore cached internal BSP structures for next level CM_LoadMap() call...
		//
		cmg.name[0] = '\0';
	}

	return bActuallyFreedSomething;
}


extern void CM_LoadRoadImage(const char *mapName);
extern void CM_LoadHeightMapImage(const char *mapName);
extern void CM_SetTerrainContents(clipMap_t &cm);

static void CM_LoadMap_Actual( const char *name, qboolean clientload, int *checksum, clipMap_t &cm )
{ //rwwRMG - function needs heavy modification
	int				*buf;
	dheader_t		header;
	static unsigned	last_checksum;
	char			origName[MAX_OSPATH];
	void			*newBuff = 0;

	if ( !name || !name[0] ) {
		Com_Error( ERR_DROP, "CM_LoadMap: NULL name" );
	}

#ifndef BSPC
	cm_noAreas = Cvar_Get ("cm_noAreas", "0", CVAR_CHEAT);
	cm_noCurves = Cvar_Get ("cm_noCurves", "0", CVAR_CHEAT);
	cm_playerCurveClip = Cvar_Get ("cm_playerCurveClip", "1", CVAR_ARCHIVE|CVAR_CHEAT );
	cm_extraVerbose = Cvar_Get ("cm_extraVerbose", "0", CVAR_TEMP );
#endif

	Com_Printf( "CM_LoadMap( %s, %i )\n", name, clientload );

	if ( !strcmp( cm.name, name ) && clientload ) {
		if ( checksum )
			*checksum = last_checksum;
		return;
	}

	strcpy(origName, name);

	if (&cm == &cmg)
	{
		// free old stuff
		CM_ClearMap();
		CM_ClearLevelPatches();
	}

	// free old stuff
	Com_Memset( &cm, 0, sizeof( cm ) );

	if ( !name[0] ) {
		cm.numLeafs = 1;
		cm.numClusters = 1;
		cm.numAreas = 1;
		cm.cmodels = (struct cmodel_s *)Hunk_Alloc( sizeof( *cm.cmodels ), h_high );
		if ( checksum )
			*checksum = 0;
		return;
	}

	//
	// load the file
	//
	//rww - Doesn't this sort of defeat the purpose? We're clearing it even if the map is the same as the last one!
	//Not touching it though in case I'm just overlooking something.
	if (gpvCachedMapDiskImage && &cm == &cmg)	// MP code: this'll only be NZ if we got an ERR_DROP during last map load,
	{							//	so it's really just a safety measure.
		Z_Free(	gpvCachedMapDiskImage);
				gpvCachedMapDiskImage = NULL;
	}

#ifdef __BSP_USE_SHARED_MEMORY__
	bool failedSharedMemory = false;

	buf = NULL;
	fileHandle_t h;
	hSharedMemory *shared = NULL;
	const int iBSPLen = FS_FOpenFileRead(name, &h, qfalse);

	if (h)
	{
		shared = OpenSharedMemory("CMSharedBSP", "CMSharedBSPMutex", iBSPLen);

		if (shared)
		{
			//Com_Printf("Shared memory allocated OK! (%u).\n", iBSPLen);

			newBuff = (void *)shared->hFileView;

			FS_Read(newBuff, iBSPLen, h);
			FS_FCloseFile(h);

			buf = (int*)newBuff;	// so the rest of the code works as normal

			/*
			if (&cm == &cmg)
			{
				gpvCachedMapDiskImage = newBuff;
				newBuff = 0;
			}
			*/
			gpvCachedMapDiskImage = NULL;

			// carry on as before...
			//
		}
		else
		{
			newBuff = Z_Malloc(iBSPLen, TAG_BSP_DISKIMAGE);

			FS_Read(newBuff, iBSPLen, h);
			FS_FCloseFile(h);

			buf = (int*)newBuff;	// so the rest of the code works as normal

									/*
									if (&cm == &cmg)
									{
									gpvCachedMapDiskImage = newBuff;
									newBuff = 0;
									}
									*/
			gpvCachedMapDiskImage = NULL;

			failedSharedMemory = true;
			// carry on as before...
			//
		}
	}
#elif !defined(BSPC)
	//
	// load the file into a buffer that we either discard as usual at the bottom, or if we've got enough memory
	//	then keep it long enough to save the renderer re-loading it (if not dedicated server),
	//	then discard it after that...
	//
	buf = NULL;
	fileHandle_t h;
	const int iBSPLen = FS_FOpenFileRead( name, &h, qfalse );
	if (h)
	{
		newBuff = Z_Malloc( iBSPLen, TAG_BSP_DISKIMAGE );
		FS_Read( newBuff, iBSPLen, h);
		FS_FCloseFile( h );

		buf = (int*) newBuff;	// so the rest of the code works as normal
		if (&cm == &cmg)
		{
			gpvCachedMapDiskImage = newBuff;
			newBuff = 0;
		}

		// carry on as before...
		//
	}
#else
	const int iBSPLen = LoadQuakeFile((quakefile_t *) name, (void **)&buf);
#endif

	if ( !buf ) {
		Com_Error (ERR_DROP, "Couldn't load %s", name);
	}


	last_checksum = LittleLong (Com_BlockChecksum (buf, iBSPLen));
	if ( checksum )
		*checksum = last_checksum;

	header = *(dheader_t *)buf;
	for (size_t i=0 ; i<sizeof(dheader_t)/4 ; i++) {
		((int *)&header)[i] = LittleLong ( ((int *)&header)[i]);
	}

	if ( header.version != BSP_VERSION ) {
		Z_Free(	gpvCachedMapDiskImage);
				gpvCachedMapDiskImage = NULL;

		Com_Error (ERR_DROP, "CM_LoadMap: %s has wrong version number (%i should be %i)"
		, name, header.version, BSP_VERSION );
	}

	cmod_base = (byte *)buf;

	// load into heap
	CMod_LoadShaders( &header.lumps[LUMP_SHADERS], cm );
	CMod_LoadLeafs (&header.lumps[LUMP_LEAFS], cm);
	CMod_LoadLeafBrushes (&header.lumps[LUMP_LEAFBRUSHES], cm);
	CMod_LoadLeafSurfaces (&header.lumps[LUMP_LEAFSURFACES], cm);
	CMod_LoadPlanes (&header.lumps[LUMP_PLANES], cm, false);
	CMod_LoadBrushSides (&header.lumps[LUMP_BRUSHSIDES], cm);
	CMod_LoadBrushes (&header.lumps[LUMP_BRUSHES], cm);
	CMod_LoadSubmodels (&header.lumps[LUMP_MODELS], cm);
	CMod_LoadNodes (&header.lumps[LUMP_NODES], cm, false);
	CMod_LoadEntityString (&header.lumps[LUMP_ENTITIES], cm);
	CMod_LoadVisibility( &header.lumps[LUMP_VISIBILITY], cm );
	CMod_LoadPatches( &header.lumps[LUMP_SURFACES], &header.lumps[LUMP_DRAWVERTS], cm );

	TotalSubModels += cm.numSubModels;

	if (&cm == &cmg)
	{
		// Load in the shader text - return instantly if already loaded
		CM_InitBoxHull ();
	}

#define __FREE_BSP_DATA__

#ifndef __FREE_BSP_DATA__
#ifndef BSPC	// I hope we can lose this crap soon
	//
	// if we've got enough memory, and it's not a dedicated-server, then keep the loaded map binary around
	//	for the renderer to chew on... (but not if this gets ported to a big-endian machine, because some of the
	//	map data will have been Little-Long'd, but some hasn't).
	//
	if (Sys_LowPhysicalMemory()
		|| com_dedicated->integer
//		|| we're on a big-endian machine
		)
	{
		Z_Free(	gpvCachedMapDiskImage );
				gpvCachedMapDiskImage = NULL;
	}
	else
	{
		// ... do nothing, and let the renderer free it after it's finished playing with it...
		//
	}
#else
	FS_FreeFile (buf);
#endif
#else //__FREE_BSP_DATA__

#ifdef __BSP_USE_SHARED_MEMORY__
	if (!failedSharedMemory)
	{
		CloseSharedMemory(shared);
		buf = NULL;
		gpvCachedMapDiskImage = NULL;
	}
	else
	{
		if (buf == gpvCachedMapDiskImage)
		{
			Z_Free(buf);
			buf = NULL;
			gpvCachedMapDiskImage = NULL;
		}
		else
		{
			Z_Free(gpvCachedMapDiskImage);
			gpvCachedMapDiskImage = NULL;
			Z_Free(buf);
			buf = NULL;
		}
	}
#else //!__BSP_USE_SHARED_MEMORY__
	if (buf == gpvCachedMapDiskImage)
	{
		Z_Free(buf);
		buf = NULL;
		gpvCachedMapDiskImage = NULL;
	}
	else
	{
		Z_Free(gpvCachedMapDiskImage);
		gpvCachedMapDiskImage = NULL;
		Z_Free(buf);
		buf = NULL;
	}

	Com_Printf("CM_LoadMap: BSP memory image (%.2f MB) was freed.\n", float(iBSPLen) / 1024.0 / 1024.0);
#endif //__BSP_USE_SHARED_MEMORY__
#endif //__FREE_BSP_DATA__

#if 0
	//
	// Load extra bsp's planes, if it has any...
	//
	{
		char strippedName[512] = { 0 };
		char loadName[512] = { 0 };
		COM_StripExtension(name, strippedName, strlen(name));
		sprintf(loadName, "%s_nonsolid.bsp", strippedName);

		Com_Printf("CM_LoadMap( %s, %i )\n", loadName, clientload);

#ifndef BSPC
		//
		// load the file into a buffer that we either discard as usual at the bottom, or if we've got enough memory
		//	then keep it long enough to save the renderer re-loading it (if not dedicated server),
		//	then discard it after that...
		//
		buf = NULL;
		fileHandle_t h;
		const int iBSPLen = FS_FOpenFileRead(loadName, &h, qfalse);
		if (h)
		{
			newBuff = Z_Malloc(iBSPLen, TAG_BSP_DISKIMAGE);
			FS_Read(newBuff, iBSPLen, h);
			FS_FCloseFile(h);

			buf = (int*)newBuff;	// so the rest of the code works as normal
			if (&cm == &cmg)
			{
				gpvCachedMapDiskImage = newBuff;
				newBuff = 0;
			}

			// carry on as before...
			//
		}
#else
		const int iBSPLen = LoadQuakeFile((quakefile_t *)loadName, (void **)&buf);
#endif

		if (buf) 
		{
			last_checksum = LittleLong(Com_BlockChecksum(buf, iBSPLen));
			if (checksum)
				*checksum = last_checksum;

			header = *(dheader_t *)buf;
			for (size_t i = 0; i < sizeof(dheader_t) / 4; i++) {
				((int *)&header)[i] = LittleLong(((int *)&header)[i]);
			}

			if (header.version != BSP_VERSION) {
				Z_Free(gpvCachedMapDiskImage);
				gpvCachedMapDiskImage = NULL;

				Com_Error(ERR_DROP, "CM_LoadMap: %s has wrong version number (%i should be %i)"
					, name, header.version, BSP_VERSION);
			}

			cmod_base = (byte *)buf;

			// load into heap
			CMod_LoadPlanes(&header.lumps[LUMP_PLANES], cm, true);
			CMod_LoadNodes(&header.lumps[LUMP_NODES], cm, true);

#ifndef __FREE_BSP_DATA__
#ifndef BSPC	// I hope we can lose this crap soon
			//
			// if we've got enough memory, and it's not a dedicated-server, then keep the loaded map binary around
			//	for the renderer to chew on... (but not if this gets ported to a big-endian machine, because some of the
			//	map data will have been Little-Long'd, but some hasn't).
			//
			if (Sys_LowPhysicalMemory()
				|| com_dedicated->integer
				//		|| we're on a big-endian machine
				)
			{
				Z_Free(gpvCachedMapDiskImage);
				gpvCachedMapDiskImage = NULL;
			}
			else
			{
				// ... do nothing, and let the renderer free it after it's finished playing with it...
				//
			}
#else
			FS_FreeFile(buf);
#endif
#else //__FREE_BSP_DATA__
			Z_Free(gpvCachedMapDiskImage);
			gpvCachedMapDiskImage = NULL;
#endif //__FREE_BSP_DATA__
		}
	}
#endif

	CM_FloodAreaConnections (cm);

	// allow this to be cached if it is loaded by the server
	if ( !clientload ) {
		Q_strncpyz( cm.name, origName, sizeof( cm.name ) );
	}

	CM_LoadRoadImage(name);
	CM_SetTerrainContents(cm);
}


// need a wrapper function around this because of multiple returns, need to ensure bool is correct...
//
void CM_LoadMap( const char *name, qboolean clientload, int *checksum )
{
	gbUsingCachedMapDataRightNow = qtrue;	// !!!!!!!!!!!!!!!!!!

		CM_LoadMap_Actual( name, clientload, checksum, cmg );

	gbUsingCachedMapDataRightNow = qfalse;	// !!!!!!!!!!!!!!!!!!
}



/*
==================
CM_ClearMap
==================
*/
void CM_ClearMap( void )
{
	int		i;

	Com_Memset( &cmg, 0, sizeof( cmg ) );
	CM_ClearLevelPatches();

	for(i = 0; i < NumSubBSP; i++)
	{
		memset(&SubBSP[i], 0, sizeof(SubBSP[0]));
	}
	NumSubBSP = 0;
	TotalSubModels = 0;
}

/*
==================
CM_ClipHandleToModel
==================
*/
cmodel_t	*CM_ClipHandleToModel( clipHandle_t handle, clipMap_t **clipMap ) {
	int		i;
	int		count;

	if ( handle < 0 )
	{
		Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i", handle );
	}
	if ( handle < cmg.numSubModels )
	{
		if (clipMap)
		{
			*clipMap = &cmg;
		}
		return &cmg.cmodels[handle];
	}
	if ( handle == BOX_MODEL_HANDLE )
	{
		if (clipMap)
		{
			*clipMap = &cmg;
		}
		return &box_model;
	}

	count = cmg.numSubModels;
	for(i = 0; i < NumSubBSP; i++)
	{
		if (handle < count + SubBSP[i].numSubModels)
		{
			if (clipMap)
			{
				*clipMap = &SubBSP[i];
			}
			return &SubBSP[i].cmodels[handle - count];
		}
		count += SubBSP[i].numSubModels;
	}

	if ( handle < MAX_SUBMODELS )
	{
		Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i < %i < %i",
			cmg.numSubModels, handle, MAX_SUBMODELS );
	}
	Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i", handle + MAX_SUBMODELS );

	return NULL;
}

/*
==================
CM_InlineModel
==================
*/
clipHandle_t	CM_InlineModel( int index ) {
	if ( index < 0 || index >= TotalSubModels ) {
		Com_Error( ERR_DROP, "CM_InlineModel: bad number: %d >= %d (may need to re-BSP map?)", index, TotalSubModels );
	}
	return index;
}

int		CM_NumInlineModels( void ) {
	return cmg.numSubModels;
}

char	*CM_EntityString( void ) {
	return cmg.entityString;
}

char *CM_SubBSPEntityString( int index )
{
	return SubBSP[index].entityString;
}

int		CM_LeafCluster( int leafnum ) {
	if (leafnum < 0 || leafnum >= cmg.numLeafs) {
		Com_Error (ERR_DROP, "CM_LeafCluster: bad number");
	}
	return cmg.leafs[leafnum].cluster;
}

int		CM_LeafArea( int leafnum ) {
	if ( leafnum < 0 || leafnum >= cmg.numLeafs ) {
		Com_Error (ERR_DROP, "CM_LeafArea: bad number");
	}
	return cmg.leafs[leafnum].area;
}

//=======================================================================


/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull (void)
{
	int			i;
	int			side;
	cplane_t	*p;
	cbrushside_t	*s;

	box_planes = &cmg.planes[cmg.numPlanes];

	box_brush = &cmg.brushes[cmg.numBrushes];
	box_brush->numsides = 6;
	box_brush->sides = cmg.brushsides + cmg.numBrushSides;
	box_brush->contents = CONTENTS_BODY;

	box_model.firstNode = -1;
	box_model.leaf.numLeafBrushes = 1;
//	box_model.leaf.firstLeafBrush = cmg.numBrushes;
	box_model.leaf.firstLeafBrush = cmg.numLeafBrushes;
	cmg.leafbrushes[cmg.numLeafBrushes] = cmg.numBrushes;

	for (i=0 ; i<6 ; i++)
	{
		side = i&1;

		// brush sides
		s = &cmg.brushsides[cmg.numBrushSides+i];
		s->plane = 	cmg.planes + (cmg.numPlanes+i*2+side);
		s->shaderNum = cmg.numShaders;

		// planes
		p = &box_planes[i*2];
		p->type = i>>1;
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = 1;

		p = &box_planes[i*2+1];
		p->type = 3 + (i>>1);
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = -1;

		SetPlaneSignbits( p );
	}
}

/*
===================
CM_TempBoxModel

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
Capsules are handled differently though.
===================
*/
clipHandle_t CM_TempBoxModel( const vec3_t mins, const vec3_t maxs, int capsule ) {

	VectorCopy( mins, box_model.mins );
	VectorCopy( maxs, box_model.maxs );

	if ( capsule ) {
		return CAPSULE_MODEL_HANDLE;
	}

	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];

	VectorCopy( mins, box_brush->bounds[0] );
	VectorCopy( maxs, box_brush->bounds[1] );

	return BOX_MODEL_HANDLE;
}

/*
===================
CM_ModelBounds
===================
*/
void CM_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	cmodel_t	*cmod;

	cmod = CM_ClipHandleToModel( model );
	VectorCopy( cmod->mins, mins );
	VectorCopy( cmod->maxs, maxs );
}

int CM_LoadSubBSP(const char *name, qboolean clientload)
{
	int		i;
	int		checksum;
	int		count;

	count = cmg.numSubModels;
	for(i = 0; i < NumSubBSP; i++)
	{
		if (!Q_stricmp(name, SubBSP[i].name))
		{
			return count;
		}
		count += SubBSP[i].numSubModels;
	}

	if (NumSubBSP == MAX_SUB_BSP)
	{
		Com_Error (ERR_DROP, "CM_LoadSubBSP: too many unique sub BSPs");
	}

	CM_LoadMap_Actual(name, clientload, &checksum, SubBSP[NumSubBSP] );
	NumSubBSP++;

	return count;
}

int CM_FindSubBSP(int modelIndex)
{
	int		i;
	int		count;

	count = cmg.numSubModels;
	if (modelIndex < count)
	{	// belongs to the main bsp
		return -1;
	}

	for(i = 0; i < NumSubBSP; i++)
	{
		count += SubBSP[i].numSubModels;
		if (modelIndex < count)
		{
			return i;
		}
	}
	return -1;
}

void CM_GetWorldBounds ( vec3_t mins, vec3_t maxs )
{
	VectorCopy ( cmg.cmodels[0].mins, mins );
	VectorCopy ( cmg.cmodels[0].maxs, maxs );
}

int CM_ModelContents_Actual( clipHandle_t model, clipMap_t *cm )
{
	cmodel_t	*cmod;
	int			contents = 0;
	int			i;

	if (!cm)
	{
		cm = &cmg;
	}

	cmod = CM_ClipHandleToModel( model, &cm );

	//MCG ADDED - return the contents, too

	for ( i = 0; i < cmod->leaf.numLeafBrushes; i++ )
	{
		int brushNum = cm->leafbrushes[cmod->leaf.firstLeafBrush + i];
		contents |= cm->brushes[brushNum].contents;
	}

	for ( i = 0; i < cmod->leaf.numLeafSurfaces; i++ )
	{
		int surfaceNum = cm->leafsurfaces[cmod->leaf.firstLeafSurface + i];
		if ( cm->surfaces[surfaceNum] != NULL )
		{//HERNH?  How could we have a null surf within our cmod->leaf.numLeafSurfaces?
			contents |= cm->surfaces[surfaceNum]->contents;
		}
	}
	
	return contents;
}

int CM_ModelContents(  clipHandle_t model, int subBSPIndex )
{
	if (subBSPIndex < 0)
	{
		return CM_ModelContents_Actual(model, NULL);
	}

	return CM_ModelContents_Actual(model, &SubBSP[subBSPIndex]);
}
