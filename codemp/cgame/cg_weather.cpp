#include "cg_local.h"

qboolean CG_AtmosphericKludge(); // below

#define __FAST_ATMOSPHERICS__


/*
**  	UniqueOne's Brand new (ultra-simple, but looks nice) weather FX using JKA's efx system...
**
**  	Current supported effects are rain, heavy rain, storm, snow and heavy snow.
*/

// Weather types...
typedef enum {
	WEATHER_NONE,
	WEATHER_RAIN,
	WEATHER_HEAVY_RAIN,
	WEATHER_RAIN_STORM,
	WEATHER_SNOW,
	WEATHER_HEAVY_SNOW,
	WEATHER_SNOW_STORM,
	WEATHER_FOREST,
	WEATHER_FOREST_ISLAND,
	WEATHER_TROPICAL_ISLAND,
	WEATHER_DAGOBAH,
	WEATHER_DESERT,
	WEATHER_CITY,
	WEATHER_CANTINA,
	WEATHER_INDUSTRIAL,
	WEATHER_SPACEPORT,
	WEATHER_IMPERIAL_BASE,
	WEATHER_SITH_TOMB,
	WEATHER_JEDI_TEMPLE,
};

// Max MAP height...
#define		MAX_ATMOSPHERIC_HEIGHT  	  	524288  	// maximum world height (FIXME: since 1.27 this should be 65536)

// Runtime values...
qboolean	ATMOSPHERICS_INITIIALIZED = qfalse;
qboolean	ATMOSPHERIC_SOUND_ONLY = qfalse;

float		ATMOSPHERIC_MAX_MAP_HEIGHT = -MAX_ATMOSPHERIC_HEIGHT;

qboolean	JKA_WEATHER_ENABLED = qfalse;
qboolean	WZ_WEATHER_ENABLED = qfalse;
qboolean	WZ_WEATHER_SOUND_ONLY = qfalse;

char		WZ_WEATHER_CUSTOM_SOUND[512] = { 0 };
char		WZ_WEATHER_CUSTOM_SOUND2[512] = { 0 };

int			ATMOSPHERIC_WEATHER_TYPE = WEATHER_NONE;
int			ATMOSPHERIC_NEXT_LIGHTNING_FLASH_TIME = 0;
int			ATMOSPHERIC_NEXT_SOUND_TIME = 0;


qhandle_t WEATHER_RAIN_EFX = NULL;
qhandle_t WEATHER_HEAVY_RAIN_EFX = NULL;
qhandle_t WEATHER_RAIN_STORM_EFX = NULL;
qhandle_t WEATHER_SNOW_EFX = NULL;
qhandle_t WEATHER_HEAVY_SNOW_EFX = NULL;
qhandle_t WEATHER_SNOW_STORM_EFX = NULL;
qhandle_t WEATHER_VOLUMETRIC_FOG_EFX = NULL;

qhandle_t WEATHER_RAIN_SOUND = NULL;
qhandle_t WEATHER_HEAVY_RAIN_SOUND = NULL;
qhandle_t WEATHER_RAIN_STORM_SOUND = NULL;
qhandle_t WEATHER_SNOW_SOUND = NULL;
qhandle_t WEATHER_SNOW_STORM_SOUND = NULL;
qhandle_t WEATHER_FOREST_SOUND = NULL;
//qhandle_t WEATHER_FOREST_NIGHT_SOUND = NULL;// TODO: Day/Night Integration...
qhandle_t WEATHER_FOREST_ISLAND_SOUND = NULL;
qhandle_t WEATHER_TROPICAL_ISLAND_SOUND = NULL;
qhandle_t WEATHER_DAGOBAH_SOUND = NULL;
qhandle_t WEATHER_DESERT_SOUND = NULL;
qhandle_t WEATHER_CITY_SOUND = NULL;
qhandle_t WEATHER_CANTINA_SOUND = NULL;
qhandle_t WEATHER_INDUSTRIAL_SOUND = NULL;
qhandle_t WEATHER_SPACEPORT_SOUND = NULL;
qhandle_t WEATHER_IMPERIAL_BASE_SOUND = NULL;
qhandle_t WEATHER_SITH_TOMB_SOUND = NULL;
qhandle_t WEATHER_JEDI_TEMPLE_SOUND = NULL;
qhandle_t WEATHER_LIGHTNING_SOUNDS[12] = { NULL };
qhandle_t WEATHER_CUSTOM_SOUND = NULL;
qhandle_t WEATHER_CUSTOM_SOUND2 = NULL;

qhandle_t	lightning1 = -1;
qhandle_t	lightning2 = -1;
qhandle_t	lightning3 = -1;
qhandle_t	lightningExplode = -1;


