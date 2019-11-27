// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_misc.c -- both games misc functions, all completely stateless

#include "qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_inventory.h"

#if defined(_GAME)
	#include "g_local.h"
#elif defined(_CGAME)
	#include "cgame/cg_local.h"
#elif defined(_UI)
	#include "ui/ui_local.h"
#endif

extern qboolean BG_HaveWeapon ( const playerState_t *ps, int weapon );

char *factionNames[FACTION_NUM_FACTIONS] = {
	"Free",
	"Empire", // red
	"Rebels", // blue
	"Mandalorians", // orange
	"Mercenaries", // green
	"Pirates", // grey
	"Wildlife", // yellow
	"Spectator",
};

const char *bgToggleableSurfaces[BG_NUM_TOGGLEABLE_SURFACES] =
{
	"l_arm_key",					//0
	"torso_canister1",
	"torso_canister2",
	"torso_canister3",
	"torso_tube1",
	"torso_tube2",					//5
	"torso_tube3",
	"torso_tube4",
	"torso_tube5",
	"torso_tube6",
	"r_arm",						//10
	"l_arm",
	"torso_shield",
	"torso_galaktorso",
	"torso_collar",
//	"torso_eyes_mouth",				//15
//	"torso_galakhead",
//	"torso_galakface",
//	"torso_antenna_base_cap",
//	"torso_antenna",
//	"l_arm_augment",				//20
//	"l_arm_middle",
//	"l_arm_wrist",
//	"r_arm_middle", //yeah.. galak's surf stuff is no longer auto, sorry! need the space for vehicle surfs.
	"r_wing1",						//15
	"r_wing2",
	"l_wing1",
	"l_wing2",
	"r_gear",
	"l_gear",						//20
	"nose",
	"blah4",
	"blah5",
	"l_hand",
	"r_hand",						//25
	"helmet",
	"head",
	"head_concussion_charger",
	"head_light_blaster_cann",		//29
	"force_shield",					//30
	NULL
};

const int bgToggleableSurfaceDebris[BG_NUM_TOGGLEABLE_SURFACES] =
{
	0,					//0
	0,
	0,
	0,
	0,
	0,					//5
	0,
	0,
	0,
	0,
	0,					//10
	0,
	0,
	0,
	0, //>= 2 means it should create a flame trail when destroyed (for vehicles)
	3,					//15
	5, //rwing2
	4,
	6, //lwing2
	0, //rgear
	0, //lgear			//20
	7, //nose
	0, //blah
	0, //blah
	0,
	0,					//25
	0,
	0,
	0,
	0,					//29
	-1
};

const char	*bg_customSiegeSoundNames[MAX_CUSTOM_SIEGE_SOUNDS] =
{
	"*att_attack",
	"*att_primary",
	"*att_second",
	"*def_guns",
	"*def_position",
	"*def_primary",
	"*def_second",
	"*reply_coming",
	"*reply_go",
	"*reply_no",
	"*reply_stay",
	"*reply_yes",
	"*req_assist",
	"*req_demo",
	"*req_hvy",
	"*req_medic",
	"*req_sup",
	"*req_tech",
	"*spot_air",
	"*spot_defenses",
	"*spot_emplaced",
	"*spot_sniper",
	"*spot_troops",
	"*tac_cover",
	"*tac_fallback",
	"*tac_follow",
	"*tac_hold",
	"*tac_split",
	"*tac_together",
	NULL
};

//rww - not putting @ in front of these because
//we don't need them in a cgame StringEd lookup.
//Let me know if this causes problems, pat.
char *forceMasteryLevels[NUM_FORCE_MASTERY_LEVELS] =
{
	"MASTERY0",	//"Uninitiated",	// FORCE_MASTERY_UNINITIATED,
	"MASTERY1",	//"Initiate",		// FORCE_MASTERY_INITIATE,
	"MASTERY2",	//"Padawan",		// FORCE_MASTERY_PADAWAN,
	"MASTERY3",	//"Jedi",			// FORCE_MASTERY_JEDI,
	"MASTERY4",	//"Jedi Adept",		// FORCE_MASTERY_JEDI_GUARDIAN,
	"MASTERY5",	//"Jedi Guardian",	// FORCE_MASTERY_JEDI_ADEPT,
	"MASTERY6",	//"Jedi Knight",	// FORCE_MASTERY_JEDI_KNIGHT,
	"MASTERY7",	//"Jedi Master"		// FORCE_MASTERY_JEDI_MASTER,
};

int forceMasteryPoints[NUM_FORCE_MASTERY_LEVELS] =
{
	0,		// FORCE_MASTERY_UNINITIATED,
	5,		// FORCE_MASTERY_INITIATE,
	10,		// FORCE_MASTERY_PADAWAN,
	20,		// FORCE_MASTERY_JEDI,
	30,		// FORCE_MASTERY_JEDI_GUARDIAN,
	50,		// FORCE_MASTERY_JEDI_ADEPT,
	75,		// FORCE_MASTERY_JEDI_KNIGHT,
	100		// FORCE_MASTERY_JEDI_MASTER,
};

int bgForcePowerCost[NUM_FORCE_POWERS][NUM_FORCE_POWER_LEVELS] = //0 == neutral
{
	{	0,	2,	4,	6	},	// Heal			// FP_HEAL
	{	0,	0,	2,	6	},	// Jump			//FP_LEVITATION,//hold/duration
	{	0,	2,	4,	6	},	// Speed		//FP_SPEED,//duration
	{	0,	1,	3,	6	},	// Push			//FP_PUSH,//hold/duration
	{	0,	1,	3,	6	},	// Pull			//FP_PULL,//hold/duration
	{	0,	4,	6,	8	},	// Mind Trick	//FP_TELEPATHY,//instant
	{	0,	1,	3,	6	},	// Grip			//FP_GRIP,//hold/duration
	{	0,	2,	5,	8	},	// Lightning	//FP_LIGHTNING,//hold/duration
	{	0,	4,	6,	8	},	// Dark Rage	//FP_RAGE,//duration
	{	0,	2,	5,	8	},	// Protection	//FP_PROTECT,//duration
	{	0,	1,	3,	6	},	// Absorb		//FP_ABSORB,//duration
	{	0,	1,	3,	6	},	// Team Heal	//FP_TEAM_HEAL,//instant
	{	0,	1,	3,	6	},	// Team Force	//FP_TEAM_FORCE,//instant
	{	0,	2,	4,	6	},	// Drain		//FP_DRAIN,//hold/duration
	{	0,	2,	5,	8	},	// Sight		//FP_SEE,//duration
	{	0,	1,	5,	8	},	// Saber Attack	//FP_SABER_OFFENSE,
	{	0,	1,	5,	8	},	// Saber Defend	//FP_SABER_DEFENSE,
	{	0,	4,	6,	8	}	// Saber Throw	//FP_SABERTHROW,
	//NUM_FORCE_POWERS
};

int forcePowerSorted[NUM_FORCE_POWERS] =
{ //rww - always use this order when drawing force powers for any reason
	FP_TELEPATHY,
	FP_HEAL,
	FP_ABSORB,
	FP_PROTECT,
	FP_TEAM_HEAL,
	FP_LEVITATION,
	FP_SPEED,
	FP_PUSH,
	FP_PULL,
	FP_SEE,
	FP_LIGHTNING,
	FP_DRAIN,
	FP_RAGE,
	FP_GRIP,
	FP_TEAM_FORCE,
	FP_SABER_OFFENSE,
	FP_SABER_DEFENSE,
	FP_SABERTHROW
};

int forcePowerDarkLight[NUM_FORCE_POWERS] = //0 == neutral
{ //nothing should be usable at rank 0..
	FORCE_LIGHTSIDE,//FP_HEAL,//instant
	0,//FP_LEVITATION,//hold/duration
	0,//FP_SPEED,//duration
	0,//FP_PUSH,//hold/duration
	0,//FP_PULL,//hold/duration
	FORCE_LIGHTSIDE,//FP_TELEPATHY,//instant
	FORCE_DARKSIDE,//FP_GRIP,//hold/duration
	FORCE_DARKSIDE,//FP_LIGHTNING,//hold/duration
	FORCE_DARKSIDE,//FP_RAGE,//duration
	FORCE_LIGHTSIDE,//FP_PROTECT,//duration
	FORCE_LIGHTSIDE,//FP_ABSORB,//duration
	FORCE_LIGHTSIDE,//FP_TEAM_HEAL,//instant
	FORCE_DARKSIDE,//FP_TEAM_FORCE,//instant
	FORCE_DARKSIDE,//FP_DRAIN,//hold/duration
	0,//FP_SEE,//duration
	0,//FP_SABER_OFFENSE,
	0,//FP_SABER_DEFENSE,
	0//FP_SABERTHROW,
		//NUM_FORCE_POWERS
};

int WeaponReadyAnim[WP_NUM_WEAPONS] =
{
	TORSO_DROPWEAP1,//WP_NONE,

	BOTH_STAND1,//WP_EMPLACED_GUN,
	TORSO_WEAPONREADY1,//WP_TURRET,

	TORSO_WEAPONREADY10,//WP_THERMAL,
	TORSO_WEAPONREADY10,//WP_FRAG_GRENADE,
	TORSO_WEAPONREADY10,//WP_CYROBAN_GRENADE,
	TORSO_WEAPONREADY10,//TORSO_WEAPONREADY11,//WP_TRIP_MINE,
	TORSO_WEAPONREADY10,//TORSO_WEAPONREADY12,//WP_DET_PACK,

	TORSO_WEAPONREADY3,//WP_MELEE,

	BOTH_STAND2,//WP_SABER,
	TORSO_WEAPONREADY3,//WP_MODULIZED_WEAPON,
};

