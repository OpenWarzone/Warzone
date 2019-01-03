//[VisualWeapons]
#include "cg_local.h"

extern int BG_SiegeGetPairedValue(char *buf, char *key, char *outbuf);
extern int BG_SiegeGetValueGroup(char *buf, char *group, char *outbuf);

stringID_table_t holsterTypeTable[] =
{
	ENUM2STRING(HLR_NONE),
	ENUM2STRING(HLR_SINGLESABER_1),	//first single saber
	ENUM2STRING(HLR_SINGLESABER_2),	//second single saber
	ENUM2STRING(HLR_STAFFSABER),		//staff saber
	ENUM2STRING(HLR_PISTOL_L),		//left hip blaster pistol
	ENUM2STRING(HLR_PISTOL_R),		//right hip blaster pistol
	ENUM2STRING(HLR_BLASTER_L),		//left hip blaster rifle
	ENUM2STRING(HLR_BLASTER_R),		//right hip blaster rifle
	ENUM2STRING(HLR_BRYARPISTOL_L),	//left hip bryer pistol
	ENUM2STRING(HLR_BRYARPISTOL_R),	//right hip bryer pistol
	ENUM2STRING(HLR_BOWCASTER),		//bowcaster
	ENUM2STRING(HLR_ROCKET_LAUNCHER),//rocket launcher
	ENUM2STRING(HLR_DEMP2),			//demp2
	ENUM2STRING(HLR_CONCUSSION),		//concussion
	ENUM2STRING(HLR_REPEATER),		//repeater
	ENUM2STRING(HLR_FLECHETTE),		//flechette
	ENUM2STRING(HLR_DISRUPTOR),		//disruptor
	ENUM2STRING(HLR_T21),			//t21
	ENUM2STRING(HLR_A280),
	ENUM2STRING(HLR_EE3),
	ENUM2STRING(HLR_DTL20A),
	ENUM2STRING(HLR_Z6_CANON),
	ENUM2STRING(HRL_CLONERIFLE),
	ENUM2STRING(HRL_DC15_EXT),
	ENUM2STRING(HLR_WOOKIE_BOWCASTER),
	ENUM2STRING(HLR_DC15),
	ENUM2STRING(HLR_WESTARM5),
	ENUM2STRING(HLR_CLONE_BLASTER_L),		
	ENUM2STRING(HLR_CLONE_BLASTER_R),
	ENUM2STRING(HRL_E60_ROCKET_LAUNCHER),
	ENUM2STRING(HRL_CW_ROCKET_LAUNCHER),
	ENUM2STRING(HLR_DC_15S_CLONE_PISTOL_L),		
	ENUM2STRING(HLR_DC_15S_CLONE_PISTOL_R),		
	ENUM2STRING(HLR_WESTER_PISTOL_L),		
	ENUM2STRING(HLR_WESTER_PISTOL_R),		
	ENUM2STRING(HLR_ELG_3A_L),		
	ENUM2STRING(HLR_ELG_3A_R),		
	ENUM2STRING(HLR_S5_PISTOL_L),		
	ENUM2STRING(HLR_S5_PISTOL_R),		
	ENUM2STRING(HLR_WOOKIES_PISTOL_L),		
	ENUM2STRING(HLR_WOOKIES_PISTOL_R),		
	ENUM2STRING(HLR_DC_17_CLONE_PISTOL_L),		
	ENUM2STRING(HLR_DC_17_CLONE_PISTOL_R),
	ENUM2STRING(HLR_TESTGUN_L),
	ENUM2STRING(HLR_TESTGUN_R),
	ENUM2STRING(HLR_SPOTING_BLASTER_L),
	ENUM2STRING(HLR_SPOTING_BLASTER_R),
	ENUM2STRING(HLR_ACP_PISTOL_L),
	ENUM2STRING(HLR_ACP_PISTOL_R),
	ENUM2STRING(HLR_FRAG_GRENADE),
	ENUM2STRING(HLR_FRAG_GRENADE_OLD),
	ENUM2STRING(HLR_THERMAL_GRENADE),
	ENUM2STRING(HLR_TRIP_MINE),
	ENUM2STRING(HLR_DET_PACK),
	ENUM2STRING(HLR_A200_ACP_BATTLERIFLE),
	ENUM2STRING(HLR_ACP_ARRAYGUN),
	ENUM2STRING(HLR_ACP_SNIPER_RIFLE),
	ENUM2STRING(HLR_ARC_CASTER_IMPERIAL_L),
	ENUM2STRING(HLR_ARC_CASTER_IMPERIAL_R),
	ENUM2STRING(HLR_BOWCASTER_CLASSIC),
	ENUM2STRING(HLR_WOOKIE_BOWCASTER_SCOPE),
	ENUM2STRING(HLR_BRYAR_CARBINE_L),
	ENUM2STRING(HLR_BRYAR_CARBINE_R),
	ENUM2STRING(HLR_BRYAR_RIFLE),
	ENUM2STRING(HLR_BRYAR_RIFLE_SCOPE),
	ENUM2STRING(HRL_PULSECANON),
	ENUM2STRING(HRL_PROTON_CARBINE_RIFLE),
	ENUM2STRING(HLR_DH_17_PISTOL_L),
	ENUM2STRING(HLR_DH_17_PISTOL_R),


	ENUM2STRING(MAX_HOLSTER),

	// All stringID tables need to end with this
	{
		NULL, 0
	}
};

