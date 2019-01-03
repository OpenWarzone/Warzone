#pragma once

#include "g_local.h"
#include "bg_saga.h"
#include "../botlib/be_ai_goal.h"
#include "../botlib/be_ai_move.h"
#include "../botlib/botlib.h"

#define __DOMINANCE_AI__ // UnqueOne's AI.

#ifdef __AAS_AI_TESTING__
#include "../botlib/be_aas.h"

//travel types
#define MAX_TRAVELTYPES				32
#define TRAVEL_INVALID				1		//temporary not possible
#define TRAVEL_WALK					2		//walking
#define TRAVEL_CROUCH				3		//crouching
#define TRAVEL_BARRIERJUMP			4		//jumping onto a barrier
#define TRAVEL_JUMP					5		//jumping
#define TRAVEL_LADDER				6		//climbing a ladder
#define TRAVEL_WALKOFFLEDGE			7		//walking of a ledge
#define TRAVEL_SWIM					8		//swimming
#define TRAVEL_WATERJUMP			9		//jump out of the water
#define TRAVEL_TELEPORT				10		//teleportation
#define TRAVEL_ELEVATOR				11		//travel by elevator
#define TRAVEL_ROCKETJUMP			12		//rocket jumping required for travel
#define TRAVEL_BFGJUMP				13		//bfg jumping required for travel
#define TRAVEL_GRAPPLEHOOK			14		//grappling hook required for travel
#define TRAVEL_DOUBLEJUMP			15		//double jump
#define TRAVEL_RAMPJUMP				16		//ramp jump
#define TRAVEL_STRAFEJUMP			17		//strafe jump
#define TRAVEL_JUMPPAD				18		//jump pad
#define TRAVEL_FUNCBOB				19		//func bob

//additional travel flags
#define TRAVELTYPE_MASK				0xFFFFFF
#define TRAVELFLAG_NOTTEAM1			(1 << 24)
#define TRAVELFLAG_NOTTEAM2			(2 << 24)
#endif //__AAS_AI_TESTING__


//#define FORCEJUMP_INSTANTMETHOD 1

#define MAX_CHAT_BUFFER_SIZE 8192
#define MAX_CHAT_LINE_SIZE 128

#define TABLE_BRANCH_DISTANCE 32
#define MAX_NODETABLE_SIZE 16384

#define MAX_LOVED_ONES 4
#define MAX_ATTACHMENT_NAME 64

#define MAX_FORCE_INFO_SIZE 2048

#define WPFLAG_JUMP				0x00000010 //jump when we hit this
#define WPFLAG_DUCK				0x00000020 //duck while moving around here
#define WPFLAG_NOVIS			0x00000400 //go here for a bit even with no visibility
#define WPFLAG_SNIPEORCAMPSTAND	0x00000800 //a good position to snipe or camp - stand
#define WPFLAG_WAITFORFUNC		0x00001000 //wait for a func brushent under this point before moving here
#define WPFLAG_SNIPEORCAMP		0x00002000 //a good position to snipe or camp - crouch
#define WPFLAG_ONEWAY_FWD		0x00004000 //can only go forward on the trial from here (e.g. went over a ledge)
#define WPFLAG_ONEWAY_BACK		0x00008000 //can only go backward on the trail from here
#define WPFLAG_GOALPOINT		0x00010000 //make it a goal to get here.. goal points will be decided by setting "weight" values
#define WPFLAG_RED_FLAG			0x00020000 //red flag
#define WPFLAG_BLUE_FLAG		0x00040000 //blue flag
#define WPFLAG_SIEGE_REBELOBJ	0x00080000 //rebel siege objective
#define WPFLAG_SIEGE_IMPERIALOBJ	0x00100000 //imperial siege objective
#define WPFLAG_NOMOVEFUNC		0x00200000 //don't move over if a func is under

#define WPFLAG_CALCULATED		0x00400000 //don't calculate it again
#define WPFLAG_NEVERONEWAY		0x00800000 //never flag it as one-way