int WeaponReadyLegsAnim[WP_NUM_WEAPONS] =
{
	BOTH_STAND1,//WP_NONE,

	BOTH_STAND1,//WP_EMPLACED_GUN,
	BOTH_STAND1,//WP_TURRET,

	BOTH_STAND1,//WP_THERMAL,
	BOTH_STAND1,//WP_FRAG_GRENADE,
	BOTH_STAND1,//TORSO_WEAPONREADY11,//WP_TRIP_MINE,
	BOTH_STAND1,//TORSO_WEAPONREADY12,//WP_DET_PACK,

	BOTH_STAND1,//WP_MELEE,

	BOTH_STAND2,//WP_SABER,
	BOTH_STAND1,//WP_MODULIZED_WEAPON,
};

int WeaponAttackAnim[WP_NUM_WEAPONS] =
{
	BOTH_ATTACK1,//WP_NONE, //(shouldn't happen)

	BOTH_STAND1,//WP_EMPLACED_GUN,
	BOTH_ATTACK1,//WP_TURRET,

	BOTH_THERMAL_THROW,//WP_THERMAL,
	BOTH_THERMAL_THROW,//WP_FRAG_GRENADE,
	BOTH_THERMAL_THROW,//WP_CYROBAN_GRENADE,
	BOTH_ATTACK3,//BOTH_ATTACK11,//WP_TRIP_MINE,
	BOTH_ATTACK3,//BOTH_ATTACK12,//WP_DET_PACK,

	BOTH_ATTACK3,//WP_MELEE,

	BOTH_STAND2,//WP_SABER, //(has its own handling)
	DC15_FIRE,// WP_MODULIZED_WEAPON,
};

qboolean BG_FileExists(const char *fileName)
{
	if (fileName && fileName[0])
	{
#if 0
		int fh = 0;
	#ifdef _GAME
		trap->FS_Open(fileName, &fh, FS_READ);
	#elif _CGAME
		trap->FS_Open(fileName, &fh, FS_READ);
	#elif _UI
		trap->FS_Open(fileName, &fh, FS_READ);
	#endif
		if (fh > 0)
		{
		#ifdef _GAME
			trap->FS_Close(fh);
		#elif _CGAME
			trap->FS_Close(fh);
		#elif _UI
			trap->FS_Close(fh);
		#endif
			return qtrue;
		}
#else
		return trap->FS_FileExists(fileName);
#endif
	}

	return qfalse;
}

// given a boltmatrix, return in vec a normalised vector for the axis requested in flags
void BG_GiveMeVectorFromMatrix(mdxaBone_t *boltMatrix, int flags, vec3_t vec)
{
	switch (flags)
	{
	case ORIGIN:
		vec[0] = boltMatrix->matrix[0][3];
		vec[1] = boltMatrix->matrix[1][3];
		vec[2] = boltMatrix->matrix[2][3];
		break;
	case POSITIVE_Y:
		vec[0] = boltMatrix->matrix[0][1];
		vec[1] = boltMatrix->matrix[1][1];
		vec[2] = boltMatrix->matrix[2][1];
 		break;
	case POSITIVE_X:
		vec[0] = boltMatrix->matrix[0][0];
		vec[1] = boltMatrix->matrix[1][0];
		vec[2] = boltMatrix->matrix[2][0];
		break;
	case POSITIVE_Z:
		vec[0] = boltMatrix->matrix[0][2];
		vec[1] = boltMatrix->matrix[1][2];
		vec[2] = boltMatrix->matrix[2][2];
		break;
	case NEGATIVE_Y:
		vec[0] = -boltMatrix->matrix[0][1];
		vec[1] = -boltMatrix->matrix[1][1];
		vec[2] = -boltMatrix->matrix[2][1];
		break;
	case NEGATIVE_X:
		vec[0] = -boltMatrix->matrix[0][0];
		vec[1] = -boltMatrix->matrix[1][0];
		vec[2] = -boltMatrix->matrix[2][0];
		break;
	case NEGATIVE_Z:
		vec[0] = -boltMatrix->matrix[0][2];
		vec[1] = -boltMatrix->matrix[1][2];
		vec[2] = -boltMatrix->matrix[2][2];
		break;
	}
}