float CG_GetSkyHeight ( trace_t *tr )
{
	int x, y;

	if (ATMOSPHERIC_MAX_MAP_HEIGHT <= -MAX_ATMOSPHERIC_HEIGHT)
	{// Find map's highest point... Once...
		// Try to load pre-created info from our map's .mapInfo file...
		ATMOSPHERIC_MAX_MAP_HEIGHT = atof(IniRead(va("maps/%s.mapInfo", cgs.currentmapname), "MAPINFO", "SKY_HEIGHT", "-999999.0"));

		if (ATMOSPHERIC_MAX_MAP_HEIGHT <= -999999.0)
		{
			for (x = -MAX_ATMOSPHERIC_HEIGHT; x < MAX_ATMOSPHERIC_HEIGHT; x += 256)
			{
				for (y = -MAX_ATMOSPHERIC_HEIGHT; y < MAX_ATMOSPHERIC_HEIGHT; y += 256)
				{
					vec3_t testpoint, testend;
					testpoint[0] = testend[0] = x;
					testpoint[1] = testend[1] = y;
					testpoint[2] = MAX_ATMOSPHERIC_HEIGHT;
					testend[2] = -MAX_ATMOSPHERIC_HEIGHT;

					CG_Trace( tr, testpoint, NULL, NULL, testend, ENTITYNUM_NONE, MASK_ALL );

					if (tr->endpos[2] > ATMOSPHERIC_MAX_MAP_HEIGHT) 
						ATMOSPHERIC_MAX_MAP_HEIGHT = tr->endpos[2];
				}
			}

			// Write newly created info to our map's .mapInfo file for future map loads...
			IniWrite(va("maps/%s.mapInfo", cgs.currentmapname), "MAPINFO", "SKY_HEIGHT", va("%f", ATMOSPHERIC_MAX_MAP_HEIGHT));
		}

		//trap->Print("^3Atmospheric height is at %f.\n", ATMOSPHERIC_MAX_MAP_HEIGHT);
	}

	return ATMOSPHERIC_MAX_MAP_HEIGHT - 128;
}

qboolean CG_AtmosphericBadSpotForParticle ( vec3_t spot )
{
	trace_t tr;
	vec3_t testpoint;
	VectorSet(testpoint, spot[0], spot[1], -MAX_ATMOSPHERIC_HEIGHT);
	CG_Trace( &tr, testpoint, NULL, NULL, spot, ENTITYNUM_NONE, MASK_ALL );
	
	if (tr.fraction == 1 || tr.endpos[2] <= -MAX_ATMOSPHERIC_HEIGHT)
		return qtrue;

	return qfalse;
}

qboolean CG_AtmosphericSkyVisibleFrom ( vec3_t spot, float skyPoint )
{
	trace_t tr;
	vec3_t testpoint;
	VectorSet(testpoint, spot[0], spot[1], MAX_ATMOSPHERIC_HEIGHT/*skyPoint*/);
	CG_Trace( &tr, spot, NULL, NULL, testpoint, cg.clientNum, MASK_SOLID );//MASK_ALL );
	
	if ((tr.fraction == 1 || tr.endpos[2] >= skyPoint-768) && tr.endpos[2] <= ATMOSPHERIC_MAX_MAP_HEIGHT/*MAX_ATMOSPHERIC_HEIGHT*/)
		return qtrue;

	//trap->Print("spot: %f. skyPoint: %f. tr: %f.\n", spot[2], skyPoint, tr.endpos[2]);

	return qfalse;
}

void CG_LightningFlash( vec3_t spot )
{
	// Attempt to 'spot' a lightning flash somewhere below the sky.

	int			choice;
	vec3_t		down = { 0, 0, -1 };
	float		scale = 1.0;
	//vec3_t		lightSpot;

	if (CG_AtmosphericBadSpotForParticle( spot ))
	{// Not here... Outside map...
		//trap->Print("Flash failed at %f %f %f.\n", spot[0], spot[1], spot[2]);
		return;
	}

	if (lightning1 == -1)
	{// Register the effect the first time.
		lightning1 = trap->FX_RegisterEffect("effects/atmospherics/lightning_flash1.efx");
		lightning2 = trap->FX_RegisterEffect("effects/atmospherics/lightning_flash2.efx");
		lightning3 = trap->FX_RegisterEffect("effects/atmospherics/lightning_flash3.efx");
		lightningExplode = trap->FX_RegisterEffect("effects/env/lightning_explode.efx");
		WEATHER_LIGHTNING_SOUNDS[0] = trap->S_RegisterSound("sound/atmospherics/thunder00.mp3");
		WEATHER_LIGHTNING_SOUNDS[1] = trap->S_RegisterSound("sound/atmospherics/thunder01.mp3");
		WEATHER_LIGHTNING_SOUNDS[2] = trap->S_RegisterSound("sound/atmospherics/thunder02.mp3");
		WEATHER_LIGHTNING_SOUNDS[3] = trap->S_RegisterSound("sound/atmospherics/thunder03.mp3");
		WEATHER_LIGHTNING_SOUNDS[4] = trap->S_RegisterSound("sound/atmospherics/thunder04.mp3");
		WEATHER_LIGHTNING_SOUNDS[5] = trap->S_RegisterSound("sound/atmospherics/thunder05.mp3");
		WEATHER_LIGHTNING_SOUNDS[6] = trap->S_RegisterSound("sound/atmospherics/thunder06.mp3");
		WEATHER_LIGHTNING_SOUNDS[7] = trap->S_RegisterSound("sound/atmospherics/thunder07.mp3");
		WEATHER_LIGHTNING_SOUNDS[8] = trap->S_RegisterSound("sound/atmospherics/thunder08.mp3");
		WEATHER_LIGHTNING_SOUNDS[9] = trap->S_RegisterSound("sound/atmospherics/thunder09.mp3");
		WEATHER_LIGHTNING_SOUNDS[10] = trap->S_RegisterSound("sound/atmospherics/thunder10.mp3");
		WEATHER_LIGHTNING_SOUNDS[11] = trap->S_RegisterSound("sound/atmospherics/thunder11.mp3");
	}

	scale = (spot[2] - cg.refdef.vieworg[2]) / 256.0;

	//VectorSet(down, cg.refdef.vieworg[0], cg.refdef.vieworg[1], ATMOSPHERIC_MAX_MAP_HEIGHT);
	//VectorSubtract( cg.refdef.vieworg, down, down );

	choice = rand()%3;

	if (choice == 1)
		trap->FX_PlayEffectID(lightning1, spot, down, 0, scale, qfalse);
	if (choice == 2)
		trap->FX_PlayEffectID(lightning2, spot, down, 0, scale, qfalse);
	else
		trap->FX_PlayEffectID(lightning3, spot, down, 0, scale, qfalse);

	//trap->FX_PlayEffectID(lightningExplode, spot, down, 0, 0, qfalse);

	//trap->FX_PlayEffectID(trap->FX_RegisterEffect("effects/atmospherics/huge_lightning.efx"), spot, down, 0, 0, qfalse);
	//trap->FX_PlayEffectID(trap->FX_RegisterEffect("effects/atmospherics/lightning_storm_huge.efx"), spot, down, 0, 0, qfalse);

	//trap->Print("Flash OK at %f %f %f.\n", spot[0], spot[1], spot[2]);

	trap->S_StartLocalSound(WEATHER_LIGHTNING_SOUNDS[irand(0,11)], CHAN_AUTO);
}

