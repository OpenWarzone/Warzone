#include "../qcommon/q_shared.h"
#include "../cgame/cg_local.h"
#include "../ui/ui_shared.h"
#include "../game/surfaceflags.h"
#include "../qcommon/inifile.h"

//
// TODO: Ingame editor for cubemap points...
//

#define MAX_CUBEMAPS 1024

typedef struct cubemapEditor_s {
	bool	cubemapEnabled;
	vec3_t	cubemapOrigins;
} cubemapEditor_t;

bool				cubemapsLoaded = false;
int					numCubemaps = 0;
cubemapEditor_t		cubeMaps[MAX_CUBEMAPS] = { 0 };

void CG_LoadCubemapPoints(void)
{
	if (!cg_cubeMapEdit.integer) return;

	if (!cubemapsLoaded)
	{
		fileHandle_t	f;

		trap->FS_Open(va("maps/%s.cubemaps", cgs.currentmapname), &f, FS_READ);

		if (f)
		{// We have an actual cubemap list... Use it's locations...
			trap->FS_Read(&numCubemaps, sizeof(int), f);

			// Check hard limit...
			if (numCubemaps > MAX_CUBEMAPS) numCubemaps = MAX_CUBEMAPS;

			for (int i = 0; i < numCubemaps; i++)
			{
				trap->FS_Read(&cubeMaps[i].cubemapEnabled, sizeof(bool), f);
				trap->FS_Read(&cubeMaps[i].cubemapOrigins, sizeof(vec3_t), f);
			}
		}

		trap->FS_Close(f);

		trap->Print("^1*** ^3%s^5: Loaded %i cubemap positions.\n", "CUBE-MAPPING", numCubemaps);

		cubemapsLoaded = true;
	}
}

void CG_SaveCubemapPoints(void)
{
	if (!cg_cubeMapEdit.integer) return;

	fileHandle_t	f;

	trap->FS_Open(va("maps/%s.cubemaps", cgs.currentmapname), &f, FS_WRITE);

	if (f)
	{// We have an actual cubemap list... Use it's locations...
		trap->FS_Write(&numCubemaps, sizeof(int), f);

		// Check hard limit...
		if (numCubemaps > MAX_CUBEMAPS) numCubemaps = MAX_CUBEMAPS;

		for (int i = 0; i < numCubemaps; i++)
		{
			trap->FS_Write(&cubeMaps[i].cubemapEnabled, sizeof(bool), f);
			trap->FS_Write(&cubeMaps[i].cubemapOrigins, sizeof(vec3_t), f);
		}
	}

	trap->FS_Close(f);

	trap->Print("^1*** ^3%s^5: Saved %i cubemap positions.\n", "CUBE-MAPPING", numCubemaps);
}

#define NUMBER_SIZE		8

void CG_DrawCubemaps(void)
{
	if (!cg_cubeMapEdit.integer) return;

	CG_LoadCubemapPoints();

	for (int i = 0; i < numCubemaps; i++)
	{
		vec3_t		delta, dir, vec, up = { 0, 0, 1 };
		float		len;
		int			digits[10], numdigits;

		refEntity_t re;

		if (Distance(cubeMaps[i].cubemapOrigins, cg.refdef.vieworg) > 4096)
		{// Too far away to bother drawing...
			continue;
		}

		re.reType = RT_SPRITE;
		re.radius = 16;

		if (cubeMaps[i].cubemapEnabled)
		{
			re.shaderRGBA[0] = 0x00;
			re.shaderRGBA[1] = 0x00;
			re.shaderRGBA[2] = 0xff;
			re.shaderRGBA[3] = 0xff;
		}
		else
		{
			re.shaderRGBA[0] = 0xff;
			re.shaderRGBA[1] = 0x00;
			re.shaderRGBA[2] = 0x00;
			re.shaderRGBA[3] = 0xff;
		}

		re.radius = NUMBER_SIZE / 2;

		VectorCopy(cubeMaps[i].cubemapOrigins, re.origin);

		VectorSubtract(cg.refdef.vieworg, re.origin, dir);
		CrossProduct(dir, up, vec);
		VectorNormalize(vec);

		// if the view would be "inside" the sprite, kill the sprite
		// so it doesn't add too much overdraw
		VectorSubtract(re.origin, cg.refdef.vieworg, delta);
		len = VectorLength(delta);
		if (len < 20) {
			continue;
		}

		int temp_num = i;

		for (numdigits = 0; !(numdigits && !temp_num); numdigits++) {
			digits[numdigits] = temp_num % 10;
			temp_num = temp_num / 10;
		}

		for (int j = 0; j < numdigits; j++) {
			VectorMA(re.origin, (float)(((float)numdigits / 2) - j) * NUMBER_SIZE, vec, re.origin);
			re.customShader = cgs.media.numberShaders[digits[numdigits - 1 - j]];
			AddRefEntityToScene(&re);
		}

		/*
		VectorClear(vec);
		AnglesToAxis(vec, re.axis);

		int temp_num = i;

		for (numdigits = 0; !(numdigits && !temp_num); numdigits++) {
			digits[numdigits] = temp_num % 10;
			temp_num = temp_num / 10;
		}

		for (int j = 0; j < numdigits; j++) {
			VectorMA(re.origin, (float) (((float) numdigits / 2) - j) * NUMBER_SIZE, vec, re.origin);
			re.customShader = cgs.media.numberShaders[digits[numdigits - 1 - j]];
			AddRefEntityToScene(&re);
		}
		*/
	}
}