/*
================
BG_LegalizedForcePowers

The magical function to end all functions.
This will take the force power string in powerOut and parse through it, then legalize
it based on the supposed rank and spit it into powerOut, returning true if it was legal
to begin with and false if not.
fpDisabled is actually only expected (needed) from the server, because the ui disables
force power selection anyway when force powers are disabled on the server.
================
*/
qboolean BG_LegalizedForcePowers(char *powerOut, size_t powerOutSize, int maxRank, qboolean freeSaber, int teamForce, int gametype, int fpDisabled)
{
	char powerBuf[128];
	char readBuf[128];
	qboolean maintainsValidity = qtrue;
	int powerLen = strlen(powerOut);
	int i = 0;
	int c = 0;
	int allowedPoints = 0;
	int usedPoints = 0;
	int countDown = 0;

	int final_Side;
	int final_Powers[NUM_FORCE_POWERS] = {0};

	if ( powerLen >= 128 )
	{ //This should not happen. If it does, this is obviously a bogus string.
		//They can have this string. Because I said so.
		Q_strncpyz( powerBuf, DEFAULT_FORCEPOWERS, sizeof( powerBuf ) );
		maintainsValidity = qfalse;
	}
	else
		Q_strncpyz( powerBuf, powerOut, sizeof( powerBuf ) ); //copy it as the original

	//first of all, print the max rank into the string as the rank
	Q_strncpyz( powerOut, va( "%i-", maxRank ), powerOutSize );

	while (i < sizeof( powerBuf ) && powerBuf[i] && powerBuf[i] != '-')
	{
		i++;
	}
	i++;
	while (i < sizeof( powerBuf ) && powerBuf[i] && powerBuf[i] != '-')
	{
		readBuf[c] = powerBuf[i];
		c++;
		i++;
	}
	readBuf[c] = 0;
	i++;
	//at this point, readBuf contains the intended side
	final_Side = atoi(readBuf);

	if (final_Side != FORCE_LIGHTSIDE &&
		final_Side != FORCE_DARKSIDE)
	{ //Not a valid side. You will be dark. Because I said so. (this is something that should never actually happen unless you purposely feed in an invalid config)
		final_Side = FORCE_DARKSIDE;
		maintainsValidity = qfalse;
	}

	if (teamForce)
	{ //If we are under force-aligned teams, make sure we're on the right side.
		if (final_Side != teamForce)
		{
			final_Side = teamForce;
			//maintainsValidity = qfalse;
			//Not doing this, for now. Let them join the team with their filtered powers.
		}
	}

	//Now we have established a valid rank, and a valid side.
	//Read the force powers in, and cut them down based on the various rules supplied.
	c = 0;
	while (i < sizeof( powerBuf ) && powerBuf[i] && powerBuf[i] != '\n' && powerBuf[i] != '\r'
		&& powerBuf[i] >= '0' && powerBuf[i] <= '3' && c < NUM_FORCE_POWERS)
	{
		readBuf[0] = powerBuf[i];
		readBuf[1] = 0;
		final_Powers[c] = atoi(readBuf);
		c++;
		i++;
	}

	//final_Powers now contains all the stuff from the string
	//Set the maximum allowed points used based on the max rank level, and count the points actually used.
	allowedPoints = forceMasteryPoints[maxRank];

	i = 0;
	while (i < NUM_FORCE_POWERS)
	{ //if this power doesn't match the side we're on, then 0 it now.
		if (final_Powers[i] &&
			forcePowerDarkLight[i] &&
			forcePowerDarkLight[i] != final_Side)
		{
			final_Powers[i] = 0;
			//This is only likely to happen with g_forceBasedTeams. Let it slide.
		}

		if ( final_Powers[i] &&
			(fpDisabled & (1 << i)) )
		{ //if this power is disabled on the server via said server option, then we don't get it.
			final_Powers[i] = 0;
		}

		i++;
	}

	if (gametype < GT_TEAM)
	{ //don't bother with team powers then
		final_Powers[FP_TEAM_HEAL] = 0;
		final_Powers[FP_TEAM_FORCE] = 0;
	}

	usedPoints = 0;
	i = 0;
	while (i < NUM_FORCE_POWERS)
	{
		countDown = Com_Clampi( 0, NUM_FORCE_POWER_LEVELS, final_Powers[i] );

		while (countDown > 0)
		{
			usedPoints += bgForcePowerCost[i][countDown]; //[fp index][fp level]
			//if this is jump, or we have a free saber and it's offense or defense, take the level back down on level 1
			if ( countDown == 1 &&
				((i == FP_LEVITATION) ||
				 (i == FP_SABER_OFFENSE && freeSaber) ||
				 (i == FP_SABER_DEFENSE && freeSaber)) )
			{
				usedPoints -= bgForcePowerCost[i][countDown];
			}
			countDown--;
		}

		i++;
	}

	if (usedPoints > allowedPoints)
	{ //Time to do the fancy stuff. (meaning, slowly cut parts off while taking a guess at what is most or least important in the config)
		int attemptedCycles = 0;
		int powerCycle = 2;
		int minPow = 0;

		if (freeSaber)
		{
			minPow = 1;
		}

		maintainsValidity = qfalse;

		while (usedPoints > allowedPoints)
		{
			c = 0;

			while (c < NUM_FORCE_POWERS && usedPoints > allowedPoints)
			{
				if (final_Powers[c] && final_Powers[c] < powerCycle)
				{ //kill in order of lowest powers, because the higher powers are probably more important
					if (c == FP_SABER_OFFENSE &&
						(final_Powers[FP_SABER_DEFENSE] > minPow || final_Powers[FP_SABERTHROW] > 0))
					{ //if we're on saber attack, only suck it down if we have no def or throw either
						int whichOne = FP_SABERTHROW; //first try throw

						if (!final_Powers[whichOne])
						{
							whichOne = FP_SABER_DEFENSE; //if no throw, drain defense
						}

						while (final_Powers[whichOne] > 0 && usedPoints > allowedPoints)
						{
							if ( final_Powers[whichOne] > 1 ||
								( (whichOne != FP_SABER_OFFENSE || !freeSaber) &&
								  (whichOne != FP_SABER_DEFENSE || !freeSaber) ) )
							{ //don't take attack or defend down on level 1 still, if it's free
								usedPoints -= bgForcePowerCost[whichOne][final_Powers[whichOne]];
								final_Powers[whichOne]--;
							}
							else
							{
								break;
							}
						}
					}
					else
					{
						while (final_Powers[c] > 0 && usedPoints > allowedPoints)
						{
							if ( final_Powers[c] > 1 ||
								((c != FP_LEVITATION) &&
								(c != FP_SABER_OFFENSE || !freeSaber) &&
								(c != FP_SABER_DEFENSE || !freeSaber)) )
							{
								usedPoints -= bgForcePowerCost[c][final_Powers[c]];
								final_Powers[c]--;
							}
							else
							{
								break;
							}
						}
					}
				}

				c++;
			}

			powerCycle++;
			attemptedCycles++;

			if (attemptedCycles > NUM_FORCE_POWERS)
			{ //I think this should be impossible. But just in case.
				break;
			}
		}

		if (usedPoints > allowedPoints)
		{ //Still? Fine then.. we will kill all of your powers, except the freebies.
			i = 0;

			while (i < NUM_FORCE_POWERS)
			{
				final_Powers[i] = 0;
				if (i == FP_LEVITATION ||
					(i == FP_SABER_OFFENSE && freeSaber) ||
					(i == FP_SABER_DEFENSE && freeSaber))
				{
					final_Powers[i] = 1;
				}
				i++;
			}
			usedPoints = 0;
		}
	}

	if (freeSaber)
	{
		if (final_Powers[FP_SABER_OFFENSE] < 1)
			final_Powers[FP_SABER_OFFENSE] = 1;
		if (final_Powers[FP_SABER_DEFENSE] < 1)
			final_Powers[FP_SABER_DEFENSE] = 1;
	}
	if (final_Powers[FP_LEVITATION] < 1)
		final_Powers[FP_LEVITATION] = 1;

	i = 0;
	while (i < NUM_FORCE_POWERS)
	{
		if (final_Powers[i] > FORCE_LEVEL_3)
			final_Powers[i] = FORCE_LEVEL_3;
		i++;
	}

	if (fpDisabled)
	{ //If we specifically have attack or def disabled, force them up to level 3. It's the way
	  //things work for the case of all powers disabled.
	  //If jump is disabled, down-cap it to level 1. Otherwise don't do a thing.
		if (fpDisabled & (1 << FP_LEVITATION))
			final_Powers[FP_LEVITATION] = 1;
		if (fpDisabled & (1 << FP_SABER_OFFENSE))
			final_Powers[FP_SABER_OFFENSE] = 3;
		if (fpDisabled & (1 << FP_SABER_DEFENSE))
			final_Powers[FP_SABER_DEFENSE] = 3;
	}

	if (final_Powers[FP_SABER_OFFENSE] < 1)
	{
		final_Powers[FP_SABER_DEFENSE] = 0;
		final_Powers[FP_SABERTHROW] = 0;
	}

	//We finally have all the force powers legalized and stored locally.
	//Put them all into the string and return the result. We already have
	//the rank there, so print the side and the powers now.
	Q_strcat(powerOut, powerOutSize, va("%i-", final_Side));

	i = strlen(powerOut);
	c = 0;
	while (c < NUM_FORCE_POWERS)
	{
		Q_strncpyz(readBuf, va( "%i", final_Powers[c] ), sizeof( readBuf ) );
		powerOut[i] = readBuf[0];
		c++;
		i++;
	}
	powerOut[i] = 0;

	return maintainsValidity;
}



float vectoyaw( const vec3_t vec ) {
	float	yaw;

	if (vec[YAW] == 0 && vec[PITCH] == 0) {
		yaw = 0;
	} else {
		if (vec[PITCH]) {
			yaw = ( atan2( vec[YAW], vec[PITCH]) * 180 / M_PI );
		} else if (vec[YAW] > 0) {
			yaw = 90;
		} else {
			yaw = 270;
		}
		if (yaw < 0) {
			yaw += 360;
		}
	}

	return yaw;
}

qboolean BG_HasYsalamiri(int gametype, playerState_t *ps)
{
	if (gametype == GT_CTY &&
		(ps->powerups[PW_REDFLAG] || ps->powerups[PW_BLUEFLAG]))
	{
		return qtrue;
	}

	if (ps->powerups[PW_YSALAMIRI])
	{
		return qtrue;
	}

	return qfalse;
}

qboolean BG_CanUseFPNow(int gametype, playerState_t *ps, int time, forcePowers_t power)
{
	if (BG_HasYsalamiri(gametype, ps))
	{
		return qfalse;
	}

	if ( ps->forceRestricted || ps->trueNonJedi )
	{
		return qfalse;
	}

	if (ps->weapon == WP_EMPLACED_GUN)
	{ //can't use any of your powers while on an emplaced weapon
		return qfalse;
	}

	if (ps->m_iVehicleNum)
	{ //can't use powers while riding a vehicle (this may change, I don't know)
		return qfalse;
	}

	if (ps->duelInProgress)
	{
		if (power != FP_SABER_OFFENSE && power != FP_SABER_DEFENSE && /*power != FP_SABERTHROW &&*/
			power != FP_LEVITATION)
		{
			if (!ps->saberLockFrame || power != FP_PUSH)
			{
				return qfalse;
			}
		}
	}

	if (ps->saberLockFrame || ps->saberLockTime > time)
	{
		if (power != FP_PUSH)
		{
			return qfalse;
		}
	}

	if (ps->fallingToDeath)
	{
		return qfalse;
	}

	if ((ps->brokenLimbs & (1 << BROKENLIMB_RARM)) ||
		(ps->brokenLimbs & (1 << BROKENLIMB_LARM)))
	{ //powers we can't use with a broken arm
        switch (power)
		{
		case FP_PUSH:
		case FP_PULL:
		case FP_GRIP:
		case FP_LIGHTNING:
		case FP_DRAIN:
			return qfalse;
		default:
			break;
		}
	}

	return qtrue;
}

/*
==============
BG_FindItemForPowerup
==============
*/
gitem_t	*BG_FindItemForPowerup( powerup_t pw ) {
	int		i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( (bg_itemlist[i].giType == IT_POWERUP ||
					bg_itemlist[i].giType == IT_TEAM) &&
			bg_itemlist[i].giTag == pw ) {
			return &bg_itemlist[i];
		}
	}

	return NULL;
}


/*
==============
BG_FindItemForHoldable
==============
*/
gitem_t	*BG_FindItemForHoldable( holdable_t pw ) {
	int		i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == pw ) {
			return &bg_itemlist[i];
		}
	}

	Com_Error( ERR_DROP, "HoldableItem not found" );

	return NULL;
}


/*
===============
BG_FindItemForWeapon

===============
*/
gitem_t	*BG_FindItemForWeapon( weapon_t weapon ) {
	gitem_t	*it;

	for ( it = bg_itemlist + 1 ; it->classname ; it++) {
		if ( it->giType == IT_WEAPON && it->giTag == weapon ) {
			return it;
		}
	}

	Com_Error( ERR_DROP, "Couldn't find item for weapon %i", weapon);
	return NULL;
}

/*
===============
BG_FindItemForAmmo

===============
*/
gitem_t	*BG_FindItemForAmmo( ammo_t ammo ) {
#ifndef __MMO__
	gitem_t	*it;

	for ( it = bg_itemlist + 1 ; it->classname ; it++) {
		if ( it->giType == IT_AMMO && it->giTag == ammo ) {
			return it;
		}
	}

	Com_Error( ERR_DROP, "Couldn't find item for ammo %i", ammo);
#endif //__MMO__
	return NULL;
}

/*
===============
BG_FindItem

===============
*/
gitem_t	*BG_FindItem( const char *classname ) {
	gitem_t	*it;

	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( !Q_stricmp( it->classname, classname) )
			return it;
	}

	return NULL;
}