#define WPFLAG_DESTROY_FUNCBREAK	0x01000000 //destroy all the func_breakables in the area
//before moving to this waypoint
#define WPFLAG_REDONLY				0x02000000 //only bots on the red team will be able to
//use this waypoint
#define WPFLAG_BLUEONLY				0x04000000 //only bots on the blue team will be able to
//use this waypoint
#define WPFLAG_FORCEPUSH			0x08000000 //force push all the active func_doors in the
//area before moving to this waypoint.
#define WPFLAG_FORCEPULL			0x10000000 //force pull all the active func_doors in the
//area before moving to this waypoint.	
#define WPFLAG_COVER				0x20000000 //cover point

#define WPFLAG_WATER				0x40000000 //water point

#define WPFLAG_ROAD					0x80000000 //road

#define LEVELFLAG_NOPOINTPREDICTION			1 //don't take waypoint beyond current into account when adjusting path view angles
#define LEVELFLAG_IGNOREINFALLBACK			2 //ignore enemies when in a fallback navigation routine
#define LEVELFLAG_IMUSTNTRUNAWAY			4 //don't be scared

#define WP_KEEP_FLAG_DIST			128

#define BWEAPONRANGE_MELEE			1
#define BWEAPONRANGE_MID			2
#define BWEAPONRANGE_LONG			3
#define BWEAPONRANGE_SABER			4

#define MELEE_ATTACK_RANGE			256
#define SABER_ATTACK_RANGE			128
#define MAX_CHICKENWUSS_TIME		10000 //wait 10 secs between checking which run-away path to take

#define BOT_RUN_HEALTH				40
#define BOT_WPTOUCH_DISTANCE		32

//Distance at which a bot knows it touched the weapon/spawnpoint it was traveling to
#define	BOT_WEAPTOUCH_DISTANCE		10

#define ENEMY_FORGET_MS				10000
//if our enemy isn't visible within 10000ms (aprx 10sec) then "forget" about him and treat him like every other threat, but still look for
//more immediate threats while main enemy is not visible

#define BOT_PLANT_DISTANCE			256 //plant if within this radius from the last spotted enemy position
#define BOT_PLANT_INTERVAL			15000 //only plant once per 15 seconds at max
#define BOT_PLANT_BLOW_DISTANCE		256 //blow det packs if enemy is within this radius and I am further away than the enemy

#define BOT_MAX_WEAPON_GATHER_TIME	1000 //spend a max of 1 second after spawn issuing orders to gather weapons before attacking enemy base
#define BOT_MAX_WEAPON_CHASE_TIME	15000 //time to spend gathering the weapon before persuing the enemy base (in case it takes longer than expected)

#define BOT_MAX_WEAPON_CHASE_CTF	5000 //time to spend gathering the weapon before persuing the enemy base (in case it takes longer than expected) [ctf-only]

#define BOT_MIN_SIEGE_GOAL_SHOOT		1024
#define BOT_MIN_SIEGE_GOAL_TRAVEL	128

#define BASE_GUARD_DISTANCE			256 //guarding the flag
#define BASE_FLAGWAIT_DISTANCE		256 //has the enemy flag and is waiting in his own base for his flag to be returned
#define BASE_GETENEMYFLAG_DISTANCE	256 //waiting around to get the enemy's flag

#define BOT_FLAG_GET_DISTANCE		256

#define BOT_SABER_THROW_RANGE		800

typedef enum
{
	CTFSTATE_NONE,
	CTFSTATE_ATTACKER,
	CTFSTATE_DEFENDER,
	CTFSTATE_RETRIEVAL,
	CTFSTATE_GUARDCARRIER,
	CTFSTATE_GETFLAGHOME,
	CTFSTATE_MAXCTFSTATES
} bot_ctf_state_t;