//
// Commands...
//

void CG_AddCubeMap(void)
{
	if (!cg_cubeMapEdit.integer) return;

	if (numCubemaps >= MAX_CUBEMAPS)
	{
		trap->Print("^1*** ^3%s^5: Cubemap limit has been reached.\n", "CUBE-MAPPING");
		return;
	}

	cubeMaps[numCubemaps].cubemapEnabled = true;
	VectorCopy(cg.refdef.vieworg, cubeMaps[numCubemaps].cubemapOrigins);
	//cubeMaps[numCubemaps].cubemapOrigins[2] += 48.0;
	numCubemaps++;

	// Just save each time we add one... Still need to update client to load the changes at some point... Or maybe pass the array though, the API... Dunno...
	trap->Print("^1*** ^3%s^5: Cubemap added at %f %f %f.\n", "CUBE-MAPPING", cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2]);
	CG_SaveCubemapPoints();
}

void CG_DisableCubeMap(void)
{
	if (!cg_cubeMapEdit.integer) return;

	int		closest = -1;
	float	closestDistance = 999999.9;

	for (int i = 0; i < numCubemaps; i++)
	{
		float dist = Distance(cg.refdef.vieworg, cubeMaps[i].cubemapOrigins);

		if (dist < closestDistance)
		{
			closest = i;
			closestDistance = dist;
		}
	}

	if (!cubeMaps[closest].cubemapEnabled)
	{
		trap->Print("^1*** ^3%s^5: The cubemap at %f %f %f is already disabled.\n", "CUBE-MAPPING", cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2]);
		return;
	}

	if (closest != -1)
	{
		cubeMaps[closest].cubemapEnabled = false;
		trap->Print("^1*** ^3%s^5: The cubemap at %f %f %f is now disabled.\n", "CUBE-MAPPING", cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2]);

		// Just save each time we add one... Still need to update client to load the changes at some point... Or maybe pass the array though, the API... Dunno...
		CG_SaveCubemapPoints();
	}
}

void CG_EnableCubeMap(void)
{
	if (!cg_cubeMapEdit.integer) return;

	int		closest = -1;
	float	closestDistance = 999999.9;

	for (int i = 0; i < numCubemaps; i++)
	{
		float dist = Distance(cg.refdef.vieworg, cubeMaps[i].cubemapOrigins);

		if (dist < closestDistance)
		{
			closest = i;
			closestDistance = dist;
		}
	}

	if (cubeMaps[closest].cubemapEnabled)
	{
		trap->Print("^1*** ^3%s^5: The cubemap at %f %f %f is already enabled.\n", "CUBE-MAPPING", cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2]);
		return;
	}

	if (closest != -1)
	{
		cubeMaps[closest].cubemapEnabled = true;
		trap->Print("^1*** ^3%s^5: The cubemap at %f %f %f is now enabled.\n", "CUBE-MAPPING", cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2]);

		// Just save each time we add one... Still need to update client to load the changes at some point... Or maybe pass the array though, the API... Dunno...
		CG_SaveCubemapPoints();
	}
}