/*
============
BG_PlayerTouchesItem

Items can be picked up without actually touching their physical bounds to make
grabbing them easier
============
*/
qboolean	BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime ) {
	vec3_t		origin;

	BG_EvaluateTrajectory( &item->pos, atTime, origin );

	// we are ignoring ducked differences here
	if ( ps->origin[0] - origin[0] > 44
		|| ps->origin[0] - origin[0] < -50
		|| ps->origin[1] - origin[1] > 36
		|| ps->origin[1] - origin[1] < -36
		|| ps->origin[2] - origin[2] > 36
		|| ps->origin[2] - origin[2] < -36 ) {
		return qfalse;
	}

	return qtrue;
}

int BG_ProperForceIndex( int power ) {
	int i;

	for ( i=0; i<NUM_FORCE_POWERS; i++ ) {
		if ( forcePowerSorted[i] == power )
			return i;
	}

	return -1;
}

void BG_CycleForce( playerState_t *ps, int direction ) {
	int i, x, presel;
	int foundnext = -1;

	presel = x = i = ps->fd.forcePowerSelected;

	// no valid force powers
	if ( x >= NUM_FORCE_POWERS || x == -1 )
		return;

	presel = x = BG_ProperForceIndex( x );

	// get the next/prev power and handle overflow
	if ( direction == 1 )	x++;
	else					x--;
	if ( x >= NUM_FORCE_POWERS )	x = 0;
	if ( x < 0 )					x = NUM_FORCE_POWERS-1;

	i = forcePowerSorted[x]; //the "sorted" value of this power

	while ( x != presel ) {
		// loop around to the current force power
		if ( ps->fd.forcePowersKnown & (1 << i) && i != (signed)ps->fd.forcePowerSelected ) {
			// we have this power
			if ( i != FP_LEVITATION && i != FP_SABER_OFFENSE && i != FP_SABER_DEFENSE && i != FP_SABERTHROW ) {
				// it's selectable
				foundnext = i;
				break;
			}
		}

		// get the next/prev power and handle overflow
		if ( direction == 1 )	x++;
		else					x--;
		if ( x >= NUM_FORCE_POWERS )	x = 0;
		if ( x < 0 )					x = NUM_FORCE_POWERS-1;

		i = forcePowerSorted[x]; //set to the sorted value again
	}

	// if we found one, select it
	if ( foundnext != -1 )
		ps->fd.forcePowerSelected = foundnext;
}

int BG_GetItemIndexByTag(int tag, int type)
{ //Get the itemlist index from the tag and type
	int i = 0;

	while (i < bg_numItems)
	{
		if (bg_itemlist[i].giTag == tag &&
			bg_itemlist[i].giType == type)
		{
			return i;
		}

		i++;
	}

	return 0;
}

//yeah..
qboolean BG_IsItemSelectable(playerState_t *ps, int item)
{
	if (item == HI_HEALTHDISP || item == HI_AMMODISP ||
		item == HI_JETPACK)
	{
		return qfalse;
	}
	return qtrue;
}

void BG_CycleInven(playerState_t *ps, int direction)
{
	int i;
	int dontFreeze = 0;
	int original;

	i = bg_itemlist[ps->stats[STAT_HOLDABLE_ITEM]].giTag;
	original = i;

	if (direction == 1)
	{ //next
		i++;
		if (i == HI_NUM_HOLDABLE)
		{
			i = 1;
		}
	}
	else
	{ //previous
		i--;
		if (i == 0)
		{
			i = HI_NUM_HOLDABLE-1;
		}
	}

	while (i != original)
	{ //go in a full loop until hitting something, if hit nothing then select nothing
		if (ps->stats[STAT_HOLDABLE_ITEMS] & (1 << i))
		{ //we have it, select it.
			if (BG_IsItemSelectable(ps, i))
			{
				ps->stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(i, IT_HOLDABLE);
				break;
			}
		}

		if (direction == 1)
		{ //next
			i++;
		}
		else
		{ //previous
			i--;
		}

		if (i <= 0)
		{ //wrap around to the last
			i = HI_NUM_HOLDABLE-1;
		}
		else if (i >= HI_NUM_HOLDABLE)
		{ //wrap around to the first
			i = 1;
		}

		dontFreeze++;
		if (dontFreeze >= 32)
		{ //yeah, sure, whatever (it's 2 am and I'm paranoid and can't frickin think)
			break;
		}
	}
}

/*
================
BG_CanItemBeGrabbed

Returns false if the item should not be picked up.
This needs to be the same for client side prediction and server use.
================
*/
qboolean BG_CanItemBeGrabbed( int gametype, const entityState_t *ent, const playerState_t *ps ) {
	gitem_t	*item;

	if ( ent->modelindex < 1 || ent->modelindex >= bg_numItems ) {
		Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: index out of range" );
	}

	item = &bg_itemlist[ent->modelindex];

	if ( ps )
	{
		if ( ps->trueJedi )
		{//force powers and saber only
			if ( item->giType != IT_TEAM //not a flag
				&& item->giType != IT_ARMOR//not shields
				&& (item->giType != IT_WEAPON
									|| item->giTag != WP_SABER)//not a saber
				&& (item->giType != IT_HOLDABLE || item->giTag != HI_SEEKER)//not a seeker
				&& (item->giType != IT_POWERUP || item->giTag == PW_YSALAMIRI) )//not a force pick-up
			{
				return qfalse;
			}
		}
		else if ( ps->trueNonJedi )
		{//can't pick up force powerups
			if ( (item->giType == IT_POWERUP && item->giTag != PW_YSALAMIRI) //if a powerup, can only can pick up ysalamiri
				|| (item->giType == IT_HOLDABLE && item->giTag == HI_SEEKER)//if holdable, cannot pick up seeker
				|| (item->giType == IT_WEAPON && item->giTag == WP_SABER ) )//or if it's a saber
			{
				return qfalse;
			}
		}
		if ( ps->isJediMaster && item && (item->giType == IT_WEAPON || item->giType == IT_AMMO))
		{//jedi master cannot pick up weapons
			return qfalse;
		}
		if ( ps->duelInProgress )
		{ //no picking stuff up while in a duel, no matter what the type is
			return qfalse;
		}
	}
	else
	{//safety return since below code assumes a non-null ps
		return qfalse;
	}

	switch( item->giType ) {
	case IT_WEAPON:
		if (ent->generic1 == ps->clientNum && ent->powerups)
		{
			return qfalse;
		}
		
		if (!(ent->eFlags & EF_DROPPEDWEAPON) && BG_HaveWeapon(ps, item->giTag) &&
			item->giTag != WP_THERMAL 
			&& item->giTag != WP_FRAG_GRENADE 
			&& item->giTag != WP_CYROBAN_GRENADE
			&& item->giTag != WP_TRIP_MINE 
			&& item->giTag != WP_DET_PACK)
		{ //weaponstay stuff.. if this isn't dropped, and you already have it, you don't get it.
			return qfalse;
		}
		return qtrue;	// weapons are always picked up

	case IT_AMMO:
		return qfalse;

	case IT_ARMOR:
		if ( ps->stats[STAT_ARMOR] >= ps->stats[STAT_MAX_HEALTH]/* * item->giTag*/ ) {
			return qfalse;
		}
		return qtrue;

	case IT_HEALTH:
		// small and mega healths will go over the max, otherwise
		// don't pick up if already at max
		if ((ps->fd.forcePowersActive & (1 << FP_RAGE)))
		{
			return qfalse;
		}

		if ( item->quantity == 5 || item->quantity == 100 ) {
			if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] * 2 ) {
				return qfalse;
			}
			return qtrue;
		}

		if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] ) {
			return qfalse;
		}
		return qtrue;

	case IT_POWERUP:
		if (ps && (ps->powerups[PW_YSALAMIRI]))
		{
			if (item->giTag != PW_YSALAMIRI)
			{
				return qfalse;
			}
		}
		return qtrue;	// powerups are always picked up

	case IT_TEAM: // team items, such as flags
		if( gametype == GT_CTF || gametype == GT_CTY ) {
			// ent->modelindex2 is non-zero on items if they are dropped
			// we need to know this because we can pick up our dropped flag (and return it)
			// but we can't pick up our flag at base
			if (ps->persistant[PERS_TEAM] == FACTION_EMPIRE) {
				if (item->giTag == PW_BLUEFLAG ||
					(item->giTag == PW_REDFLAG && ent->modelindex2) ||
					(item->giTag == PW_REDFLAG && ps->powerups[PW_BLUEFLAG]) )
					return qtrue;
			} else if (ps->persistant[PERS_TEAM] == FACTION_REBEL) {
				if (item->giTag == PW_REDFLAG ||
					(item->giTag == PW_BLUEFLAG && ent->modelindex2) ||
					(item->giTag == PW_BLUEFLAG && ps->powerups[PW_REDFLAG]) )
					return qtrue;
			}
		}

		return qfalse;

	case IT_HOLDABLE:
		if ( ps->stats[STAT_HOLDABLE_ITEMS] & (1 << item->giTag))
		{
			return qfalse;
		}
		return qtrue;

        case IT_BAD:
            Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD" );
        default:
#ifndef NDEBUG // bk0001204
          Com_Printf("BG_CanItemBeGrabbed: unknown enum %d\n", item->giType );