qboolean CG_CheckRangedFog( void )
{
	if (ATMOSPHERIC_WEATHER_TYPE == WEATHER_SNOW_STORM)
	{
		trap->R_SetRangedFog(512.0);
		return qtrue;
	}

	return qfalse;
}

void CG_AddVolumetricFog( void )
{
	int i;

	for (i = 0; i < 16; i++)
	{
		vec3_t direction = { 0, 1, 0 };
		vec3_t spot = { ((rand()%512)+cg.refdef.vieworg[0])-256, ((rand()%512)+cg.refdef.vieworg[1])-256, ((rand()%512)+cg.refdef.vieworg[2])-256 };
		trap->FX_PlayEffectID(WEATHER_VOLUMETRIC_FOG_EFX, spot, direction, 0, 0, qfalse);
	}
}

void CG_AddAtmosphericEffects()
{
	int MAX_FRAME_PARTICLES = 32;
	int i, sizeX, sizeY, skyHeight;
	trace_t tr;

  	// Add atmospheric effects (e.g. rain, snow etc.) to view
	if (!ATMOSPHERICS_INITIIALIZED)
	{
		CG_AtmosphericKludge();

		switch (ATMOSPHERIC_WEATHER_TYPE)
		{
		case WEATHER_RAIN:
			WEATHER_RAIN_EFX = trap->FX_RegisterEffect("effects/atmospherics/atmospheric_rain.efx");
			WEATHER_RAIN_SOUND = trap->S_RegisterSound("sound/atmospherics/light_rain.mp3");
			break;
		case WEATHER_HEAVY_RAIN:
			WEATHER_HEAVY_RAIN_EFX = trap->FX_RegisterEffect("effects/atmospherics/atmospheric_heavyrain.efx");
			WEATHER_HEAVY_RAIN_SOUND = trap->S_RegisterSound("sound/atmospherics/heavy_rain.mp3");
			break;
		case WEATHER_RAIN_STORM:
			WEATHER_RAIN_STORM_EFX = trap->FX_RegisterEffect("effects/atmospherics/atmospheric_storm.efx");
			WEATHER_RAIN_STORM_SOUND = trap->S_RegisterSound("sound/atmospherics/thunder_storm.mp3");
			break;
		case WEATHER_SNOW:
			WEATHER_SNOW_EFX = trap->FX_RegisterEffect("effects/atmospherics/atmospheric_snow.efx");
			WEATHER_SNOW_SOUND = trap->S_RegisterSound("sound/atmospherics/snow.mp3");
			break;
		case WEATHER_HEAVY_SNOW:
			WEATHER_HEAVY_SNOW_EFX = trap->FX_RegisterEffect("effects/atmospherics/atmospheric_heavysnow.efx");
			WEATHER_SNOW_SOUND = trap->S_RegisterSound("sound/atmospherics/snow.mp3");
			break;
		case WEATHER_SNOW_STORM:
			WEATHER_SNOW_STORM_EFX = trap->FX_RegisterEffect("effects/atmospherics/atmospheric_snowstorm.efx");
			WEATHER_VOLUMETRIC_FOG_EFX = trap->FX_RegisterEffect("effects/atmospherics/fog.efx");
			WEATHER_SNOW_STORM_SOUND = trap->S_RegisterSound("sound/atmospherics/snow_storm.mp3");
			break;
		case WEATHER_FOREST:
			WEATHER_FOREST_SOUND = trap->S_RegisterSound("sound/atmospherics/forest.mp3");
			//WEATHER_FOREST_NIGHT_SOUND = trap->S_RegisterSound("sound/atmospherics/forest_night.mp3"); // TODO: Day/Night Integration...
			break;
		case WEATHER_FOREST_ISLAND:
			WEATHER_FOREST_ISLAND_SOUND = trap->S_RegisterSound("sound/atmospherics/forest_island.mp3");
			break;
		case WEATHER_TROPICAL_ISLAND:
			WEATHER_TROPICAL_ISLAND_SOUND = trap->S_RegisterSound("sound/atmospherics/tropical_island.mp3");
			break;
		case WEATHER_DAGOBAH:
			WEATHER_DAGOBAH_SOUND = trap->S_RegisterSound("sound/atmospherics/dagobah.mp3");
			break;
		case WEATHER_DESERT:
			WEATHER_DESERT_SOUND = trap->S_RegisterSound("sound/atmospherics/desert.mp3");
			break;
		case WEATHER_CITY:
			WEATHER_CITY_SOUND = trap->S_RegisterSound("sound/atmospherics/city.mp3");
			break;
		case WEATHER_CANTINA:
			WEATHER_CANTINA_SOUND = trap->S_RegisterSound("sound/atmospherics/cantina.mp3");
			break;
		case WEATHER_INDUSTRIAL:
			WEATHER_INDUSTRIAL_SOUND = trap->S_RegisterSound("sound/atmospherics/industrial.mp3");
			break;
		case WEATHER_SPACEPORT:
			WEATHER_SPACEPORT_SOUND = trap->S_RegisterSound("sound/atmospherics/spaceport.mp3");
			break;
		case WEATHER_IMPERIAL_BASE:
			WEATHER_IMPERIAL_BASE_SOUND = trap->S_RegisterSound("sound/atmospherics/imp_base.mp3");
			break;
		case WEATHER_SITH_TOMB:
			WEATHER_SITH_TOMB_SOUND = trap->S_RegisterSound("sound/atmospherics/sith_tomb.mp3");
			break;
		case WEATHER_JEDI_TEMPLE:
			WEATHER_JEDI_TEMPLE_SOUND = trap->S_RegisterSound("sound/atmospherics/jedi_temple.mp3");
			break;
		default:
			break;
		}

		if (WZ_WEATHER_CUSTOM_SOUND[0] != 0)
		{
			WEATHER_CUSTOM_SOUND = trap->S_RegisterSound(WZ_WEATHER_CUSTOM_SOUND);
		}

		if (WZ_WEATHER_CUSTOM_SOUND2[0] != 0)
		{
			WEATHER_CUSTOM_SOUND2 = trap->S_RegisterSound(WZ_WEATHER_CUSTOM_SOUND2);
		}
		
		ATMOSPHERICS_INITIIALIZED = qtrue;
	}

	if (ATMOSPHERIC_WEATHER_TYPE == WEATHER_NONE)
	{
		return;
	}

	skyHeight = CG_GetSkyHeight(&tr)-128;

	sizeX = 2048;
	sizeY = 2048;

	if (ATMOSPHERIC_WEATHER_TYPE == WEATHER_RAIN_STORM)
	{// Some lightning explosions randomly?
		if (ATMOSPHERIC_NEXT_LIGHTNING_FLASH_TIME <= cg.time)
		{// ready for our next lightning flash...
			vec3_t spot = { ((rand()%8192)+cg.refdef.vieworg[0])-4096, ((rand()%8192)+cg.refdef.vieworg[1])-4096, ATMOSPHERIC_MAX_MAP_HEIGHT-256 };

			if (rand()%20 < 1) // Occasionally we want it to strike twice quickly.
				ATMOSPHERIC_NEXT_LIGHTNING_FLASH_TIME = cg.time + rand()%300;
			else
				ATMOSPHERIC_NEXT_LIGHTNING_FLASH_TIME = cg.time + 10000 + rand()%15000;
		
			CG_LightningFlash(spot);
		}
	}

	switch (ATMOSPHERIC_WEATHER_TYPE)
	{
	case WEATHER_RAIN:
		MAX_FRAME_PARTICLES = 16;
		if (WEATHER_CUSTOM_SOUND) trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND);
		break;
	case WEATHER_HEAVY_RAIN:
		MAX_FRAME_PARTICLES = 24;
		if (WEATHER_CUSTOM_SOUND) trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND);
		break;
	case WEATHER_RAIN_STORM:
		if (WEATHER_CUSTOM_SOUND) trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND);
		MAX_FRAME_PARTICLES = 24;
		break;
	case WEATHER_SNOW:
		if (WEATHER_CUSTOM_SOUND) trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND);
		MAX_FRAME_PARTICLES = 16;
		break;
	case WEATHER_HEAVY_SNOW:
		if (WEATHER_CUSTOM_SOUND) trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND);
		MAX_FRAME_PARTICLES = 24;
		break;
	case WEATHER_SNOW_STORM:
		if (WEATHER_CUSTOM_SOUND) trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND);
		MAX_FRAME_PARTICLES = 24;
		break;
	case WEATHER_FOREST:
		if (ATMOSPHERIC_NEXT_SOUND_TIME <= cg.time)
		{
			//trap->S_StartLocalSound(WEATHER_FOREST_SOUND, CHAN_AMBIENT);
			//trap->S_StartLocalSound(WEATHER_FOREST_NIGHT_SOUND, CHAN_AUTO); // TODO: Day/Night Integration...
			//ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 595000;
			trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND ? WEATHER_CUSTOM_SOUND : WEATHER_FOREST_SOUND);
			ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 4000;
		}
		return;
		break;
	case WEATHER_FOREST_ISLAND:
		if (ATMOSPHERIC_NEXT_SOUND_TIME <= cg.time)
		{
			//trap->S_StartLocalSound(WEATHER_FOREST_ISLAND_SOUND, CHAN_AMBIENT);
			//ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 595000;
			trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND ? WEATHER_CUSTOM_SOUND : WEATHER_FOREST_ISLAND_SOUND);
			ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 4000;
		}
		return;
		break;
	case WEATHER_TROPICAL_ISLAND:
		if (ATMOSPHERIC_NEXT_SOUND_TIME <= cg.time)
		{
			//trap->S_StartLocalSound(WEATHER_TROPICAL_ISLAND_SOUND, CHAN_AMBIENT);
			//ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 595000;
			trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND ? WEATHER_CUSTOM_SOUND : WEATHER_TROPICAL_ISLAND_SOUND);
			ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 4000;
		}
		return;
		break;
	case WEATHER_DAGOBAH:
		if (ATMOSPHERIC_NEXT_SOUND_TIME <= cg.time)
		{
			//trap->S_StartLocalSound(WEATHER_DAGOBAH_SOUND, CHAN_AMBIENT);
			//ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 595000;
			trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND ? WEATHER_CUSTOM_SOUND : WEATHER_DAGOBAH_SOUND);
			ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 4000;
		}
		return;
		break;
	case WEATHER_DESERT:
		if (ATMOSPHERIC_NEXT_SOUND_TIME <= cg.time)
		{
			//trap->S_StartLocalSound(WEATHER_DESERT_SOUND, CHAN_AMBIENT);
			//ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 595000;
			trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND ? WEATHER_CUSTOM_SOUND : WEATHER_DESERT_SOUND);
			ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 4000;
		}
		return;
		break;
	case WEATHER_CITY:
		if (ATMOSPHERIC_NEXT_SOUND_TIME <= cg.time)
		{
			//trap->S_StartLocalSound(WEATHER_CITY_SOUND, CHAN_AMBIENT);
			//ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 595000;
			trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND ? WEATHER_CUSTOM_SOUND : WEATHER_CITY_SOUND);
			ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 4000;
		}
		return;
		break;
	case WEATHER_CANTINA:
		if (ATMOSPHERIC_NEXT_SOUND_TIME <= cg.time)
		{
			//trap->S_StartLocalSound(WEATHER_CANTINA_SOUND, CHAN_AMBIENT);
			//ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 175000;
			trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND ? WEATHER_CUSTOM_SOUND : WEATHER_CANTINA_SOUND);
			ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 4000;
		}
		return;
		break;
	case WEATHER_INDUSTRIAL:
		if (ATMOSPHERIC_NEXT_SOUND_TIME <= cg.time)
		{
			//trap->S_StartLocalSound(WEATHER_INDUSTRIAL_SOUND, CHAN_AMBIENT);
			//ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 595000;
			trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND ? WEATHER_CUSTOM_SOUND : WEATHER_INDUSTRIAL_SOUND);
			ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 4000;
		}
		return;
		break;
	case WEATHER_SPACEPORT:
		if (ATMOSPHERIC_NEXT_SOUND_TIME <= cg.time)
		{
			//trap->S_StartLocalSound(WEATHER_SPACEPORT_SOUND, CHAN_AMBIENT);
			//ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 595000;
			trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND ? WEATHER_CUSTOM_SOUND : WEATHER_SPACEPORT_SOUND);
			ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 4000;
		}
		return;
		break;
	case WEATHER_IMPERIAL_BASE:
		if (ATMOSPHERIC_NEXT_SOUND_TIME <= cg.time)
		{
			//trap->S_StartLocalSound(WEATHER_IMPERIAL_BASE_SOUND, CHAN_AMBIENT);
			//ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 595000;
			trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND ? WEATHER_CUSTOM_SOUND : WEATHER_IMPERIAL_BASE_SOUND);
			ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 4000;
		}
		return;
		break;
	case WEATHER_SITH_TOMB:
		if (ATMOSPHERIC_NEXT_SOUND_TIME <= cg.time)
		{
			//trap->S_StartLocalSound(WEATHER_SITH_TOMB_SOUND, CHAN_AMBIENT);
			//ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 595000;
			trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND ? WEATHER_CUSTOM_SOUND : WEATHER_SITH_TOMB_SOUND);
			ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 4000;
		}
		return;
		break;
	case WEATHER_JEDI_TEMPLE:
		if (ATMOSPHERIC_NEXT_SOUND_TIME <= cg.time)
		{
			//trap->S_StartLocalSound(WEATHER_JEDI_TEMPLE_SOUND, CHAN_AMBIENT);
			//ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 595000;
			trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND ? WEATHER_CUSTOM_SOUND : WEATHER_JEDI_TEMPLE_SOUND);
			ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 4000;
		}
		return;
		break;
	default:
		if (WEATHER_CUSTOM_SOUND) trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND);
		MAX_FRAME_PARTICLES = 32;
		break;
	}