typedef enum
{
	SIEGESTATE_NONE,
	SIEGESTATE_ATTACKER,
	SIEGESTATE_DEFENDER,
	SIEGESTATE_MAXSIEGESTATES
} bot_siege_state_t;

typedef enum
{
	TEAMPLAYSTATE_NONE,
	TEAMPLAYSTATE_FOLLOWING,
	TEAMPLAYSTATE_ASSISTING,
	TEAMPLAYSTATE_REGROUP,
	TEAMPLAYSTATE_MAXTPSTATES
} bot_teamplay_state_t;

typedef struct botattachment_s
{
	int level;
	char name[MAX_ATTACHMENT_NAME];
} botattachment_t;

typedef struct nodeobject_s
{
	vec3_t origin;
	//	int index;
	float weight;
	int flags;
	int neighbornum;
	int inuse;
} nodeobject_t;

typedef struct boteventtracker_s
{
	int			eventSequence;
	int			events[MAX_PS_EVENTS];
	float		eventTime;
} boteventtracker_t;

typedef struct botskills_s
{
	int					reflex;
	float				accuracy;
	float				turnspeed;
	float				turnspeed_combat;
	float				maxturn;
	int					perfectaim;
} botskills_t;

typedef int bot_route_t[MAX_WPARRAY_SIZE];

#define MAX_BOTSETTINGS_FILEPATH			144

//bot settings
typedef struct bot_settings_s
{
	char personalityfile[MAX_BOTSETTINGS_FILEPATH];
	float skill;
	char team[MAX_BOTSETTINGS_FILEPATH];
} bot_settings_t;

#ifdef __AAS_AI_TESTING__
#define MAX_ACTIVATESTACK		8
#define MAX_ACTIVATEAREAS		32

typedef struct bot_activategoal_s
{
	int inuse;
	bot_goal_t goal;						//goal to activate (buttons etc.)
	float time;								//time to activate something
	float start_time;						//time starting to activate something
	float justused_time;					//time the goal was used
	int shoot;								//true if bot has to shoot to activate
	int weapon;								//weapon to be used for activation
	vec3_t target;							//target to shoot at to activate something
	vec3_t origin;							//origin of the blocking entity to activate
	int areas[MAX_ACTIVATEAREAS];			//routing areas disabled by blocking entity
	int numareas;							//number of disabled routing areas
	int areasdisabled;						//true if the areas are disabled for the routing
	struct bot_activategoal_s *next;		//next activate goal on stack
} bot_activategoal_t;

#define BFL_STRAFENONE		0
#define BFL_ATTACKED		1
#define BFL_STRAFERIGHT		2
#define BFL_STRAFELEFT		3
#define BFL_IDEALVIEWSET	4
#define BFL_AVOIDRIGHT		5
#define BFL_AVOIDLEFT		6
#endif //__AAS_AI_TESTING__