#endif
         break;
	}

	return qfalse;
}

//======================================================================

/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result ) {
	float		deltaTime;
	float		phase;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorCopy( tr->trBase, result );
		break;
	case TR_LINEAR:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = sin( deltaTime * M_PI * 2 );
		VectorMA( tr->trBase, phase, tr->trDelta, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		if ( deltaTime < 0 ) {
			deltaTime = 0;
		}
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_NONLINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration )
		{
			atTime = tr->trTime + tr->trDuration;
		}
		//new slow-down at end
		if ( atTime - tr->trTime > tr->trDuration || atTime - tr->trTime <= 0  )
		{
			deltaTime = 0;
		}
		else
		{//FIXME: maybe scale this somehow?  So that it starts out faster and stops faster?
			deltaTime = tr->trDuration*0.001f*((float)cos( DEG2RAD(90.0f - (90.0f*((float)(atTime-tr->trTime))/(float)tr->trDuration)) ));
		}
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;		// FIXME: local gravity...
		break;
	default:
#ifdef _GAME
		Com_Error( ERR_DROP, "BG_EvaluateTrajectory: [ GAME] unknown trType: %i", tr->trType );
#else
		Com_Error( ERR_DROP, "BG_EvaluateTrajectory: [CGAME] unknown trType: %i", tr->trType );
#endif
		break;
	}
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result ) {
	float	deltaTime;
	float	phase;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorClear( result );
		break;
	case TR_LINEAR:
		VectorCopy( tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = cos( deltaTime * M_PI * 2 );	// derivative of sin = cos
		phase *= 0.5;
		VectorScale( tr->trDelta, phase, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		VectorCopy( tr->trDelta, result );
		break;
	case TR_NONLINEAR_STOP:
		if ( atTime - tr->trTime > tr->trDuration || atTime - tr->trTime <= 0  )
		{
			VectorClear( result );
			return;
		}
		deltaTime = tr->trDuration*0.001f*((float)cos( DEG2RAD(90.0f - (90.0f*((float)(atTime-tr->trTime))/(float)tr->trDuration)) ));
		VectorScale( tr->trDelta, deltaTime, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= DEFAULT_GRAVITY * deltaTime;		// FIXME: local gravity...
		break;
	default:
#ifdef _GAME
		Com_Error( ERR_DROP, "BG_EvaluateTrajectoryDelta: [ GAME] unknown trType: %i", tr->trType );
#else
		Com_Error( ERR_DROP, "BG_EvaluateTrajectoryDelta: [CGAME] unknown trType: %i", tr->trType );
#endif
		break;
	}
}

char *eventnames[] = {
	"EV_NONE",

	"EV_CLIENTJOIN",

	"EV_FOOTSTEP",
	"EV_FOOTSTEP_METAL",
	"EV_FOOTSPLASH",
	"EV_FOOTWADE",
	"EV_SWIM",

	"EV_STEP_4",
	"EV_STEP_8",
	"EV_STEP_12",
	"EV_STEP_16",

	"EV_FALL",

	"EV_JUMP_PAD",			// boing sound at origin", jump sound on player

	"EV_GHOUL2_MARK",			//create a projectile impact mark on something with a client-side g2 instance.

	"EV_GLOBAL_DUEL",
	"EV_PRIVATE_DUEL",

	"EV_JUMP",
	"EV_ROLL",
	"EV_WATER_TOUCH",	// foot touches
	"EV_WATER_LEAVE",	// foot leaves
	"EV_WATER_UNDER",	// head touches
	"EV_WATER_CLEAR",	// head leaves

	"EV_ITEM_PICKUP",			// normal item pickups are predictable
	"EV_GLOBAL_ITEM_PICKUP",	// powerup / team sounds are broadcast to everyone

	"EV_VEH_FIRE",

	"EV_NOAMMO",
	"EV_CHANGE_WEAPON",
	"EV_FIRE_WEAPON",
	"EV_ALT_FIRE",
	"EV_SABER_ATTACK",
	"EV_SABER_HIT",
	"EV_SABER_BLOCK",
	"EV_SABER_CLASHFLARE",
	"EV_SABER_UNHOLSTER",
	"EV_BECOME_JEDIMASTER",
	"EV_DISRUPTOR_MAIN_SHOT",
	"EV_DISRUPTOR_SNIPER_SHOT",
	"EV_DISRUPTOR_SNIPER_MISS",
	"EV_DISRUPTOR_HIT",
	"EV_DISRUPTOR_ZOOMSOUND",

	"EV_PREDEFSOUND",

	"EV_TEAM_POWER",

	"EV_SCREENSHAKE",

	"EV_LOCALTIMER",

	"EV_USE",			// +Use key

	"EV_USE_ITEM0",
	"EV_USE_ITEM1",
	"EV_USE_ITEM2",
	"EV_USE_ITEM3",
	"EV_USE_ITEM4",
	"EV_USE_ITEM5",
	"EV_USE_ITEM6",
	"EV_USE_ITEM7",
	"EV_USE_ITEM8",
	"EV_USE_ITEM9",
	"EV_USE_ITEM10",
	"EV_USE_ITEM11",
	"EV_USE_ITEM12",
	"EV_USE_ITEM13",
	"EV_USE_ITEM14",
	"EV_USE_ITEM15",

	"EV_ITEMUSEFAIL",

	"EV_ITEM_RESPAWN",
	"EV_ITEM_POP",
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",

	"EV_GRENADE_BOUNCE",		// eventParm will be the soundindex
	"EV_MISSILE_STICK",

	"EV_PLAY_EFFECT",
	"EV_PLAY_EFFECT_ID", //finally gave in and added it..
	"EV_PLAY_PORTAL_EFFECT_ID",

	"EV_PLAYDOORSOUND",
	"EV_PLAYDOORLOOPSOUND",
	"EV_BMODEL_SOUND",

	"EV_MUTE_SOUND",
	"EV_VOICECMD_SOUND",
	"EV_GENERAL_SOUND",
	"EV_GLOBAL_SOUND",		// no attenuation
	"EV_GLOBAL_TEAM_SOUND",
	"EV_ENTITY_SOUND",

	"EV_PLAY_ROFF",

	"EV_GLASS_SHATTER",
	"EV_DEBRIS",
	"EV_MISC_MODEL_EXP",

	"EV_CONC_ALT_IMPACT",

	"EV_MISSILE_HIT",
	"EV_MISSILE_MISS",
	"EV_MISSILE_MISS_METAL",
	"EV_BULLET",				// otherEntity is the shooter

	"EV_PAIN",
	"EV_DEATH1",
	"EV_DEATH2",
	"EV_DEATH3",
	"EV_OBITUARY",

	#ifdef BASE_COMPAT
		"EV_POWERUP_QUAD",
		"EV_POWERUP_BATTLESUIT",
	#endif // BASE_COMPAT
	//"EV_POWERUP_REGEN",

	"EV_FORCE_DRAINED",

	"EV_GIB_PLAYER",			// gib a previously living player
	"EV_SCOREPLUM",			// score plum

	"EV_CTFMESSAGE",

	"EV_BODYFADE",

	"EV_SIEGE_ROUNDOVER",
	"EV_SIEGE_OBJECTIVECOMPLETE",

	"EV_DESTROY_GHOUL2_INSTANCE",

	"EV_DESTROY_WEAPON_MODEL",

	"EV_GIVE_NEW_RANK",
	"EV_SET_FREE_SABER",
	"EV_SET_FORCE_DISABLE",

	"EV_WEAPON_CHARGE",
	"EV_WEAPON_CHARGE_ALT",

	"EV_SHIELD_HIT",

	"EV_DEBUG_LINE",
	"EV_TESTLINE",
	"EV_STOPLOOPINGSOUND",
	"EV_STARTLOOPINGSOUND",
	"EV_TAUNT",

//fixme, added a bunch that aren't here!
};

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps ) {

#ifdef _DEBUG
	{
		static vmCvar_t		showEvents;
		static qboolean		isRegistered = qfalse;

		if (!isRegistered)
		{
			trap->Cvar_Register(&showEvents, "showevents", "0", 0);
			isRegistered = qtrue;
		}

		if ( showEvents.integer != 0 ) {
#ifdef _GAME
			Com_Printf(" game event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#else
			Com_Printf("Cgame event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#endif
		}
	}
#endif
	ps->events[ps->eventSequence & (MAX_PS_EVENTS-1)] = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS-1)] = eventParm;
	ps->eventSequence++;
}

/*
========================
BG_TouchJumpPad
========================
*/
void BG_TouchJumpPad(playerState_t *ps, entityState_t *jumppad) {
	vec3_t	angles;
	float p;
	int effectNum;

	// spectators don't use jump pads
	if (ps->pm_type != PM_NORMAL && ps->pm_type != PM_JETPACK && ps->pm_type != PM_FLOAT) {
		return;
	}

	// if we didn't hit this same jumppad the previous frame
	// then don't play the event sound again if we are in a fat trigger
	if (ps->jumppad_ent != jumppad->number) {

		vectoangles(jumppad->origin2, angles);
		p = fabs(AngleNormalize180(angles[PITCH]));
		if (p < 45) {
			effectNum = 0;
		}
		else {
			effectNum = 1;
		}
	}
	// remember hitting this jumppad this frame
	ps->jumppad_ent = jumppad->number;
	ps->jumppad_frame = ps->pmove_framecount;
	// give the player the velocity from the jumppad
	VectorCopy(jumppad->origin2, ps->velocity);
}

/*
=================
BG_EmplacedView

Shared code for emplaced angle gun constriction
=================
*/
int BG_EmplacedView(vec3_t baseAngles, vec3_t angles, float *newYaw, float constraint)
{
	float dif = AngleSubtract(baseAngles[YAW], angles[YAW]);

	if (dif > constraint ||
		dif < -constraint)
	{
		float amt;

		if (dif > constraint)
		{
			amt = (dif-constraint);
			dif = constraint;
		}
		else if (dif < -constraint)
		{
			amt = (dif+constraint);
			dif = -constraint;
		}
		else
		{
			amt = 0.0f;
		}

		*newYaw = AngleSubtract(angles[YAW], -dif);

		if (amt > 1.0f || amt < -1.0f)
		{ //significant, force the view
			return 2;
		}
		else
		{ //just a little out of range
			return 1;
		}
	}

	return 0;
}

//To see if the client is trying to use one of the included skins not meant for MP.
//I don't much care for hardcoded strings, but this seems the best way to go.
qboolean BG_IsValidCharacterModel(const char *modelName, const char *skinName)
{
	if (!Q_stricmp(skinName, "menu"))
	{
		return qfalse;
	}
	else if (!Q_stricmp(modelName, "kyle"))
	{
		if (!Q_stricmp(skinName, "fpls"))
		{
			return qfalse;
		}
		else if (!Q_stricmp(skinName, "fpls2"))
		{
			return qfalse;
		}
		else if (!Q_stricmp(skinName, "fpls3"))
		{
			return qfalse;
		}
	}
	return qtrue;
}

qboolean BG_ValidateSkinForTeam( const char *modelName, char *skinName, int team, float *colors )
{
#ifdef __FORCED_TEAM_COLORS__
	if (strlen (modelName) > 5 && Q_stricmpn (modelName, "jedi_", 5) == 0)
	{ //argh, it's a custom player skin!
		if (team == FACTION_EMPIRE && colors)
		{
			colors[0] = 1.0f;
			colors[1] = 0.0f;
			colors[2] = 0.0f;
		}
		else if (team == FACTION_REBEL && colors)
		{
			colors[0] = 0.0f;
			colors[1] = 0.0f;
			colors[2] = 1.0f;
		}
		return qtrue;
	}

	if (team == FACTION_EMPIRE)
	{
		if ( Q_stricmp( "red", skinName ) != 0 )
		{//not "red"
			if ( Q_stricmp( "blue", skinName ) == 0
				|| Q_stricmp( "default", skinName ) == 0
				|| strchr(skinName, '|')//a multi-skin playerModel
				|| !BG_IsValidCharacterModel(modelName, skinName) )
			{
				Q_strncpyz(skinName, "red", MAX_QPATH);
				return qfalse;
			}
			else
			{//need to set it to red
				int len = strlen( skinName );
				if ( len < 3 )
				{//too short to be "red"
					Q_strcat(skinName, MAX_QPATH, "_red");
				}
				else
				{
					char	*start = &skinName[len-3];
					if ( Q_strncmp( "red", start, 3 ) != 0 )
					{//doesn't already end in "red"
						if ( len+4 >= MAX_QPATH )
						{//too big to append "_red"
							Q_strncpyz(skinName, "red", MAX_QPATH);
							return qfalse;
						}
						else
						{
							Q_strcat(skinName, MAX_QPATH, "_red");
						}
					}
				}
				//if file does not exist, set to "red"
				if ( !BG_FileExists( va( "models/players/%s/model_%s.skin", modelName, skinName ) ) )
				{
					Q_strncpyz(skinName, "red", MAX_QPATH);
				}
				return qfalse;
			}
		}

	}
	else if (team == FACTION_REBEL)
	{
		if ( Q_stricmp( "blue", skinName ) != 0 )
		{
			if ( Q_stricmp( "red", skinName ) == 0
				|| Q_stricmp( "default", skinName ) == 0
				|| strchr(skinName, '|')//a multi-skin playerModel
				|| !BG_IsValidCharacterModel(modelName, skinName) )
			{
				Q_strncpyz(skinName, "blue", MAX_QPATH);
				return qfalse;
			}
			else
			{//need to set it to blue
				int len = strlen( skinName );
				if ( len < 4 )
				{//too short to be "blue"
					Q_strcat(skinName, MAX_QPATH, "_blue");
				}
				else
				{
					char	*start = &skinName[len-4];
					if ( Q_strncmp( "blue", start, 4 ) != 0 )
					{//doesn't already end in "blue"
						if ( len+5 >= MAX_QPATH )
						{//too big to append "_blue"
							Q_strncpyz(skinName, "blue", MAX_QPATH);
							return qfalse;
						}
						else
						{
							Q_strcat(skinName, MAX_QPATH, "_blue");
						}
					}
				}
				//if file does not exist, set to "blue"
				if ( !BG_FileExists( va( "models/players/%s/model_%s.skin", modelName, skinName ) ) )
				{
					Q_strncpyz(skinName, "blue", MAX_QPATH);
				}
				return qfalse;
			}
		}
	}
#endif //__FORCED_TEAM_COLORS__

	return qtrue;
}

#define __BANDWIDTH_SAVE__

/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap ) {
	int		i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR ) {
		s->eType = ET_INVISIBLE;
	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_INTERPOLATE;

#ifdef __BANDWIDTH_SAVE__
#ifdef _GAME
	if (s->eType == ET_PLAYER)
	{
		gentity_t *ent = &g_entities[s->number];
		qboolean FULL_UPDATE = qfalse;
		
		if (ent->next_full_update <= level.time)
		{
			FULL_UPDATE = qtrue;
		}

		if (FULL_UPDATE || Distance(ent->prev_posTrBase, ps->origin) > 2)
		{
			VectorCopy( ps->origin, s->pos.trBase );
			VectorCopy( s->pos.trBase, ent->prev_posTrBase );
		}
		else
		{
			VectorCopy( ent->prev_posTrBase, s->pos.trBase );
		}

		if ( snap ) {
			SnapVector( s->pos.trBase );
		}

		// set the trDelta for flag direction
		if (FULL_UPDATE || Distance(ent->prev_posTrDelta, ps->velocity) > 16)
		{
			VectorCopy( ps->velocity, s->pos.trDelta );
			VectorCopy( s->pos.trDelta, ent->prev_posTrDelta );
		}
		else
		{
			VectorCopy( ent->prev_posTrDelta, s->pos.trDelta );
		}

		s->apos.trType = TR_INTERPOLATE;

		if (FULL_UPDATE || Distance(ent->prev_aposTrBase, ps->viewangles) > 32)
		{
			VectorCopy( ps->viewangles, s->apos.trBase );
			VectorCopy( s->apos.trBase, ent->prev_aposTrBase );
		}
		else
		{
			VectorCopy( ent->prev_aposTrBase, s->apos.trBase );
		}

		if ( snap ) {
			SnapVector( s->apos.trBase );
		}

		if (FULL_UPDATE || ent->prev_moveDir - ps->movementDir > 24 || ent->prev_moveDir - ps->movementDir < -24 || ps->movementDir == 0)
		{
			s->angles2[YAW] = ps->movementDir;
			ent->prev_moveDir = ps->movementDir;
		}
		else
		{
			s->angles2[YAW] = ent->prev_moveDir;
		}

		if (FULL_UPDATE || ent->prev_speed - ps->speed > 8 || ent->prev_speed - ps->speed < -8 || ps->speed == 0)
		{
			s->speed = ps->speed;
			ent->prev_speed = ps->speed;
		}
		else
		{
			s->speed = ent->prev_speed;
		}

		if (FULL_UPDATE) ent->next_full_update = level.time + 2000; // Force a full update every 2 seconds...
	}
	else
#endif
#endif //__BANDWIDTH_SAVE__
	{
		VectorCopy( ps->origin, s->pos.trBase );

		if ( snap ) {
			SnapVector( s->pos.trBase );
		}
		// set the trDelta for flag direction
		VectorCopy( ps->velocity, s->pos.trDelta );

		s->apos.trType = TR_INTERPOLATE;
		VectorCopy( ps->viewangles, s->apos.trBase );
		if ( snap ) {
			SnapVector( s->apos.trBase );
		}

		s->angles2[YAW] = ps->movementDir;
		s->speed = ps->speed;
	}

	s->trickedentindex = ps->fd.forceMindtrickTargetIndex;
	s->trickedentindex2 = ps->fd.forceMindtrickTargetIndex2;
	s->trickedentindex3 = ps->fd.forceMindtrickTargetIndex3;
	s->trickedentindex4 = ps->fd.forceMindtrickTargetIndex4;

	s->forceFrame = ps->saberLockFrame;

	s->emplacedOwner = ps->electrifyTime;

	s->genericenemyindex = ps->genericEnemyIndex;

	s->activeForcePass = ps->activeForcePass;

	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;

	s->legsFlip = ps->legsFlip;
	s->torsoFlip = ps->torsoFlip;

	s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	s->eFlags2 = ps->eFlags2;

	s->saberInFlight = ps->saberInFlight;
	s->saberEntityNum = ps->saberEntityNum;
	s->saberMove = ps->saberMove;
	s->forcePowersActive = ps->fd.forcePowersActive;

	if (ps->duelInProgress)
	{
		s->bolt1 = 1;
	}
	else
	{
		s->bolt1 = 0;
	}

	s->otherEntityNum2 = ps->emplacedIndex;

	s->saberHolstered = ps->saberHolstered;

	if (ps->genericEnemyIndex != -1)
	{
		s->eFlags |= EF_SEEKERDRONE;
	}

	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int		seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}


	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;

	//NOT INCLUDED IN ENTITYSTATETOPLAYERSTATE:
	s->modelindex2 = ps->weaponstate;
	s->constantLight = ps->weaponChargeTime;

	VectorCopy(ps->lastHitLoc, s->origin2);

	s->isJediMaster = ps->isJediMaster;

	s->time2 = ps->holocronBits;

	s->fireflag = ps->fd.saberAnimLevel;

	s->heldByClient = ps->heldByClient;
	s->ragAttach = ps->ragAttach;

	s->iModelScale = ps->iModelScale;

	s->brokenLimbs = ps->brokenLimbs;

	s->hasLookTarget = ps->hasLookTarget;
	s->lookTarget = ps->lookTarget;

	s->customRGBA[0] = ps->customRGBA[0];
	s->customRGBA[1] = ps->customRGBA[1];
	s->customRGBA[2] = ps->customRGBA[2];
	s->customRGBA[3] = ps->customRGBA[3];

	s->m_iVehicleNum = ps->m_iVehicleNum;

	// UQ1: Added - to display damage stats on client...
	s->damageCrit = ps->damageCrit;
	s->damageValue = ps->damageValue;

	s->damageTypeFlags = ps->damageTypeFlags;
	s->freezeLegsAnim = ps->freezeLegsAnim;
	s->freezeTorsoAnim = ps->freezeTorsoAnim;


	// UQ1: Added - Player Class...
	s->playerClass = ps->playerClass;

	// UQ1: Added - weapons...
	s->primaryWeapon = ps->primaryWeapon;
	s->secondaryWeapon = ps->secondaryWeapon;
	s->temporaryWeapon = ps->temporaryWeapon;

	// UQ1: Added - Event Index... So we can send an extra value with events without creating a whole new entity each time...
	s->eventIndex = ps->eventIndex;

	s->healFxTime = ps->healFxTime;

	// UQ1: Added - NPC sounds...
	s->csSounds_Std = ps->csSounds_Std;
	s->csSounds_Combat = ps->csSounds_Combat;
	s->csSounds_Extra = ps->csSounds_Extra;
	s->csSounds_Jedi = ps->csSounds_Jedi;

	// UQ1: Added - Extra flags...
	//s->extra_flags = ps->extra_flags;

	memcpy(&s->inventoryItems, &ps->inventoryItems, sizeof(ps->inventoryItems));
	memcpy(&s->inventoryMod1, &ps->inventoryMod1, sizeof(ps->inventoryMod1));
	memcpy(&s->inventoryMod2, &ps->inventoryMod2, sizeof(ps->inventoryMod2));
	memcpy(&s->inventoryMod3, &ps->inventoryMod3, sizeof(ps->inventoryMod3));
	memcpy(&s->inventoryEquipped, &ps->inventoryEquipped, sizeof(ps->inventoryEquipped));
}

