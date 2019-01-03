//[VisualWeapons]

//header file for holstered weapons data
#ifndef _H_HOLSTER_
#define _H_HOLSTER_

typedef struct holster_s holster_t;
struct holster_s
{
	qhandle_t	boneIndex;			//bolt index to base this weapon off of.
	vec3_t		posOffset;			//the positional offset of the weapon
	vec3_t		angOffset;			//the angular offset of the weapon
};

//enum for the types of holstered weapons you can have
enum
{
	HLR_NONE,
	HLR_SINGLESABER_1,	//first single saber
	HLR_SINGLESABER_2,	//second single saber
	HLR_STAFFSABER,		//staff saber
	HLR_PISTOL_L,		//left hip blaster pistol
	HLR_PISTOL_R,		//right hip blaster pistol
	HLR_BLASTER_L,		//left hip blaster rifle
	HLR_BLASTER_R,		//right hip blaster rifle
	HLR_BRYARPISTOL_L,	//left hip bryer pistol
	HLR_BRYARPISTOL_R,	//right hip bryer pistol
	HLR_BOWCASTER,		//bowcaster
	HLR_ROCKET_LAUNCHER,//rocket launcher
	HLR_DEMP2,			//demp2
	HLR_CONCUSSION,		//concussion
	HLR_REPEATER,		//repeater
	HLR_FLECHETTE,		//flechette
	HLR_DISRUPTOR,		//disruptor
	// add new holster under here
	HLR_T21,
	HLR_A280,
	HLR_EE3,
	HLR_DTL20A,
	HLR_Z6_CANON,
	HRL_CLONERIFLE,
	HRL_DC15_EXT,
	HLR_WOOKIE_BOWCASTER,
	HLR_DC15,
	HLR_WESTARM5,
	HLR_CLONE_BLASTER_L,
	HLR_CLONE_BLASTER_R,
	HLR_DC_15S_CLONE_PISTOL_L,		
	HLR_DC_15S_CLONE_PISTOL_R,		
	HLR_WESTER_PISTOL_L,		
	HLR_WESTER_PISTOL_R,		
	HLR_ELG_3A_L,		
	HLR_ELG_3A_R,		
	HLR_S5_PISTOL_L,		
	HLR_S5_PISTOL_R,		
	HLR_WOOKIES_PISTOL_L,		
	HLR_WOOKIES_PISTOL_R,		
	HLR_DC_17_CLONE_PISTOL_L,		
	HLR_DC_17_CLONE_PISTOL_R,		
	HLR_TESTGUN_L,
	HLR_TESTGUN_R,
	HLR_SPOTING_BLASTER_L,
	HLR_SPOTING_BLASTER_R,
	HLR_ACP_PISTOL_L,
	HLR_ACP_PISTOL_R,
	HRL_E60_ROCKET_LAUNCHER,
	HRL_CW_ROCKET_LAUNCHER,
	HLR_FRAG_GRENADE,
	HLR_FRAG_GRENADE_OLD,
	HLR_THERMAL_GRENADE,
	HLR_TRIP_MINE,
	HLR_DET_PACK,
	HLR_A200_ACP_BATTLERIFLE,
	HLR_ACP_ARRAYGUN,
	HLR_ACP_SNIPER_RIFLE,
	HLR_ARC_CASTER_IMPERIAL_L,
	HLR_ARC_CASTER_IMPERIAL_R,
	HLR_BOWCASTER_CLASSIC,
	HLR_WOOKIE_BOWCASTER_SCOPE,
	HLR_BRYAR_CARBINE_L,
	HLR_BRYAR_CARBINE_R,
	HLR_BRYAR_RIFLE,
	HLR_BRYAR_RIFLE_SCOPE,
	HRL_PULSECANON,
	HRL_PROTON_CARBINE_RIFLE,
	HLR_DH_17_PISTOL_L,
	HLR_DH_17_PISTOL_R,

	MAX_HOLSTER		//max possible holster weapon positions
};

enum
{
	HOLSTER_NONE,
	HOLSTER_UPPERBACK,	
	HOLSTER_LOWERBACK,
	HOLSTER_LEFTHIP,
	HOLSTER_RIGHTHIP,
};

//max char size of individual holster.cfg files
#define		MAX_HOLSTER_INFO_SIZE					2*8192
//max char size for the individual holster.cfg holster type data
#define		MAX_HOLSTER_GROUP_SIZE					MAX_HOLSTER_INFO_SIZE/3

#endif
//[/VisualWeapons]