stringID_table_t holsterBoneTable[] =
{
	ENUM2STRING(HOLSTER_NONE),
	ENUM2STRING(HOLSTER_UPPERBACK),	
	ENUM2STRING(HOLSTER_LOWERBACK),
	ENUM2STRING(HOLSTER_LEFTHIP),
	ENUM2STRING(HOLSTER_RIGHTHIP),

	// All stringID tables need to end with this
	{
		NULL, 0
	}
};

/*
holster_t defaultHolsterData[MAX_HOLSTER] =
{
	{//HLR_NONE
		HOLSTER_NONE,		//boneindex
		{0, 0, 0},			//posOffset
		{0, 0, 0},			//angOffset
	},
	{//HLR_SINGLESABER_1
		HOLSTER_RIGHTHIP,	//boneindex
		{2, -3, 7},			//posOffset
		{0, 0, 0},			//angOffset
	},
	{//HLR_SINGLESABER_2	
		HOLSTER_LEFTHIP,	//boneindex
		{-1.5, -2.8, 7},	//posOffset
		{0, 0, 0},			//angOffset
	},
	{//HLR_STAFFSABER
		HOLSTER_UPPERBACK,	//boneindex
		{-5.5, -2, 0},		//posOffset
		{120, 0, 0},		//angOffset
	},
	{//HLR_PISTOL_L
		HOLSTER_LOWERBACK,	//boneindex
		{-6, -2, 1},		//posOffset
		{-90, -10, 0},		//angOffset
	},
	{//HLR_PISTOL_R
		HOLSTER_LOWERBACK,	//boneindex
		{7, -4, -1},		//posOffset
		{-90, 10, 0},		//angOffset
	},
	{//HLR_BLASTER_L
		HOLSTER_LOWERBACK,	//boneindex
		{-7, -4, -1},		//posOffset
		{0, 0, 0},			//angOffset
	},
	{//HLR_BLASTER_R	
		HOLSTER_LOWERBACK,	//boneindex
		{7, -4, -1},		//posOffset
		{-90, 10, 0},		//angOffset
	},
	{//HLR_BRYERPISTOL_L	
		HOLSTER_LOWERBACK,	//boneindex
		{-6, -2, 1},		//posOffset
		{-90, -10, 0},		//angOffset
	},
	{//HLR_BRYERPISTOL_R	
		HOLSTER_LOWERBACK,	//boneindex
		{6.5, -4, 2},		//posOffset
		{-90, 10, 0},		//angOffset
	},
	{//HLR_BOWCASTER	
		HOLSTER_UPPERBACK,	//boneindex
		{-9.5, -2, 2},		//posOffset
		{-40, 0, -90},		//angOffset
	},
	{//HLR_ROCKET_LAUNCHER
		HOLSTER_UPPERBACK,	//boneindex
		{-6, -3, 0},		//posOffset
		{-40, 0, 0},		//angOffset
	},
	{//HLR_DEMP2
		HOLSTER_UPPERBACK,	//boneindex
		{-6, -3, 0},		//posOffset
		{-40, 0, 0},		//angOffset
	},
	{//HLR_CONCUSSION
		HOLSTER_UPPERBACK,	//boneindex
		{-6, -3, 0},		//posOffset
		{-40, 0, 0},		//angOffset
	},
	{//HLR_REPEATER
		HOLSTER_UPPERBACK,	//boneindex
		{-5, -3, 0},		//posOffset
		{-40, 0, 0},		//angOffset
	},
	{//HLR_FLECHETTE
		HOLSTER_UPPERBACK,	//boneindex
		{-6, -3, 0},		//posOffset
		{-40, 0, 0},		//angOffset
	},
	{//HLR_DISRUPTOR
		HOLSTER_UPPERBACK,	//boneindex
		{-5, -7, 3},		//posOffset
		{-40, 0, 0},		//angOffset
	}
	{//HLR_T21
		HOLSTER_UPPERBACK,	//boneindex
		{-5, -7, 3},		//posOffset
		{-40, 0, 0},		//angOffset
	}
	{//HLR_A280
		HOLSTER_UPPERBACK,	//boneindex
		{-5, -7, 3},		//posOffset
		{-40, 0, 0},		//angOffset
	}
	{//HLR_EE3
		HOLSTER_UPPERBACK,	//boneindex
		{-5, -7, 3},		//posOffset
		{-40, 0, 0},		//angOffset
	}
	{//HLR_DTL20A
		HOLSTER_UPPERBACK,	//boneindex
		{-5, -7, 3},		//posOffset
		{-40, 0, 0},		//angOffset
	}
	{//HLR_Z6_CANON
		HOLSTER_UPPERBACK,	//boneindex
		{-5, -7, 3},		//posOffset
		{-40, 0, 0},		//angOffset
	}
	{//HRL_CLONERIFLE
		HOLSTER_UPPERBACK,	//boneindex
		{-5, -7, 3},		//posOffset
		{-40, 0, 0},		//angOffset
	}
	{//HRL_DC15_EXT
		HOLSTER_UPPERBACK,	//boneindex
		{-5, -7, 3},		//posOffset
		{-40, 0, 0},		//angOffset
	},
};
*/