/*
========================
BG_PlayerStateToEntityStateExtraPolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap ) {
	int		i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR ) {
		s->eType = ET_INVISIBLE;
	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_LINEAR_STOP;

#ifdef __BANDWIDTH_SAVE__
#ifdef _GAME
	if (s->eType == ET_PLAYER)
	{
		gentity_t *ent = &g_entities[s->number];
		qboolean FULL_UPDATE = qfalse;
		
		if (ent->next_full_update <= level.time)
		{
			FULL_UPDATE = qtrue;
		}

		if (FULL_UPDATE || Distance(ent->prev_posTrBase, ps->origin) > 2)
		{
			VectorCopy( ps->origin, s->pos.trBase );
			VectorCopy( s->pos.trBase, ent->prev_posTrBase );
		}
		else
		{
			VectorCopy( ent->prev_posTrBase, s->pos.trBase );
		}

		if ( snap ) {
			SnapVector( s->pos.trBase );
		}

		// set the trDelta for flag direction
		if (FULL_UPDATE || Distance(ent->prev_posTrDelta, ps->velocity) > 16)
		{
			VectorCopy( ps->velocity, s->pos.trDelta );
			VectorCopy( s->pos.trDelta, ent->prev_posTrDelta );
		}
		else
		{
			VectorCopy( ent->prev_posTrDelta, s->pos.trDelta );
		}

		s->apos.trType = TR_INTERPOLATE;

		if (FULL_UPDATE || Distance(ent->prev_aposTrBase, ps->viewangles) > 32)
		{
			VectorCopy( ps->viewangles, s->apos.trBase );
			VectorCopy( s->apos.trBase, ent->prev_aposTrBase );
		}
		else
		{
			VectorCopy( ent->prev_aposTrBase, s->apos.trBase );
		}

		if ( snap ) {
			SnapVector( s->apos.trBase );
		}

		if (FULL_UPDATE || ent->prev_moveDir - ps->movementDir > 24 || ent->prev_moveDir - ps->movementDir < -24 || ps->movementDir == 0)
		{
			s->angles2[YAW] = ps->movementDir;
			ent->prev_moveDir = ps->movementDir;
		}
		else
		{
			s->angles2[YAW] = ent->prev_moveDir;
		}

		if (FULL_UPDATE || ent->prev_speed - ps->speed > 8 || ent->prev_speed - ps->speed < -8 || ps->speed == 0)
		{
			s->speed = ps->speed;
			ent->prev_speed = ps->speed;
		}
		else
		{
			s->speed = ent->prev_speed;
		}

		if (FULL_UPDATE) ent->next_full_update = level.time + 2000; // Force a full update every 2 seconds...
	}
	else
#endif
#endif //__BANDWIDTH_SAVE__
	{
		VectorCopy( ps->origin, s->pos.trBase );

		if ( snap ) {
			SnapVector( s->pos.trBase );
		}

		// set the trDelta for flag direction and linear prediction
		VectorCopy( ps->velocity, s->pos.trDelta );

		VectorCopy( ps->viewangles, s->apos.trBase );

		if ( snap ) {
			SnapVector( s->apos.trBase );
		}

		s->speed = ps->speed;
		s->angles2[YAW] = ps->movementDir;
	}

	// set the time for linear prediction
	//s->pos.trTime = time;
#ifdef CGAMEDLL
	s->pos.trTime = cg.time - 50; // UQ1: NPC think time...
#else //!CGAMEDLL
	s->pos.trTime = 0;
#endif //CGAMEDLL

	// set maximum extra polation time
#ifdef _GAME
	s->pos.trDuration = 1000 / sv_fps.integer;// 50; // 1000 / sv_fps (default = 20)
#elif defined(_CGAME)
	s->pos.trDuration = 1000 / sv_fps.integer;// 50; // 1000 / sv_fps (default = 20)
#else
	s->pos.trDuration = 100; // 1000 / sv_fps (default = 20)
#endif

	s->apos.trType = TR_INTERPOLATE;

	

	s->trickedentindex = ps->fd.forceMindtrickTargetIndex;
	s->trickedentindex2 = ps->fd.forceMindtrickTargetIndex2;
	s->trickedentindex3 = ps->fd.forceMindtrickTargetIndex3;
	s->trickedentindex4 = ps->fd.forceMindtrickTargetIndex4;

	s->forceFrame = ps->saberLockFrame;

	s->emplacedOwner = ps->electrifyTime;

	s->genericenemyindex = ps->genericEnemyIndex;

	s->activeForcePass = ps->activeForcePass;

	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;

	s->legsFlip = ps->legsFlip;
	s->torsoFlip = ps->torsoFlip;

	s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	s->eFlags2 = ps->eFlags2;

	s->saberInFlight = ps->saberInFlight;
	s->saberEntityNum = ps->saberEntityNum;
	s->saberMove = ps->saberMove;
	s->forcePowersActive = ps->fd.forcePowersActive;

	if (ps->duelInProgress)
	{
		s->bolt1 = 1;
	}
	else
	{
		s->bolt1 = 0;
	}

	s->otherEntityNum2 = ps->emplacedIndex;

	s->saberHolstered = ps->saberHolstered;

	if (ps->genericEnemyIndex != -1)
	{
		s->eFlags |= EF_SEEKERDRONE;
	}

	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int		seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}
	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;

	//NOT INCLUDED IN ENTITYSTATETOPLAYERSTATE:
	s->modelindex2 = ps->weaponstate;
	s->constantLight = ps->weaponChargeTime;

	VectorCopy(ps->lastHitLoc, s->origin2);

	s->isJediMaster = ps->isJediMaster;

	s->time2 = ps->holocronBits;

	s->fireflag = ps->fd.saberAnimLevel;

	s->heldByClient = ps->heldByClient;
	s->ragAttach = ps->ragAttach;

	s->iModelScale = ps->iModelScale;

	s->brokenLimbs = ps->brokenLimbs;

	s->hasLookTarget = ps->hasLookTarget;
	s->lookTarget = ps->lookTarget;

	s->customRGBA[0] = ps->customRGBA[0];
	s->customRGBA[1] = ps->customRGBA[1];
	s->customRGBA[2] = ps->customRGBA[2];
	s->customRGBA[3] = ps->customRGBA[3];

	s->m_iVehicleNum = ps->m_iVehicleNum;

	// UQ1: Added - to display damage stats on client...
	s->damageCrit = ps->damageCrit;
	s->damageValue = ps->damageValue;

	s->damageTypeFlags = ps->damageTypeFlags;
	s->freezeLegsAnim = ps->freezeLegsAnim;
	s->freezeTorsoAnim = ps->freezeTorsoAnim;


	// UQ1: Added - Player Class...
	s->playerClass = ps->playerClass;

	// UQ1: Added - weapons...
	s->primaryWeapon = ps->primaryWeapon;
	s->secondaryWeapon = ps->secondaryWeapon;
	s->temporaryWeapon = ps->temporaryWeapon;

	// UQ1: Added - Event Index... So we can send an extra value with events without creating a whole new entity each time...
	s->eventIndex = ps->eventIndex;

	s->healFxTime = ps->healFxTime;

	// UQ1: Added - NPC sounds...
	s->csSounds_Std = ps->csSounds_Std;
	s->csSounds_Combat = ps->csSounds_Combat;
	s->csSounds_Extra = ps->csSounds_Extra;
	s->csSounds_Jedi = ps->csSounds_Jedi;

	// UQ1: Added - Extra flags...
	//s->extra_flags = ps->extra_flags;

	memcpy(&s->inventoryItems, &ps->inventoryItems, sizeof(ps->inventoryItems));
	memcpy(&s->inventoryMod1, &ps->inventoryMod1, sizeof(ps->inventoryMod1));
	memcpy(&s->inventoryMod2, &ps->inventoryMod2, sizeof(ps->inventoryMod2));
	memcpy(&s->inventoryMod3, &ps->inventoryMod3, sizeof(ps->inventoryMod3));
	memcpy(&s->inventoryEquipped, &ps->inventoryEquipped, sizeof(ps->inventoryEquipped));
}

/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

int BG_ModelCache(const char *modelName, const char *skinName)
{
	#ifdef _GAME
		void *g2 = NULL;

		if ( VALIDSTRING( skinName ) )
			trap->R_RegisterSkin( skinName );

		//I could hook up a precache ghoul2 function, but oh well, this works
		trap->G2API_InitGhoul2Model( &g2, modelName, 0, 0, 0, 0, 0 );
		//now get rid of it
		if ( g2 )
			trap->G2API_CleanGhoul2Models( &g2 );

		return 0;
	#else // !_GAME
		if ( VALIDSTRING( skinName ) )
		{
			#ifdef _CGAME
				trap->R_RegisterSkin( skinName );
			#else // !_CGAME
				trap->R_RegisterSkin( skinName );
			#endif // _CGAME
		}
		#ifdef _CGAME
			return trap->R_RegisterModel( modelName );
		#else // !_CGAME
			return trap->R_RegisterModel( modelName );
		#endif // _CGAME
	#endif // _GAME
}

#ifdef __MMO__

#if defined(_GAME)
	#define MAX_POOL_SIZE	20480000
#elif defined(_CGAME) //don't need as much for cgame stuff. 2mb will be fine.
	#define MAX_POOL_SIZE	20480000
#elif defined(_UI) //And for the ui the only thing we'll be using this for anyway is allocating anim data for g2 menu models
	#define MAX_POOL_SIZE	512000
#endif

#else //!__MMO__

#if defined(_GAME)
	#define MAX_POOL_SIZE	3000000 //1024000
#elif defined(_CGAME) //don't need as much for cgame stuff. 2mb will be fine.
	#define MAX_POOL_SIZE	2048000
#elif defined(_UI) //And for the ui the only thing we'll be using this for anyway is allocating anim data for g2 menu models
	#define MAX_POOL_SIZE	512000
#endif

#endif //__MMO__

//I am using this for all the stuff like NPC client structures on server/client and
//non-humanoid animations as well until/if I can get dynamic memory working properly
//with casted datatypes, which is why it is so large.


static char		bg_pool[MAX_POOL_SIZE];
static int		bg_poolSize = 0;
static int		bg_poolTail = MAX_POOL_SIZE;

void *BG_Alloc ( int size )
{
	bg_poolSize = ((bg_poolSize + 0x00000003) & 0xfffffffc);

	if (bg_poolSize + size > bg_poolTail)
	{
		Com_Error( ERR_DROP, "BG_Alloc: buffer exceeded tail (%d > %d)", bg_poolSize + size, bg_poolTail);
		return 0;
	}

	bg_poolSize += size;

	return &bg_pool[bg_poolSize-size];
}

void *BG_AllocUnaligned ( int size )
{
	if (bg_poolSize + size > bg_poolTail)
	{
		Com_Error( ERR_DROP, "BG_AllocUnaligned: buffer exceeded tail (%d > %d)", bg_poolSize + size, bg_poolTail);
		return 0;
	}

	bg_poolSize += size;

	return &bg_pool[bg_poolSize-size];
}

/*
// UQ1: NEVER use these! They get corrupted!!!
void *BG_TempAlloc( int size )
{
	size = ((size + 0x00000003) & 0xfffffffc);

	if (bg_poolTail - size < bg_poolSize)
	{
		Com_Error( ERR_DROP, "BG_TempAlloc: buffer exceeded head (%d > %d)", bg_poolTail - size, bg_poolSize);
		return 0;
	}

	bg_poolTail -= size;

	return &bg_pool[bg_poolTail];
}

void BG_TempFree( int size )
{
	size = ((size + 0x00000003) & 0xfffffffc);

	if (bg_poolTail+size > MAX_POOL_SIZE)
	{
		Com_Error( ERR_DROP, "BG_TempFree: tail greater than size (%d > %d)", bg_poolTail+size, MAX_POOL_SIZE );
	}

	bg_poolTail += size;
}
*/