//bot state
typedef struct bot_state_s
{
	int inuse;										//true if this state is used by a bot client
	int botthink_residual;							//residual for the bot thinks
	int client;										//client number of the bot
	int entitynum;									//entity number of the bot
	playerState_t cur_ps;							//current player state
	usercmd_t lastucmd;								//usercmd from last frame
	bot_settings_t settings;						//several bot settings
	float thinktime;								//time the bot thinks this frame
	vec3_t origin;									//origin of the bot
	vec3_t velocity;								//velocity of the bot
	vec3_t eye;										//eye coordinates of the bot
	int setupcount;									//true when the bot has just been setup
	float ltime;									//local bot time
	float entergame_time;							//time the bot entered the game
	int ms;											//move state of the bot
	int gs;											//goal state of the bot
	int ws;											//weapon state of the bot
	vec3_t viewangles;								//current view angles
	vec3_t ideal_viewangles;						//ideal view angles
	vec3_t viewanglespeed;

	//rww - new AI values
	gentity_t			*currentEnemy;
	gentity_t			*revengeEnemy;

	gentity_t			*squadLeader;

	gentity_t			*lastHurt;
	gentity_t			*lastAttacked;

	gentity_t			*wantFlag;

	gentity_t			*touchGoal;
	gentity_t			*shootGoal;

	gentity_t			*dangerousObject;

	vec3_t				staticFlagSpot;

	int					revengeHateLevel;
	int					isSquadLeader;

	int					squadRegroupInterval;
	int					squadCannotLead;

	int					lastDeadTime;

	wpobject_t			*wpCurrent;
	wpobject_t			*wpNext;
	wpobject_t			*wpLast;
	wpobject_t			*wpDestination;
	wpobject_t			*wpStoreDest;
	vec3_t				goalAngles;
	vec3_t				goalMovedir;
	vec3_t				goalPosition;

	vec3_t				lastEnemySpotted;
	vec3_t				hereWhenSpotted;
	int					lastVisibleEnemyIndex;
	int					hitSpotted;

	int					wpDirection;

	float				destinationGrabTime;
	float				wpSeenTime;
	float				wpTravelTime;
	float				wpDestSwitchTime;
	float				wpSwitchTime;
	float				wpDestIgnoreTime;

	float				timeToReact;

	int					enemySeenTime;
	int					enemyScanTime;

	float				chickenWussCalculationTime;

	float				beStill;
	float				duckTime;
	float				jumpTime;
	float				jumpHoldTime;
	float				jumpPrep;
	float				forceJumping;
	float				jDelay;

	float				aimOffsetTime;
	float				aimOffsetAmtYaw;
	float				aimOffsetAmtPitch;

	float				frame_Waypoint_Len;
	int					frame_Waypoint_Vis;
	float				frame_Enemy_Len;
	int					frame_Enemy_Vis;

	int					isCamper;
	float				isCamping;
	wpobject_t			*wpCamping;
	wpobject_t			*wpCampingTo;
	qboolean			campStanding;

	int					randomNavTime;
	int					randomNav;

	int					saberSpecialist;

	int					canChat;
	int					chatFrequency;
	char				currentChat[MAX_CHAT_LINE_SIZE];
	float				chatTime;
	float				chatTime_stored;
	int					doChat;
	int					chatTeam;
	gentity_t			*chatObject;
	gentity_t			*chatAltObject;

	float				meleeStrafeTime;
	int					meleeStrafeDir;
	float				meleeStrafeDisable;

	int					altChargeTime;

	float				escapeDirTime;

	float				dontGoBack;

	int					doAttack;
	int					doAltAttack;

	int					forceWeaponSelect;
	int					virtualWeapon;

	int					plantTime;
	int					plantDecided;
	int					plantContinue;
	int					plantKillEmAll;

	int					runningLikeASissy;
	int					runningToEscapeThreat;

	//char				chatBuffer[MAX_CHAT_BUFFER_SIZE];
	//Since we're once again not allocating bot structs dynamically,
	//shoving a 64k chat buffer into one is a bad thing.

	botskills_t			skills;

	botattachment_t		loved[MAX_LOVED_ONES];
	int					lovednum;

	int					loved_death_thresh;

	int					deathActivitiesDone;

	float				botWeaponWeights[WP_NUM_WEAPONS];

	int					ctfState;

	int					siegeState;

	int					teamplayState;

	int					jmState;

	int					state_Forced; //set by player ordering menu

	int					saberDefending;
	int					saberDefendDecideTime;
	int					saberBFTime;
	int					saberBTime;
	int					saberSTime;
	int					saberThrowTime;

	qboolean			saberPower;
	int					saberPowerTime;

	int					botChallengingTime;

	char				forceinfo[MAX_FORCE_INFO_SIZE];

#ifndef FORCEJUMP_INSTANTMETHOD
	int					forceJumpChargeTime;
#endif

	int					doForcePush;

	int					noUseTime;
	qboolean			doingFallback;

	int					iHaveNoIdeaWhereIAmGoing;
	vec3_t				lastSignificantAreaChange;
	int					lastSignificantChangeTime;

	int					forceMove_Forward;
	int					forceMove_Right;
	int					forceMove_Up;

	//bot's wp route path
	bot_route_t			botRoute;

	//Order level stuff
	//bot's current order/behavior
	int					botOrder;
	//bot orderer's clientNum
	int					ordererNum;
	//order's relivent entity
	gentity_t			*orderEntity;
	//order siege objective
	int					orderObjective;

	//viewangles of enemy when you last saw him.
	vec3_t				lastEnemyAngles;

	//Tactical Level
	int					currentTactic;
	gentity_t			*tacticEntity;
	//objective number
	int					tacticObjective;
	//objective type
	int					objectiveType;

	//Stuff to make the BOTORDER_KNEELBEFOREZOD work
	qboolean			doZodKneel;
	int					zodKneelTime;

	//Visual scan behavior
	qboolean			doVisualScan;
	int					VisualScanTime;
	vec3_t				VisualScanDir;

	//current bot behavior
	int					botBehave;

	//evade direction
	int					evadeTime;
	int					evadeDir;

	//Walk flag
	qboolean			doWalk;

	vec3_t				DestPosition;

	//Used to prevent a whole much of destination checks when moving around
	vec3_t				lastDestPosition;

	//performing some sort of special action (destroying breakables, pushing switches, etc)
	//Don't try to override this when this is occurring.
	qboolean			wpSpecial;

	//Do Jump Flag for the TAB Bot
	qboolean			doJump;

	//do Force Pull for this amount of time
	int					doForcePull;

	//position we were at when we first decided to go to this waypoint
	vec3_t				wpCurrentLoc;

	//This debounces the push pull to prevent the bots from push/pulling stuff for navigation
	//purposes
	int					DontSpamPushPull;

	//debouncer for button presses, since this doesn't reset with wp point changes, be 
	//careful not to set this too high
	int					DontSpamButton;

	//have you checked for an alternate route?
	qboolean			AltRouteCheck;

	//entity number you ignore for move traces.
	int					DestIgnore;

	//hold down the Use Button.
	int					useTime;

	//Debouncer for vchats to prevent the bots from spamming the hell out of them.
	int					vchatTime;

	//debouncer for the saberlock button presses.  So you can boost the bot fps without
	//problems.
	int					saberLockDebounce;

#ifdef __NEW_ASTAR__
	int					pathsize;
#endif //__NEW_ASTAR__

	int					next_path_calculate_time;
	qboolean			ready_to_calculate_path;
	//end rww

#ifdef __AAS_AI_TESTING__
	bot_goal_t			goal;
	bot_input_t			bi;
	int					areanum;
	int					predictobstacles_goalareanum;
	float				predictobstacles_time;
	float				attackchase_time;
	float				attackcrouch_time;
	float				attackjump_time;
	float				attackstrafe_time;
	float				teleport_time;
	float				weaponchange_time;
	float				firethrottlewait_time;
	float				firethrottleshoot_time;
	float				notblocked_time;
	float				enemysight_time;
	float				enemydeath_time;
	float				enemyvisible_time;
	float				enemyposition_time;
	qboolean			enemysuicide;
	vec3_t				enemyvelocity;
	vec3_t				enemyorigin;
	int					lastenemyareanum;
	vec3_t				lastenemyorigin;
	int					flags;
	int					tfl;
	vec3_t				aimtarget;

	bot_activategoal_t *activatestack;				//first activate goal on the stack
	bot_activategoal_t activategoalheap[MAX_ACTIVATESTACK];	//activate goal heap

	float				reachedaltroutegoal_time;
	bot_goal_t			altroutegoal;

	vec3_t				moveDir;
#endif //__AAS_AI_TESTING__
} bot_state_t;


