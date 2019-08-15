/*

  **    ***    ***            ***   ****                                   *               ***
  **    ***    ***            ***  *****                                 ***               ***
  ***  *****  ***             ***  ***                                   ***
  ***  *****  ***    *****    *** ******   *****    *** ****    ******  ******    *****    ***  *** ****
  ***  *****  ***   *******   *** ******  *******   *********  ***  *** ******   *******   ***  *********
  ***  ** **  ***  **** ****  ***  ***   ***   ***  ***   ***  ***       ***    ***   ***  ***  ***   ***
  *** *** *** ***  ***   ***  ***  ***   *********  ***   ***  ******    ***    *********  ***  ***   ***
   ****** ******   ***   ***  ***  ***   *********  ***   ***   ******   ***    *********  ***  ***   ***
   *****   *****   ***   ***  ***  ***   ***        ***   ***    ******  ***    ***        ***  ***   ***
   *****   *****   **** ****  ***  ***   ****  ***  ***   ***       ***  ***    ****  ***  ***  ***   ***
   ****     ****    *******   ***  ***    *******   ***   ***  ***  ***  *****   *******   ***  ***   ***
    ***     ***      *****    ***  ***     *****    ***   ***   ******    ****    *****    ***  ***   ***

            ******** **                 ******                        *  **  **
            ******** **                 ******                       **  **  **
               **    **                 **                           **  **
               **    ** **    ***       **      ** *  ****   ** **  **** **  **  ** **    ***
               **    ******  *****      *****   ****  ****   ****** **** **  **  ******  *****
               **    **  **  ** **      *****   **   **  **  **  **  **  **  **  **  **  ** **
               **    **  **  *****      **      **   **  **  **  **  **  **  **  **  **  *****
               **    **  **  **         **      **   **  **  **  **  **  **  **  **  **  **
               **    **  **  ** **      **      **   **  **  **  **  **  **  **  **  **  ** **
               **    **  **  *****      **      **    ****   **  **  *** **  **  **  **  *****
               **    **  **   ***       **      **    ****   **  **  *** **  **  **  **   ***

*/

#define __AUTOWAYPOINT__
#define __AUTOWAYPOINT2__
//#define __COVER_SPOTS__ // UQ1: Not used because we now have NPC bot evasion/cover/etc...
#define __RANDOM_REMOVAL__ // AWP uses completely random removal of excess spots...

#define MAX_NODELINKS       32              // Maximum Node Links (12)
#define MAX_AWP_NODELINKS   16
//#define MAX_NODES           65536
#define MAX_NODES           MAX_WPARRAY_SIZE//131072
#define INVALID				-1
#define MAX_TEMP_AREAS		8388608//4194304//2048000

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#endif //_WIN32

#include "../qcommon/q_shared.h"
#include "../cgame/cg_local.h"
#include "../ui/ui_shared.h"
#include "../game/surfaceflags.h"

extern qboolean RoadExistsAtPoint(vec3_t point);

#ifdef __AUTOWAYPOINT__

//#pragma warning( disable : 4133 )	// signed/unsigned mismatch

#define MOD_DIRECTORY "warzone"

//#define MAX_MAP_SIZE 131072
#define MAX_MAP_SIZE 262144//524288//65536 // UQ1: Checked. q3map2 is incable of generating surfaces outside of this.

#define Vector4Copy(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3])


// ==============================================================================================================================
//
//
//                                                            BEGIN
//                                                AUTO WAYPOINTER MAIN VARIABLES
//
//
//
// ==============================================================================================================================

#define MAX_SLOPE 46.0

//float waypoint_distance_multiplier = 2.5f;
//float waypoint_distance_multiplier = 3.0f;
float waypoint_distance_multiplier = 4.5f;
//float waypoint_distance_multiplier = 3.5f;
//float area_distance_multiplier = 2.0f;
float area_distance_multiplier = 1.5f;

//int waypoint_scatter_distance = 24;
//int outdoor_waypoint_scatter_distance = 63;

//int waypoint_scatter_distance = 192;
//int waypoint_scatter_distance = 150;
int waypoint_scatter_distance = 48;//96;//128;
//int waypoint_scatter_distance = 32;
int outdoor_waypoint_scatter_distance = 192;

#define __TEST_CLEANER__ // Experimental New Cleaning Method...

// ==============================================================================================================================
//
//
//                                                             END
//                                                AUTO WAYPOINTER MAIN VARIABLES
//
//
//
// ==============================================================================================================================


qboolean optimize_again = qfalse;
qboolean DO_THOROUGH = qfalse;
qboolean DO_TRANSLUCENT = qfalse;
qboolean DO_FAST_LINK = qfalse;
qboolean DO_ULTRAFAST = qfalse;
qboolean DO_WATER = qfalse;
qboolean DO_ROCK = qfalse;
qboolean DO_NOSKY = qfalse;
qboolean DO_OPEN_AREA_SPREAD = qfalse;
qboolean DO_NEW_METHOD = qfalse;
qboolean DO_EXTRA_REACH = qfalse;
qboolean DO_SINGLE = qfalse;

// warning C4996: 'strcpy' was declared deprecated
#pragma warning( disable : 4996 )

// warning #1478: function "strcpy" (declared at line XX of "XXX") was declared "deprecated"
//#pragma warning( disable : 1478 )

//
// UQ1: Get CPU Info...
// processor: x86, x64
// Use the __cpuid intrinsic to get information about a CPU
//

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <intrin.h>
#endif //_WIN32
///*#ifdef __linux__
//#include <string>
//#include <vector>
//#include <Lib/Variable.h>
//#include <Lib/Expression.h>
//#include <Lib/ProcessorLexer.h>
//#include <asm-i386/processor.h>
//#endif //__linux__*/

qboolean WP_CheckInSolid (vec3_t position); // below

float VectorDistance( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v;

	VectorSubtract (p2, p1, v);
	return VectorLength( v );
}

float HeightDistance( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v, p1a, p2a;
	VectorCopy(p1, p1a);
	p1a[0] = 0;
	p1a[1] = 0;

	VectorCopy(p2, p2a);
	p2a[0] = 0;
	p2a[1] = 0;

	VectorSubtract (p2a, p1a, v);
	return VectorLength( v );
}

float VerticalDistance( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v, p1a, p2a;
	VectorCopy(p1, p1a);
	p1a[2] = 0;

	VectorCopy(p2, p2a);
	p2a[2] = 0;

	VectorSubtract (p2a, p1a, v);
	return VectorLength( v );
}

const char* szFeatures[] =
{
    "x87 FPU On Chip",
    "Virtual-8086 Mode Enhancement",
    "Debugging Extensions",
    "Page Size Extensions",
    "Time Stamp Counter",
    "RDMSR and WRMSR Support",
    "Physical Address Extensions",
    "Machine Check Exception",
    "CMPXCHG8B Instruction",
    "APIC On Chip",
    "Unknown1",
    "SYSENTER and SYSEXIT",
    "Memory Type Range Registers",
    "PTE Global Bit",
    "Machine Check Architecture",
    "Conditional Move/Compare Instruction",
    "Page Attribute Table",
    "Page Size Extension",
    "Processor Serial Number",
    "CFLUSH Extension",
    "Unknown2",
    "Debug Store",
    "Thermal Monitor and Clock Ctrl",
    "MMX Technology",
    "FXSAVE/FXRSTOR",
    "SSE Extensions",
    "SSE2 Extensions",
    "Self Snoop",
    "Hyper-threading Technology",
    "Thermal Monitor",
    "Unknown4",
    "Pend. Brk. EN."
};

qboolean CPU_CHECKED = qfalse;
qboolean SSE_CPU = qfalse;

float		ENTITY_HEIGHTS[MAX_GENTITIES];
int			NUM_ENTITY_HEIGHTS = 0;
qboolean	ENTITY_HIGHTS_INITIALIZED = qfalse;

qboolean AlreadyHaveEntityAtHeight( float height )
{
	int i = 0;

	for (i = 0; i < NUM_ENTITY_HEIGHTS; i++)
	{
		float heightDif = height - ENTITY_HEIGHTS[i];

		if (heightDif < 0) heightDif = ENTITY_HEIGHTS[i] - height;

		if (heightDif < 128) return qtrue;
	}

	return qfalse;
}

qboolean AIMOD_HaveEntityNearHeight( float height )
{
	int i = 0;

	for (i = 0; i < NUM_ENTITY_HEIGHTS; i++)
	{
		float heightDif = height - ENTITY_HEIGHTS[i];

		if (heightDif < 0) heightDif = ENTITY_HEIGHTS[i] - height;

		if (heightDif < 512) return qtrue;
	}

	return qfalse;
}

void AIMOD_MapEntityHeights()
{
	int i;

	if (ENTITY_HIGHTS_INITIALIZED) return; // Already made the list for this level...

	for (i = 0; i < MAX_GENTITIES; i++)
	{
		centity_t *cent = &cg_entities[i];

		if (!cent) continue;
		if (AlreadyHaveEntityAtHeight( cent->currentState.origin[2] )) continue;

		ENTITY_HEIGHTS[NUM_ENTITY_HEIGHTS] = cent->currentState.origin[2];
		NUM_ENTITY_HEIGHTS++;
	}

	//trap->Print("^3*** ^3%s: ^5Mapped %i entity heights.\n", "AUTO-WAYPOINTER", NUM_ENTITY_HEIGHTS);

	ENTITY_HIGHTS_INITIALIZED = qtrue;
}

float	BAD_HEIGHTS[1024];
int		NUM_BAD_HEIGHTS = 0;

qboolean AIMOD_IsWaypointHeightMarkedAsBad( vec3_t org )
{
	int i = 0;

	AIMOD_MapEntityHeights(); // Initialize the entity height list...

	if (!AIMOD_HaveEntityNearHeight(org[2]))
	{
		if (NUM_ENTITY_HEIGHTS > 0) // Only exit here if the level actually has entities!
			return qtrue;
	}

	for (i = 0; i < NUM_BAD_HEIGHTS; i++)
	{
		float heightDif = org[2] - BAD_HEIGHTS[i];

		if (heightDif < 0) heightDif = BAD_HEIGHTS[i] - org[2];

		if (heightDif < 96/*48*/) return qtrue;
	}

	return qfalse;
}

int CPU_INFO_VALUE = -1;

int UQ_Get_CPU_Info( void )
{
#ifdef _WIN32 // UQ1: Hell, I don't know.. Should work in linux but doesnt...
    char CPUString[0x20];
    char CPUBrandString[0x40];
    int CPUInfo[4] = {-1};
    int nSteppingID = 0;
    int nModel = 0;
    int nFamily = 0;
    int nProcessorType = 0;
    int nExtendedmodel = 0;
    int nExtendedfamily = 0;
    int nBrandIndex = 0;
    int nCLFLUSHcachelinesize = 0;
    int nAPICPhysicalID = 0;
    int nFeatureInfo = 0;
    int nCacheLineSize = 0;
    int nL2Associativity = 0;
    int nCacheSizeK = 0;
    int nRet = 0;
    unsigned    nIds, nExIds, i;
    qboolean    bSSE3NewInstructions = qfalse;
    qboolean    bMONITOR_MWAIT = qfalse;
    qboolean    bCPLQualifiedDebugStore = qfalse;
    qboolean    bThermalMonitor2 = qfalse;

	if (CPU_CHECKED) return CPU_INFO_VALUE;

//#define __PRINT_CPU_INFO__

#ifdef __PRINT_CPU_INFO__
	trap->Print("^4-------------------------- ^5[^7CPU Information^5] ^4--------------------------\n");
#endif //__PRINT_CPU_INFO__

    // __cpuid with an InfoType argument of 0 returns the number of
    // valid Ids in CPUInfo[0] and the CPU identification string in
    // the other three array elements. The CPU identification string is
    // not in linear order. The code below arranges the information
    // in a human readable form.
    __cpuid(CPUInfo, 0);
    nIds = CPUInfo[0];
    memset(CPUString, 0, sizeof(CPUString));
    *((int*)CPUString) = CPUInfo[1];
    *((int*)(CPUString+4)) = CPUInfo[3];
    *((int*)(CPUString+8)) = CPUInfo[2];

    // Get the information associated with each valid Id
    for (i=0; i<=nIds; ++i)
    {
        __cpuid(CPUInfo, i);
#ifdef __PRINT_CPU_INFO__
        /*trap->Print("\nFor InfoType %d\n", i);
        trap->Print("CPUInfo[0] = 0x%x\n", CPUInfo[0]);
        trap->Print("CPUInfo[1] = 0x%x\n", CPUInfo[1]);
        trap->Print("CPUInfo[2] = 0x%x\n", CPUInfo[2]);
        trap->Print("CPUInfo[3] = 0x%x\n", CPUInfo[3]);*/
#endif //__PRINT_CPU_INFO__

        // Interpret CPU feature information.
        if  (i == 1)
        {
            nSteppingID = CPUInfo[0] & 0xf;
            nModel = (CPUInfo[0] >> 4) & 0xf;
            nFamily = (CPUInfo[0] >> 8) & 0xf;
            nProcessorType = (CPUInfo[0] >> 12) & 0x3;
            nExtendedmodel = (CPUInfo[0] >> 16) & 0xf;
            nExtendedfamily = (CPUInfo[0] >> 20) & 0xff;
            nBrandIndex = CPUInfo[1] & 0xff;
            nCLFLUSHcachelinesize = ((CPUInfo[1] >> 8) & 0xff) * 8;
            nAPICPhysicalID = (CPUInfo[1] >> 24) & 0xff;
            bSSE3NewInstructions = (qboolean)((CPUInfo[2] & 0x1) || qfalse);
            bMONITOR_MWAIT = (qboolean)((CPUInfo[2] & 0x8) || qfalse);
            bCPLQualifiedDebugStore = (qboolean)((CPUInfo[2] & 0x10) || qfalse);
            bThermalMonitor2 = (qboolean)((CPUInfo[2] & 0x100) || qfalse);
            nFeatureInfo = CPUInfo[3];
        }
    }

	if  (nFeatureInfo & 25)
	{
		SSE_CPU = qtrue;
	}

    // Calling __cpuid with 0x80000000 as the InfoType argument
    // gets the number of valid extended IDs.
    __cpuid(CPUInfo, 0x80000000);
    nExIds = CPUInfo[0];
    memset(CPUBrandString, 0, sizeof(CPUBrandString));

    // Get the information associated with each extended ID.
    for (i=0x80000000; i<=nExIds; ++i)
    {
        __cpuid(CPUInfo, i);
#ifdef __PRINT_CPU_INFO__
        //trap->Print("\nFor InfoType %x\n", i);
        //trap->Print("CPUInfo[0] = 0x%x\n", CPUInfo[0]);
        //trap->Print("CPUInfo[1] = 0x%x\n", CPUInfo[1]);
        //trap->Print("CPUInfo[2] = 0x%x\n", CPUInfo[2]);
        //trap->Print("CPUInfo[3] = 0x%x\n", CPUInfo[3]);
#endif //__PRINT_CPU_INFO__

        // Interpret CPU brand string and cache information.
        if  (i == 0x80000002)
		{
			memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
		}
        else if  (i == 0x80000003)
		{
			memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
		}
        else if  (i == 0x80000004)
		{
			memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
		}
        else if  (i == 0x80000006)
        {
            nCacheLineSize = CPUInfo[2] & 0xff;
            nL2Associativity = (CPUInfo[2] >> 12) & 0xf;
            nCacheSizeK = (CPUInfo[2] >> 16) & 0xffff;
        }
    }

    // Display all the information in user-friendly format.

#ifdef __PRINT_CPU_INFO__
    if  (nExIds >= 0x80000004)
        trap->Print("^5CPU Brand String: ^3%s\n", CPUBrandString);

	trap->Print("^5CPU String: ^3%s\n", CPUString);

    if  (nExIds >= 0x80000006)
    {
        trap->Print("^5Cache Line Size = ^7%d\n", nCacheLineSize);
        trap->Print("^5L2 Associativity = ^7%d\n", nL2Associativity);
        trap->Print("^5Cache Size = ^7%dK\n", nCacheSizeK);
    }

	if  (nIds >= 1)
    {
        if  (nSteppingID)
            trap->Print("^5Stepping ID = ^7%d\n", nSteppingID);
        if  (nModel)
            trap->Print("^5Model = ^7%d\n", nModel);
        if  (nFamily)
            trap->Print("^5Family = ^7%d\n", nFamily);
        if  (nProcessorType)
            trap->Print("^5Processor Type = ^7%d\n", nProcessorType);
        if  (nExtendedmodel)
            trap->Print("^5Extended model = ^7%d\n", nExtendedmodel);
        if  (nExtendedfamily)
            trap->Print("^5Extended family = ^7%d\n", nExtendedfamily);
        if  (nBrandIndex)
            trap->Print("^5Brand Index = ^7%d\n", nBrandIndex);
        if  (nCLFLUSHcachelinesize)
            trap->Print("^5CLFLUSH cache line size = ^7%d\n",
                     nCLFLUSHcachelinesize);
        if  (nAPICPhysicalID)
            trap->Print("^5APIC Physical ID = ^7%d\n", nAPICPhysicalID);

		if  (nFeatureInfo || bSSE3NewInstructions ||
             bMONITOR_MWAIT || bCPLQualifiedDebugStore ||
             bThermalMonitor2)
        {
            trap->Print("\n^7The following features are supported:\n");

			if  (bSSE3NewInstructions)
				trap->Print("^5* ^4SSE3 New Instructions\n");
			if  (bMONITOR_MWAIT)
				trap->Print("^5* ^4MONITOR/MWAIT\n");
			if  (bCPLQualifiedDebugStore)
				trap->Print("^5* ^4CPL Qualified Debug Store\n");
			if  (bThermalMonitor2)
				trap->Print("^5* ^4Thermal Monitor 2\n");

            i = 0;
            nIds = 1;

#ifdef __SHOW_FULL_CPU_INFO__
            while (i < (sizeof(szFeatures)/sizeof(const char*)))
            {
                if  (nFeatureInfo & nIds)
                {
                    trap->Print("^5* ^4");
                    trap->Print(szFeatures[i]);
                    trap->Print("\n");
                }

                nIds <<= 1;
                ++i;
            }
#endif //__SHOW_FULL_CPU_INFO__
        }
    }

	trap->Print("^4-----------------------------------------------------------------------\n");
#endif //__PRINT_CPU_INFO__

	CPU_CHECKED = qtrue;
	CPU_INFO_VALUE = nRet;
	return  CPU_INFO_VALUE;
#else //_WIN32
	//SSE_CPU = qtrue; // UQ1: Assume because the code doesnt support linux!
	CPU_INFO_VALUE = -1;
	return CPU_INFO_VALUE;
#endif //_WIN32
}


#define	G_MAX_SCRIPT_ACCUM_BUFFERS 10

//vec3_t			botTraceMins = { -20, -20, -1 };
//vec3_t			botTraceMaxs = { 20, 20, 32 };
vec3_t			botTraceMins = { -15, -15, 0 };
vec3_t			botTraceMaxs = { 15, 15, 64 };

//  _  _         _       _____
// | \| |___  __| |___  |_   _|  _ _ __  ___ ___
// | .` / _ \/ _` / -_)   | || || | '_ \/ -_|_-<
// |_|\_\___/\__,_\___|   |_| \_, | .__/\___/__/
//                            |__/|_|
//------------------------------------------------------------------------------------

//===========================================================================
// Description  : Node flags + Link Flags...
#define NODE_INVALID				-1
#define NODE_MOVE					0       // Move Node
#define NODE_OBJECTIVE				1
#define NODE_TARGET					2
#define NODE_LAND_VEHICLE			4
#define NODE_FASTHOP				8
#define NODE_COVER					16
#define NODE_WATER					32
#define NODE_LADDER					64      // Ladder Node
#define	NODE_ROAD					128
#define	NODE_DYNAMITE				256
#define	NODE_BUILD					512
#define	NODE_JUMP					1024
#define	NODE_DUCK					2048
#define	NODE_ICE					4096	// Node is located on ice (slick)...
#define NODE_ALLY_UNREACHABLE		8192
#define NODE_AXIS_UNREACHABLE		16384
#define	NODE_AXIS_DELIVER			32768	//place axis should deliver stolen documents/objective
#define	NODE_ALLY_DELIVER			65536	//place allies should deliver stolen documents/objective

// Node finding flags...
#define NODEFIND_BACKWARD			1      // For selecting nodes behind us.
#define NODEFIND_FORCED				2      // For manually selecting nodes (without using links)
#define NODEFIND_ALL				4      // For selecting all nodes

// Objective types...
#define	OBJECTIVE_DYNAMITE			0		//blow this objective up!
#define	OBJECTIVE_DYNOMITE			0		// if next objective is OBJ_DYNOMITE => engi's are important
#define	OBJECTIVE_STEAL				1		//steal the documents!
#define	OBJECTIVE_CAPTURE			2		//get the flag - not intented for checkpoint, but for spawn flags
#define	OBJECTIVE_BUILD				3		//get the flag - not intented for checkpoint, but for spawn flags
#define	OBJECTIVE_CARRY				4
#define	OBJECTIVE_FLAG				5
#define	OBJECTIVE_POPFLAG			6
#define	OBJECTIVE_AXISONLY			128
#define	OBJECTIVE_ALLYSONLY			256

// es_fix : added comments to flags
// This way the bot knows how to approach the next node
#define		PATH_NORMAL				0		// Bot moves normally to next node : ie equivalant to pressing the forward move key on your keyboard
#define		PATH_CROUCH				1		//bot should duck and walk toward next node
#define		PATH_SPRINT				2		//bot should sprint to next node
#define		PATH_JUMP				4		//bot should jump to next node
#define		PATH_WALK				8		//bot should walk to next node
#define		PATH_BLOCKED			16		//path to next node is blocked
#define		PATH_LADDER				32		//ladders!
#define		PATH_NOTANKS			64		//No Land Vehicles...
#define		PATH_DANGER				128		//path to next node is dangerous - Lava/Slime/Water

//------------------------------------------------------------------------------------

//  _  _         _       ___     _         _ _    _
// | \| |___  __| |___  | _ \___| |__ _  _(_) |__| |
// | .` / _ \/ _` / -_) |   / -_) '_ \ || | | / _` |
// |_|\_\___/\__,_\___| |_|_\___|_.__/\_,_|_|_\__,_|
//------------------------------------------------------------------------------------
qboolean    dorebuild;          // for rebuilding nodes
qboolean	shownodes;
qboolean	nodes_loaded;
//------------------------------------------------------------------------------------

//  _  _         _       _    _      _     ___ _               _
// | \| |___  __| |___  | |  (_)_ _ | |__ / __| |_ _ _ _  _ __| |_ _  _ _ _ ___
// | .` / _ \/ _` / -_) | |__| | ' \| / / \__ \  _| '_| || / _|  _| || | '_/ -_)
// |_|\_\___/\__,_\___| |____|_|_||_|_\_\ |___/\__|_|  \_,_\__|\__|\_,_|_| \___|
//------------------------------------------------------------------------------------
typedef struct nodelink_s       // Node Link Structure
{
    /*short*/ int       targetNode; // Target Node
    float           cost;       // Cost for PathSearch algorithm
	int				flags;
}nodelink_t;                    // Node Link Typedef
//------------------------------------------------------------------------------------
//  _  _         _       ___ _               _
// | \| |___  __| |___  / __| |_ _ _ _  _ __| |_ _  _ _ _ ___
// | .` / _ \/ _` / -_) \__ \  _| '_| || / _|  _| || | '_/ -_)
// |_|\_\___/\__,_\___| |___/\__|_|  \_,_\__|\__|\_,_|_| \___|
//------------------------------------------------------------------------------------
typedef struct node_s           // Node Structure
{
    vec3_t      origin;         // Node Origin
    int         type;           // Node Type
    short int   enodenum;		// Mostly just number of players around this node
	nodelink_t  links[MAX_NODELINKS];	// Store all links
	short int	objectNum[3];		//id numbers of any world objects associated with this node (used only with unreachable flags)
	int			objFlags;			//objective flags if this node is an objective node
	short int	objTeam;			//the team that should complete this objective
	short int	objType;			//what type of objective this is - see OBJECTIVE_XXX flags
	short int	objEntity;			//the entity number of what object to dynamite at a dynamite objective node
	//int			coverpointNum;		// Number of waypoints this node can be used as cover for...
	//int			coverpointFor[1024];	// List of all the waypoints this waypoint is cover for...
} node_t;                       // Node Typedef

typedef struct nodelink_convert_s       // Node Link Structure
{
    /*short*/ int       targetNode; // Target Node
    float           cost;       // Cost for PathSearch algorithm
}nodelink_convert_t;            // Node Link Typedef

typedef struct node_convert_s           // Node Structure
{
    vec3_t				origin;         // Node Origin
    int					type;           // Node Type
    short int			enodenum;		// Mostly just number of players around this node
	nodelink_convert_t  links[MAX_NODELINKS];	// Store all links
} node_convert_t;                       // Node Typedef
//------------------------------------------------------------------------------------

typedef struct enode_s
{
	int			link_node;
	int			num_routes;				// Number of alternate routes available, maximum 5
	int			routes[5];				// Possible alternate routes to reach the node
	team_t		team;
} enode_t;

int		number_of_nodes = 0;
int		optimized_number_of_nodes = 0;
int		aw_num_nodes = 0;
int		optimized_aw_num_nodes = 0;
node_t	*nodes;
node_t	*optimized_nodes;

#define BOT_MOD_NAME	"aimod"
//#define NOD_VERSION		1.1f
float NOD_VERSION = 1.1f;

float aw_percent_complete = 0.0f;
char task_string1[255];
char task_string2[255];
char task_string3[255];
char last_node_added_string[255];

clock_t	aw_stage_start_time = 0;
float	aw_last_percent = 0;
clock_t	aw_last_percent_time = 0;
clock_t	aw_last_times[100];
int		aw_num_last_times = 0;

vec4_t	popBG			=	{0.f,0.f,0.f,0.3f};
vec4_t	popBorder		=	{0.28f,0.28f,0.28f,1.f};
vec4_t	popHover		=	{0.3f,0.3f,0.3f,1.f};
vec4_t	popText			=	{1.f,1.f,1.f,1.f};
vec4_t	popLime			=	{0.f,1.f,0.f,0.7f};
vec4_t	popCyan			=	{0.f,1.f,1.f,0.7f};
vec4_t	popRed			=	{1.f,0.f,0.f,0.7f};
vec4_t	popBlue			=	{0.f,0.f,1.f,0.7f};
vec4_t	popOrange		=	{1.f,0.63f,0.1f,0.7f};
vec4_t	popDefaultGrey	=	{0.38f,0.38f,0.38f,1.0f};
vec4_t	popLightGrey	=	{0.5f,0.5f,0.5f,1.0f};
vec4_t	popDarkGrey		=	{0.33f,0.33f,0.33f,1.0f};
vec4_t	popAlmostWhite	=	{0.83f,0.81f,0.71f,1.0f};
vec4_t	popAlmostBlack	=	{0.16f,0.16f,0.16f,1.0f};

#define POP_HUD_BORDERSIZE 1

#define	Vector4Average(v, b, s, o)	((o)[0]=((v)[0]*(1-(s)))+((b)[0]*(s)),(o)[1]=((v)[1]*(1-(s)))+((b)[1]*(s)),(o)[2]=((v)[2]*(1-(s)))+((b)[2]*(s)),(o)[3]=((v)[3]*(1-(s)))+((b)[3]*(s)))

/*
==============
CG_HorizontalPercentBar
	Generic routine for pretty much all status indicators that show a fractional
	value to the palyer by virtue of how full a drawn box is.

flags:
	left		- 1
	center		- 2		// direction is 'right' by default and orientation is 'horizontal'
	vert		- 4
	nohudalpha	- 8		// don't adjust bar's alpha value by the cg_hudalpha value
	bg			- 16	// background contrast box (bg set with bgColor of 'NULL' means use default bg color (1,1,1,0.25)
	spacing		- 32	// some bars use different sorts of spacing when drawing both an inner and outer box

	lerp color	- 256	// use an average of the start and end colors to set the fill color
==============
*/


// TODO: these flags will be shared, but it was easier to work on stuff if I wasn't changing header files a lot
#define BAR_LEFT		0x0001
#define BAR_CENTER		0x0002
#define BAR_VERT		0x0004
#define BAR_NOHUDALPHA	0x0008
#define BAR_BG			0x0010
// different spacing modes for use w/ BAR_BG
#define BAR_BGSPACING_X0Y5	0x0020
#define BAR_BGSPACING_X0Y0	0x0040

#define BAR_LERP_COLOR	0x0100

#define BAR_BORDERSIZE 2

void CG_FilledBar(float x, float y, float w, float h, float *startColor, float *endColor, const float *bgColor, float frac, int flags) {
	vec4_t	backgroundcolor = {1, 1, 1, 0.25f}, colorAtPos;	// colorAtPos is the lerped color if necessary
	int indent = BAR_BORDERSIZE;

	if( frac > 1 ) {
		frac = 1.f;
	}
	if( frac < 0 ) {
		frac = 0;
	}

	if((flags&BAR_BG) && bgColor) {	// BAR_BG set, and color specified, use specified bg color
		Vector4Copy(bgColor, backgroundcolor);
	}

	if(flags&BAR_LERP_COLOR) {
		Vector4Average(startColor, endColor, frac, colorAtPos);
	}

	// background
	if((flags&BAR_BG)) {
		// draw background at full size and shrink the remaining box to fit inside with a border.  (alternate border may be specified by a BAR_BGSPACING_xx)
		CG_FillRect (	x,
						y,
						w,
						h,
						backgroundcolor );

		if(flags&BAR_BGSPACING_X0Y0) {			// fill the whole box (no border)

		} else if(flags&BAR_BGSPACING_X0Y5) {	// spacing created for weapon heat
			indent*=3;
			y+=indent;
			h-=(2*indent);

		} else {								// default spacing of 2 units on each side
			x+=indent;
			y+=indent;
			w-=(2*indent);
			h-=(2*indent);
		}
	}


	// adjust for horiz/vertical and draw the fractional box
	if(flags&BAR_VERT) {
		if(flags&BAR_LEFT) {	// TODO: remember to swap colors on the ends here
			y+=(h*(1-frac));
		} else if (flags&BAR_CENTER) {
			y+=(h*(1-frac)/2);
		}

		if(flags&BAR_LERP_COLOR) {
			CG_FillRect ( x, y, w, h * frac, colorAtPos );
		} else {
//			CG_FillRectGradient ( x, y, w, h * frac, startColor, endColor, 0 );
			CG_FillRect ( x, y, w, h * frac, startColor );
		}

	} else {

		if(flags&BAR_LEFT) {	// TODO: remember to swap colors on the ends here
			x+=(w*(1-frac));
		} else if (flags&BAR_CENTER) {
			x+=(w*(1-frac)/2);
		}

		if(flags&BAR_LERP_COLOR) {
			CG_FillRect ( x, y, w*frac, h, colorAtPos );
		} else {
//			CG_FillRectGradient ( x, y, w * frac, h, startColor, endColor, 0 );
			CG_FillRect ( x, y, w*frac, h, startColor );
		}
	}

}

void CG_AdjustFrom640( float *x, float *y, float *w, float *h ) {
#if 0
	// adjust for wide screens
	if ( cgs.glconfig.vidWidth * 480 > cgs.glconfig.vidHeight * 640 ) {
		*x += 0.5 * ( cgs.glconfig.vidWidth - ( cgs.glconfig.vidHeight * 640 / 480 ) );
	}
#endif

/*	if ( (cg.showGameView) && cg.refdef_current->width ) {
		float xscale = ( ( cg.refdef_current->width / cgs.screenXScale ) / 640.f );
		float yscale = ( ( cg.refdef_current->height / cgs.screenYScale ) / 480.f );

		(*x) = (*x) * xscale + ( cg.refdef_current->x / cgs.screenXScale );
		(*y) = (*y) * yscale + ( cg.refdef_current->y / cgs.screenYScale );
		(*w) *= xscale;
		(*h) *= yscale;
	}*/

	// scale for screen sizes
	*x *= cgs.screenXScale;
	*y *= cgs.screenYScale;
	*w *= cgs.screenXScale;
	*h *= cgs.screenYScale;
}

/*
================
CG_DrawSides

Coords are virtual 640x480
================
*/
void CG_DrawSides2( float x, float y, float w, float h, float size ) {
	CG_AdjustFrom640( &x, &y, &w, &h );
	size *= cgs.screenXScale;
	trap->R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
	trap->R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
}

void CG_DrawTopBottom2( float x, float y, float w, float h, float size ) {
	CG_AdjustFrom640( &x, &y, &w, &h );
	size *= cgs.screenYScale;
	trap->R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
	trap->R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
}

void CG_DrawSides_NoScale( float x, float y, float w, float h, float size ) {
	CG_AdjustFrom640( &x, &y, &w, &h );
	trap->R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
	trap->R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
}

void CG_DrawTopBottom_NoScale( float x, float y, float w, float h, float size ) {
	CG_AdjustFrom640( &x, &y, &w, &h );
	trap->R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
	trap->R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
}

// CHRUKER: b076 - Scoreboard background had black lines drawn twice
void CG_DrawBottom_NoScale( float x, float y, float w, float h, float size ) {
	CG_AdjustFrom640( &x, &y, &w, &h );
	trap->R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
}

void CG_DrawRect_FixedBorder( float x, float y, float width, float height, int border, const float *color ) {
	trap->R_SetColor( color );

	CG_DrawTopBottom_NoScale( x, y, width, height, border );
	CG_DrawSides_NoScale( x, y, width, height, border );

	trap->R_SetColor( NULL );
}

void AIMod_AutoWaypoint_DrawProgress ( void )
{
	int				flags = 64|128;
	float			frac;
	rectDef_t		rect;
	//int				total_seconds_left = 0;
	int				seconds_left = 0;
	int				minutes_left = 0;
	clock_t			time_taken;
	clock_t			total_seconds_left;
	//qboolean		estimating = qfalse;
	clock_t			current_time;

	if (aw_percent_complete == 0.0f || aw_percent_complete < aw_last_percent || aw_stage_start_time == 0)
	{// Init timer...
		//aw_stage_start_time = trap->Milliseconds();
		//aw_stage_start_time = clock();
		aw_stage_start_time = clock();
		aw_last_percent_time = aw_stage_start_time;
		aw_last_percent = 0;
		aw_num_last_times = 0;
	}

	if (aw_percent_complete > 100.0f)
	{// UQ: It can happen... Somehow... LOL!
		aw_percent_complete = 100.0f;
		aw_last_percent = 0;
		aw_num_last_times = 0;
	}

	current_time = clock();

	aw_last_percent = aw_percent_complete;

	time_taken = current_time - aw_stage_start_time;

	{
		float perc_done = aw_percent_complete / 100.0;
		float perc_left = 1 - perc_done;
		time_t TimePerPerc = time_taken / perc_done;
		time_t TotalTime = TimePerPerc * 100;
		time_t TakenTime = TimePerPerc * perc_done;
		time_t LeftTime = TimePerPerc * perc_left;
		time_t TimeRemaining = LeftTime;

		//trap->Print("Ptime: %i. Total: %i. Taken: %i. Left: %i. Remaining: %i.\n", (int)(TimePerPerc), (int)(TotalTime), (int)(TakenTime), (int)(LeftTime), (int)(TimeRemaining));

		total_seconds_left = TimeRemaining / 1000;
	}

	minutes_left = total_seconds_left/60;
	seconds_left = total_seconds_left-(minutes_left*60);

	// Draw the bar!
	frac = (float)((aw_percent_complete)*0.01);

	rect.w = 500;
	rect.h = 30;

	rect.x = 69;
	rect.y = 369;

	// draw the background, then filled bar, then border
	CG_FillRect( rect.x, rect.y, rect.w, rect.h, popBG );
	CG_FilledBar( rect.x, rect.y, rect.w, rect.h, popRed, NULL, NULL, frac, flags );
	CG_DrawRect_FixedBorder( rect.x, rect.y, rect.w, rect.h, POP_HUD_BORDERSIZE, popBorder );

	CG_Text_Paint((rect.x), (rect.y + (rect.h*0.5) - 120), 0.5f, colorWhite, va("^3AUTO-WAYPOINTING^5 - Please wait..."), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_MEDIUM );
	CG_Text_Paint((rect.x), (rect.y + (rect.h*0.5) - 80), 0.5f, colorWhite, task_string1, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_MEDIUM );
	CG_Text_Paint((rect.x), (rect.y + (rect.h*0.5) - 60), 0.5f, colorWhite, task_string2, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_MEDIUM );
	CG_Text_Paint((rect.x), (rect.y + (rect.h*0.5) - 40), 0.5f, colorWhite, task_string3, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_MEDIUM );
	CG_Text_Paint((rect.x + (rect.w*0.5) - 35), (rect.y + (rect.h*0.5) - 18/*+ 8*/), 1.0f, colorWhite, va("^7%.2f%%", aw_percent_complete), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_LARGE );

	if (aw_percent_complete > 2.0f)
		CG_Text_Paint((rect.x + 160), (rect.y + (rect.h*0.5) + 16), 0.5f, colorWhite, va("^3%i ^5minutes ^3%i ^5seconds remaining (estimated)", minutes_left, seconds_left), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_SMALL );
	else
		CG_Text_Paint((rect.x + 160), (rect.y + (rect.h*0.5) + 16), 0.5f, colorWhite, va("^5    ... estimating time remaining ..."), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_SMALL );

	CG_Text_Paint((rect.x + 100), (rect.y + (rect.h*0.5) + 30), 0.5f, colorWhite, last_node_added_string, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_MEDIUM );

	trap->R_SetColor( NULL );
}

void AWP_UpdatePercentBar(float percent, char *text, char *text2, char *text3)
{
	aw_percent_complete = percent;

	if (text[0] != '\0') trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^7%s\n", text);
	strcpy(task_string1, va("^5%s", text));
	strcpy(task_string2, va("^5%s", text2));
	strcpy(task_string3, va("^5%s", text3));
	trap->UpdateScreen();
}

void AWP_UpdatePercentBar2(float percent, char *text, char *text2, char *text3)
{
	aw_percent_complete = percent;
	strcpy(task_string1, va("^5%s", text));
	strcpy(task_string2, va("^5%s", text2));
	strcpy(task_string3, va("^5%s", text3));
	trap->UpdateScreen();
}

void AWP_UpdatePercentBarOnly(float percent)
{
	aw_percent_complete = percent;
	trap->UpdateScreen();
}


qboolean AW_Map_Has_Waypoints ( void )
{
	fileHandle_t	f;
	char			filename[60];
	int				len = 0;

	strcpy( filename, "nodes/" );

	////////////////////
	/*trap->Cvar_VariableStringBuffer( "g_scriptName", filename, sizeof(filename) );
	if ( strlen( filename) > 0 )
	{
		trap->Cvar_Register( &mapname, "g_scriptName", "", CVAR_ROM );
	}
	else
	{
		trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
	}*/

	Q_strcat( filename, sizeof(filename), cgs.currentmapname );

	///////////////////
	//open the node file for reading, return false on error
	len = trap->FS_Open( va( "nodes/%s.bwp", filename), &f, FS_READ );
	trap->FS_Close(f);

	if( len <= 0 )
	{
		FILE *file;
		file = fopen( va( "%s/nodes/%s.bwp", MOD_DIRECTORY, filename), "r" );

		if ( !file )
		{
			return qfalse;
		}

		fclose(file);
	}

	return qtrue;
}


qboolean	MOVER_LIST_GENERATED = qfalse;
vec3_t		MOVER_LIST[1024];
vec3_t		MOVER_LIST_TOP[1024];
int			MOVER_LIST_NUM = 0;

void GenerateMoverList ( void )
{
	int i = 0;

	if (MOVER_LIST_GENERATED) return;

	//trap->Print("Generating mover list.\n");

	for (i = 0; i < MAX_GENTITIES; i++)
	{
		centity_t *cent = &cg_entities[i];
		vec3_t mover_org;
		trace_t tr;
		vec3_t	newOrgDown;

		if (!cent) continue;
		if (cent->currentState.eType != ET_MOVER_MARKER) continue;

		VectorCopy(cent->currentState.origin, mover_org);
		mover_org[2] += 128.0; // Look from up a little...

		VectorCopy( mover_org, newOrgDown );
		newOrgDown[2] = -64000.0f;

		CG_Trace( &tr, mover_org, NULL, NULL, newOrgDown, cg.clientNum, MASK_PLAYERSOLID );

		if ( tr.fraction < 1 )
		{
			VectorCopy( tr.endpos, mover_org );
			mover_org[2]+=16.0;
			VectorCopy(mover_org, MOVER_LIST[MOVER_LIST_NUM]);
			VectorCopy(cent->currentState.origin2, MOVER_LIST_TOP[MOVER_LIST_NUM]);
			MOVER_LIST_TOP[MOVER_LIST_NUM][2]+=128.0;
			MOVER_LIST_NUM++;
			//trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Mover found at %f %f %f.\n", mover_org[0], mover_org[1], mover_org[2]);
		}
	}

	//trap->Print("There are %i movers.\n", MOVER_LIST_NUM);

	MOVER_LIST_GENERATED = qtrue;
}

qboolean NearMoverEntityLocation( vec3_t org )
{
	int i = 0;

#pragma omp critical (__GENERATE_MOVER_LIST__)
	{
		GenerateMoverList(); // init the mover list on first check...
	}

	for (i = 0; i < MOVER_LIST_NUM; i++)
	{
		if (DistanceHorizontal(org, MOVER_LIST[i]) >= 48.0/*54.0*//*68.0*/) continue;

		return qtrue;
	}

	return qfalse;
}

qboolean
NodeIsOnMover ( vec3_t org1 )
{
	/*
	trace_t tr;
	vec3_t	newOrg, newOrgDown;

	VectorCopy( org1, newOrg );

	newOrg[2] += 8; // Look from up a little...

	VectorCopy( org1, newOrgDown );
	newOrgDown[2] = -64000.0f;

	CG_Trace( &tr, newOrg, NULL, NULL, newOrgDown, cg.clientNum, MASK_PLAYERSOLID );

	if ( tr.fraction != 1
		&& tr.entityNum != ENTITYNUM_NONE
		&& tr.entityNum < ENTITYNUM_MAX_NORMAL )
	{
		if (cg_entities[tr.entityNum].currentState.eType == ET_MOVER)
			return ( qtrue );
	}
	*/
	if (NearMoverEntityLocation(org1)) return qtrue;

	/*
	if ( tr.fraction == 1 )
	{
		return ( qfalse );
	}
	*/

	return ( qfalse );
}

/* */
qboolean
BAD_WP_Height ( vec3_t start, vec3_t end )
{
	/*float distance = DistanceHorizontal(start, end);
	float height_diff = DistanceVertical(start, end);

	if (distance > 8)
	{// < 8 is probebly a ladder...
		if ((start[2] + 32 < end[2])
			&& (height_diff*1.5) > distance
			&& distance > 48)
			return qtrue;

		if ((start[2] < end[2] + 48)
			&& (height_diff*1.5) > distance
			&& distance > 48)
			return qtrue;
	}*/

	if (DistanceVertical(start, end) > 64 && start[2] < end[2])
	{
		if (!NodeIsOnMover(start))
			return qtrue;
	}

	return ( qfalse );
}

/* */
qboolean
BAD_WP_Distance ( vec3_t start, vec3_t end, qboolean double_distance )
{
	qboolean hitsmover = qfalse;
	float distance = VectorDistance( start, end );
	float height_diff = DistanceVertical(start, end);
	float length_diff = DistanceHorizontal(start, end);
	float double_mod = 1.0;

	if (double_distance) double_mod = 2.0;

	if (distance > waypoint_scatter_distance * waypoint_distance_multiplier * double_mod
		|| (double_distance && distance <= waypoint_scatter_distance * waypoint_distance_multiplier) )
	{
		return ( qtrue );
	}

	if (!DO_FAST_LINK)
	{
		if (NodeIsOnMover(start))
		{
			// Too far, even for mover node...
			hitsmover = qtrue;
		}

		if (!hitsmover && length_diff * 0.8 < height_diff)
		{
			// This slope looks too sharp...
			return ( qtrue );
		}
	}

	// Looks good...
	return ( qfalse );
}

qboolean HasPortalFlags ( int surfaceFlags, int contents )
{
	if ( ( (surfaceFlags & SURF_NOMARKS)
		&& (surfaceFlags & SURF_NOIMPACT)
		&& (surfaceFlags & SURF_NODRAW)
#ifndef __DISABLE_PLAYERCLIP__
		&& (contents & CONTENTS_PLAYERCLIP)
#endif //__DISABLE_PLAYERCLIP__
		&& (contents & CONTENTS_TRANSLUCENT) ) )
		return qtrue;

	return qfalse;
}

qboolean VisibleAllowEntType ( int type, int flags )
{
	switch (type)
	{
	case ET_GENERAL:
	case ET_PLAYER:
	case ET_ITEM:
	//case ET_MISSILE:
	//case ET_SPECIAL:				// rww - force fields
	case ET_HOLOCRON:			// rww - holocron icon displays
	case ET_BEAM:
	case ET_PORTAL:
	case ET_SPEAKER:
	//case ET_PUSH_TRIGGER:
	//case ET_TELEPORT_TRIGGER:
	case ET_INVISIBLE:
	case ET_NPC:					// ghoul2 player-like entity
	case ET_TEAM:
	case ET_BODY:
	//case ET_TERRAIN:
	//case ET_FX:
#ifdef __DOMINANCE__
	case ET_FLAG:
#endif //__DOMINANCE__
		return ( qtrue );
	case ET_MOVER:
		//if (flags & EF_JETPACK_ACTIVE)
			return ( qtrue );
	default:
		break;
	}

	return ( qfalse );
}

qboolean AIMod_AutoWaypoint_Check_PlayerWidth ( vec3_t origin )
{
#if 0
	trace_t		trace;
	vec3_t		org, destorg;

	//Offset the step height
	//vec3_t	mins = {-18, -18, 0};
	//vec3_t	maxs = {18, 18, 48};
	//vec3_t	mins = {-48, -48, 0};
	//vec3_t	maxs = {48, 48, 48};
	//vec3_t	mins = {-24, -24, 0};
	//vec3_t	maxs = {24, 24, 48};

	vec3_t	mins = {-26, -26, 0};
	vec3_t	maxs = {26, 26, 48};

	VectorCopy(origin, org);
	org[2]+=18;
	VectorCopy(origin, destorg);
	destorg[2]+=18;

	CG_Trace( &trace, org, mins, maxs, destorg, -1, MASK_PLAYERSOLID );

	//Do a simple check
	if ( trace.allsolid || trace.startsolid )
	{//inside solid
		return qfalse;
	}

	//if ( trace.contents == 0 && trace.surfaceFlags == 0 )
	//{// Dont know what to do about these... Gonna try disabling them and see what happens...
	//	return qfalse;
	//}

	if ( trace.contents == CONTENTS_WATER || trace.contents == CONTENTS_LAVA )
	{
		return qfalse;
	}

	return qtrue;
#else
	vec3_t		org;
	VectorCopy(origin, org);
	org[2]+=18;
	if (WP_CheckInSolid(org)) return qfalse;
	return qtrue;
#endif
}

float FloorHeightAt ( vec3_t org ); // below

int CheckHeightsBetween( vec3_t from, vec3_t dest )
{
	vec3_t	from_point, to_point, current_point;
	vec3_t	dir, forward;
	float	distance;

	VectorCopy(from, from_point);
	from_point[2]+=32;
	VectorCopy(dest, to_point);
	to_point[2]+=32;

	distance = VectorDistance(from_point, to_point);

	VectorSubtract(to_point, from_point, dir);
	vectoangles(dir, dir);

	AngleVectors( from_point, forward, NULL, NULL );

	VectorCopy(from_point, current_point);

	while (distance > -15)
	{
		float floor = 0;

		VectorMA( current_point, 16, forward, current_point );

		floor = FloorHeightAt(current_point);

		if (floor > 65000.0f || floor < -65000.0f || floor < from_point[2] - 128.0f)
		{// Found a bad drop!
			return 0;
		}

		distance -= 16.0f; // Subtract this move from the distance...
	}

	//we made it!
	return 1;
}

int NodeVisible_WithExtraHeightAtTraceEnd( vec3_t from, vec3_t dest, int ignore )
{
	trace_t		trace;
	vec3_t		org, destorg;
	int			j = 0;

	//Offset the step height
	//vec3_t	mins = {-18, -18, -24};
	vec3_t	mins = {-8, -8, -6};
	//vec3_t	maxs = {18, 18, 48};
	vec3_t	maxs = {8, 8, 48-STEPSIZE};

	VectorCopy(from, org);
	org[2]+=STEPSIZE;

	VectorCopy(dest, destorg);
	destorg[2]+=STEPSIZE;

	CG_Trace( &trace, org, mins, maxs, destorg, ignore, MASK_PLAYERSOLID );

	//Do a simple check
	if ( trace.allsolid || trace.startsolid )
	{//inside solid
		return 0;
	}

	if ( trace.fraction < 1.0f )
	{//hit something
		if (trace.entityNum != ENTITYNUM_NONE
			&& trace.entityNum < ENTITYNUM_MAX_NORMAL )
		{
			if (VisibleAllowEntType(cg_entities[trace.entityNum].currentState.eType, cg_entities[trace.entityNum].currentState.eFlags))
				if (CheckHeightsBetween(from, dest) != 0)
					return ( 1 );
		}

		return 0;
	}

	// Doors... Meh!
	for (j = MAX_CLIENTS; j < MAX_GENTITIES; j++)
	{
		centity_t *ent = &cg_entities[j];

		if (!ent) continue;

		// Too far???
		if (Distance(from, ent->currentState.pos.trBase) > 64/*waypoint_scatter_distance*waypoint_distance_multiplier*/)
			continue;

		if (ent->currentState.eType == ET_GENERAL || ent->currentState.eType == ET_SPEAKER)
		{
			if (CheckHeightsBetween(from, dest) != 0)
				return ( 2 ); // Doors???
		}
	}

	// Check heights along this route for falls...
	if (CheckHeightsBetween(from, dest) == 0)
		return 0; // Bad height (drop) in between these points...

	//we made it!
	return 1;
}

//0 = wall in way
//1 = player or no obstruction
//2 = useable door in the way.
//3 = team door entity in the way.
int NodeVisible( vec3_t from, vec3_t dest, int ignore )
{
	trace_t		trace;
	vec3_t		org, destorg;
	int			j = 0;

	//Offset the step height
	//vec3_t	mins = {-18, -18, -24};
	//vec3_t	maxs = {18, 18, 48};

	//vec3_t	mins = {-10, -10, 0};
	//vec3_t	maxs = {10, 10, 48-STEPSIZE};

	vec3_t	mins = {-18, -18, -8};
	vec3_t	maxs = {18, 18, 48-STEPSIZE};

	VectorCopy(from, org);
	org[2]+=STEPSIZE;

	VectorCopy(dest, destorg);
	destorg[2]+=STEPSIZE;

	CG_Trace( &trace, org, mins, maxs, destorg, ignore, MASK_PLAYERSOLID );

	//Do a simple check
	if ( trace.allsolid || trace.startsolid )
	{//inside solid
		return 0;
	}

	if ( trace.fraction < 1.0f )
	{//hit something
		if (trace.entityNum != ENTITYNUM_NONE
			&& trace.entityNum < ENTITYNUM_MAX_NORMAL )
		{
			if (VisibleAllowEntType(cg_entities[trace.entityNum].currentState.eType, cg_entities[trace.entityNum].currentState.eFlags))
				//if (CheckHeightsBetween(from, dest) != 0)
					return ( 1 );
		}

		// Instead of simply failing, first check if looking from above the trace end a little would see over a bump (steps)...
		//if (NodeVisible_WithExtraHeightAtTraceEnd( trace.endpos, dest, ignore ) == 0)
			return 0;
	}

	// Doors... Meh!
	for (j = MAX_CLIENTS; j < MAX_GENTITIES; j++)
	{
		centity_t *ent = &cg_entities[j];

		if (!ent) continue;

		// Too far???
		if (Distance(from, ent->currentState.pos.trBase) > 64/*waypoint_scatter_distance*waypoint_distance_multiplier*/)
			continue;

		if (ent->currentState.eType == ET_GENERAL || ent->currentState.eType == ET_SPEAKER)
		{
			//if (CheckHeightsBetween(from, dest) != 0)
				return ( 2 ); // Doors???
		}
	}

	// Check heights along this route for falls...
	//if (CheckHeightsBetween(from, dest) == 0)
	//	return 0; // Bad height (drop) in between these points...

	//we made it!
	return 1;
}

//0 = wall in way
//1 = player or no obstruction
//2 = useable door in the way.
//3 = team door entity in the way.
int
NodeVisible_OLD ( vec3_t org1, vec3_t org2, int ignore )
{
	trace_t tr;
	vec3_t	newOrg, newOrg2;
	vec3_t	mins, maxs;

//	vec3_t			traceMins = { -10, -10, -1 };
//	vec3_t			traceMaxs = { 10, 10, 48/*32*/ };
	vec3_t			traceMins = { -15, -15, 0 };
	vec3_t			traceMaxs = { 15, 15, 32 };

	VectorCopy( traceMins, mins );
	VectorCopy( traceMaxs, maxs );
	VectorCopy( org2, newOrg );
	VectorCopy( org1, newOrg2 );

	newOrg[2] += 16/*8*/; // Look from up a little...
	newOrg2[2] += 16/*8*/; // Look from up a little...

	CG_Trace( &tr, newOrg2, NULL/*mins*/, NULL/*maxs*/, newOrg, ignore, MASK_PLAYERSOLID );

	if ( tr.startsolid )
	{
		//trap->Print("START SOLID!\n");
		return ( 0 );
	}

	if ( tr.fraction == 1 /*&& !(tr.contents & CONTENTS_LAVA)*/ /*|| HasPortalFlags(tr.surfaceFlags, tr.contents)*/ )
	{
		return ( 1 );
	}

	if ( tr.fraction != 1
		&& tr.entityNum != ENTITYNUM_NONE
		&& tr.entityNum < ENTITYNUM_MAX_NORMAL )
	{
		if (VisibleAllowEntType(cg_entities[tr.entityNum].currentState.eType, cg_entities[tr.entityNum].currentState.eFlags))
			return ( 1 );
	}

	// Search for door triggers...
	//if ( tr.fraction != 1 )
	{
		//if ((tr.surfaceFlags & SURF_NOMARKS) && (tr.surfaceFlags & SURF_NODRAW) && (tr.contents & CONTENTS_SOLID) && (tr.contents & CONTENTS_OPAQUE))
		{
			int j = 0;

			for (j = MAX_CLIENTS; j < MAX_GENTITIES; j++)
			{
				centity_t *ent = &cg_entities[j];

				if (!ent) continue;

				// Too far???
				if (Distance(org1, ent->currentState.pos.trBase) > waypoint_scatter_distance*waypoint_distance_multiplier
					|| Distance(org2, ent->currentState.pos.trBase) > waypoint_scatter_distance*waypoint_distance_multiplier)
					continue;

				if (ent->currentState.eType == ET_GENERAL || ent->currentState.eType == ET_SPEAKER)
				{
					return ( 2 ); // Doors???
				}
			}
		}
	}

	//trap->Print("NO VIS!\n");
	return ( 0 );
}

int
NodeVisibleJump ( vec3_t org1, vec3_t org2, int ignore )
{
	trace_t tr;
	vec3_t	newOrg, newOrg2;
	vec3_t	mins, maxs;

	//vec3_t			botTraceMins = { -20, -20, -1 };
	//vec3_t			botTraceMaxs = { 20, 20, 32 };
	//vec3_t			traceMins = { -20, -20, -1 };
	//vec3_t			traceMaxs = { 20, 20, 32 };

//	vec3_t			traceMins = { -10, -10, -1 };
//	vec3_t			traceMaxs = { 10, 10, 48/*32*/ };
	vec3_t			traceMins = { -15, -15, 0 };
	vec3_t			traceMaxs = { 15, 15, 64 };

	//vec3_t	traceMins = { -10, -10, -1 };
	//vec3_t	traceMaxs = { 10, 10, 15 };

	VectorCopy( traceMins, mins );
	VectorCopy( traceMaxs, maxs );
	VectorCopy( org1, newOrg );
	VectorCopy( org1, newOrg2 );

	// UQ1: First check the up position is reachable (for inward sloped walls)
	newOrg[2] += 2; // Look from up a little...
	newOrg2[2] += 18; // Look from up a little...

	CG_Trace( &tr, newOrg2, mins, maxs, newOrg, ignore, MASK_PLAYERSOLID );

	if ( tr.startsolid || tr.fraction != 1.0f )
	{
		return ( 0 );
	}

	VectorCopy( org2, newOrg );
	VectorCopy( org2, newOrg2 );

	newOrg[2] += 2; // Look from up a little...
	newOrg2[2] += 18; // Look from up a little...

	CG_Trace( &tr, newOrg2, mins, maxs, newOrg, ignore, MASK_PLAYERSOLID );

	if ( tr.startsolid || tr.fraction != 1.0f )
	{
		return ( 0 );
	}

	// Init the variables for the actual (real) vis check...
	VectorCopy( org2, newOrg );
	VectorCopy( org1, newOrg2 );

	newOrg[2] += 16; // Look from up a little...
	newOrg2[2] += 16; // Look from up a little...

	//CG_Trace(&tr, newOrg, mins, maxs, org2, ignore, MASK_SHOT);
	CG_Trace( &tr, newOrg2, mins, maxs, newOrg, ignore, MASK_PLAYERSOLID );

	if ( tr.startsolid )
	{
		return ( 0 );
	}

	if ( tr.fraction == 1 /*&& !(tr.contents & CONTENTS_LAVA)*/ /*|| HasPortalFlags(tr.surfaceFlags, tr.contents)*/ )
	{
		return ( 1 );
	}

	if ( tr.fraction != 1
		&& tr.entityNum != ENTITYNUM_NONE
		&& tr.entityNum < ENTITYNUM_MAX_NORMAL )
	{
		if (VisibleAllowEntType(cg_entities[tr.entityNum].currentState.eType, cg_entities[tr.entityNum].currentState.eFlags))
			return ( 1 );
	}

	return ( 0 );
}

int
NodeVisibleCrouch ( vec3_t org1, vec3_t org2, int ignore )
{
	trace_t tr;
	vec3_t	newOrg, newOrg2;
	vec3_t	mins, maxs;

	//vec3_t			botTraceMins = { -20, -20, -1 };
	//vec3_t			botTraceMaxs = { 20, 20, 32 };
	//vec3_t			traceMins = { -20, -20, -1 };
	//vec3_t			traceMaxs = { 20, 20, 32 };

//	vec3_t			traceMins = { -10, -10, -1 };
//	vec3_t			traceMaxs = { 10, 10, 32/*16*/ };
	vec3_t			traceMins = { -15, -15, 0 };
	vec3_t			traceMaxs = { 15, 15, 64 };

	//vec3_t	traceMins = { -10, -10, -1 };
	//vec3_t	traceMaxs = { 10, 10, 15 };

	VectorCopy( traceMins, mins );
	VectorCopy( traceMaxs, maxs );
	VectorCopy( org2, newOrg );
	VectorCopy( org1, newOrg2 );

	newOrg[2] += 1; // Look from up a little...
	newOrg2[2] += 1; // Look from up a little...

	//CG_Trace(&tr, newOrg, mins, maxs, org2, ignore, MASK_SHOT);
	CG_Trace( &tr, newOrg2, mins, maxs, newOrg, ignore, MASK_PLAYERSOLID );

	if ( tr.startsolid )
	{
		return ( 0 );
	}

	if ( tr.fraction == 1 /*&& !(tr.contents & CONTENTS_LAVA)*/ /*|| HasPortalFlags(tr.surfaceFlags, tr.contents)*/ )
	{
		return ( 1 );
	}

/*	if (tr.surfaceFlags & SURF_LADDER)
	{
		return ( 1 );
	}
*/
	if ( tr.fraction != 1
		&& tr.entityNum != ENTITYNUM_NONE
		&& tr.entityNum < ENTITYNUM_MAX_NORMAL )
	{
		if (VisibleAllowEntType(cg_entities[tr.entityNum].currentState.eType, cg_entities[tr.entityNum].currentState.eFlags))
			return ( 1 );
	}

	return ( 0 );
}

//extern int OrgVisibleBox(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, int ignore);
//special node visibility check for bot linkages..
//0 = wall in way
//1 = player or no obstruction
//2 = useable door in the way.

//3 = team door entity in the way.
int
TankNodeVisible ( vec3_t org1, vec3_t org2, vec3_t mins, vec3_t maxs, int ignore )
{
	trace_t tr;
	vec3_t	newOrg, newOrg2;
	VectorCopy( org1, newOrg2 );
	VectorCopy( org2, newOrg );

	//newOrg[2] += 4;		// Look from up a little...
	//newOrg2[2] += 4;	// Look from up a little...
	newOrg[2] += 32;		// Look from up a little...
	newOrg2[2] += 32;	// Look from up a little...

	CG_Trace( &tr, newOrg2, mins, maxs, newOrg, ignore, MASK_PLAYERSOLID );

	if ( tr.startsolid )
	{
		return ( 0 );
	}

	if ( tr.fraction == 1 /*&& !(tr.contents & CONTENTS_LAVA)*/ || HasPortalFlags(tr.surfaceFlags, tr.contents) )
	{
		return ( 1 );
	}

	/*if ( tr.fraction != 1
		&& tr.entityNum != ENTITYNUM_NONE
		&& tr.entityNum < ENTITYNUM_MAX_NORMAL )
	{
		switch (cg_entities[tr.entityNum].currentState.eType)
		{
		case ET_MOVER:
		case ET_ITEM:
		case ET_PORTAL:
//		case ET_PUSH_TRIGGER:
		case ET_TELEPORT_TRIGGER:
		case ET_INVISIBLE:
		case ET_OID_TRIGGER:
		case ET_EXPLOSIVE_INDICATOR:
		case ET_EXPLOSIVE:
//		case ET_EF_SPOTLIGHT:
//		case ET_ALARMBOX:
		case ET_MOVERSCALED:
		case ET_CONSTRUCTIBLE_INDICATOR:
		case ET_CONSTRUCTIBLE:
		case ET_CONSTRUCTIBLE_MARKER:
		case ET_WAYPOINT:
		case ET_TANK_INDICATOR:
		case ET_TANK_INDICATOR_DEAD:
		case ET_BOTGOAL_INDICATOR:
		case ET_CORPSE:
		case ET_SMOKER:
		case ET_TRIGGER_MULTIPLE:
		case ET_TRIGGER_FLAGONLY:
		case ET_TRIGGER_FLAGONLY_MULTIPLE:
//		case ET_CABINET_H:
//		case ET_CABINET_A:
		case ET_HEALER:
		case ET_SUPPLIER:
		case ET_COMMANDMAP_MARKER:
		case ET_WOLF_OBJECTIVE:
		case ET_SECONDARY:
		case ET_FLAG:
		case ET_ASSOCIATED_SPAWNAREA:
		case ET_UNASSOCIATED_SPAWNAREA:
//		case ET_AMMO_CRATE:
//		case ET_HEALTH_CRATE:
		case ET_CONSTRUCTIBLE_SANDBAGS:
		case ET_VEHICLE:
		case ET_PARTICLE_SYSTEM:
			return ( 1 );
		default:
			break;
		}
	}*/

	return ( 0 );
}

int			num_cover_spots = 0;
int			cover_nodes[MAX_NODES];

//standard visibility check
int
OrgVisible ( vec3_t org1, vec3_t org2, int ignore )
{
	trace_t tr;
	CG_Trace( &tr, org1, NULL, NULL, org2, ignore, MASK_SOLID | MASK_OPAQUE /*| MASK_WATER*/ );
	if ( tr.fraction == 1 )
	{
		return ( 1 );
	}

	return ( 0 );
}

#ifdef __COVER_SPOTS__
void
AIMOD_SaveCoverPoints ( void )
{
	fileHandle_t	f;
	int				i;

	vmCvar_t mapname;
	trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

	trap->Print( "^3*** ^3%s: ^7Saving cover point table.\n", "AUTO-WAYPOINTER" );

	///////////////////
	//try to open the output file, return if it failed
	trap->FS_Open( va( "nodes/%s.cpw", cgs.currentmapname), &f, FS_WRITE );
	if ( !f )
	{
		trap->Print( "^1*** ^3ERROR^5: Error opening cover point file ^7/nodes/%s.cpw^5!!!\n", "AUTO-WAYPOINTER", cgs.currentmapname);
		return;
	}

	trap->FS_Write( &number_of_nodes, sizeof(int), f );							//write the number of nodes in the map
									//write the map name
	trap->FS_Write( &num_cover_spots, sizeof(int), f );							//write the number of nodes in the map

	for ( i = 0; i < num_cover_spots; i++ )											//loop through all the nodes
	{
		int j = 0;

		trap->FS_Write( &(cover_nodes[i]), sizeof(int), f );

		/*
		// UQ1: Now write the spots this is a coverpoint for...
		trap->FS_Write( &(nodes[cover_nodes[i]].coverpointNum), sizeof(int), f );

		// And then save each one...
		for ( j = 0; j < nodes[cover_nodes[i]].coverpointNum; j++)
		{
			trap->FS_Write( &(nodes[cover_nodes[i]].coverpointFor[j]), sizeof(int), f );
		}
		*/
	}

	trap->FS_Close( f );

	trap->Print( "^3*** ^3%s: ^5Cover point table saved to file ^7/nodes/%s.cpw^5.\n", "AUTO-WAYPOINTER", cgs.currentmapname);
}

qboolean
AIMOD_LoadCoverPoints2 ( void )
{
	FILE			*f;
	vmCvar_t		fs_homepath, fs_game;
	int				i = 0;
	int				num_map_waypoints = 0;

	vmCvar_t mapname;
	trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

	// Init...
	num_cover_spots = 0;

	trap->Cvar_Register( &fs_homepath, "fs_homepath", "", CVAR_SERVERINFO | CVAR_ROM );
	trap->Cvar_Register( &fs_game, "fs_game", "", CVAR_SERVERINFO | CVAR_ROM );

	f = fopen( va("%s/%s/nodes/%s.cpw", fs_homepath.string, fs_game.string, cgs.currentmapname), "rb" );

	if ( !f )
	{
		trap->Print( "^1*** ^3%s^5: Reading coverpoints from ^7/nodes/%s.cpw^3 failed^5!!!\n", "AUTO-WAYPOINTER", cgs.currentmapname);
		return qfalse;
	}

	fread( &num_map_waypoints, sizeof(int), 1, f );

	if (num_map_waypoints != number_of_nodes)
	{// Is an old file! We need to make a new one!
		trap->Print( "^1*** ^3%s^5: Reading coverpoints from ^7/nodes/%s.cpw^3 failed ^5(old coverpoint file: map wpNum %i - cpw wpNum: %i)^5!!!\n", "AUTO-WAYPOINTER", cgs.currentmapname, number_of_nodes, num_map_waypoints );
		fclose( f );
		return qfalse;
	}

	fread( &num_cover_spots, sizeof(int), 1, f );

	for ( i = 0; i < num_cover_spots; i++ )
	{
		int j = 0;

		fread( &cover_nodes[i], sizeof(int), 1, f );

		/*
		// UQ1: Now read the spots this is a coverpoint for...
		fread( &nodes[cover_nodes[i]].coverpointNum, sizeof(int), 1, f );

		// And then read each one...
		for ( j = 0; j < nodes[cover_nodes[i]].coverpointNum; j++)
		{
			fread( &nodes[cover_nodes[i]].coverpointFor[j], sizeof(int), 1, f );
		}
		*/

		if (!(nodes[cover_nodes[i]].type & NODE_COVER))
			nodes[cover_nodes[i]].type |= NODE_COVER;

		//trap->Print("Cover spot #%i (node %i) is at %f %f %f.\n", i, cover_nodes[i], nodes[cover_nodes[i]].origin[0], nodes[cover_nodes[i]].origin[1], nodes[cover_nodes[i]].origin[2]);
	}

	fclose( f );

	trap->Print( "^1*** ^3%s^5: Successfully loaded %i cover points from file ^7/nodes/%s.cpw^5.\n", "AUTO-WAYPOINTER", num_cover_spots, cgs.currentmapname);

	return qtrue;
}

qboolean
AIMOD_LoadCoverPoints ( void )
{
	//FILE			*pIn;
	int				i = 0;
	fileHandle_t	f;
	int				num_map_waypoints = 0;

	vmCvar_t mapname;
	trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

	// Init...
	num_cover_spots = 0;

	trap->FS_Open( va( "nodes/%s.cpw", cgs.currentmapname), &f, FS_READ );

	if (!f)
	{
		return AIMOD_LoadCoverPoints2();
	}

	trap->FS_Read( &num_map_waypoints, sizeof(int), f );

	if (num_map_waypoints != number_of_nodes)
	{// Is an old file! We need to make a new one!
		trap->Print( "^1*** ^3%s^5: Reading coverpoints from ^7/nodes/%s.cpw^3 failed ^5(old coverpoint file)^5!!!\n", "AUTO-WAYPOINTER", cgs.currentmapname);
		trap->FS_Close( f );
		return qfalse;
	}

	trap->FS_Read( &num_cover_spots, sizeof(int), f );

	for ( i = 0; i < num_cover_spots; i++ )
	{
		int j = 0;

		trap->FS_Read( &(cover_nodes[i]), sizeof(int), f );

		/*
		// UQ1: Now read the spots this is a coverpoint for...
		trap->FS_Read( &(nodes[cover_nodes[i]].coverpointNum), sizeof(int), f );

		// And then read each one...
		for ( j = 0; j < nodes[cover_nodes[i]].coverpointNum; j++)
		{
			trap->FS_Read( &(nodes[cover_nodes[i]].coverpointFor[j]), sizeof(int), f );
		}
		*/

		if (!(nodes[cover_nodes[i]].type & NODE_COVER))
			nodes[cover_nodes[i]].type |= NODE_COVER;

		//trap->Print("Cover spot #%i (node %i) is at %f %f %f.\n", i, cover_nodes[i], nodes[cover_nodes[i]].origin[0], nodes[cover_nodes[i]].origin[1], nodes[cover_nodes[i]].origin[2]);
	}

	trap->FS_Close( f );

	trap->Print( "^1*** ^3%s^5: Successfully loaded %i cover points from file ^7/nodes/%s.cpw^5.\n", "AUTO-WAYPOINTER", num_cover_spots, cgs.currentmapname);

	return qtrue;
}

void AIMOD_Generate_Cover_Spots ( void )
{
	{// Need to make some from waypoint list if we can!
		if (number_of_nodes > 32000)
		{
			trap->Print("^3*** ^3%s: ^5Too many waypoints to make cover spots. Use ^3/awo^5 (auto-waypoint optimizer) to reduce the numbers!\n", "AUTO-WAYPOINTER");
			return;
		}

		num_cover_spots = 0;

		if (number_of_nodes > 0)
		{
			int i = 0;
			int update_timer = 0;

			trap->Print( "^1*** ^3%s^1: ^5Generating and saving coverspot waypoints list.\n", "AUTO-WAYPOINTER" );
			strcpy( task_string3, va("^5Generating and saving coverspot waypoints list...") );
			trap->UpdateScreen();

			aw_percent_complete = 0.0f;
			aw_stage_start_time = clock();

			for (i = 0; i < number_of_nodes; i++)
			{
				int			j = 0;
				vec3_t		up_org2;
				//qboolean	IS_GOOD_COVERPOINT = qfalse;

				//nodes[i].coverpointNum = 0;

				// Draw a nice little progress bar ;)
				aw_percent_complete = (float)((float)i/(float)number_of_nodes)*100.0f;

				update_timer++;

				if (update_timer >= 100)
				{
					trap->UpdateScreen();
					update_timer = 0;
				}

				for (j = 0; j < number_of_nodes; j++)
				{
					if (VectorDistance(nodes[i].origin, nodes[j].origin) < 256.0f)
					{
						vec3_t up_org;

						VectorCopy(nodes[j].origin, up_org);
						up_org[2]+=DEFAULT_VIEWHEIGHT; // Standing height!

						VectorCopy(nodes[i].origin, up_org2);
						up_org2[2]+=DEFAULT_VIEWHEIGHT; // Standing height!

						if (!OrgVisible(up_org, up_org2, -1))
						{
							/*
							IS_GOOD_COVERPOINT = qtrue;
							nodes[i].coverpointFor[nodes[i].coverpointNum] = j;
							nodes[i].coverpointNum++;

							if (nodes[i].coverpointNum > 1023)
								break; // Maxed them out...
							*/
							nodes[i].type |= NODE_COVER;
							cover_nodes[num_cover_spots] = i;
							num_cover_spots++;

							strcpy( last_node_added_string, va("^5Waypoint ^3%i ^5set as a cover waypoint.", i) );
							break;
						}
					}
				}

				/*
				if (IS_GOOD_COVERPOINT)
				{
					nodes[i].type |= NODE_COVER;
					cover_nodes[num_cover_spots] = i;
					num_cover_spots++;

					strcpy( last_node_added_string, va("^5Waypoint ^3%i ^5set as a cover waypoint for %i other waypoints.", i, nodes[i].coverpointNum) );
				}
				*/
			}

			aw_percent_complete = 0.0f;
			trap->UpdateScreen();
			update_timer = 0;

			//if (bot_debug.integer)
			{
				trap->Print( "^1*** ^3%s^1: ^5 Generated ^7%i^5 coverspot waypoints.\n", "AUTO-WAYPOINTER", num_cover_spots );
			}

			// Save them for fast loading next time!
			AIMOD_SaveCoverPoints();
		}
	}
}
#endif //__COVER_SPOTS__


#define NUM_SLOPE_CHECKS 16

qboolean AIMod_Check_Slope_Between ( vec3_t org1, vec3_t org2 ) {
	int		j;
	float	dist, last_height;
	vec3_t	orgA, orgB, originalOrgB, forward, dir, testangles;
	trace_t tr;
	vec3_t		boxMins = { -1, -1, -1 };
	vec3_t		boxMaxs = { 1, 1, 1 };
	//vec3_t	boxMins= {-8, -8, -8}; // @fixme , tune this to be more smooth on delailed terrian (eg. railroad )
	//vec3_t	boxMaxs= {8, 8, 8};
	int		incrument;

	if (org1[2] > org2[2])
	{
		VectorSet(orgA, org1[0], org1[1], org1[2]);
		VectorSet(orgB, org2[0], org2[1], org2[2]);
	}
	else
	{
		VectorSet(orgB, org1[0], org1[1], org1[2]);
		VectorSet(orgA, org2[0], org2[1], org2[2]);
	}

	if (NearMoverEntityLocation(orgA) || NearMoverEntityLocation(orgB))
	{// Movers are always OK...
		return qtrue;
	}

	VectorCopy(orgB, originalOrgB);

	last_height = orgA[2];

	orgA[2] += 18.0;
	orgB[2] = orgA[2];

	dist = DistanceHorizontal(orgA, orgB);

	incrument = dist/NUM_SLOPE_CHECKS;
	if (incrument < 1) incrument = 1; // since j is an integer, set minimum incument to 1 to avoid endless loops...

	for (j = incrument; j < dist-(incrument+1.0); j+=incrument)
	{
		if (org1[2] > org2[2])
		{
			VectorSet(orgA, org1[0], org1[1], org1[2]);
			VectorSet(orgB, org2[0], org2[1], org2[2]);
		}
		else
		{
			VectorSet(orgB, org1[0], org1[1], org1[2]);
			VectorSet(orgA, org2[0], org2[1], org2[2]);
		}

		orgA[2] += 18.0;
		orgB[2] = orgA[2];

		VectorSubtract(orgB, orgA, dir);
		vectoangles(dir, testangles);
		AngleVectors( testangles, forward, NULL, NULL );
		VectorMA(orgA, j, forward, orgA);

		orgB[2] = -65000;
		//orgB[2] = -4000;

		CG_Trace( &tr, orgA, boxMins, boxMaxs, orgB, -1, MASK_PLAYERSOLID );

		if (tr.contents & CONTENTS_LAVA)
		{
			return qfalse; // Hit LAVA. BAD!
		}
		else if (tr.entityNum != ENTITYNUM_NONE
			&& tr.entityNum < ENTITYNUM_MAX_NORMAL
			&& VisibleAllowEntType(cg_entities[tr.entityNum].currentState.eType, 0))
		{// Movers are always OK...
			continue;
		}
		else
		{
			//if (tr.endpos[2] < originalOrgB[2] - 18.0)
			//	return qfalse; // Too much of a drop...

			if (tr.endpos[2] < last_height - 18.0 || tr.endpos[2] > last_height + 18.0)
			{
				//trap->Print("A: %f %f %f - B: %f %f %f - EP: %f - LAST: %f.\n", orgA[0], orgA[1], orgA[2], orgB[0], orgB[1], orgB[2], tr.endpos[2], last_height - 18.0);
				return qfalse; // Too much of a drop...
			}
		}

		last_height = tr.endpos[2];
	}

	return qtrue;
}

#define Q3_INFINITE			16777216

qboolean AWP_CheckFallPositionOK(vec3_t position)
{
	trace_t		tr;
	vec3_t testPos, downPos;
	vec3_t mins, maxs;

	//VectorSet(mins, -8, -8, -1);
	//VectorSet(maxs, 8, 8, 1);
	VectorSet(mins, -24, -24, -1);
	VectorSet(maxs, 24, 24, 1);

	VectorCopy(position, testPos);
	VectorCopy(position, downPos);

	downPos[2] -= 64.0;
	testPos[2] += 96.0;

	//trap->Trace( &tr, testPos, NULL/*NPC->r.mins*/, NULL/*NPC->r.maxs*/, downPos, NPC->s.number, MASK_PLAYERSOLID, 0, 0, 0 );
	CG_Trace( &tr, testPos, mins, maxs, downPos, -1, MASK_PLAYERSOLID );

	if (tr.entityNum != ENTITYNUM_NONE)
	{
		return qtrue;
	}
	else if (tr.fraction == 1.0f)
	{
		//trap->Print("%s is holding position to not fall!\n", NPC->client->pers.netname);
		return qfalse;
	}

	return qtrue;
}

qboolean AWP_Jump( vec3_t start, vec3_t dest )
{//FIXME: if land on enemy, knock him down & jump off again
	{
		float	targetDist, shotSpeed = 300, travelTime, impactDist, bestImpactDist = Q3_INFINITE;//fireSpeed,
		vec3_t	targetDir, shotVel, failCase;
		trace_t	trace;
		trajectory_t	tr;
		qboolean	blocked;
		int		elapsedTime, timeStep = 500, hitCount = 0, maxHits = 7;
		vec3_t	lastPos, testPos, bottom, mins, maxs;

		//VectorSet(mins, -15, -15, DEFAULT_MINS_2);
		//VectorSet(maxs, 15, 15, DEFAULT_MAXS_2);
		VectorSet(mins, -8, -8, -1);
		VectorSet(maxs, 8, 8, 1);

		while ( hitCount < maxHits )
		{
			VectorSubtract( dest, start, targetDir );
			targetDist = VectorNormalize( targetDir );

			VectorScale( targetDir, shotSpeed, shotVel );
			travelTime = targetDist/shotSpeed;
			shotVel[2] += travelTime * 0.5 * /*aiEnt->client->ps.gravity*/ 1.0;

			if ( !hitCount )
			{//save the first one as the worst case scenario
				VectorCopy( shotVel, failCase );
			}

			if ( 1 )//tracePath )
			{//do a rough trace of the path
				blocked = qfalse;

				VectorCopy( start, tr.trBase );
				VectorCopy( shotVel, tr.trDelta );
				tr.trType = TR_GRAVITY;
				tr.trTime = cg.time;
				travelTime *= 1000.0f;
				VectorCopy( start, lastPos );

				//This may be kind of wasteful, especially on long throws... use larger steps?  Divide the travelTime into a certain hard number of slices?  Trace just to apex and down?
				for ( elapsedTime = timeStep; elapsedTime < floor(travelTime)+timeStep; elapsedTime += timeStep )
				{
					if ( (float)elapsedTime > travelTime )
					{//cap it
						elapsedTime = floor( travelTime );
					}
					BG_EvaluateTrajectory( &tr, cg.time + elapsedTime, testPos );
					if ( testPos[2] < lastPos[2] )
					{//going down, ignore botclip
						CG_Trace( &trace, lastPos, mins, maxs, testPos, -1, MASK_PLAYERSOLID);
					}
					else
					{//going up, check for botclip
						CG_Trace( &trace, lastPos, mins, maxs, testPos, -1, MASK_PLAYERSOLID|CONTENTS_BOTCLIP);
					}

					if ( trace.allsolid || trace.startsolid )
					{
						blocked = qtrue;
						break;
					}

					if ( trace.fraction < 1.0f )
					{//hit something
						if ( Distance( trace.endpos, dest ) < 128/*96*/
							&& AWP_CheckFallPositionOK(trace.endpos) )
						{//hit the spot, that's perfect!
							break;
						}
						else
						{
							if ( trace.contents & CONTENTS_BOTCLIP )
							{//hit a do-not-enter brush
								blocked = qtrue;
								break;
							}
							if ( trace.plane.normal[2] > 0.7 && DistanceSquared( trace.endpos, dest ) < 4096 )//hit within 64 of desired location, should be okay
							{//close enough!
								break;
							}
							else
							{//FIXME: maybe find the extents of this brush and go above or below it on next try somehow?
								impactDist = DistanceSquared( trace.endpos, dest );
								if ( impactDist < bestImpactDist )
								{
									bestImpactDist = impactDist;
									VectorCopy( shotVel, failCase );
								}
								blocked = qtrue;
								break;
							}
						}
					}

					if ( elapsedTime == floor( travelTime ) )
					{//reached end, all clear
						if ( trace.fraction >= 1.0f )
						{//hmm, make sure we'll land on the ground...
							//FIXME: do we care how far below ourselves or our dest we'll land?
							VectorCopy( trace.endpos, bottom );
							//bottom[2] -= 128;
							bottom[2] -= 64; // UQ1: Try less fall...
							CG_Trace( &trace, trace.endpos, mins, maxs, bottom, -1, MASK_PLAYERSOLID);
							if ( trace.fraction >= 1.0f )
							{//would fall too far
								blocked = qtrue;
							}
						}
						break;
					}
					else
					{
						//all clear, try next slice
						VectorCopy( testPos, lastPos );
					}
				}

				if ( blocked )
				{//hit something, adjust speed (which will change arc)
					hitCount++;
					shotSpeed = 300 + ((hitCount-2) * 100);//from 100 to 900 (skipping 300)
					if ( hitCount >= 2 )
					{//skip 300 since that was the first value we tested
						shotSpeed += 100;
					}
				}
				else
				{//made it!
					break;
				}
			}
			else
			{//no need to check the path, go with first calc
				break;
			}
		}

		if ( hitCount < maxHits )
		{//NOTE: all good...
			return qtrue;
		}
	}

	// UQ1: A more simple check...
	{
		vec3_t start2, end, mins, maxs;
		trace_t	trace;

		VectorSet(mins, -8, -8, -1);
		VectorSet(maxs, 8, 8, 1);

		VectorCopy(start, end);
		end[2] += 256.0;

		CG_Trace( &trace, start, mins, maxs, end, -1, MASK_PLAYERSOLID);

		VectorCopy(trace.endpos, start2);
		start2[2] -= 8.0;

		if (OrgVisible(start2, dest, -1))
		{// Destination is visible from here!
			return qtrue;
		}
	}

	return qfalse;
}

extern qboolean FOLIAGE_TreeSolidBlocking_AWP_Path(vec3_t from, vec3_t to);

/* */
int
AIMOD_MAPPING_CreateNodeLinks ( int node )
{
	vec3_t	upOrg;
	int		linknum = 0;

	node_t	*addNode = &nodes[node];
	float	currentDistance = 0.0;
	int		currentNode = -1;
	int		fails = 0;

	VectorCopy(addNode->origin, upOrg);
	upOrg[2] += 8;

	int maxLinks = MAX_AWP_NODELINKS;
	if (DO_EXTRA_REACH) maxLinks = MAX_NODELINKS;

	while (linknum < maxLinks)
	{
		float	currentClosestDistance = 999999.0;
		int		currentClosestNode = -1;

		if (fails > 4) break;

		for (int loop = 0; loop < number_of_nodes; loop++)
		{
			if (linknum >= maxLinks)
			{
				break;
			}

			if (loop == node)
				continue;

			if (loop == currentNode)
				continue;

			node_t		*closeNode = &nodes[loop];

			float		dist = Distance(addNode->origin, closeNode->origin);

			if (!(dist <= currentClosestDistance && dist >= currentDistance))
				continue;

			qboolean inList = qfalse;

			for (int lks = 0; lks < linknum; lks++)
			{
				if (addNode->links[lks].targetNode == loop)
				{
					inList = qtrue;
					break;
				}
			}

			if (inList) continue;

			currentClosestNode = loop;
			currentClosestDistance = dist;
		}
		
		if (currentClosestNode == -1)
		{// None found.
			break;
		}

		// Should now have the next best option...
		currentNode = currentClosestNode;
		currentDistance = currentClosestDistance;

		node_t		*closeNode = &nodes[currentNode];

		int		visCheck = 0;
		vec3_t	this_org;

		VectorCopy(closeNode->origin, this_org);
		this_org[2] += 8;

		if (DO_FAST_LINK || DO_ULTRAFAST)
		{// It's in range... Just accept the linkage... Could easy be bad, but oh well...
			float vdist = DistanceVertical(closeNode->origin, addNode->origin);
			float hdist = DistanceHorizontal(closeNode->origin, addNode->origin);

			addNode->links[linknum].targetNode = currentNode;
			addNode->links[linknum].cost = currentDistance + (vdist*vdist);
			addNode->links[linknum].flags = 0;

			if (hdist * 1.3 < vdist)
			{// Force jump...
				addNode->links[linknum].flags |= NODE_JUMP;
			}
			else if (addNode->type == NODE_WATER || closeNode->type == NODE_WATER)
			{
				addNode->links[linknum].flags |= NODE_WATER;
			}

			linknum++;
		}
		else
		{
			visCheck = NodeVisible(this_org, upOrg, cg.clientNum);

			//0 = wall in way
			//1 = player or no obstruction
			//2 = useable door in the way.
			//3 = door entity in the way.
			if (visCheck == 1 || visCheck == 2 || visCheck == 3 /*|| loop == node - 1*/)
			{
				float vdist = DistanceVertical(closeNode->origin, addNode->origin);
				float hdist = DistanceHorizontal(closeNode->origin, addNode->origin);

				addNode->links[linknum].targetNode = currentNode;
				addNode->links[linknum].cost = currentDistance + (vdist*vdist);
				addNode->links[linknum].flags = 0;

				if (hdist * 1.3 < vdist)
				{// Force jump...
					addNode->links[linknum].flags |= NODE_JUMP;
				}
				else if (addNode->type == NODE_WATER || closeNode->type == NODE_WATER)
				{
					addNode->links[linknum].flags |= NODE_WATER;
				}
				else if (AIMod_Check_Slope_Between(upOrg, this_org))
				{// No need for jump...

				}
				else if (AWP_Jump(upOrg, this_org))
				{// Can jump there!
					addNode->links[linknum].flags |= NODE_JUMP;
				}

				fails = 0;
				linknum++;
			}
			else
			{
				fails++;
			}
		}

		if (fails > 4) break;
	}

	nodes[node].enodenum = linknum;

	//if (nodes[node].enodenum > 0)
	//	trap->Print("Node %i has %i links. ", node, nodes[node].enodenum);

	return ( linknum );
}

/* */
qboolean
AI_PM_SlickTrace ( vec3_t point, int clientNum )
{
	/*trace_t trace;
	vec3_t	point2;
	VectorCopy( point, point2 );
	point2[2] = point2[2] - 0.25f;

	CG_Trace( &trace, point, botTraceMins, botTraceMaxs, point2, clientNum, MASK_SHOT );

	if ( trace.surfaceFlags & SURF_SLICK )
	{
		return ( qtrue );
	}
	else
	{*/
		return ( qfalse );
	//}
}

/* */
void
AIMOD_MAPPING_CreateSpecialNodeFlags ( int node )
{	// Need to check for duck (etc) nodes and mark them...
	trace_t tr;
	vec3_t	up, temp, uporg;

	if (!DO_FAST_LINK)
	{
		VectorCopy( nodes[node].origin, temp );
		temp[2] += 1;
		nodes[node].type &= ~NODE_DUCK;
		VectorCopy( nodes[node].origin, up );
		up[2] += 16550;
		CG_Trace( &tr, nodes[node].origin, NULL, NULL, up, -1, MASK_SHOT | MASK_OPAQUE | MASK_WATER /*MASK_ALL*/ );

		if ( VectorDistance( nodes[node].origin, tr.endpos) <= 72 )
		{	// Could not see the up pos.. Need to duck to go here!
			nodes[node].type |= NODE_DUCK;
			//trap->Print( "^4*** ^3%s^5: Node ^7%i^5 marked as a duck node.\n", "AUTO-WAYPOINTER", node );
		}

		if ( AI_PM_SlickTrace( nodes[node].origin, -1) )
		{	// This node is on slippery ice... Mark it...
			nodes[node].type |= NODE_ICE;
			//trap->Print( "^4*** ^3%s^5: Node ^7%i^5 marked as an ice (slick) node.\n", "AUTO-WAYPOINTER", node );
		}

		VectorCopy(nodes[node].origin, temp);
		temp[2] += 1;
		nodes[node].type &= ~NODE_WATER;
		VectorCopy(nodes[node].origin, up);
		up[2] -= 256.0;
		CG_Trace(&tr, nodes[node].origin, NULL, NULL, up, -1, MASK_SHOT | MASK_OPAQUE | MASK_WATER /*MASK_ALL*/);

		if ((tr.contents & CONTENTS_WATER) || (tr.materialType) == MATERIAL_WATER)
		{	// This node is on slippery ice... Mark it...
			nodes[node].type |= NODE_WATER;
			//trap->Print( "^4*** ^3%s^5: Node ^7%i^5 marked as an water node.\n", "AUTO-WAYPOINTER", node );
		}

		if (RoadExistsAtPoint(nodes[node].origin))
		{
			nodes[node].type |= NODE_ROAD;
		}

		VectorCopy(nodes[node].origin, uporg);
		uporg[2]+=104;
	}
}

//#define __BOT_AUTOWAYPOINT_OPTIMIZE__

/* */
void
AIMOD_MAPPING_MakeLinks ( void )
{
	int		loop = 0;
//	node_t	*good_nodes;
//	int		upto = 0;
//	int		total_good_count = 0;
//	int		bad_nodes = 0;
	int		final_tests = 0;
	int		update_timer = 0;

#ifdef __BOT_AUTOWAYPOINT_OPTIMIZE__
	good_nodes = malloc( (sizeof(node_t)+1)*MAX_NODES );
#endif //__BOT_AUTOWAYPOINT_OPTIMIZE__

	if ( aw_num_nodes > 0 )
	{
		number_of_nodes = aw_num_nodes;
	}

	aw_percent_complete = 0.0f;
	aw_stage_start_time = clock();
 	strcpy( last_node_added_string, va("") );

	RoadExistsAtPoint(vec3_origin); // Just to make sure that road map is pre-loaded...

	trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Creating waypoint linkages and flags...\n") );
	strcpy( task_string3, va("^5Creating waypoint linkages and flags...") );
	trap->UpdateScreen();

	final_tests = 0;
	update_timer = 0;
	aw_percent_complete = 0.0f;
	trap->UpdateScreen();

	{
#pragma omp parallel for ordered schedule(dynamic)// num_threads(32)
		for ( loop = 0; loop < number_of_nodes; loop++ )
		{// Do links...
			nodes[loop].enodenum = 0;

			// Draw a nice little progress bar ;)
			aw_percent_complete = (float)((float)((float)loop/(float)number_of_nodes)*100.0f);

			update_timer++;

			if(omp_get_thread_num() == 0)
			{
				if (update_timer >= 100)
				{
					trap->UpdateScreen();
					update_timer = 0;
				}
			}

			// Also check if the node needs special flags...
			AIMOD_MAPPING_CreateSpecialNodeFlags( loop );
			AIMOD_MAPPING_CreateNodeLinks( loop );
			#pragma omp critical (__CREATE_NODE_LINKS_UPDATE__)
			{
				strcpy( last_node_added_string, va("^5Created ^3%i ^5links for waypoint ^7%i^5.", nodes[loop].enodenum, loop) );
			}
		}
	}

	aw_percent_complete = 0.0f;
	strcpy( last_node_added_string, va("") );
	aw_num_nodes = number_of_nodes;
	trap->UpdateScreen();
}

/* */
void
AIMOD_NODES_SetObjectiveFlags ( int node )
{										// Find objects near this node.
	// Init them first...
	nodes[node].objectNum[0] = nodes[node].objectNum[1] = nodes[node].objectNum[2] = ENTITYNUM_NONE;
	nodes[node].objEntity = -1;
	nodes[node].objFlags = -1;
	nodes[node].objTeam = -1;
	nodes[node].objType = -1;
}

/*////////////////////////////////////////////////
ConnectNodes
Connects 2 nodes and sets the flags for the path between them
/*/


///////////////////////////////////////////////
qboolean
ConnectNodes ( int from, int to, int flags )
{
	int maxLinks = MAX_AWP_NODELINKS;
	if (DO_EXTRA_REACH) maxLinks = MAX_NODELINKS;

	//check that we don't have too many connections from the 'from' node already
	if ( nodes[from].enodenum + 1 > maxLinks)
	{
		return ( qfalse );
	}

	//check that we are creating a path between 2 valid nodes
	if ( (nodes[from].type == NODE_INVALID) || (to == NODE_INVALID) || from > MAX_NODES || from < 0 || to > MAX_NODES || to < 0)
	{	//nodes[to].type is invalid on LoadNodes()
		return ( qfalse );
	}

	//update the individual nodes
	nodes[from].links[nodes[from].enodenum].targetNode = to;
	nodes[from].links[nodes[from].enodenum].flags = flags;
	nodes[from].enodenum++;
	return ( qtrue );
}

/*//////////////////////////////////////////////
AddNode
creates a new waypoint/node with the specified values
*/

/////////////////////////////////////////////
int numAxisOnlyNodes = 0;
int AxisOnlyFirstNode = -1;
int numAlliedOnlyNodes = 0;
int AlliedOnlyFirstNode = -1;


/* */
qboolean
Load_AddNode ( vec3_t origin, int fl, short int *ents, int objFl )
{
	if ( number_of_nodes + 1 > MAX_NODES )
	{
		return ( qfalse );
	}

	VectorCopy( origin, nodes[number_of_nodes].origin );	//set the node's position

	nodes[number_of_nodes].type = fl;						//set the flags (NODE_OBJECTIVE, for example)
	nodes[number_of_nodes].objectNum[0] = ents[0];			//set any map objects associated with this node
	nodes[number_of_nodes].objectNum[1] = ents[1];			//only applies to objects linked to the unreachable flag
	nodes[number_of_nodes].objectNum[2] = ents[2];
	nodes[number_of_nodes].objFlags = objFl;				//set the objective flags

	if ( nodes[number_of_nodes].type & NODE_AXIS_UNREACHABLE )
	{
		if ( AlliedOnlyFirstNode < 0 )
		{
			AlliedOnlyFirstNode = number_of_nodes;
		}
		nodes[number_of_nodes].objTeam |= FACTION_REBEL;
		numAlliedOnlyNodes++;
	}

	if ( nodes[number_of_nodes].type & NODE_ALLY_UNREACHABLE )
	{
		if ( AxisOnlyFirstNode < 0 )
		{
			AxisOnlyFirstNode = number_of_nodes;
		}
		nodes[number_of_nodes].objTeam |= FACTION_EMPIRE;
		numAxisOnlyNodes++;
	}

	number_of_nodes++;
	return ( qtrue );
}

static char * FS_BuildOSPath( const char *base, const char *game, const char *qpath )  {
	__asm {
		mov		eax, game
		mov		edx, qpath
		push	base
		mov		ebx, 0x43A550
		call	ebx
		add		esp, 4
	}
}

/* */
void
AIMOD_NODES_LoadNodes2 ( void )
{
	FILE			*f;
	int				i, j;
	vmCvar_t		fs_homepath, fs_game;
	short int		objNum[3] = { 0, 0, 0 },
	objFlags, numLinks;
	int				flags;
	vec3_t			vec;
	short int		fl2;
	int				target;
	char			name[] = BOT_MOD_NAME;
	char			nm[64] = "";
	float			version;
	char			map[64] = "";
	char			mp[64] = "";
	/*short*/ int		numberNodes;
	short int		temp, fix_aas_nodes;

	trap->Cvar_Register( &fs_homepath, "fs_homepath", "", CVAR_SERVERINFO | CVAR_ROM );
	trap->Cvar_Register( &fs_game, "fs_game", "", CVAR_SERVERINFO | CVAR_ROM );

	RoadExistsAtPoint(vec3_origin); // Just to make sure that road map is pre-loaded...

	i = 0;

	f = fopen( va("%s/%s/nodes/%s.bwp", fs_homepath.string, fs_game.string, cgs.currentmapname), "rb" );

	if ( !f )
	{
		trap->Print( "^1*** ^3WARNING^5: Reading from ^7nodes/%s.bwp^3 failed^5!!!\n", cgs.currentmapname);
		trap->Print( "^1*** ^3       ^5  You need to make bot routes for this map.\n" );
		trap->Print( "^1*** ^3       ^5  Bots will move randomly for this map.\n" );
		return;
	}

	strcpy( mp, cgs.currentmapname);

	fread( &nm, strlen( name) + 1, 1, f);

	fread( &version, sizeof(float), 1, f);

	if ( version != NOD_VERSION && version != 1.0f)
	{
		trap->Print( "^1*** ^3WARNING^5: Reading from ^7nodes/%s.bwp^3 failed^5!!!\n", cgs.currentmapname);
		trap->Print( "^1*** ^3       ^5  Old node file detected.\n" );
		fclose(f);
		return;
	}

	fread( &map, strlen( mp) + 1, 1, f);
	if ( Q_stricmp( map, cgs.currentmapname) != 0 )
	{
		trap->Print( "^1*** ^3WARNING^5: Reading from ^7nodes/%s.bwp^3 failed^5!!!\n", cgs.currentmapname);
		trap->Print( "^1*** ^3       ^5  Node file is not for this map!\n" );
		fclose(f);
		return;
	}

	if (version == NOD_VERSION)
	{
		fread( &numberNodes, sizeof(/*short*/ int), 1, f);
	}
	else
	{
		fread( &temp, sizeof(short int), 1, f);
		numberNodes = temp;
	}

	for ( i = 0; i < numberNodes; i++ )					//loop through all the nodes
	{
		nodes[i].enodenum = 0;

		//read in all the node info stored in the file
		fread( &vec, sizeof(vec3_t), 1, f);
		fread( &flags, sizeof(int), 1, f);
		fread( objNum, (sizeof(short int) * 3), 1, f);
		fread( &objFlags, sizeof(short int), 1, f);
		fread( &numLinks, sizeof(short int), 1, f);

		Load_AddNode( vec, flags, objNum, objFlags );	//add the node

		//loop through all of the links and read the data
		for ( j = 0; j < numLinks; j++ )
		{
			if (version == NOD_VERSION)
			{
				fread( &target, sizeof(/*short*/ int), 1, f);
			}
			else
			{
				fread( &temp, sizeof(short int), 1, f);
				target = temp;
			}

			fread( &fl2, sizeof(short int), 1, f);
			ConnectNodes( i, target, fl2 );				//add any links
		}

		// Set node objective flags..
		AIMOD_NODES_SetObjectiveFlags( i );
	}

	fread( &fix_aas_nodes, sizeof(short int), 1, f);

	fclose(f);
	trap->Print( "^1*** ^3%s^5: Successfully loaded %i waypoints from waypoint file ^7nodes/%s.bwp^5.\n", "AUTO-WAYPOINTER",
			  number_of_nodes, cgs.currentmapname);

	nodes_loaded = qtrue;

	return;
}

/* */
void
AIMOD_NODES_LoadNodes ( void )
{
	fileHandle_t	f;
	int				i, j;
	char			filename[60];
//	vmCvar_t		mapname;
	short int		objNum[3] = { 0, 0, 0 },
	objFlags, numLinks;
	int				flags;
	vec3_t			vec;
	short int		fl2;
	int				target;
	char			name[] = BOT_MOD_NAME;
	char			nm[64] = "";
	float			version;
	char			map[64] = "";
	char			mp[64] = "";
	/*short*/ int		numberNodes;
	short int		temp, fix_aas_nodes;

	vmCvar_t mapname;
	trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

	i = 0;

	///////////////////
	//open the node file for reading, return false on error
	trap->FS_Open( va( "nodes/%s.bwp", cgs.currentmapname), &f, FS_READ );
	if ( !f )
	{
		AIMOD_NODES_LoadNodes2();
		return;
	}

	RoadExistsAtPoint(vec3_origin); // Just to make sure that road map is pre-loaded...

	strcpy( mp, cgs.currentmapname);
	trap->FS_Read( &nm, strlen( name) + 1, f );									//read in a string the size of the mod name (+1 is because all strings end in hex '00')
	trap->FS_Read( &version, sizeof(float), f );			//read and make sure the version is the same

	if ( version != NOD_VERSION && version != 1.0f )
	{
		trap->Print( "^1*** ^3WARNING^5: Reading from ^7nodes/%s.bwp^3 failed^5!!!\n", filename );
		trap->Print( "^1*** ^3       ^5  Old node file detected.\n" );
		trap->FS_Close( f );
		return;
	}

	trap->FS_Read( &map, strlen( mp) + 1, f );			//make sure the file is for the current map
	if ( Q_stricmp( map, mp) != 0 )
	{
		trap->Print( "^1*** ^3WARNING^5: Reading from ^7nodes/%s.bwp^3 failed^5!!!\n", filename );
		trap->Print( "^1*** ^3       ^5  Node file is not for this map!\n" );
		trap->FS_Close( f );
		return;
	}

	if (version == NOD_VERSION)
	{
		trap->FS_Read( &numberNodes, sizeof(/*short*/ int), f ); //read in the number of nodes in the map
	}
	else
	{
		trap->FS_Read( &temp, sizeof(short int), f ); //read in the number of nodes in the map
		numberNodes = temp;
	}

	for ( i = 0; i < numberNodes; i++ )					//loop through all the nodes
	{
		nodes[i].enodenum = 0;

		//read in all the node info stored in the file
		trap->FS_Read( &vec, sizeof(vec3_t), f );
		trap->FS_Read( &flags, sizeof(int), f );
		trap->FS_Read( objNum, sizeof(short int) * 3, f );
		trap->FS_Read( &objFlags, sizeof(short int), f );
		trap->FS_Read( &numLinks, sizeof(short int), f );

		Load_AddNode( vec, flags, objNum, objFlags );	//add the node

		//loop through all of the links and read the data
		for ( j = 0; j < numLinks; j++ )
		{
			if (version == NOD_VERSION)
			{
				trap->FS_Read( &target, sizeof(/*short*/ int), f );
			}
			else
			{
				trap->FS_Read( &temp, sizeof(short int), f );
				target = temp;
			}

			trap->FS_Read( &fl2, sizeof(short int), f );
			ConnectNodes( i, target, fl2 );				//add any links
		}

		// Set node objective flags..
		AIMOD_NODES_SetObjectiveFlags( i );
	}

	trap->FS_Read( &fix_aas_nodes, sizeof(short int), f );
	trap->FS_Close( f );							//close the file
	trap->Print( "^1*** ^3%s^5: Successfully loaded %i waypoints from waypoint file ^7nodes/%s.bwp^5.\n", "AUTO-WAYPOINTER",
			  number_of_nodes, cgs.currentmapname );
	nodes_loaded = qtrue;

	return;
}

void AIMOD_NODES_LoadOldJKAPathData( void )
{
	fileHandle_t	f;
	char			*fileString;
	char			*currentVar;
	char			*routePath;
	wpobject_t		thiswp;
	int				len;
	int				i, i_cv;
	int				nei_num;
	vmCvar_t		mapname;
	int				gLevelFlags;
	short int		objNum[3] = { 0, 0, 0 };

	trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

	RoadExistsAtPoint(vec3_origin); // Just to make sure that road map is pre-loaded...

	i = 0;
	i_cv = 0;

	routePath = (char *)malloc(1024);

	Com_sprintf(routePath, 1024, "botroutes/%s.wnt\0", cgs.currentmapname);

	len = trap->FS_Open(routePath, &f, FS_READ);

	free(routePath); //routePath

	if (!f)
	{
		trap->Print(S_COLOR_YELLOW "Bot route data not found for %s\n", cgs.currentmapname);
		return;
	}

	if (len >= 524288)
	{
		trap->Print(S_COLOR_RED "Route file exceeds maximum length\n");
		return;
	}

	fileString = (char *)malloc(524288);
	currentVar = (char *)malloc(2048);

	trap->FS_Read(fileString, len, f);

	if (fileString[i] == 'l')
	{ //contains a "levelflags" entry..
		char readLFlags[64];
		i_cv = 0;

		while (fileString[i] != ' ')
		{
			i++;
		}
		i++;
		while (fileString[i] != '\n')
		{
			readLFlags[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		readLFlags[i_cv] = 0;
		i++;

		gLevelFlags = atoi(readLFlags);
	}
	else
	{
		gLevelFlags = 0;
	}

	while (i < len)
	{
		int n = 0;

		i_cv = 0;

		thiswp.index = 0;
		thiswp.flags = 0;
		thiswp.inuse = 0;
		thiswp.neighbornum = 0;
		thiswp.origin[0] = 0;
		thiswp.origin[1] = 0;
		thiswp.origin[2] = 0;
		//thiswp.weight = 0;
		//thiswp.associated_entity = ENTITYNUM_NONE;
		//thiswp.forceJumpTo = 0;
		//thiswp.disttonext = 0;
		nei_num = 0;

		while (nei_num < MAX_NEIGHBOR_SIZE)
		{
			thiswp.neighbors[nei_num].num = 0;
			thiswp.neighbors[nei_num].forceJumpTo = 0;

			nei_num++;
		}

		while (fileString[i] != ' ')
		{
			currentVar[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		currentVar[i_cv] = '\0';

		thiswp.index = atoi(currentVar);

		i_cv = 0;
		i++;

		while (fileString[i] != ' ')
		{
			currentVar[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		currentVar[i_cv] = '\0';

		thiswp.flags = atoi(currentVar);

		i_cv = 0;
		i++;

		while (fileString[i] != ' ')
		{
			currentVar[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		currentVar[i_cv] = '\0';

		//thiswp.weight = atof(currentVar);

		i_cv = 0;
		i++;
		i++;

		while (fileString[i] != ' ')
		{
			currentVar[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		currentVar[i_cv] = '\0';

		thiswp.origin[0] = atof(currentVar);

		i_cv = 0;
		i++;

		while (fileString[i] != ' ')
		{
			currentVar[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		currentVar[i_cv] = '\0';

		thiswp.origin[1] = atof(currentVar);

		i_cv = 0;
		i++;

		while (fileString[i] != ')')
		{
			currentVar[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		currentVar[i_cv] = '\0';

		thiswp.origin[2] = atof(currentVar);

		thiswp.origin[2] += 8.0;
		thiswp.origin[2] = FloorHeightAt( thiswp.origin );
		trap->Print("Added wp at %f %f %f.\n", thiswp.origin[0], thiswp.origin[1], thiswp.origin[2]);
		Load_AddNode( thiswp.origin, 0, objNum, 0 );	//add the node
		nodes[thiswp.index].enodenum = 0;

		i += 4;

		while (fileString[i] != '}')
		{
			i_cv = 0;
			while (fileString[i] != ' ' && fileString[i] != '-')
			{
				currentVar[i_cv] = fileString[i];
				i_cv++;
				i++;
			}
			currentVar[i_cv] = '\0';

			thiswp.neighbors[thiswp.neighbornum].num = atoi(currentVar);

			if (fileString[i] == '-')
			{
				i_cv = 0;
				i++;

				while (fileString[i] != ' ')
				{
					currentVar[i_cv] = fileString[i];
					i_cv++;
					i++;
				}
				currentVar[i_cv] = '\0';

				thiswp.neighbors[thiswp.neighbornum].forceJumpTo = 999; //atoi(currentVar); //FJSR
			}
			else
			{
				thiswp.neighbors[thiswp.neighbornum].forceJumpTo = 0;
			}

			thiswp.neighbornum++;

			i++;
		}

		i_cv = 0;
		i++;
		i++;

		while (fileString[i] != '\n')
		{
			currentVar[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		currentVar[i_cv] = '\0';

		//thiswp.disttonext = atof(currentVar);


		// Set node objective flags..
		AIMOD_NODES_SetObjectiveFlags( thiswp.index );

		for (n = 0; n < thiswp.neighbornum; n++)
			ConnectNodes( thiswp.index, thiswp.neighbors[n].num, 0 );				//convert connections

		i++;
	}

	free(fileString); //fileString
	free(currentVar); //currentVar

	trap->FS_Close(f);

	trap->Print( "^1*** ^3%s^5: Successfully loaded %i waypoints from JKA waypoint file ^7botroutes/%s.wnt^5.\n", "AUTO-WAYPOINTER",
			  number_of_nodes, cgs.currentmapname );
	nodes_loaded = qtrue;

	return;
}

/* */
void
AIMOD_NODES_SaveNodes_Autowaypointed ( void )
{
	fileHandle_t	f;
	int				i;
	/*short*/ int		j;
	float			version = NOD_VERSION;										//version is 1.0 for now
	char			name[] = BOT_MOD_NAME;
	//vmCvar_t		mapname;
	char			map[64] = "";
	char			filename[60];
	/*short*/ int		num_nodes = number_of_nodes;

	vmCvar_t mapname;
	trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

	RoadExistsAtPoint(vec3_origin); // Just to make sure that road map is pre-loaded...

	aw_num_nodes = number_of_nodes;
	strcpy( filename, "nodes/" );

	///////////////////
	//try to open the output file, return if it failed
	trap->FS_Open( va( "nodes/%s.bwp", cgs.currentmapname), &f, FS_WRITE );
	if ( !f )
	{
		trap->Print( "^1*** ^3ERROR^5: Error opening node file ^7nodes/%s.bwp^5!!!\n", cgs.currentmapname/*filename*/ );
		return;
	}

	for ( i = 0; i < aw_num_nodes/*num_nodes*/; i++ )
	{
		nodes[i].enodenum = 0;

		for ( j = 0; j < MAX_NODELINKS; j++ )
		{
			nodes[i].links[j].targetNode = INVALID;
			nodes[i].links[j].cost = 999999;
			nodes[i].links[j].flags = 0;
			nodes[i].objectNum[0] = nodes[i].objectNum[1] = nodes[i].objectNum[2] = ENTITYNUM_NONE;
		}

		//trap->Print("Waypoint %i is at %f %f %f.\n", i, nodes[i].origin[0], nodes[i].origin[1], nodes[i].origin[2]);
	}

//	RemoveDoorsAndDestroyablesForSave();
//	num_nodes = number_of_nodes;

	// Resolve paths
	//-------------------------------------------------------
	AIMOD_MAPPING_MakeLinks();

	num_nodes = aw_num_nodes;

	for ( i = 0; i < aw_num_nodes/*num_nodes*/; i++ )
	{
		// Set node objective flags..
		AIMOD_NODES_SetObjectiveFlags( i );
	}

	//trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );	//get the map name
	strcpy( map, cgs.currentmapname);
	trap->FS_Write( &name, strlen( name) + 1, f );								//write the mod name to the file
	trap->FS_Write( &version, sizeof(float), f );								//write the version of this file
	trap->FS_Write( &map, strlen( map) + 1, f );									//write the map name
	trap->FS_Write( &num_nodes, sizeof(/*short*/ int), f );							//write the number of nodes in the map

	for ( i = 0; i < aw_num_nodes/*num_nodes*/; i++ )											//loop through all the nodes
	{
		//trap->Print("Saved wp at %f %f %f.\n", nodes[i].origin[0], nodes[i].origin[1], nodes[i].origin[2]);

		//write all the node data to the file
		trap->FS_Write( &(nodes[i].origin), sizeof(vec3_t), f );
		trap->FS_Write( &(nodes[i].type), sizeof(int), f );
		trap->FS_Write( &(nodes[i].objectNum), sizeof(short int) * 3, f );
		trap->FS_Write( &(nodes[i].objFlags), sizeof(short int), f );
		trap->FS_Write( &(nodes[i].enodenum), sizeof(short int), f );
		for ( j = 0; j < nodes[i].enodenum; j++ )
		{
			trap->FS_Write( &(nodes[i].links[j].targetNode), sizeof(/*short*/ int), f );
			trap->FS_Write( &(nodes[i].links[j].flags), sizeof(short int), f );
		}
	}
	{
		//short int	fix = 1;
		short int	fix = 0;
		trap->FS_Write( &fix, sizeof(short int), f );
	}

	trap->FS_Close( f );													//close the file
	trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^5Successfully saved node file ^7nodes/%s.bwp^5.\n", cgs.currentmapname/*filename*/ );
}

//
// The utilities for faster (then vector/float) integer maths...
//
typedef long int	intvec_t;
typedef intvec_t	intvec3_t[3];


/* */
void
intToVectorCopy ( const intvec3_t in, vec3_t out )
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}


/* */
void
intVectorCopy ( const intvec3_t in, intvec3_t out )
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}


/* */
intvec_t
intVectorLength ( const intvec3_t v2 )
{
	vec3_t v;
	v[0] = v2[0];
	v[1] = v2[1];
	v[2] = v2[2];
	return ( sqrt( v[0] * v[0] + v[1] * v[1] + v[2] * v[2]) );
}


/* */
void
intVectorSubtract ( const intvec3_t veca, const intvec3_t vecb, intvec3_t out )
{
	out[0] = veca[0] - vecb[0];
	out[1] = veca[1] - vecb[1];
	out[2] = veca[2] - vecb[2];
}


/* */
long int
intVectorDistance ( intvec3_t v1, intvec3_t v2 )
{
	intvec3_t	dir;
	intVectorSubtract( v2, v1, dir );
	return ( intVectorLength( dir) );
}

//
// Now the actual number crunching and visualizations...

extern float BG_GetGroundHeightAtPoint( vec3_t pos );
extern qboolean BG_TraceMapLoaded ( void );
extern void CG_GenerateTracemap( void );

qboolean Waypoint_FloorSurfaceOK ( int surfaceFlags )
{
	if (surfaceFlags == 0)
		return qtrue;

/*
#define	SURF_SKY				0x00002000	// lighting from environment map
#define	SURF_SLICK				0x00004000	// affects game physics
#define	SURF_METALSTEPS			0x00008000	// CHC needs this since we use same tools (though this flag is temp?)
#define SURF_FORCEFIELD			0x00010000	// CHC ""			(but not temp)
#define	SURF_NODAMAGE			0x00040000	// never give falling damage
#define	SURF_NOIMPACT			0x00080000	// don't make missile explosions
#define	SURF_NOMARKS			0x00100000	// don't leave missile marks
#define	SURF_NODRAW				0x00200000	// don't generate a drawsurface at all
#define	SURF_NOSTEPS			0x00400000	// no footstep sounds
#define	SURF_NODLIGHT			0x00800000	// don't dlight even if solid (solid lava, skies)
#define	SURF_NOMISCENTS			0x01000000	// no client models allowed on this surface
*/
	if (surfaceFlags & SURF_SLICK)
		return qtrue;

	if (surfaceFlags & SURF_METALSTEPS)
		return qtrue;

	if (surfaceFlags & SURF_FORCEFIELD)
		return qtrue;

	if (surfaceFlags & SURF_NOSTEPS)
		return qtrue;

	//if (surfaceFlags & SURF_SKY)
	//	return qfalse;

	return qfalse;
}

qboolean CG_HaveRoofAbove ( vec3_t origin )
{// Hopefully this will stop awp from adding waypoints on map roofs...
	vec3_t org, down_org;
	trace_t tr;

	if (DO_NOSKY) return qtrue;

	VectorCopy(origin, org);
	org[2]+=4.0;
	VectorCopy(origin, down_org);
	down_org[2] = MAX_MAP_SIZE;

	// Do forward test...
	CG_Trace( &tr, org, NULL, NULL, down_org, cg.clientNum, MASK_PLAYERSOLID/*|CONTENTS_TRIGGER|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP|CONTENTS_SHOTCLIP|CONTENTS_NODROP|CONTENTS_SHOTCLIP|CONTENTS_TRANSLUCENT*/ );

	//
	// Surface
	//

	if (tr.fraction >= 1.0 || tr.endpos[2] >= down_org[2])//tr.surfaceFlags == 0 && tr.contents == 0)
		return qfalse;

	if (!DO_ROCK && (tr.materialType) == MATERIAL_ROCK)
		return qfalse;

	if (DO_WATER && (tr.materialType) == MATERIAL_WATER)
		return qfalse;

	return qtrue;
}

float GroundHeightNoSurfaceChecks ( vec3_t org )
{
	trace_t tr;
	vec3_t org1, org2;

	VectorCopy(org, org1);
	org1[2]+=48;

	VectorCopy(org, org2);
	org2[2]= -MAX_MAP_SIZE;

	CG_Trace( &tr, org1, NULL, NULL, org2, -1, MASK_PLAYERSOLID);

	return tr.endpos[2];
}

float GroundHeightAt ( vec3_t org )
{
	trace_t tr;
	vec3_t org1, org2;
//	float height = 0;

	VectorCopy(org, org1);
	org1[2]+=48;

	VectorCopy(org, org2);
	org2[2]= -MAX_MAP_SIZE;

	CG_Trace( &tr, org1, NULL, NULL, org2, -1, MASK_PLAYERSOLID);
	//CG_Trace( &tr, org1, NULL, NULL, org2, cg.clientNum, MASK_PLAYERSOLID|CONTENTS_TRIGGER|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP|CONTENTS_SHOTCLIP|CONTENTS_NODROP|CONTENTS_TRANSLUCENT );

	if ( tr.startsolid || tr.allsolid )
	{
		return -MAX_MAP_SIZE;
	}

	if ( tr.surfaceFlags & SURF_SKY )
	{// Sky...
		return -MAX_MAP_SIZE;
	}

	if ( tr.contents & CONTENTS_TRIGGER )
	{// Trigger hurt???
		return -MAX_MAP_SIZE;
	}

	if (!DO_WATER && (tr.contents & CONTENTS_WATER) )
	{// Water. Bad m'kay...
		return -MAX_MAP_SIZE;
	}

//	if ( (tr.surfaceFlags & SURF_NODRAW)
//		&& (tr.surfaceFlags & SURF_NOMARKS)
//		/*&& !Waypoint_FloorSurfaceOK(tr.surfaceFlags)
//		&& !HasPortalFlags(tr.surfaceFlags, tr.contents)*/ )
//	{// Sky...
//		return -MAX_MAP_SIZE;
//	}

	if (tr.endpos[2] < -131000)
		return -MAX_MAP_SIZE;

	// UQ1: MOVER TEST
	if ( tr.fraction != 1
		&& tr.entityNum != ENTITYNUM_NONE
		&& tr.entityNum < ENTITYNUM_MAX_NORMAL )
	{
		if (NearMoverEntityLocation( tr.endpos ))
		{
			return tr.endpos[2];
		}

		if (cg_entities[tr.entityNum].currentState.eType == ET_MOVER)
		{// Hit a mover... Add waypoints at all of them!
			return tr.endpos[2];
		}
	}
	/*
	if ( tr.fraction != 1
		&& tr.entityNum != ENTITYNUM_NONE
		&& tr.entityNum < ENTITYNUM_MAX_NORMAL )
	{
		if (cg_entities[tr.entityNum].currentState.eType == ET_MOVER
			|| cg_entities[tr.entityNum].currentState.eType == ET_PUSH_TRIGGER
			|| cg_entities[tr.entityNum].currentState.eType == ET_PORTAL
			|| cg_entities[tr.entityNum].currentState.eType == ET_TELEPORT_TRIGGER
			|| cg_entities[tr.entityNum].currentState.eType == ET_TEAM
			|| cg_entities[tr.entityNum].currentState.eType == ET_TERRAIN
			|| cg_entities[tr.entityNum].currentState.eType == ET_FX
			|| HasPortalFlags(tr.surfaceFlags, tr.contents))
		{// Hit a mover... Add waypoints at all of them!
			return tr.endpos[2];
		}
	}*/

//	if ( (tr.surfaceFlags & SURF_NODRAW) && (tr.surfaceFlags & SURF_NOMARKS)
//		/*&& !Waypoint_FloorSurfaceOK(tr.surfaceFlags)
//		&& !HasPortalFlags(tr.surfaceFlags, tr.contents)*/)
//	{// Sky...
//		//trap->Print("(tr.surfaceFlags & SURF_NODRAW) && (tr.surfaceFlags & SURF_NOMARKS)\n");
//		return MAX_MAP_SIZE;
//	}

	VectorCopy(org, org1);
	org1[2]+=18;

	if (WP_CheckInSolid(org1))
	{
		return MAX_MAP_SIZE;
	}

	if (!CG_HaveRoofAbove(org1))
	{
		return MAX_MAP_SIZE;
	}

	return tr.endpos[2];
}

qboolean BadHeightNearby( vec3_t org )
{
	vec3_t org1, org2, angles, forward, right;

	VectorSet(angles, 0, 0, 0);
	VectorCopy(org, org1);
	org1[2] += 18;

	AngleVectors( angles, forward, right, NULL );

	// Check forward...
	VectorMA( org1, 192, forward, org2 );

	if (OrgVisible(org1, org2, -1))
		if (GroundHeightAt( org2 ) < org[2]-100)
			return qtrue;

	// Check back...
	VectorCopy(org, org1);
	org1[2] += 18;
	VectorMA( org1, -192, forward, org2 );

	if (OrgVisible(org1, org2, -1))
		if (GroundHeightAt( org2 ) < org[2]-100)
			return qtrue;

	// Check right...
	VectorCopy(org, org1);
	org1[2] += 18;
	VectorMA( org1, 192, right, org2 );

	if (OrgVisible(org1, org2, -1))
		if (GroundHeightAt( org2 ) < org[2]-100)
			return qtrue;

	// Check left...
	VectorCopy(org, org1);
	org1[2] += 18;
	VectorMA( org1, -192, right, org2 );

	if (OrgVisible(org1, org2, -1))
		if (GroundHeightAt( org2 ) < org[2]-100)
			return qtrue;

	// Check forward right...
	VectorCopy(org, org1);
	org1[2] += 18;
	VectorMA( org1, 192, forward, org2 );
	VectorMA( org2, 192, right, org2 );

	if (OrgVisible(org1, org2, -1))
		if (GroundHeightAt( org2 ) < org[2]-100)
			return qtrue;

	// Check forward left...
	VectorCopy(org, org1);
	org1[2] += 18;
	VectorMA( org1, 192, forward, org2 );
	VectorMA( org2, -192, right, org2 );

	if (OrgVisible(org1, org2, -1))
		if (GroundHeightAt( org2 ) < org[2]-100)
			return qtrue;

	// Check back right...
	VectorCopy(org, org1);
	org1[2] += 18;
	VectorMA( org1, -192, forward, org2 );
	VectorMA( org2, 192, right, org2 );

	if (OrgVisible(org1, org2, -1))
		if (GroundHeightAt( org2 ) < org[2]-100)
			return qtrue;

	// Check back left...
	VectorCopy(org, org1);
	org1[2] += 18;
	VectorMA( org1, -192, forward, org2 );
	VectorMA( org2, -192, right, org2 );

	if (OrgVisible(org1, org2, -1))
		if (GroundHeightAt( org2 ) < org[2]-100)
			return qtrue;

	return qfalse;
}

float ShortestWallRangeFrom ( vec3_t org )
{
	trace_t tr;
	vec3_t org1, org2;
	float dist;
	float closestRange = MAX_MAP_SIZE;

	if (!DO_OPEN_AREA_SPREAD) return 0.0;

	VectorCopy(org, org1);
	org[2] += 32.0;
	VectorCopy(org, org2);
	org2[0] += 512.0f;

	CG_Trace( &tr, org1, NULL, NULL, org2, -1, MASK_PLAYERSOLID );

	dist = Distance(tr.endpos, org);

	if (dist < closestRange)
	{
		closestRange = dist;
	}

	VectorCopy(org, org2);
	org2[0] -= 512.0f;

	CG_Trace( &tr, org1, NULL, NULL, org2, -1, MASK_PLAYERSOLID );

	dist = Distance(tr.endpos, org);

	if (dist < closestRange)
	{
		closestRange = dist;
	}

	VectorCopy(org, org2);
	org2[1] += 512.0f;

	CG_Trace( &tr, org1, NULL, NULL, org2, -1, MASK_PLAYERSOLID );

	dist = Distance(tr.endpos, org);

	if (dist < closestRange)
	{
		closestRange = dist;
	}

	VectorCopy(org, org2);
	org2[1] -= 512.0f;

	CG_Trace( &tr, org1, NULL, NULL, org2, -1, MASK_PLAYERSOLID );

	dist = Distance(tr.endpos, org);

	if (dist < closestRange)
	{
		closestRange = dist;
	}

	return closestRange;
}

qboolean MaterialIsValidForWP(int materialType)
{
	switch (materialType)
	{
	case MATERIAL_SHORTGRASS:		// 5			// manicured lawn
	case MATERIAL_LONGGRASS:		// 6			// long jungle grass
	case MATERIAL_SAND:				// 8			// sandy beach
	case MATERIAL_CARPET:			// 27			// lush carpet
	case MATERIAL_GRAVEL:			// 9			// lots of small stones
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
	case MATERIAL_RUBBER:			// 24			// hard tire like rubber
	case MATERIAL_PLASTIC:			// 25			//
	case MATERIAL_PLASTER:			// 28			// drywall style plaster
	case MATERIAL_SHATTERGLASS:		// 29			// glass with the Crisis Zone style shattering
	case MATERIAL_ARMOR:			// 30			// body armor
	case MATERIAL_ICE:				// 15			// packed snow/solid ice
	case MATERIAL_GLASS:			// 10			//
	case MATERIAL_BPGLASS:			// 18			// bulletproof glass
	case MATERIAL_TREEBARK:
	case MATERIAL_STONE:
	case MATERIAL_DISTORTEDGLASS:
	case MATERIAL_DISTORTEDPUSH:
	case MATERIAL_DISTORTEDPULL:
	case MATERIAL_CLOAK:
		return qtrue;
		break;
	case MATERIAL_ROCK:				// 23			//
		if (DO_ROCK)
			return qtrue;
		break;
	case MATERIAL_WATER:			// 13			// light covering of water on a surface
		if (DO_WATER)
			return qtrue;
		break;
	case MATERIAL_FLESH:			// 16			// hung meat, corpses in the world
	case MATERIAL_COMPUTER:			// 31			// computers/electronic equipment
		break;
	case MATERIAL_SKYSCRAPER:
		break;
	default:
		//return qtrue; // no material.. just accept...
		break;
	}

	return qfalse;
}

qboolean aw_floor_trace_hit_mover = qfalse;
int aw_floor_trace_hit_ent = -1;

float FloorHeightAt ( vec3_t org )
{
	trace_t tr;
	vec3_t org1, org2, slopeangles;
	float pitch = 0;
	qboolean HIT_WATER = qfalse;

	aw_floor_trace_hit_mover = qfalse;

	/*
	if (AIMOD_IsWaypointHeightMarkedAsBad( org ))
	{
		return MAX_MAP_SIZE;
	}
	*/

	VectorCopy(org, org1);
	org1[2]+=8;
	
	VectorCopy(org, org2);
	org2[2]= -MAX_MAP_SIZE;

	CG_Trace( &tr, org1, NULL, NULL, org2, -1, MASK_PLAYERSOLID|CONTENTS_WATER );
	//CG_Trace( &tr, org1, NULL, NULL, org2, cg.clientNum, MASK_PLAYERSOLID|CONTENTS_TRIGGER|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP|CONTENTS_SHOTCLIP|CONTENTS_NODROP|CONTENTS_TRANSLUCENT );

	if (tr.startsolid)
	{
		if (DO_WATER && (tr.contents & CONTENTS_WATER))
		{// If we started in water, do another trace ignoring water...
			VectorCopy(org, org1);
			org1[2] += 8;

			VectorCopy(org, org2);
			org2[2] = -MAX_MAP_SIZE;

			HIT_WATER = qtrue;

			CG_Trace(&tr, org1, NULL, NULL, org2, -1, MASK_PLAYERSOLID);
		}
	}

	if (tr.startsolid)
	{
		return MAX_MAP_SIZE;
	}

	if ( tr.fraction != 1
		&& tr.entityNum != ENTITYNUM_NONE
		&& tr.entityNum < ENTITYNUM_MAX_NORMAL )
	{
		if (NearMoverEntityLocation( tr.endpos ))
		{
			// Hit a mover... Add waypoints at all of them!
			aw_floor_trace_hit_mover = qtrue;
			aw_floor_trace_hit_ent = tr.entityNum;
			return tr.endpos[2];
		}

		if (cg_entities[tr.entityNum].currentState.eType == ET_MOVER
			|| cg_entities[tr.entityNum].currentState.eType == ET_PUSH_TRIGGER
			|| cg_entities[tr.entityNum].currentState.eType == ET_PORTAL
			|| cg_entities[tr.entityNum].currentState.eType == ET_TELEPORT_TRIGGER
			|| cg_entities[tr.entityNum].currentState.eType == ET_TEAM
			|| cg_entities[tr.entityNum].currentState.eType == ET_TERRAIN
			|| cg_entities[tr.entityNum].currentState.eType == ET_FX)
		{// Hit a mover... Add waypoints at all of them!
			aw_floor_trace_hit_mover = qtrue;
			aw_floor_trace_hit_ent = tr.entityNum;
			return tr.endpos[2];
		}
	}

	if (tr.endpos[2] < -131000.0f /*|| tr.endpos[2] < cg.mapcoordsMins[2]-2000*/)
	{
		return -MAX_MAP_SIZE;
	}

	if ( tr.surfaceFlags & SURF_SKY )
	{// Sky...
		//trap->Print("SURF_SKY\n");
		return MAX_MAP_SIZE;
	}

	if ( tr.contents & CONTENTS_LAVA )
	{// Lava...
		//trap->Print("CONTENTS_LAVA\n");
		return MAX_MAP_SIZE;
	}

	if ( /*(tr.contents & CONTENTS_SOLID) && (tr.contents & CONTENTS_OPAQUE) &&*/ (tr.materialType) == MATERIAL_NONE)
	{// Invisible brush?!?!? Probably skybox above or below the map...
		return MAX_MAP_SIZE;
	}

	if (!DO_WATER && /*tr.contents & CONTENTS_WATER ||*/ (tr.materialType) == MATERIAL_WATER )
	{// Water... I'm just gonna ignore these!
		//trap->Print("CONTENTS_WATER\n");
		return MAX_MAP_SIZE;
	}

	if ( !DO_ULTRAFAST && !DO_TRANSLUCENT && (tr.contents & CONTENTS_TRANSLUCENT) )
	{// Invisible surface... I'm just gonna ignore these!
		if (DO_WATER)
		{
			if (/*tr.contents & CONTENTS_WATER ||*/ (tr.materialType) == MATERIAL_WATER)
			{

			}
			else
			{
				//trap->Print("CONTENTS_TRANSLUCENT\n");
				return MAX_MAP_SIZE;
			}
		}
		else
		{
			//trap->Print("CONTENTS_TRANSLUCENT\n");
			return MAX_MAP_SIZE;
		}
	}

	if (!MaterialIsValidForWP((tr.materialType)))
	{
		if (!DO_ROCK && (tr.materialType) == MATERIAL_ROCK)
			return -MAX_MAP_SIZE;

		return MAX_MAP_SIZE;
	}

	aw_floor_trace_hit_mover = qfalse;
	aw_floor_trace_hit_ent = -1;

	// Added -- Check slope...
	vectoangles( tr.plane.normal, slopeangles );

	pitch = slopeangles[0];

	if (pitch > 180)
		pitch -= 360;

	if (pitch < -180)
		pitch += 360;

	pitch += 90.0f;

	if (pitch > MAX_SLOPE || pitch < -MAX_SLOPE)
		return MAX_MAP_SIZE; // bad slope...

	if ( tr.startsolid || tr.allsolid )
	{
		//trap->Print("ALLSOLID\n");
		return MAX_MAP_SIZE;
	}

	if (DO_THOROUGH && BadHeightNearby( org ))
	{
		//trap->Print("BAD_HEIGHT\n");
		return MAX_MAP_SIZE;
	}

	VectorCopy(org, org1);
	org1[2]+=18;

	if (WP_CheckInSolid(org1))
	{
		//trap->Print("INSOLID\n");
		return MAX_MAP_SIZE;
	}

	if (!CG_HaveRoofAbove(org1))
	{
		//trap->Print("NOROOF\n");
		return MAX_MAP_SIZE;
	}

	if (HIT_WATER)
	{// Always skip water bottom, but let the traces under it continue (eg: areas under the water)...
		return MAX_MAP_SIZE;
	}

	return tr.endpos[2];
}

float RoofHeightAt ( vec3_t org )
{
	trace_t tr;
	vec3_t org1, org2;
//	float height = 0;

	VectorCopy(org, org1);
	org1[2]+=4;

	VectorCopy(org, org2);
	org2[2]= MAX_MAP_SIZE;

	CG_Trace( &tr, org1, NULL, NULL, org2, cg.clientNum, MASK_PLAYERSOLID);
	//CG_Trace( &tr, org1, NULL, NULL, org2, cg.clientNum, MASK_PLAYERSOLID|CONTENTS_TRIGGER|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP|CONTENTS_SHOTCLIP|CONTENTS_NODROP|CONTENTS_TRANSLUCENT );

	if ( tr.startsolid || tr.allsolid )
	{
		//trap->Print("start or allsolid.\n");
		return -MAX_MAP_SIZE;
	}

	if (tr.surfaceFlags & SURF_SKY)
	{
		return -131072.f;
	}

	if ((tr.surfaceFlags & SURF_NOMARKS) && (tr.surfaceFlags & SURF_NODRAW) && (tr.contents & CONTENTS_SOLID) && (tr.contents & CONTENTS_OPAQUE))
	{
		return -131072.f;
	}

	if (!DO_ROCK && (tr.materialType) == MATERIAL_ROCK)
	{
		return -MAX_MAP_SIZE;
	}

	return tr.endpos[2];
}

#define POS_FW 0
#define POS_BC 1
#define POS_L 2
#define POS_R 3
#define POS_MAX 4

/*
	 _____
	|  0  |
    |     |
    |2   3|
	|     |
	|__1__|
*/

void CG_ShowSlope ( void ) {
	vec3_t	org;
	trace_t	 trace;
	vec3_t	forward, right, up, start, end;
	vec3_t	testangles;
	vec3_t	boxMins= {-1, -1, -1};
	vec3_t	boxMaxs= {1, 1, 1};
	float	pitch, roll, yaw, roof;
	vec3_t	slopeangles;

	VectorCopy(cg_entities[cg.clientNum].lerpOrigin, org);

	testangles[0] = testangles[1] = testangles[2] = 0;

	AngleVectors( testangles, forward, right, up );

	roof = RoofHeightAt( org );
	roof -= 16;

	VectorCopy(org, start);
	VectorMA ( start, -65000 , up , end);
	start[2] = roof;

	CG_Trace( &trace, start, NULL, NULL, end, cg.clientNum, MASK_PLAYERSOLID );

	vectoangles( trace.plane.normal, slopeangles );

	pitch = slopeangles[0];
	if (pitch > 180)
		pitch -= 360;

	if (pitch < -180)
		pitch += 360;

	pitch += 90.0f;

	yaw = slopeangles[1];
	if (yaw > 180)
		yaw -= 360;

	if (yaw < -180)
		yaw += 360;

	roll = slopeangles[2];
	if (roll > 180)
		roll -= 360;

	if (roll < -180)
		roll += 360;

	trap->Print("Slope is %f %f %f\n", pitch, yaw, roll);
}

void DebugSurfaceType( int materialType)
{
	switch( materialType )
	{
	case MATERIAL_WATER:			// 13			// light covering of water on a surface
		trap->Print("Surface material is MATERIAL_WATER.\n");
		break;
	case MATERIAL_SHORTGRASS:		// 5			// manicured lawn
		trap->Print("Surface material is MATERIAL_SHORTGRASS.\n");
		break;
	case MATERIAL_LONGGRASS:		// 6			// long jungle grass
		trap->Print("Surface material is MATERIAL_LONGGRASS.\n");
		break;
	case MATERIAL_SAND:				// 8			// sandy beach
		trap->Print("Surface material is MATERIAL_SAND.\n");
		break;
	case MATERIAL_CARPET:			// 27			// lush carpet
		trap->Print("Surface material is MATERIAL_CARPET.\n");
		break;
	case MATERIAL_GRAVEL:			// 9			// lots of small stones
		trap->Print("Surface material is MATERIAL_GRAVEL.\n");
		break;
	case MATERIAL_ROCK:				// 23			//
		trap->Print("Surface material is MATERIAL_ROCK.\n");
		break;
	case MATERIAL_STONE:
		trap->Print("Surface material is MATERIAL_STONE.\n");
		break;
	case MATERIAL_TILES:			// 26			// tiled floor
		trap->Print("Surface material is MATERIAL_TILES.\n");
		break;
	case MATERIAL_SOLIDWOOD:		// 1			// freshly cut timber
		trap->Print("Surface material is MATERIAL_SOLIDWOOD.\n");
		break;
	case MATERIAL_HOLLOWWOOD:		// 2			// termite infested creaky wood
		trap->Print("Surface material is MATERIAL_HOLLOWWOOD.\n");
		break;
	case MATERIAL_POLISHEDWOOD:
		trap->Print("Surface material is MATERIAL_POLISHEDWOOD.\n");
		break;
	case MATERIAL_SOLIDMETAL:		// 3			// solid girders
		trap->Print("Surface material is MATERIAL_SOLIDMETAL.\n");
		break;
	case MATERIAL_HOLLOWMETAL:		// 4			// hollow metal machines
		trap->Print("Surface material is MATERIAL_HOLLOWMETAL.\n");
		break;
	case MATERIAL_DRYLEAVES:		// 19			// dried up leaves on the floor
		trap->Print("Surface material is MATERIAL_DRYLEAVES.\n");
		break;
	case MATERIAL_GREENLEAVES:		// 20			// fresh leaves still on a tree
		trap->Print("Surface material is MATERIAL_GREENLEAVES.\n");
		break;
	case MATERIAL_FABRIC:			// 21			// Cotton sheets
		trap->Print("Surface material is MATERIAL_FABRIC.\n");
		break;
	case MATERIAL_CANVAS:			// 22			// tent material
		trap->Print("Surface material is MATERIAL_CANVAS.\n");
		break;
	case MATERIAL_MARBLE:			// 12			// marble floors
		trap->Print("Surface material is MATERIAL_MARBLE.\n");
		break;
	case MATERIAL_SNOW:				// 14			// freshly laid snow
		trap->Print("Surface material is MATERIAL_SNOW.\n");
		break;
	case MATERIAL_MUD:				// 17			// wet soil
		trap->Print("Surface material is MATERIAL_MUD.\n");
		break;
	case MATERIAL_DIRT:				// 7			// hard mud
		trap->Print("Surface material is MATERIAL_DIRT.\n");
		break;
	case MATERIAL_CONCRETE:			// 11			// hardened concrete pavement
		trap->Print("Surface material is MATERIAL_CONCRETE.\n");
		break;
	case MATERIAL_FLESH:			// 16			// hung meat, corpses in the world
		trap->Print("Surface material is MATERIAL_FLESH.\n");
		break;
	case MATERIAL_RUBBER:			// 24			// hard tire like rubber
		trap->Print("Surface material is MATERIAL_RUBBER.\n");
		break;
	case MATERIAL_PLASTIC:			// 25			//
		trap->Print("Surface material is MATERIAL_PLASTIC.\n");
		break;
	case MATERIAL_PLASTER:			// 28			// drywall style plaster
		trap->Print("Surface material is MATERIAL_PLASTER.\n");
		break;
	case MATERIAL_SHATTERGLASS:		// 29			// glass with the Crisis Zone style shattering
		trap->Print("Surface material is MATERIAL_SHATTERGLASS.\n");
		break;
	case MATERIAL_ARMOR:			// 30			// body armor
		trap->Print("Surface material is MATERIAL_ARMOR.\n");
		break;
	case MATERIAL_ICE:				// 15			// packed snow/solid ice
		trap->Print("Surface material is MATERIAL_ICE.\n");
		break;
	case MATERIAL_GLASS:			// 10			//
		trap->Print("Surface material is MATERIAL_GLASS.\n");
		break;
	case MATERIAL_BPGLASS:			// 18			// bulletproof glass
		trap->Print("Surface material is MATERIAL_BPGLASS.\n");
		break;
	case MATERIAL_COMPUTER:			// 31			// computers/electronic equipment
		trap->Print("Surface material is MATERIAL_COMPUTER.\n");
		break;
	case MATERIAL_TREEBARK:
		trap->Print("Surface material is MATERIAL_TREEBARK.\n");
		break;
	case MATERIAL_SKYSCRAPER:
		trap->Print("Surface material is MATERIAL_SKYSCRAPER.\n");
		break;
	case MATERIAL_DISTORTEDGLASS:
		trap->Print("Surface material is MATERIAL_DISTORTEDGLASS.\n");
		break;
	case MATERIAL_DISTORTEDPUSH:
		trap->Print("Surface material is MATERIAL_DISTORTEDPUSH.\n");
		break;
	case MATERIAL_DISTORTEDPULL:
		trap->Print("Surface material is MATERIAL_DISTORTEDPULL.\n");
		break;
	case MATERIAL_CLOAK:
		trap->Print("Surface material is MATERIAL_CLOAK.\n");
		break;
	default:
		trap->Print("Surface material is MATERIAL_NONE.\n");
		break;
	}
}

void CG_ShowForwardSurface ( void )
{
	vec3_t org, down_org, forward;
	trace_t tr;

	VectorCopy(cg.refdef.vieworg, org);
	AngleVectors( cg.refdef.viewangles, forward, NULL, NULL );
	VectorMA( cg.refdef.vieworg, MAX_MAP_SIZE, forward, down_org );

	// Do forward test...
	CG_Trace( &tr, org, NULL, NULL, down_org, cg.clientNum, MASK_ALL);//MASK_PLAYERSOLID|CONTENTS_TRIGGER|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP|CONTENTS_SHOTCLIP|CONTENTS_NODROP|CONTENTS_SHOTCLIP|CONTENTS_TRANSLUCENT );

	//
	// Shader Names
	//
	if (tr.shaderName != NULL)
	{
		trap->Print("Shader: %s.\n", tr.shaderName);
	}
	else
	{
		trap->Print("Shader: UNKNOWN.\n");
	}

	//
	// Surface
	//

	trap->Print("Current surface flags (%i):\n", tr.surfaceFlags);

/*
#define	SURF_SKY				0x00002000	// lighting from environment map
#define	SURF_SLICK				0x00004000	// affects game physics
#define	SURF_METALSTEPS			0x00008000	// CHC needs this since we use same tools (though this flag is temp?)
#define SURF_FORCEFIELD			0x00010000	// CHC ""			(but not temp)
#define	SURF_NODAMAGE			0x00040000	// never give falling damage
#define	SURF_NOIMPACT			0x00080000	// don't make missile explosions
#define	SURF_NOMARKS			0x00100000	// don't leave missile marks
#define	SURF_NODRAW				0x00200000	// don't generate a drawsurface at all
#define	SURF_NOSTEPS			0x00400000	// no footstep sounds
#define	SURF_NODLIGHT			0x00800000	// don't dlight even if solid (solid lava, skies)
#define	SURF_NOMISCENTS			0x01000000	// no client models allowed on this surface
*/

	if (tr.surfaceFlags & SURF_NODAMAGE)
		trap->Print("SURF_NODAMAGE ");

	if (tr.surfaceFlags & SURF_SLICK)
		trap->Print("SURF_SLICK ");

	if (tr.surfaceFlags & SURF_SKY)
		trap->Print("SURF_SKY ");

	if (tr.surfaceFlags & SURF_METALSTEPS)
		trap->Print("SURF_METALSTEPS ");

	if (tr.surfaceFlags & SURF_FORCEFIELD)
		trap->Print("SURF_FORCEFIELD ");

	if (tr.surfaceFlags & SURF_NOMARKS)
		trap->Print("SURF_NOMARKS ");

	if (tr.surfaceFlags & SURF_NOIMPACT)
		trap->Print("SURF_NOIMPACT ");

	if (tr.surfaceFlags & SURF_NODRAW)
		trap->Print("SURF_NODRAW ");

	if (tr.surfaceFlags & SURF_NOSTEPS)
		trap->Print("SURF_NOSTEPS ");

	if (tr.surfaceFlags & SURF_NODLIGHT)
		trap->Print("SURF_NODLIGHT ");

	if (tr.surfaceFlags & SURF_NOSTEPS)
		trap->Print("SURF_NOSTEPS ");

	if (tr.surfaceFlags & SURF_NOMISCENTS)
		trap->Print("SURF_NOMISCENTS ");

	trap->Print("\n");

	//
	// Contents...
	//

	trap->Print("Current contents flags (%i):\n", tr.surfaceFlags);

	if (tr.contents & CONTENTS_SOLID)
		trap->Print("CONTENTS_SOLID ");

	if (tr.contents & CONTENTS_LAVA)
		trap->Print("CONTENTS_LAVA ");

	if (tr.contents & CONTENTS_WATER)
		trap->Print("CONTENTS_WATER ");

	if (tr.contents & CONTENTS_FOG)
		trap->Print("CONTENTS_FOG ");

	if (tr.contents & CONTENTS_PLAYERCLIP)
		trap->Print("CONTENTS_PLAYERCLIP ");

	if (tr.contents & CONTENTS_BOTCLIP)
		trap->Print("CONTENTS_BOTCLIP ");

	if (tr.contents & CONTENTS_SHOTCLIP)
		trap->Print("CONTENTS_SHOTCLIP ");

	if (tr.contents & CONTENTS_BODY)
		trap->Print("CONTENTS_BODY ");

	if (tr.contents & CONTENTS_CORPSE)
		trap->Print("CONTENTS_CORPSE ");

	if (tr.contents & CONTENTS_TRIGGER)
		trap->Print("CONTENTS_TRIGGER ");

	if (tr.contents & CONTENTS_NODROP)
		trap->Print("CONTENTS_NODROP ");

	if (tr.contents & CONTENTS_TERRAIN)
		trap->Print("CONTENTS_TERRAIN ");

	if (tr.contents & CONTENTS_LADDER)
		trap->Print("CONTENTS_LADDER ");

	if (tr.contents & CONTENTS_ABSEIL)
		trap->Print("CONTENTS_ABSEIL ");

	if (tr.contents & CONTENTS_OPAQUE)
		trap->Print("CONTENTS_OPAQUE ");

	if (tr.contents & CONTENTS_OUTSIDE)
		trap->Print("CONTENTS_OUTSIDE ");

	if (tr.contents & CONTENTS_INSIDE)
		trap->Print("CONTENTS_INSIDE ");

	if (tr.contents & CONTENTS_SLIME)
		trap->Print("CONTENTS_SLIME ");

	if (tr.contents & CONTENTS_LIGHTSABER)
		trap->Print("CONTENTS_LIGHTSABER ");

	if (tr.contents & CONTENTS_TELEPORTER)
		trap->Print("CONTENTS_TELEPORTER ");

	if (tr.contents & CONTENTS_ITEM)
		trap->Print("CONTENTS_ITEM ");

	if (tr.contents & CONTENTS_NOSHOT)
		trap->Print("CONTENTS_NOSHOT ");

	if (tr.contents & CONTENTS_DETAIL)
		trap->Print("CONTENTS_DETAIL ");

	if (tr.contents & CONTENTS_TRANSLUCENT)
		trap->Print("CONTENTS_TRANSLUCENT ");

	trap->Print("\n");

	DebugSurfaceType( tr.materialType);

	// UQ1: May as well show the slope as well...
	CG_ShowSlope();

	trap->Print("ENTITY: %i - Type %i.\n", tr.entityNum, cg_entities[tr.entityNum].currentState.eType);
}

void CG_ShowSurface ( void )
{
	vec3_t org, down_org;
	trace_t tr;

	VectorCopy(cg_entities[cg.clientNum].lerpOrigin, org);
	VectorCopy(cg_entities[cg.clientNum].lerpOrigin, down_org);
	//down_org[2]-=48;
	//down_org[2] = -65000;
	down_org[2] = -MAX_MAP_SIZE;

	// Do forward test...
	CG_Trace( &tr, org, NULL, NULL, down_org, cg.clientNum, MASK_ALL);//MASK_PLAYERSOLID|CONTENTS_TRIGGER|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP|CONTENTS_SHOTCLIP|CONTENTS_NODROP|CONTENTS_SHOTCLIP|CONTENTS_TRANSLUCENT );

	//
	// Shader Names
	//
	if (tr.shaderName != NULL)
	{
		trap->Print("Shader: %s.\n", tr.shaderName);
	}
	else
	{
		trap->Print("Shader: UNKNOWN.\n");
	}

	//
	// Surface
	//

	trap->Print("Current surface flags (%i):\n", tr.surfaceFlags);

/*
#define	SURF_SKY				0x00002000	// lighting from environment map
#define	SURF_SLICK				0x00004000	// affects game physics
#define	SURF_METALSTEPS			0x00008000	// CHC needs this since we use same tools (though this flag is temp?)
#define SURF_FORCEFIELD			0x00010000	// CHC ""			(but not temp)
#define	SURF_NODAMAGE			0x00040000	// never give falling damage
#define	SURF_NOIMPACT			0x00080000	// don't make missile explosions
#define	SURF_NOMARKS			0x00100000	// don't leave missile marks
#define	SURF_NODRAW				0x00200000	// don't generate a drawsurface at all
#define	SURF_NOSTEPS			0x00400000	// no footstep sounds
#define	SURF_NODLIGHT			0x00800000	// don't dlight even if solid (solid lava, skies)
#define	SURF_NOMISCENTS			0x01000000	// no client models allowed on this surface
*/

	if (tr.surfaceFlags & SURF_NODAMAGE)
		trap->Print("SURF_NODAMAGE ");

	if (tr.surfaceFlags & SURF_SLICK)
		trap->Print("SURF_SLICK ");

	if (tr.surfaceFlags & SURF_SKY)
		trap->Print("SURF_SKY ");

	if (tr.surfaceFlags & SURF_METALSTEPS)
		trap->Print("SURF_METALSTEPS ");

	if (tr.surfaceFlags & SURF_FORCEFIELD)
		trap->Print("SURF_FORCEFIELD ");

	if (tr.surfaceFlags & SURF_NOMARKS)
		trap->Print("SURF_NOMARKS ");

	if (tr.surfaceFlags & SURF_NOIMPACT)
		trap->Print("SURF_NOIMPACT ");

	if (tr.surfaceFlags & SURF_NODRAW)
		trap->Print("SURF_NODRAW ");

	if (tr.surfaceFlags & SURF_NOSTEPS)
		trap->Print("SURF_NOSTEPS ");

	if (tr.surfaceFlags & SURF_NODLIGHT)
		trap->Print("SURF_NODLIGHT ");

	if (tr.surfaceFlags & SURF_NOSTEPS)
		trap->Print("SURF_NOSTEPS ");

	if (tr.surfaceFlags & SURF_NOMISCENTS)
		trap->Print("SURF_NOMISCENTS ");

	trap->Print("\n");

	//
	// Contents...
	//

	trap->Print("Current contents flags (%i):\n", tr.surfaceFlags);

	if (tr.contents & CONTENTS_SOLID)
		trap->Print("CONTENTS_SOLID ");

	if (tr.contents & CONTENTS_LAVA)
		trap->Print("CONTENTS_LAVA ");

	if (tr.contents & CONTENTS_WATER)
		trap->Print("CONTENTS_WATER ");

	if (tr.contents & CONTENTS_FOG)
		trap->Print("CONTENTS_FOG ");

	if (tr.contents & CONTENTS_PLAYERCLIP)
		trap->Print("CONTENTS_PLAYERCLIP ");

	if (tr.contents & CONTENTS_BOTCLIP)
		trap->Print("CONTENTS_BOTCLIP ");

	if (tr.contents & CONTENTS_SHOTCLIP)
		trap->Print("CONTENTS_SHOTCLIP ");

	if (tr.contents & CONTENTS_BODY)
		trap->Print("CONTENTS_BODY ");

	if (tr.contents & CONTENTS_CORPSE)
		trap->Print("CONTENTS_CORPSE ");

	if (tr.contents & CONTENTS_TRIGGER)
		trap->Print("CONTENTS_TRIGGER ");

	if (tr.contents & CONTENTS_NODROP)
		trap->Print("CONTENTS_NODROP ");

	if (tr.contents & CONTENTS_TERRAIN)
		trap->Print("CONTENTS_TERRAIN ");

	if (tr.contents & CONTENTS_LADDER)
		trap->Print("CONTENTS_LADDER ");

	if (tr.contents & CONTENTS_ABSEIL)
		trap->Print("CONTENTS_ABSEIL ");

	if (tr.contents & CONTENTS_OPAQUE)
		trap->Print("CONTENTS_OPAQUE ");

	if (tr.contents & CONTENTS_OUTSIDE)
		trap->Print("CONTENTS_OUTSIDE ");

	if (tr.contents & CONTENTS_INSIDE)
		trap->Print("CONTENTS_INSIDE ");

	if (tr.contents & CONTENTS_SLIME)
		trap->Print("CONTENTS_SLIME ");

	if (tr.contents & CONTENTS_LIGHTSABER)
		trap->Print("CONTENTS_LIGHTSABER ");

	if (tr.contents & CONTENTS_TELEPORTER)
		trap->Print("CONTENTS_TELEPORTER ");

	if (tr.contents & CONTENTS_ITEM)
		trap->Print("CONTENTS_ITEM ");

	if (tr.contents & CONTENTS_NOSHOT)
		trap->Print("CONTENTS_NOSHOT ");

	if (tr.contents & CONTENTS_DETAIL)
		trap->Print("CONTENTS_DETAIL ");

	if (tr.contents & CONTENTS_TRANSLUCENT)
		trap->Print("CONTENTS_TRANSLUCENT ");

	trap->Print("\n");

	DebugSurfaceType( tr.materialType);

	// UQ1: May as well show the slope as well...
	CG_ShowSlope();

	trap->Print("ENTITY: %i - Type %i.\n", tr.entityNum, cg_entities[tr.entityNum].currentState.eType);
}

void CG_ShowSkySurface ( void )
{
	vec3_t org, down_org;
	trace_t tr;

	VectorCopy(cg_entities[cg.clientNum].lerpOrigin, org);
	VectorCopy(cg_entities[cg.clientNum].lerpOrigin, down_org);
	//down_org[2]-=48;
	//down_org[2] = -65000;
	down_org[2] = +MAX_MAP_SIZE;

	// Do forward test...
	CG_Trace( &tr, org, NULL, NULL, down_org, cg.clientNum, MASK_PLAYERSOLID|CONTENTS_TRIGGER|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP|CONTENTS_SHOTCLIP|CONTENTS_NODROP|CONTENTS_SHOTCLIP|CONTENTS_TRANSLUCENT );

	//
	// Surface
	//

	trap->Print("Current sky surface flags (%i):\n", tr.surfaceFlags);

/*
#define	SURF_SKY				0x00002000	// lighting from environment map
#define	SURF_SLICK				0x00004000	// affects game physics
#define	SURF_METALSTEPS			0x00008000	// CHC needs this since we use same tools (though this flag is temp?)
#define SURF_FORCEFIELD			0x00010000	// CHC ""			(but not temp)
#define	SURF_NODAMAGE			0x00040000	// never give falling damage
#define	SURF_NOIMPACT			0x00080000	// don't make missile explosions
#define	SURF_NOMARKS			0x00100000	// don't leave missile marks
#define	SURF_NODRAW				0x00200000	// don't generate a drawsurface at all
#define	SURF_NOSTEPS			0x00400000	// no footstep sounds
#define	SURF_NODLIGHT			0x00800000	// don't dlight even if solid (solid lava, skies)
#define	SURF_NOMISCENTS			0x01000000	// no client models allowed on this surface
*/

	if (tr.surfaceFlags & SURF_NODAMAGE)
		trap->Print("SURF_NODAMAGE ");

	if (tr.surfaceFlags & SURF_SLICK)
		trap->Print("SURF_SLICK ");

	if (tr.surfaceFlags & SURF_SKY)
		trap->Print("SURF_SKY ");

	if (tr.surfaceFlags & SURF_METALSTEPS)
		trap->Print("SURF_METALSTEPS ");

	if (tr.surfaceFlags & SURF_FORCEFIELD)
		trap->Print("SURF_FORCEFIELD ");

	if (tr.surfaceFlags & SURF_NOMARKS)
		trap->Print("SURF_NOMARKS ");

	if (tr.surfaceFlags & SURF_NOIMPACT)
		trap->Print("SURF_NOIMPACT ");

	if (tr.surfaceFlags & SURF_NODRAW)
		trap->Print("SURF_NODRAW ");

	if (tr.surfaceFlags & SURF_NOSTEPS)
		trap->Print("SURF_NOSTEPS ");

	if (tr.surfaceFlags & SURF_NODLIGHT)
		trap->Print("SURF_NODLIGHT ");

	if (tr.surfaceFlags & SURF_NOSTEPS)
		trap->Print("SURF_NOSTEPS ");

	if (tr.surfaceFlags & SURF_NOMISCENTS)
		trap->Print("SURF_NOMISCENTS ");

	trap->Print("\n");

	//
	// Contents...
	//

	trap->Print("Current sky contents flags (%i):\n", tr.surfaceFlags);

	if (tr.contents & CONTENTS_SOLID)
		trap->Print("CONTENTS_SOLID ");

	if (tr.contents & CONTENTS_LAVA)
		trap->Print("CONTENTS_LAVA ");

	if (tr.contents & CONTENTS_WATER)
		trap->Print("CONTENTS_WATER ");

	if (tr.contents & CONTENTS_FOG)
		trap->Print("CONTENTS_FOG ");

	if (tr.contents & CONTENTS_PLAYERCLIP)
		trap->Print("CONTENTS_PLAYERCLIP ");

	if (tr.contents & CONTENTS_BOTCLIP)
		trap->Print("CONTENTS_BOTCLIP ");

	if (tr.contents & CONTENTS_SHOTCLIP)
		trap->Print("CONTENTS_SHOTCLIP ");

	if (tr.contents & CONTENTS_BODY)
		trap->Print("CONTENTS_BODY ");

	if (tr.contents & CONTENTS_CORPSE)
		trap->Print("CONTENTS_CORPSE ");

	if (tr.contents & CONTENTS_TRIGGER)
		trap->Print("CONTENTS_TRIGGER ");

	if (tr.contents & CONTENTS_NODROP)
		trap->Print("CONTENTS_NODROP ");

	if (tr.contents & CONTENTS_TERRAIN)
		trap->Print("CONTENTS_TERRAIN ");

	if (tr.contents & CONTENTS_LADDER)
		trap->Print("CONTENTS_LADDER ");

	if (tr.contents & CONTENTS_ABSEIL)
		trap->Print("CONTENTS_ABSEIL ");

	if (tr.contents & CONTENTS_OPAQUE)
		trap->Print("CONTENTS_OPAQUE ");

	if (tr.contents & CONTENTS_OUTSIDE)
		trap->Print("CONTENTS_OUTSIDE ");

	if (tr.contents & CONTENTS_INSIDE)
		trap->Print("CONTENTS_INSIDE ");

	if (tr.contents & CONTENTS_SLIME)
		trap->Print("CONTENTS_SLIME ");

	if (tr.contents & CONTENTS_LIGHTSABER)
		trap->Print("CONTENTS_LIGHTSABER ");

	if (tr.contents & CONTENTS_TELEPORTER)
		trap->Print("CONTENTS_TELEPORTER ");

	if (tr.contents & CONTENTS_ITEM)
		trap->Print("CONTENTS_ITEM ");

	if (tr.contents & CONTENTS_NOSHOT)
		trap->Print("CONTENTS_NOSHOT ");

	if (tr.contents & CONTENTS_DETAIL)
		trap->Print("CONTENTS_DETAIL ");

	if (tr.contents & CONTENTS_TRANSLUCENT)
		trap->Print("CONTENTS_TRANSLUCENT ");

	trap->Print("\n");

	// UQ1: May as well show the slope as well...
	CG_ShowSlope();

	trap->Print("ENTITY: %i - Type %i.\n", tr.entityNum, cg_entities[tr.entityNum].currentState.eType);
}

qboolean AIMod_AutoWaypoint_Check_Stepps ( vec3_t org )
{// return qtrue if NOT stepps...
	return qtrue;
/*
	trace_t		trace;
	vec3_t		testangles, forward, right, up, start, end;
	float		roof = 0.0f, last_height = 0.0f, last_diff = 0.0f, this_diff = 0.0f;
	int			i = 0, num_same_diffs = 0, num_zero_diffs = 0;

	testangles[0] = testangles[1] = testangles[2] = 0;
	AngleVectors( testangles, forward, right, up );

	roof = RoofHeightAt( org );
	roof -= 16;

	last_height = FloorHeightAt( org );

	for (i = 0; i < 64; i+=4)
	{
		VectorCopy(org, start);
		VectorMA ( start, i , forward , start);
		VectorMA ( start, -64000 , up , end);
		start[2] = roof;//RoofHeightAt( start );
		//start[2] -= 16;

		trap->CM_BoxTrace( &trace, start, end, NULL, NULL,  0, MASK_PLAYERSOLID );

		if (num_zero_diffs > 4 && num_same_diffs <= 0)
		{// Flat in this direction. Skip!
			num_same_diffs = 0;
			num_zero_diffs = 0;
			break;
		}

		if (trace.endpos[2] == last_height)
		{
			num_zero_diffs++;
			continue;
		}

		if (last_height > trace.endpos[2])
			this_diff = last_height - trace.endpos[2];
		else
			this_diff = trace.endpos[2] - last_height;

		if (this_diff > 24)
		{
			num_same_diffs = 0;
			num_zero_diffs = 0;
			break;
		}

		if (last_diff == this_diff)
			num_same_diffs++;

		last_diff = this_diff;
		last_height = trace.endpos[2];

		if (num_same_diffs > 1 && num_zero_diffs <= 0)
		{// Save thinking.. It's obviously a slope!
			num_same_diffs = 0;
			num_zero_diffs = 0;
			return qtrue;
		}

		if (num_same_diffs > 1 && num_zero_diffs > 1)
			break; // We seem to have found stepps...
	}

	if (num_same_diffs > 1 && num_zero_diffs > 1)
		return qfalse; // We seem to have found stepps...

	num_same_diffs = 0;
	num_zero_diffs = 0;
	last_height = FloorHeightAt( org );
	last_diff = 0;
	this_diff = 0;

	for (i = 0; i < 64; i+=4)
	{
		VectorCopy(org, start);
		VectorMA ( start, i , right , start);
		VectorMA ( start, -64000 , up , end);
		start[2] = roof;//RoofHeightAt( start );
		//start[2] -= 16;

		trap->CM_BoxTrace( &trace, start, end, NULL, NULL,  0, MASK_PLAYERSOLID );

		if (num_zero_diffs > 4 && num_same_diffs <= 0)
		{// Flat in this direction. Skip!
			num_same_diffs = 0;
			num_zero_diffs = 0;
			break;
		}

		if (trace.endpos[2] == last_height)
		{
			num_zero_diffs++;
			continue;
		}

		if (last_height > trace.endpos[2])
			this_diff = last_height - trace.endpos[2];
		else
			this_diff = trace.endpos[2] - last_height;

		if (this_diff > 24)
		{
			num_same_diffs = 0;
			num_zero_diffs = 0;
			break;
		}

		if (last_diff == this_diff)
			num_same_diffs++;

		last_diff = this_diff;
		last_height = trace.endpos[2];

		if (num_same_diffs > 1 && num_zero_diffs <= 0)
		{// Save thinking.. It's obviously a slope!
			num_same_diffs = 0;
			num_zero_diffs = 0;
			return qtrue;
		}

		if (num_same_diffs > 1 && num_zero_diffs > 1)
			break; // We seem to have found stepps...
	}

	if (num_same_diffs > 1 && num_zero_diffs > 1)
		return qfalse; // We seem to have found stepps...

	num_same_diffs = 0;
	num_zero_diffs = 0;
	last_height = FloorHeightAt( org );
	last_diff = 0;
	this_diff = 0;

	for (i = 0; i < 64; i+=4)
	{
		VectorCopy(org, start);
		VectorMA ( start, 0-i , forward , start);
		VectorMA ( start, -64000 , up , end);
		start[2] = roof;//RoofHeightAt( start );
		//start[2] -= 16;

		trap->CM_BoxTrace( &trace, start, end, NULL, NULL,  0, MASK_PLAYERSOLID );

		if (num_zero_diffs > 4 && num_same_diffs <= 0)
		{// Flat in this direction. Skip!
			num_same_diffs = 0;
			num_zero_diffs = 0;
			break;
		}

		if (trace.endpos[2] == last_height)
		{
			num_zero_diffs++;
			continue;
		}

		if (last_height > trace.endpos[2])
			this_diff = last_height - trace.endpos[2];
		else
			this_diff = trace.endpos[2] - last_height;

		if (this_diff > 24)
		{
			num_same_diffs = 0;
			num_zero_diffs = 0;
			break;
		}

		if (last_diff == this_diff)
			num_same_diffs++;

		last_diff = this_diff;
		last_height = trace.endpos[2];

		if (num_same_diffs > 1 && num_zero_diffs <= 0)
		{// Save thinking.. It's obviously a slope!
			num_same_diffs = 0;
			num_zero_diffs = 0;
			return qtrue;
		}

		if (num_same_diffs > 1 && num_zero_diffs > 1)
			break; // We seem to have found stepps...
	}

	if (num_same_diffs > 1 && num_zero_diffs > 1)
		return qfalse; // We seem to have found stepps...

	num_same_diffs = 0;
	num_zero_diffs = 0;
	last_height = FloorHeightAt( org );
	last_diff = 0;
	this_diff = 0;

	for (i = 0; i < 64; i+=4)
	{
		VectorCopy(org, start);
		VectorMA ( start, 0-i , right , start);
		VectorMA ( start, -64000 , up , end);
		start[2] = roof;//RoofHeightAt( start );
		//start[2] -= 16;

		trap->CM_BoxTrace( &trace, start, end, NULL, NULL,  0, MASK_PLAYERSOLID );

		if (num_zero_diffs > 4 && num_same_diffs <= 0)
		{// Flat in this direction. Skip!
			num_same_diffs = 0;
			num_zero_diffs = 0;
			break;
		}

		if (trace.endpos[2] == last_height)
		{
			num_zero_diffs++;
			continue;
		}

		if (last_height > trace.endpos[2])
			this_diff = last_height - trace.endpos[2];
		else
			this_diff = trace.endpos[2] - last_height;

		if (this_diff > 24)
		{
			num_same_diffs = 0;
			num_zero_diffs = 0;
			break;
		}

		if (last_diff == this_diff)
			num_same_diffs++;

		last_diff = this_diff;
		last_height = trace.endpos[2];

		if (num_same_diffs > 1 && num_zero_diffs <= 0)
		{// Save thinking.. It's obviously a slope!
			num_same_diffs = 0;
			num_zero_diffs = 0;
			return qtrue;
		}

		if (num_same_diffs > 1 && num_zero_diffs > 1)
			break; // We seem to have found stepps...
	}

	if (num_same_diffs > 1 && num_zero_diffs > 1)
		return qfalse; // We seem to have found stepps...

	return qtrue;
*/
}

qboolean AIMod_AutoWaypoint_Check_Slope ( vec3_t org ) {
	// UQ1: Now down in the floor check code, saves a trace per spot check...
/*	trace_t	 trace;
	vec3_t	forward, right, up, start, end;
	vec3_t	testangles;
	vec3_t	boxMins= {-1, -1, -1};
	vec3_t	boxMaxs= {1, 1, 1};
	float	pitch, roof;
	vec3_t	slopeangles;

	testangles[0] = testangles[1] = testangles[2] = 0;
	AngleVectors( testangles, forward, right, up );
	roof = org[2] + 127;

	VectorCopy(org, start);
	VectorMA ( start, -65000 , up , end);
	start[2] = roof;

	CG_Trace( &trace, start, NULL, NULL, end, cg.clientNum, MASK_PLAYERSOLID );

	vectoangles( trace.plane.normal, slopeangles );

	pitch = slopeangles[0];

	if (pitch > 180)
		pitch -= 360;

	if (pitch < -180)
		pitch += 360;

	pitch += 90.0f;

	if (pitch > MAX_SLOPE || pitch < -MAX_SLOPE)
		return qtrue;
*/
	return qfalse;
}

vec3_t fixed_position;

void RepairPosition ( intvec3_t org1 )
{
#ifdef __AW_UNUSED__
//	trace_t tr;
	vec3_t	/*newOrg, newOrg2,*/ forward, right, up;
	vec3_t	angles = { 0, 0, 0 };

	AngleVectors(angles, forward, right, up);

	// Init fixed_position
	fixed_position[0] = org1[0];
	fixed_position[1] = org1[1];
	fixed_position[2] = org1[2];

	// Prepare for forward test...
	VectorCopy( fixed_position, newOrg );
	VectorCopy( fixed_position, newOrg2 );
	VectorMA(newOrg2, 64, forward, newOrg2);

	// Do forward test...
	CG_Trace( &tr, newOrg, NULL, NULL, newOrg2, -1, MASK_PLAYERSOLID );

	if ( tr.fraction != 1 )
	{// Repair the position...
		float move_ammount = VectorDistance(tr.endpos, newOrg2);

		VectorMA(newOrg, 0-(move_ammount+1), forward, newOrg);
	}

	VectorCopy(newOrg, fixed_position);

	// Prepare for back test...
	VectorCopy( fixed_position, newOrg );
	VectorCopy( fixed_position, newOrg2 );
	VectorMA(newOrg2, -64, forward, newOrg2);

	// Do back test...
	CG_Trace( &tr, newOrg, NULL, NULL, newOrg2, -1, MASK_PLAYERSOLID );

	if ( tr.fraction != 1 )
	{// Repair the position...
		float move_ammount = VectorDistance(tr.endpos, newOrg2);

		VectorMA(newOrg, move_ammount+1, forward, newOrg);
	}

	VectorCopy(newOrg, fixed_position);

	// Prepare for right test...
	VectorCopy( fixed_position, newOrg );
	VectorCopy( fixed_position, newOrg2 );
	VectorMA(newOrg2, 64, right, newOrg2);

	// Do right test...
	CG_Trace( &tr, newOrg, NULL, NULL, newOrg2, -1, MASK_PLAYERSOLID );

	if ( tr.fraction != 1 )
	{// Repair the position...
		float move_ammount = VectorDistance(tr.endpos, newOrg2);

		VectorMA(newOrg, 0-(move_ammount+1), right, newOrg);
	}

	VectorCopy(newOrg, fixed_position);

	// Prepare for left test...
	VectorCopy( fixed_position, newOrg );
	VectorCopy( fixed_position, newOrg2 );
	VectorMA(newOrg2, -64, right, newOrg2);

	// Do left test...
	CG_Trace( &tr, newOrg, NULL, NULL, newOrg2, -1, MASK_PLAYERSOLID );

	if ( tr.fraction != 1 )
	{// Repair the position...
		float move_ammount = VectorDistance(tr.endpos, newOrg2);

		VectorMA(newOrg, move_ammount+1, right, newOrg);
	}

	VectorCopy(newOrg, fixed_position);

	// Prepare for solid test...
	VectorCopy( fixed_position, newOrg );
	VectorCopy( fixed_position, newOrg2 );
	VectorMA(newOrg2, 16, up, newOrg2);

	// Do start-solid test...
	CG_Trace( &tr, newOrg, NULL, NULL, newOrg2, -1, MASK_PLAYERSOLID );

	if ( tr.fraction != 1 || tr.startsolid || tr.contents & CONTENTS_WATER)
	{// Bad waypoint. Remove it!
		fixed_position[0] = 0.0f;
		fixed_position[1] = 0.0f;
		fixed_position[2] = -MAX_MAP_SIZE;
		return;
	}

	// New floor test...
	/*fixed_position[2]=FloorHeightAt(fixed_position)+16;

	if (fixed_position[2] == -MAX_MAP_SIZE || fixed_position[2] == MAX_MAP_SIZE)
	{// Bad waypoint. Remove it!
		fixed_position[0] = 0.0f;
		fixed_position[1] = 0.0f;
		fixed_position[2] = -MAX_MAP_SIZE;
		return;
	}*/

	//
	// Let's try also centralizing the points...
	//
/*
	// Prepare for forward test...
	VectorCopy( fixed_position, newOrg );
	VectorCopy( fixed_position, newOrg2 );
	VectorMA(newOrg2, 131072, forward, newOrg2);

	// Do forward test...
	CG_Trace( &tr, newOrg, NULL, NULL, newOrg2, -1, CONTENTS_PLAYERCLIP | MASK_SHOT | MASK_OPAQUE | CONTENTS_LAVA | MASK_WATER );

	if ( VectorDistance(newOrg, tr.endpos) < 256 )
	{// Possibly a hallway.. Can we centralize it?
		float move_ammount = VectorDistance(newOrg, tr.endpos);

		// Prepare for back test...
		VectorCopy( fixed_position, newOrg );
		VectorCopy( fixed_position, newOrg2 );
		VectorMA(newOrg2, -131072, forward, newOrg2);

		// Do back test...
		CG_Trace( &tr, newOrg, NULL, NULL, newOrg2, -1, CONTENTS_PLAYERCLIP | MASK_SHOT | MASK_OPAQUE | CONTENTS_LAVA | MASK_WATER );

		if ( VectorDistance(newOrg, tr.endpos) < 256 )
		{
			move_ammount -= VectorDistance(newOrg, tr.endpos);
			VectorMA(newOrg, move_ammount, forward, newOrg);
			VectorCopy(newOrg, fixed_position);
		}
	}

	// Prepare for right test...
	VectorCopy( fixed_position, newOrg );
	VectorCopy( fixed_position, newOrg2 );
	VectorMA(newOrg2, 64, right, newOrg2);

	// Do right test...
	CG_Trace( &tr, newOrg, NULL, NULL, newOrg2, -1, CONTENTS_PLAYERCLIP | MASK_SHOT | MASK_OPAQUE | CONTENTS_LAVA | MASK_WATER );

	if ( VectorDistance(newOrg, tr.endpos) < 256 )
	{// Possibly a hallway.. Can we centralize it?
		float move_ammount = VectorDistance(newOrg, tr.endpos);

		// Prepare for left test...
		VectorCopy( fixed_position, newOrg );
		VectorCopy( fixed_position, newOrg2 );
		VectorMA(newOrg2, -131072, right, newOrg2);

		// Do back test...
		CG_Trace( &tr, newOrg, NULL, NULL, newOrg2, -1, CONTENTS_PLAYERCLIP | MASK_SHOT | MASK_OPAQUE | CONTENTS_LAVA | MASK_WATER );

		if ( VectorDistance(newOrg, tr.endpos) < 256 )
		{
			move_ammount -= VectorDistance(newOrg, tr.endpos);
			VectorMA(newOrg, move_ammount, right, newOrg);
			VectorCopy(newOrg, fixed_position);
		}
	}*/
#endif //__AW_UNUSED__
}

vec3_t	aw_ladder_positions[MAX_NODES];
int		aw_num_ladder_positions = 0;
int		aw_total_waypoints = 0;

void AIMod_AutoWaypoint_Check_Create_Ladder_Waypoints ( vec3_t original_org, vec3_t angles )
{// UQ1: JKA -- Needed ???
	/*vec3_t			org1, org2, forward, right, up, last_endpos, ladder_pos;
	qboolean	complete = qfalse;
	vec3_t			traceMins = { -20, -20, -1 };
	vec3_t			traceMaxs = { 20, 20, 32 };

	VectorCopy(original_org, org1);
	//org1[2] = FloorHeightAt(org1);

	VectorCopy(original_org, org2);
	AngleVectors( angles, forward, right, up );
	VectorMA ( org2, waypoint_scatter_distance*3 , forward , org2);
	//org2[2] = FloorHeightAt(org2);

	while (!complete)
	{
		trace_t		trace;

		trap->CM_BoxTrace( &trace, org1, org2, traceMins, traceMaxs,  0, MASK_PLAYERSOLID );

		if (trace.surfaceFlags & SURF_LADDER)
		{// Ladder found here! Add a new waypoint!
			short int	objNum[3] = { 0, 0, 0 };

			VectorCopy(trace.endpos, ladder_pos);
			Load_AddNode( ladder_pos, 0, objNum, 0 );	//add the node
			aw_total_waypoints++;
			VectorCopy(ladder_pos, last_endpos);
		}
		else
		{// We found the top of the ladder... Add 1 more waypoint and we are done! :)
			short int	objNum[3] = { 0, 0, 0 };

			last_endpos[2]+=16;

			Load_AddNode( last_endpos, 0, objNum, 0 );	//add the node
			aw_total_waypoints++;

			complete = qtrue;
		}

		org1[2]+=16;
		org2[2]+=16;
	}*/
}
/*
vec3_t aw_ladder_origin;
vec3_t aw_ladder_angles;

void AIMod_AutoWaypoint_Best_Ladder_Side ( vec3_t org )
{
	vec3_t			org1, org2, forward, right, up, testangles, best_dir;
	int				i = 0, j = 0;
	vec3_t			traceMins = { -20, -20, -1 };
	vec3_t			traceMaxs = { 20, 20, 32 };
	float			best_height = -64000.0f;
	vec3_t			best_positions[MAX_NODES];
	int				num_best_positions = 0;

	VectorCopy(org, org1);
	VectorCopy(org, org2);

	for (i = 0; i <= 360; i+=1)
	{
		trace_t		trace;

		VectorCopy(org, org1);

		testangles[0] = testangles[1] = testangles[2] = 0;
		testangles[YAW] = (float)i;
		AngleVectors( testangles, forward, right, up );
		VectorMA ( org1, 40 , forward , org1);

		VectorCopy(org1, org2);
		org2[2]+=8192;

		trap->CM_BoxTrace( &trace, org1, org2, traceMins, traceMaxs,  0, MASK_PLAYERSOLID );

		if (trace.endpos[2] > best_height)
		{
			best_height = trace.endpos[2];
			VectorCopy(org1, best_positions[0]);
			num_best_positions = 1;
		}
		else if (trace.endpos[2] == best_height)
		{
			VectorCopy(org1, best_positions[num_best_positions]);
			num_best_positions++;
		}
	}

	if (num_best_positions == 1)
	{
		VectorCopy(best_positions[0], aw_ladder_origin);
	}
	else
	{// Use most central one...
		int choice = (num_best_positions-1)*0.5;
		VectorCopy(best_positions[choice], aw_ladder_origin);
	}

	VectorSubtract(org, aw_ladder_origin, best_dir);
	vectoangles(best_dir, aw_ladder_angles);
}*/

void AIMod_AutoWaypoint_Check_For_Ladders ( vec3_t org )
{// UQ1: JKA -- Needed ???
/*	vec3_t			org1, org2, forward, right, up, testangles;
	int				i = 0;//, j = 0;
//	vec3_t			traceMins = { -20, -20, -1 };
//	vec3_t			traceMaxs = { 20, 20, 32 };

	for ( i = 0; i < aw_num_ladder_positions; i++)
	{// Do a quick check to make sure we do not do the same ladder twice!
		if (DistanceHorizontal(org, aw_ladder_positions[i]) < 128)
			return;
	}

	VectorCopy(org, org1);
	org1[2]+=16;

	for (i = 0; i < 360; i++)
	{
		trace_t		trace;

		VectorCopy(org, org2);

		testangles[0] = testangles[1] = testangles[2] = 0;
		testangles[YAW] = (float)i;
		AngleVectors( testangles, forward, right, up );
		VectorMA ( org2, waypoint_scatter_distance*3 , forward , org2);
		org2[2]+=16;

		trap->CM_BoxTrace( &trace, org1, org2, NULL, NULL,  0, MASK_PLAYERSOLID );

		if (trace.surfaceFlags & SURF_LADDER)
		{// Ladder found here! Make the waypoints!
			strcpy( last_node_added_string, va("^5Adding ladder (^3%i^5) waypoints at ^7%f %f %f^5.", aw_num_ladder_positions+1, trace.endpos[0], trace.endpos[1], trace.endpos[2]) );
			AIMod_AutoWaypoint_Check_Create_Ladder_Waypoints(org1, testangles);
			VectorCopy(trace.endpos, aw_ladder_positions[aw_num_ladder_positions]);
			aw_num_ladder_positions++;
			return; // Not likely more ladders here!
		}
	}*/
}

void AIMod_AutoWaypoint_Init_Memory ( void ); // below...
void AIMod_AutoWaypoint_Free_Memory ( void ); // below...

qboolean ContentsOK ( int contents )
{
	if (contents & CONTENTS_DETAIL || contents & CONTENTS_NODROP )
		return qfalse;

	return qtrue;
}

clock_t BOUNDS_CHECK_TIME = 0;

void
AIMod_GetMapBounts ( void )
{
	//int		i;
	float	startx = -MAX_MAP_SIZE, starty = -MAX_MAP_SIZE, startz = -MAX_MAP_SIZE;
	float	highest_z_point = -MAX_MAP_SIZE;
	float	INCRUMENT = 128.0;//192.0;//128.0; //64.0;// 256.0
	//trace_t tr;
	//vec3_t	org1;
	//vec3_t	org2;
	vec3_t	mapMins, mapMaxs;

	if (cg.mapcoordsValid) return; // No point doing it twice...

	if (BOUNDS_CHECK_TIME > clock())
	{// Wait until next check time... We are waiting on the renderer to write the bounds to the mapInfo file...
		return;
	}

	BOUNDS_CHECK_TIME = clock() + 5000;

	//
	// Try to load previously stored bounds from .mapInfo file...
	//

	mapMins[0] = atof(IniRead(va("maps/%s.mapInfo", cgs.currentmapname), "BOUNDS", "MINS0", "999999.0"));
	
	if (mapMins[0] == 999999.0)
		return; // Looks like they don't exist yet, skip until it does...

	mapMins[1] = atof(IniRead(va("maps/%s.mapInfo", cgs.currentmapname), "BOUNDS", "MINS1", "999999.0"));
	mapMins[2] = atof(IniRead(va("maps/%s.mapInfo", cgs.currentmapname), "BOUNDS", "MINS2", "999999.0"));

	mapMaxs[0] = atof(IniRead(va("maps/%s.mapInfo", cgs.currentmapname), "BOUNDS", "MAXS0", "-999999.0"));
	mapMaxs[1] = atof(IniRead(va("maps/%s.mapInfo", cgs.currentmapname), "BOUNDS", "MAXS1", "-999999.0"));
	mapMaxs[2] = atof(IniRead(va("maps/%s.mapInfo", cgs.currentmapname), "BOUNDS", "MAXS2", "-999999.0"));

	if (mapMins[0] < 999999.0 && mapMins[1] < 999999.0 && mapMins[2] < 999999.0
		&& mapMaxs[0] > -999999.0 && mapMaxs[1] > -999999.0 && mapMaxs[2] > -999999.0)
	{
		VectorCopy(mapMins, cg.mapcoordsMins);
		VectorCopy(mapMaxs, cg.mapcoordsMaxs);
		cg.mapcoordsValid = qtrue;
		return;
	}

#if 0 // Moved to renderer...
	//
	// No map bounds info available in the .mapInfo file? OK, calculate and save for next time...
	//

	aw_percent_complete = 0;

	trap->Print(va("^1*** ^3%s^5: Searching for map bounds.\n", "MAP-BOUNDS"));
	strcpy(task_string1, va("^1*** ^3%s^5: Searching for map bounds.\n", "MAP-BOUNDS"));
	trap->UpdateScreen();

	strcpy( task_string2, va("") );
	trap->UpdateScreen();

	strcpy( task_string3, va("") );
	trap->UpdateScreen();

	trap->UpdateScreen();

	VectorSet(mapMins, MAX_MAP_SIZE, MAX_MAP_SIZE, MAX_MAP_SIZE);
	VectorSet(mapMaxs, -MAX_MAP_SIZE, -MAX_MAP_SIZE, -MAX_MAP_SIZE);

	aw_percent_complete = 1.0;
	trap->UpdateScreen();

	//
	// Z
	//
	i = 0;
	while ( startx < MAX_MAP_SIZE )
	{
		while ( starty < MAX_MAP_SIZE )
		{
			VectorSet( org1, startx, starty, startz );
			VectorSet( org2, startx, starty, startz );
			org2[2] += (MAX_MAP_SIZE*2);
			CG_Trace( &tr, org1, NULL, NULL, org2, ENTITYNUM_NONE, MASK_PLAYERSOLID /*| MASK_WATER*/ );

			if ( tr.endpos[2] < mapMins[2] )
			{
				mapMins[2] = tr.endpos[2];
				starty += INCRUMENT;
				continue;
			}

			starty += INCRUMENT;
		}

		startx += INCRUMENT;
		starty = -MAX_MAP_SIZE;
	}

	aw_percent_complete = 100.0 / 6.0;
	trap->UpdateScreen();

	//Com_Printf("Stage 1 completed.\n");

	mapMins[2] += 16;
	startx = MAX_MAP_SIZE;
	starty = MAX_MAP_SIZE;
	startz = MAX_MAP_SIZE;
	i = 0;
	while ( startx > -MAX_MAP_SIZE )
	{
		while ( starty > -MAX_MAP_SIZE )
		{
			VectorSet( org1, startx, starty, startz );
			VectorSet( org2, startx, starty, startz );
			org2[2] -= (MAX_MAP_SIZE*2);
			CG_Trace( &tr, org1, NULL, NULL, org2, ENTITYNUM_NONE, MASK_PLAYERSOLID /*| MASK_WATER*/ );
			if ( tr.endpos[2] > mapMaxs[2] )
			{
				mapMaxs[2] = tr.endpos[2];
				starty -= INCRUMENT;
				continue;
			}

			starty -= INCRUMENT;
		}

		startx -= INCRUMENT;
		starty = MAX_MAP_SIZE;
	}

	aw_percent_complete = (100.0 / 6.0) * 2.0;
	trap->UpdateScreen();

	//Com_Printf("Stage 2 completed.\n");

	mapMaxs[2] -= 16;

	//
	// X
	//
	startx = -MAX_MAP_SIZE;
	starty = -MAX_MAP_SIZE;
	startz = mapMins[2];
	i = 0;
	while ( startz < mapMaxs[2] )
	{
		while ( starty < MAX_MAP_SIZE )
		{
			VectorSet( org1, startx, starty, startz );
			VectorSet( org2, startx, starty, startz );
			org2[0] += (MAX_MAP_SIZE*2);
			CG_Trace( &tr, org1, NULL, NULL, org2, ENTITYNUM_NONE, MASK_PLAYERSOLID /*| MASK_WATER*/ );
			if ( tr.endpos[0] < mapMins[0] )
			{
				starty += INCRUMENT;
				mapMins[0] = tr.endpos[0];
				continue;
			}

			starty += INCRUMENT;
		}

		startz += INCRUMENT;
		starty = -MAX_MAP_SIZE;
	}

	aw_percent_complete = (100.0 / 6.0) * 3.0;
	trap->UpdateScreen();

	//Com_Printf("Stage 3 completed.\n");

	mapMins[0] += 16;
	startx = MAX_MAP_SIZE;
	starty = MAX_MAP_SIZE;
	startz = mapMaxs[2];
	i = 0;
	while ( startz > mapMins[2] )
	{
		while ( starty > -MAX_MAP_SIZE )
		{
			VectorSet( org1, startx, starty, startz );
			VectorSet( org2, startx, starty, startz );
			org2[0] -= (MAX_MAP_SIZE*2);
			CG_Trace( &tr, org1, NULL, NULL, org2, ENTITYNUM_NONE, MASK_PLAYERSOLID /*| MASK_WATER*/ );
			if ( tr.endpos[0] > mapMaxs[0] )
			{
				mapMaxs[0] = tr.endpos[0];
				starty -= INCRUMENT;
				continue;
			}

			starty -= INCRUMENT;
		}

		startz -= INCRUMENT;
		starty = MAX_MAP_SIZE;
	}

	aw_percent_complete = (100.0 / 6.0) * 4.0;
	trap->UpdateScreen();

	//Com_Printf("Stage 4 completed.\n");

	mapMaxs[0] -= 16;

	//
	// Y
	//
	startx = -MAX_MAP_SIZE;
	starty = -MAX_MAP_SIZE;
	startz = mapMins[2];
	i = 0;
	while ( startz < mapMaxs[2] )
	{
		while ( startx < MAX_MAP_SIZE )
		{
			VectorSet( org1, startx, starty, startz );
			VectorSet( org2, startx, starty, startz );
			org2[1] += (MAX_MAP_SIZE*2);
			CG_Trace( &tr, org1, NULL, NULL, org2, ENTITYNUM_NONE, MASK_PLAYERSOLID /*| MASK_WATER*/ );
			if ( tr.endpos[1] < mapMins[1] )
			{
				mapMins[1] = tr.endpos[1];
				startx += INCRUMENT;
				continue;
			}

			startx += INCRUMENT;
		}

		startz += INCRUMENT;
		startx = -MAX_MAP_SIZE;
	}

	aw_percent_complete = (100.0 / 6.0) * 5.0;
	trap->UpdateScreen();

	//Com_Printf("Stage 5 completed.\n");

	mapMins[1] += 16;
	startx = MAX_MAP_SIZE;
	starty = MAX_MAP_SIZE;
	startz = mapMaxs[2];
	i = 0;
	while ( startz > mapMins[2] )
	{
		while ( startx > -MAX_MAP_SIZE )
		{
			VectorSet( org1, startx, starty, startz );
			VectorSet( org2, startx, starty, startz );
			org2[1] -= (MAX_MAP_SIZE*2);
			CG_Trace( &tr, org1, NULL, NULL, org2, ENTITYNUM_NONE, MASK_PLAYERSOLID /*| MASK_WATER*/ );
			if ( tr.endpos[1] > mapMaxs[1] )
			{
				mapMaxs[1] = tr.endpos[1];
				startx -= INCRUMENT;
				continue;
			}

			startx -= INCRUMENT;
		}

		startz -= INCRUMENT;
		startx = MAX_MAP_SIZE;
	}

	aw_percent_complete = 100.0;
	trap->UpdateScreen();

	//Com_Printf("Stage 6 completed.\n");

	mapMaxs[1] -= 16;

	highest_z_point = mapMaxs[2];

	//Com_Printf("Stage 7 completed.\n");

	if ( highest_z_point <= mapMins[2] )
	{
		highest_z_point = mapMaxs[2] - 32;
	}

	if ( highest_z_point <= mapMins[2] + 128 )
	{
		highest_z_point = mapMaxs[2] - 32;
	}

	mapMaxs[2] = highest_z_point;

	if (mapMaxs[0] < mapMins[0])
	{
		float temp = mapMins[0];
		mapMins[0] = mapMaxs[0];
		mapMaxs[0] = temp;
	}

	if (mapMaxs[1] < mapMins[1])
	{
		float temp = mapMins[1];
		mapMins[1] = mapMaxs[1];
		mapMaxs[1] = temp;
	}

	if (mapMaxs[2] < mapMins[2])
	{
		float temp = mapMins[2];
		mapMins[2] = mapMaxs[2];
		mapMaxs[2] = temp;
	}

	//trap->Print( "^4*** ^3MAP-BOUNDS^4: ^7Map mins %f %f %f.\n", mapMins[0], mapMins[1], mapMins[2]);
	//trap->Print( "^4*** ^3MAP-BOUNDS^4: ^7Map maxs %f %f %f.\n", mapMaxs[0], mapMaxs[1], mapMaxs[2]);

	VectorCopy(mapMins, cg.mapcoordsMins);
	VectorCopy(mapMaxs, cg.mapcoordsMaxs);
	cg.mapcoordsValid = qtrue;

	aw_percent_complete = 0.0;
	trap->UpdateScreen();

	//
	// Write newly created info to our map's .mapInfo file for future usage...
	//

	IniWrite(va("maps/%s.mapInfo", cgs.currentmapname), "BOUNDS", "MINS0", va("%f", mapMins[0]));
	IniWrite(va("maps/%s.mapInfo", cgs.currentmapname), "BOUNDS", "MINS1", va("%f", mapMins[1]));
	IniWrite(va("maps/%s.mapInfo", cgs.currentmapname), "BOUNDS", "MINS2", va("%f", mapMins[2]));

	IniWrite(va("maps/%s.mapInfo", cgs.currentmapname), "BOUNDS", "MAXS0", va("%f", mapMaxs[0]));
	IniWrite(va("maps/%s.mapInfo", cgs.currentmapname), "BOUNDS", "MAXS1", va("%f", mapMaxs[1]));
	IniWrite(va("maps/%s.mapInfo", cgs.currentmapname), "BOUNDS", "MAXS2", va("%f", mapMaxs[2]));
#endif
}


extern qboolean FOLIAGE_TreeSolidBlocking_AWP(vec3_t moveOrg);

void AIMod_AutoWaypoint_StandardMethod( void )
{// Advanced method for multi-level maps...
	int			i;
	float		startx = -MAX_MAP_SIZE, starty = -MAX_MAP_SIZE, startz = -MAX_MAP_SIZE;
	int			areas = 0, total_waypoints = 0, total_areas = 0;
	intvec3_t	*arealist;
	float		map_size, temp, original_waypoint_scatter_distance = waypoint_scatter_distance;
	vec3_t		mapMins, mapMaxs;
	int			total_tests = 0, final_tests = 0;
	int			start_time = trap->Milliseconds();
	int			update_timer = 0;
	float		waypoint_scatter_realtime_modifier = 1.0f;
	float		waypoint_scatter_realtime_modifier_alt = 0.5f; // 0.3f;
	int			wp_loop = 0;
	float		remove_ratio = 1.0;

	vec3_t		MAP_INFO_SIZE;
	clock_t		previous_time = 0;
	float		offsetY = 0.0;
	float		yoff;
	float		density = waypoint_scatter_distance;
	int x;
	float y, z;


	trap->Cvar_Set("warzone_waypoint_render", "0");
	trap->UpdateScreen();
	trap->UpdateScreen();
	trap->UpdateScreen();

	AIMod_AutoWaypoint_Init_Memory();
	AIMod_GetMapBounts();

	if (!cg.mapcoordsValid)
	{
		trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^7Map Coordinates are invalid. Can not use auto-waypointer!\n");
		return;
	}

	trap->UpdateScreen();

	arealist = (intvec3_t*)malloc((sizeof(intvec3_t)+1)*MAX_TEMP_AREAS/*512000*/);

	VectorCopy(cg.mapcoordsMins, mapMins);
	VectorCopy(cg.mapcoordsMaxs, mapMaxs);

	float hZp = atof(IniRead(va("maps/%s.mapInfo", cgs.currentmapname), "BOUNDS", "HIGHEST_SURFACE", "999999.0"));

	if (hZp >= 999999.0)
	{
		AWP_UpdatePercentBar(0.01, "Refining map coordinates.", "", "");
		trap->UpdateScreen();

		{
			//
			// Refine Z Top Point to highest ground height!
			//
			float highest_z_point = mapMins[2];
			float INCRUMENT = 128.0;
			float prevPercDone = 0.0;

			startx = mapMaxs[0] - 32;
			starty = mapMaxs[1] - 32;
			startz = mapMaxs[2] - 32;
			highest_z_point = mapMins[2];
			i = 0;

			float totalSize = startx - mapMins[2];

			while (startx > mapMins[2])
			{
				float percDone = ((totalSize - (startx - mapMins[2])) / totalSize) * 100.0; // WTB: for loop :)

				if (percDone - prevPercDone > 0.02)
				{
					AWP_UpdatePercentBarOnly(percDone);
					prevPercDone = percDone;
					trap->UpdateScreen();
				}

				while (starty > mapMins[1])
				{
					vec3_t		org1, org2;
					trace_t		tr;

					VectorSet(org1, startx, starty, startz);
					VectorSet(org2, startx, starty, startz);
					org2[2] -= (MAX_MAP_SIZE * 2);

					CG_Trace(&tr, org1, NULL, NULL, org2, ENTITYNUM_NONE, MASK_PLAYERSOLID /*| MASK_WATER*/);

					if (tr.endpos[2] > highest_z_point)
					{
						highest_z_point = tr.endpos[2];
						starty -= INCRUMENT;
						continue;
					}

					starty -= INCRUMENT / 4.0; //64
				}

				startx -= INCRUMENT / 4.0; //64
				starty = mapMaxs[1];
			}

			mapMaxs[2] = highest_z_point + 256;
		}

		// Save to mapInfo file for next time... :)
		IniWrite(va("maps/%s.mapInfo", cgs.currentmapname), "BOUNDS", "HIGHEST_SURFACE", va("%f", mapMaxs[2]));
	}
	else
	{// Not your first time here, huh? That should save us some time... :)
		mapMaxs[2] = hZp;
	}
	
	AWP_UpdatePercentBar2(0, "", "", "");

	if (mapMaxs[0] < mapMins[0])
	{
		temp = mapMins[0];
		mapMins[0] = mapMaxs[0];
		mapMaxs[0] = temp;
	}

	if (mapMaxs[1] < mapMins[1])
	{
		temp = mapMins[1];
		mapMins[1] = mapMaxs[1];
		mapMaxs[1] = temp;
	}

	if (mapMaxs[2] < mapMins[2])
	{
		temp = mapMins[2];
		mapMins[2] = mapMaxs[2];
		mapMaxs[2] = temp;
	}

	if (DO_NEW_METHOD)
	{
		trap->S_Shutup(qtrue);

		startx = mapMaxs[0];
		starty = mapMaxs[1];
		startz = mapMaxs[2];

		map_size = Distance(mapMins, mapMaxs);

		trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Map bounds are ^3%.2f %.2f %.2f ^5to ^3%.2f %.2f %.2f^5.\n", mapMins[0], mapMins[1], mapMins[2], mapMaxs[0], mapMaxs[1], mapMaxs[2]) );
		strcpy( task_string1, va("^5Map bounds are ^3%.2f %.2f %.2f ^7to ^3%.2f %.2f %.2f^5.", mapMins[0], mapMins[1], mapMins[2], mapMaxs[0], mapMaxs[1], mapMaxs[2]) );
		trap->UpdateScreen();

		trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Generating waypoints. This could take a while... (Map size ^3%.2f^5)\n", map_size) );
		strcpy( task_string2, va("^5Generating waypoints. This could take a while... (Map size ^3%.2f^5)", map_size) );
		trap->UpdateScreen();

		trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Finding waypoints...\n") );
		strcpy( task_string3, va("^5Finding waypoints...") );
		trap->UpdateScreen();

		//
		// Create bulk temporary nodes...
		//

		previous_time = clock();
		aw_stage_start_time = clock();
		aw_percent_complete = 0;

		// Create the map...
		MAP_INFO_SIZE[0] = mapMaxs[0] - mapMins[0];
		MAP_INFO_SIZE[1] = mapMaxs[1] - mapMins[1];
		MAP_INFO_SIZE[2] = mapMaxs[2] - mapMins[2];

		yoff = density * 0.333;

		//#pragma omp parallel for schedule(dynamic)
		for (x = (int)mapMins[0]; x <= (int)mapMaxs[0]; x += density)
		{
			float current, complete;

			if (areas >= MAX_TEMP_AREAS)
			{
				continue;
			}

			current =  MAP_INFO_SIZE[0] - (mapMaxs[0] - (float)x);
			complete = current / MAP_INFO_SIZE[0];

			aw_percent_complete = (float)(complete * 100.0);

			if (yoff == density * 0.333)
				yoff = density * 0.666;
			else if (yoff == density * 0.666)
				yoff = density;
			else if (yoff == density)
				yoff = density * 1.333;
			else if (yoff == density * 1.333)
				yoff = density * 1.666;
			else
				yoff = density * 0.333;

			for (y = mapMins[1]; y <= mapMaxs[1]; y += yoff/*density*/)
			{
				if (areas >= MAX_TEMP_AREAS)
				{
					break;
				}

				for (z = mapMaxs[2]; z >= mapMins[2]; z -= 48.0)
				{
					trace_t		tr;
					vec3_t		pos, pos2, down, slopeangles;
					float		pitch;
					qboolean	FOUND = qfalse;

					if (areas >= MAX_TEMP_AREAS)
					{
						break;
					}

					if(omp_get_thread_num() == 0)
					{// Draw a nice little progress bar ;)
						if (clock() - previous_time > 500) // update display every 500ms...
						{
							previous_time = clock();
							trap->UpdateScreen();
						}
					}

					VectorSet(pos, x, y, z);
					pos[2] += 8.0;
					VectorCopy(pos, down);
					down[2] = mapMins[2];

					CG_Trace( &tr, pos, NULL, NULL, down, ENTITYNUM_NONE, MASK_PLAYERSOLID|CONTENTS_WATER );

					if (tr.endpos[2] < mapMins[2])
					{// Went off map...
						break;
					}

					if (tr.endpos[2] > mapMaxs[2])
					{// Went off map...
						break;
					}

					if (tr.startsolid || tr.allsolid)
					{
						continue;
					}

					if ( tr.surfaceFlags & SURF_SKY )
					{// Sky...
						continue;
					}

					if ( tr.surfaceFlags & SURF_NODRAW )
					{// don't generate a drawsurface at all
						continue;
					}

					if ( !DO_WATER && (tr.contents & CONTENTS_WATER) )
					{// Anything below here is underwater...
						continue;
					}

					if ( tr.contents & CONTENTS_LAVA )
					{// Anything below here is underwater...
						continue;
					}

					if ( (tr.contents & CONTENTS_TRANSLUCENT) && !(DO_WATER && (tr.contents & CONTENTS_WATER)))
					{// Invisible surface... I'm just gonna ignore these!
						continue;
					}

					// Check slope...
					vectoangles( tr.plane.normal, slopeangles );

					pitch = slopeangles[0];

					if (pitch > 180)
						pitch -= 360;

					if (pitch < -180)
						pitch += 360;

					pitch += 90.0f;

					if (pitch > MAX_SLOPE || pitch < -MAX_SLOPE)
					{// Bad slope...
						continue;
					}

					VectorCopy(tr.endpos, pos2);
					pos2[2]+=8.0;

					if (WP_CheckInSolid(pos2))
					{
						continue;
					}

					if (!CG_HaveRoofAbove(pos2))
					{
						continue;
					}

					/*if (!AIMod_AutoWaypoint_Check_PlayerWidth(pos2))
					{
						continue;
					}*/

					if ( tr.fraction != 1
						&& tr.entityNum != ENTITYNUM_NONE
						&& tr.entityNum < ENTITYNUM_MAX_NORMAL )
					{
						if (cg_entities[tr.entityNum].currentState.eType == ET_MOVER
							|| cg_entities[tr.entityNum].currentState.eType == ET_PUSH_TRIGGER
							|| cg_entities[tr.entityNum].currentState.eType == ET_PORTAL
							|| cg_entities[tr.entityNum].currentState.eType == ET_TELEPORT_TRIGGER
							|| cg_entities[tr.entityNum].currentState.eType == ET_TEAM
							|| cg_entities[tr.entityNum].currentState.eType == ET_TERRAIN
							|| cg_entities[tr.entityNum].currentState.eType == ET_FX)
						{// Hit a mover... Add waypoints at all of them!
							aw_floor_trace_hit_mover = qtrue;
							aw_floor_trace_hit_ent = tr.entityNum;
#pragma omp critical (__ADD_TEMP_NODE__)
							{
								sprintf(last_node_added_string, "^5Adding waypoint ^3%i ^5at ^7%f %f %f^5.", areas, tr.endpos[0], tr.endpos[1], tr.endpos[2]+8);

								arealist[areas][0] = tr.endpos[0];
								arealist[areas][1] = tr.endpos[1];
								arealist[areas][2] = tr.endpos[2]+8;
								areas++;
							}
							continue;
						}

						if (NearMoverEntityLocation( tr.endpos ))
						{
							// Hit a mover... Add waypoints at all of them!
							aw_floor_trace_hit_mover = qtrue;
							aw_floor_trace_hit_ent = tr.entityNum;
#pragma omp critical (__ADD_TEMP_NODE__)
							{
								sprintf(last_node_added_string, "^5Adding waypoint ^3%i ^5at ^7%f %f %f^5.", areas, tr.endpos[0], tr.endpos[1], tr.endpos[2]+8);

								arealist[areas][0] = tr.endpos[0];
								arealist[areas][1] = tr.endpos[1];
								arealist[areas][2] = tr.endpos[2]+8;
								areas++;
							}
						}
					}

					if (MaterialIsValidForWP((tr.materialType)))
					{
						if (FOLIAGE_TreeSolidBlocking_AWP(tr.endpos))
						{// There is a tree in this position, skip it...
							continue;
						}

						// Look around here for a different slope angle... Cull if found...
#pragma omp critical (__ADD_TEMP_NODE__)
						{
							sprintf(last_node_added_string, "^5Adding waypoint ^3%i ^5at ^7%f %f %f^5.", areas, tr.endpos[0], tr.endpos[1], tr.endpos[2]+8);

							arealist[areas][0] = tr.endpos[0];
							arealist[areas][1] = tr.endpos[1];
							arealist[areas][2] = tr.endpos[2]+8;
							areas++;
						}
					}
				}
			}
		}

		if (areas >= MAX_TEMP_AREAS)
		{
			trap->Print( "^1*** ^3%s^5: Too many waypoints detected... Try again with a higher density value...\n", "AUTO-WAYPOINTER" );
			aw_percent_complete = 0.0f;
			trap->S_Shutup(qfalse);
			AIMod_AutoWaypoint_Free_Memory();
			return;
		}

		trap->S_Shutup(qfalse);

		aw_percent_complete = 0.0f;
	}
	else
	{
		//int		x;
		int		parallel_x = 0;
		int		parallel_x_max = 0;
		int		parallel_y_max = 0;
		float	scatter = 0;
		float	scatter_avg = 0;
		float	scatter_min = 0;
		float	scatter_z = 0;
		float	scatter_max = 0;
		float	scatter_x = 0;
		clock_t	previous_time = 0;
		qboolean sjc_jkg_preview = qfalse;
		float	offsetY = 0.0;

		// This is just for the sjc_jkg_preview incomplete map with large bad surfaces... Grrr....
		if (!Q_stricmpn("sjc_jkg_preview", cgs.currentmapname, 15)) sjc_jkg_preview = qtrue;

		trap->S_Shutup(qtrue);

		mapMaxs[0]+=2048;
		mapMaxs[1]+=2048;
		mapMaxs[2]+=2048;

		mapMins[0]-=2048;
		mapMins[1]-=2048;
		mapMins[2]-=2048;

		startx = mapMaxs[0];
		starty = mapMaxs[1];
		startz = mapMaxs[2];

		map_size = VectorDistance(mapMins, mapMaxs);

		trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Map bounds are ^3%.2f %.2f %.2f ^5to ^3%.2f %.2f %.2f^5.\n", mapMins[0], mapMins[1], mapMins[2], mapMaxs[0], mapMaxs[1], mapMaxs[2]) );
		strcpy( task_string1, va("^5Map bounds are ^3%.2f %.2f %.2f ^7to ^3%.2f %.2f %.2f^5.", mapMins[0], mapMins[1], mapMins[2], mapMaxs[0], mapMaxs[1], mapMaxs[2]) );
		trap->UpdateScreen();

		trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Generating AI waypoints. This could take a while... (Map size ^3%.2f^5)\n", map_size) );
		strcpy( task_string2, va("^5Generating AI waypoints. This could take a while... (Map size ^3%.2f^5)", map_size) );
		trap->UpdateScreen();

		trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5First pass. Finding temporary waypoints...\n") );
		strcpy( task_string3, va("^5First pass. Finding temporary waypoints...") );
		trap->UpdateScreen();

		//
		// Create bulk temporary nodes...
		//

		//scatter = (waypoint_scatter_distance / (32000 / map_size));
		// Let's vary the scatter distances :)
		scatter = waypoint_scatter_distance;
		scatter_min = scatter * 0.33;
		scatter_max = scatter * 2.0;
		scatter_z = scatter_min;

		// UQ1: Vary scatter based on map size...
		if (map_size >= 50000)
		{
			if (map_size >= 90000)
			{
				scatter_min = scatter * 3.0;
				scatter_max = scatter * 5.0;
				scatter_z = scatter * 0.33;
			}
			else if (map_size >= 80000)
			{
				scatter_min = scatter * 2.0;
				scatter_max = scatter * 4.0;
				scatter_z = scatter * 0.33;
			}
			else if (map_size >= 70000)
			{
				scatter_min = scatter * 1.5;
				scatter_max = scatter * 3.0;
				scatter_z = scatter * 0.33;
			}
			else if (map_size >= 60000)
			{
				scatter_min = scatter * 0.75;
				scatter_max = scatter * 2.5;
				scatter_z = scatter * 0.33;
			}
			else if (map_size >= 50000)
			{
				scatter_min = scatter * 0.5;
				scatter_max = scatter * 2.5;
				scatter_z = scatter * 0.33;
			}
		}

		scatter_avg = (scatter + scatter_min + scatter_max) / 3.0;
		scatter_x = scatter;

		parallel_x_max = ((mapMaxs[0] - mapMins[0]) / scatter_avg);
		parallel_y_max = ((mapMaxs[1] - mapMins[1]) / scatter_avg);

		total_tests = ((mapMaxs[0] - mapMins[0]) / scatter_avg);
		total_tests *= ((mapMaxs[1] - mapMins[1]) / scatter_avg);
		total_tests *= ((mapMaxs[2] - mapMins[2]) / scatter_min);

		final_tests = 0;
		previous_time = clock();

//omp_set_nested(1);
omp_set_nested(0);

		scatter_x = scatter;

		final_tests = 0;
		previous_time = clock();
		float awPreviousPercent = 0.0;
		aw_stage_start_time = clock();

		for (parallel_x = 0; parallel_x < parallel_x_max; parallel_x++) // To OMP this sucker...
		{
			int		x;
			int		parallel_y = 0;
			float	scatter_y = scatter;
			float	scatter_mult_X = 1.0;
			vec3_t	last_org;

			if (areas+1 >= MAX_TEMP_AREAS)
			{
				break;
			}

			// Vary X scatter distance...
			if (scatter_x == scatter) scatter_x = scatter_min;
			else if (scatter_x == scatter) scatter_x = scatter_max;
			else scatter_x = scatter;

			if (DO_OPEN_AREA_SPREAD && ShortestWallRangeFrom( last_org ) >= 256)
				scatter_mult_X = 2.0;

			x = startx - (parallel_x * (scatter_x * scatter_mult_X));

			if (offsetY == 0.0)
				offsetY = (scatter_y * 0.25);
			else if (offsetY == scatter_y * 0.25)
				offsetY = (scatter_y * 0.5);
			else if (offsetY == scatter_y * 0.5)
				offsetY = (scatter_y * 0.75);
			else
				offsetY = 0.0;

#pragma omp parallel for ordered schedule(dynamic) //num_threads(32)
			for (parallel_y = 0; parallel_y < parallel_y_max; parallel_y++) // To OMP this sucker...
			{
				int		z, y;
				float	current_height = mapMaxs[2]; // Init the current height to max map height...
				float	scatter_mult_Y = 1.0;

				if (areas + 1 >= MAX_TEMP_AREAS)
				{
					break;
				}

				// Vary Y scatter distance...
				if (scatter_y == scatter) scatter_y = scatter_min;
				else if (scatter_y == scatter) scatter_y = scatter_max;
				else scatter_y = scatter;

				if (DO_OPEN_AREA_SPREAD && ShortestWallRangeFrom( last_org ) >= 256)
					scatter_mult_Y = 2.0;

				y = (starty + offsetY) - (parallel_y * (scatter_y * scatter_mult_Y));

				for (z = startz; z >= mapMins[2]; z -= scatter_min)
				{
					vec3_t		new_org, org;
					float		floor = 0;
					qboolean	force_continue = qfalse;
					clock_t		current_time = clock();

					if (areas + 1 >= MAX_TEMP_AREAS)
					{
						break;
					}

					// Update the current test number...
					final_tests++;

					if (final_tests > 1000)
					{
						if(omp_get_thread_num() == 0)
						{// Draw a nice little progress bar ;)
							aw_percent_complete = (float)((float)final_tests/(float)total_tests)*100.0f;

							if (/*current_time - previous_time > 500*/aw_percent_complete - awPreviousPercent > 0.1) // update display every 500ms...
							{
								//previous_time = current_time;
								awPreviousPercent = aw_percent_complete;
								trap->UpdateScreen();
							}
						}
					}

					if (z >= current_height)
					{// We can skip down to this position...
						continue;
					}

					// Set this test location's origin...
					VectorSet(new_org, x, y, z);

//#pragma omp critical (__FLOOR_CHECK__)
					{
						// Find the ground at this point...
						floor = FloorHeightAt(new_org);
					}

					//if (floor > -424)
					//	trap->Print("Start at %f %f %f. Floor at %f.\n", new_org[0], new_org[1], new_org[2], floor);

					// Set the point found on the floor as the test location...
					VectorSet(org, new_org[0], new_org[1], floor);

					if (floor < mapMins[2] || (DO_SINGLE && floor >= MAX_MAP_SIZE))
					{// Can skip this one!
						// Mark current hit location to continue from...
						current_height = mapMins[2]-2048; // so we still update final_tests
						continue; // so we still update final_tests
						//break;
						//{
						//	current_height = GroundHeightNoSurfaceChecks(new_org); // get the actual hit location height...
						//}
						//continue;
					}
					else if (floor > mapMaxs[2])
					{// Marks a start-solid or on top of the sky... Skip...
						// We can't mark current_height to FloorHeightAt value...
//#pragma omp critical (__GROUND_HEIGHT_CHECK__)
//						{
//							current_height = GroundHeightNoSurfaceChecks(new_org); // get the actual hit location height...
//						}
						current_height = z - scatter_min;
						continue;
					}
					else if (VectorDistance(org, last_org) < waypoint_scatter_distance)
					{
						current_height = floor;
						continue;
					}

					if (force_continue) continue; // because omp critical can not "continue"...

//#pragma omp critical (__PLAYER__WIDTH_CHECK__)
					{
						if (!AIMod_AutoWaypoint_Check_PlayerWidth(org))
						{// Not wide enough for a player to fit!
							current_height = floor;
							force_continue = qtrue; // because omp critical can not "continue"...

							//trap->Print("Point is too small.\n");
						}
					}

					if (force_continue) continue; // because omp critical can not "continue"...

					if (sjc_jkg_preview && org[2]+8 < -423 && org[2]+8 > -424.0)
					{// grrrr....
						current_height = floor;
						continue;
					}

					if (FOLIAGE_TreeSolidBlocking_AWP(org))
					{// There is a tree in this position, skip it...
						current_height = floor;
						continue;
					}

//#pragma omp ordered
					{
#pragma omp critical (__ADD_TEMP_NODE__)
						{
							sprintf(last_node_added_string, "^5Adding temp waypoint ^3%i ^5at ^7%f %f %f^5.", areas, org[0], org[1], org[2]+8);

							//trap->Print("WP added at %f %f %f.\n", org[0], org[1], org[2]+8);

							arealist[areas][0] = org[0];
							arealist[areas][1] = org[1];
							arealist[areas][2] = org[2]+8;

							last_org[0] = arealist[areas][0];
							last_org[1] = arealist[areas][1];
							last_org[2] = arealist[areas][2];
							areas++;
						}

						current_height = floor;
					}
				}
			}
		}

		if (areas + 1 >= MAX_TEMP_AREAS)
		{
			trap->Print("^1*** ^3%s^5: Too many waypoints detected... Try again with a higher density value...\n", "AUTO-WAYPOINTER");
			aw_percent_complete = 0.0f;
			trap->S_Shutup(qfalse);
			AIMod_AutoWaypoint_Free_Memory();
			return;
		}
	}

	trap->S_Shutup(qfalse);

	//
	// Add nodes for all movers...
	//

	GenerateMoverList(); // init the mover list on first check...

	for (i = 0; i < MOVER_LIST_NUM; i++)
	{
		int count = 0;
		float temp_roof, temp_roof2, temp_ground;
		vec3_t temp_org;
		qboolean isDoor = qfalse;

		VectorCopy(MOVER_LIST[i], temp_org);

		temp_ground = MOVER_LIST[i][2];

		//temp_org[2] += waypoint_scatter_distance; // Start above the lift...
		temp_roof2 = RoofHeightAt(temp_org);
		temp_roof = MOVER_LIST_TOP[i][2];

		if (temp_roof2 > mapMaxs[2]) temp_roof2 = mapMaxs[2]; // Never go above the roof of the map... lol

		if (temp_roof - temp_ground <= 96) isDoor = qtrue;

		if (temp_roof2 > temp_roof && !isDoor) temp_roof = temp_roof2; // Use the highest height... just to be sure...

		//VectorCopy(org, temp_org);
		//temp_org[2] -= waypoint_scatter_distance*2; // Start below the lift...
		//temp_ground = GroundHeightAt(temp_org);
		temp_ground = MOVER_LIST[i][2];

		if (isDoor)
		{
			arealist[areas][0] = temp_org[0];
			arealist[areas][1] = temp_org[1];
			arealist[areas][2] = temp_org[2];
			areas++;

			arealist[areas][0] = temp_org[0];
			arealist[areas][1] = temp_org[1];
			arealist[areas][2] = temp_org[2] + waypoint_scatter_distance;
			areas++;
			continue; // Most likely a door... Skip it... But add two points near the base...
		}

		//temp_org[2] += waypoint_scatter_distance;

		while (temp_org[2] <= temp_roof)
		{// Add waypoints all the way up!
			arealist[areas][0] = temp_org[0];
			arealist[areas][1] = temp_org[1];
			arealist[areas][2] = temp_org[2];
			areas++;

			arealist[areas][0] = temp_org[0]+56.0;
			arealist[areas][1] = temp_org[1];
			arealist[areas][2] = temp_org[2];
			areas++;

			arealist[areas][0] = temp_org[0];
			arealist[areas][1] = temp_org[1]+56.0;
			arealist[areas][2] = temp_org[2];
			areas++;

			arealist[areas][0] = temp_org[0]+56.0;
			arealist[areas][1] = temp_org[1]+56.0;
			arealist[areas][2] = temp_org[2];
			areas++;

			arealist[areas][0] = temp_org[0]-56.0;
			arealist[areas][1] = temp_org[1];
			arealist[areas][2] = temp_org[2];
			areas++;

			arealist[areas][0] = temp_org[0];
			arealist[areas][1] = temp_org[1]-56.0;
			arealist[areas][2] = temp_org[2];
			areas++;

			arealist[areas][0] = temp_org[0]-56.0;
			arealist[areas][1] = temp_org[1]-56.0;
			arealist[areas][2] = temp_org[2];
			areas++;

			temp_org[2] += waypoint_scatter_distance;
			count++;
		}

		trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Added %i waypoints for mover %i.\n", count, i);
	}

	trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Generated a total of %i temporary waypoints.\n", areas);

	//
	// Check for cleaning...
	//

	if (areas < MAX_WPARRAY_SIZE)
	{// UQ1: Can use them all!
		total_waypoints = 0;

		trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Temporary waypoint cleanup not required. Converting to final waypoints.\n");

		aw_percent_complete = 0.0f;
		aw_stage_start_time = clock();
		strcpy( task_string3, va("^5Final (cleanup) pass. Building final waypoints...") );
		trap->UpdateScreen();

		total_areas = areas;

		for ( i = 0; i < areas; i++ )
		{
			vec3_t		area_org;
			short int	objNum[3] = { 0, 0, 0 };

			// Draw a nice little progress bar ;)
			aw_percent_complete = (float)((float)((float)i/(float)total_areas)*100.0f);

			update_timer++;

			if (update_timer >= 500)
			{
				trap->UpdateScreen();
				update_timer = 0;
			}

			area_org[0] = arealist[i][0];
			area_org[1] = arealist[i][1];
			area_org[2] = arealist[i][2];

			if (area_org[2] <= -MAX_MAP_SIZE)
			{// This is a bad height!
				continue;
			}

			strcpy( last_node_added_string, va("^5Adding waypoint ^3%i ^5at ^7%f %f %f^5.", total_waypoints, area_org[0], area_org[1], area_org[2]) );
			Load_AddNode( area_org, 0, objNum, 0 );	//add the node
			total_waypoints++;
		}

		// Ladders...
//		aw_total_waypoints = total_waypoints;
//		aw_percent_complete = 0.0f;
//		aw_stage_start_time = clock();

//		strcpy( task_string3, va("^5Looking for ladders...") );
//		trap->UpdateScreen();

//		for ( i = 0; i < total_waypoints; i++ )
//		{
//			// Draw a nice little progress bar ;)
//			aw_percent_complete = (float)((float)((float)i/(float)total_waypoints)*100.0f);

//			update_timer++;

//			if (update_timer >= 100)
//			{
//				trap->UpdateScreen();
//				update_timer = 0;
//			}

//			AIMod_AutoWaypoint_Check_For_Ladders( nodes[i].origin );
//		}

//		total_waypoints = aw_total_waypoints;
//		// End Ladders...

		strcpy( task_string3, va("^5Saving %i generated waypoints.", total_waypoints) );
		trap->UpdateScreen();

		trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Generated a total of %i waypoints.\n", total_waypoints);

		number_of_nodes = total_waypoints;
		waypoint_scatter_distance *= 1.5;
		AIMOD_NODES_SaveNodes_Autowaypointed();

		trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Waypoint database created successfully in ^3%.2f ^5seconds with ^7%i ^5waypoints.\n",
				 (float) ((trap->Milliseconds() - start_time) / 1000), total_waypoints) );

		strcpy( task_string3, va("^5Waypoint database created successfully in ^3%.2f ^5seconds with ^7%i ^5waypoints.\n", (float) ((trap->Milliseconds() - start_time) / 1000), total_waypoints) );
		trap->UpdateScreen();

		aw_percent_complete = 0.0f;
		strcpy( task_string3, va("^5Informing the server to load & test the new waypoints...") );
		trap->UpdateScreen();

		//trap->SendConsoleCommand( "set bot_wp_visconnect 1\n" );
		//trap->SendConsoleCommand( "bot_wp_convert_awp\n" );

		aw_percent_complete = 0.0f;
		strcpy( task_string3, va("^5Waypoint auto-generation is complete...") );
		trap->UpdateScreen();

		waypoint_scatter_distance = original_waypoint_scatter_distance;

#ifdef __COVER_SPOTS__
		AIMOD_Generate_Cover_Spots(); // UQ1: Want to add these to JKA???
#endif //__COVER_SPOTS__

		free(arealist);

		AIMod_AutoWaypoint_Free_Memory();

		aw_percent_complete = 0.0f;
		trap->UpdateScreen();

		return;
	}

	//
	// OK. We created more then MAX_WPARRAY_SIZE temporary nodes. We need to do some clearning to reduce the number to below 32000...
	//

	total_areas = areas;
	total_waypoints = 0;

	aw_percent_complete = 0.0f;
	aw_stage_start_time = clock();

	strcpy( task_string3, va("^5Final (cleanup) pass. Building final waypoints...") );
	trap->UpdateScreen();

#ifdef __RANDOM_REMOVAL__
	int			*added_list = NULL;
	int			remove_number = areas - MAX_WPARRAY_SIZE;
	int			numAdded = 0;
	short int	objNum[3] = { 0, 0, 0 };
	float		previousPercent = 0.0;

	added_list = (int*)malloc(sizeof(int) * MAX_WPARRAY_SIZE);
	memset(added_list, -1, sizeof(int) * MAX_WPARRAY_SIZE);

	aw_stage_start_time = clock();

//#pragma omp parallel for ordered schedule(dynamic)
	for (i = 0; i < MAX_WPARRAY_SIZE; i++)
	{
		//if(omp_get_thread_num() == 0)
		{
			// Draw a nice little progress bar ;)
			aw_percent_complete = (float)((float)((float)numAdded / (float)MAX_WPARRAY_SIZE)*100.0f);

			if (aw_percent_complete - previousPercent > 0.1)
			{
				AWP_UpdatePercentBarOnly(aw_percent_complete);
				trap->UpdateScreen();
				previousPercent = aw_percent_complete;
			}
		}

		int			selected = -1;
		vec3_t		currentSpot;

		while (1)
		{
			qboolean bad = qfalse;

			selected = irand_big(0, areas - 1);

			//selected = (int)(rand() * (double)(areas - 1));

			for (int j = 0; j < /*MAX_WPARRAY_SIZE*/i; j++)
			{
				if (added_list[j] == selected)
				{// Already have this one...
					bad = qtrue;
					break;
				}
			}

			if (!bad) // Found one that we don't have...
				break;
		}

		currentSpot[0] = arealist[selected][0];
		currentSpot[1] = arealist[selected][1];
		currentSpot[2] = arealist[selected][2];

//#pragma omp critical// (__ADD_WAYPOINT_CHECK__)
		{
			added_list[i] = selected;
			strcpy(last_node_added_string, va("^5Adding waypoint ^3%i ^5at ^7%f %f %f^5.", total_waypoints, currentSpot[0], currentSpot[1], currentSpot[2]));
			Load_AddNode(currentSpot, 0, objNum, 0);	//add the node
			total_waypoints++;
			numAdded++;
		}
	}

	free(added_list);

	AWP_UpdatePercentBar2(0, "", "", "");

	aw_percent_complete = 0.0f;
	trap->UpdateScreen();

#else //!
	remove_ratio = (areas / MAX_WPARRAY_SIZE);

	/*
	if (remove_ratio <= 1.5)
		remove_ratio *= 7.0; // Hopefully this ratio will provide nearly 32000 waypoints every time...
	else
		remove_ratio *= 11.0; // Hopefully this ratio will provide nearly 32000 waypoints every time...
	*/

	trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Generated too many temporary waypoints for the game. Running cleanup.\n");

	{
		qboolean	clean_fail = qfalse;
		float		use_scatter = 0;
		int			remove_per_area = 0;

		use_scatter = 512.0;//original_waypoint_scatter_distance * waypoint_distance_multiplier;
		remove_per_area = 32 / (remove_ratio+1);

		aw_stage_start_time = clock();

//#pragma omp parallel for ordered schedule(dynamic)
		for ( i = 0; i < areas; i++ )
		{
			vec3_t		area_org;
			short int	objNum[3] = { 0, 0, 0 };
			qboolean	bad = qfalse;
			int			j;
			int			found = 0;
			int			num_found = 0;

			// Draw a nice little progress bar ;)
			aw_percent_complete = (float)((float)((float)i/(float)areas)*100.0f);

			update_timer++;

			//if(omp_get_thread_num() == 0)
			{
				if (update_timer >= 100)
				{
					trap->UpdateScreen();
					update_timer = 0;
				}
			}

			if (total_waypoints > MAX_WPARRAY_SIZE)
			{// We failed!
				clean_fail = qtrue;
				continue;
			}

			area_org[0] = arealist[i][0];
			area_org[1] = arealist[i][1];
			area_org[2] = arealist[i][2];

			if (area_org[2] <= -MAX_MAP_SIZE)
			{// This is a bad height!
				continue;
			}

			if (area_org[0] == 0.0 && area_org[1] == 0.0 && area_org[2] == 0.0)
			{// Ignore 0,0,0
				continue;
			}

			for (j = 0; j < total_waypoints; j++)
			{
				float dist = VectorDistance(area_org, nodes[j].origin);

				if (dist < use_scatter)//*area_distance_multiplier)
				{
					num_found++;

					if (num_found >= remove_per_area)
					{
						//if(omp_get_thread_num() == 0)
						//{
						//	trap->Print("Distance between area %i at %f %f %f and wp %i at %f %f %f is %f. MARKED BAD!\n", i, area_org[0], area_org[1], area_org[2], j, nodes[j].origin[0], nodes[j].origin[1], nodes[j].origin[2], dist );
						//}

						bad = qtrue;
						break;
					}
				}
			}

			if (bad)
			{
				continue;
			}

//#pragma omp ordered
			{
//#pragma omp critical (__ADD_WAYPOINT_CHECK__)
				{
					strcpy( last_node_added_string, va("^5Adding waypoint ^3%i ^5at ^7%f %f %f^5.", total_waypoints, area_org[0], area_org[1], area_org[2]) );
					Load_AddNode( area_org, 0, objNum, 0 );	//add the node
					total_waypoints++;
				}
			}
		}

		if (clean_fail)
		{
			trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^2Cleaning waypoints has failed. Try using a larger scatter distance.\n");

			aw_percent_complete = 0.0f;
			strcpy( task_string3, va("^5Waypoint auto-generation is complete...") );
			trap->UpdateScreen();

			waypoint_scatter_distance = original_waypoint_scatter_distance;

			free(arealist);

			AIMod_AutoWaypoint_Free_Memory();

			aw_percent_complete = 0.0f;
			trap->UpdateScreen();
			return;
		}
	}
#endif //!__RANDOM_REMOVAL__

	// Ladders...
	/*aw_total_waypoints = total_waypoints;
	aw_percent_complete = 0.0f;
	strcpy( task_string3, va("^5Looking for ladders...") );
	trap->UpdateScreen();

	for ( i = 0; i < total_waypoints; i++ )
	{
		// Draw a nice little progress bar ;)
		aw_percent_complete = (float)((float)((float)i/(float)total_waypoints)*100.0f);

		update_timer++;

		if (update_timer >= 100)
		{
			trap->UpdateScreen();
			update_timer = 0;
		}

		AIMod_AutoWaypoint_Check_For_Ladders( nodes[i].origin );
	}

	total_waypoints = aw_total_waypoints;*/
	// End Ladders...

	strcpy( task_string3, va("^5Saving the generated waypoints.\n") );
	trap->UpdateScreen();

	trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Generated a total of %i waypoints.\n", total_waypoints);

	number_of_nodes = total_waypoints;
	waypoint_scatter_distance *= 1.5;
	AIMOD_NODES_SaveNodes_Autowaypointed();

#ifdef __COVER_SPOTS__
	AIMOD_Generate_Cover_Spots(); // UQ1: Want to add these to JKA???
#endif //__COVER_SPOTS__

	trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Waypoint database created successfully in ^3%.2f ^5seconds with ^7%i ^5waypoints.\n",
				 (float) ((trap->Milliseconds() - start_time) / 1000), total_waypoints) );

	strcpy( task_string3, va("^5Waypoint database created successfully in ^3%.2f ^5seconds with ^7%i ^5waypoints.\n", (float) ((trap->Milliseconds() - start_time) / 1000), total_waypoints) );
	trap->UpdateScreen();

	aw_percent_complete = 0.0f;
	strcpy( task_string3, va("^5Informing the server to load & test the new waypoints...") );
	trap->UpdateScreen();

	//trap->SendConsoleCommand( "set bot_wp_visconnect 1\n" );
	//trap->SendConsoleCommand( "bot_wp_convert_awp\n" );

	aw_percent_complete = 0.0f;
	strcpy( task_string3, va("^5Waypoint auto-generation is complete...") );
	trap->UpdateScreen();

	waypoint_scatter_distance = original_waypoint_scatter_distance;

	free(arealist);

	AIMod_AutoWaypoint_Free_Memory();

	aw_percent_complete = 0.0f;
	trap->UpdateScreen();
}

qboolean wp_memory_initialized = qfalse;

void AIMod_AutoWaypoint_Init_Memory ( void )
{
	number_of_nodes = 0;

	if (wp_memory_initialized == qfalse)
	{
		nodes = (node_t*)malloc( (sizeof(node_t)+1)*MAX_NODES );
		wp_memory_initialized = qtrue;
	}
}

void AIMod_AutoWaypoint_Free_Memory ( void )
{
	number_of_nodes = 0;

	if (wp_memory_initialized == qtrue)
	{
		free(nodes);
		wp_memory_initialized = qfalse;
	}
}

void AIMod_AutoWaypoint_Init_Memory ( void ); // below...
void AIMod_AutoWaypoint_Optimizer ( void ); // below...
void AIMod_AutoWaypoint_Cleaner ( qboolean quiet, qboolean null_links_only, qboolean relink_only, qboolean multipass, qboolean initial_pass, qboolean extra, qboolean marked_locations, qboolean extra_reach, qboolean reset_reach, qboolean convert_old, qboolean pathtest, qboolean trees );

void AIMod_AutoWaypoint_Clean ( void )
{
	char	str[MAX_TOKEN_CHARS];

	if ( trap->Cmd_Argc() < 2 )
	{
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^7Usage:\n" );
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3/awc <method> <novischeck 0/1>^5. (novicheck can be anything)\n" );
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^5Available methods are: Generally only a pathtest pass is needed.\n" );
		//trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"convert\" ^5- Convert old JKA wp file to Warzone format.\n");
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"relink\" ^5- Just do relinking.\n");
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"pathtest\" ^5- Remove waypoints with no path to server's first spawnpoint.\n");
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"trees\" ^5- Remove nodes inside trees.\n");
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"extrareach\" ^5- Remove waypoints nearby your marked locations (awc_addremovalspot & awc_addbadheight) and add extra reachability (wp link ranges).\n");
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"resetreach\" ^5- Remove waypoints with no path to server's first spawnpoint and reset max link ranges.\n");
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"clean\" ^5- Do a full clean.\n");
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"markedlocations\" ^5- Remove waypoints nearby your marked locations (awc_addremovalspot & awc_addbadheight).\n");
		//trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"multipass\" ^5- Do a multi-pass full clean (max optimize).\n");
		//trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"extra\" ^5- Do a full clean (but remove more - good if the number is still too high after optimization).\n");
		//trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"cover\" ^5- Just generate coverpoints.\n");
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"list\" ^5- List locations of all waypoints.\n");
		trap->UpdateScreen();
		return;
	}

	if (trap->Cmd_Argc() >= 2)
	{
		trap->Cmd_Argv(2, str, sizeof(str));

		if (atoi(str) == 1)
			DO_FAST_LINK = qtrue;
		else
			DO_FAST_LINK = qfalse;
	}
	else
	{
		DO_FAST_LINK = qfalse;
	}

	trap->Cmd_Argv( 1, str, sizeof(str) );

	if ( Q_stricmp( str, "convert") == 0 )
	{
		AIMod_AutoWaypoint_Cleaner(qtrue, qfalse, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse, qfalse);
	}
	else if ( Q_stricmp( str, "relink") == 0 )
	{
		AIMod_AutoWaypoint_Cleaner(qtrue, qfalse, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse);
	}
	else if ( Q_stricmp( str, "trees") == 0 )
	{
		AIMod_AutoWaypoint_Cleaner(qfalse, qfalse, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue);
	}
	else if ( Q_stricmp( str, "pathtest") == 0 )
	{
		AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);
	}
	else if ( Q_stricmp( str, "clean") == 0 )
	{
		AIMod_AutoWaypoint_Cleaner(qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse);
	}
	else if ( Q_stricmp( str, "multipass") == 0 )
	{
		AIMod_AutoWaypoint_Cleaner(qtrue, qfalse, qfalse, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse);
	}
	else if ( Q_stricmp( str, "extra") == 0 )
	{
		AIMod_AutoWaypoint_Cleaner(qtrue, qfalse, qfalse, qtrue, qfalse, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse);
	}
	else if ( Q_stricmp( str, "markedlocations") == 0 )
	{
		AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse);
	}
	else if ( Q_stricmp( str, "extrareach") == 0 )
	{
		AIMod_AutoWaypoint_Cleaner(qtrue, qfalse, qtrue, qfalse, qfalse, qfalse, qtrue, qtrue, qfalse, qfalse, qfalse, qfalse);
	}
	else if ( Q_stricmp( str, "resetreach") == 0 )
	{
		AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse, qfalse, qfalse);
	}
	else if ( Q_stricmp( str, "list") == 0 )
	{
		int i;

		DO_FAST_LINK = qfalse;

		AIMod_AutoWaypoint_Init_Memory();

		if (number_of_nodes > 0)
		{// UQ1: Init nodes list!
			number_of_nodes = 0;
			optimized_number_of_nodes = 0;

			memset( nodes, 0, ((sizeof(node_t)+1)*MAX_NODES) );
			memset( optimized_nodes, 0, ((sizeof(node_t)+1)*MAX_NODES) );
		}

		AIMOD_NODES_LoadNodes();

		if (number_of_nodes <= 0)
		{
			AIMod_AutoWaypoint_Free_Memory();
			return;
		}

		for (i = 0; i < number_of_nodes; i++)
		{
			trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Waypoint ^7%i^5 is at ^7%f %f %f^5.\n", i, nodes[i].origin[0], nodes[i].origin[1], nodes[i].origin[2]);
		}

		AIMod_AutoWaypoint_Free_Memory();
	}
	else if ( Q_stricmp( str, "cover") == 0 )
	{
		DO_FAST_LINK = qfalse;

		AIMod_AutoWaypoint_Init_Memory();

		if (number_of_nodes > 0)
		{// UQ1: Init nodes list!
			number_of_nodes = 0;
			optimized_number_of_nodes = 0;

			memset( nodes, 0, ((sizeof(node_t)+1)*MAX_NODES) );
			memset( optimized_nodes, 0, ((sizeof(node_t)+1)*MAX_NODES) );
		}

		AIMOD_NODES_LoadNodes();

		if (number_of_nodes <= 0)
		{
			AIMod_AutoWaypoint_Free_Memory();
			return;
		}

#ifdef __COVER_SPOTS__
		AIMOD_Generate_Cover_Spots();
#endif //__COVER_SPOTS__

		AIMod_AutoWaypoint_Free_Memory();
	}

	DO_FAST_LINK = qfalse;
}

/* */
void
AIMod_AutoWaypoint ( void )
{
	char	str[MAX_TOKEN_CHARS];
	int		original_wp_scatter_dist = waypoint_scatter_distance;

	// UQ1: Check if we have an SSE CPU.. It can speed up our memory allocation by a lot!
	if (!CPU_CHECKED)
		UQ_Get_CPU_Info();

	if ( trap->Cmd_Argc() < 2 )
	{
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^7Usage:\n" );
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3/autowaypoint <method> <scatter_distance> <allowRock 0/1> <openSpread 0/1> <doClean 0/1>^5. Distance, allowRock, openSpread and doClean are optional.\n" );
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^5Available methods are:\n" );
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"standard\" ^5- For standard multi-level maps.\n");
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"altmethod\" ^5- For standard multi-level maps (alternative method).\n");
		trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^3\"single\" ^5- For standard single-level maps.\n");
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"nosky\" ^5- For standard multi-level maps. Don't check for sky.\n");
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"water\" ^5- For standard multi-level maps. Also add waypoints on water.\n");
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"thorough\" ^5- Use extensive fall waypoint checking.\n");
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"translucent\" ^5- Allow translucent surfaces (for maps with surfaces being ignored by standard).\n");
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"fast\" ^5- For standard multi-level maps. This version does not visibility check waypoint links.\n");
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"waterfast\" ^5- For standard multi-level maps. This version does not visibility check waypoint links.\n");
		trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"ultrafast\" ^5- For standard multi-level maps. This version does not visibility check waypoint links.\n");
		//trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"noclean\" ^5- For standard multi-level maps (with no cleaning passes).\n");
		//trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^3\"outdoor_only\" ^5- For standard single level maps.\n");
		trap->UpdateScreen();
		return;
	}

	DO_THOROUGH = qfalse;
	DO_TRANSLUCENT = qfalse;
	DO_FAST_LINK = qfalse;
	DO_ULTRAFAST = qfalse;
	DO_WATER = qfalse;
	DO_ROCK = qfalse;
	DO_OPEN_AREA_SPREAD = qfalse;
	DO_NEW_METHOD = qfalse;
	DO_SINGLE = qfalse;

	qboolean DO_CLEANER = qfalse;

	if (trap->Cmd_Argc() >= 3)
	{
		trap->Cmd_Argv(3, str, sizeof(str));
		if (atoi(str) == 1)
			DO_ROCK = qtrue;
		else
			DO_ROCK = qfalse;
	}
	else
	{
		DO_ROCK = qtrue;
	}

	if (trap->Cmd_Argc() >= 4)
	{
		trap->Cmd_Argv(4, str, sizeof(str));
		if (atoi(str) == 1)
			DO_OPEN_AREA_SPREAD = qtrue;
		else
			DO_OPEN_AREA_SPREAD = qfalse;
	}
	else
	{
		DO_OPEN_AREA_SPREAD = qfalse;
	}

	if (trap->Cmd_Argc() >= 5)
	{
		trap->Cmd_Argv(3, str, sizeof(str));
		if (atoi(str) == 1)
			DO_CLEANER = qtrue;
		else
			DO_CLEANER = qfalse;
	}
	else
	{
		DO_CLEANER = qtrue;
	}

	trap->Cmd_Argv(1, str, sizeof(str));

	if ( Q_stricmp( str, "altmethod") == 0 )
	{
		DO_NEW_METHOD = qtrue;

		if ( trap->Cmd_Argc() >= 2 )
		{
			// Override normal scatter distance...
			int dist = waypoint_scatter_distance;

			trap->Cmd_Argv( 2, str, sizeof(str) );
			dist = atoi(str);

			if (dist <= 4)
			{
				// Fallback and warning...
				dist = original_wp_scatter_dist;

				trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^7Warning: ^5Invalid scatter distance set (%i). Using default (%i)...\n", atoi(str), original_wp_scatter_dist );
			}

			waypoint_scatter_distance = dist;
			AIMod_AutoWaypoint_StandardMethod();

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);

			waypoint_scatter_distance = original_wp_scatter_dist;
		}
		else
		{
			AIMod_AutoWaypoint_StandardMethod();

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);
		}

		DO_NEW_METHOD = qfalse;
	}
	else if ( Q_stricmp( str, "standard") == 0 )
	{
		if ( trap->Cmd_Argc() >= 2 )
		{
			// Override normal scatter distance...
			int dist = waypoint_scatter_distance;

			trap->Cmd_Argv( 2, str, sizeof(str) );
			dist = atoi(str);

			if (dist <= 4)
			{
				// Fallback and warning...
				dist = original_wp_scatter_dist;

				trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^7Warning: ^5Invalid scatter distance set (%i). Using default (%i)...\n", atoi(str), original_wp_scatter_dist );
			}

			waypoint_scatter_distance = dist;
			AIMod_AutoWaypoint_StandardMethod();

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);

			waypoint_scatter_distance = original_wp_scatter_dist;
		}
		else
		{
			AIMod_AutoWaypoint_StandardMethod();

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);
		}
	}
	else if (Q_stricmp(str, "single") == 0)
	{
		DO_SINGLE = qtrue;

		if (trap->Cmd_Argc() >= 2)
		{
			// Override normal scatter distance...
			int dist = waypoint_scatter_distance;

			trap->Cmd_Argv(2, str, sizeof(str));
			dist = atoi(str);

			if (dist <= 4)
			{
				// Fallback and warning...
				dist = original_wp_scatter_dist;

				trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^7Warning: ^5Invalid scatter distance set (%i). Using default (%i)...\n", atoi(str), original_wp_scatter_dist);
			}

			waypoint_scatter_distance = dist;
			AIMod_AutoWaypoint_StandardMethod();

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);

			waypoint_scatter_distance = original_wp_scatter_dist;
		}
		else
		{
			AIMod_AutoWaypoint_StandardMethod();

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);
		}

		DO_SINGLE = qfalse;
	}
	else if ( Q_stricmp( str, "nosky") == 0 )
	{
		DO_NOSKY = qtrue;

		if ( trap->Cmd_Argc() >= 2 )
		{
			// Override normal scatter distance...
			int dist = waypoint_scatter_distance;

			trap->Cmd_Argv( 2, str, sizeof(str) );
			dist = atoi(str);

			if (dist <= 4)
			{
				// Fallback and warning...
				dist = original_wp_scatter_dist;

				trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^7Warning: ^5Invalid scatter distance set (%i). Using default (%i)...\n", atoi(str), original_wp_scatter_dist );
			}

			waypoint_scatter_distance = dist;
			AIMod_AutoWaypoint_StandardMethod();

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);

			waypoint_scatter_distance = original_wp_scatter_dist;
		}
		else
		{
			AIMod_AutoWaypoint_StandardMethod();

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);
		}

		DO_NOSKY = qfalse;
	}
	else if ( Q_stricmp( str, "water") == 0 )
	{
		DO_WATER = qtrue;

		if ( trap->Cmd_Argc() >= 2 )
		{
			// Override normal scatter distance...
			int dist = waypoint_scatter_distance;

			trap->Cmd_Argv( 2, str, sizeof(str) );
			dist = atoi(str);

			if (dist <= 4)
			{
				// Fallback and warning...
				dist = original_wp_scatter_dist;

				trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^7Warning: ^5Invalid scatter distance set (%i). Using default (%i)...\n", atoi(str), original_wp_scatter_dist );
			}

			waypoint_scatter_distance = dist;
			AIMod_AutoWaypoint_StandardMethod();

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);

			waypoint_scatter_distance = original_wp_scatter_dist;
		}
		else
		{
			AIMod_AutoWaypoint_StandardMethod();

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);
		}

		DO_WATER = qfalse;
	}
	else if ( Q_stricmp( str, "fast") == 0 )
	{
		DO_FAST_LINK = qtrue;

		if ( trap->Cmd_Argc() >= 2 )
		{
			// Override normal scatter distance...
			int dist = waypoint_scatter_distance;

			trap->Cmd_Argv( 2, str, sizeof(str) );
			dist = atoi(str);

			if (dist <= 4)
			{
				// Fallback and warning...
				dist = original_wp_scatter_dist;

				trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^7Warning: ^5Invalid scatter distance set (%i). Using default (%i)...\n", atoi(str), original_wp_scatter_dist );
			}

			waypoint_scatter_distance = dist;
			AIMod_AutoWaypoint_StandardMethod();

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);

			waypoint_scatter_distance = original_wp_scatter_dist;
		}
		else
		{
			AIMod_AutoWaypoint_StandardMethod();

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);
		}

		DO_FAST_LINK = qfalse;
	}
	else if ( Q_stricmp( str, "ultrafast") == 0 )
	{
		DO_FAST_LINK = qtrue;
		DO_ULTRAFAST = qtrue;

		if ( trap->Cmd_Argc() >= 2 )
		{
			// Override normal scatter distance...
			int dist = waypoint_scatter_distance;

			trap->Cmd_Argv( 2, str, sizeof(str) );
			dist = atoi(str);

			if (dist <= 4)
			{
				// Fallback and warning...
				dist = original_wp_scatter_dist;

				trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^7Warning: ^5Invalid scatter distance set (%i). Using default (%i)...\n", atoi(str), original_wp_scatter_dist );
			}

			waypoint_scatter_distance = dist;
			AIMod_AutoWaypoint_StandardMethod();

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);

			waypoint_scatter_distance = original_wp_scatter_dist;
		}
		else
		{
			AIMod_AutoWaypoint_StandardMethod();

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);
		}

		DO_FAST_LINK = qfalse;
		DO_ULTRAFAST = qfalse;
	}
	else if ( Q_stricmp( str, "waterfast") == 0 )
	{
		DO_FAST_LINK = qtrue;
		DO_ULTRAFAST = qtrue;
		DO_WATER = qtrue;

		if ( trap->Cmd_Argc() >= 2 )
		{
			// Override normal scatter distance...
			int dist = waypoint_scatter_distance;

			trap->Cmd_Argv( 2, str, sizeof(str) );
			dist = atoi(str);

			if (dist <= 4)
			{
				// Fallback and warning...
				dist = original_wp_scatter_dist;

				trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^7Warning: ^5Invalid scatter distance set (%i). Using default (%i)...\n", atoi(str), original_wp_scatter_dist );
			}

			waypoint_scatter_distance = dist;
			AIMod_AutoWaypoint_StandardMethod();

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);

			waypoint_scatter_distance = original_wp_scatter_dist;
		}
		else
		{
			AIMod_AutoWaypoint_StandardMethod();

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);
		}

		DO_FAST_LINK = qfalse;
		DO_ULTRAFAST = qfalse;
		DO_WATER = qfalse;
	}
	else if ( Q_stricmp( str, "thorough") == 0 )
	{
		if ( trap->Cmd_Argc() >= 2 )
		{
			// Override normal scatter distance...
			int dist = waypoint_scatter_distance;

			trap->Cmd_Argv( 2, str, sizeof(str) );
			dist = atoi(str);

			if (dist <= 4)
			{
				// Fallback and warning...
				dist = original_wp_scatter_dist;

				trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^7Warning: ^5Invalid scatter distance set (%i). Using default (%i)...\n", atoi(str), original_wp_scatter_dist );
			}

			waypoint_scatter_distance = dist;

			DO_THOROUGH = qtrue;

			AIMod_AutoWaypoint_StandardMethod();

			waypoint_scatter_distance = original_wp_scatter_dist;

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);
		}
		else
		{
			DO_THOROUGH = qtrue;

			AIMod_AutoWaypoint_StandardMethod();

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);
		}
	}
	else if ( Q_stricmp( str, "translucent") == 0 )
	{
		if ( trap->Cmd_Argc() >= 2 )
		{
			// Override normal scatter distance...
			int dist = waypoint_scatter_distance;

			trap->Cmd_Argv( 2, str, sizeof(str) );
			dist = atoi(str);

			if (dist <= 4)
			{
				// Fallback and warning...
				dist = original_wp_scatter_dist;

				trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^7Warning: ^5Invalid scatter distance set (%i). Using default (%i)...\n", atoi(str), original_wp_scatter_dist );
			}

			waypoint_scatter_distance = dist;

			DO_TRANSLUCENT = qtrue;

			AIMod_AutoWaypoint_StandardMethod();

			waypoint_scatter_distance = original_wp_scatter_dist;

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);
		}
		else
		{
			DO_TRANSLUCENT = qtrue;

			AIMod_AutoWaypoint_StandardMethod();

			if (DO_CLEANER)
				AIMod_AutoWaypoint_Cleaner(qtrue, qtrue, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qfalse, qtrue, qfalse);
		}
	}
	

	DO_OPEN_AREA_SPREAD = qfalse;
	DO_ROCK = qfalse;
}

//
// Autowaypoint Optimizer...
//

/*
===========================================================================
GetFCost
Utility function used by A* pathfinding to calculate the
cost to move between nodes towards a goal.  Using the A*
algorithm F = G + H, G here is the distance along the node
paths the bot must travel, and H is the straight-line distance
to the goal node.
Returned as an int because more precision is unnecessary and it
will slightly speed up heap access
===========================================================================
*/
int
GetFCost ( centity_t *bot, int to, int num, int parentNum, float *gcost )
{
	float	gc = 0;
	float	hc = 0;
	vec3_t	v;

	if ( gcost[num] == -1 )
	{
		if ( parentNum != -1 )
		{
			gc = gcost[parentNum];
			VectorSubtract( nodes[num].origin, nodes[parentNum].origin, v );
			gc += VectorLength( v );

			// UQ1: Make WATER, BARB WIRE, and ICE nodes cost more!
			if (nodes[num].type & NODE_WATER)
			{// Huge cost for water nodes!
				//gc+=(gc*100);
				gc = 65000.0f;
			}
			else if (nodes[num].type & NODE_ICE)
			{// Huge cost for ice(slick) nodes!
				//gc+=(gc*100);
				gc = 65000.0f;
			}

			if (gc > 65000)
				gc = 65000.0f;

#ifdef __COVER_SPOTS__
			/*if (nodes[num].type & NODE_COVER)
			{// Encorage the use of cover spots!
				gc = 0.0f;
			}*/
#endif //__COVER_SPOTS__
		}

		gcost[num] = gc;
	}
	else
	{
		gc = gcost[num];
		//G_Printf("gcost for num %i is %f\n", num, gc);
	}

	VectorSubtract( nodes[to].origin, nodes[num].origin, v );
	hc = VectorLength( v );

	/*if (nodes[num].type & NODE_WATER)
	{// Huge cost for water nodes!
		hc*=2;
	}
	else if (nodes[num].type & NODE_ICE)
	{// Huge cost for ice(slick) nodes!
		hc*=4;
	}*/

#ifdef __COVER_SPOTS__
	/*if (nodes[num].type & NODE_COVER)
	{// Encorage the use of cover spots!
		hc *= 0.5;
	}*/
#endif //__COVER_SPOTS__

	return (int) ( gc + hc );
}

qboolean wp_optimize_memory_initialized = qfalse;

void AIMod_AutoWaypoint_Optimize_Init_Memory ( void )
{
	if (wp_optimize_memory_initialized == qfalse)
	{
		optimized_nodes = (node_t*)malloc( (sizeof(node_t)+1)*MAX_NODES );
		wp_optimize_memory_initialized = qtrue;
	}
}

void AIMod_AutoWaypoint_Optimize_Free_Memory ( void )
{
	if (wp_optimize_memory_initialized == qtrue)
	{
		free(optimized_nodes);
		wp_optimize_memory_initialized = qfalse;
	}
}

/* */
qboolean
Optimize_AddNode ( vec3_t origin, int fl, short int *ents, int objFl )
{
	if ( optimized_number_of_nodes + 1 > MAX_NODES )
	{
		return ( qfalse );
	}

	VectorCopy( origin, optimized_nodes[optimized_number_of_nodes].origin );	//set the node's position

	nodes[optimized_number_of_nodes].type = fl;						//set the flags (NODE_OBJECTIVE, for example)
	nodes[optimized_number_of_nodes].objectNum[0] = ents[0];			//set any map objects associated with this node
	nodes[optimized_number_of_nodes].objectNum[1] = ents[1];			//only applies to objects linked to the unreachable flag
	nodes[optimized_number_of_nodes].objectNum[2] = ents[2];
	nodes[optimized_number_of_nodes].objFlags = objFl;				//set the objective flags
	optimized_number_of_nodes++;
	return ( qtrue );
}

qboolean Is_Waypoint_Entity ( int eType )
{
	switch (eType)
	{
	//case ET_GENERAL:
	//case ET_PLAYER:
	case ET_ITEM:
	//case ET_MISSILE:
	//case ET_SPECIAL:				// rww - force fields
	case ET_HOLOCRON:			// rww - holocron icon displays
	//case ET_MOVER:
	//case ET_BEAM:
	case ET_PORTAL:
	//case ET_SPEAKER:
	case ET_PUSH_TRIGGER:
	case ET_TELEPORT_TRIGGER:
	//case ET_INVISIBLE:
	case ET_NPC:					// ghoul2 player-like entity
	case ET_TEAM:
	//case ET_BODY:
	//case ET_TERRAIN:
	//case ET_FX:
#ifdef __DOMINANCE__
	case ET_FLAG:
#endif //__DOMINANCE__
		return qtrue;
		break;
	default:
		break;
	}

	return qfalse;
}

//#define DEFAULT_AREA_SEPERATION 512
#define DEFAULT_AREA_SEPERATION 340
#define MAP_BOUNDS_OFFSET 2048

int ClosestNodeTo(vec3_t origin, qboolean isEntity)
{
	int		i;
	float	AREA_SEPERATION = DEFAULT_AREA_SEPERATION;
	float	closest_dist = 8192.0f;// 1024.0f;//AREA_SEPERATION*1.5;
	int		closest_node = -1;

	for (i = 0; i < number_of_nodes; i++)
	{
		float dist = VectorDistance(nodes[i].origin, origin);

		if (dist >= closest_dist)
			continue;

		if (nodes[i].objectNum[0] == 1)
			continue; // Skip water/ice disabled node!

		if (isEntity)
		{
			if (DistanceVertical(origin, nodes[i].origin) > 64)
				continue;
		}

		closest_dist = dist;
		closest_node = i;
	}

	return closest_node;
}

extern void AIMOD_SaveMapCoordFile ( void );
extern qboolean AIMOD_LoadMapCoordFile ( void );
extern void AIMod_GetMapBounts ( void );
extern qboolean CG_SpawnVector2D( const char *key, const char *defaultString, float *out );

qboolean bad_surface = qfalse;
int		aw_num_bad_surfaces = 0;

/* */
int
AI_PM_GroundTrace ( vec3_t point, int clientNum, trace_t *trace )
{
	vec3_t	playerMins = {-18, -18, -24};
	vec3_t	playerMaxs = {18, 18, 48};
	vec3_t	point2;
	VectorCopy( point, point2 );
	point2[2] -= 128.0f;

	bad_surface = qfalse;

	CG_Trace( trace, point, playerMins, playerMaxs, point2, clientNum, (MASK_PLAYERSOLID | MASK_WATER) & ~CONTENTS_BODY/*MASK_SHOT*/ );

	//if ( (trace.surfaceFlags & SURF_NODRAW) && (trace.surfaceFlags & SURF_NOMARKS) && !HasPortalFlags(trace.surfaceFlags, trace.contents) )
	//	bad_surface = qtrue;

	/* TESTING THIS */
	if ( (trace->surfaceFlags & SURF_NODRAW)
		&& (trace->surfaceFlags & SURF_NOMARKS)
		&& !((trace->materialType) == MATERIAL_WATER)
		&& !(
#ifndef __DISABLE_PLAYERCLIP__
		(trace->contents & CONTENTS_PLAYERCLIP) &&
#endif //__DISABLE_PLAYERCLIP__
		(trace->contents & CONTENTS_TRANSLUCENT)) )
		bad_surface = qtrue;

	if ( (trace->surfaceFlags & SURF_SKY) )
		bad_surface = qtrue;

	if ((trace->materialType) == MATERIAL_WATER)
	{
		int contents = trace->contents;
		contents |= CONTENTS_WATER;
		return contents;
	}

	return (trace->contents);
}

void AIMOD_AI_InitNodeContentsFlags ( void )
{
	int i;

	for (i = 0; i < number_of_nodes; i++)
	{
		trace_t tr;
		int contents = 0;
		vec3_t up_pos;

		VectorCopy(nodes[i].origin, up_pos);
		up_pos[2]+= 32;

		contents = AI_PM_GroundTrace(nodes[i].origin, -1, &tr);

		if (contents & CONTENTS_LAVA)
		{// Not ICE, but still avoid if possible!
			if (!(nodes[i].type & NODE_ICE))
				nodes[i].type |= NODE_ICE;
		}

		if (contents & CONTENTS_SLIME)
		{
			if (!(nodes[i].type & NODE_ICE))
				nodes[i].type |= NODE_ICE;
		}

		if (contents & CONTENTS_WATER || (tr.materialType) == MATERIAL_WATER || (tr.contents & CONTENTS_WATER))
		{
			nodes[i].type |= NODE_WATER;
		}

		if (bad_surface)
		{// Made code also check for sky surfaces, should be able to remove a lot of crappy waypoints on the skybox!
			nodes[i].objectNum[0] = 1;
			aw_num_bad_surfaces++;
		}
	}
}

#define MAX_ROUTE_FILE_LENGTH (1024 * 1024 * 1.6) // 1.6mb

int FileLength(FILE *fp)
{
	int pos;
	int end;

	pos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	end = ftell(fp);
	fseek(fp, pos, SEEK_SET);

	return end;
} //end of the function FileLength

qboolean IsPathDataTooLarge(const char *mapname)
{
	fileHandle_t f;
	int len;

	len = trap->FS_Open(va("botroutes/%s.wnt", mapname), &f, FS_READ);

	if (!f || len <= 0)
	{
		FILE *fp;

		fp = fopen(va("%s/botroutes/%s.wnt", MOD_DIRECTORY, mapname), "rb");

		if (!fp)
			return qfalse;

		len = FileLength(fp);

		if (len >= MAX_ROUTE_FILE_LENGTH)
			return qtrue;

		return qfalse;
	}

#ifdef __DOMINANCE__
	if (len >= MAX_ROUTE_FILE_LENGTH)
#else //__DOMINANCE__
	if (len >= 524288)
#endif //__DOMINANCE__
	{
		trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^1Created route file exceeds maximum length - Running another optimization!\n");
		trap->FS_Close(f);
		return qtrue;
	}

	trap->FS_Close(f);

	return qfalse;
}
extern int Q_TrueRand ( int low, int high );

qboolean AIMod_AutoWaypoint_Cleaner_NodeHasLinkWithFewLinks ( int node )
{// Check the links of the current node for any nodes with very few links (so we dont remove an important node for reachability)
	int i = 0;
	int removed_neighbours_count = 0;

	for (i = 0; i < nodes[node].enodenum; i++)
	{
		if (nodes[node].enodenum > 64)
			continue; // Corrupt?

		if (nodes[nodes[node].links[i].targetNode].objectNum[0] == 1)
			removed_neighbours_count++;
	}

	if (nodes[node].enodenum - removed_neighbours_count <= 8/*4*/)
		return qtrue; // lready removed too many neighbours here! Remove no more!

	for (i = 0; i < nodes[node].enodenum; i++)
	{
		if (nodes[node].enodenum > 64)
			continue; // Corrupt?

		if (nodes[nodes[node].links[i].targetNode].enodenum <= 8/*4*/)
			return qtrue;
	}

	return qfalse;
}

int			trigger_hurt_counter = 0;
centity_t	*trigger_hurt_list[MAX_GENTITIES];

void AIMod_AutoWaypoint_Trigger_Hurt_List_Setup ( void )
{// Generate a list for the current map to speed up processing...
	/*int i = 0;

	trigger_hurt_counter = 0;

	for (i = 0; i < MAX_GENTITIES; i++)
	{
		centity_t *cent = &cg_entities[i];

		if (cent->currentState.eType == ET_TRIGGER_HURT)
		{
			trigger_hurt_list[trigger_hurt_counter] = cent;
			trigger_hurt_counter++;
		}
	}*/
}

qboolean LocationIsNearTriggerHurt ( vec3_t origin )
{
	/*int i = 0;

	for (i = 0; i < trigger_hurt_counter; i++)
	{
		if (VectorDistance(trigger_hurt_list[i]->currentState.origin, origin) < 64)
			return qtrue;
	}
	*/

	return qfalse;
}

qboolean HasSameLinksAsNode (int node, int node2, qboolean LINKS_TO_US, qboolean WE_LINK_TO_THEM)
{
	int j;

	for (j = 0; j < nodes[node].enodenum; j++)
	{
		int k = 0;
		qboolean found = qfalse;

		if (WE_LINK_TO_THEM && nodes[node].links[j].targetNode == node2)
			continue; // this is a link to us.. ignore...

		// See if this node has this link target node (j)
		for (k = 0; k < nodes[node2].enodenum; k++)
		{
			if (nodes[node2].links[k].targetNode == nodes[node].links[j].targetNode)
			{
				found = qtrue;
				break;
			}
		}

		if (!found)
		{// This one was not found... It does not have all our links...
			return qfalse;
		}
	}

	// If we got to here, then it has all our links...
	return qtrue;
}

qboolean NodeLinksTo (int node, int nodeTo)
{
	int j;

	for (j = 0; j < nodes[node].enodenum; j++)
	{
		if (nodes[node].links[j].targetNode == nodeTo)
			return qtrue; // this is a link to us..
	}

	return qfalse;
}

int num_dupe_nodes = 0;

void CheckForNearbyDupeNodes( int node )
{// Removes any nodes with the same links as this node to reduce numbers...
	int i = 0;

	if (nodes[node].objectNum[0] == 1)
		return; // This node is disabled already...

	for (i = 0; i < number_of_nodes; i++)
	{
		int j = 0;
		//int num_same = 0;
		qboolean REMOVE_NODE = qtrue;
		qboolean LINKS_TO_US = qfalse;
		qboolean WE_LINK_TO_THEM = qfalse;

		if (i == node)
			continue;

		if (nodes[i].enodenum <= 0)
			continue;

		if (nodes[i].objectNum[0] == 1)
			continue;

		if (VectorDistance(nodes[node].origin, nodes[i].origin) > waypoint_scatter_distance*waypoint_distance_multiplier)
			continue;

		if (NodeLinksTo(i, node))
		{// This node links to us... Ignore this link...
			LINKS_TO_US = qtrue;

			if (nodes[i].enodenum-1 > nodes[node].enodenum)
				continue; // Never remove a better node...
		}

		if (NodeLinksTo(node, i))
		{// We link to this node... Ignore us...
			WE_LINK_TO_THEM = qtrue;

			if (nodes[i].enodenum > nodes[node].enodenum-1)
				continue; // Never remove a better node...
		}

		REMOVE_NODE = HasSameLinksAsNode(node, i, LINKS_TO_US, WE_LINK_TO_THEM);

		if (REMOVE_NODE)
		{// Node i is a dupe node! Disable it!
			if ((nodes[i].type & NODE_LAND_VEHICLE) && !(nodes[node].type & NODE_LAND_VEHICLE))
			{// Never remove a vehicle node and replace it with a non vehicle one!

			}
			else
			{
				nodes[i].objectNum[0] = 1;
				num_dupe_nodes++;
			}
		}
	}
}

int num_skiped_nodes = 0;

void CheckWalkAround ( int node )
{// Removes any nodes that we can get to by another close route...
	int i = 0;
	qboolean HAS_ALL_LINKS = qtrue;

	if (nodes[node].objectNum[0] == 1)
		return; // This node is disabled already...

	for (i = 0; i < nodes[node].enodenum; i++)
	{// Go through each of this node's links to find a path around us...
		qboolean REMOVE_NODE = qtrue;
		qboolean LINKS_TO_US = qfalse;
		qboolean WE_LINK_TO_THEM = qfalse;
		int thisLink = nodes[node].links[i].targetNode;
		int j;
		int CAN_GET_TO_THISLINK = 0;
		int CAN_GET_TO_THISLINK2 = 0;
		int CAN_GET_TO_US = 0;
		int CAN_GET_TO_US2 = 0;

		if (thisLink == node)
			continue; // this is just a link to us... ignore the link to us...

		if (nodes[thisLink].enodenum <= 0)
			continue; // This link has no links, ignore it...

		if (nodes[thisLink].objectNum[0] == 1)
			continue; // This link is disabled... Ignore it...

		if (nodes[node].links[i].flags & NODE_JUMP)
			continue; // This is a jump-to node... Skip it...

		// Looks like this is a valid place we need to be able to get to... See if the other links around us also go there...
		for (j = 0; j < nodes[node].enodenum; j++)
		{
			int toLink = nodes[node].links[j].targetNode;

			if (thisLink == toLink)
				continue; // The same link as we are searching for a link to, ignore...

			if (nodes[toLink].objectNum[0] == 1)
				continue; // This link is disabled... Ignore it...

			if (nodes[toLink].enodenum <= 0)
				continue; // This link has no links, ignore it...

			//if (nodes[toLink].origin[2]-16 > nodes[thisLink].origin[2])
			//	continue; // Always remove higher waypoints (ledges) instead... Hopefully this will force it to remove that link instead...

			if ((CAN_GET_TO_THISLINK == 0 || CAN_GET_TO_THISLINK2 == 0) && NodeLinksTo(toLink, thisLink))
			{// This link has a route to this wanted node...
				if (toLink != CAN_GET_TO_US && toLink != CAN_GET_TO_US2) // Need both a from and a to node for a complete path...
				{
					if (!CAN_GET_TO_THISLINK)
						CAN_GET_TO_THISLINK = toLink;
					else
						CAN_GET_TO_THISLINK2 = toLink;
				}
			}

			if ((CAN_GET_TO_US == 0 || CAN_GET_TO_US2 == 0) && NodeLinksTo(thisLink, toLink))
			{// We have a route to this wanted node...
				if (toLink != CAN_GET_TO_THISLINK && toLink != CAN_GET_TO_THISLINK2) // Need both a from and a to node for a complete path...
				{
					if (!CAN_GET_TO_US)
						CAN_GET_TO_US = toLink;
					else
						CAN_GET_TO_US2 = toLink;
				}
			}

			if (CAN_GET_TO_THISLINK && CAN_GET_TO_US && CAN_GET_TO_THISLINK2 && CAN_GET_TO_US2)
			{// Looks like we found a way around this node for this link...
				break; // All is good for this link... Stop looking for ways there...
			}
		}

		if (!(CAN_GET_TO_THISLINK && CAN_GET_TO_US && CAN_GET_TO_THISLINK2 && CAN_GET_TO_US2))
		{// Looks like we never found a way around this node for this link... We can't delete this waypoint...
			HAS_ALL_LINKS = qfalse;
			break; // Early out...
		}
	}

	if (HAS_ALL_LINKS)
	{// Seems we found a way to every one of this node's links... Should be safe to remove it...
		nodes[node].objectNum[0] = 1;
		num_skiped_nodes++;
	}
}

qboolean CheckIfTooManyLinksRemoved ( int node, qboolean extra )
{
	int i = 0;
	int num_found = 0;

	for (i = 0; i < nodes[node].enodenum; i++)
	{
		int target = nodes[node].links[i].targetNode;

		if (target < 0)
			continue;

		if (target > number_of_nodes)
			continue;

		if (nodes[target].objectNum[0] == 1)
			continue;

		num_found++;

		if (extra)
		{
			if (num_found > nodes[node].enodenum*0.3)//4)
				return qfalse;
		}
		else
		{
			if (num_found > nodes[node].enodenum*0.6)//8)
				return qfalse;
		}
	}

	return qtrue;
}

vec3_t	LIFT_POINTS[MAX_NODES];
int		NUM_LIFT_POINTS = 0;

void AIMod_AddLiftPoint ( void )
{
	VectorCopy(cg.refdef.vieworg, LIFT_POINTS[NUM_LIFT_POINTS]);
	trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Lift Point added at %f %f %f...\n", LIFT_POINTS[NUM_LIFT_POINTS][0], LIFT_POINTS[NUM_LIFT_POINTS][1], LIFT_POINTS[NUM_LIFT_POINTS][2]) );
	trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Run a /awc relink to complete all lift additions...\n") );
	NUM_LIFT_POINTS++;
}

#if 0
void AIMod_AddLifts ( void )
{
	int liftNum = 0;
	int orig_num_nodes = number_of_nodes;

	if (NUM_LIFT_POINTS <= 0) return;

	if (!cg.mapcoordsValid)
		AIMod_GetMapBounts();

	trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Adding %i lifts...\n", NUM_LIFT_POINTS) );

	//
	// Movers...
	//
	for (liftNum = 0; liftNum < NUM_LIFT_POINTS; liftNum++)
	{// Need to generate waypoints to the top/bottom of the mover...
		float temp_roof, temp_ground;
		vec3_t temp_org, temp_org2;

		VectorCopy(LIFT_POINTS[liftNum], temp_org2);
		VectorCopy(LIFT_POINTS[liftNum], temp_org);
		temp_org[2] += waypoint_scatter_distance; // Start above the lift...
		temp_roof = RoofHeightAt(temp_org);

		VectorCopy(LIFT_POINTS[liftNum], temp_org);
		temp_org[2] -= waypoint_scatter_distance*2; // Start below the lift...
		temp_ground = GroundHeightAt(temp_org);

		temp_org2[2] = temp_roof;

		if (/*temp_roof <= cg.mapcoordsMaxs[2] &&*/ DistanceVertical(temp_org, temp_org2) >= 128)
		{// Looks like it goes up!
			int z = 0;

			VectorCopy(LIFT_POINTS[liftNum], temp_org);
			temp_org[2] += waypoint_scatter_distance;

			while (temp_org[2] <= temp_org2[2])
			{// Add waypoints all the way up!
				VectorCopy(temp_org, nodes[number_of_nodes].origin);
				number_of_nodes++;
				temp_org[2] += waypoint_scatter_distance;
			}
		}

		temp_org2[2] = temp_ground;

		if (/*temp_roof >= cg.mapcoordsMins[2] &&*/ DistanceVertical(temp_org, temp_org2) >= 128)
		{// Looks like it goes down!
			int z = 0;

			VectorCopy(LIFT_POINTS[liftNum], temp_org);
			temp_org[2] -= waypoint_scatter_distance;

			while (temp_org[2] >= temp_org2[2])
			{// Add waypoints all the way up!
				VectorCopy(temp_org, nodes[number_of_nodes].origin);
				number_of_nodes++;
				temp_org[2] -= waypoint_scatter_distance;
			}
		}
	}

	trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Finished adding %i waypoints for %i lifts...\n", number_of_nodes - orig_num_nodes, NUM_LIFT_POINTS) );

	NUM_LIFT_POINTS = 0;
}
#endif //0

vec3_t	ADD_POINTS[MAX_NODES];
int		NUM_ADD_POINTS = 0;

void AIMod_AddWayPoint ( void )
{
	VectorCopy(cg.refdef.vieworg, ADD_POINTS[NUM_ADD_POINTS]);
	ADD_POINTS[NUM_ADD_POINTS][2] -= 48.0;

	trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Waypoint will be added at %f %f %f...\n", ADD_POINTS[NUM_ADD_POINTS][0], ADD_POINTS[NUM_ADD_POINTS][1], ADD_POINTS[NUM_ADD_POINTS][2]) );
	trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Run a /awc relink to complete all lift additions...\n") );
	NUM_ADD_POINTS++;
}

vec3_t	REMOVAL_POINTS[MAX_NODES];
int		NUM_REMOVAL_POINTS = 0;

void AIMod_AddRemovalPoint ( void )
{
	VectorCopy(cg.refdef.vieworg, REMOVAL_POINTS[NUM_REMOVAL_POINTS]);
	trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Removal Point added at %f %f %f...\n", REMOVAL_POINTS[NUM_REMOVAL_POINTS][0], REMOVAL_POINTS[NUM_REMOVAL_POINTS][1], REMOVAL_POINTS[NUM_REMOVAL_POINTS][2]) );
	trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Waypoints within 128 of this location will be removed on awc run...\n") );
	NUM_REMOVAL_POINTS++;
}

void AIMod_AWC_MarkBadHeight ( void )
{// Mark this height on the map as bad for waypoints...
	BAD_HEIGHTS[NUM_BAD_HEIGHTS] = cg.refdef.vieworg[2];
	NUM_BAD_HEIGHTS++;
	trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Height %f marked as bad for waypoints.\n", cg.refdef.vieworg[2]) );
}

int BOT_GetFCost(int to, int num, int parentNum, float *gcost)
{
	float	gc = 0;
	float	hc = 0;
	vec3_t	v;
	float	height_diff = 0;

	if (gcost[num] == -1)
	{
		if (parentNum != -1)
		{
			gc = gcost[parentNum];
			VectorSubtract(nodes[num].origin, nodes[parentNum].origin, v);
			gc += VectorLength(v);

			gc += DistanceVertical(nodes[num].origin, nodes[parentNum].origin) * 4;

			if (gc > 64000)
				gc = 64000.0f;
		}

		gcost[num] = gc;
	}
	else
	{
		gc = gcost[num];
	}

	hc = Distance(nodes[num].origin, nodes[parentNum].origin);
	height_diff = DistanceVertical(nodes[num].origin, nodes[parentNum].origin);
	hc += (height_diff * height_diff); // Squared for massive preferance to staying at same plane...

	return (int)((gc*0.1) + (hc*0.1));
}

int ASTAR_FindPathFast(int from, int to, int *pathlist, qboolean shorten)
{
	//all the data we have to hold...since we can't do dynamic allocation, has to be MAX_WPARRAY_SIZE
	//we can probably lower this later - eg, the open list should never have more than at most a few dozen items on it
	int			badwp = -1;
	int			numOpen = 0;
	int			atNode, temp, newnode = -1;
	qboolean	found = qfalse;
	int			count = -1;
	float		gc;
	int			i, j, u, v, m;
	int			gWPNum = number_of_nodes;

	int			*openlist;					//add 1 because it's a binary heap, and they don't use 0 - 1 is the first used index
	float		*gcost;
	int			*fcost;
	char		*list;						//0 is neither, 1 is open, 2 is closed - char because it's the smallest data type
	int			*parent;

	if ((from == NODE_INVALID) || (to == NODE_INVALID) || (from >= gWPNum) || (to >= gWPNum) || (from == to))
	{
		//trap->Print("Bad from or to node.\n");
		return (-1);
	}

	// Check if memory needs to be allocated...
	openlist = (int *)malloc(sizeof(int)*(MAX_NODES + 1));
	gcost = (float *)malloc(sizeof(float)*(MAX_NODES));
	fcost = (int *)malloc(sizeof(int)*(MAX_NODES));
	list = (char *)malloc(sizeof(char)*(MAX_NODES));
	parent = (int *)malloc(sizeof(int)*(MAX_NODES));

	memset(openlist, 0, (sizeof(int)* (MAX_NODES + 1)));
	memset(gcost, 0, (sizeof(float)* MAX_NODES));
	memset(fcost, 0, (sizeof(int)* MAX_NODES));
	memset(list, 0, (sizeof(char)* MAX_NODES));
	memset(parent, 0, (sizeof(int)* MAX_NODES));

	{
//#pragma omp parallel for ordered schedule(dynamic)
		for (i = 0; i < gWPNum; i++)
		{
			gcost[i] = Distance(nodes[i].origin, nodes[to].origin);
		}
	}

	openlist[gWPNum + 1] = 0;

	openlist[1] = from;																	//add the starting node to the open list
	numOpen++;
	gcost[from] = 0;																	//its f and g costs are obviously 0
	fcost[from] = 0;

	while (1)
	{
		if (numOpen != 0)																//if there are still items in the open list
		{
			//pop the top item off of the list
			atNode = openlist[1];
			list[atNode] = 2;															//put the node on the closed list so we don't check it again
			numOpen--;
			openlist[1] = openlist[numOpen + 1];										//move the last item in the list to the top position
			v = 1;

			//this while loop reorders the list so that the new lowest fcost is at the top again
			while (1)
			{
				u = v;
				if ((2 * u + 1) < numOpen)											//if both children exist
				{
					if (fcost[openlist[u]] >= fcost[openlist[2 * u]])
					{
						v = 2 * u;
					}

					if (fcost[openlist[v]] >= fcost[openlist[2 * u + 1]])
					{
						v = 2 * u + 1;
					}
				}
				else
				{
					if ((2 * u) < numOpen)											//if only one child exists
					{
						if (fcost[openlist[u]] >= fcost[openlist[2 * u]])
						{
							v = 2 * u;
						}
					}
				}

				if (u != v)															//if they're out of order, swap this item with its parent
				{
					temp = openlist[u];
					openlist[u] = openlist[v];
					openlist[v] = temp;
				}
				else
				{
					break;
				}
			}

			for (i = 0; i < nodes[atNode].enodenum && i < MAX_NODELINKS; i++)								//loop through all the links for this node
			{
				newnode = nodes[atNode].links[i].targetNode;

				if (newnode > gWPNum)
					continue;

				if (newnode < 0)
					continue;

				if (nodes[newnode].objectNum[0] == 1)
					continue; // This node was disabled already by the waypointer, never try to use it...

				if (list[newnode] == 2)
				{																		//if this node is on the closed list, skip it
					continue;
				}

				if (list[newnode] != 1)												//if this node is not already on the open list
				{
					openlist[++numOpen] = newnode;										//add the new node to the open list
					list[newnode] = 1;
					parent[newnode] = atNode;											//record the node's parent

					if (newnode == to)
					{																	//if we've found the goal, don't keep computing paths!
						break;															//this will break the 'for' and go all the way to 'if (list[to] == 1)'
					}

					fcost[newnode] = BOT_GetFCost(to, newnode, parent[newnode], gcost);	//store it's f cost value

					//this loop re-orders the heap so that the lowest fcost is at the top
					m = numOpen;

					while (m != 1)													//while this item isn't at the top of the heap already
					{
						if (fcost[openlist[m]] <= fcost[openlist[m / 2]])				//if it has a lower fcost than its parent
						{
							temp = openlist[m / 2];
							openlist[m / 2] = openlist[m];
							openlist[m] = temp;											//swap them
							m /= 2;
						}
						else
						{
							break;
						}
					}
				}
				else										//if this node is already on the open list
				{
					gc = gcost[atNode];

					if (nodes[atNode].links[i].cost > 0 && nodes[atNode].links[i].cost < 9999)
					{// UQ1: Already have a cost value, skip the calculations!
						gc += nodes[atNode].links[i].cost;
					}
					else
					{
						vec3_t	vec;

						VectorSubtract(nodes[newnode].origin, nodes[atNode].origin, vec);
						gc += VectorLength(vec);				//calculate what the gcost would be if we reached this node along the current path
						nodes[atNode].links[i].cost = VectorLength(vec);
					}

					if (gc < gcost[newnode])				//if the new gcost is less (ie, this path is shorter than what we had before)
					{
						parent[newnode] = atNode;			//set the new parent for this node
						gcost[newnode] = gc;				//and the new g cost

						for (j = 1; j < numOpen; j++)		//loop through all the items on the open list
						{
							if (openlist[j] == newnode)	//find this node in the list
							{
								//calculate the new fcost and store it
								fcost[newnode] = BOT_GetFCost(to, newnode, parent[newnode], gcost);

								//reorder the list again, with the lowest fcost item on top
								m = j;

								while (m != 1)
								{
									if (fcost[openlist[m]] < fcost[openlist[m / 2]])	//if the item has a lower fcost than it's parent
									{
										temp = openlist[m / 2];
										openlist[m / 2] = openlist[m];
										openlist[m] = temp;								//swap them
										m /= 2;
									}
									else
									{
										break;
									}
								}
								break;													//exit the 'for' loop because we already changed this node
							}															//if
						}																//for
					}											//if (gc < gcost[newnode])
				}												//if (list[newnode] != 1) --> else
			}													//for (loop through links)
		}														//if (numOpen != 0)
		else
		{
			found = qfalse;										//there is no path between these nodes
			break;
		}

		if (list[to] == 1)									//if the destination node is on the open list, we're done
		{
			found = qtrue;
			break;
		}
	}															//while (1)

	if (found == qtrue)							//if we found a path, and are trying to store the pathlist...
	{
		count = 0;
		temp = to;												//start at the end point

		while (temp != from)									//travel along the path (backwards) until we reach the starting point
		{
			if (count + 1 >= MAX_WPARRAY_SIZE)
			{
				trap->Print("ERROR: pathlist count > MAX_WPARRAY_SIZE.\n");
				return -1; // UQ1: Added to stop crash if path is too long for the memory allocation...
			}

			pathlist[count++] = temp;							//add the node to the pathlist and increment the count
			temp = parent[temp];								//move to the parent of this node to continue the path
		}

		pathlist[count++] = from;								//add the beginning node to the end of the pathlist




		free(openlist);
		free(gcost);
		free(fcost);
		free(list);
		free(parent);


		//trap->Print("Pathsize is %i.\n", count);
		return (count);
	}

	free(openlist);
	free(gcost);
	free(fcost);
	free(list);
	free(parent);

	//trap->Print("Failed to find path.\n");
	return (-1);											//return the number of nodes in the path, -1 if not found
}

qboolean WP_CheckInSolid (vec3_t position)
{
	trace_t	trace;
	vec3_t	end, mins, maxs;
	vec3_t pos;

	int contents = CONTENTS_TRIGGER;
	int clipmask = MASK_DEADSOLID;

	VectorSet(mins, -15, -15, DEFAULT_MINS_2);
	VectorSet(maxs, 15, 15, DEFAULT_MAXS_2);

	VectorCopy(position, pos);
	pos[2] += 32;// 28;
	VectorCopy(pos, end);
	end[2] += mins[2];
	mins[2] = 0;

	CG_Trace(&trace, position, mins, maxs, end, -1, clipmask);
	if(trace.allsolid || trace.startsolid)
	{
		return qtrue;
	}

	if(trace.fraction < 1.0)
	{
		return qtrue;
	}

	return qfalse;
}

qboolean Warzone_CheckBelowWaypoint( int wp, qboolean checkSurfaces )
{
	trace_t tr;
	vec3_t org, org2;

	VectorCopy(nodes[wp].origin, org);
	org[2] += 24;

	if (WP_CheckInSolid(org)) return qfalse;

	if (checkSurfaces)
	{
		VectorCopy(nodes[wp].origin, org);
		VectorCopy(nodes[wp].origin, org2);
		org2[2] = -MAX_MAP_SIZE;//org[2] - 256;

		CG_Trace(&tr, org, NULL, NULL, org2, -1, MASK_PLAYERSOLID | CONTENTS_TRIGGER);

		if (tr.startsolid)
		{
			//trap->Print("Waypoint %i is in solid.\n", wp);
			return qfalse;
		}

		if (tr.allsolid)
		{
			//trap->Print("Waypoint %i is in solid.\n", wp);
			return qfalse;
		}

		if (tr.fraction == 1)
		{
			//trap->Print("Waypoint %i is too high above ground.\n", wp);
			return qfalse;
		}

		if (tr.contents & CONTENTS_LAVA)
		{
			//trap->Print("Waypoint %i is in lava.\n", wp);
			return qfalse;
		}

		if (tr.contents & CONTENTS_SLIME)
		{
			//trap->Print("Waypoint %i is in slime.\n", wp);
			return qfalse;
		}

		if (tr.contents & CONTENTS_TRIGGER)
		{
			//trap->Print("Waypoint %i is in trigger.\n", wp);
			return qfalse;
		}

		if ((tr.surfaceFlags & SURF_NOMARKS) && (tr.surfaceFlags & SURF_NODRAW))
		{
			//trap->Print("Waypoint %i is in trigger.\n", wp);
			return qfalse;
		}
	}

	return qtrue;
}

/*
typedef enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_SPECIAL,				// rww - force fields
	ET_HOLOCRON,			// rww - holocron icon displays
	ET_MOVER,
	ET_BEAM,
	ET_PORTAL,
	ET_SPEAKER,
	ET_PUSH_TRIGGER,
	ET_TELEPORT_TRIGGER,
	ET_INVISIBLE,
	ET_NPC,					// ghoul2 player-like entity
	ET_TEAM,
	ET_BODY,
	ET_TERRAIN,
	ET_FX,
	ET_MOVER_MARKER,
	ET_SPAWNPOINT,
	ET_TRIGGER_HURT,
	ET_SERVERMODEL,

	ET_FREED,				// UQ1: Added to mark freed entities...

	ET_EVENTS				// any of the EV_* events can be added freestanding
							// by setting eType to ET_EVENTS + eventNum
							// this avoids having to set eFlags and eventNum
} entityType_t;
*/

qboolean Warzone_CheckRoutingFrom( int wp )
{
	int i;
	centity_t *spot = NULL;
	int wpCurrent = -1;
	int goal = wp;
	int pathsize = 0;
	int pathlist[MAX_NODES];
	int tests_completed = 0;
	int num_items = 0;

	// Try other ents...
	for (i = MAX_CLIENTS; i < MAX_GENTITIES;i++)
	{
		centity_t *cent = &cg_entities[i];

		if (!cent) continue;

#if 0
		if (cent->currentState.eType != ET_ITEM
			&& cent->currentState.eType != ET_HOLOCRON
			&& cent->currentState.eType != ET_PORTAL
			&& cent->currentState.eType != ET_PUSH_TRIGGER
			&& cent->currentState.eType != ET_TELEPORT_TRIGGER
			//&& cent->currentState.eType != ET_NPC
			//&& cent->currentState.eType != ET_GENERAL
			&& cent->currentState.eType != ET_MOVER_MARKER)
		continue;
#else
		if (cent->currentState.eType != ET_SPAWNPOINT)
			continue;
#endif

		if ( cent->currentState.origin[0] == 0 && cent->currentState.origin[1] == 0 && cent->currentState.origin[2] == 0)
			continue;

		if (Distance(cent->currentState.origin, nodes[goal].origin) < 512)
			continue; // Need something harder...

		wpCurrent = ClosestNodeTo(cent->currentState.origin, qfalse);

		if (wpCurrent)
		{
			pathsize = ASTAR_FindPathFast(wpCurrent, goal, pathlist, qfalse);

			num_items++;

			if (pathsize > 0)
			{
				return qtrue; // Found a route... This waypoint looks good...
			}
		}

		tests_completed++;
	}

	return qfalse;
}

/* */
void
AIMod_AutoWaypoint_Cleaner ( qboolean quiet, qboolean null_links_only, qboolean relink_only, qboolean multipass, qboolean initial_pass, qboolean extra, qboolean marked_locations, qboolean extra_reach, qboolean reset_reach, qboolean convert_old, qboolean pathtest, qboolean remove_trees )
{
	int i = 0;//, j = 0;//, k = 0, l = 0;//, m = 0;
	int	total_calculations = 0;
	int	calculations_complete = 0;
	//int	*areas;//[16550];
//	int num_areas = 0;
#if 0
	float map_size;
	vec3_t mapMins, mapMaxs;
	float temp;
#endif //0
//	float AREA_SEPERATION = DEFAULT_AREA_SEPERATION;
//	int screen_update_timer = 0;
//	int entities_start = 0;
//	int	node_disable_ticker = 0;
	int	num_disabled_nodes = 0;
	int num_nolink_nodes = 0;
	int num_tree_nodes = 0;
	int num_noroute_nodes = 0;
	int num_allsolid_nodes = 0;
	int num_this_location_nodes = 0;
	int num_marked_height_nodes = 0;
	int	node_disable_ratio = 2;
	int total_removed = 0;
//	qboolean	bad_surfaces_only = qfalse;
//	qboolean	noiceremove = qfalse;
//	qboolean	nowaterremove = qfalse;
//	int			skip = 0;
//	int			skip_threshold = 2;
//	qboolean	reducecount = qtrue;
	int			start_wp_total = 0;
	int			node_clean_ticker = 0;
	int			num_passes_completed = 0;
	float		original_wp_max_distance = 0;
	float		original_wp_scatter_multiplier = 0;
	qboolean	SCREENDRAW_ACTIVE = qfalse;

	trap->Cvar_Set("warzone_waypoint_render", "0");
	trap->UpdateScreen();
	trap->UpdateScreen();
	trap->UpdateScreen();

	aw_num_bad_surfaces = 0;
	num_dupe_nodes = 0;
	num_skiped_nodes = 0;

	// UQ1: Check if we have an SSE CPU.. It can speed up our memory allocation by a lot!
	if (!CPU_CHECKED)
		UQ_Get_CPU_Info();

	AIMod_AutoWaypoint_Init_Memory();
	AIMod_AutoWaypoint_Optimize_Init_Memory();

	if (number_of_nodes > 0)
	{// UQ1: Init nodes list!
		number_of_nodes = 0;
		optimized_number_of_nodes = 0;

		memset( nodes, 0, ((sizeof(node_t)+1)*MAX_NODES) );
		memset( optimized_nodes, 0, ((sizeof(node_t)+1)*MAX_NODES) );
	}

	if (convert_old)
	{
		AIMOD_NODES_LoadOldJKAPathData();
	}
	else
	{
		AIMOD_NODES_LoadNodes();
	}

	if (number_of_nodes <= 0)
	{
		AIMod_AutoWaypoint_Free_Memory();
		AIMod_AutoWaypoint_Optimize_Free_Memory();
		return;
	}

	start_wp_total = number_of_nodes;

	//areas = malloc( (sizeof(int)+1)*512000 );

	trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Cleaning waypoint list...\n") );
	strcpy( task_string1, va("^7Cleaning waypoint list....") );
	trap->UpdateScreen();

	trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^7This could take a while...\n") );
	strcpy( task_string2, va("^7This could take a while...") );
	trap->UpdateScreen();

	// UQ1: Set node types..
	AIMOD_AI_InitNodeContentsFlags();
	AIMod_AutoWaypoint_Trigger_Hurt_List_Setup();

	if (number_of_nodes > 160000)
		node_disable_ratio = 15;
	else if (number_of_nodes > 80000)
		node_disable_ratio = 12;
	else if (number_of_nodes > 40000)
		node_disable_ratio = 10;
	else if (number_of_nodes > 36000)
		node_disable_ratio = 9;
	else if (number_of_nodes > 32000)
		node_disable_ratio = 8;
	else if (number_of_nodes > 30000)
		node_disable_ratio = 7;
	else if (number_of_nodes > 24000)
		node_disable_ratio = 6;
	else if (number_of_nodes > 22000)
		node_disable_ratio = 5;
	else if (number_of_nodes > 18000)
		node_disable_ratio = 4;
	else if (number_of_nodes > 14000)
		node_disable_ratio = 3;
	else if (number_of_nodes > 8000)
		node_disable_ratio = 2;

	// waypoint_scatter_distance*waypoint_distance_multiplier
	// Get original awp's distance multiplier...
	for (i = 0; i < number_of_nodes; i++)
	{
		int j;

		for (j = 0; j < nodes[i].enodenum; j++)
		{
			float dist = VectorDistance(nodes[i].origin, nodes[nodes[i].links[j].targetNode].origin);

			if (dist*0.5 > original_wp_max_distance)
				original_wp_max_distance = dist*0.5;
		}
	}

	DO_EXTRA_REACH = qfalse;

	if (extra_reach)
	{
		DO_EXTRA_REACH = qtrue;
	}

	/*if (extra_reach)
	{// 1.5x?
		original_wp_max_distance *= 1.5f;
	}
	else*/ if (convert_old)
	{
		original_wp_max_distance = 4096.0f;
	}
	else if (reset_reach)
	{
		original_wp_max_distance = 96.0f;
	}

	if (convert_old)
	{
		aw_num_nodes = number_of_nodes;

		// Save the new list...
		{
			fileHandle_t	f;
			int				i;
			/*short*/ int		j;
			float			version = NOD_VERSION;										//version is 1.0 for now
			char			name[] = BOT_MOD_NAME;
			char			map[64] = "";
			char			filename[60];
			/*short*/ int		num_nodes = number_of_nodes;

			aw_num_nodes = number_of_nodes;
			strcpy( filename, "nodes/" );

			///////////////////
			//try to open the output file, return if it failed
			trap->FS_Open( va( "nodes/%s.bwp", cgs.currentmapname), &f, FS_WRITE );
			if ( !f )
			{
				trap->Print( "^1*** ^3ERROR^5: Error opening node file ^7nodes/%s.bwp^5!!!\n", cgs.currentmapname/*filename*/ );
				return;
			}

			for ( i = 0; i < aw_num_nodes/*num_nodes*/; i++ )
			{
				nodes[i].enodenum = 0;

				for ( j = 0; j < MAX_NODELINKS; j++ )
				{
					nodes[i].links[j].flags = 0;
					nodes[i].objectNum[0] = nodes[i].objectNum[1] = nodes[i].objectNum[2] = ENTITYNUM_NONE;
				}
			}

			num_nodes = aw_num_nodes;

			//trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );	//get the map name
			strcpy( map, cgs.currentmapname);
			trap->FS_Write( &name, strlen( name) + 1, f );								//write the mod name to the file
			trap->FS_Write( &version, sizeof(float), f );								//write the version of this file
			trap->FS_Write( &map, strlen( map) + 1, f );									//write the map name
			trap->FS_Write( &num_nodes, sizeof(/*short*/ int), f );							//write the number of nodes in the map

			for ( i = 0; i < aw_num_nodes/*num_nodes*/; i++ )											//loop through all the nodes
			{
				//trap->Print("Saved wp at %f %f %f.\n", nodes[i].origin[0], nodes[i].origin[1], nodes[i].origin[2]);

				//write all the node data to the file
				trap->FS_Write( &(nodes[i].origin), sizeof(vec3_t), f );
				trap->FS_Write( &(nodes[i].type), sizeof(int), f );
				trap->FS_Write( &(nodes[i].objectNum), sizeof(short int) * 3, f );
				trap->FS_Write( &(nodes[i].objFlags), sizeof(short int), f );
				trap->FS_Write( &(nodes[i].enodenum), sizeof(short int), f );
				for ( j = 0; j < nodes[i].enodenum; j++ )
				{
					trap->FS_Write( &(nodes[i].links[j].targetNode), sizeof(/*short*/ int), f );
					trap->FS_Write( &(nodes[i].links[j].flags), sizeof(short int), f );
				}
			}
			{
				//short int	fix = 1;
				short int	fix = 0;
				trap->FS_Write( &fix, sizeof(short int), f );
			}

			trap->FS_Close( f );													//close the file
			trap->Print( "^4*** ^3AUTO-WAYPOINTER^4: ^5Successfully saved node file ^7nodes/%s.bwp^5.\n", cgs.currentmapname/*filename*/ );
		}

		AIMod_AutoWaypoint_Free_Memory();
		AIMod_AutoWaypoint_Optimize_Free_Memory();
		DO_EXTRA_REACH = qfalse;
		return;
	}

	// Set out distance multiplier...
	original_wp_scatter_multiplier = waypoint_distance_multiplier;
	waypoint_distance_multiplier = original_wp_max_distance/waypoint_scatter_distance;

	/*
	if (relink_only || convert_old)
	{
		AIMod_AddLifts();
	}
	*/

	if (relink_only)
	{
		//
		// Add any manual waypoints...
		//
		for (i = 0; i < NUM_ADD_POINTS; i++)
		{
			VectorCopy(ADD_POINTS[i], nodes[number_of_nodes].origin);
			nodes[number_of_nodes].origin[2] = GroundHeightNoSurfaceChecks(nodes[number_of_nodes].origin) + 8.0;
			number_of_nodes++;
		}

		NUM_ADD_POINTS = 0;

		//
		// Add any manual elevator waypoints...
		//
		for (i = 0; i < NUM_LIFT_POINTS; i++)
		{
			int count = 0;
			float temp_roof, temp_ground;
			vec3_t temp_org;
			qboolean isDoor = qfalse;

			VectorCopy(LIFT_POINTS[i], temp_org);

			temp_ground = GroundHeightAt(temp_org);
			temp_roof = RoofHeightAt(temp_org);

			while (temp_org[2] <= temp_roof)
			{// Add waypoints all the way up!
				nodes[number_of_nodes].origin[0] = temp_org[0];
				nodes[number_of_nodes].origin[1] = temp_org[1];
				nodes[number_of_nodes].origin[2] = temp_org[2];
				number_of_nodes++;

				nodes[number_of_nodes].origin[0] = temp_org[0]+56.0;
				nodes[number_of_nodes].origin[1] = temp_org[1];
				nodes[number_of_nodes].origin[2] = temp_org[2];
				number_of_nodes++;

				nodes[number_of_nodes].origin[0] = temp_org[0];
				nodes[number_of_nodes].origin[1] = temp_org[1]+56.0;
				nodes[number_of_nodes].origin[2] = temp_org[2];
				number_of_nodes++;

				nodes[number_of_nodes].origin[0] = temp_org[0]+56.0;
				nodes[number_of_nodes].origin[1] = temp_org[1]+56.0;
				nodes[number_of_nodes].origin[2] = temp_org[2];
				number_of_nodes++;

				nodes[number_of_nodes].origin[0] = temp_org[0]-56.0;
				nodes[number_of_nodes].origin[1] = temp_org[1];
				nodes[number_of_nodes].origin[2] = temp_org[2];
				number_of_nodes++;

				nodes[number_of_nodes].origin[0] = temp_org[0];
				nodes[number_of_nodes].origin[1] = temp_org[1]-56.0;
				nodes[number_of_nodes].origin[2] = temp_org[2];
				number_of_nodes++;

				nodes[number_of_nodes].origin[0] = temp_org[0]-56.0;
				nodes[number_of_nodes].origin[1] = temp_org[1]-56.0;
				nodes[number_of_nodes].origin[2] = temp_org[2];
				number_of_nodes++;

				temp_org[2] += waypoint_scatter_distance;
				count++;
			}

			trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Added %i waypoints for manually added elevator %i.\n", count, i);
		}

		NUM_LIFT_POINTS = 0;
	}

	while (1)
	{
		trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Cleaning waypoints list... Please wait...\n") );
		strcpy( task_string3, va("^5Cleaning waypoints list... Please wait...") );
		trap->UpdateScreen();

		total_calculations = number_of_nodes;
		calculations_complete = 0;
		node_clean_ticker = 0;
		num_nolink_nodes = 0;
		num_tree_nodes = 0;
		num_noroute_nodes = 0;
		num_disabled_nodes = 0;
		num_allsolid_nodes = 0;
		num_dupe_nodes = 0;
		aw_num_bad_surfaces = 0;
		num_skiped_nodes = 0;
		num_this_location_nodes = 0;
		num_marked_height_nodes = 0;
		total_removed = 0;

		num_passes_completed++;

		for (i = 0; i < number_of_nodes; i++)
		{// Initialize...
			nodes[i].objectNum[0] = 0;
			nodes[i].objEntity = 0;
		}

		aw_percent_complete = 0.0f;
		aw_stage_start_time = clock();

		// Disable some ice/water ndoes...
		if (!relink_only && !convert_old)
		{
#pragma omp parallel for ordered schedule(dynamic)
			for (i = 0; i < number_of_nodes; i++)
			{
				if(omp_get_thread_num() == 0)
				{
					if (node_clean_ticker > 100)
					{
						//strcpy( last_node_added_string, va("^5Checking waypoint ^3%i ^5at ^7%f %f %f^5.", i, nodes[i].origin[0], nodes[i].origin[1], nodes[i].origin[2]) );
						strcpy( last_node_added_string, va("^5Completed ^3%i ^5of ^7%i^5 waypoints. (%i removed)", calculations_complete, total_calculations, num_skiped_nodes + num_dupe_nodes + num_disabled_nodes + aw_num_bad_surfaces + num_nolink_nodes + num_tree_nodes + num_noroute_nodes + num_allsolid_nodes + num_this_location_nodes + num_marked_height_nodes) );
						trap->UpdateScreen();
						node_clean_ticker = 0;
					}
				}

				if (remove_trees)
				{
					if (nodes[i].objectNum[0] == 1)
						continue;

					if (FOLIAGE_TreeSolidBlocking_AWP(nodes[i].origin))
					{
						nodes[i].objectNum[0] = 1;
						num_tree_nodes++;
						continue;
					}
				}
				else
				{
					node_clean_ticker++;
					calculations_complete++;

					// Draw a nice little progress bar ;)
					aw_percent_complete = (float)((float)((float)(calculations_complete)/(float)(total_calculations))*100.0f);



					if (nodes[i].objectNum[0] == 1)
						continue;

					if (nodes[i].enodenum <= 1/*0*/)
					{// Remove all waypoints without any links...
						nodes[i].objectNum[0] = 1;
						num_nolink_nodes++;
						continue;
					}

					/*if (checkSurfaces && !Warzone_CheckBelowWaypoint(i, qtrue))
					{// Removes all waypoints in solids...
						nodes[i].objectNum[0] = 1;
						num_allsolid_nodes++;
						continue;
					}
					else*/ if (!Warzone_CheckBelowWaypoint( i, qfalse ))
					{// Removes all waypoints in solids...
						nodes[i].objectNum[0] = 1;
						num_allsolid_nodes++;
						continue;
					}

					if (FOLIAGE_TreeSolidBlocking_AWP(nodes[i].origin))
					{
						nodes[i].objectNum[0] = 1;
						num_tree_nodes++;
						continue;
					}

					if (pathtest && !Warzone_CheckRoutingFrom( i ))
					{// Removes all waypoints without any route to the server's specified spawnpoint location...
						nodes[i].objectNum[0] = 1;
						num_noroute_nodes++;
						continue;
					}

					if (NUM_BAD_HEIGHTS > 0 || NUM_REMOVAL_POINTS > 0/*marked_locations*/)
					{
						int z = 0;

						if (AIMOD_IsWaypointHeightMarkedAsBad( nodes[i].origin ))
						{
							nodes[i].objectNum[0] = 1;
							num_marked_height_nodes++;
							continue;
						}

						for (z = 0; z < NUM_REMOVAL_POINTS; z++)
						{
							if (VectorDistance(nodes[i].origin, REMOVAL_POINTS[z]) <= 128)
							{
								nodes[i].objectNum[0] = 1;
								num_this_location_nodes++;
							}
						}

						// Skip cleaner...
						continue;
					}

					if (relink_only)
						continue;

					if (null_links_only)
						continue;

					if (nodes[i].enodenum >= 1)
					{
						CheckForNearbyDupeNodes(i);

						if (nodes[i].objectNum[0] != 1)
							CheckWalkAround(i);
					}
				}
			}
		}

		aw_percent_complete = 0.0f;
		trap->UpdateScreen();

		trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Disabled ^3%i^5 water/ice waypoints.\n", num_disabled_nodes);
		trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Disabled ^3%i^5 bad surfaces.\n", aw_num_bad_surfaces);
		trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Disabled ^3%i^5 waypoints in solids.\n", num_allsolid_nodes);
		trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Disabled ^3%i^5 waypoints without links.\n", num_nolink_nodes);
		trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Disabled ^3%i^5 waypoints inside trees.\n", num_tree_nodes);
		trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Disabled ^3%i^5 waypoints without valid routes.\n", num_noroute_nodes);
		trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Disabled ^3%i^5 waypoints with duplicate links to a neighbor.\n", num_dupe_nodes);
		trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Disabled ^3%i^5 waypoints in over-waypointed areas.\n", num_skiped_nodes);
		trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Disabled ^3%i^5 waypoints at your marked locations (removal spots).\n", num_this_location_nodes);
		trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Disabled ^3%i^5 waypoints at your given bad heights (bad height spots).\n", num_marked_height_nodes);

		total_removed = num_skiped_nodes + num_dupe_nodes + num_disabled_nodes + aw_num_bad_surfaces + num_nolink_nodes + num_tree_nodes + num_noroute_nodes + num_allsolid_nodes + num_this_location_nodes + num_marked_height_nodes;

		trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Disabled ^3%i^5 total waypoints in this run.\n", total_removed);

		if (total_removed <= 100 && multipass && !relink_only )
		{
			trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Multipass waypoint cleaner completed in %i passes. Nothing left to remove.\n", num_passes_completed);
			AIMod_AutoWaypoint_Free_Memory();
			AIMod_AutoWaypoint_Optimize_Free_Memory();

			// Restore the original multiplier...
			waypoint_distance_multiplier = original_wp_scatter_multiplier;
			DO_EXTRA_REACH = qfalse;
			return;
		}
		else if (total_removed == 0 && !relink_only )
		{// No point relinking and saving...
//			free(areas);
			AIMod_AutoWaypoint_Free_Memory();
			AIMod_AutoWaypoint_Optimize_Free_Memory();
			trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Waypoint cleaner completed. Nothing left to remove.\n");

			// Restore the original multiplier...
			waypoint_distance_multiplier = original_wp_scatter_multiplier;
			DO_EXTRA_REACH = qfalse;
			return;
		}

		if (multipass)
		{
			trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Creating new waypoints list (pass# ^7%i^5)... Please wait...\n", num_passes_completed) );
			strcpy( task_string3, va("^5Creating new waypoints list (pass# ^7%i^5)... Please wait...", num_passes_completed) );
		}
		else
		{
			trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Creating new waypoints list... Please wait...\n") );
			strcpy( task_string3, va("^5Creating new waypoints list... Please wait...") );
		}
		trap->UpdateScreen();

		total_calculations = number_of_nodes;
		calculations_complete = 0;

		aw_percent_complete = 0.0f;
		aw_stage_start_time = clock();

		for (i = 0; i < number_of_nodes; i++)
		{
			int				num_nodes_added = 0;
			short int		objNum[3] = { 0, 0, 0 };

			if (nodes[i].objEntity != 1 && nodes[i].objectNum[0] != 1)
			{
				num_nodes_added++;

				calculations_complete++;

				// Draw a nice little progress bar ;)
				aw_percent_complete = (float)((float)((float)(calculations_complete)/(float)(total_calculations))*100.0f);

				if (num_nodes_added > 100)
				{
					strcpy( last_node_added_string, va("^5Adding waypoint ^3%i ^5at ^7%f %f %f^5.", optimized_number_of_nodes, nodes[i].origin[0], nodes[i].origin[1], nodes[i].origin[2]) );
					trap->UpdateScreen();
					num_nodes_added = 0;
				}

				Optimize_AddNode( nodes[i].origin, 0, objNum, 0 );	//add the node
				nodes[i].objEntity = 1;
			}
		}

		calculations_complete = 0;
		aw_percent_complete = 0.0f;
		trap->UpdateScreen();

//		free(areas);

		trap->Print( va( "^4*** ^3AUTO-WAYPOINTER^4: ^5Waypoint list reduced from ^3%i^5 waypoints to ^3%i^5 waypoints.\n", number_of_nodes, optimized_number_of_nodes) );

		number_of_nodes = 0;

		// Copy over the new list!
		memcpy(nodes, optimized_nodes, (sizeof(node_t)*MAX_NODES)+1);
		number_of_nodes = optimized_number_of_nodes;
		optimized_number_of_nodes = 0;

		aw_num_nodes = number_of_nodes;

		// Save the new list...
		AIMOD_NODES_SaveNodes_Autowaypointed();

		aw_percent_complete = 0.0f;
		trap->UpdateScreen();

		trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Multipass waypoint cleaner pass #%i completed. Beginning next pass.\n", num_passes_completed);

		initial_pass = qfalse;

		if (!multipass)
		{// No point relinking and saving...
			trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Waypoint cleaner completed.\n");
			break;
		}

		if (!extra && number_of_nodes > 10000 && total_removed < 1000)
		{// Too many, we need an "extra" pass...
			extra = qtrue;
			trap->Print("^4*** ^3AUTO-WAYPOINTER^4: ^5Not enough removed in this pass, forcing next pass to be an \"extra\" pass.\n");
		}
		else
		{
			extra = qfalse;
		}

		calculations_complete = 0;
		node_clean_ticker = 0;
		num_nolink_nodes = 0;
		num_tree_nodes = 0;
		num_disabled_nodes = 0;
		num_allsolid_nodes = 0;
		num_dupe_nodes = 0;
		aw_num_bad_surfaces = 0;
		num_skiped_nodes = 0;
		num_this_location_nodes = 0;
		num_marked_height_nodes = 0;
		total_removed = 0;
	}

#ifdef __COVER_SPOTS__
	// Remake cover spots...
	if (start_wp_total != number_of_nodes)
		AIMOD_Generate_Cover_Spots();
#endif //__COVER_SPOTS__

	aw_percent_complete = 0.0f;
	trap->UpdateScreen();

	AIMod_AutoWaypoint_Free_Memory();
	AIMod_AutoWaypoint_Optimize_Free_Memory();

	// Restore the original multiplier...
	waypoint_distance_multiplier = original_wp_scatter_multiplier;
	
	DO_EXTRA_REACH = qfalse;

	//trap->SendConsoleCommand( "!loadnodes\n" );
}

#define NUMBER_SIZE		8

void CG_Waypoint( int wp_num ) {
	refEntity_t		re;
	vec3_t			angles;// , vec, dir, up;
	int				i, numdigits, digits[10], temp_num;

	memset( &re, 0, sizeof( re ) );

	VectorCopy(nodes[wp_num].origin, re.origin);
	re.origin[2]+=16;

	re.reType = RT_SPRITE;
	re.radius = 16;

	re.shaderRGBA[0] = 0xff;
	re.shaderRGBA[1] = 0x11;
	re.shaderRGBA[2] = 0x11;

	re.radius = NUMBER_SIZE / 2;

	VectorClear(angles);
	AnglesToAxis( angles, re.axis );

	/*
	VectorSubtract(cg.refdef.vieworg, re.origin, dir);
	CrossProduct(dir, up, vec);
	VectorNormalize(vec);
	*/

	temp_num = wp_num;

	for (numdigits = 0; !(numdigits && !temp_num); numdigits++) {
		digits[numdigits] = temp_num % 10;
		temp_num = temp_num / 10;
	}

	for (i = 0; i < numdigits; i++) {
		//VectorMA(nodes[wp_num].origin, (float) (((float) numdigits / 2) - i) * NUMBER_SIZE, vec, re.origin);
		re.customShader = cgs.media.numberShaders[digits[numdigits-1-i]];
		AddRefEntityToScene( &re );
	}
}

qboolean LinkCanReachMe ( int wp_from, int wp_to )
{
	int i = 0;

	for (i = 0; i < nodes[wp_to].enodenum; i ++)
	{
		if (nodes[wp_to].links[i].targetNode == wp_from)
			return qtrue;
	}

	return qfalse;
}

void CG_AddWaypointLinkLine( int wp_from, int wp_to, int link_flags )
{
	refEntity_t		re;

	memset( &re, 0, sizeof( re ) );

	re.reType = RT_LINE;
	re.radius = 1;

#ifdef __COVER_SPOTS__
	if (nodes[wp_from].type & NODE_COVER)
	{// Cover spots show as yellow...
		re.shaderRGBA[0] = 0xff;
		re.shaderRGBA[1] = 0xff;
		re.shaderRGBA[2] = 0x00;
		re.shaderRGBA[3] = 0xff;
	}
	else
#endif //__COVER_SPOTS__
	if (link_flags & NODE_ROAD || nodes[wp_from].type == NODE_ROAD || nodes[wp_to].type == NODE_ROAD)
	{// Road waypoints show as green...
		re.shaderRGBA[0] = 0x00;
		re.shaderRGBA[1] = 0xff;
		re.shaderRGBA[2] = 0x00;
		re.shaderRGBA[3] = 0xff;
	}
	else if (link_flags & NODE_WATER || nodes[wp_from].type == NODE_WATER || nodes[wp_to].type == NODE_WATER)
	{// This is a water link... Display in yellow..
		re.shaderRGBA[0] = 0xff;
		re.shaderRGBA[1] = 0xff;
		re.shaderRGBA[2] = 0x00;
		re.shaderRGBA[3] = 0xff;
	}
	else if (link_flags & NODE_JUMP)
	{// This is a jump-to link... Display in blue..
		re.shaderRGBA[0] = 0x00;
		re.shaderRGBA[1] = 0x00;
		re.shaderRGBA[2] = 0xff;
		re.shaderRGBA[3] = 0xff;
	}
	else if (LinkCanReachMe( wp_from, wp_to ))
	{// Link is bi-directional.. Display in white..
		re.shaderRGBA[0] = re.shaderRGBA[1] = re.shaderRGBA[2] = re.shaderRGBA[3] = 0xff;
	}
	else
	{// Link is uni-directional.. Display in red..
		re.shaderRGBA[0] = 0xff;
		re.shaderRGBA[1] = 0x00;
		re.shaderRGBA[2] = 0x00;
		re.shaderRGBA[3] = 0xff;
	}

	re.customShader = cgs.media.whiteShader;

	VectorCopy(nodes[wp_from].origin, re.origin);
	re.origin[2]+=16;
	VectorCopy(nodes[wp_to].origin, re.oldorigin);
	re.oldorigin[2]+=16;

	AddRefEntityToScene( &re );
}

extern vmCvar_t warzone_waypoint_render;

qboolean CURRENTLY_RENDERRING = qfalse;

qboolean InFOV( vec3_t spot, vec3_t from, vec3_t fromAngles, int hFOV, int vFOV )
{
	vec3_t	deltaVector, angles, deltaAngles;

	VectorSubtract ( spot, from, deltaVector );
	vectoangles ( deltaVector, angles );

	deltaAngles[PITCH]	= AngleDelta ( fromAngles[PITCH], angles[PITCH] );
	deltaAngles[YAW]	= AngleDelta ( fromAngles[YAW], angles[YAW] );

	if ( fabs ( deltaAngles[PITCH] ) <= vFOV && fabs ( deltaAngles[YAW] ) <= hFOV )
	{
		return qtrue;
	}

	return qfalse;
}

void DrawWaypoints()
{
	int node = 0;

	if (warzone_waypoint_render.integer <= 0)
	{
		if (CURRENTLY_RENDERRING)
		{
			AIMod_AutoWaypoint_Free_Memory();
		}

		CURRENTLY_RENDERRING = qfalse;
		return;
	}

	if (!CURRENTLY_RENDERRING)
	{
		number_of_nodes = 0;

		AIMod_AutoWaypoint_Init_Memory();

		AIMOD_NODES_LoadNodes(); // Load node file on first check...
#ifdef __COVER_SPOTS__
		AIMOD_LoadCoverPoints();
#endif //__COVER_SPOTS__

		if (number_of_nodes <= 0)
		{
			AIMod_AutoWaypoint_Free_Memory();
			CURRENTLY_RENDERRING = qfalse;
			return;
		}

		CURRENTLY_RENDERRING = qtrue;
	}

	if (number_of_nodes <= 0) return; // If still no nodes, exit early...

	for (node = 0; node < number_of_nodes; node++)
	{
		// Draw anything closeish to us...
		int			len = 0;
		int			link = 0;
		vec3_t		delta;
		qboolean	wpInFOV = qtrue;

		if (!InFOV( nodes[node].origin, cg.refdef.vieworg, cg.refdef.viewangles, cg.refdef.fov_x + 20, cg.refdef.fov_y + 20 ))
			wpInFOV = qfalse;

		if (warzone_waypoint_render.integer == 1 && !wpInFOV)
			continue; // If in fast mode, don't check links...

		VectorSubtract( nodes[node].origin, cg.refdef.vieworg, delta );
		len = VectorLength( delta );

		if (warzone_waypoint_render.integer == 1)
		{
			if (len < 20) continue;
			if (len > 512) continue;
		}
		else
		{
			if (len > 512 * warzone_waypoint_render.integer) continue;
		}

		//if (VectorDistance(cg_entities[cg.clientNum].lerpOrigin, nodes[node].origin) > 2048) continue;

		//CG_Waypoint( node );

		for (link = 0; link < nodes[node].enodenum; link++)
		{
			if (!wpInFOV)
			{
				if (!InFOV(nodes[nodes[node].links[link].targetNode].origin, cg.refdef.vieworg, cg.refdef.viewangles, cg.refdef.fov_x + 20, cg.refdef.fov_y + 20))
					continue;
			}

			CG_AddWaypointLinkLine( node, nodes[node].links[link].targetNode, nodes[node].links[link].flags );
		}
	}
}

//float	BAD_HEIGHTS[1024];
//int		NUM_BAD_HEIGHTS = 0;

void AIMod_MarkBadHeight ( void )
{// Mark this height on the map as bad for waypoints...
	BAD_HEIGHTS[NUM_BAD_HEIGHTS] = cg.refdef.vieworg[2];
	NUM_BAD_HEIGHTS++;
	trap->Print("Height %f marked as bad for waypoints.\n", cg.refdef.vieworg[2]);
}

#ifdef __AUTOWAYPOINT2__
int AWP_CountIndices(const dsurface_t *surfaces, int numSurfaces)
{
	int count = 0;
	for (int i = 0; i < numSurfaces; i++, surfaces++)
	{
		if (surfaces->surfaceType != MST_PLANAR && surfaces->surfaceType != MST_TRIANGLE_SOUP)
		{
			continue;
		}

		count += surfaces->numIndexes;
	}

	return count;
}

void AWP_LoadTriangles(const int *indexes, int* tris, const dsurface_t *surfaces, int numSurfaces)
{
	int t = 0;
	int v = 0;
	for (int i = 0; i < numSurfaces; i++, surfaces++)
	{
		if (surfaces->surfaceType != MST_PLANAR && surfaces->surfaceType != MST_TRIANGLE_SOUP)
		{
			continue;
		}

		for (int j = surfaces->firstIndex, k = 0; k < surfaces->numIndexes; j++, k++)
		{
			tris[t++] = v + indexes[j];
		}

		v += surfaces->numVerts;
	}
}

void AWP_LoadVertices(const drawVert_t *vertices, float* verts, const dsurface_t *surfaces, const int numSurfaces)
{
	int v = 0;
	for (int i = 0; i < numSurfaces; i++, surfaces++)
	{
		// will handle patches later...
		if (surfaces->surfaceType != MST_PLANAR && surfaces->surfaceType != MST_TRIANGLE_SOUP)
		{
			continue;
		}

		for (int j = surfaces->firstVert, k = 0; k < surfaces->numVerts; j++, k++)
		{
			verts[v++] = vertices[j].xyz[0];
			verts[v++] = vertices[j].xyz[1];
			verts[v++] = vertices[j].xyz[2];
		}
	}
}

qboolean AWP_LoadMapGeometry(const char *buffer, vec3_t mapmins, vec3_t mapmaxs, float* &verts, int &numverts, int* &tris, int &numtris)
{
	const dheader_t *header = (const dheader_t *)buffer;
	if (header->ident != BSP_IDENT)
	{
		trap->Print(va("Expected ident '%d', found %d.\n", BSP_IDENT, header->ident));
		return qfalse;
	}

	if (header->version != BSP_VERSION)
	{
		trap->Print(va("Expected version '%d', found %d.\n", BSP_VERSION, header->version));
		return qfalse;
	}

	// Load indices
	const int *indexes = (const int *)(buffer + header->lumps[LUMP_DRAWINDEXES].fileofs);
	const dsurface_t *surfaces = (const dsurface_t *)(buffer + header->lumps[LUMP_SURFACES].fileofs);
	int numSurfaces = header->lumps[LUMP_SURFACES].filelen / sizeof(dsurface_t);

	numtris = AWP_CountIndices(surfaces, numSurfaces);
	tris = new int[numtris];
	numtris /= 3;

	AWP_LoadTriangles(indexes, tris, surfaces, numSurfaces);

	// Load vertices
	const drawVert_t *vertices = (const drawVert_t *)(buffer + header->lumps[LUMP_DRAWVERTS].fileofs);
	numverts = header->lumps[LUMP_DRAWVERTS].filelen / sizeof(drawVert_t);

	verts = new float[3 * numverts];
	AWP_LoadVertices(vertices, verts, surfaces, numSurfaces);

	// Get map bounds. First model is always the entire map
	const dmodel_t *models = (const dmodel_t *)(buffer + header->lumps[LUMP_MODELS].fileofs);
	mapmins[0] = models[0].maxs[0];
	mapmins[1] = models[0].mins[1];
	mapmins[2] = models[0].maxs[2];

	mapmaxs[0] = models[0].mins[0];
	mapmaxs[1] = models[0].maxs[1];
	mapmaxs[2] = models[0].mins[2];

	/*
	mapmins[0] = -models[0].maxs[0];
	mapmins[1] = models[0].mins[2];
	mapmins[2] = -models[0].maxs[1];

	mapmaxs[0] = -models[0].mins[0];
	mapmaxs[1] = models[0].maxs[2];
	mapmaxs[2] = -models[0].mins[1];
	*/

	return qtrue;
}

qboolean AWP_CheckOrg(vec3_t org, vec3_t mapMins, vec3_t mapMaxs)
{
	float floor = FloorHeightAt(org);

	// Set the point found on the floor as the test location...
	//VectorSet(org, new_org[0], new_org[1], floor);

	if (floor < mapMins[2])
	{// Can skip this one!
		return qfalse;
	}
	else if (floor > mapMaxs[2])
	{// Can skip this one!
		return qfalse;
	}

	if (!AIMod_AutoWaypoint_Check_PlayerWidth(org))
	{// Not wide enough for a player to fit!
		return qfalse;
	}

	return qtrue;
}

void AWP_CreateWaypoints(const char *mapname)
{
	fileHandle_t f = 0;
	int fileLength = trap->FS_Open(mapname, &f, FS_READ);
	if (fileLength == -1 || !f)
	{
		trap->Print(va("Unable to open '%s' to create the waypoints.\n", mapname));
		return;
	}

	char *buffer = new char[fileLength + 1];
	trap->FS_Read(buffer, fileLength, f);
	buffer[fileLength] = '\0';
	trap->FS_Close(f);

	vec3_t mapmins;
	vec3_t mapmaxs;
	float *verts = NULL;
	int numverts;
	int *tris = NULL;
	int numtris;
	short int	objNum[3] = { 0, 0, 0 };

	aw_stage_start_time = clock();

	AWP_UpdatePercentBar(1, "Loading Map Geometry...", "", "");

	if (!AWP_LoadMapGeometry(buffer, mapmins, mapmaxs, verts, numverts, tris, numtris))
	{
		trap->Print(va("Unable to load map geometry from '%s'.\n", mapname));
		AWP_UpdatePercentBar(0, "", "", "");
		return;
	}

	AWP_UpdatePercentBar(1, "Generating waypoints from map verts...", "", "");

	AIMod_AutoWaypoint_Init_Memory();

	float prev_completed = 0;
	float prev_completed2 = 0;

	for (int index = 0; index < numtris; index += 3)
	{
		float completed = ((float)index / (float)numverts) * 100.0;
		
		if (completed - prev_completed > 1)
		{
			if (number_of_nodes > 1)
			{
				node_t *wp = &nodes[number_of_nodes - 1];
				AWP_UpdatePercentBar2(completed, "Generating waypoints from map verts...", va("%i waypoints added.", number_of_nodes), va("Last waypoint was at %f %f %f.", wp->origin[0], wp->origin[1], wp->origin[2]));
				prev_completed = completed;
			}
			else
			{
				AWP_UpdatePercentBar2(completed, "Generating waypoints from map verts...", va("%i waypoints added.", number_of_nodes), "");
				prev_completed = completed;
			}
		}
		else if (completed - prev_completed2 > 0.14)
		{
			AWP_UpdatePercentBarOnly(completed);
			prev_completed2 = completed;
		}

		if (number_of_nodes + 4 > MAX_WPARRAY_SIZE)
		{
			AWP_UpdatePercentBar(0, "", "", "");
			trap->Print("ERROR: Created too many waypoints...\n");
			AIMod_AutoWaypoint_Free_Memory();
			return;
		}

		vec3_t vert1, vert2, vert3, center;
		memcpy(&vert1, (const void *)&verts[tris[index]], sizeof(float) * 3);
		memcpy(&vert2, (const void *)&verts[tris[index + 1]], sizeof(float) * 3);
		memcpy(&vert3, (const void *)&verts[tris[index + 2]], sizeof(float) * 3);

#if 0
		// Added -- Check slope...
		float e0[3], e1[3];
		for (int j = 0; j < 3; ++j)
		{
			e0[j] = vert2[j] - vert1[j];
			e1[j] = vert3[j] - vert1[j];
		}
		vec3_t n;
		n[0] = e0[1] * e1[2] - e0[2] * e1[1];
		n[1] = e0[2] * e1[0] - e0[0] * e1[2];
		n[2] = e0[0] * e1[1] - e0[1] * e1[0];
		float d = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
		if (d > 0)
		{
			d = 1.0f / d;
			n[0] *= d;
			n[1] *= d;
			n[2] *= d;
		}

		vec3_t slopeangles;
		vectoangles(n, slopeangles);

		float pitch = slopeangles[0];

		if (pitch > 180)
			pitch -= 360;

		if (pitch < -180)
			pitch += 360;

		pitch += 90.0f;

		if (pitch > MAX_SLOPE || pitch < -MAX_SLOPE)
			continue; // bad slope...
#endif

		center[0] = (vert1[0] + vert2[0] + vert3[0]) / 3.0;
		center[1] = (vert1[1] + vert2[1] + vert3[1]) / 3.0;
		center[2] = (vert1[2] + vert2[2] + vert3[2]) / 3.0;

		if (AWP_CheckOrg(center, mapmins, mapmaxs)) Load_AddNode(center, 0, objNum, 0);	//add the center node

		vec3_t dir;
		float dist = 0;

		VectorSubtract(center, vert1, dir);
		dist = VectorLength(dir);
		VectorMA(vert1, dist / 3.0, dir, vert1);

		VectorSubtract(center, vert2, dir);
		dist = VectorLength(dir);
		VectorMA(vert2, dist / 3.0, dir, vert2);

		VectorSubtract(center, vert3, dir);
		dist = VectorLength(dir);
		VectorMA(vert3, dist / 3.0, dir, vert3);

		if (AWP_CheckOrg(vert1, mapmins, mapmaxs)) Load_AddNode(vert1, 0, objNum, 0);	//add the edge nodes
		if (AWP_CheckOrg(vert2, mapmins, mapmaxs)) Load_AddNode(vert2, 0, objNum, 0);	//add the edge nodes
		if (AWP_CheckOrg(vert3, mapmins, mapmaxs)) Load_AddNode(vert3, 0, objNum, 0);	//add the edge nodes
	}

	AWP_UpdatePercentBar(0, "", "", "");

	if (number_of_nodes > 0)
	{
		waypoint_scatter_distance = 256.0;
		waypoint_distance_multiplier = 1.0;

		AIMOD_NODES_SaveNodes_Autowaypointed();
	}

	AIMod_AutoWaypoint_Free_Memory();
}

void AWP_AutoWaypoint2(void)
{
	char mapname[128] = { 0 };
	sprintf(mapname, "maps/%s.bsp", cgs.currentmapname);
	trap->Print("Creating waypoints...\n");
	AWP_CreateWaypoints(mapname);
	trap->Print("Finished!\n");
}
#else
void AWP_AutoWaypoint2(void)
{
	trap->Printf("Experimental AWP is disabled in this build...\n");
}
#endif //__AUTOWAYPOINT2__

#endif //__AUTOWAYPOINT__