char *BG_StringAlloc ( const char *source )
{
	char *dest = (char*)BG_Alloc( strlen ( source ) + 1 );
	strcpy( dest, source );
	return dest;
}

qboolean BG_OutOfMemory ( void )
{
	return (qboolean)(bg_poolSize >= MAX_POOL_SIZE);
}

const char *gametypeStringShort[GT_MAX_GAME_TYPE] = {
	"FFA",
	"HOLO",
	"JM",
	"1v1",
	"2v1",
	"INSTANCE",
	"SP",
	"TDM",
	"SAGA",
	"CTF",
	"CTY"
};

const char *BG_GetGametypeString( int gametype )
{
	switch ( gametype )
	{
	case GT_FFA:
		return "Free For All";
	case GT_HOLOCRON:
		return "Holocron";
	case GT_JEDIMASTER:
		return "Jedi Master";
	case GT_DUEL:
		return "Duel";
	case GT_POWERDUEL:
		return "Power Duel";
	case GT_SINGLE_PLAYER:
		return "Cooperative";
	case GT_INSTANCE:
		return "Group Instance";
	case GT_TEAM:
		return "Team Deathmatch";
	case GT_SIEGE:
		return "Siege";
	case GT_CTF:
		return "Capture The Flag";
	case GT_CTY:
		return "Capture The Ysalimiri";
	case GT_WARZONE:
		return "War Zone";

	default:
		return "Unknown Gametype";
	}
}

