#pragma once

//Distance ratings
typedef enum
{
	DIST_MELEE,
	DIST_LONG,
} distance_e;

//Attack types
typedef enum
{
	ATTACK_MELEE,
	ATTACK_RANGE,
} attack_e;

enum
{
	SQUAD_IDLE,					//No target found, waiting
	SQUAD_STAND_AND_SHOOT,		//Standing in position and shoot (no cover)
	SQUAD_RETREAT,				//Running away from combat
	SQUAD_COVER,				//Under protective cover
	SQUAD_TRANSITION,			//Moving between points, not firing
	SQUAD_POINT,				//On point, laying down suppressive fire
	SQUAD_SCOUT,				//Poking out to draw enemy
	NUM_SQUAD_STATES,
};

//sigh... had to move in here for groupInfo
typedef enum //# rank_e
{
	RANK_CIVILIAN,
	RANK_CREWMAN,
	RANK_ENSIGN,
	RANK_LT_JG,
	RANK_LT,
	RANK_LT_COMM,
	RANK_COMMANDER,
	RANK_CAPTAIN
} rank_t;

qboolean NPC_CheckPlayerTeamStealth(gentity_t *aiEnt);

//AI_GRENADIER
void NPC_BSGrenadier_Default(gentity_t *aiEnt);

//AI_SNIPER
void NPC_BSSniper_Default( gentity_t *aiEnt );

//AI_STORMTROOPER
void NPC_BSST_Investigate(gentity_t *aiEnt);
void NPC_BSST_Default(gentity_t *aiEnt);
void NPC_BSST_Sleep(gentity_t *aiEnt);

//AI_JEDI
void NPC_BSJedi_Investigate(gentity_t *aiEnt);
void NPC_BSJedi_Default(gentity_t *aiEnt);
void NPC_BSJedi_FollowLeader(gentity_t *aiEnt);

// AI_DROID
void NPC_BSDroid_Default(gentity_t *aiEnt);

// AI_ImperialProbe
void NPC_BSImperialProbe_Default(gentity_t *aiEnt);

// AI_atst
void NPC_BSATST_Default(gentity_t *aiEnt);

void NPC_BSInterrogator_Default(gentity_t *aiEnt);

// AI Mark 1
void NPC_BSMark1_Default(gentity_t *aiEnt);

// AI Mark 2
void NPC_BSMark2_Default(gentity_t *aiEnt);


void NPC_BSMineMonster_Default(gentity_t *aiEnt);
void NPC_BSHowler_Default(gentity_t *aiEnt);
void NPC_BSRancor_Default(gentity_t *aiEnt);

//Utilities
//Group AI
#define	MAX_FRAME_GROUPS	32
// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
typedef struct AIGroupMember_s
{
	int	number;
	int waypoint;
	int pathCostToEnemy;
	int	closestBuddy;
} AIGroupMember_t;

#define MAX_GROUP_MEMBERS 32
// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
typedef struct AIGroupInfo_s
{
	int			numGroup;
	qboolean	processed;
	team_t		team;
	gentity_t	*enemy;
	int			enemyWP;
	int			speechDebounceTime;
	int			lastClearShotTime;
	int			lastSeenEnemyTime;
	int			morale;
	int			moraleAdjust;
	int			moraleDebounce;
	int			memberValidateTime;
	int			activeMemberNum;
	gentity_t	*commander;
	vec3_t		enemyLastSeenPos;
	int			numState[ NUM_SQUAD_STATES ];
	AIGroupMember_t member[ MAX_GROUP_MEMBERS ];
} AIGroupInfo_t;

int	AI_GetGroupSize( vec3_t origin, int radius, team_t playerTeam, gentity_t *avoid );
int AI_GetGroupSize2( gentity_t *ent, int radius );

void AI_GetGroup( gentity_t *self );

qboolean AI_CheckEnemyCollision( gentity_t *ent, qboolean takeEnemy );
gentity_t *AI_DistributeAttack( gentity_t *attacker, gentity_t *enemy, team_t team, int threshold );