/*#ifdef __FAST_ATMOSPHERICS__
	// Fast version just checks visibility from player's position to sky, not each particle... Will be somewhat inaccurate, but fast...
	if (!CG_AtmosphericSkyVisibleFrom(cg.refdef.vieworg, skyHeight))
		return;
#endif //__FAST_ATMOSPHERICS__*/

	if (cg_atmosphericFrameParticleOverride.integer)
		MAX_FRAME_PARTICLES = cg_atmosphericFrameParticleOverride.integer;

	if (ATMOSPHERIC_SOUND_ONLY)
		MAX_FRAME_PARTICLES = 1; // We just want the sound to play...

	for (i = 0; i < MAX_FRAME_PARTICLES; i++)
	{
		vec3_t spot = { ((rand()%sizeX)+cg.refdef.vieworg[0])-1024, ((rand()%sizeY)+cg.refdef.vieworg[1])-1024, cg.refdef.vieworg[2]+256 };
		vec3_t down = { 0, 0, -1 };

		//if (CG_AtmosphericBadSpotForParticle( spot ))
		//	continue;

#ifndef __FAST_ATMOSPHERICS__
		if (!CG_AtmosphericSkyVisibleFrom ( spot, skyHeight ))
			continue;
#endif //__FAST_ATMOSPHERICS__

		switch (ATMOSPHERIC_WEATHER_TYPE)
		{
		case WEATHER_RAIN:
			if (!ATMOSPHERIC_SOUND_ONLY) trap->FX_PlayEffectID(WEATHER_RAIN_EFX, spot, down, 0, 0, qfalse);

			if (ATMOSPHERIC_NEXT_SOUND_TIME <= cg.time)
			{
				//trap->S_StartLocalSound(WEATHER_RAIN_SOUND, CHAN_AMBIENT);
				//ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 595000;
				trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_RAIN_SOUND);
				ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 4000;
			}
			break;
		case WEATHER_HEAVY_RAIN:
			if (!ATMOSPHERIC_SOUND_ONLY) trap->FX_PlayEffectID(WEATHER_HEAVY_RAIN_EFX, spot, down, 0, 0, qfalse);

			if (ATMOSPHERIC_NEXT_SOUND_TIME <= cg.time)
			{
				//trap->S_StartLocalSound(WEATHER_HEAVY_RAIN_SOUND, CHAN_AMBIENT);
				//ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 595000;
				trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_HEAVY_RAIN_SOUND);
				ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 4000;
			}
			break;
		case WEATHER_RAIN_STORM:
			if (!ATMOSPHERIC_SOUND_ONLY) trap->FX_PlayEffectID(WEATHER_RAIN_STORM_EFX, spot, down, 0, 0, qfalse);

			if (ATMOSPHERIC_NEXT_SOUND_TIME <= cg.time)
			{
				//trap->S_StartLocalSound(WEATHER_RAIN_STORM_SOUND, CHAN_AMBIENT);
				//ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 595000;
				trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_RAIN_STORM_SOUND);
				ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 4000;
			}
			break;
		case WEATHER_SNOW:
			if (!ATMOSPHERIC_SOUND_ONLY) trap->FX_PlayEffectID(WEATHER_SNOW_EFX, spot, down, 0, 0, qfalse);

			if (ATMOSPHERIC_NEXT_SOUND_TIME <= cg.time)
			{
				//trap->S_StartLocalSound(WEATHER_SNOW_SOUND, CHAN_AMBIENT);
				//ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 595000;
				trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_SNOW_SOUND);
				ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 4000;
			}
			break;
		case WEATHER_HEAVY_SNOW:
			if (!ATMOSPHERIC_SOUND_ONLY) trap->FX_PlayEffectID(WEATHER_HEAVY_SNOW_EFX, spot, down, 0, 0, qfalse);

			if (ATMOSPHERIC_NEXT_SOUND_TIME <= cg.time)
			{
				//trap->S_StartLocalSound(WEATHER_SNOW_SOUND, CHAN_AMBIENT);
				//ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 595000;
				trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_SNOW_SOUND);
				ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 4000;
			}
			break;
		case WEATHER_SNOW_STORM:
			if (!ATMOSPHERIC_SOUND_ONLY) trap->FX_PlayEffectID(WEATHER_SNOW_STORM_EFX, spot, down, 0, 0, qfalse);

			if (ATMOSPHERIC_NEXT_SOUND_TIME <= cg.time)
			{
				//trap->S_StartLocalSound(WEATHER_SNOW_STORM_SOUND, CHAN_AMBIENT);
				//ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 595000;
				trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_SNOW_STORM_SOUND);
				ATMOSPHERIC_NEXT_SOUND_TIME = cg.time + 4000;
			}
			break;
		default:
			break;
		}
	}

	if (WEATHER_CUSTOM_SOUND2) trap->S_AddLoopingSound(-1, NULL, vec3_origin, WEATHER_CUSTOM_SOUND2);

	if (!ATMOSPHERIC_SOUND_ONLY && ATMOSPHERIC_WEATHER_TYPE == WEATHER_SNOW_STORM)
	{// Would you like some volumetric fog with that? Oh yes please! - This is, however, very FPS costly...
		CG_AddVolumetricFog();
	}
}