void InitHolsterData (clientInfo_t *ci)
{//initialize holster data with the premade defaults.
	int i;
	for( i = 0; i < MAX_HOLSTER; i++ )
	{
		ci->holsterData[i].boneIndex = HOLSTER_NONE;
		VectorCopy(vec3_origin, ci->holsterData[i].posOffset);
		VectorCopy(vec3_origin, ci->holsterData[i].angOffset);
	}
}

extern char *BG_GetNextValueGroup(char *inbuf, char *outbuf);
void CG_LoadHolsterData (clientInfo_t *ci)
{//adjusts the manual holster positional data based on the holster.cfg file associated with the model or simply
	//use the default values


	int				i;
	fileHandle_t	f;
	int				fLen = 0;
	char			*fileBuffer;//[MAX_HOLSTER_INFO_SIZE];
	char			holsterTypeValue[MAX_QPATH];
	char			holsterTypeGroup[MAX_HOLSTER_INFO_SIZE];
	char			*s;
	vec3_t			vectorData;

	InitHolsterData(ci);
	
	//
	// UQ1 - One of these holser.cfg's has a missing } causing a very very annoying crash that deletes all config settings... 
	// I have added a workaround for now... By making BG_GetNextValueGroup print an error instead of crashing...
	//

	if ( !ci->skinName || !Q_stricmp( "default", ci->skinName ) )
	{//try default holster.cfg first
		fLen = trap->FS_Open(va("models/players/%s/holster.cfg", ci->modelName), &f, FS_READ);

		if( !f )
		{//no file, use kyle's then.
			fLen = trap->FS_Open("models/players/holster.cfg", &f, FS_READ);
		}
	}
	else
	{//use the holster.cfg associated with this skin
		fLen = trap->FS_Open(va("models/players/%s/holster_%s.cfg", ci->modelName, ci->skinName), &f, FS_READ);
		if ( !f )
		{//fall back to default holster.cfg
			fLen = trap->FS_Open(va("models/players/%s/holster.cfg", ci->modelName), &f, FS_READ);
		}

		if( !f )
		{//still no dice, use kyle's then.
			fLen = trap->FS_Open("models/players/holster.cfg", &f, FS_READ);
		}
	}

	if ( !f || !fLen )
	{//couldn't open file or it was empty, just use the defaults
		return;
	}

	if( fLen >= MAX_HOLSTER_INFO_SIZE )
	{
		trap->Print("Error: holster.cfg for %s is over the holster.cfg filesize limit.\n", ci->modelName);
		trap->FS_Close(f);
		return;
	}

	fileBuffer = (char *)malloc(fLen+1);

	trap->FS_Read(fileBuffer, fLen, f);

	trap->FS_Close(f);

	s = fileBuffer;

	//parse file
	while ((s = BG_GetNextValueGroup(s, holsterTypeGroup)) != NULL)
	{
		if (!BG_SiegeGetPairedValue(holsterTypeGroup, "holsterType", holsterTypeValue))
		{//couldn't find holster type in group
			trap->Print("Error:  The holster.cfg for %s appears to be missing a holsterType in one of its define groups.\n",
				ci->modelName);
			continue;
		}

		i = GetIDForString(holsterTypeTable, holsterTypeValue);

		if (i == -1)
		{//bad holster type
			trap->Print("Error:  The holster.cfg for %s has a bad holsterType in one of the define groups.\n",
				ci->modelName);
			continue;
		}

		if (BG_SiegeGetPairedValue(holsterTypeGroup, "boneIndex", holsterTypeValue))
		{//have bone index data for this holster type, use it
			if (!Q_stricmp(holsterTypeValue, "disabled"))
			{//disable the rendering of this holster type on this model
				ci->holsterData[i].boneIndex = HOLSTER_NONE;
			}
			else
			{
				ci->holsterData[i].boneIndex = GetIDForString(holsterBoneTable, holsterTypeValue);
			}
		}

		if (BG_SiegeGetPairedValue(holsterTypeGroup, "posOffset", holsterTypeValue))
		{//parsing positional offset data
			sscanf(holsterTypeValue, "%f, %f, %f", &vectorData[0], &vectorData[1], &vectorData[2]);
			VectorCopy(vectorData, ci->holsterData[i].posOffset);

			//&ci->holsterData[i].posOffset[0], &ci->holsterData[i].posOffset[1], 
			//&ci->holsterData[i].posOffset[2]);
		}

		if (BG_SiegeGetPairedValue(holsterTypeGroup, "angOffset", holsterTypeValue))
		{//parsing angular offset
			sscanf(holsterTypeValue, "%f, %f, %f", &vectorData[0], &vectorData[1], &vectorData[2]);
			VectorCopy(vectorData, ci->holsterData[i].angOffset);
		}
	}
#ifdef _DEBUG
	trap->Print("Holstered Weapon Data Loaded for %s.\n", ci->modelName);
#endif

	free(fileBuffer);
}

//[/VisualWeapons]