//used for objective dependancy stuff
#define		MAX_OBJECTIVES			6

//max allowed objective dependancy
#define		MAX_OBJECTIVEDEPENDANCY	6

//TAB bot orders/tactical options
#ifndef __linux__
typedef enum {
#else
enum {
#endif
	BOTORDER_NONE,  //no order
	BOTORDER_KNEELBEFOREZOD,  //Kneel before the ordered person
	BOTORDER_SEARCHANDDESTROY,	//Attack mode.  If given an entity the bot will search for
	//and then attack that entity.  If NULL, the bot will just
	//hunt around and attack enemies.
	BOTORDER_OBJECTIVE,	//Do objective play for seige.  Bot will defend or attack objective
	//based on who's objective it is.
	BOTORDER_SIEGECLASS_INFANTRY,
	BOTORDER_SIEGECLASS_VANGUARD,
	BOTORDER_SIEGECLASS_SUPPORT,
	BOTORDER_SIEGECLASS_JEDI,
	BOTORDER_SIEGECLASS_DEMOLITIONIST,
	BOTORDER_SIEGECLASS_HEAVY_WEAPONS,
	BOTORDER_MAX
};

#ifndef __linux__
typedef enum {
#else
enum {
#endif
	//[/Linux]
	OT_NONE,	//no OT selected or bad OT
	OT_ATTACK,	//Attack this objective, for destroyable stationary objectives
	OT_DEFEND,  //Defend this objective, for destroyable stationary objectives 
	//or touch objectives
	OT_CAPTURE,  //Capture this objective
	OT_DEFENDCAPTURE,  //prevent capture of this objective
	OT_TOUCH,
	OT_VEHICLE,  //get this vehicle to the related trigger_once.
	OT_WAIT		//This is used by the bots to while they are waiting for a vehicle to respawn

};

//resets the whole bot state
void BotResetState(bot_state_t *bs);
//returns the number of bots in the game
int NumBots(void);

void BotUtilizePersonality(bot_state_t *bs);
int BotDoChat(bot_state_t *bs, char *section, int always);
void StandardBotAI(bot_state_t *bs, float thinktime);
void BotWaypointRender(void);
int OrgVisibleBox(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, int ignore);
int BotIsAChickenWuss(bot_state_t *bs);
int GetNearestVisibleWP(vec3_t org, int ignore);
int GetBestIdleGoal(bot_state_t *bs);

char *ConcatArgs(int start);

extern vmCvar_t bot_forcepowers;
extern vmCvar_t bot_forgimmick;
extern vmCvar_t bot_honorableduelacceptance;
#ifdef _DEBUG
extern vmCvar_t bot_nogoals;
extern vmCvar_t bot_debugmessages;
#endif

extern vmCvar_t bot_attachments;
extern vmCvar_t bot_camp;

extern vmCvar_t bot_wp_info;
extern vmCvar_t bot_wp_edit;
extern vmCvar_t bot_wp_clearweight;
extern vmCvar_t bot_wp_distconnect;
extern vmCvar_t bot_wp_visconnect;

extern wpobject_t *flagRed;
extern wpobject_t *oFlagRed;
extern wpobject_t *flagBlue;
extern wpobject_t *oFlagBlue;

extern gentity_t *eFlagRed;
extern gentity_t *eFlagBlue;

extern char gBotChatBuffer[MAX_CLIENTS][MAX_CHAT_BUFFER_SIZE];
extern float gWPRenderTime;
extern float gDeactivated;
extern float gBotEdit;
extern int gWPRenderedFrame;

extern wpobject_t *gWPArray[MAX_WPARRAY_SIZE];
extern int gWPNum;

extern int gLastPrintedIndex;
extern nodeobject_t nodetable[MAX_NODETABLE_SIZE];
extern int nodenum;

extern int gLevelFlags;

extern float floattime;
#define FloatTime() floattime




#ifdef __DOMINANCE_AI__
extern void DOM_StandardBotAI(bot_state_t *bs, float thinktime);
extern void DOM_StandardBotAI2(bot_state_t *bs, float thinktime);
#endif //__DOMINANCE_AI__
