#include "../qcommon/q_shared.h"
#include "../cgame/cg_local.h"
#include "../ui/ui_shared.h"
#include "../game/surfaceflags.h"
#include "../qcommon/inifile.h"

//
// TODO: Ingame editor for efx points...
//

#define MAX_MAP_EFX 128

typedef struct mapEfx_s {
	char		fileName[128] = { 0 };
	int			id;
	vec3_t		origin;
	vec3_t		angles;
	int			vol;
	int			rad;
} mapEfx_t;

qboolean		mapEfxLoaded = qfalse;
int				numMapEfx = 0;
mapEfx_t		mapEfx[MAX_MAP_EFX];

qboolean ParseVector(char *intext, int count, float *v) {
	COM_BeginParseSession("fuckthisconstcharstarstarshit");

	const char **text = (const char **)&intext;

	char	*token;
	int		i;

	// FIXME: spaces are currently required after parens, should change parseext...
	token = COM_ParseExt(text, qfalse);
	if (strcmp(token, "(")) {
		Com_Printf(S_COLOR_YELLOW  "WARNING: missing ( parenthesis in mapefx\n");
		return qfalse;
	}

	for (i = 0; i < count; i++) {
		token = COM_ParseExt(text, qfalse);
		if (!token[0]) {
			Com_Printf(S_COLOR_YELLOW  "WARNING: missing vector element in mapefx\n");
			return qfalse;
		}
		v[i] = atof(token);
	}

	token = COM_ParseExt(text, qfalse);
	if (strcmp(token, ")")) {
		Com_Printf(S_COLOR_YELLOW  "WARNING: missing ) parenthesis in mapefx\n");
		return qfalse;
	}

	return qtrue;
}

void CG_LoadEfxPoints(void)
{
	if (!mapEfxLoaded)
	{
		memset(mapEfx, 0, sizeof(mapEfx));

		char efxFile[128] = { 0 };
		sprintf(efxFile, "mapefx/%s.mapefx", cgs.currentmapname);

		char tstr[128];

		for (int i = 0; i < MAX_MAP_EFX; i++)
		{
			strcpy(mapEfx[numMapEfx].fileName, IniRead(efxFile, "EFX", va("EFX_FILE%i", i), ""));

			if (mapEfx[numMapEfx].fileName[0] != 0 && strlen(mapEfx[numMapEfx].fileName) > 0)
			{
				mapEfx[numMapEfx].vol = atoi(IniRead(efxFile, "EFX", va("EFX_VOLUME%i", i), "0"));
				mapEfx[numMapEfx].rad = atoi(IniRead(efxFile, "EFX", va("EFX_RADIUS%i", i), "0"));

				strcpy(tstr, IniRead(efxFile, "EFX", va("EFX_ORIGIN%i", i), ""));
				
				if (ParseVector(tstr, 3, mapEfx[numMapEfx].origin))
				{
					strcpy(tstr, IniRead(efxFile, "EFX", va("EFX_ANGLES%i", i), ""));

					if (!ParseVector(tstr, 3, mapEfx[numMapEfx].angles))
					{// Assume 0,0,0 if not specified...
						VectorClear(mapEfx[numMapEfx].angles);
					}

					mapEfx[numMapEfx].id = trap->FX_RegisterEffect(mapEfx[numMapEfx].fileName);

					numMapEfx++;
				}
			}
		}

		trap->Print("^1*** ^3%s^5: Loaded %i map efx infos.\n", "MAP-EFX", numMapEfx);
	}

	mapEfxLoaded = qtrue;
}

void CG_ReloadLoadEfxPoints(void)
{
	trap->Print("^1*** ^3%s^5: Reloading map efx infos.\n", "MAP-EFX");

	mapEfxLoaded = qfalse;
	numMapEfx = 0;

	CG_LoadEfxPoints();
}

void CG_AddMapEfx(void)
{
	// Make sure they have been loaded...
	CG_LoadEfxPoints();

	for (int i = 0; i < numMapEfx; i++)
	{// Add all the loaded efx to screen...
		mapEfx_t *efx = &mapEfx[i];

		if (!efx) continue;
		if (!efx->id) continue;

		vec3_t fwd;

		AngleVectors(efx->angles, fwd, NULL, NULL);
		
		extern qboolean CG_InFOV(vec3_t spot, vec3_t from, vec3_t fromAngles, int hFOV, int vFOV);

		float dist = Distance(efx->origin, cg.refdef.vieworg);
		qboolean inRange = (dist <= 8192) ? qtrue : qfalse;
		qboolean FOV = qfalse;

		if (!inRange)
		{
			FOV = CG_InFOV(efx->origin, cg.refdef.vieworg, cg.refdef.viewangles, 100, 180);
		}

		if (inRange || FOV)
		{
			trap->FX_PlayEffectID(efx->id, efx->origin, fwd, efx->vol, efx->rad, qfalse);
		}
	}
}