/*
**  	G_AtmosphericKludge
*/

static qboolean kludgeChecked, kludgeResult;
qboolean CG_AtmosphericKludge()
{
  	// Activate effects for specified kludge maps that don't
  	// have it specified for them.

	char *atmosphericString = NULL;

  	if( kludgeChecked )
  	  	return( kludgeResult );

  	kludgeChecked = qtrue;
  	kludgeResult = qfalse;

	ATMOSPHERIC_SOUND_ONLY = qfalse;

	//
	// Check the ini file with the map...
	//

	char mapname[256] = { 0 };

	// because JKA uses mp/ dir, why??? so pointless...
	if (IniExists(va("maps/%s.mapInfo", cgs.currentmapname)))
		sprintf(mapname, "maps/%s.mapInfo", cgs.currentmapname);
	else if (IniExists(va("maps/mp/%s.mapInfo", cgs.currentmapname)))
		sprintf(mapname, "maps/mp/%s.mapInfo", cgs.currentmapname);
	else
		sprintf(mapname, "maps/%s.mapInfo", cgs.currentmapname);

	JKA_WEATHER_ENABLED = (atoi(IniRead(mapname, "ATMOSPHERICS", "JKA_WEATHER_ENABLED", "0")) > 0) ? qtrue : qfalse;
	WZ_WEATHER_ENABLED = (atoi(IniRead(mapname, "ATMOSPHERICS", "WZ_WEATHER_ENABLED", "0")) > 0) ? qtrue : qfalse;
	WZ_WEATHER_SOUND_ONLY = (atoi(IniRead(mapname, "ATMOSPHERICS", "WZ_WEATHER_SOUND_ONLY", "0")) > 0) ? qtrue : qfalse;
	memset(WZ_WEATHER_CUSTOM_SOUND, 0, sizeof(WZ_WEATHER_CUSTOM_SOUND));
	strcpy(WZ_WEATHER_CUSTOM_SOUND, IniRead(mapname, "ATMOSPHERICS", "WZ_WEATHER_CUSTOM_SOUND", ""));
	memset(WZ_WEATHER_CUSTOM_SOUND2, 0, sizeof(WZ_WEATHER_CUSTOM_SOUND2));
	strcpy(WZ_WEATHER_CUSTOM_SOUND2, IniRead(mapname, "ATMOSPHERICS", "WZ_WEATHER_CUSTOM_SOUND2", ""));

	if (JKA_WEATHER_ENABLED && WZ_WEATHER_ENABLED)
	{
		ATMOSPHERIC_SOUND_ONLY = qtrue;
	}
	else if (WZ_WEATHER_SOUND_ONLY)
	{
		ATMOSPHERIC_SOUND_ONLY = qtrue;
	}

	atmosphericString = (char*)IniRead(mapname, "ATMOSPHERICS", "WEATHER_TYPE", "");

	if (strlen(atmosphericString) <= 1)
	{// Check old config file for redundancy...
		atmosphericString = (char*)IniRead(va("maps/%s.atmospherics", cgs.currentmapname), "ATMOSPHERICS", "WEATHER_TYPE", "");

		if (strlen(atmosphericString) > 1)
		{// Redundancy...
			if (JKA_WEATHER_ENABLED)
				ATMOSPHERIC_SOUND_ONLY = qtrue;
			else
				WZ_WEATHER_ENABLED = qtrue;
		}
	}

	if (!WZ_WEATHER_ENABLED && !ATMOSPHERIC_SOUND_ONLY)
	{
		return(kludgeResult = qfalse);
	}


	if (!Q_stricmp(atmosphericString, "rain"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics set to ^7rain^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_RAIN;
  	  	return( kludgeResult = qtrue );
	}
	else if (!Q_stricmp(atmosphericString, "heavyrain"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics set to ^7heavyrain^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_HEAVY_RAIN;
  	  	return( kludgeResult = qtrue );
	}
	else if (!Q_stricmp(atmosphericString, "rainstorm") || !Q_stricmp(atmosphericString, "storm"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics set to ^7rainstorm^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_RAIN_STORM;
  	  	return( kludgeResult = qtrue );
	}
	else if (!Q_stricmp(atmosphericString, "snow"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics set to ^7snow^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_SNOW;
  	  	return( kludgeResult = qtrue );
	}
	else if (!Q_stricmp(atmosphericString, "heavysnow"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics set to ^7heavysnow^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_HEAVY_SNOW;
  	  	return( kludgeResult = qtrue );
	}
	else if (!Q_stricmp(atmosphericString, "snowstorm"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics set to ^7snowstorm^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_SNOW_STORM;
  	  	return( kludgeResult = qtrue );
	}
	else if (!Q_stricmp(atmosphericString, "forest_island"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics set to ^7forest_island^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_FOREST_ISLAND;
		return(kludgeResult = qtrue);
	}
	else if (!Q_stricmp(atmosphericString, "forest"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics set to ^7forest^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_FOREST;
		return(kludgeResult = qtrue);
	}
	else if (!Q_stricmp(atmosphericString, "tropical_island"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics set to ^7tropical_island^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_TROPICAL_ISLAND;
		return(kludgeResult = qtrue);
	}
	else if (!Q_stricmp(atmosphericString, "dagobah"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics set to ^7dagobah^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_DAGOBAH;
		return(kludgeResult = qtrue);
	}
	else if (!Q_stricmp(atmosphericString, "desert"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics set to ^7desert^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_DESERT;
		return(kludgeResult = qtrue);
	}
	else if (!Q_stricmp(atmosphericString, "city"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics set to ^7city^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_CITY;
		return(kludgeResult = qtrue);
	}
	else if (!Q_stricmp(atmosphericString, "cantina"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics set to ^7cantina^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_CANTINA;
		return(kludgeResult = qtrue);
	}
	else if (!Q_stricmp(atmosphericString, "industrial"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics set to ^7industrial^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_INDUSTRIAL;
		return(kludgeResult = qtrue);
	}
	else if (!Q_stricmp(atmosphericString, "spaceport"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics set to ^7spaceport^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_SPACEPORT;
		return(kludgeResult = qtrue);
	}
	else if (!Q_stricmp(atmosphericString, "imp_base"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics set to ^7imp_base^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_IMPERIAL_BASE;
		return(kludgeResult = qtrue);
	}
	else if (!Q_stricmp(atmosphericString, "sith_tomb"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics set to ^7sith_tomb^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_SITH_TOMB;
		return(kludgeResult = qtrue);
	}
	else if (!Q_stricmp(atmosphericString, "jedi_temple"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics set to ^7jedi_temple^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_JEDI_TEMPLE;
		return(kludgeResult = qtrue);
	}


	//
	// And some hard coded maps, if not overridden above...
	//

	if( !Q_stricmp( cgs.currentmapname, "maps/ffa_coruscant" ) )
  	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics ^7forced^5 to ^7rainstorm^5 for this map.\n");
  	  	ATMOSPHERIC_WEATHER_TYPE = WEATHER_RAIN_STORM;
  	  	return( kludgeResult = qtrue );
  	}

	if( !Q_stricmp( cgs.currentmapname, "maps/ffa_coruscant" ) )
  	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics ^7forced^5 to ^7rainstorm^5 for this map.\n");
  	  	ATMOSPHERIC_WEATHER_TYPE = WEATHER_RAIN_STORM;
  	  	return( kludgeResult = qtrue );
  	}

	if( !Q_stricmp( cgs.currentmapname, "maps/imphoth_a" ) )
  	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics ^7forced^5 to ^7snow^5 for this map.\n");
  	  	ATMOSPHERIC_WEATHER_TYPE = WEATHER_SNOW;
  	  	return( kludgeResult = qtrue );
  	}

	if( !Q_stricmp( cgs.currentmapname, "maps/imphoth_b" ) )
  	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics ^7forced^5 to ^7snow^5 for this map.\n");
  	  	ATMOSPHERIC_WEATHER_TYPE = WEATHER_SNOW;
  	  	return( kludgeResult = qtrue );
  	}

	if( !Q_stricmp( cgs.currentmapname, "maps/jedicouncilgc2" ) )
  	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics ^7forced^5 to ^7rainstorm^5 for this map.\n");
  	  	ATMOSPHERIC_WEATHER_TYPE = WEATHER_RAIN_STORM;
  	  	return( kludgeResult = qtrue );
  	}

	if( !Q_stricmp( cgs.currentmapname, "maps/jedicouncilgc" ) )
  	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics ^7forced^5 to ^7rainstorm^5 for this map.\n");
  	  	ATMOSPHERIC_WEATHER_TYPE = WEATHER_RAIN_STORM;
  	  	return( kludgeResult = qtrue );
  	}

	if( !Q_stricmp( cgs.currentmapname, "maps/bespinaflstyle3" ) )
  	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics ^7forced^5 to ^7rainstorm^5 for this map.\n");
  	  	ATMOSPHERIC_WEATHER_TYPE = WEATHER_RAIN_STORM;
  	  	return( kludgeResult = qtrue );
  	}

	if( !Q_stricmp( cgs.currentmapname, "maps/ffa_kujarforest" ) )
  	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics ^7forced^5 to ^7rain^5 for this map.\n");
  	  	ATMOSPHERIC_WEATHER_TYPE = WEATHER_RAIN;
  	  	return( kludgeResult = qtrue );
  	}

	if( !Q_stricmp( cgs.currentmapname, "maps/wookievillage" ) )
  	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics ^7forced^5 to ^7rain^5 for this map.\n");
  	  	ATMOSPHERIC_WEATHER_TYPE = WEATHER_RAIN;
  	  	return( kludgeResult = qtrue );
  	}

	if( !Q_stricmp( cgs.currentmapname, "maps/ewok_village" ) )
  	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics ^7forced^5 to ^7rain^5 for this map.\n");
  	  	ATMOSPHERIC_WEATHER_TYPE = WEATHER_RAIN;
  	  	return( kludgeResult = qtrue );
  	}

	if( !Q_stricmp( cgs.currentmapname, "maps/coruscant_promenade" ) )
  	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics ^7forced^5 to ^7heavyrain^5 for this map.\n");
  	  	ATMOSPHERIC_WEATHER_TYPE = WEATHER_HEAVY_RAIN;
  	  	return( kludgeResult = qtrue );
  	}

	if (StringContainsWord(cgs.currentmapname, "baldemnicpine"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics ^7forced^5 to ^7forest^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_FOREST;
		return(kludgeResult = qtrue);
	}

	if (StringContainsWord(cgs.currentmapname, "baldemnicmushroom"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics ^7forced^5 to ^7forest^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_FOREST;
		return(kludgeResult = qtrue);
	}

	/*if( StringContainsWord( cgs.currentmapname, "baldemnic" ) && !StringContainsWord( cgs.currentmapname, "baldemnic11" ))
  	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics ^7forced^5 to ^7rainstorm^5 for this map.\n");
  	  	ATMOSPHERIC_WEATHER_TYPE = WEATHER_RAIN_STORM;
  	  	return( kludgeResult = qtrue );
  	}*/

	if (!Q_stricmp(cgs.currentmapname, "maps/mp/ctf3"))
	{
		trap->Print("^1*** ^3ATMOSPHERICS^5: atmospherics ^7forced^5 to ^7forest^5 for this map.\n");
		ATMOSPHERIC_WEATHER_TYPE = WEATHER_FOREST;
		return(kludgeResult = qtrue);
	}

  	return( kludgeResult = qfalse );
}