// JKG: Bit of damage types stuff
qboolean JKG_DamageTypeFreezes(const damageType_t damageType)
{
	if (damageType & DT_STUN)
	{
		return qtrue;
	}

	if (damageType & DT_CARBONITE)
	{
		return qtrue;
	}

	if (damageType & DT_FREEZE)
	{
		return qtrue;
	}

	return qfalse;
}


int BG_GetGametypeForString( const char *gametype )
{
		 if ( !Q_stricmp( gametype, "ffa" )
			||!Q_stricmp( gametype, "dm" ) )			return GT_FFA;
	else if ( !Q_stricmp( gametype, "holocron" ) )		return GT_HOLOCRON;
	else if ( !Q_stricmp( gametype, "jm" ) )			return GT_JEDIMASTER;
	else if ( !Q_stricmp( gametype, "duel" ) )			return GT_DUEL;
	else if ( !Q_stricmp( gametype, "powerduel" ) )		return GT_POWERDUEL;
	else if ( !Q_stricmp( gametype, "sp" )
			||!Q_stricmp( gametype, "coop" ) )			return GT_SINGLE_PLAYER;
	else if ( !Q_stricmp( gametype, "instance" ) )		return GT_INSTANCE;
	else if ( !Q_stricmp( gametype, "tdm" )
			||!Q_stricmp( gametype, "tffa" )
			||!Q_stricmp( gametype, "team" ) )			return GT_TEAM;
	else if ( !Q_stricmp( gametype, "siege" ) )			return GT_SIEGE;
	else if ( !Q_stricmp( gametype, "ctf" ) )			return GT_CTF;
	else if ( !Q_stricmp( gametype, "cty" ) )			return GT_CTY;
	else if ( !Q_stricmp( gametype, "warzone" ) )		return GT_WARZONE;
	else												return -1;
}
