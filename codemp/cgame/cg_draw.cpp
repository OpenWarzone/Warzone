// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

#include "game/bg_saga.h"
//#include "game/bg_class.h"


#include "ui/ui_shared.h"
#include "ui/ui_public.h"


extern qboolean drawingSniperScopeView;

//[AUTOWAYPOINT]
extern float aw_percent_complete;
extern void AIMod_AutoWaypoint_DrawProgress( void );
//[/AUTOWAYPOINT]

extern float CG_RadiusForCent( centity_t *cent );
qboolean CG_WorldCoordToScreenCoordFloat(vec3_t worldCoord, float *x, float *y);
qboolean CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle );
static void CG_DrawSiegeTimer(int timeRemaining, qboolean isMyTeam);
static void CG_DrawSiegeDeathTimer( int timeRemaining );
// nmckenzie: DUEL_HEALTH
void CG_DrawDuelistHealth ( float x, float y, float w, float h, int duelist );

// used for scoreboard
extern displayContextDef_t cgDC;

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;

int lastvalidlockdif;

extern float zoomFov; //this has to be global client-side

char systemChat[256];
char teamChat1[256];
char teamChat2[256];

// The time at which you died and the time it will take for you to rejoin game.
int cg_siegeDeathTime = 0;

#define MAX_HUD_TICS 4
const char *armorTicName[MAX_HUD_TICS] =
{
"armor_tic1",
"armor_tic2",
"armor_tic3",
"armor_tic4",
};

const char *healthTicName[MAX_HUD_TICS] =
{
"health_tic1",
"health_tic2",
"health_tic3",
"health_tic4",
};

const char *forceTicName[MAX_HUD_TICS] =
{
"force_tic1",
"force_tic2",
"force_tic3",
"force_tic4",
};

const char *ammoTicName[MAX_HUD_TICS] =
{
"ammo_tic1",
"ammo_tic2",
"ammo_tic3",
"ammo_tic4",
};

char *showPowersName[] =
{
	"HEAL2",//FP_HEAL
	"JUMP2",//FP_LEVITATION
	"SPEED2",//FP_SPEED
	"PUSH2",//FP_PUSH
	"PULL2",//FP_PULL
	"MINDTRICK2",//FP_TELEPTAHY
	"GRIP2",//FP_GRIP
	"LIGHTNING2",//FP_LIGHTNING
	"DARK_RAGE2",//FP_RAGE
	"PROTECT2",//FP_PROTECT
	"ABSORB2",//FP_ABSORB
	"TEAM_HEAL2",//FP_TEAM_HEAL
	"TEAM_REPLENISH2",//FP_TEAM_FORCE
	"DRAIN2",//FP_DRAIN
	"SEEING2",//FP_SEE
	"SABER_OFFENSE2",//FP_SABER_OFFENSE
	"SABER_DEFENSE2",//FP_SABER_DEFENSE
	"SABER_THROW2",//FP_SABERTHROW
	NULL
};


//Called from UI shared code. For now we'll just redirect to the normal anim load function.


int UI_ParseAnimationFile(const char *filename, animation_t *animset, qboolean isHumanoid)
{
	return BG_ParseAnimationFile(filename, animset, isHumanoid);
}

int MenuFontToHandle(int iMenuFont)
{
	switch (iMenuFont)
	{
		case FONT_SMALL:	return cgDC.Assets.qhSmallFont;
		case FONT_SMALL2:	return cgDC.Assets.qhSmall2Font;
		case FONT_MEDIUM:	return cgDC.Assets.qhMediumFont;
		case FONT_LARGE:	return cgDC.Assets.qhMediumFont;//cgDC.Assets.qhBigFont;
			//fixme? Big fonr isn't registered...?
	}

	return cgDC.Assets.qhMediumFont;
}


int CG_Text_Width(const char *text, float scale, int iMenuFont)
{
	int iFontIndex = MenuFontToHandle(iMenuFont);

	return trap->R_Font_StrLenPixels(text, iFontIndex, scale);
}

int CG_Text_Height(const char *text, float scale, int iMenuFont)
{
	int iFontIndex = MenuFontToHandle(iMenuFont);

	return trap->R_Font_HeightPixels(iFontIndex, scale);
}

#include "qcommon/qfiles.h"	// for STYLE_BLINK etc
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style, int iMenuFont)
{
	int iStyleOR = 0;
	int iFontIndex = MenuFontToHandle(iMenuFont);

	switch (style)
	{
	case  ITEM_TEXTSTYLE_NORMAL:			iStyleOR = 0;break;					// JK2 normal text
	case  ITEM_TEXTSTYLE_BLINK:				iStyleOR = STYLE_BLINK;break;		// JK2 fast blinking
	case  ITEM_TEXTSTYLE_PULSE:				iStyleOR = STYLE_BLINK;break;		// JK2 slow pulsing
	case  ITEM_TEXTSTYLE_SHADOWED:			iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_OUTLINED:			iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_OUTLINESHADOWED:	iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_SHADOWEDMORE:		iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	}

	trap->R_Font_DrawString(	x,		// int ox
							y,		// int oy
							text,	// const char *text
							color,	// paletteRGBA_c c
							iStyleOR | iFontIndex,	// const int iFontHandle
							!limit?-1:limit,		// iCharLimit (-1 = none)
							scale	// const float scale = 1.0f
							);
}

/*
================
CG_DrawZoomMask

================
*/

static void CG_DrawZoomMask( void )
{
	vec4_t		color1;
	float		level;
	static qboolean	flip = qtrue;

//	int ammo = cg_entities[0].gent->client->ps.ammo[weaponData[cent->currentState.weapon].ammoIndex];
	float cx, cy;
//	int val[5];
	float max, fi;

	// Check for Binocular specific zooming since we'll want to render different bits in each case
	if ( cg.predictedPlayerState.scopeType == SCOPE_BINOCULARS )
	{
		int val, i;
		float off;

		// zoom level
		level = (float)(80.0f - cg.predictedPlayerState.zoomFov) / 80.0f;

		// ...so we'll clamp it
		if ( level < 0.0f )
		{
			level = 0.0f;
		}
		else if ( level > 1.0f )
		{
			level = 1.0f;
		}

		// Using a magic number to convert the zoom level to scale amount
		level *= 162.0f;

		// draw blue tinted distortion mask, trying to make it as small as is necessary to fill in the viewable area
		trap->R_SetColor( colorTable[CT_WHITE] );
		CG_DrawPic( 34, 48, 570, 362, cgs.media.binocularStatic );

		// Black out the area behind the numbers
		trap->R_SetColor( colorTable[CT_BLACK]);
		CG_DrawPic( 212, 367, 200, 40, cgs.media.whiteShader );

		// Numbers should be kind of greenish
		color1[0] = 0.2f;
		color1[1] = 0.4f;
		color1[2] = 0.2f;
		color1[3] = 0.3f;
		trap->R_SetColor( color1 );

		// Draw scrolling numbers, use intervals 10 units apart--sorry, this section of code is just kind of hacked
		//	up with a bunch of magic numbers.....
		val = ((int)((cg.refdef.viewangles[YAW] + 180) / 10)) * 10;
		off = (cg.refdef.viewangles[YAW] + 180) - val;

		for ( i = -10; i < 30; i += 10 )
		{
			val -= 10;

			if ( val < 0 )
			{
				val += 360;
			}

			// we only want to draw the very far left one some of the time, if it's too far to the left it will
			//	poke outside the mask.
			if (( off > 3.0f && i == -10 ) || i > -10 )
			{
				// draw the value, but add 200 just to bump the range up...arbitrary, so change it if you like
				CG_DrawNumField( 155 + i * 10 + off * 10, 374, 3, val + 200, 24, 14, NUM_FONT_CHUNKY, qtrue );
				CG_DrawPic( 245 + (i-1) * 10 + off * 10, 376, 6, 6, cgs.media.whiteShader );
			}
		}

		CG_DrawPic( 212, 367, 200, 28, cgs.media.binocularOverlay );

		color1[0] = sin( cg.time * 0.01f ) * 0.5f + 0.5f;
		color1[0] = color1[0] * color1[0];
		color1[1] = color1[0];
		color1[2] = color1[0];
		color1[3] = 1.0f;

		trap->R_SetColor( color1 );

		CG_DrawPic( 82, 94, 16, 16, cgs.media.binocularCircle );

		// Flickery color
		color1[0] = 0.7f + crandom() * 0.1f;
		color1[1] = 0.8f + crandom() * 0.1f;
		color1[2] = 0.7f + crandom() * 0.1f;
		color1[3] = 1.0f;
		trap->R_SetColor( color1 );

		CG_DrawPic( 0, 0, 640, 480, cgs.media.binocularMask );

		CG_DrawPic( 4, 282 - level, 16, 16, cgs.media.binocularArrow );

		// The top triangle bit randomly flips
		if ( flip )
		{
			CG_DrawPic( 330, 60, -26, -30, cgs.media.binocularTri );
		}
		else
		{
			CG_DrawPic( 307, 40, 26, 30, cgs.media.binocularTri );
		}

		if ( random() > 0.98f && ( cg.time & 1024 ))
		{
			flip = (qboolean)!flip;
		}
	}
	else if ( cg.predictedPlayerState.scopeType )
	{
		qboolean drawTick = qfalse;

		//
		// Paint the scope itself over the view...
		//

		// disruptor zoom mode
		level = (float)(50.0f - zoomFov) / 50.0f;//(float)(80.0f - zoomFov) / 80.0f;

		// ...so we'll clamp it
		if ( level < 0.0f )
		{
			level = 0.0f;
		}
		else if ( level > 1.0f )
		{
			level = 1.0f;
		}

		// Using a magic number to convert the zoom level to a rotation amount that correlates more or less with the zoom artwork.
		level *= 103.0f;

		// Draw target mask
		trap->R_SetColor( colorTable[CT_WHITE] );

		if (strncmp(scopeData[cg.predictedPlayerState.scopeType].maskShader, "", strlen(scopeData[cg.predictedPlayerState.scopeType].maskShader)))
			CG_DrawPic(0, 0, 640, 480, trap->R_RegisterShader(scopeData[cg.predictedPlayerState.scopeType].maskShader));

		// apparently 99.0f is the full zoom level
		if ( level >= 99 )
		{
			// Fully zoomed, so make the rotating insert pulse
			color1[0] = 1.0f;
			color1[1] = 1.0f;
			color1[2] = 1.0f;
			color1[3] = 0.7f + sin( cg.time * 0.01f ) * 0.3f;

			trap->R_SetColor( color1 );
		}

		// Draw rotating insert
		if (cg.predictedPlayerState.scopeType == SCOPE_DISRUPTOR)
		{
			if (strncmp(scopeData[cg.predictedPlayerState.scopeType].insertShader, "", strlen(scopeData[cg.predictedPlayerState.scopeType].insertShader)))
				CG_DrawRotatePic2(320, 240, 640, 480, -level, trap->R_RegisterShader(scopeData[cg.predictedPlayerState.scopeType].insertShader));
		}
		else
		{
			if (strncmp(scopeData[cg.predictedPlayerState.scopeType].insertShader, "", strlen(scopeData[cg.predictedPlayerState.scopeType].insertShader)))
				CG_DrawPic(0, 0, 640, 480, trap->R_RegisterShader(scopeData[cg.predictedPlayerState.scopeType].insertShader));
		}

		max = 1.0f;

		color1[0] = (1.0f - max) * 2.0f;
		color1[1] = max * 1.5f;
		color1[2] = 0.0f;
		color1[3] = 1.0f;

		// If we are low on health, make us flash
		if ( max < 0.15f && ( cg.time & 512 ))
		{
			VectorClear( color1 );
		}

		if ( color1[0] > 1.0f )
		{
			color1[0] = 1.0f;
		}

		if ( color1[1] > 1.0f )
		{
			color1[1] = 1.0f;
		}

		trap->R_SetColor( color1 );

		max *= 58.0f;

		drawTick = (strncmp(scopeData[cg.predictedPlayerState.scopeType].tickShader, "", strlen(scopeData[cg.predictedPlayerState.scopeType].tickShader))) ? qtrue : qfalse;

		if (drawTick)
		{
			for (fi = 18.5f; fi <= 18.5f + max; fi += 3) // going from 15 to 45 degrees, with 5 degree increments
			{
				cx = 320 + sin((fi + 90.0f) / 57.296f) * 190;
				cy = 240 + cos((fi + 90.0f) / 57.296f) * 190;
				CG_DrawRotatePic2(cx, cy, 12, 24, 90 - fi, trap->R_RegisterShader(scopeData[cg.predictedPlayerState.scopeType].tickShader));
			}
		}

		if ( cg.predictedPlayerState.weaponstate == WEAPON_CHARGING_ALT )
		{
			trap->R_SetColor( colorTable[CT_WHITE] );

			// draw the charge level
			max = ( cg.time - cg.predictedPlayerState.weaponChargeTime ) / ( 50.0f * 30.0f ); // bad hardcodedness 50 is disruptor charge unit and 30 is max charge units allowed.

			if ( max > 1.0f )
			{
				max = 1.0f;
			}

			if (strncmp(scopeData[cg.predictedPlayerState.scopeType].chargeShader, "", strlen(scopeData[cg.predictedPlayerState.scopeType].chargeShader)))
				trap->R_DrawStretchPic(257, 435, 134 * max, 34, 0, 0, max, 1, trap->R_RegisterShader(scopeData[cg.predictedPlayerState.scopeType].chargeShader));
		}
	}
}


/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, void *ghoul2, int g2radius, qhandle_t skin, vec3_t origin, vec3_t angles ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3DIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.ghoul2 = ghoul2;
	ent.radius = g2radius;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap->R_ClearScene();
	AddRefEntityToScene( &ent );
	trap->R_RenderScene( &refdef );
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles )
{
	clientInfo_t	*ci;

	if (clientNum >= MAX_CLIENTS)
	{ //npc?
		return;
	}

	ci = &cgs.clientinfo[ clientNum ];

	CG_DrawPic( x, y, w, h, ci->modelIcon );

	// if they are deferred, draw a cross out
	if ( ci->deferred )
	{
		CG_DrawPic( x, y, w, h, cgs.media.deferShader );
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D ) {
	qhandle_t		cm;
	float			len;
	vec3_t			origin, angles;
	vec3_t			mins, maxs;
	qhandle_t		handle;

	if ( !force2D && cg_draw3DIcons.integer ) {
		x *= cgs.screenXScale;
		y *= cgs.screenYScale;
		w *= cgs.screenXScale;
		h *= cgs.screenYScale;

		VectorClear( angles );

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap->R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * ( maxs[2] - mins[2] );
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		angles[YAW] = 60 * sin( cg.time / 2000.0 );;

		if( team == FACTION_EMPIRE ) {
			handle = cgs.media.redFlagModel;
		} else if( team == FACTION_REBEL ) {
			handle = cgs.media.blueFlagModel;
		} else if( team == FACTION_FREE ) {
			handle = 0;//cgs.media.neutralFlagModel;
		} else {
			return;
		}
		CG_Draw3DModel( x, y, w, h, handle, NULL, 0, 0, origin, angles );
	} else if ( cg_drawIcons.integer ) {
		gitem_t *item;

		if( team == FACTION_EMPIRE ) {
			item = BG_FindItemForPowerup( PW_REDFLAG );
		} else if( team == FACTION_REBEL ) {
			item = BG_FindItemForPowerup( PW_BLUEFLAG );
		} else if( team == FACTION_FREE ) {
			item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		} else {
			return;
		}
		if (item) {
		  CG_DrawPic( x, y, w, h, cg_items[ ITEM_INDEX(item) ].icon );
		}
	}
}

/*
================
CG_DrawHealth
================
*/
void CG_DrawHealth( menuDef_t *menuHUD )
{
	vec4_t			calcColor;
	playerState_t	*ps;
	int				healthAmt;
	int				i,currValue,inc;
	itemDef_t		*focusItem;
	float percent;

	// Can we find the menu?
	if (!menuHUD)
	{
		return;
	}

	ps = &cg.snap->ps;

	// What's the health?
	healthAmt = ps->stats[STAT_HEALTH];
	if (healthAmt > ps->stats[STAT_MAX_HEALTH])
	{
		healthAmt = ps->stats[STAT_MAX_HEALTH];
	}


	inc = (float) ps->stats[STAT_MAX_HEALTH] / MAX_HUD_TICS;
	currValue = healthAmt;

	// Print the health tics, fading out the one which is partial health
	for (i=(MAX_HUD_TICS-1);i>=0;i--)
	{
		focusItem = Menu_FindItemByName(menuHUD, healthTicName[i]);

		if (!focusItem)	// This is bad
		{
			continue;
		}

		memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			percent = (float) currValue / inc;
			calcColor[3] *= percent;		// Fade it out
		}

		trap->R_SetColor( calcColor);

		CG_DrawPic(
			focusItem->window.rect.x,
			focusItem->window.rect.y,
			focusItem->window.rect.w,
			focusItem->window.rect.h,
			focusItem->window.background
			);

		currValue -= inc;
	}

	// Print the mueric amount
	focusItem = Menu_FindItemByName(menuHUD, "healthamount");
	if (focusItem)
	{
		// Print health amount
		trap->R_SetColor( focusItem->window.foreColor );

		CG_DrawNumField (
			focusItem->window.rect.x,
			focusItem->window.rect.y,
			3,
			ps->stats[STAT_HEALTH],
			focusItem->window.rect.w,
			focusItem->window.rect.h,
			NUM_FONT_SMALL,
			qfalse);
	}

}

/*
================
CG_DrawArmor
================
*/
void CG_DrawArmor( menuDef_t *menuHUD )
{
	vec4_t			calcColor;
	playerState_t	*ps;
	int				maxArmor;
	itemDef_t		*focusItem;
	float			percent,quarterArmor;
	int				i,currValue,inc;

	//ps = &cg.snap->ps;
	ps = &cg.predictedPlayerState;

	// Can we find the menu?
	if (!menuHUD)
	{
		return;
	}

	maxArmor = ps->stats[STAT_MAX_HEALTH];

	currValue = ps->stats[STAT_ARMOR];
	inc = (float) maxArmor / MAX_HUD_TICS;

	memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));
	for (i=(MAX_HUD_TICS-1);i>=0;i--)
	{
		focusItem = Menu_FindItemByName(menuHUD, armorTicName[i]);

		if (!focusItem)	// This is bad
		{
			continue;
		}

		memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			percent = (float) currValue / inc;
			calcColor[3] *= percent;
		}

		trap->R_SetColor( calcColor);

		if ((i==(MAX_HUD_TICS-1)) && (currValue < inc))
		{
			if (cg.HUDArmorFlag)
			{
				CG_DrawPic(
					focusItem->window.rect.x,
					focusItem->window.rect.y,
					focusItem->window.rect.w,
					focusItem->window.rect.h,
					focusItem->window.background
					);
			}
		}
		else
		{
				CG_DrawPic(
					focusItem->window.rect.x,
					focusItem->window.rect.y,
					focusItem->window.rect.w,
					focusItem->window.rect.h,
					focusItem->window.background
					);
		}

		currValue -= inc;
	}

	focusItem = Menu_FindItemByName(menuHUD, "armoramount");

	if (focusItem)
	{
		// Print armor amount
		trap->R_SetColor( focusItem->window.foreColor );

		CG_DrawNumField (
			focusItem->window.rect.x,
			focusItem->window.rect.y,
			3,
			ps->stats[STAT_ARMOR],
			focusItem->window.rect.w,
			focusItem->window.rect.h,
			NUM_FONT_SMALL,
			qfalse);
	}

	// If armor is low, flash a graphic to warn the player
	if (ps->stats[STAT_ARMOR])	// Is there armor? Draw the HUD Armor TIC
	{
		quarterArmor = (float) (ps->stats[STAT_MAX_HEALTH] / 4.0f);

		// Make tic flash if armor is at 25% of full armor
		if (ps->stats[STAT_ARMOR] < quarterArmor)		// Do whatever the flash timer says
		{
			if (cg.HUDTickFlashTime < cg.time)			// Flip at the same time
			{
				cg.HUDTickFlashTime = cg.time + 400;
				if (cg.HUDArmorFlag)
				{
					cg.HUDArmorFlag = qfalse;
				}
				else
				{
					cg.HUDArmorFlag = qtrue;
				}
			}
		}
		else
		{
			cg.HUDArmorFlag=qtrue;
		}
	}
	else						// No armor? Don't show it.
	{
		cg.HUDArmorFlag=qfalse;
	}

}

/*
================
CG_DrawSaberStyle

If the weapon is a light saber (which needs no ammo) then draw a graphic showing
the saber style (fast, medium, strong)
================
*/
static void CG_DrawSaberStyle( centity_t *cent, menuDef_t *menuHUD)
{
	itemDef_t		*focusItem;

	if (!cent->currentState.weapon ) // We don't have a weapon right now
	{
		return;
	}

	if ( cent->currentState.weapon != WP_SABER )
	{
		return;
	}

	// Can we find the menu?
	if (!menuHUD)
	{
		return;
	}


	// draw the current saber style in this window
	switch ( cg.predictedPlayerState.fd.saberDrawAnimLevel )
	{
	case 1://FORCE_LEVEL_1:
	case 5://FORCE_LEVEL_5://Tavion

		focusItem = Menu_FindItemByName(menuHUD, "saberstyle_fast");

		if (focusItem)
		{
			trap->R_SetColor( colorTable[CT_WHITE] );

			CG_DrawPic(
				focusItem->window.rect.x,
				focusItem->window.rect.y,
				focusItem->window.rect.w,
				focusItem->window.rect.h,
				focusItem->window.background
				);
		}

		break;
	case 2://FORCE_LEVEL_2:
	case 6://SS_DUAL
	case 7://SS_STAFF
		focusItem = Menu_FindItemByName(menuHUD, "saberstyle_medium");

		if (focusItem)
		{
			trap->R_SetColor( colorTable[CT_WHITE] );

			CG_DrawPic(
				focusItem->window.rect.x,
				focusItem->window.rect.y,
				focusItem->window.rect.w,
				focusItem->window.rect.h,
				focusItem->window.background
				);
		}
		break;
	case 3://FORCE_LEVEL_3:
	case 4://FORCE_LEVEL_4://Desann
		focusItem = Menu_FindItemByName(menuHUD, "saberstyle_strong");

		if (focusItem)
		{
			trap->R_SetColor( colorTable[CT_WHITE] );

			CG_DrawPic(
				focusItem->window.rect.x,
				focusItem->window.rect.y,
				focusItem->window.rect.w,
				focusItem->window.rect.h,
				focusItem->window.background
				);
		}
		break;
	}

}

/*
================
CG_DrawAmmo
================
*/
static void CG_DrawAmmo( centity_t	*cent,menuDef_t *menuHUD)
{
	playerState_t	*ps;
	int				i;
	vec4_t			calcColor;
	float			value=0.0f,inc = 0.0f,percent;
	itemDef_t		*focusItem;

	ps = &cg.snap->ps;

	// Can we find the menu?
	if (!menuHUD)
	{
		return;
	}

	if (!cent->currentState.weapon ) // We don't have a weapon right now
	{
		return;
	}

	value = 1;
	if (value < 0)	// No ammo
	{
		return;
	}

	//
	// ammo
	//
	if (cg.oldammo < value)
	{
		cg.oldAmmoTime = cg.time + 200;
	}

	cg.oldammo = value;

	focusItem = Menu_FindItemByName(menuHUD, "ammoamount");

#ifndef __MMO__
	if (weaponData[cent->currentState.weapon].energyPerShot == 0 &&
		weaponData[cent->currentState.weapon].altEnergyPerShot == 0)
#endif //__MMO__
	{ //just draw "infinite"
		inc = 8 / MAX_HUD_TICS;
		value = 8;

		focusItem = Menu_FindItemByName(menuHUD, "ammoinfinite");
		trap->R_SetColor( colorTable[CT_YELLOW] );
		if (focusItem)
		{
			CG_DrawProportionalString(focusItem->window.rect.x, focusItem->window.rect.y, "--", NUM_FONT_SMALL, focusItem->window.foreColor);
		}
	}

	trap->R_SetColor( colorTable[CT_WHITE] );

	// Draw tics
	for (i=MAX_HUD_TICS-1;i>=0;i--)
	{
		focusItem = Menu_FindItemByName(menuHUD, ammoTicName[i]);

		if (!focusItem)
		{
			continue;
		}

		memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));

		if ( value <= 0 )	// done
		{
			break;
		}
		else if (value < inc)	// partial tic
		{
			percent = value / inc;
			calcColor[3] = percent;
		}

		trap->R_SetColor( calcColor);

		CG_DrawPic(
			focusItem->window.rect.x,
			focusItem->window.rect.y,
			focusItem->window.rect.w,
			focusItem->window.rect.h,
			focusItem->window.background
			);

		value -= inc;
	}
}

/*
================
CG_DrawForcePower
================
*/
void CG_DrawForcePower( menuDef_t *menuHUD )
{
	int				i;
	vec4_t			calcColor;
	float			value,inc,percent;
	itemDef_t		*focusItem;
	const int		maxForcePower = 100;
	qboolean	flash=qfalse;

	// Can we find the menu?
	if (!menuHUD)
	{
		return;
	}

	// Make the hud flash by setting forceHUDTotalFlashTime above cg.time
	if (cg.forceHUDTotalFlashTime > cg.time )
	{
		flash = qtrue;
		if (cg.forceHUDNextFlashTime < cg.time)
		{
			cg.forceHUDNextFlashTime = cg.time + 400;
			trap->S_StartSound (NULL, 0, CHAN_LOCAL, cgs.media.noforceSound );

			if (cg.forceHUDActive)
			{
				cg.forceHUDActive = qfalse;
			}
			else
			{
				cg.forceHUDActive = qtrue;
			}

		}
	}
	else	// turn HUD back on if it had just finished flashing time.
	{
		cg.forceHUDNextFlashTime = 0;
		cg.forceHUDActive = qtrue;
	}

//	if (!cg.forceHUDActive)
//	{
//		return;
//	}

	inc = (float)  maxForcePower / MAX_HUD_TICS;
	value = cg.snap->ps.fd.forcePower;

	for (i=MAX_HUD_TICS-1;i>=0;i--)
	{
		focusItem = Menu_FindItemByName(menuHUD, forceTicName[i]);

		if (!focusItem)
		{
			continue;
		}

//		memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));

		if ( value <= 0 )	// done
		{
			break;
		}
		else if (value < inc)	// partial tic
		{
			if (flash)
			{
				memcpy(calcColor,  colorTable[CT_RED], sizeof(vec4_t));
			}
			else
			{
				memcpy(calcColor,  colorTable[CT_WHITE], sizeof(vec4_t));
			}

			percent = value / inc;
			calcColor[3] = percent;
		}
		else
		{
			if (flash)
			{
				memcpy(calcColor,  colorTable[CT_RED], sizeof(vec4_t));
			}
			else
			{
				memcpy(calcColor,  colorTable[CT_WHITE], sizeof(vec4_t));
			}
		}

		trap->R_SetColor( calcColor);

		CG_DrawPic(
			focusItem->window.rect.x,
			focusItem->window.rect.y,
			focusItem->window.rect.w,
			focusItem->window.rect.h,
			focusItem->window.background
			);

		value -= inc;
	}

	focusItem = Menu_FindItemByName(menuHUD, "forceamount");

	if (focusItem)
	{
		// Print force amount
		trap->R_SetColor( focusItem->window.foreColor );

		CG_DrawNumField (
			focusItem->window.rect.x,
			focusItem->window.rect.y,
			3,
			cg.snap->ps.fd.forcePower,
			focusItem->window.rect.w,
			focusItem->window.rect.h,
			NUM_FONT_SMALL,
			qfalse);
	}
}

//[STYLEBAR]Stoiss
#define STYLEBAR_H			15.0f								
#define STYLEBAR_W			100.0f
#define STYLEBAR_X			(SCREEN_WIDTH-STYLEBAR_W-2)			
#define STYLEBAR_Y			440.0f								
//[/STYLEBAR]Stoiss

void CG_DrawSaberStyleBar(void)
{
	vec4_t aColor = { 0.f, 0.f, 0.f, 1.f };
	//vec4_t bColor;
	//vec4_t cColor;
	float x = STYLEBAR_X;
	float y = STYLEBAR_Y;

	if (cg.snap->ps.weapon == WP_SABER)
	{
		switch (cg.snap->ps.fd.saberDrawAnimLevel)
		{
		case SS_FAST:
		case SS_TAVION:
		default:
			// blue
			aColor[0] = 0.f;
			aColor[1] = 0.f;
			aColor[2] = 1.f;
			break;
		case SS_MEDIUM:
		case SS_STAFF:
		case SS_DUAL:
			// yellow
			aColor[0] = 1.f;
			aColor[1] = 1.f;
			aColor[2] = 0.f;
			break;
		case SS_STRONG:
		case SS_DESANN:
			// red
			aColor[0] = 1.f;
			aColor[1] = 0.f;
			aColor[2] = 0.f;
			break;
		case SS_CROWD_CONTROL:
			aColor[0] = 0.f;
			aColor[1] = 1.f;
			aColor[2] = 1.f;
			break;
		}
	}

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x + 1.0f + STYLEBAR_W, y + 1.0f, STYLEBAR_W - 1.0f, STYLEBAR_H - 1.0f, aColor);


	//then draw the other part greyed out
	CG_FillRect(x + 1.0f, y + 1.0f, STYLEBAR_W - 1.0f, STYLEBAR_H - 1.0f, aColor);

	//draw the background (black)
	CG_DrawRect(x, y, STYLEBAR_W, STYLEBAR_H, 1.0f, colorTable[CT_BLACK]);

	cgs.media.currentBackground = trap->R_RegisterShader("gfx/hud/testhud2");
	CG_DrawPic(x - 55.0f, y - 10, STYLEBAR_W + 75, STYLEBAR_H + 20, cgs.media.currentBackground);



}


//[FORCEBAR]Scooper
#define FFUELBAR_H			15.0f								// These two decides the size of the fuel bar / Disse to bestemmer størrelsen på ... fuel bar'n =p
#define FFUELBAR_W			115.0f
#define FFUELBAR_X			(SCREEN_WIDTH-FFUELBAR_W-2)			// This here decides where on screen it's drawn. / Bestemmer hvor på skjermen det blir skrevet.
#define FFUELBAR_Y			415.0f								// This too / Denne også, x and y
//[/FORCEBAR]Scooper
// Du må justere DFUELBAR_X og DFUELBAR_Y ovenfor for å flytte hvor den blir tegnet.

void CG_DrawForceBar(void)										// Function that will draw the fuel bar 
{
	vec4_t aColor;
	vec4_t bColor;
	vec4_t cColor;
	float x = FFUELBAR_X;
	float y = FFUELBAR_Y;
	float percent;		// Finds the percent of your current dodge level.
	// So if you have 50 dodge, percent will be 50, representing 50%. And this will work even if you change the max dodge values. No need to change this in the future.
	float forcePowerMax = 100;

	if (cg.snap->ps.fd.forcePowerLevel[FP_SEE] >= FORCE_LEVEL_3)
		forcePowerMax += 15;

	percent = ((float)cg.snap->ps.fd.forcePower / forcePowerMax)*FFUELBAR_W;

	if (percent > FFUELBAR_W)
	{
		return;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	//color of the bar			This is the colors, you can change them as you want later. R G B A
	//aColor[0] = 0.0f;			// Mengde rødt (Alle verdier fra 0.0 til 1.0) R G B ikke R B G xP
	//aColor[1] = 0.0f;			// mengde grønt
	//aColor[2] = 0.5f;			// mengde blått
	//aColor[3] = 0.8f;			// gjennomsiktighet / transperency.
	// Nå blir fargen blå, for force powers.

	VectorCopy4(colorTable[CT_LTBLUE3], aColor);	// Nå får vi samme farge som teksten. nice

	//color of the border
	bColor[0] = 0.0f;
	bColor[1] = 0.0f;
	bColor[2] = 0.0f;
	bColor[3] = 0.3f;

	//color of greyed out "missing fuel"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.1f;



	//now draw the part to show how much health there is in the color specified
	//CG_FillRect(x+1.0f, y+1.0f+(FFUELBAR_H-percent), FFUELBAR_W-1.0f, FFUELBAR_H-1.0f-(FFUELBAR_H-percent), aColor);
	CG_FillRect(x + 1.0f + (FFUELBAR_W - percent), y + 1.0f/*+(FFUELBAR_H-percent)*/, FFUELBAR_W - 1.0f - (FFUELBAR_W - percent), FFUELBAR_H - 1.0f, aColor);

	//draw the background (black)
	CG_DrawRect(x, y, FFUELBAR_W, FFUELBAR_H, 1.0f, colorTable[CT_BLACK]);

	//then draw the other part greyed out
	CG_FillRect(x + 1.0f, y + 1.0f, FFUELBAR_W - percent, FFUELBAR_H - 1.0f, cColor);

	cgs.media.currentBackground = trap->R_RegisterShader("gfx/hud/testhud2");
	CG_DrawPic(x - 63.0f, y - 10, FFUELBAR_W + 90, FFUELBAR_H + 20, cgs.media.currentBackground);
}

//[HPEBAR]Stoiss
#define HPFUELBAR_H			15.0f								// These two decides the size of the fuel bar / Disse to bestemmer størrelsen på ... fuel bar'n =p
#define HPFUELBAR_W			115.0f
#define HPFUELBAR_X			(HPFUELBAR_W-112)	    // This here decides where on screen it's drawn. / Bestemmer hvor på skjermen det blir skrevet.
#define HPFUELBAR_Y			415.0f								// This too / Denne også, x and y
//[/HPBAR]Stoiss
// Du må justere DFUELBAR_X og DFUELBAR_Y ovenfor for å flytte hvor den blir tegnet.
// Prøv nå =p
void CG_DrawHPBar(void)										// Function that will draw the fuel bar 
{
	vec4_t aColor;
	vec4_t bColor;
	vec4_t cColor;
	//	vec4_t oColor;	// overflow color
	float x = HPFUELBAR_X;
	float y = HPFUELBAR_Y;
	float percent = ((float)cg.snap->ps.stats[STAT_HEALTH] / (float)cg.snap->ps.stats[STAT_MAX_HEALTH])*HPFUELBAR_W;		// Finds the percent of your current dodge level.
	// So if you have 50 dodge, percent will be 50, representing 50%. And this will work even if you change the max dodge values. No need to change this in the future.
	//float overflow = 0.0f;
	//int i;


	if (percent > HPFUELBAR_W)
	{
		percent = HPFUELBAR_W;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	//color of the bar			This is the colors, you can change them as you want later. R G B A
	/*	aColor[0] = 0.5f;			// Mengde rødt (Alle verdier fra 0.0 til 1.0) R G B ikke R B G xP
	aColor[1] = 0.0f;			// mengde grønt
	aColor[2] = 0.0f;			// mengde blått
	aColor[3] = 0.8f;			// gjennomsiktighet / transperency.
	*/
	VectorCopy4(colorTable[CT_HUD_RED], aColor);

	//	for(i = 0; i<3;++i)
	//		oColor[i] = aColor[i]*1.1f;
	//	oColor[3] = aColor[3];

	//color of the border
	bColor[0] = 0.0f;
	bColor[1] = 0.0f;
	bColor[2] = 0.0f;
	bColor[3] = 0.3f;

	//color of greyed out "missing fuel"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.1f;



	//now draw the part to show how much health there is in the color specified
	//CG_FillRect(x+1.0f, y+1.0f+(HPFUELBAR_H-percent), HPFUELBAR_W-1.0f, HPFUELBAR_H-1.0f-(HPFUELBAR_H-percent), aColor);
	CG_FillRect(x + 1.0f, y + 1.0f/*+(HPFUELBAR_H-percent)*/, HPFUELBAR_W - 1.0f - (HPFUELBAR_W - percent), HPFUELBAR_H - 1.0f, aColor);

	//draw the background (black)//Stoiss add Moved it under here cis it was not showing the black line up right on the coler code
	CG_DrawRect(x, y, HPFUELBAR_W, HPFUELBAR_H, 1.0f, colorTable[CT_BLACK]);

	//then draw the other part greyed out
	CG_FillRect(x + 1.0f + percent, y + 1.0f, HPFUELBAR_W - percent, HPFUELBAR_H - 1.0f, cColor);

	cgs.media.currentBackground = trap->R_RegisterShader("gfx/hud/testhud");
	CG_DrawPic(x - 40.0f, y - 10, HPFUELBAR_W + 88, HPFUELBAR_H + 20, cgs.media.currentBackground);

}

//[ARMOREBAR]Stoiss
#define AFUELBAR_H			15.0f								// These two decides the size of the fuel bar / Disse to bestemmer størrelsen på ... fuel bar'n =p
#define AFUELBAR_W			100.0f
#define AFUELBAR_X		    (AFUELBAR_W-97)	    // This here decides where on screen it's drawn. / Bestemmer hvor på skjermen det blir skrevet.
#define AFUELBAR_Y			440.0f								// This too / Denne også, x and y
//[/ARMORBAR]Stoiss
// Du må justere DFUELBAR_X og DFUELBAR_Y ovenfor for å flytte hvor den blir tegnet.
// Prøv nå =p
void CG_DrawArmorBar(void)										// Function that will draw the fuel bar 
{
	vec4_t aColor;
	vec4_t bColor;
	vec4_t cColor;
	//	vec4_t oColor;	// overflow color
	float x = AFUELBAR_X;
	float y = AFUELBAR_Y;
	float percent = ((float)cg.snap->ps.stats[STAT_ARMOR] / (float)cg.snap->ps.stats[STAT_MAX_HEALTH])*AFUELBAR_W;		// Finds the percent of your current dodge level.
	// So if you have 50 dodge, percent will be 50, representing 50%. And this will work even if you change the max dodge values. No need to change this in the future.
	//float overflow = 0.0f;
	//int i;


	if (percent > AFUELBAR_W)
	{
		percent = AFUELBAR_W;
		//return;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	//color of the bar			This is the colors, you can change them as you want later. R G B A
	/*	aColor[0] = 0.5f;			// Mengde rødt (Alle verdier fra 0.0 til 1.0) R G B ikke R B G xP
	aColor[1] = 0.0f;			// mengde grønt
	aColor[2] = 0.0f;			// mengde blått
	aColor[3] = 0.8f;			// gjennomsiktighet / transperency.
	*/
	VectorCopy4(colorTable[CT_HUD_GREEN], aColor);

	//	for(i = 0; i<3;++i)
	//		oColor[i] = aColor[i]*1.1f;
	//	oColor[3] = aColor[3];

	//color of the border
	bColor[0] = 0.0f;
	bColor[1] = 0.0f;
	bColor[2] = 0.0f;
	bColor[3] = 0.3f;

	//color of greyed out "missing fuel"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.1f;




	//now draw the part to show how much health there is in the color specified
	//CG_FillRect(x+1.0f, y+1.0f+(AFUELBAR_H-percent), AFUELBAR_W-1.0f, AFUELBAR_H-1.0f-(AFUELBAR_H-percent), aColor);
	CG_FillRect(x + 1.0f, y + 1.0f/*+(AFUELBAR_H-percent)*/, AFUELBAR_W - 1.0f - (AFUELBAR_W - percent), AFUELBAR_H - 1.0f, aColor);

	//draw the background (black)
	CG_DrawRect(x, y, AFUELBAR_W, AFUELBAR_H, 1.0f, colorTable[CT_BLACK]);

	//then draw the other part greyed out
	CG_FillRect(x + 1.0f + percent, y + 1.0f, AFUELBAR_W - percent, AFUELBAR_H - 1.0f, cColor);

	cgs.media.currentBackground = trap->R_RegisterShader("gfx/hud/testhud");
	CG_DrawPic(x - 40.0f, y - 10, HPFUELBAR_W + 71, HPFUELBAR_H + 20, cgs.media.currentBackground);
}

//draw meter showing jetpack fuel when it's not full
#define JETPACFUELBAR_H			100.0f
#define JETPACFUELBAR_W			15.0f
#define JETPACFUELBAR_X			(SCREEN_WIDTH-JETPACFUELBAR_W-620.0f)
#define JETPACFUELBAR_Y			260.0f
void CG_DrawJetPackBar(void)
{
	vec4_t aColor;
	vec4_t bColor;
	vec4_t cColor;
	float x = JETPACFUELBAR_X;
	float y = JETPACFUELBAR_Y;
	float percent = ((float)cg.snap->ps.jetpackFuel / 100.0f)*JETPACFUELBAR_H;  // jetpack changed (-at-) "percent" => "height"
	//float percent = ((float)cg.snap->ps.jetpackFuel/200.0f);  // jetpack changed (-at-) added for colors

	if (percent > JETPACFUELBAR_H)
	{
		return;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}
	VectorCopy4(colorTable[CT_HUD_ORANGE],aColor);

	//color of the bar
	/*aColor[0] = 0.5f;
	aColor[1] = 0.0f;
	aColor[2] = 0.0f;
	aColor[3] = 0.8f;*/

	//color of the border
	bColor[0] = 0.0f;
	bColor[1] = 0.0f;
	bColor[2] = 0.0f;
	bColor[3] = 0.3f;

	//color of greyed out "missing fuel"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.1f;

	

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x + 1.0f, y + 1.0f + (JETPACFUELBAR_H - percent), JETPACFUELBAR_W - 1.0f, JETPACFUELBAR_H - 1.0f - (JETPACFUELBAR_H - percent), aColor);

	//draw the background (black)
	CG_DrawRect(x, y, JETPACFUELBAR_W, JETPACFUELBAR_H, 1.0f, colorTable[CT_BLACK]);

	//then draw the other part greyed out
	CG_FillRect(x + 1.0f, y + 1.0f, JETPACFUELBAR_W - 1.0f, JETPACFUELBAR_H - percent, cColor);
}

//draw meter showing cloak fuel when it's not full
#define CLFUELBAR_H			100.0f
#define CLFUELBAR_W			20.0f
#define CLFUELBAR_X			(SCREEN_WIDTH-CLFUELBAR_W-8.0f)
#define CLFUELBAR_Y			240.0f
void CG_DrawCloakFuel(void)
{
	vec4_t aColor;
	vec4_t bColor;
	vec4_t cColor;
	float x = CLFUELBAR_X;
	float y = CLFUELBAR_Y;
	float percent = ((float)cg.snap->ps.cloakFuel/100.0f)*CLFUELBAR_H;

	if (percent > CLFUELBAR_H)
	{
		return;
	}

	if ( cg.snap->ps.jetpackFuel < 100 )
	{//if drawing jetpack fuel bar too, then move this over...?
		x -= (CLFUELBAR_W + 8.0f);
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	//color of the bar
	aColor[0] = 0.0f;
	aColor[1] = 0.0f;
	aColor[2] = 0.6f;
	aColor[3] = 0.8f;

	//color of the border
	bColor[0] = 0.0f;
	bColor[1] = 0.0f;
	bColor[2] = 0.0f;
	bColor[3] = 0.3f;

	//color of greyed out "missing fuel"
	cColor[0] = 0.1f;
	cColor[1] = 0.1f;
	cColor[2] = 0.3f;
	cColor[3] = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, CLFUELBAR_W, CLFUELBAR_H, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much fuel there is in the color specified
	CG_FillRect(x+1.0f, y+1.0f+(CLFUELBAR_H-percent), CLFUELBAR_W-1.0f, CLFUELBAR_H-1.0f-(CLFUELBAR_H-percent), aColor);

	//then draw the other part greyed out
	CG_FillRect(x+1.0f, y+1.0f, CLFUELBAR_W-1.0f, CLFUELBAR_H-percent, cColor);
}

//draw meter showing cloak fuel when it's not full
#define CLOAKFUELBAR_H			100.0f
#define CLOAKFUELBAR_W			15.0f
#define CLOAKFUELBAR_X			(SCREEN_WIDTH-CLOAKFUELBAR_W-6.0f)
#define CLOAKFUELBAR_Y			260.0f
void CG_DrawCloakBar(void)
{
	vec4_t aColor;
	vec4_t bColor;
	vec4_t cColor;
	float x = CLOAKFUELBAR_X;
	float y = CLOAKFUELBAR_Y;
	float percent = ((float)cg.snap->ps.cloakFuel / 100.0f)*CLOAKFUELBAR_H;

	if (percent > CLOAKFUELBAR_H)
	{
		return;
	}

	/*if ( cg.snap->ps.jetpackFuel < 100 )
	{//if drawing jetpack fuel bar too, then move this over...?
		x -= (JPFUELBAR_W+8.0f);
	}*/

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	VectorCopy4(colorTable[CT_GREEN],aColor);

	//color of the bar
	/*aColor[0] = 0.5f;
	aColor[1] = 0.0f;
	aColor[2] = 0.0f;
	aColor[3] = 0.8f;*/
	//color of the border
	bColor[0] = 0.0f;
	bColor[1] = 0.0f;
	bColor[2] = 0.0f;
	bColor[3] = 0.3f;

	//color of greyed out "missing fuel"
	cColor[0] = 0.1f;
	cColor[1] = 0.1f;
	cColor[2] = 0.3f;
	cColor[3] = 0.1f;

	

	//now draw the part to show how much fuel there is in the color specified
	CG_FillRect(x + 1.0f, y + 1.0f + (CLOAKFUELBAR_H - percent), CLOAKFUELBAR_W - 1.0f, CLOAKFUELBAR_H - 1.0f - (CLOAKFUELBAR_H - percent), aColor);

	//draw the background (black)
	CG_DrawRect(x, y, CLOAKFUELBAR_W, CLOAKFUELBAR_H, 1.0f, colorTable[CT_BLACK]);

	//then draw the other part greyed out
	CG_FillRect(x + 1.0f, y + 1.0f, CLOAKFUELBAR_W - 1.0f, CLOAKFUELBAR_H - percent, cColor);
}

#define MAX_SHOWPOWERS NUM_FORCE_POWERS

qboolean ForcePower_Valid(int i)
{
	if (i == FP_LEVITATION ||
		i == FP_SABER_OFFENSE ||
		i == FP_SABER_DEFENSE ||
		i == FP_SABERTHROW)
	{
		return qfalse;
	}

	if (cg.snap->ps.fd.forcePowersKnown & (1 << i))
	{
		return qtrue;
	}

	return qfalse;
}

void CG_DrawForceQuickBar( void )
{
	int		i;
	int		count;
	int		smallIconSize,bigIconSize;
	int		holdX, x, y, pad;
	int		sideLeftIconCnt,sideRightIconCnt;
	int		sideMax,holdCount,iconCnt;
	int		yOffset = 0;

	if (!cg.snap->ps.fd.forcePowersKnown)
	{
		return;
	}

	// count the number of powers owned
	count = 0;

	for (i=0;i < NUM_FORCE_POWERS;++i)
	{
		if (ForcePower_Valid(i))
		{
			count++;
		}
	}

	if (count == 0)	// If no force powers, don't display
	{
		return;
	}

	sideMax = 3;	// Max number of icons on the side

	// Calculate how many icons will appear to either side of the center one
	holdCount = count - 1;	// -1 for the center icon
	if (holdCount == 0)			// No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > (2*sideMax))	// Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else							// Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount/2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	smallIconSize = 30;
	bigIconSize = 30;
	pad = 12;

	x = 320;
	//y = 425;
	y = 370;

	i = BG_ProperForceIndex(cg.forceSelect) - 1;
	if (i < 0)
	{
		i = MAX_SHOWPOWERS - 1;
	}

	trap->R_SetColor(NULL);
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize);
	for (iconCnt=1;iconCnt<(sideLeftIconCnt+1);i--)
	{
		if (i < 0)
		{
			i = MAX_SHOWPOWERS - 1;
		}

		if (!ForcePower_Valid(forcePowerSorted[i]))	// Does he have this power?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (cgs.media.forcePowerIcons[forcePowerSorted[i]])
		{
			CG_DrawPic( holdX, y + yOffset, smallIconSize, smallIconSize, cgs.media.forcePowerIcons[forcePowerSorted[i]] );
			CG_DrawProportionalString(holdX, y + yOffset, va("F%i", iconCnt), UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);
			holdX -= (smallIconSize+pad);
		}
	}

	if (ForcePower_Valid(cg.forceSelect))
	{
		// Current Center Icon
		if (cgs.media.forcePowerIcons[cg.forceSelect])
		{
			CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2)) + yOffset, bigIconSize, bigIconSize, cgs.media.forcePowerIcons[cg.forceSelect] ); //only cache the icon for display
			CG_DrawProportionalString(holdX, y + yOffset, va("F%i", iconCnt), UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);
		}
	}

	i = BG_ProperForceIndex(cg.forceSelect) + 1;
	if (i>=MAX_SHOWPOWERS)
	{
		i = 0;
	}

	// Work forwards from current icon
	holdX = x + (bigIconSize/2) + pad;
	for (iconCnt=1;iconCnt<(sideRightIconCnt+1);i++)
	{
		if (i>=MAX_SHOWPOWERS)
		{
			i = 0;
		}

		if (!ForcePower_Valid(forcePowerSorted[i]))	// Does he have this power?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (cgs.media.forcePowerIcons[forcePowerSorted[i]])
		{
			CG_DrawPic( holdX, y + yOffset, smallIconSize, smallIconSize, cgs.media.forcePowerIcons[forcePowerSorted[i]] ); //only cache the icon for display
			CG_DrawProportionalString(holdX, y + yOffset, va("F%i", iconCnt), UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);
			holdX += (smallIconSize+pad);
		}
	}

	//if ( showPowersName[cg.forceSelect] )
	//{
	//	CG_DrawProportionalString(320, y + 30 + yOffset, CG_GetStringEdString("SP_INGAME", showPowersName[cg.forceSelect]), UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);
	//}
}

/*
================
CG_CheckInventoryQuickbars
================
*/

int QUICKBAR_CURRENT[12] = { { -1 } };
int EQUIPPED_CURRENT[16] = { { -1 } };

void CG_CheckInventoryQuickbars( void )
{
	//
	// Inventory Item Trashing...
	//

	if (inv_trashslot.integer >= 0)
	{// Tell server to trash the item in the given slot...
		if (inv_trashslot.integer < 64)
		{
			trap->SendClientCommand(va("trashslot %i", inv_trashslot.integer));
		}

		trap->Cvar_Set("inv_trashslot", "-1");
	}

	//
	// Inventory Item Moving...
	//

	if (inv_moveslotstart.integer >= 0 && inv_moveslotend.integer >= 0)
	{
		if (inv_moveslotstart.integer < 64 && inv_moveslotend.integer < 64)
		{
			trap->SendClientCommand(va("moveslot %i %i", inv_moveslotstart.integer, inv_moveslotend.integer));
		}

		trap->Cvar_Set("inv_moveslotstart", "-1");
		trap->Cvar_Set("inv_moveslotend", "-1");
	}

	//
	// Quickbar Updating...
	//

	if (inv_quickbar1.integer != QUICKBAR_CURRENT[0])
	{
		//trap->SendClientCommand(va("equipquickbar 1 %i", inv_quickbar1.integer));
		QUICKBAR_CURRENT[0] = inv_quickbar1.integer;
	}

	if (inv_quickbar2.integer != QUICKBAR_CURRENT[1])
	{
		//trap->SendClientCommand(va("equipquickbar 2 %i", inv_quickbar2.integer));
		QUICKBAR_CURRENT[1] = inv_quickbar2.integer;
	}

	if (inv_quickbar3.integer != QUICKBAR_CURRENT[2])
	{
		//trap->SendClientCommand(va("equipquickbar 3 %i", inv_quickbar3.integer));
		QUICKBAR_CURRENT[2] = inv_quickbar3.integer;
	}

	if (inv_quickbar4.integer != QUICKBAR_CURRENT[3])
	{
		//trap->SendClientCommand(va("equipquickbar 4 %i", inv_quickbar4.integer));
		QUICKBAR_CURRENT[3] = inv_quickbar4.integer;
	}

	if (inv_quickbar5.integer != QUICKBAR_CURRENT[4])
	{
		//trap->SendClientCommand(va("equipquickbar 5 %i", inv_quickbar5.integer));
		QUICKBAR_CURRENT[4] = inv_quickbar5.integer;
	}

	if (inv_quickbar6.integer != QUICKBAR_CURRENT[5])
	{
		//trap->SendClientCommand(va("equipquickbar 6 %i", inv_quickbar6.integer));
		QUICKBAR_CURRENT[5] = inv_quickbar6.integer;
	}

	if (inv_quickbar7.integer != QUICKBAR_CURRENT[6])
	{
		//trap->SendClientCommand(va("equipquickbar 7 %i", inv_quickbar7.integer));
		QUICKBAR_CURRENT[6] = inv_quickbar7.integer;
	}

	if (inv_quickbar8.integer != QUICKBAR_CURRENT[7])
	{
		//trap->SendClientCommand(va("equipquickbar 8 %i", inv_quickbar8.integer));
		QUICKBAR_CURRENT[7] = inv_quickbar8.integer;
	}

	if (inv_quickbar9.integer != QUICKBAR_CURRENT[8])
	{
		//trap->SendClientCommand(va("equipquickbar 9 %i", inv_quickbar9.integer));
		QUICKBAR_CURRENT[8] = inv_quickbar9.integer;
	}

	if (inv_quickbar10.integer != QUICKBAR_CURRENT[9])
	{
		//trap->SendClientCommand(va("equipquickbar 10 %i", inv_quickbar10.integer));
		QUICKBAR_CURRENT[9] = inv_quickbar10.integer;
	}

	if (inv_quickbar11.integer != QUICKBAR_CURRENT[10])
	{
		//trap->SendClientCommand(va("equipquickbar 11 %i", inv_quickbar11.integer));
		QUICKBAR_CURRENT[10] = inv_quickbar11.integer;
	}

	if (inv_quickbar12.integer != QUICKBAR_CURRENT[11])
	{
		//trap->SendClientCommand(va("equipquickbar 12 %i", inv_quickbar12.integer));
		QUICKBAR_CURRENT[11] = inv_quickbar12.integer;
	}

	//
	// Equip Slot Updating...
	//
	if (inv_equipment0.integer != EQUIPPED_CURRENT[0] && inv_equipment0.integer >= 0)
	{
		int		num;

		if (!cg.snap) {
			return;
		}
		if (cg.snap->ps.pm_flags & PMF_FOLLOW) {
			return;
		}

		if (cg.snap->ps.emplacedIndex)
		{
			return;
		}

		inventoryItem *item = BG_GetInventoryItemByID(cg.snap->ps.inventoryItems[inv_equipment0.integer]);

		if (item->getBaseItem()->giTag == WP_SABER)
		{
			cg.saberShutupTime = 0;
			num = WP_SABER;
		}
		else if (item->getBaseItem()->giTag == WP_MODULIZED_WEAPON)
		{
			num = WP_MODULIZED_WEAPON;
		}

		extern qboolean CG_WeaponSelectable(int i);

		if (CG_WeaponSelectable(num))
		{
			if (cg.weaponSelect != num)
			{
				trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPON);
				trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPONLOCAL);
				trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_SABER);
				trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_SABERLOCAL);

				if (num != WP_SABER)
				{
					cg.saberShutupTime = cg.time + WEAPON_SELECT_TIME;
				}
			}

			trap->SendClientCommand(va("equipslot 0 %i", inv_equipment0.integer));
			EQUIPPED_CURRENT[0] = inv_equipment0.integer;

			cg.weaponSelectTime = cg.time;
			cg.weaponSelect = num;
		}
	}

	if (inv_equipment1.integer != EQUIPPED_CURRENT[1])
	{
		trap->SendClientCommand(va("equipslot 1 %i", inv_equipment1.integer));
		EQUIPPED_CURRENT[1] = inv_equipment1.integer;
	}

	if (inv_equipment2.integer != EQUIPPED_CURRENT[2])
	{
		trap->SendClientCommand(va("equipslot 2 %i", inv_equipment2.integer));
		EQUIPPED_CURRENT[2] = inv_equipment2.integer;
	}

	if (inv_equipment3.integer != EQUIPPED_CURRENT[3])
	{
		trap->SendClientCommand(va("equipslot 3 %i", inv_equipment3.integer));
		EQUIPPED_CURRENT[3] = inv_equipment3.integer;
	}

	if (inv_equipment4.integer != EQUIPPED_CURRENT[4])
	{
		trap->SendClientCommand(va("equipslot 4 %i", inv_equipment4.integer));
		EQUIPPED_CURRENT[4] = inv_equipment4.integer;
	}

	if (inv_equipment5.integer != EQUIPPED_CURRENT[5])
	{
		trap->SendClientCommand(va("equipslot 5 %i", inv_equipment5.integer));
		EQUIPPED_CURRENT[5] = inv_equipment5.integer;
	}

	if (inv_equipment6.integer != EQUIPPED_CURRENT[6])
	{
		trap->SendClientCommand(va("equipslot 6 %i", inv_equipment6.integer));
		EQUIPPED_CURRENT[6] = inv_equipment6.integer;
	}

	if (inv_equipment7.integer != EQUIPPED_CURRENT[7])
	{
		trap->SendClientCommand(va("equipslot 7 %i", inv_equipment7.integer));
		EQUIPPED_CURRENT[7] = inv_equipment7.integer;
	}

	if (inv_equipment8.integer != EQUIPPED_CURRENT[8])
	{
		trap->SendClientCommand(va("equipslot 8 %i", inv_equipment8.integer));
		EQUIPPED_CURRENT[8] = inv_equipment8.integer;
	}

	if (inv_equipment9.integer != EQUIPPED_CURRENT[9])
	{
		trap->SendClientCommand(va("equipslot 9 %i", inv_equipment9.integer));
		EQUIPPED_CURRENT[9] = inv_equipment9.integer;
	}

	if (inv_equipment10.integer != EQUIPPED_CURRENT[10])
	{
		trap->SendClientCommand(va("equipslot 10 %i", inv_equipment10.integer));
		EQUIPPED_CURRENT[10] = inv_equipment10.integer;
	}

	if (inv_equipment11.integer != EQUIPPED_CURRENT[11])
	{
		trap->SendClientCommand(va("equipslot 11 %i", inv_equipment11.integer));
		EQUIPPED_CURRENT[11] = inv_equipment11.integer;
	}

	if (inv_equipment12.integer != EQUIPPED_CURRENT[12])
	{
		trap->SendClientCommand(va("equipslot 12 %i", inv_equipment12.integer));
		EQUIPPED_CURRENT[12] = inv_equipment12.integer;
	}

	if (inv_equipment13.integer != EQUIPPED_CURRENT[13])
	{
		trap->SendClientCommand(va("equipslot 13 %i", inv_equipment13.integer));
		EQUIPPED_CURRENT[13] = inv_equipment13.integer;
	}

	if (inv_equipment14.integer != EQUIPPED_CURRENT[14])
	{
		trap->SendClientCommand(va("equipslot 14 %i", inv_equipment14.integer));
		EQUIPPED_CURRENT[14] = inv_equipment14.integer;
	}

	if (inv_equipment15.integer != EQUIPPED_CURRENT[15])
	{
		trap->SendClientCommand(va("equipslot 15 %i", inv_equipment15.integer));
		EQUIPPED_CURRENT[15] = inv_equipment15.integer;
	}
}

/*
================
CG_DrawHUD
================
*/

void CG_DrawHUD(centity_t	*cent)
{
	CG_CheckInventoryQuickbars();

#ifdef __OLD_UI__
	menuDef_t	*menuHUD = NULL;
	itemDef_t	*focusItem = NULL;
	const char *scoreStr = NULL;
	int	scoreBias;
	char scoreBiasStr[16];
#endif //__OLD_UI__

	if (cg_hudFiles.integer)
	{
#ifdef __OLD_UI__
		int x = 0;
		int y = SCREEN_HEIGHT - 80;
		char ammoString[64];
		int weapX = x;

		CG_DrawScaledProportionalString(x + 122, y + 15, va("%i", cg.snap->ps.stats[STAT_HEALTH]),
			UI_SMALLFONT | UI_DROPSHADOW, colorTable[CT_HUD_RED], 0.5f);

		CG_DrawScaledProportionalString(x + 108, y + 54 - 14, va("%i", cg.snap->ps.stats[STAT_ARMOR]),
			UI_SMALLFONT | UI_DROPSHADOW, colorTable[CT_HUD_GREEN], 0.5f);

		if (cg.snap->ps.weapon == WP_SABER)
		{
			weapX += 16;

			if (cg.snap->ps.fd.saberDrawAnimLevel == SS_STAFF)
			{
				Com_sprintf(ammoString, sizeof(ammoString), "Juyo");
			}
			else if (cg.snap->ps.fd.saberDrawAnimLevel == SS_DUAL)
			{
				Com_sprintf(ammoString, sizeof(ammoString), "Niman");
			}
			else if (cg.snap->ps.fd.saberDrawAnimLevel == SS_TAVION)
			{
				Com_sprintf(ammoString, sizeof(ammoString), "Djem So");
			}
			else if (cg.snap->ps.fd.saberDrawAnimLevel == SS_DESANN)
			{
				Com_sprintf(ammoString, sizeof(ammoString), "Ataru");
			}
			else if (cg.snap->ps.fd.saberDrawAnimLevel == SS_STRONG)
			{
				Com_sprintf(ammoString, sizeof(ammoString), "Soresu");
			}
			else if (cg.snap->ps.fd.saberDrawAnimLevel == SS_MEDIUM)
			{
				Com_sprintf(ammoString, sizeof(ammoString), "Shii-Cho");
			}
			else if (cg.snap->ps.fd.saberDrawAnimLevel == SS_CROWD_CONTROL)
			{
				Com_sprintf(ammoString, sizeof(ammoString), "Warzone Experimental");
			}
			else if (cg.snap->ps.fd.saberDrawAnimLevel == SS_FAST)
			{
				//UI_SMALLFONT|UI_DROPSHADOW, colorTable[CT_BLUE], 0.5f );
				Com_sprintf(ammoString, sizeof(ammoString), "Makashi");
			}
		}
#ifndef __MMO__
		else
		{
			Com_sprintf(ammoString, sizeof(ammoString), "%i", cg.snap->ps.ammo[weaponData[cent->currentState.weapon].ammoIndex]);
		}
#endif //__MMO__

		/*CG_DrawScaledProportionalString(SCREEN_WIDTH - (weapX + 16 + 26), y + 57, va("%s", ammoString),
			UI_SMALLFONT | UI_DROPSHADOW, colorTable[CT_HUD_ORANGE], 0.5f);*/

		CG_DrawScaledProportionalString(SCREEN_WIDTH - (x + 18 + 14 + 106), y + 15, va("%i", cg.snap->ps.fd.forcePower),
			UI_SMALLFONT | UI_DROPSHADOW, colorTable[CT_LTBLUE3], 0.5f);

		//DP HUD in CG_HudFiles "1"
		/*CG_DrawScaledProportionalString( SCREEN_WIDTH-(x+18+18+85), y+36, va( "%i", cg.snap->ps.stats[STAT_LOCKBLOCK_POINT]),
		UI_SMALLFONT|UI_DROPSHADOW, colorTable[CT_HUD_RED], 0.5f );*/


		CG_DrawScaledProportionalString(SMALLCHAR_WIDTH - (x - 300), y + 50, va("^7XP ^2%i^7 / ^3%i", cg.snap->ps.stats[STAT_EXP], cg.maxExperience),
		UI_SMALLFONT|UI_DROPSHADOW, colorTable[CT_WHITE], 0.4f );
#endif //__OLD_UI__

		//[DODGEBAR]Scooper
		// This will cause it to draw the fuel bar / dette vil gjøre at den tegner fuel bar'n =p
#ifdef __OLD_UI__		

		//CG_DrawDodgeBar();
		CG_DrawSaberStyleBar();
		CG_DrawForceBar();
		CG_DrawHPBar();
		CG_DrawArmorBar();
#endif //__OLD_UI__
		//CG_DrawForceQuickBar();
		//CG_DrawJetPackBar();
		//CG_DrawCloakBar();

#ifdef __OLD_UI__
		cgs.media.currentBackground = trap->R_RegisterShader("gfx/hud/skywalker");
		CG_DrawPic(x + 518.0f, y + 37, STYLEBAR_W - 85, STYLEBAR_H + 5, cgs.media.currentBackground);
#endif //__OLD_UI__
		//[/DODGEBAR]Scooper
		return;
	}

#ifdef __OLD_UI__
	if (cg.predictedPlayerState.pm_type != PM_SPECTATOR)
	{
		// Draw the left HUD
		menuHUD = Menus_FindByName("lefthud");
		Menu_Paint( menuHUD, qtrue );

		if (menuHUD)
		{
			itemDef_t *focusItem;

			// Print scanline
			focusItem = Menu_FindItemByName(menuHUD, "scanline");
			if (focusItem)
			{
				trap->R_SetColor( colorTable[CT_WHITE] );
				CG_DrawPic(
					focusItem->window.rect.x,
					focusItem->window.rect.y,
					focusItem->window.rect.w,
					focusItem->window.rect.h,
					focusItem->window.background
					);
			}

			// Print frame
			focusItem = Menu_FindItemByName(menuHUD, "frame");
			if (focusItem)
			{
				trap->R_SetColor( colorTable[CT_WHITE] );
				CG_DrawPic(
					focusItem->window.rect.x,
					focusItem->window.rect.y,
					focusItem->window.rect.w,
					focusItem->window.rect.h,
					focusItem->window.background
					);
			}

			CG_DrawArmor(menuHUD);
			CG_DrawHealth(menuHUD);
		}
		else
		{
			//trap->Error( ERR_DROP, "CG_ChatBox_ArrayInsert: unable to locate HUD menu file ");
		}

		//scoreStr = va("Score: %i", cgs.clientinfo[cg.snap->ps.clientNum].score);
		if ( cgs.gametype == GT_DUEL )
		{//A duel that requires more than one kill to knock the current enemy back to the queue
			//show current kills out of how many needed
			scoreStr = va("%s: %i/%i", CG_GetStringEdString("MP_INGAME", "SCORE"), cg.snap->ps.persistant[PERS_SCORE], cgs.fraglimit);
		}
		else if (0 && cgs.gametype < GT_TEAM )
		{	// This is a teamless mode, draw the score bias.
			scoreBias = cg.snap->ps.persistant[PERS_SCORE] - cgs.scores1;
			if (scoreBias == 0)
			{	// We are the leader!
				if (cgs.scores2 <= 0)
				{	// Nobody to be ahead of yet.
					Com_sprintf(scoreBiasStr, sizeof(scoreBiasStr), "");
				}
				else
				{
					scoreBias = cg.snap->ps.persistant[PERS_SCORE] - cgs.scores2;
					if (scoreBias == 0)
					{
						Com_sprintf(scoreBiasStr, sizeof(scoreBiasStr), " (Tie)");
					}
					else
					{
						Com_sprintf(scoreBiasStr, sizeof(scoreBiasStr), " (+%d)", scoreBias);
					}
				}
			}
			else // if (scoreBias < 0)
			{	// We are behind!
				Com_sprintf(scoreBiasStr, sizeof(scoreBiasStr), " (%d)", scoreBias);
			}
			scoreStr = va("%s: %i%s", CG_GetStringEdString("MP_INGAME", "SCORE"), cg.snap->ps.persistant[PERS_SCORE], scoreBiasStr);
		}
		else
		{	// Don't draw a bias.
			scoreStr = va("%s: %i", CG_GetStringEdString("MP_INGAME", "SCORE"), cg.snap->ps.persistant[PERS_SCORE]);
		}

		menuHUD = Menus_FindByName("righthud");
		Menu_Paint( menuHUD, qtrue );

		if (menuHUD)
		{
			if (cgs.gametype != GT_POWERDUEL)
			{
				focusItem = Menu_FindItemByName(menuHUD, "score_line");
				if (focusItem)
				{
					CG_DrawScaledProportionalString(
						focusItem->window.rect.x,
						focusItem->window.rect.y,
						scoreStr,
						UI_RIGHT|UI_DROPSHADOW,
						focusItem->window.foreColor,
						0.7f);
				}
			}

			// Print scanline
			focusItem = Menu_FindItemByName(menuHUD, "scanline");
			if (focusItem)
			{
				trap->R_SetColor( colorTable[CT_WHITE] );
				CG_DrawPic(
					focusItem->window.rect.x,
					focusItem->window.rect.y,
					focusItem->window.rect.w,
					focusItem->window.rect.h,
					focusItem->window.background
					);
			}

			focusItem = Menu_FindItemByName(menuHUD, "frame");
			if (focusItem)
			{
				trap->R_SetColor( colorTable[CT_WHITE] );
				CG_DrawPic(
					focusItem->window.rect.x,
					focusItem->window.rect.y,
					focusItem->window.rect.w,
					focusItem->window.rect.h,
					focusItem->window.background
					);
			}

			CG_DrawForcePower(menuHUD);

			// Draw ammo tics or saber style
			if ( cent->currentState.weapon == WP_SABER )
			{
				CG_DrawSaberStyle(cent,menuHUD);
			}
			else
			{
				CG_DrawAmmo(cent,menuHUD);
			}
		}
		else
		{
			//trap->Error( ERR_DROP, "CG_ChatBox_ArrayInsert: unable to locate HUD menu file ");
		}
	}
#endif //__OLD_UI__
}

/*
===================
CG_DrawForceSelect
===================
*/
void CG_DrawForceSelect( void )
{
	int		i;
	int		count;
	int		smallIconSize,bigIconSize;
	int		holdX, x, y, pad;
	int		sideLeftIconCnt,sideRightIconCnt;
	int		sideMax,holdCount,iconCnt;
	int		yOffset = 0;

	// don't display if dead
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 )
	{
		return;
	}

	if ((cg.forceSelectTime+WEAPON_SELECT_TIME)<cg.time)	// Time is up for the HUD to display
	{
		cg.forceSelect = cg.snap->ps.fd.forcePowerSelected;
		return;
	}

	if (!cg.snap->ps.fd.forcePowersKnown)
	{
		return;
	}

	// count the number of powers owned
	count = 0;

	for (i=0;i < NUM_FORCE_POWERS;++i)
	{
		if (ForcePower_Valid(i))
		{
			count++;
		}
	}

	if (count == 0)	// If no force powers, don't display
	{
		return;
	}

	sideMax = 3;	// Max number of icons on the side

	// Calculate how many icons will appear to either side of the center one
	holdCount = count - 1;	// -1 for the center icon
	if (holdCount == 0)			// No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > (2*sideMax))	// Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else							// Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount/2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	smallIconSize = 30;
	bigIconSize = 60;
	pad = 12;

	x = 320;
	//y = 425;
	y = 370;

	i = BG_ProperForceIndex(cg.forceSelect) - 1;
	if (i < 0)
	{
		i = MAX_SHOWPOWERS - 1;
	}

	trap->R_SetColor(NULL);
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize);
	for (iconCnt=1;iconCnt<(sideLeftIconCnt+1);i--)
	{
		if (i < 0)
		{
			i = MAX_SHOWPOWERS - 1;
		}

		if (!ForcePower_Valid(forcePowerSorted[i]))	// Does he have this power?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (cgs.media.forcePowerIcons[forcePowerSorted[i]])
		{
			CG_DrawPic( holdX, y + yOffset, smallIconSize, smallIconSize, cgs.media.forcePowerIcons[forcePowerSorted[i]] );
			holdX -= (smallIconSize+pad);
		}
	}

	if (ForcePower_Valid(cg.forceSelect))
	{
		// Current Center Icon
		if (cgs.media.forcePowerIcons[cg.forceSelect])
		{
			CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2)) + yOffset, bigIconSize, bigIconSize, cgs.media.forcePowerIcons[cg.forceSelect] ); //only cache the icon for display
		}
	}

	i = BG_ProperForceIndex(cg.forceSelect) + 1;
	if (i>=MAX_SHOWPOWERS)
	{
		i = 0;
	}

	// Work forwards from current icon
	holdX = x + (bigIconSize/2) + pad;
	for (iconCnt=1;iconCnt<(sideRightIconCnt+1);i++)
	{
		if (i>=MAX_SHOWPOWERS)
		{
			i = 0;
		}

		if (!ForcePower_Valid(forcePowerSorted[i]))	// Does he have this power?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (cgs.media.forcePowerIcons[forcePowerSorted[i]])
		{
			CG_DrawPic( holdX, y + yOffset, smallIconSize, smallIconSize, cgs.media.forcePowerIcons[forcePowerSorted[i]] ); //only cache the icon for display
			holdX += (smallIconSize+pad);
		}
	}

	if ( showPowersName[cg.forceSelect] )
	{
		CG_DrawProportionalString(320, y + 30 + yOffset, CG_GetStringEdString("SP_INGAME", showPowersName[cg.forceSelect]), UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);

		if (cg_ttsPlayerVoice.integer >= 2)
			TextToSpeech((char *)CG_GetStringEdString("SP_INGAME", showPowersName[cg.forceSelect]), CG_GetTextToSpeechVoiceForEntity(&cg_entities[cg.clientNum]), cg.clientNum, cg.snap->ps.origin);
	}
}

/*
===================
CG_DrawInventorySelect
===================
*/
void CG_DrawInvenSelect( void )
{
	int				i;
	int				sideMax,holdCount,iconCnt;
	int				smallIconSize,bigIconSize;
	int				sideLeftIconCnt,sideRightIconCnt;
	int				count;
	int				holdX, x, y, y2, pad;
//	float			addX;

	// don't display if dead
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 )
	{
		return;
	}

	if ((cg.invenSelectTime+WEAPON_SELECT_TIME)<cg.time)	// Time is up for the HUD to display
	{
		return;
	}

	if (!cg.snap->ps.stats[STAT_HOLDABLE_ITEM] || !cg.snap->ps.stats[STAT_HOLDABLE_ITEMS])
	{
		return;
	}

	if (cg.itemSelect == -1)
	{
		cg.itemSelect = bg_itemlist[cg.snap->ps.stats[STAT_HOLDABLE_ITEM]].giTag;
	}

//const int bits = cg.snap->ps.stats[ STAT_ITEMS ];

	// count the number of items owned
	count = 0;
	for ( i = 0 ; i < HI_NUM_HOLDABLE ; i++ )
	{
		if (/*CG_InventorySelectable(i) && inv_icons[i]*/
			(cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << i)) )
		{
			count++;
		}
	}

	if (!count)
	{
		y2 = 0; //err?
		CG_DrawProportionalString(320, y2 + 22, "EMPTY INVENTORY", UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);
		return;
	}

	sideMax = 3;	// Max number of icons on the side

	// Calculate how many icons will appear to either side of the center one
	holdCount = count - 1;	// -1 for the center icon
	if (holdCount == 0)			// No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > (2*sideMax))	// Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else							// Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount/2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	i = cg.itemSelect - 1;
	if (i<0)
	{
		i = HI_NUM_HOLDABLE-1;
	}

	smallIconSize = 40;
	bigIconSize = 80;
	pad = 16;

	x = 320;
	y = 410;

	// Left side ICONS
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize);
//	addX = (float) smallIconSize * .75;

	for (iconCnt=0;iconCnt<sideLeftIconCnt;i--)
	{
		if (i<0)
		{
			i = HI_NUM_HOLDABLE-1;
		}

		if ( !(cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << i)) || i == cg.itemSelect )
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (!BG_IsItemSelectable(&cg.predictedPlayerState, i))
		{
			continue;
		}

		if (cgs.media.invenIcons[i])
		{
			trap->R_SetColor(NULL);
			CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, cgs.media.invenIcons[i] );

			trap->R_SetColor(colorTable[CT_ICON_BLUE]);
			/*CG_DrawNumField (holdX + addX, y + smallIconSize, 2, cg.snap->ps.inventory[i], 6, 12,
				NUM_FONT_SMALL,qfalse);
				*/

			holdX -= (smallIconSize+pad);
		}
	}

	// Current Center Icon
	if (cgs.media.invenIcons[cg.itemSelect] && BG_IsItemSelectable(&cg.predictedPlayerState, cg.itemSelect))
	{
		int itemNdex;
		trap->R_SetColor(NULL);
		CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10, bigIconSize, bigIconSize, cgs.media.invenIcons[cg.itemSelect] );
	//	addX = (float) bigIconSize * .75;
		trap->R_SetColor(colorTable[CT_ICON_BLUE]);
		/*CG_DrawNumField ((x-(bigIconSize/2)) + addX, y, 2, cg.snap->ps.inventory[cg.inventorySelect], 6, 12,
			NUM_FONT_SMALL,qfalse);*/

		itemNdex = BG_GetItemIndexByTag(cg.itemSelect, IT_HOLDABLE);
		if (bg_itemlist[itemNdex].classname)
		{
			vec4_t	textColor = { .312f, .75f, .621f, 1.0f };
			char	text[1024];
			char	upperKey[1024];

			strcpy(upperKey, bg_itemlist[itemNdex].classname);

			if ( trap->SE_GetStringTextString( va("SP_INGAME_%s",Q_strupr(upperKey)), text, sizeof( text )))
			{
				CG_DrawProportionalString(320, y+45, text, UI_CENTER | UI_SMALLFONT, textColor);
				
				if (cg_ttsPlayerVoice.integer >= 2) 
					TextToSpeech((char *)text, CG_GetTextToSpeechVoiceForEntity(&cg_entities[cg.clientNum]), cg.clientNum, cg.snap->ps.origin);
			}
			else
			{
				CG_DrawProportionalString(320, y+45, bg_itemlist[itemNdex].classname, UI_CENTER | UI_SMALLFONT, textColor);
				
				if (cg_ttsPlayerVoice.integer >= 2) 
					TextToSpeech((char *)bg_itemlist[itemNdex].classname, CG_GetTextToSpeechVoiceForEntity(&cg_entities[cg.clientNum]), cg.clientNum, cg.snap->ps.origin);
			}
		}
	}

	i = cg.itemSelect + 1;
	if (i> HI_NUM_HOLDABLE-1)
	{
		i = 0;
	}

	// Right side ICONS
	// Work forwards from current icon
	holdX = x + (bigIconSize/2) + pad;
//	addX = (float) smallIconSize * .75;
	for (iconCnt=0;iconCnt<sideRightIconCnt;i++)
	{
		if (i> HI_NUM_HOLDABLE-1)
		{
			i = 0;
		}

		if ( !(cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << i)) || i == cg.itemSelect )
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (!BG_IsItemSelectable(&cg.predictedPlayerState, i))
		{
			continue;
		}

		if (cgs.media.invenIcons[i])
		{
			trap->R_SetColor(NULL);
			CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, cgs.media.invenIcons[i] );

			trap->R_SetColor(colorTable[CT_ICON_BLUE]);
			/*CG_DrawNumField (holdX + addX, y + smallIconSize, 2, cg.snap->ps.inventory[i], 6, 12,
				NUM_FONT_SMALL,qfalse);*/

			holdX += (smallIconSize+pad);
		}
	}
}

int cg_targVeh = ENTITYNUM_NONE;
int cg_targVehLastTime = 0;
qboolean CG_CheckTargetVehicle( centity_t **pTargetVeh, float *alpha )
{
	int targetNum = ENTITYNUM_NONE;
	centity_t	*targetVeh = NULL;

	if ( !pTargetVeh || !alpha )
	{//hey, where are my pointers?
		return qfalse;
	}

	*alpha = 1.0f;

	//FIXME: need to clear all of these when you die?
	if ( cg.predictedPlayerState.rocketLockIndex < ENTITYNUM_WORLD )
	{
		targetNum = cg.predictedPlayerState.rocketLockIndex;
	}
	else if ( cg.crosshairVehNum < ENTITYNUM_WORLD
		&& cg.time - cg.crosshairVehTime < 3000 )
	{//crosshair was on a vehicle in the last 3 seconds
		targetNum = cg.crosshairVehNum;
	}
    else if ( cg.crosshairClientNum < ENTITYNUM_WORLD )
	{
		targetNum = cg.crosshairClientNum;
	}

	if ( targetNum < MAX_CLIENTS )
	{//real client
		if ( cg_entities[targetNum].currentState.m_iVehicleNum >= MAX_CLIENTS )
		{//in a vehicle
			targetNum = cg_entities[targetNum].currentState.m_iVehicleNum;
		}
	}
    if ( targetNum < ENTITYNUM_WORLD
		&& targetNum >= MAX_CLIENTS )
	{
		//centity_t *targetVeh = &cg_entities[targetNum];
		targetVeh = &cg_entities[targetNum];
		if ( targetVeh->currentState.NPC_class == CLASS_VEHICLE
			&& targetVeh->m_pVehicle
			&& targetVeh->m_pVehicle->m_pVehicleInfo
			&& targetVeh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER )
		{//it's a vehicle
			cg_targVeh = targetNum;
			cg_targVehLastTime = cg.time;
			*alpha = 1.0f;
		}
		else
		{
			targetVeh = NULL;
		}
	}
	if ( targetVeh )
	{
		*pTargetVeh = targetVeh;
		return qtrue;
	}

	if ( cg_targVehLastTime && cg.time - cg_targVehLastTime < 3000 )
	{
		targetVeh = &cg_entities[cg_targVeh];

		//stay at full alpha for 1 sec after lose them from crosshair
		if ( cg.time-cg_targVehLastTime < 1000 )
			*alpha = 1.0f;
		else //fade out over 2 secs
			*alpha = 1.0f-((cg.time-cg_targVehLastTime-1000)/2000.0f);
	}
	return qfalse;
}

#define MAX_VHUD_SHIELD_TICS 12
#define MAX_VHUD_SPEED_TICS 5
#define MAX_VHUD_ARMOR_TICS 5
#define MAX_VHUD_AMMO_TICS 5

float CG_DrawVehicleShields( const menuDef_t	*menuHUD, const centity_t *veh )
{
	int				i;
	char			itemName[64];
	float			inc, currValue,maxShields;
	vec4_t			calcColor;
	itemDef_t		*item;
	float			percShields;

	item = Menu_FindItemByName((menuDef_t	*) menuHUD, "armorbackground");

	if (item)
	{
		trap->R_SetColor( item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}

	maxShields = veh->m_pVehicle->m_pVehicleInfo->shields;
	currValue = cg.predictedVehicleState.stats[STAT_ARMOR];
	percShields = (float)currValue/(float)maxShields;
	// Print all the tics of the shield graphic
	// Look at the amount of health left and show only as much of the graphic as there is health.
	// Use alpha to fade out partial section of health
	inc = (float) maxShields / MAX_VHUD_ARMOR_TICS;
	for (i=1;i<=MAX_VHUD_ARMOR_TICS;i++)
	{
		sprintf( itemName, "armor_tic%d",	i );

		item = Menu_FindItemByName((menuDef_t *) menuHUD, itemName);

		if (!item)
		{
			continue;
		}

		memcpy(calcColor, item->window.foreColor, sizeof(vec4_t));

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			float percent = currValue / inc;
			calcColor[3] *= percent;		// Fade it out
		}

		trap->R_SetColor( calcColor);

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );

		currValue -= inc;
	}

	return percShields;
}

int cg_vehicleAmmoWarning = 0;
int cg_vehicleAmmoWarningTime = 0;
void CG_DrawVehicleAmmo( const menuDef_t *menuHUD, const centity_t *veh )
{
	int i;
	char itemName[64];
	float inc, currValue,maxAmmo;
	vec4_t	calcColor;
	itemDef_t	*item;

	item = Menu_FindItemByName((menuDef_t *) menuHUD, "ammobackground");

	if (item)
	{
		trap->R_SetColor( item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}

	maxAmmo = veh->m_pVehicle->m_pVehicleInfo->weapon[0].ammoMax;
	currValue = 1;//cg.predictedVehicleState.ammo[0];

	inc = (float) maxAmmo / MAX_VHUD_AMMO_TICS;
	for (i=1;i<=MAX_VHUD_AMMO_TICS;i++)
	{
		sprintf( itemName, "ammo_tic%d",	i );

		item = Menu_FindItemByName((menuDef_t *)menuHUD, itemName);

		if (!item)
		{
			continue;
		}

		if ( cg_vehicleAmmoWarningTime > cg.time
			&& cg_vehicleAmmoWarning == 0 )
		{
			memcpy(calcColor, g_color_table[ColorIndex(COLOR_RED)], sizeof(vec4_t));
			calcColor[3] = sin(cg.time*0.005)*0.5f+0.5f;
		}
		else
		{
			memcpy(calcColor, item->window.foreColor, sizeof(vec4_t));

			if (currValue <= 0)	// don't show tic
			{
				break;
			}
			else if (currValue < inc)	// partial tic (alpha it out)
			{
				float percent = currValue / inc;
				calcColor[3] *= percent;		// Fade it out
			}
		}

		trap->R_SetColor( calcColor);

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );

		currValue -= inc;
	}
}


void CG_DrawVehicleAmmoUpper( const menuDef_t *menuHUD, const centity_t *veh )
{
	int			i;
	char		itemName[64];
	float		inc, currValue,maxAmmo;
	vec4_t		calcColor;
	itemDef_t	*item;

	item = Menu_FindItemByName((menuDef_t *)menuHUD, "ammoupperbackground");

	if (item)
	{
		trap->R_SetColor( item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}

	maxAmmo = veh->m_pVehicle->m_pVehicleInfo->weapon[0].ammoMax;
	currValue = 1;//cg.predictedVehicleState.ammo[0];

	inc = (float) maxAmmo / MAX_VHUD_AMMO_TICS;
	for (i=1;i<MAX_VHUD_AMMO_TICS;i++)
	{
		sprintf( itemName, "ammoupper_tic%d",	i );

		item = Menu_FindItemByName((menuDef_t *)menuHUD, itemName);

		if (!item)
		{
			continue;
		}

		if ( cg_vehicleAmmoWarningTime > cg.time
			&& cg_vehicleAmmoWarning == 0 )
		{
			memcpy(calcColor, g_color_table[ColorIndex(COLOR_RED)], sizeof(vec4_t));
			calcColor[3] = sin(cg.time*0.005)*0.5f+0.5f;
		}
		else
		{
			memcpy(calcColor, item->window.foreColor, sizeof(vec4_t));

			if (currValue <= 0)	// don't show tic
			{
				break;
			}
			else if (currValue < inc)	// partial tic (alpha it out)
			{
				float percent = currValue / inc;
				calcColor[3] *= percent;		// Fade it out
			}
		}

		trap->R_SetColor( calcColor);

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );

		currValue -= inc;
	}
}


void CG_DrawVehicleAmmoLower( const menuDef_t *menuHUD, const centity_t *veh )
{
	int				i;
	char			itemName[64];
	float			inc, currValue,maxAmmo;
	vec4_t			calcColor;
	itemDef_t		*item;


	item = Menu_FindItemByName((menuDef_t *)menuHUD, "ammolowerbackground");

	if (item)
	{
		trap->R_SetColor( item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}

	maxAmmo = veh->m_pVehicle->m_pVehicleInfo->weapon[1].ammoMax;
	currValue = 1;//cg.predictedVehicleState.ammo[1];

	inc = (float) maxAmmo / MAX_VHUD_AMMO_TICS;
	for (i=1;i<MAX_VHUD_AMMO_TICS;i++)
	{
		sprintf( itemName, "ammolower_tic%d",	i );

		item = Menu_FindItemByName((menuDef_t *)menuHUD, itemName);

		if (!item)
		{
			continue;
		}

		if ( cg_vehicleAmmoWarningTime > cg.time
			&& cg_vehicleAmmoWarning == 1 )
		{
			memcpy(calcColor, g_color_table[ColorIndex(COLOR_RED)], sizeof(vec4_t));
			calcColor[3] = sin(cg.time*0.005)*0.5f+0.5f;
		}
		else
		{
			memcpy(calcColor, item->window.foreColor, sizeof(vec4_t));

			if (currValue <= 0)	// don't show tic
			{
				break;
			}
			else if (currValue < inc)	// partial tic (alpha it out)
			{
				float percent = currValue / inc;
				calcColor[3] *= percent;		// Fade it out
			}
		}

		trap->R_SetColor( calcColor);

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );

		currValue -= inc;
	}
}

// The HUD.menu file has the graphic print with a negative height, so it will print from the bottom up.
void CG_DrawVehicleTurboRecharge( const menuDef_t	*menuHUD, const centity_t *veh )
{
	itemDef_t	*item;
	int			height;

	item = Menu_FindItemByName( (menuDef_t	*) menuHUD, "turborecharge");

	if (item)
	{
		float percent=0.0f;
		int diff = ( cg.time - veh->m_pVehicle->m_iTurboTime );

		height = item->window.rect.h;

		if (diff > veh->m_pVehicle->m_pVehicleInfo->turboRecharge)
		{
			percent = 1.0f;
			trap->R_SetColor( colorTable[CT_GREEN] );
		}
		else
		{
			percent = (float) diff / veh->m_pVehicle->m_pVehicleInfo->turboRecharge;
			if (percent < 0.0f)
			{
				percent = 0.0f;
			}
			trap->R_SetColor( colorTable[CT_RED] );
		}

		height *= percent;

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			height,
			cgs.media.whiteShader);
	}
}

qboolean cg_drawLink = qfalse;
void CG_DrawVehicleWeaponsLinked( const menuDef_t	*menuHUD, const centity_t *veh )
{
	qboolean drawLink = qfalse;
	if ( veh->m_pVehicle
		&& veh->m_pVehicle->m_pVehicleInfo
		&& (veh->m_pVehicle->m_pVehicleInfo->weapon[0].linkable == 2|| veh->m_pVehicle->m_pVehicleInfo->weapon[1].linkable == 2) )
	{//weapon is always linked
		drawLink = qtrue;
	}
	else
	{
//MP way:
		//must get sent over network
		if ( cg.predictedVehicleState.vehWeaponsLinked )
		{
			drawLink = qtrue;
		}
//NOTE: below is SP way
/*
		//just cheat it
		if ( veh->gent->m_pVehicle->weaponStatus[0].linked
			|| veh->gent->m_pVehicle->weaponStatus[1].linked )
		{
			drawLink = qtrue;
		}
*/
	}

	if ( cg_drawLink != drawLink )
	{//state changed, play sound
		cg_drawLink = drawLink;
		trap->S_StartSound (NULL, cg.predictedPlayerState.clientNum, CHAN_LOCAL, trap->S_RegisterSound( "sound/vehicles/common/linkweaps.wav" ) );
	}

	if ( drawLink )
	{
		itemDef_t	*item;

		item = Menu_FindItemByName( (menuDef_t	*) menuHUD, "weaponslinked");

		if (item)
		{
			trap->R_SetColor( colorTable[CT_CYAN] );

				CG_DrawPic(
				item->window.rect.x,
				item->window.rect.y,
				item->window.rect.w,
				item->window.rect.h,
				cgs.media.whiteShader);
		}
	}
}

void CG_DrawVehicleSpeed( const menuDef_t	*menuHUD, const centity_t *veh )
{
	int i;
	char itemName[64];
	float inc, currValue,maxSpeed;
	vec4_t		calcColor;
	itemDef_t	*item;

	item = Menu_FindItemByName((menuDef_t *) menuHUD, "speedbackground");

	if (item)
	{
		trap->R_SetColor( item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}

	maxSpeed = veh->m_pVehicle->m_pVehicleInfo->speedMax;
	currValue = cg.predictedVehicleState.speed;


	// Print all the tics of the shield graphic
	// Look at the amount of health left and show only as much of the graphic as there is health.
	// Use alpha to fade out partial section of health
	inc = (float) maxSpeed / MAX_VHUD_SPEED_TICS;
	for (i=1;i<=MAX_VHUD_SPEED_TICS;i++)
	{
		sprintf( itemName, "speed_tic%d",	i );

		item = Menu_FindItemByName((menuDef_t *)menuHUD, itemName);

		if (!item)
		{
			continue;
		}

		if ( cg.time > veh->m_pVehicle->m_iTurboTime )
		{
			memcpy(calcColor, item->window.foreColor, sizeof(vec4_t));
		}
		else	// In turbo mode
		{
			if (cg.VHUDFlashTime < cg.time)
			{
				cg.VHUDFlashTime = cg.time + 200;
				if (cg.VHUDTurboFlag)
				{
					cg.VHUDTurboFlag = qfalse;
				}
				else
				{
					cg.VHUDTurboFlag = qtrue;
				}
			}

			if (cg.VHUDTurboFlag)
			{
				memcpy(calcColor, colorTable[CT_LTRED1], sizeof(vec4_t));
			}
			else
			{
				memcpy(calcColor, item->window.foreColor, sizeof(vec4_t));
			}
		}


		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			float percent = currValue / inc;
			calcColor[3] *= percent;		// Fade it out
		}

		trap->R_SetColor( calcColor);

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );

		currValue -= inc;
	}
}

void CG_DrawVehicleArmor( const menuDef_t *menuHUD, const centity_t *veh )
{
	int			i;
	vec4_t		calcColor;
	char		itemName[64];
	float		inc, currValue,maxArmor;
	itemDef_t	*item;

	maxArmor = veh->m_pVehicle->m_pVehicleInfo->armor;
	currValue = cg.predictedVehicleState.stats[STAT_HEALTH];

	item = Menu_FindItemByName( (menuDef_t	*) menuHUD, "shieldbackground");

	if (item)
	{
		trap->R_SetColor( item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}


	// Print all the tics of the shield graphic
	// Look at the amount of health left and show only as much of the graphic as there is health.
	// Use alpha to fade out partial section of health
	inc = (float) maxArmor / MAX_VHUD_SHIELD_TICS;
	for (i=1;i <= MAX_VHUD_SHIELD_TICS;i++)
	{
		sprintf( itemName, "shield_tic%d",	i );

		item = Menu_FindItemByName((menuDef_t	*) menuHUD, itemName);

		if (!item)
		{
			continue;
		}


		memcpy(calcColor, item->window.foreColor, sizeof(vec4_t));

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			float percent = currValue / inc;
			calcColor[3] *= percent;		// Fade it out
		}

		trap->R_SetColor( calcColor);

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );

		currValue -= inc;
	}
}

enum
{
	VEH_DAMAGE_FRONT=0,
	VEH_DAMAGE_BACK,
	VEH_DAMAGE_LEFT,
	VEH_DAMAGE_RIGHT,
};

typedef struct
{
	const char	*itemName;
	short	heavyDamage;
	short	lightDamage;
} veh_damage_t;

veh_damage_t vehDamageData[4] =
{
	{ "vehicle_front",SHIPSURF_DAMAGE_FRONT_HEAVY,SHIPSURF_DAMAGE_FRONT_LIGHT },
	{ "vehicle_back",SHIPSURF_DAMAGE_BACK_HEAVY,SHIPSURF_DAMAGE_BACK_LIGHT },
	{ "vehicle_left",SHIPSURF_DAMAGE_LEFT_HEAVY,SHIPSURF_DAMAGE_LEFT_LIGHT },
	{ "vehicle_right",SHIPSURF_DAMAGE_RIGHT_HEAVY,SHIPSURF_DAMAGE_RIGHT_LIGHT },
};

// Draw health graphic for given part of vehicle
void CG_DrawVehicleDamage(const centity_t *veh,int brokenLimbs,const menuDef_t	*menuHUD,float alpha,int index)
{
	itemDef_t		*item;
	int				colorI;
	vec4_t			color;
	int				graphicHandle=0;

	item = Menu_FindItemByName((menuDef_t *)menuHUD, vehDamageData[index].itemName);
	if (item)
	{
		if (brokenLimbs & (1<<vehDamageData[index].heavyDamage))
		{
			colorI = CT_RED;
			if (brokenLimbs & (1<<vehDamageData[index].lightDamage))
			{
				colorI = CT_DKGREY;
			}
		}
		else if (brokenLimbs & (1<<vehDamageData[index].lightDamage))
		{
			colorI = CT_YELLOW;
		}
		else
		{
			colorI = CT_GREEN;
		}

		VectorCopy4 ( colorTable[colorI], color );
		color[3] = alpha;
		trap->R_SetColor( color );

		switch ( index )
		{
			case VEH_DAMAGE_FRONT :
				graphicHandle = veh->m_pVehicle->m_pVehicleInfo->iconFrontHandle;
				break;
			case VEH_DAMAGE_BACK :
				graphicHandle = veh->m_pVehicle->m_pVehicleInfo->iconBackHandle;
				break;
			case VEH_DAMAGE_LEFT :
				graphicHandle = veh->m_pVehicle->m_pVehicleInfo->iconLeftHandle;
				break;
			case VEH_DAMAGE_RIGHT :
				graphicHandle = veh->m_pVehicle->m_pVehicleInfo->iconRightHandle;
				break;
		}

		if (graphicHandle)
		{
			CG_DrawPic(
				item->window.rect.x,
				item->window.rect.y,
				item->window.rect.w,
				item->window.rect.h,
				graphicHandle );
		}
	}
}


// Used on both damage indicators :  player vehicle and the vehicle the player is locked on
void CG_DrawVehicleDamageHUD(const centity_t *veh,int brokenLimbs,float percShields,char *menuName, float alpha)
{
	menuDef_t		*menuHUD;
	itemDef_t		*item;
	vec4_t			color;

	menuHUD = Menus_FindByName(menuName);

	if ( !menuHUD )
	{
		return;
	}

	item = Menu_FindItemByName(menuHUD, "background");
	if (item)
	{
		if (veh->m_pVehicle->m_pVehicleInfo->dmgIndicBackgroundHandle)
		{
			if ( veh->damageTime > cg.time )
			{//ship shields currently taking damage
				//NOTE: cent->damageAngle can be accessed to get the direction from the ship origin to the impact point (in 3-D space)
				float perc = 1.0f - ((veh->damageTime - cg.time) / 2000.0f/*MIN_SHIELD_TIME*/);
				if ( perc < 0.0f )
				{
					perc = 0.0f;
				}
				else if ( perc > 1.0f )
				{
					perc = 1.0f;
				}
				color[0] = item->window.foreColor[0];//flash red
				color[1] = item->window.foreColor[1]*perc;//fade other colors back in over time
				color[2] = item->window.foreColor[2]*perc;//fade other colors back in over time
				color[3] = item->window.foreColor[3];//always normal alpha
				trap->R_SetColor( color );
			}
			else
			{
				trap->R_SetColor( item->window.foreColor );
			}

			CG_DrawPic(
				item->window.rect.x,
				item->window.rect.y,
				item->window.rect.w,
				item->window.rect.h,
				veh->m_pVehicle->m_pVehicleInfo->dmgIndicBackgroundHandle );
		}
	}

	item = Menu_FindItemByName(menuHUD, "outer_frame");
	if (item)
	{
		if (veh->m_pVehicle->m_pVehicleInfo->dmgIndicFrameHandle)
		{
			trap->R_SetColor( item->window.foreColor );
			CG_DrawPic(
				item->window.rect.x,
				item->window.rect.y,
				item->window.rect.w,
				item->window.rect.h,
				veh->m_pVehicle->m_pVehicleInfo->dmgIndicFrameHandle );
		}
	}

	item = Menu_FindItemByName(menuHUD, "shields");
	if (item)
	{
		if (veh->m_pVehicle->m_pVehicleInfo->dmgIndicShieldHandle)
		{
			VectorCopy4 ( colorTable[CT_HUD_GREEN], color );
			color[3] = percShields;
			trap->R_SetColor( color );
			CG_DrawPic(
				item->window.rect.x,
				item->window.rect.y,
				item->window.rect.w,
				item->window.rect.h,
				veh->m_pVehicle->m_pVehicleInfo->dmgIndicShieldHandle );
		}
	}

	//TODO: if we check nextState.brokenLimbs & prevState.brokenLimbs, we can tell when a damage flag has been added and flash that part of the ship
	//FIXME: when ship explodes, either stop drawing ship or draw all parts black
	CG_DrawVehicleDamage(veh,brokenLimbs,menuHUD,alpha,VEH_DAMAGE_FRONT);
	CG_DrawVehicleDamage(veh,brokenLimbs,menuHUD,alpha,VEH_DAMAGE_BACK);
	CG_DrawVehicleDamage(veh,brokenLimbs,menuHUD,alpha,VEH_DAMAGE_LEFT);
	CG_DrawVehicleDamage(veh,brokenLimbs,menuHUD,alpha,VEH_DAMAGE_RIGHT);
}

qboolean CG_DrawVehicleHud( const centity_t *cent )
{
	itemDef_t		*item;
	menuDef_t		*menuHUD;
	playerState_t	*ps;
	centity_t		*veh;
	float			shieldPerc,alpha;

	menuHUD = Menus_FindByName("swoopvehiclehud");
	if (!menuHUD)
	{
		return qtrue;	// Draw player HUD
	}

	ps = &cg.predictedPlayerState;

	if (!ps || !(ps->m_iVehicleNum))
	{
		return qtrue;	// Draw player HUD
	}
	veh = &cg_entities[ps->m_iVehicleNum];

	if ( !veh || !veh->m_pVehicle )
	{
		return qtrue;	// Draw player HUD
	}

	CG_DrawVehicleTurboRecharge( menuHUD, veh );
	CG_DrawVehicleWeaponsLinked( menuHUD, veh );

	item = Menu_FindItemByName(menuHUD, "leftframe");

	// Draw frame
	if (item)
	{
		trap->R_SetColor( item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}

	item = Menu_FindItemByName(menuHUD, "rightframe");

	if (item)
	{
		trap->R_SetColor( item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}


	CG_DrawVehicleArmor( menuHUD, veh );

	// Get animal hud for speed
//	if (veh->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
//	{
//		menuHUD = Menus_FindByName("tauntaunhud");
//	}


	CG_DrawVehicleSpeed( menuHUD, veh );

	// Revert to swoophud
//	if (veh->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
//	{
//		menuHUD = Menus_FindByName("swoopvehiclehud");
//	}

//	if (veh->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
//	{
		shieldPerc = CG_DrawVehicleShields( menuHUD, veh );
//	}

	if (veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID && !veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID)
	{
		CG_DrawVehicleAmmo( menuHUD, veh );
	}
	else if (veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID && veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID)
	{
		CG_DrawVehicleAmmoUpper( menuHUD, veh );
		CG_DrawVehicleAmmoLower( menuHUD, veh );
	}

	// If he's hidden, he must be in a vehicle
	if (veh->m_pVehicle->m_pVehicleInfo->hideRider)
	{
		CG_DrawVehicleDamageHUD(veh,cg.predictedVehicleState.brokenLimbs,shieldPerc,"vehicledamagehud",1.0f);

		// Has he targeted an enemy?
		if (CG_CheckTargetVehicle( &veh, &alpha ))
		{
			CG_DrawVehicleDamageHUD(veh,veh->currentState.brokenLimbs,((float)veh->currentState.activeForcePass/10.0f),"enemyvehicledamagehud",alpha);
		}

		return qfalse;	// Don't draw player HUD
	}

	return qtrue;	// Draw player HUD

}

/*
================
CG_DrawStats

================
*/
static void CG_DrawStats( void )
{
	centity_t		*cent;
	playerState_t	*ps;
	qboolean		drawHUD = qtrue;
/*	playerState_t	*ps;
	vec3_t			angles;
//	vec3_t		origin;

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}
*/
	cent = &cg_entities[cg.snap->ps.clientNum];
/*	ps = &cg.snap->ps;

	VectorClear( angles );

	// Do start
	if (!cg.interfaceStartupDone)
	{
		CG_InterfaceStartup();
	}

	cgi_UI_MenuPaintAll();*/

	if ( cent )
	{
		ps = &cg.predictedPlayerState;

		if ( (ps->m_iVehicleNum ) )	// In a vehicle???
		{
			drawHUD = CG_DrawVehicleHud( cent );
		}
	}

	if (drawHUD)
	{
		CG_DrawHUD(cent);
	}

	/*CG_DrawArmor(cent);
	CG_DrawHealth(cent);
	CG_DrawAmmo(cent);

	CG_DrawTalk(cent);*/
}

/*
===================
CG_DrawPickupItem
===================
*/
static void CG_DrawPickupItem( void ) {
	int		value;
	float	*fadeColor;

	value = cg.itemPickup;

	if ( value && cg_items[ value ].icon != -1 )
	{
		fadeColor = CG_FadeColor( cg.itemPickupTime, 3000 );
		if ( fadeColor )
		{
			CG_RegisterItemVisuals( value );
			trap->R_SetColor( fadeColor );
			CG_DrawPic( 573, 320, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
			trap->R_SetColor( NULL );
		}
	}
}

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team )
{
	vec4_t		hcolor;

	hcolor[3] = alpha;
	if ( team == FACTION_EMPIRE ) {
		hcolor[0] = 1;
		hcolor[1] = .2f;
		hcolor[2] = .2f;
	} else if ( team == FACTION_REBEL ) {
		hcolor[0] = .2f;
		hcolor[1] = .2f;
		hcolor[2] = 1;
	} else if ( team == FACTION_MANDALORIAN ) {
		hcolor[0] = .2f;
		hcolor[1] = .2f;
		hcolor[2] = 1;
	} else if ( team == FACTION_MERC ) {
		hcolor[0] = .2f;
		hcolor[1] = .2f;
		hcolor[2] = 1;
	} else if (team == FACTION_PIRATES ) {
		hcolor[0] = .5f;
		hcolor[1] = .5f;
		hcolor[2] = .5f;
	} else if (team == FACTION_WILDLIFE ) {
		hcolor[0] = .4f;
		hcolor[1] = .4f;
		hcolor[2] = 0;
	} else {
		return;
	}
//	trap->R_SetColor( hcolor );

	CG_FillRect ( x, y, w, h, hcolor );
//	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap->R_SetColor( NULL );
}


/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawMiniScoreboard
================
*/
static float CG_DrawMiniScoreboard ( float y )
{
	char temp[MAX_QPATH];
	int xOffset = 0;

	if ( !cg_drawScores.integer )
	{
		return y;
	}

	if (cgs.gametype == GT_SIEGE)
	{ //don't bother with this in siege
		return y;
	}

	if ( cgs.gametype >= GT_TEAM )
	{
		Q_strncpyz( temp, va( "%s: ", CG_GetStringEdString( "MP_INGAME", "RED" ) ), sizeof( temp ) );
		Q_strcat( temp, sizeof( temp ), cgs.scores1 == SCORE_NOT_PRESENT ? "-" : (va( "%i", cgs.scores1 )) );
		Q_strcat( temp, sizeof( temp ), va( " %s: ", CG_GetStringEdString( "MP_INGAME", "BLUE" ) ) );
		Q_strcat( temp, sizeof( temp ), cgs.scores2 == SCORE_NOT_PRESENT ? "-" : (va( "%i", cgs.scores2 )) );

		CG_Text_Paint( 630 - CG_Text_Width ( temp, 0.7f, FONT_MEDIUM ) + xOffset, y, 0.7f, colorWhite, temp, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM );
		y += 15;
	}
	else
	{
		/*
		strcpy ( temp, "1st: " );
		Q_strcat ( temp, MAX_QPATH, cgs.scores1==SCORE_NOT_PRESENT?"-":(va("%i",cgs.scores1)) );

		Q_strcat ( temp, MAX_QPATH, " 2nd: " );
		Q_strcat ( temp, MAX_QPATH, cgs.scores2==SCORE_NOT_PRESENT?"-":(va("%i",cgs.scores2)) );

		CG_Text_Paint( 630 - CG_Text_Width ( temp, 0.7f, FONT_SMALL ), y, 0.7f, colorWhite, temp, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM );
		y += 15;
		*/
		//rww - no longer doing this. Since the attacker now shows who is first, we print the score there.
	}


	return y;
}


//
//
//
//  NPC NAMES
//
//
//

qboolean HumanNamesLoaded = qfalse;

typedef struct
{// FIXME: Add other species name files...
	char	HumanNames[MAX_QPATH];
} name_list_t;

name_list_t NPC_NAME_LIST[8000];

int NUM_HUMAN_NAMES = 0;

extern qboolean CG_InFOV( vec3_t spot, vec3_t from, vec3_t fromAngles, int hFOV, int vFOV );

int			next_vischeck[MAX_GENTITIES];
qboolean	currently_visible[MAX_GENTITIES];

qboolean CG_CheckClientVisibility ( centity_t *cent )
{
	trace_t		trace;
	vec3_t		start, end;//, forward, right, up;
	centity_t	*traceEnt = NULL;

	if (!CG_InFOV( cent->lerpOrigin, cg.refdef.vieworg, cg.refdef.viewangles, cg.refdef.fov_x * 1.3, cg.refdef.fov_y * 1.3))
	{// We can skip a vischeck...
		return qfalse;
	}

	if (next_vischeck[cent->currentState.number] > cg.time)
	{
		return currently_visible[cent->currentState.number];
	}

	//next_vischeck[cent->currentState.number] = cg.time + 4000 + Q_irand(0, 1000); // offset the checks... -- UQ1: Gonna update more often when not visible...

	VectorCopy(cg.refdef.vieworg, start);
	start[2]+=42;

	VectorCopy(cent->lerpOrigin, end);
	end[2]+=42;

	CG_Trace( &trace, start, NULL, NULL, end, cg.clientNum, MASK_PLAYERSOLID/*MASK_SHOT*/ );

	traceEnt = &cg_entities[trace.entityNum];

	if (traceEnt == cent || trace.fraction == 1.0f)
	{
		currently_visible[cent->currentState.number] = qtrue;
		next_vischeck[cent->currentState.number] = cg.time + 4000 + Q_irand(0, 1000); // offset the checks...
		return qtrue;
	}

	currently_visible[cent->currentState.number] = qfalse;
	next_vischeck[cent->currentState.number] = cg.time + Q_irand(0, 1000); // offset the checks...
	return qfalse;
}

/* */
void
Load_NPC_Names ( void )
{				// Load bot first names from external file.
	char			*s, *t;
	int				len;
	fileHandle_t	f;
	char			*buf;
	char			*loadPath;
	int				num = 0;

	if (HumanNamesLoaded)
		return;

	loadPath = va( "npc_names_list.dat" );

	len = trap->FS_Open( loadPath, &f, FS_READ );

	HumanNamesLoaded = qtrue;

	if ( !f )
	{
		return;
	}

	if ( !len )
	{			//empty file
		trap->FS_Close( f );
		return;
	}

	if ( (buf = (char *)malloc( len + 1)) == 0 )
	{			//alloc memory for buffer
		trap->FS_Close( f );
		return;
	}

	trap->FS_Read( buf, len, f );
	buf[len] = 0;
	trap->FS_Close( f );

	strcpy( NPC_NAME_LIST[NUM_HUMAN_NAMES].HumanNames, "NONAME");
	NUM_HUMAN_NAMES++;
	/*strcpy( NPC_NAME_LIST[NUM_HUMAN_NAMES].HumanNames, "R2D2 Droid");
	NUM_HUMAN_NAMES++;
	strcpy( NPC_NAME_LIST[NUM_HUMAN_NAMES].HumanNames, "R5D2 Droid");
	NUM_HUMAN_NAMES++;
	strcpy( NPC_NAME_LIST[NUM_HUMAN_NAMES].HumanNames, "Protocol Droid");
	NUM_HUMAN_NAMES++;
	strcpy( NPC_NAME_LIST[NUM_HUMAN_NAMES].HumanNames, "Weequay");
	NUM_HUMAN_NAMES++;*/

	for ( t = s = buf; *t; /* */ )
	{
		num++;
		s = strchr( s, '\n' );
		if ( !s || num > len )
		{
			break;
		}

		while ( *s == '\n' )
		{
			*s++ = 0;
		}

		if ( *t )
		{
			if ( t[0] != '\0' && !Q_strncmp( "//", va( "%s", t), 2) == 0 )
			{	// Not a comment either... Record it in our list...
				Q_strncpyz( NPC_NAME_LIST[NUM_HUMAN_NAMES].HumanNames, va( "%s", t), strlen( va( "%s", t)) );
				NUM_HUMAN_NAMES++;
			}
		}

		t = s;
	}

	free(buf);
	NUM_HUMAN_NAMES--;
}

void CG_SanitizeString( char *in, char *out )
{
	int i = 0;
	int r = 0;

	while (in[i])
	{
		if (i >= 128-1)
		{ //the ui truncates the name here..
			break;
		}

		if (in[i] == '^')
		{
			if (in[i+1] >= 48 && //'0'
				in[i+1] <= 57) //'9'
			{ //only skip it if there's a number after it for the color
				i += 2;
				continue;
			}
			else
			{ //just skip the ^
				i++;
				continue;
			}
		}

		if (in[i] < 32)
		{
			i++;
			continue;
		}

		out[r] = in[i];
		r++;
		i++;
	}
	out[r] = 0;
}


/*
================
CG_DrawEnemyInfo
================
*/

//void CG_DrawHealthBar(centity_t *cent, float chX, float chY, float chW, float chH);
extern void CG_DrawRect_FixedBorder( float x, float y, float width, float height, int border, const float *color );
extern void CG_FilledBar(float x, float y, float w, float h, float *startColor, float *endColor, const float *bgColor, float frac, int flags);

//vec4_t	uqBG			=	{0.f,0.f,0.f,0.3f};
vec4_t	uqBG			=	{0.f,0.f,0.f,0.7f};
vec4_t	uqBorder		=	{0.28f,0.28f,0.28f,1.f};
vec4_t	uqHover			=	{0.3f,0.3f,0.3f,1.f};
vec4_t	uqText			=	{1.f,1.f,1.f,1.f};
vec4_t	uqLime			=	{0.f,1.f,0.f,0.7f};
vec4_t	uqCyan			=	{0.f,1.f,1.f,0.7f};
vec4_t	uqRed			=	{1.f,0.f,0.f,0.7f};
vec4_t	uqGreen			=	{0.f,1.f,0.f,0.7f};
vec4_t	uqYellow		=	{1.0, 1.0, 0.0, 1.0};
vec4_t	uqBlue			=	{0.f,0.f,1.f,0.7f};
vec4_t	uqOrange		=	{1.f,0.63f,0.1f,0.7f};
vec4_t	uqDefaultGrey	=	{0.38f,0.38f,0.38f,1.0f};
vec4_t	uqLightGrey		=	{0.5f,0.5f,0.5f,1.0f};
vec4_t	uqDarkGrey		=	{0.33f,0.33f,0.33f,1.0f};
vec4_t	uqAlmostWhite	=	{0.83f,0.81f,0.71f,1.0f};
vec4_t	uqAlmostBlack	=	{0.16f,0.16f,0.16f,1.0f};

void CG_DrawMyStatus( void )
{
	int				y = 0;
	char			str1[255], str2[255];
	vec4_t			tclr, tclr2;
	float			boxX, boxXmid, sizeX, sizeY, healthPerc, forcePerc, armorPerc;
	int				flags = 64|128;
	centity_t		*crosshairEnt;
	clientInfo_t	*ci;

	// Select our crosshair entity for stats...
	crosshairEnt = &cg_entities[cg.clientNum];
	ci = &cgs.clientinfo[cg.clientNum];

	if (!crosshairEnt)
	{
		return; // nothing to show...
	}

	if (crosshairEnt->playerState->fd.forcePowerMax <= 0) crosshairEnt->playerState->fd.forcePowerMax = 100;

	//str1 = ci->name;
	sprintf(str1, "%s", ci->cleanname);
	sprintf(str2, "< Jedi >"); // UQ1: FIXME - Selected Player Class Name...
	tclr[0] = 0.125f;
	tclr[1] = 0.325f;
	tclr[2] = 0.7f;
	tclr[3] = 1.0f;

	tclr2[0] = 0.125f;
	tclr2[1] = 0.325f;
	tclr2[2] = 0.7f;
	tclr2[3] = 1.0f;

	/*
	sprintf(str2, "< Sith >");
	tclr[0] = 1.0f;
	tclr[1] = 0.325f;
	tclr[2] = 0.125f;
	tclr[3] = 1.0f;

	tclr2[0] = 1.0f;
	tclr2[1] = 0.325f;
	tclr2[2] = 0.125f;
	tclr2[3] = 1.0f;
	*/

	y = 5;

	sizeX = 160;
	sizeY = 38;

	boxX = 5;
	boxXmid = (boxX + sizeY - 4) + ((sizeX - (boxX + sizeY - 4))/2);

	// Draw a transparent box background...
	CG_FillRect( boxX, y, sizeX, sizeY, uqBG );

	// Draw the border...
	//CG_DrawRect_FixedBorder( boxX, y, sizeX, sizeY, 1, uqBorder );

	y += 2;

	if ( ci->modelIcon )
	{
		CG_DrawPic( boxX + 2, y, sizeY-4, sizeY-4, ci->modelIcon );
	}

	boxX += sizeY + 2;

	// Draw their name...
	CG_Text_Paint( boxXmid - (CG_Text_Width ( str1, 0.4f, FONT_SMALL ) * 0.5), y, 0.4f, colorWhite/*tclr*/, str1, 0, 0, 0, FONT_SMALL );
	y += 10;

	// Draw their type...
	CG_Text_Paint( boxXmid - (CG_Text_Width ( str2, 0.35f, FONT_SMALL ) * 0.5), y, 0.35f, colorWhite/*tclr2*/, str2, 0, 0, 0, FONT_SMALL );
	y += 12;

	// Draw their health bar...
	//if (crosshairEnt->currentState.health == 0 || crosshairEnt->currentState.maxhealth == 0)
	//	healthPerc = 1; // No health data yet. Assume 100%.
	//else
	//	healthPerc = ((float)crosshairEnt->currentState.health / (float)crosshairEnt->currentState.maxhealth);

	if (crosshairEnt->playerState->stats[STAT_HEALTH] == 0 || crosshairEnt->playerState->stats[STAT_MAX_HEALTH] == 0)
		healthPerc = 1; // No health data yet. Assume 100%.
	else
		healthPerc = ((float)crosshairEnt->playerState->stats[STAT_HEALTH] / (float)crosshairEnt->playerState->stats[STAT_MAX_HEALTH]);

	CG_FilledBar( boxX + 2, y, sizeX-sizeY-4-6, 5, uqRed, NULL, NULL, healthPerc, flags );
	//CG_DrawRect_FixedBorder( boxX + 2, y, sizeX-sizeY-4-8, 5, 1, uqBorder );

	// Draw their armor bar...
	if (crosshairEnt->playerState->stats[STAT_ARMOR] == 0 || crosshairEnt->playerState->stats[STAT_MAX_HEALTH] == 0)
		armorPerc = 0; // No health data yet. Assume 0%.
	else
		armorPerc = ((float)crosshairEnt->playerState->stats[STAT_ARMOR] / (float)crosshairEnt->playerState->stats[STAT_MAX_HEALTH]);

	CG_FilledBar(boxX + 2, y, sizeX - sizeY - 4 - 6, 2, uqGreen, NULL, NULL, armorPerc, flags);
	// Write "XXX%/XXX%" over the bar in white...
	CG_Text_Paint( boxXmid - (CG_Text_Width ( va("%i/%i", (int)(armorPerc*100), (int)(healthPerc*100)), 0.35f, FONT_SMALL ) * 0.5), y-2, 0.35f, colorWhite, va("%i/%i", (int)(armorPerc*100), (int)(healthPerc*100)), 0, 0, 0, FONT_SMALL );

	y += 7;

	// Draw their force bar...
	if (!crosshairEnt->playerState || crosshairEnt->playerState->fd.forcePower == 0 || crosshairEnt->playerState->fd.forcePowerMax == 0)
		forcePerc = 1; // No force/power data yet. Assume 100%.
	else
		forcePerc = ((float)crosshairEnt->playerState->fd.forcePower / (float)crosshairEnt->playerState->fd.forcePowerMax);

	CG_FilledBar( boxX + 2, y, sizeX-sizeY-4-6, 5, uqBlue, NULL, NULL, forcePerc, flags );
	// Write "XXX%" over the bar in white...
	CG_Text_Paint( boxXmid - (CG_Text_Width ( va("%i", (int)(forcePerc*100)), 0.35f, FONT_SMALL ) * 0.5), y-2, 0.35f, colorWhite, va("%i", (int)(forcePerc*100)), 0, 0, 0, FONT_SMALL );
	//CG_DrawRect_FixedBorder( boxX + 2, y, sizeX-sizeY-4-8, 5, 1, uqBorder );

	if (healthPerc <= 0.25 && cg_ttsPlayerVoice.integer) 
		TextToSpeech("My health is low.", CG_GetTextToSpeechVoiceForEntity(&cg_entities[cg.clientNum]), cg.clientNum, cg.snap->ps.origin);
	if (forcePerc <= 0.25 && cg_ttsPlayerVoice.integer) 
		TextToSpeech("My force is low.", CG_GetTextToSpeechVoiceForEntity(&cg_entities[cg.clientNum]), cg.clientNum, cg.snap->ps.origin);

	y += 7;
}

int currentCrosshairEntity = -1;

void CG_DrawEnemyStatus( void )
{
	int				y = 0;
	char			str1[255], str2[255];
	vec4_t			tclr, tclr2;
	float			boxX, boxXmid, sizeX, sizeY, healthPerc, forcePerc, armorPerc;
	int				flags = 64|128;
	centity_t		*crosshairEnt;
	clientInfo_t	*ci = NULL;

	if ( cg.crosshairClientNum >= 0 && cg.crosshairClientNum != currentCrosshairEntity ) // player
	{
		// Store current (last looked at) target if it changes...
		currentCrosshairEntity = cg.crosshairClientNum; // player
	}

	// Select our crosshair entity for stats...
	crosshairEnt = &cg_entities[currentCrosshairEntity];

	//CG_DrawHealthBar(cent, x3, y3, w, 100);
	if (!crosshairEnt)
	{
		return; // nothing to show...
	}

	if (crosshairEnt->currentState.eType != ET_NPC && crosshairEnt->currentState.eType != ET_PLAYER)
	{
		return; // nothing to show...
	}

	if (crosshairEnt->currentState.eType == ET_NPC)
	{
		if (!crosshairEnt->npcClient || !crosshairEnt->npcClient->ghoul2Model)
			return;

		if ( !crosshairEnt->npcClient->infoValid )
			return;
	}

	if (crosshairEnt->currentState.number == cg.clientNum || crosshairEnt->currentState.number == cg.snap->ps.clientNum)
	{
		return;
	}

	if (!crosshairEnt->ghoul2)
	{
		return;
	}

	if (crosshairEnt->currentState.eFlags & EF_DEAD)
		return;

	if (crosshairEnt->playerState->pm_type == PM_DEAD)
		return;

	if (crosshairEnt->currentState.eType == ET_FREED)
		return;

	//if (crosshairEnt->currentState.health <= 0)
	//	return;

	if (crosshairEnt->currentState.number < MAX_CLIENTS)
	{
		ci = &cgs.clientinfo[crosshairEnt->currentState.number];
	}
	else
	{
		ci = crosshairEnt->npcClient;
	}

	if (!ci)
	{
		return;
	}
	
	if (!ci->infoValid)
	{
		return;
	}

	if (ci->team == FACTION_SPECTATOR)
	{
		return;
	}

	if (crosshairEnt->playerState->persistant[PERS_TEAM] == FACTION_SPECTATOR)
		return;

	if (crosshairEnt->playerState->fd.forcePowerMax <= 0) crosshairEnt->playerState->fd.forcePowerMax = 100;

	if (crosshairEnt->currentState.eType == ET_NPC)
	{
		//if (crosshairEnt->currentState.health <= 0) return;

		// Load the list on first check...
		Load_NPC_Names();

		//if (crosshairEnt->currentState.NPC_NAME_ID > 0)
		{// Was assigned a full name already! Yay!
			switch( crosshairEnt->currentState.NPC_class )
			{// UQ1: Supported Class Types...
			case CLASS_CIVILIAN:
			case CLASS_GENERAL_VENDOR:
			case CLASS_WEAPONS_VENDOR:
			case CLASS_ARMOR_VENDOR:
			case CLASS_SUPPLIES_VENDOR:
			case CLASS_FOOD_VENDOR:
			case CLASS_MEDICAL_VENDOR:
			case CLASS_GAMBLER_VENDOR:
			case CLASS_TRADE_VENDOR:
			case CLASS_ODDITIES_VENDOR:
			case CLASS_DRUG_VENDOR:
			case CLASS_TRAVELLING_VENDOR:
			case CLASS_LUKE:
			case CLASS_JEDI:
			case CLASS_PADAWAN:
			case CLASS_HK51:
			case CLASS_NATIVE:
			case CLASS_NATIVE_GUNNER:
			case CLASS_KYLE:
			case CLASS_JAN:
			case CLASS_MONMOTHA:			
			case CLASS_MORGANKATARN:
			case CLASS_TAVION:
			case CLASS_ALORA:
			case CLASS_INQUISITOR:
			case CLASS_REBORN:
			case CLASS_DESANN:
			case CLASS_BOBAFETT:
			case CLASS_COMMANDO:
			case CLASS_DEATHTROOPER:
			case CLASS_MERC:
			case CLASS_BARTENDER:
			case CLASS_BESPIN_COP:
			case CLASS_GALAK:
			case CLASS_GRAN:
			case CLASS_LANDO:			
			case CLASS_REBEL:
			case CLASS_REELO:
			case CLASS_MURJJ:
			case CLASS_PRISONER:
			case CLASS_RODIAN:
			case CLASS_TRANDOSHAN:
			case CLASS_UGNAUGHT:
			case CLASS_JAWA:
				sprintf(str1, "%s", NPC_NAME_LIST[crosshairEnt->currentState.NPC_NAME_ID].HumanNames);
				break;
			case CLASS_PURGETROOPER:
				sprintf(str1, "PT-%i", crosshairEnt->currentState.NPC_NAME_ID);	// EVIL. for a number of reasons --eez
				break;
			case CLASS_STORMTROOPER_ADVANCED:
				sprintf(str1, "TA-%i", crosshairEnt->currentState.NPC_NAME_ID);	// EVIL. for a number of reasons --eez
				break;
			case CLASS_STORMTROOPER_ATAT_PILOT:
			case CLASS_STORMTROOPER_ATST_PILOT:
				sprintf(str1, "TA-%i", crosshairEnt->currentState.NPC_NAME_ID);	// EVIL. for a number of reasons --eez
				break;
			case CLASS_STORMTROOPER:
				sprintf(str1, "TK-%i", crosshairEnt->currentState.NPC_NAME_ID);	// EVIL. for a number of reasons --eez
				break;
			case CLASS_SWAMPTROOPER:
				sprintf(str1, "TS-%i", crosshairEnt->currentState.NPC_NAME_ID);	// EVIL. for a number of reasons --eez
				break;
			case CLASS_IMPWORKER:
				sprintf(str1, "IW-%i", crosshairEnt->currentState.NPC_NAME_ID);	// EVIL. for a number of reasons --eez
				break;
			case CLASS_SHADOWTROOPER:
				sprintf(str1, "ST-%i", crosshairEnt->currentState.NPC_NAME_ID);	// EVIL. for a number of reasons --eez
				break;
			case CLASS_IMPERIAL:
				sprintf(str1, "Commander %s", NPC_NAME_LIST[crosshairEnt->currentState.NPC_NAME_ID].HumanNames);	// EVIL. for a number of reasons --eez
				break;
			case CLASS_ATST_OLD:				// technically droid...
			case CLASS_ATST:
				sprintf(str1, "AT-ST");
				break;
			case CLASS_ATAT:
				sprintf(str1, "AT-AT");
				break;
			case CLASS_ATPT:
				sprintf(str1, "AT-PT");
				break;
			case CLASS_CLAW:
				sprintf(str1, "Claw");
				break;
			case CLASS_FISH:
				sprintf(str1, "Sea Creature");
				break;
			case CLASS_FLIER2:
				sprintf(str1, "Flier");
				break;
			case CLASS_GLIDER:
				sprintf(str1, "Glider");
				break;
			case CLASS_HOWLER:
				sprintf(str1, "Howler");
				break;
			case CLASS_REEK:
				sprintf(str1, "Reek");
				break;
			case CLASS_NEXU:
				sprintf(str1, "Nexu");
				break;
			case CLASS_ACKLAY:
				sprintf(str1, "Acklay");
				break;
			case CLASS_LIZARD:
				sprintf(str1, "Lizard");
				break;
			case CLASS_MINEMONSTER:
				sprintf(str1, "Mine Monster");
				break;
			case CLASS_SWAMP:
				sprintf(str1, "Swamp Monster");
				break;
			case CLASS_RANCOR:
				sprintf(str1, "Rancor");
				break;
			case CLASS_WAMPA:
				sprintf(str1, "Wampa");
				break;
			case CLASS_K2SO:
				sprintf(str1, "K2-SO");
				break;
			case CLASS_R2D2:
			case CLASS_CIVILIAN_R2D2:
				sprintf(str1, "R2D2 Droid");
				break;
			case CLASS_R5D2:
			case CLASS_CIVILIAN_R5D2:
				sprintf(str1, "R5D2 Droid");
				break;
			case CLASS_PROTOCOL:
			case CLASS_CIVILIAN_PROTOCOL:
				sprintf(str1, "Protocol Droid");
				break;
			case CLASS_WEEQUAY:
			case CLASS_CIVILIAN_WEEQUAY:
				sprintf(str1, "Weequay");
				break;
			case CLASS_VEHICLE:
				return;
				break;
			default:
				//CG_Printf("NPC %i is not a civilian or vendor (class %i).\n", cent->currentState.number, cent->currentState.NPC_class);
				return;
				break;
			}
		}

		switch( crosshairEnt->currentState.NPC_class )
		{// UQ1: Supported Class Types...
		case CLASS_CIVILIAN:
		case CLASS_CIVILIAN_R2D2:
		case CLASS_CIVILIAN_R5D2:
		case CLASS_CIVILIAN_PROTOCOL:
		case CLASS_CIVILIAN_WEEQUAY:
			sprintf(str2, "< Civilian >");
			tclr[0] = 0.125f;
			tclr[1] = 0.125f;
			tclr[2] = 0.7f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.125f;
			tclr2[1] = 0.125f;
			tclr2[2] = 0.7f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_REBEL:
		case CLASS_JAN:
			sprintf(str2, "< Rebel >");
			tclr[0] = 0.125f;
			tclr[1] = 0.125f;
			tclr[2] = 0.7f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.125f;
			tclr2[1] = 0.125f;
			tclr2[2] = 0.7f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_NATIVE:
		case CLASS_NATIVE_GUNNER:
			sprintf(str2, "< Native >");
			tclr[0] = 0.125f;
			tclr[1] = 0.125f;
			tclr[2] = 0.7f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.125f;
			tclr2[1] = 0.125f;
			tclr2[2] = 0.7f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_JEDI:
		case CLASS_KYLE:
		case CLASS_LUKE:
		case CLASS_MONMOTHA:			
		case CLASS_MORGANKATARN:
			sprintf(str2, "< Jedi >");
			tclr[0] = 0.125f;
			tclr[1] = 0.325f;
			tclr[2] = 0.7f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.125f;
			tclr2[1] = 0.325f;
			tclr2[2] = 0.7f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_PADAWAN:
			sprintf(str2, "< Padawan >");
			tclr[0] = 0.125f;
			tclr[1] = 0.325f;
			tclr[2] = 0.7f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.125f;
			tclr2[1] = 0.325f;
			tclr2[2] = 0.7f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_GENERAL_VENDOR:
			sprintf(str2, "< General Vendor >");
			tclr[0] = 0.525f;
			tclr[1] = 0.525f;
			tclr[2] = 1.0f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.525f;
			tclr2[1] = 0.525f;
			tclr2[2] = 1.0f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_WEAPONS_VENDOR:
			sprintf(str2, "< Weapons Vendor >");
			tclr[0] = 0.525f;
			tclr[1] = 0.525f;
			tclr[2] = 1.0f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.525f;
			tclr2[1] = 0.525f;
			tclr2[2] = 1.0f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_ARMOR_VENDOR:
			sprintf(str2, "< Armor Vendor >");
			tclr[0] = 0.525f;
			tclr[1] = 0.525f;
			tclr[2] = 1.0f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.525f;
			tclr2[1] = 0.525f;
			tclr2[2] = 1.0f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_SUPPLIES_VENDOR:
			sprintf(str2, "< Supplies Vendor >");
			tclr[0] = 0.525f;
			tclr[1] = 0.525f;
			tclr[2] = 1.0f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.525f;
			tclr2[1] = 0.525f;
			tclr2[2] = 1.0f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_FOOD_VENDOR:
			sprintf(str2, "< Food Vendor >");
			tclr[0] = 0.525f;
			tclr[1] = 0.525f;
			tclr[2] = 1.0f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.525f;
			tclr2[1] = 0.525f;
			tclr2[2] = 1.0f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_MEDICAL_VENDOR:
			sprintf(str2, "< Medical Vendor >");
			tclr[0] = 0.525f;
			tclr[1] = 0.525f;
			tclr[2] = 1.0f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.525f;
			tclr2[1] = 0.525f;
			tclr2[2] = 1.0f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_GAMBLER_VENDOR:
			sprintf(str2, "< Gambling Vendor >");
			tclr[0] = 0.525f;
			tclr[1] = 0.525f;
			tclr[2] = 1.0f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.525f;
			tclr2[1] = 0.525f;
			tclr2[2] = 1.0f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_TRADE_VENDOR:
			sprintf(str2, "< Trade Vendor >");
			tclr[0] = 0.525f;
			tclr[1] = 0.525f;
			tclr[2] = 1.0f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.525f;
			tclr2[1] = 0.525f;
			tclr2[2] = 1.0f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_ODDITIES_VENDOR:
			sprintf(str2, "< Oddities Vendor >");
			tclr[0] = 0.525f;
			tclr[1] = 0.525f;
			tclr[2] = 1.0f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.525f;
			tclr2[1] = 0.525f;
			tclr2[2] = 1.0f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_DRUG_VENDOR:
			sprintf(str2, "< Drug Vendor >");
			tclr[0] = 0.525f;
			tclr[1] = 0.525f;
			tclr[2] = 1.0f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.525f;
			tclr2[1] = 0.525f;
			tclr2[2] = 1.0f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_TRAVELLING_VENDOR:
			sprintf(str2, "< Travelling Vendor >");
			tclr[0] = 0.525f;
			tclr[1] = 0.525f;
			tclr[2] = 1.0f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.525f;
			tclr2[1] = 0.525f;
			tclr2[2] = 1.0f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_MERC:
			sprintf(str2, "< Mercenary >");
			tclr[0] = 0.125f;
			tclr[1] = 1.0f;
			tclr[2] = 0.125f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.125f;
			tclr2[1] = 1.0f;
			tclr2[2] = 0.125f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_STORMTROOPER:
		case CLASS_STORMTROOPER_ADVANCED:
		case CLASS_STORMTROOPER_ATST_PILOT:
		case CLASS_STORMTROOPER_ATAT_PILOT:
		case CLASS_SWAMPTROOPER:
		case CLASS_IMPWORKER:
		case CLASS_IMPERIAL:
		case CLASS_SHADOWTROOPER:
		case CLASS_COMMANDO:
			sprintf(str2, "< Imperial >");
			tclr[0] = 1.0f;
			tclr[1] = 0.125f;
			tclr[2] = 0.125f;
			tclr[3] = 1.0f;

			tclr2[0] = 1.0f;
			tclr2[1] = 0.125f;
			tclr2[2] = 0.125f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_DEATHTROOPER:
			sprintf(str2, "< Death Trooper >");
			tclr[0] = 1.0f;
			tclr[1] = 0.125f;
			tclr[2] = 0.125f;
			tclr[3] = 1.0f;

			tclr2[0] = 1.0f;
			tclr2[1] = 0.125f;
			tclr2[2] = 0.125f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_HK51:
			sprintf(str2, "< HK-51 >");
			tclr[0] = 1.0f;
			tclr[1] = 0.125f;
			tclr[2] = 0.125f;
			tclr[3] = 1.0f;

			tclr2[0] = 1.0f;
			tclr2[1] = 0.125f;
			tclr2[2] = 0.125f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_K2SO:
			sprintf(str2, "< Security Droid >");
			tclr[0] = 1.0f;
			tclr[1] = 0.125f;
			tclr[2] = 0.125f;
			tclr[3] = 1.0f;

			tclr2[0] = 1.0f;
			tclr2[1] = 0.125f;
			tclr2[2] = 0.125f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_INQUISITOR:
			sprintf(str2, "< Sith Inquisitor >");
			tclr[0] = 1.0f;
			tclr[1] = 0.325f;
			tclr[2] = 0.125f;
			tclr[3] = 1.0f;

			tclr2[0] = 1.0f;
			tclr2[1] = 0.325f;
			tclr2[2] = 0.125f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_PURGETROOPER:
			sprintf(str2, "< Purge Trooper >");
			tclr[0] = 1.0f;
			tclr[1] = 0.325f;
			tclr[2] = 0.125f;
			tclr[3] = 1.0f;

			tclr2[0] = 1.0f;
			tclr2[1] = 0.325f;
			tclr2[2] = 0.125f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_TAVION:
		case CLASS_REBORN:
		case CLASS_DESANN:
		case CLASS_ALORA:
			sprintf(str2, "< Sith >");
			tclr[0] = 1.0f;
			tclr[1] = 0.325f;
			tclr[2] = 0.125f;
			tclr[3] = 1.0f;

			tclr2[0] = 1.0f;
			tclr2[1] = 0.325f;
			tclr2[2] = 0.125f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_BOBAFETT:
			sprintf(str2, "< Mandalorian >");
			tclr[0] = 1.0f;
			tclr[1] = 0.5f;
			tclr[2] = 0.0f;
			tclr[3] = 1.0f;

			tclr2[0] = 1.0f;
			tclr2[1] = 0.5f;
			tclr2[2] = 0.0f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_ATST_OLD:
		case CLASS_ATST:
		case CLASS_ATAT:
		case CLASS_ATPT:
			sprintf(str2, "< Vehicle >");
			tclr[0] = 1.0f;
			tclr[1] = 0.225f;
			tclr[2] = 0.125f;
			tclr[3] = 1.0f;

			tclr2[0] = 1.0f;
			tclr2[1] = 0.225f;
			tclr2[2] = 0.125f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_CLAW:
		case CLASS_FISH:
		case CLASS_FLIER2:
		case CLASS_GLIDER:
		case CLASS_HOWLER:
		case CLASS_REEK:
		case CLASS_NEXU:
		case CLASS_ACKLAY:
		case CLASS_LIZARD:
		case CLASS_MINEMONSTER:
		case CLASS_SWAMP:
		case CLASS_RANCOR:
		case CLASS_WAMPA:
			sprintf(str2, "< Wildlife >");
			tclr[0] = 1.0f;
			tclr[1] = 1.0f;
			tclr[2] = 0.125f;
			tclr[3] = 1.0f;

			tclr2[0] = 1.0f;
			tclr2[1] = 1.0f;
			tclr2[2] = 0.125f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_VEHICLE:
			sprintf(str2, "< Vehicle >");
			tclr[0] = 1.0f;
			tclr[1] = 0.125f;
			tclr[2] = 0.125f;
			tclr[3] = 1.0f;

			tclr2[0] = 1.0f;
			tclr2[1] = 0.125f;
			tclr2[2] = 0.125f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_BESPIN_COP:
		case CLASS_LANDO:
		case CLASS_PRISONER:
			sprintf(str2, "< Rebel >");
			tclr[0] = 0.125f;
			tclr[1] = 0.125f;
			tclr[2] = 0.7f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.125f;
			tclr2[1] = 0.125f;
			tclr2[2] = 0.7f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_RODIAN:
		case CLASS_WEEQUAY:
			sprintf(str2, "< Pirate >");
			tclr[0] = 0.5f;
			tclr[1] = 0.5f;
			tclr[2] = 0.5f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.5f;
			tclr2[1] = 0.5f;
			tclr2[2] = 0.5f;
			tclr2[3] = 1.0f;
			break;
		case CLASS_GALAK:
		case CLASS_GRAN:
		case CLASS_REELO:
		case CLASS_MURJJ:
		case CLASS_TRANDOSHAN:
		case CLASS_UGNAUGHT:
		case CLASS_BARTENDER:
		case CLASS_JAWA:
			if (crosshairEnt->playerState->persistant[PERS_TEAM] == NPCTEAM_ENEMY)
			{
				sprintf(str2, "< Thug >");
				tclr[0] = 0.5f;
				tclr[1] = 0.5f;
				tclr[2] = 0.125f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.5f;
				tclr2[1] = 0.5f;
				tclr2[2] = 0.125f;
				tclr2[3] = 1.0f;
			}
			else if (crosshairEnt->playerState->persistant[PERS_TEAM] == NPCTEAM_PLAYER)
			{
				sprintf(str2, "< Rebel >");
				tclr[0] = 0.125f;
				tclr[1] = 0.125f;
				tclr[2] = 0.7f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.125f;
				tclr2[1] = 0.125f;
				tclr2[2] = 0.7f;
				tclr2[3] = 1.0f;
			}
			else
			{
				sprintf(str2, "< Civilian >");
				tclr[0] = 0.7f;
				tclr[1] = 0.7f;
				tclr[2] = 0.125f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.7f;
				tclr2[1] = 0.7f;
				tclr2[2] = 0.125f;
				tclr2[3] = 1.0f;
			}
			break;

		default:
			//CG_Printf("NPC %i is not a civilian or vendor (class %i).\n", cent->currentState.number, cent->currentState.NPC_class);
			sprintf(str2, "");
			break;
		}
	}

	y = 5;

	sizeX = 180;
	sizeY = 38;

	boxX = 5 + sizeX + 5;
	boxXmid = (boxX + sizeY - 4) + ((sizeX - (boxX + sizeY - 180))/2);

	// Draw a transparent box background...
	CG_FillRect( boxX, y, sizeX, sizeY, uqBG );

	// Draw the border...
	//CG_DrawRect_FixedBorder( boxX, y, sizeX, sizeY, 1, uqBorder );

	y += 2;

	if (crosshairEnt->currentState.eType == ET_PLAYER)
	{
		clientInfo_t *ci = &cgs.clientinfo[currentCrosshairEntity];

		// FIXME: NPC icons??? And then the bars/name on the right of it like other MMOs???
		if ( ci->modelIcon )
		{
			CG_DrawPic( boxX + 2, y, sizeY-4, sizeY-4, ci->modelIcon );
		}

		sprintf(str1, "%s", ci->cleanname);
		
		if (cgs.clientinfo[currentCrosshairEntity].team == FACTION_EMPIRE)
		{
			sprintf(str2, "< Imperial >");
			tclr[0] = 0.75f;
			tclr[1] = 0.5f;
			tclr[2] = 0.125f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.75f;
			tclr2[1] = 0.5f;
			tclr2[2] = 0.125f;
			tclr2[3] = 1.0f;
		}
		else if (cgs.clientinfo[currentCrosshairEntity].team == FACTION_REBEL)
		{
			sprintf(str2, "< Rebel >");
			tclr[0] = 0.125f;
			tclr[1] = 0.125f;
			tclr[2] = 0.7f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.125f;
			tclr2[1] = 0.125f;
			tclr2[2] = 0.7f;
			tclr2[3] = 1.0f;
		}
		else if (cgs.clientinfo[currentCrosshairEntity].team == FACTION_MANDALORIAN)
		{
			sprintf(str2, "< Mandalorian >");
			tclr[0] = 1.0f;
			tclr[1] = 0.5f;
			tclr[2] = 0.0f;
			tclr[3] = 1.0f;

			tclr2[0] = 1.0f;
			tclr2[1] = 0.5f;
			tclr2[2] = 0.0f;
			tclr2[3] = 1.0f;
		}
		else if (cgs.clientinfo[currentCrosshairEntity].team == FACTION_MERC)
		{
			sprintf(str2, "< Mercenary >");
			tclr[0] = 0.125f;
			tclr[1] = 1.0f;
			tclr[2] = 0.125f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.125f;
			tclr2[1] = 1.0f;
			tclr2[2] = 0.125f;
			tclr2[3] = 1.0f;
		}
		else if (cgs.clientinfo[currentCrosshairEntity].team == FACTION_PIRATES)
		{
			sprintf(str2, "< Pirate >");
			tclr[0] = 0.5f;
			tclr[1] = 0.5f;
			tclr[2] = 0.5f;
			tclr[3] = 1.0f;

			tclr[0] = 0.5f;
			tclr[1] = 0.5f;
			tclr[2] = 0.5f;
			tclr[3] = 1.0f;
		}
		else if (cgs.clientinfo[currentCrosshairEntity].team == FACTION_WILDLIFE)
		{
			sprintf(str2, "< Wildlife >");
			tclr[0] = 1.0f;
			tclr[1] = 1.0f;
			tclr[2] = 0.0f;
			tclr[3] = 1.0f;

			tclr2[0] = 1.0f;
			tclr2[1] = 1.0f;
			tclr2[2] = 0.0f;
			tclr2[3] = 1.0f;
		}
		else
		{
			sprintf(str2, "< Player >");
			tclr[0] = 0.7f;
			tclr[1] = 0.7f;
			tclr[2] = 0.125f;
			tclr[3] = 1.0f;

			tclr2[0] = 0.7f;
			tclr2[1] = 0.7f;
			tclr2[2] = 0.125f;
			tclr2[3] = 1.0f;
		}
	}
	else
	{
		// FIXME: NPC icons??? And then the bars/name on the right of it like other MMOs???
		if ( crosshairEnt->npcClient->modelIcon )
		{
			CG_DrawPic( boxX + 2, y, sizeY-4, sizeY-4, crosshairEnt->npcClient->modelIcon );
		}
	}

	boxX += sizeY + 2;

	// Draw their name...
	CG_Text_Paint( boxXmid - (CG_Text_Width ( str1, 0.4f, FONT_SMALL ) * 0.5), y, 0.4f, tclr, str1, 0, 0, 0, FONT_SMALL );
	y += 10;

	// Draw their type...
	CG_Text_Paint( boxXmid - (CG_Text_Width ( str2, 0.35f, FONT_SMALL ) * 0.5), y, 0.35f, tclr2, str2, 0, 0, 0, FONT_SMALL );
	y += 12;

	// Draw their health bar...
	if (crosshairEnt->currentState.eType == ET_NPC)
	{
		if (crosshairEnt->currentState.health == 0 || crosshairEnt->currentState.maxhealth == 0)
			healthPerc = 1; // No health data yet. Assume 100%.
		else
			healthPerc = ((float)crosshairEnt->currentState.health / (float)crosshairEnt->currentState.maxhealth);
	}
	else
	{
		if (crosshairEnt->playerState->stats[STAT_HEALTH] == 0 || crosshairEnt->playerState->stats[STAT_MAX_HEALTH] == 0)
			healthPerc = 1; // No health data yet. Assume 100%.
		else
			healthPerc = ((float)crosshairEnt->playerState->stats[STAT_HEALTH] / (float)crosshairEnt->playerState->stats[STAT_MAX_HEALTH]);
	}

	CG_FilledBar( boxX + 2, y, sizeX-sizeY-4-6, 5, uqRed, NULL, NULL, healthPerc, flags );
	//CG_DrawRect_FixedBorder( boxX + 2, y, sizeX-sizeY-4-8, 5, 1, uqBorder );

	// Draw their armor bar...
	if (crosshairEnt->playerState->stats[STAT_ARMOR] == 0 || crosshairEnt->playerState->stats[STAT_MAX_HEALTH] == 0)
		armorPerc = 0; // No health data yet. Assume 0%.
	else
		armorPerc = ((float)crosshairEnt->playerState->stats[STAT_ARMOR] / (float)crosshairEnt->playerState->stats[STAT_MAX_HEALTH]);

	CG_FilledBar(boxX + 2, y, sizeX - sizeY - 4 - 6, 2, uqGreen, NULL, NULL, armorPerc, flags);
	// Write "XXX%/XXX%" over the bar in white...
	CG_Text_Paint( boxXmid - (CG_Text_Width ( va("%i/%i", (int)(armorPerc*100), (int)(healthPerc*100)), 0.35f, FONT_SMALL ) * 0.5), y-2, 0.35f, colorWhite, va("%i/%i", (int)(armorPerc*100), (int)(healthPerc*100)), 0, 0, 0, FONT_SMALL );


	y += 7;

	// Draw their force bar...
	if (!crosshairEnt->playerState || crosshairEnt->playerState->fd.forcePower == 0 || crosshairEnt->playerState->fd.forcePowerMax == 0)
		forcePerc = 1; // No force/power data yet. Assume 100%.
	else
		forcePerc = ((float)crosshairEnt->playerState->fd.forcePower / (float)crosshairEnt->playerState->fd.forcePowerMax);

	CG_FilledBar( boxX + 2, y, sizeX-sizeY-4-6, 5, uqBlue, NULL, NULL, forcePerc, flags );
	// Write "XXX%" over the bar in white...
	CG_Text_Paint( boxXmid - (CG_Text_Width ( va("%i", (int)(forcePerc*100)), 0.35f, FONT_SMALL ) * 0.5), y-2, 0.35f, colorWhite, va("%i", (int)(forcePerc*100)), 0, 0, 0, FONT_SMALL );
	//CG_DrawRect_FixedBorder( boxX + 2, y, sizeX-sizeY-4-6, 5, 1, uqBorder );

	y += 7;
}

static float CG_DrawEnemyInfo ( float y )
{
	float			size;
	int				clientNum;
	const char		*title;
	clientInfo_t	*ci;
	int				xOffset = 0;

	//CG_DrawMyStatus();

	if (!cg.snap)
	{
		return y;
	}

	if ( !cg_drawEnemyInfo.integer )
	{
		return y;
	}

	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 )
	{
		return y;
	}

	if (cgs.gametype == GT_POWERDUEL)
	{ //just get out of here then
		return y;
	}

#ifndef __MMO__
	if (cgs.gametype == GT_INSTANCE || cgs.gametype == GT_WARZONE)
#else //__MMO__
	if (cgs.gametype != GT_DUEL) // In MMO mode, we ALWAYS show target stats - ALL GAMETYPES except for GT_DUEL!
#endif //__MMO__
	{
		CG_DrawEnemyStatus();
		return y;
	}
	else if ( cgs.gametype == GT_JEDIMASTER )
	{
		//title = "Jedi Master";
		title = CG_GetStringEdString("MP_INGAME", "MASTERY7");
		clientNum = cgs.jediMaster;

		if ( clientNum < 0 )
		{
			//return y;
//			title = "Get Saber!";
			title = CG_GetStringEdString("MP_INGAME", "GET_SABER");


			size = ICON_SIZE * 1.25;
			y += 5;

			CG_DrawPic( 640 - size - 12 + xOffset, y, size, size, cgs.media.weaponIcons[WP_SABER] );

			y += size;

			/*
			CG_Text_Paint( 630 - CG_Text_Width ( ci->name, 0.7f, FONT_MEDIUM ), y, 0.7f, colorWhite, ci->name, 0, 0, 0, FONT_MEDIUM );
			y += 15;
			*/

			CG_Text_Paint( 630 - CG_Text_Width ( title, 0.7f, FONT_MEDIUM ) + xOffset, y, 0.7f, colorWhite, title, 0, 0, 0, FONT_MEDIUM );

			return y + BIGCHAR_HEIGHT + 2;
		}
	}
	else if ( cg.snap->ps.duelInProgress )
	{
//		title = "Dueling";
		title = CG_GetStringEdString("MP_INGAME", "DUELING");
		clientNum = cg.snap->ps.duelIndex;
	}
	else if ( cgs.gametype == GT_DUEL && cgs.clientinfo[cg.snap->ps.clientNum].team != FACTION_SPECTATOR)
	{
		title = CG_GetStringEdString("MP_INGAME", "DUELING");
		if (cg.snap->ps.clientNum == cgs.duelist1)
		{
			clientNum = cgs.duelist2; //if power duel, should actually draw both duelists 2 and 3 I guess
		}
		else if (cg.snap->ps.clientNum == cgs.duelist2)
		{
			clientNum = cgs.duelist1;
		}
		else if (cg.snap->ps.clientNum == cgs.duelist3)
		{
			clientNum = cgs.duelist1;
		}
		else
		{
			return y;
		}
	}
	else
	{
		/*
		title = "Attacker";
		clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];

		if ( clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum )
		{
			return y;
		}

		if ( cg.time - cg.attackerTime > ATTACKER_HEAD_TIME )
		{
			cg.attackerTime = 0;
			return y;
		}
		*/
		//As of current, we don't want to draw the attacker. Instead, draw whoever is in first place.
		if (cgs.duelWinner < 0 || cgs.duelWinner >= MAX_CLIENTS)
		{
			return y;
		}


		title = va("%s: %i",CG_GetStringEdString("MP_INGAME", "LEADER"), cgs.scores1);

		/*
		if (cgs.scores1 == 1)
		{
			title = va("%i kill", cgs.scores1);
		}
		else
		{
			title = va("%i kills", cgs.scores1);
		}
		*/
		clientNum = cgs.duelWinner;
	}

	if ( clientNum >= MAX_CLIENTS || !(&cgs.clientinfo[ clientNum ]) )
	{
		return y;
	}

	ci = &cgs.clientinfo[ clientNum ];

	size = ICON_SIZE * 1.25;
	y += 5;

	if ( ci->modelIcon )
	{
		CG_DrawPic( 640 - size - 5 + xOffset, y, size, size, ci->modelIcon );
	}

	y += size;

//	CG_Text_Paint( 630 - CG_Text_Width ( ci->name, 0.7f, FONT_MEDIUM ) + xOffset, y, 0.7f, colorWhite, ci->name, 0, 0, 0, FONT_MEDIUM );
	CG_Text_Paint( 630 - CG_Text_Width ( ci->name, 1.0f, FONT_SMALL2 ) + xOffset, y, 1.0f, colorWhite, ci->name, 0, 0, 0, FONT_SMALL2 );

	y += 15;
//	CG_Text_Paint( 630 - CG_Text_Width ( title, 0.7f, FONT_MEDIUM ) + xOffset, y, 0.7f, colorWhite, title, 0, 0, 0, FONT_MEDIUM );
	CG_Text_Paint( 630 - CG_Text_Width ( title, 1.0f, FONT_SMALL2 ) + xOffset, y, 1.0f, colorWhite, title, 0, 0, 0, FONT_SMALL2 );

	if ( (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL) && cgs.clientinfo[cg.snap->ps.clientNum].team != FACTION_SPECTATOR)
	{//also print their score
		char text[1024];
		y += 15;
		Com_sprintf(text, sizeof(text), "%i/%i", cgs.clientinfo[clientNum].score, cgs.fraglimit );
		CG_Text_Paint( 630 - CG_Text_Width ( text, 0.7f, FONT_MEDIUM ) + xOffset, y, 0.7f, colorWhite, text, 0, 0, 0, FONT_MEDIUM );
	}

// nmckenzie: DUEL_HEALTH - fixme - need checks and such here.  And this is coded to duelist 1 right now, which is wrongly.
	if ( cgs.showDuelHealths >= 2)
	{
		y += 15;
		if ( cgs.duelist1 == clientNum )
		{
			CG_DrawDuelistHealth ( 640 - size - 5 + xOffset, y, 64, 8, 1 );
		}
		else if ( cgs.duelist2 == clientNum )
		{
			CG_DrawDuelistHealth ( 640 - size - 5 + xOffset, y, 64, 8, 2 );
		}
	}

	return y + BIGCHAR_HEIGHT + 2;
}

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char		*s;
	int			w;
	int			xOffset = 0;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime,
		cg.latestSnapshotNum, cgs.serverCommandSequence );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( 635 - w + xOffset, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define	FPS_FRAMES	16

int CG_GetCurrentFPS( void )
{
	//int			w;
	static unsigned short previousTimes[FPS_FRAMES];
	static unsigned short index;
	static int	previous, lastupdate;
	int		t, i, fps, total;
	unsigned short frameTime;
	const int		xOffset = 0;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap->Milliseconds();
	frameTime = t - previous;
	previous = t;
	if (t - lastupdate > 50)	//don't sample faster than this
	{
		lastupdate = t;
		previousTimes[index % FPS_FRAMES] = frameTime;
		index++;
	}
	// average multiple frames together to smooth changes out a bit
	total = 0;
	for (i = 0; i < FPS_FRAMES; i++) {
		total += previousTimes[i];
	}
	if (!total) {
		total = 1;
	}
	
	fps = 1000 * FPS_FRAMES / total;
	
	cgs.currentFPS = fps;

	return fps;
}

static float CG_DrawFPS( float y ) {
	char			*s;
	const int		xOffset = 0;
	int				w;
	int				fps = cgs.currentFPS;

	s = va( "%ifps", fps );
	w = CG_DrawStrlen(s) * SMALLCHAR_WIDTH;//BIGCHAR_WIDTH;

	//CG_DrawBigString( 635 - w + xOffset, y + 2, s, 1.0F);
	CG_DrawSmallString(635 - w + xOffset, y + 2 /*+ SMALLCHAR_HEIGHT + 2*/, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

// nmckenzie: DUEL_HEALTH
#define MAX_HEALTH_FOR_IFACE	100
void CG_DrawHealthBarRough (float x, float y, int width, int height, float ratio, const float *color1, const float *color2)
{
	float midpoint, remainder;
	float color3[4] = {1, 0, 0, .7f};

	midpoint = width * ratio - 1;
	remainder = width - midpoint;
	color3[0] = color1[0] * 0.5f;

	assert(!(height%4));//this won't line up otherwise.
	CG_DrawRect(x + 1,			y + height/2-1,  midpoint,	1,	   height/4+1,  color1);	// creme-y filling.
	CG_DrawRect(x + midpoint,	y + height/2-1,  remainder,	1,	   height/4+1,  color3);	// used-up-ness.
	CG_DrawRect(x,				y,				 width,		height, 1,			color2);	// hard crispy shell
}

void CG_DrawDuelistHealth ( float x, float y, float w, float h, int duelist )
{
	float	duelHealthColor[4] = {1, 0, 0, 0.7f};
	float	healthSrc = 0.0f;
	float	ratio;

	if ( duelist == 1 )
	{
		healthSrc = cgs.duelist1health;
	}
	else if (duelist == 2 )
	{
		healthSrc = cgs.duelist2health;
	}

	ratio = healthSrc / MAX_HEALTH_FOR_IFACE;
	if ( ratio > 1.0f )
	{
		ratio = 1.0f;
	}
	if ( ratio < 0.0f )
	{
		ratio = 0.0f;
	}
	duelHealthColor[0] = (ratio * 0.2f) + 0.5f;

	CG_DrawHealthBarRough (x, y, w, h, ratio, duelHealthColor, colorTable[CT_WHITE]);	// new art for this?  I'm not crazy about how this looks.
}

/*
=====================
CG_DrawRadar
=====================
*/

float	cg_radarRange = 32768.0f;//16384.0f;// 2500.0f;

#define __RADAR_DRAW_TOWN__
#define __RADAR_DRAW_EVENTS__

#define RADAR_RADIUS			40
#define RADAR_X					(595 - RADAR_RADIUS)

#define RADAR_RADIUS_DISTEN		60
#define RADAR_DISTEN_X			(580 - RADAR_RADIUS_DISTEN)

#define RADAR_CHAT_DURATION		6000
static int radarLockSoundDebounceTime = 0;
static int impactSoundDebounceTime = 0;
#define	RADAR_MISSILE_RANGE					3000.0f
#define	RADAR_ASTEROID_RANGE				10000.0f
#define	RADAR_MIN_ASTEROID_SURF_WARN_DIST	1200.0f
//#define __DISABLE_RADAR_STUFF__	
float CG_DrawRadar(float y)
{
	vec4_t			color;
	vec4_t			teamColor;
	float			arrow_w;
	float			arrow_h;
#ifdef __DISABLE_RADAR_STUFF__
	clientInfo_t	*cl;
#endif //__DISABLE_RADAR_STUFF__
	clientInfo_t	*local;
	int				i;
	float			arrowBaseScale;
#ifdef __DISABLE_RADAR_STUFF__
	float			zScale;
#endif //__DISABLE_RADAR_STUFF__
	int				xOffset = 0;
	qhandle_t		directionImages[9] = { 0 };
	int				directionImagesPriority[9] = { 0 };
	int				distancePrintCounter = 0;


	if (!cg.snap)
	{
		return y;
	}

	// Make sure the radar should be showing
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 )
	{
		return y;
	}

	if ( (cg.predictedPlayerState.pm_flags & PMF_FOLLOW) || cg.predictedPlayerState.persistant[PERS_TEAM] == FACTION_SPECTATOR )
	{
		return y;
	}

	local = &cgs.clientinfo[ cg.snap->ps.clientNum ];
	if ( !local->infoValid )
	{
		return y;
	}


	// Draw the radar background image
	color[0] = color[1] = color[2] = 1.0f;
	color[3] = 0.6f;
	trap->R_SetColor ( color );
	CG_DrawPic(RADAR_X + xOffset, y, RADAR_RADIUS * 2, RADAR_RADIUS * 2, cgs.media.warzone_radarShader);

	//Always green for your own team.
	VectorCopy ( g_color_table[ColorIndex(COLOR_GREEN)], teamColor );
	teamColor[3] = 1.0f;

	// Draw all of the radar entities.  Draw them backwards so players are drawn last
	for ( i = cg.radarEntityCount -1 ; i >= 0 ; i-- )
	{
		vec3_t		dirLook;
		vec3_t		dirPlayer;
		float		angleLook;
		float		anglePlayer;
		float		angle;
		float		distance, actualDist;
		centity_t*	cent = &cg_entities[cg.radarEntities[i]];

		// Don't show friendly targets.
		if (cent->currentState.teamowner == cgs.clientinfo[cg.clientNum].team) continue;

		if (cent->currentState.eType == ET_PLAYER || cent->currentState.eType == ET_NPC)
		{
			if (cent->currentState.eFlags & EF_DEAD) continue;
			if (cent->playerState->pm_type == PM_DEAD) continue;
			if (cent->currentState.eType == ET_FREED) continue;

			if (cent->currentState.eType == ET_NPC)
			{
				if (!cent->npcClient || !cent->npcClient->ghoul2Model) continue;
				if (!cent->npcClient->infoValid) continue;
				if (!cent->ghoul2)  continue;
			}

			if (cent->currentState.eType == ET_PLAYER)
			{
				if (!cent->ghoul2) continue;
				if (cent->playerState->persistant[PERS_TEAM] == FACTION_SPECTATOR) continue;
			}

			clientInfo_t *ci = NULL;

			if (cent->currentState.number < MAX_CLIENTS)
			{
				ci = &cgs.clientinfo[cent->currentState.number];
			}
			else
			{
				ci = cent->npcClient;
			}

			if (!ci) continue;
			if (!ci->infoValid) continue;
			if (cent->currentState.eType == ET_PLAYER && ci->team == FACTION_SPECTATOR) continue;
		}

		// Get the distances first
		VectorSubtract ( cg.predictedPlayerState.origin, cent->lerpOrigin, dirPlayer );
		dirPlayer[2] = 0;
		actualDist = distance = VectorNormalize ( dirPlayer );

		if ( distance > cg_radarRange * 0.8f)
		{
			if ( (cent->currentState.eFlags & EF_RADAROBJECT)//still want to draw the direction
				|| ( cent->currentState.eType==ET_NPC//FIXME: draw last, with players...
					/*&& cent->currentState.NPC_class == CLASS_VEHICLE
					&& cent->currentState.speed > 0*/ ) )//always draw vehicles
			{
				distance = cg_radarRange*0.8f;
			}
			else
			{
				continue;
			}
		}

		distance  = distance / cg_radarRange;
		distance *= RADAR_RADIUS;

		AngleVectors ( cg.predictedPlayerState.viewangles, dirLook, NULL, NULL );

		dirLook[2] = 0;
		anglePlayer = atan2(dirPlayer[0],dirPlayer[1]);
		VectorNormalize ( dirLook );
		angleLook = atan2(dirLook[0],dirLook[1]);
		angle = angleLook - anglePlayer;

		{
#if 0// settings for indoor areas for later
#define RADAR_FAR_RANGE	800//600			// Adjust this one to adjust when the radar picks up a target AT ALL. Will show the outside tic.
#define RADAR_CLOSE_RANGE 500//300			// Adjust this for switching from the Far away indicator to the close-by one. The weak highlight, but full tic.
#define RADAR_REALLY_CLOSE_RANGE 150//100	// Adjust this one for the distance to draw the strong full tic. Needs to be smaller than the one above
#define RADAR_RIGHT_HERE 50					// Draw the center piece at this distance
#define ELEVATION_DIFFERENCE_LIMIT 25		// Change to adjust when it starts drawing the elevation versions of the "CLOSE" ones.
#endif
		//for outdoor big maps
#define RADAR_FAR_RANGE	32768//16384			// Adjust this one to adjust when the radar picks up a target AT ALL. Will show the outside tic.
#define RADAR_CLOSE_RANGE 6000//4096			// Adjust this for switching from the Far away indicator to the close-by one. The weak highlight, but full tic.
#define RADAR_REALLY_CLOSE_RANGE 1024			// Adjust this one for the distance to draw the strong full tic. Needs to be smaller than the one above
#define RADAR_RIGHT_HERE 256//100	//50		// Draw the center piece at this distance
#define ELEVATION_DIFFERENCE_LIMIT 25		// Change to adjust when it starts drawing the elevation versions of the "CLOSE" ones.
			// actualDist will be used to figure out which image to show. Or to skip if the enemy is too far away.
			if (actualDist <= RADAR_FAR_RANGE) // Skip if not within radar range.
			{
				float degAngle = fmod((RAD2DEG(angle) + 360.0f),360.0f);
				int directionValue = ((int)(floor((degAngle + 0.5f*45.0f) / 45.0f)*45.0f) / 45) % 8;
				//char* debugMessage = "Far";
				float elevationDifference = 0;
				int directionPriority = 1;
				int directionPriorityIndex = directionValue;

				// Default to the farthest away list of radar images.
				qhandle_t angleGFXHandle = cgs.media.warzone_radar_tic_far[directionValue];

				color[0] = color[1] = color[2] = 1.0f;
				color[3] = 1.0f;
				trap->R_SetColor(color);

				elevationDifference = fabs((cent->lerpOrigin[2] - cg.predictedPlayerState.origin[2]));
				if (actualDist > RADAR_RIGHT_HERE)
				{
					if (directionValue < 8)
					{
						if (elevationDifference > ELEVATION_DIFFERENCE_LIMIT && actualDist <= RADAR_CLOSE_RANGE)
						{
							angleGFXHandle = cgs.media.warzone_radar_tic_close_elevation[directionValue];
							//debugMessage = "Elevation";
							directionPriority = 2;
						}
						else if (actualDist <= RADAR_REALLY_CLOSE_RANGE)
						{
							angleGFXHandle = cgs.media.warzone_radar_tic_reallyclose[directionValue];
							//debugMessage = "ReallyClose";
							directionPriority = 4;
						}
						else if (actualDist <= RADAR_CLOSE_RANGE)
						{
							angleGFXHandle = cgs.media.warzone_radar_tic_close[directionValue];
							//debugMessage = "Close";
							directionPriority = 3;
						}
					}
				}
				else
				{
					// Print mid-circle piece.
					if (elevationDifference > ELEVATION_DIFFERENCE_LIMIT)
					{
						directionPriority = 1;
						angleGFXHandle = cgs.media.warzone_radar_midtpoint_glow_elevation;
					}
					else
					{
						directionPriority = 2;
						angleGFXHandle = cgs.media.warzone_radar_midtpoint_glow_0;
					}
					directionPriorityIndex = 8;
					//debugMessage = "OnTop";
				}

				if (directionImagesPriority[directionPriorityIndex] < directionPriority)
				{
					directionImagesPriority[directionPriorityIndex] = directionPriority;
					directionImages[directionPriorityIndex] = angleGFXHandle;
				}
				
				if (cg_turnondistenscalc.integer)
				{
					CG_Text_Paint(RADAR_DISTEN_X, y + 100 + (distancePrintCounter*12.5f), 1, color, va("#%d Distance: %f", distancePrintCounter, actualDist), 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_SMALL2);
					distancePrintCounter++;
				}
			}
		}

#ifdef __DISABLE_RADAR_STUFF__
		switch ( cent->currentState.eType )
		{
			default:
				{
					float  x;
					float  ly;
					qhandle_t shader;
					vec4_t    color;

					x = (float)RADAR_X + (float)RADAR_RADIUS + (float)sin (angle) * distance;
					ly = y + (float)RADAR_RADIUS + (float)cos (angle) * distance;

					arrowBaseScale = 9.0f;
					shader = 0;
					zScale = 1.0f;

					//we want to scale the thing up/down based on the relative Z (up/down) positioning
					if (cent->lerpOrigin[2] > cg.predictedPlayerState.origin[2])
					{ //higher, scale up (between 16 and 24)
						float dif = (cent->lerpOrigin[2] - cg.predictedPlayerState.origin[2]);

						//max out to 1.5x scale at 512 units above local player's height
						dif /= 1024.0f;
						if (dif > 0.5f)
						{
							dif = 0.5f;
						}
						zScale += dif;
					}
					else if (cent->lerpOrigin[2] < cg.predictedPlayerState.origin[2])
					{ //lower, scale down (between 16 and 8)
						float dif = (cg.predictedPlayerState.origin[2] - cent->lerpOrigin[2]);

						//half scale at 512 units below local player's height
						dif /= 1024.0f;
						if (dif > 0.5f)
						{
							dif = 0.5f;
						}
						zScale -= dif;
					}

					arrowBaseScale *= zScale;

					if (cent->currentState.brokenLimbs)
					{ //slightly misleading to use this value, but don't want to add more to entstate.
						//any ent with brokenLimbs non-0 and on radar is an objective ent.
						//brokenLimbs is literal team value.
						char objState[1024];
						int complete;

						//we only want to draw it if the objective for it is not complete.
						//frame represents objective num.
						trap->Cvar_VariableStringBuffer(va("team%i_objective%i", cent->currentState.brokenLimbs, cent->currentState.frame), objState, 1024);

						complete = atoi(objState);

						if (!complete)
						{

							// generic enemy index specifies a shader to use for the radar entity.
							if ( cent->currentState.genericenemyindex && cent->currentState.genericenemyindex < MAX_ICONS )
							{
								color[0] = color[1] = color[2] = color[3] = 1.0f;
								shader = cgs.gameIcons[cent->currentState.genericenemyindex];
							}
							else
							{
								if (cg.snap &&
									cent->currentState.brokenLimbs == cg.snap->ps.persistant[PERS_TEAM])
								{
									VectorCopy ( g_color_table[ColorIndex(COLOR_RED)], color );
								}
								else
								{
									VectorCopy ( g_color_table[ColorIndex(COLOR_GREEN)], color );
								}

								shader = cgs.media.siegeItemShader;
							}
						}
					}
					else
					{
						color[0] = color[1] = color[2] = color[3] = 1.0f;

						// generic enemy index specifies a shader to use for the radar entity.
						if ( cent->currentState.genericenemyindex )
						{
							shader = cgs.gameIcons[cent->currentState.genericenemyindex];
						}
						else
						{
							shader = cgs.media.siegeItemShader;
						}

					}

					if ( shader )
					{
						// Pulse the alpha if time2 is set.  time2 gets set when the entity takes pain
						if ( (cent->currentState.time2 && cg.time - cent->currentState.time2 < 5000) ||
							(cent->currentState.time2 == 0xFFFFFFFF) )
						{
							if ( (cg.time / 200) & 1 )
							{
								color[3] = 0.1f + 0.9f * (float) (cg.time % 200) / 200.0f;
							}
							else
							{
								color[3] = 1.0f - 0.9f * (float) (cg.time % 200) / 200.0f;
							}
						}

						trap->R_SetColor ( color );
						CG_DrawPic ( x - 4 + xOffset, ly - 4, arrowBaseScale, arrowBaseScale, shader );
					}
				}
				break;

			case ET_NPC://FIXME: draw last, with players...
				if ( cent->currentState.NPC_class == CLASS_VEHICLE
					&& cent->currentState.speed > 0 )
				{
					if ( cent->m_pVehicle && cent->m_pVehicle->m_pVehicleInfo->radarIconHandle )
					{
						float  x;
						float  ly;

						x = (float)RADAR_X + (float)RADAR_RADIUS + (float)sin (angle) * distance;
						ly = y + (float)RADAR_RADIUS + (float)cos (angle) * distance;

						arrowBaseScale = 9.0f;
						zScale = 1.0f;

						//we want to scale the thing up/down based on the relative Z (up/down) positioning
						if (cent->lerpOrigin[2] > cg.predictedPlayerState.origin[2])
						{ //higher, scale up (between 16 and 24)
							float dif = (cent->lerpOrigin[2] - cg.predictedPlayerState.origin[2]);

							//max out to 1.5x scale at 512 units above local player's height
							dif /= 4096.0f;
							if (dif > 0.5f)
							{
								dif = 0.5f;
							}
							zScale += dif;
						}
						else if (cent->lerpOrigin[2] < cg.predictedPlayerState.origin[2])
						{ //lower, scale down (between 16 and 8)
							float dif = (cg.predictedPlayerState.origin[2] - cent->lerpOrigin[2]);

							//half scale at 512 units below local player's height
							dif /= 4096.0f;
							if (dif > 0.5f)
							{
								dif = 0.5f;
							}
							zScale -= dif;
						}

						arrowBaseScale *= zScale;

						if ( cent->currentState.m_iVehicleNum //vehicle has a driver
							&& cgs.clientinfo[ cent->currentState.m_iVehicleNum-1 ].infoValid )
						{
							if ( cgs.clientinfo[ cent->currentState.m_iVehicleNum-1 ].team == local->team )
							{
								trap->R_SetColor ( teamColor );
							}
							else
							{
								trap->R_SetColor ( g_color_table[ColorIndex(COLOR_RED)] );
							}
						}
						else
						{
							trap->R_SetColor ( NULL );
						}
						CG_DrawPic ( x - 4 + xOffset, ly - 4, arrowBaseScale, arrowBaseScale, cent->m_pVehicle->m_pVehicleInfo->radarIconHandle );
					}
				}
				else 
				{// UQ1: Added - Normal NPCs...
					vec4_t color;

					cl = cent->npcClient;

					// not valid then dont draw it
					if ( !cl->infoValid )
					{
						continue;
					}

					VectorCopy4 ( teamColor, color );

					arrowBaseScale = 16.0f;
					zScale = 1.0f;

					// Pulse the radar icon after a voice message
					if ( cent->vChatTime + 2000 > cg.time )
					{
						float f = (cent->vChatTime + 2000 - cg.time) / 3000.0f;
						arrowBaseScale = 16.0f + 4.0f * f;
						color[0] = teamColor[0] + (1.0f - teamColor[0]) * f;
						color[1] = teamColor[1] + (1.0f - teamColor[1]) * f;
						color[2] = teamColor[2] + (1.0f - teamColor[2]) * f;
					}

					trap->R_SetColor ( color );

					//we want to scale the thing up/down based on the relative Z (up/down) positioning
					if (cent->lerpOrigin[2] > cg.predictedPlayerState.origin[2])
					{ //higher, scale up (between 16 and 32)
						float dif = (cent->lerpOrigin[2] - cg.predictedPlayerState.origin[2]);

						//max out to 2x scale at 1024 units above local player's height
						dif /= 1024.0f;
						if (dif > 1.0f)
						{
							dif = 1.0f;
						}
						zScale += dif;
					}
					else if (cent->lerpOrigin[2] < cg.predictedPlayerState.origin[2])
					{ //lower, scale down (between 16 and 8)
						float dif = (cg.predictedPlayerState.origin[2] - cent->lerpOrigin[2]);

						//half scale at 512 units below local player's height
						dif /= 1024.0f;
						if (dif > 0.5f)
						{
							dif = 0.5f;
						}
						zScale -= dif;
					}

					arrowBaseScale *= zScale;

					arrow_w = arrowBaseScale * RADAR_RADIUS / 128;
					arrow_h = arrowBaseScale * RADAR_RADIUS / 128;

					CG_DrawRotatePic2( RADAR_X + RADAR_RADIUS + sin (angle) * distance + xOffset,
						y + RADAR_RADIUS + cos (angle) * distance,
						arrow_w, arrow_h,
						(360 - cent->lerpAngles[YAW]) + cg.predictedPlayerState.viewangles[YAW], cgs.media.mAutomapPlayerIcon );
					break;
				}
				break; //maybe do something?

			case ET_MOVER:
				if ( cent->currentState.speed//the mover's size, actually
					&& actualDist < (cent->currentState.speed+RADAR_ASTEROID_RANGE)
					&& cg.predictedPlayerState.m_iVehicleNum )
				{//a mover that's close to me and I'm in a vehicle
					qboolean mayImpact = qfalse;
					float surfaceDist = (actualDist-cent->currentState.speed);
					if ( surfaceDist < 0.0f )
					{
						surfaceDist = 0.0f;
					}
					if ( surfaceDist < RADAR_MIN_ASTEROID_SURF_WARN_DIST )
					{//always warn!
						mayImpact = qtrue;
					}
					else
					{//not close enough to always warn, yet, so check its direction
						vec3_t	asteroidPos, myPos, moveDir;
						int		predictTime, timeStep = 500;
						float	newDist;
						for ( predictTime = timeStep; predictTime < 5000; predictTime+=timeStep )
						{
							//asteroid dir, speed, size, + my dir & speed...
							BG_EvaluateTrajectory( &cent->currentState.pos, cg.time+predictTime, asteroidPos );
							//FIXME: I don't think it's calcing "myPos" correctly
							AngleVectors( cg.predictedVehicleState.viewangles, moveDir, NULL, NULL );
							VectorMA( cg.predictedVehicleState.origin, cg.predictedVehicleState.speed*predictTime/1000.0f, moveDir, myPos );
							newDist = Distance( myPos, asteroidPos );
							if ( (newDist-cent->currentState.speed) <= RADAR_MIN_ASTEROID_SURF_WARN_DIST )//200.0f )
							{//heading for an impact within the next 5 seconds
								mayImpact = qtrue;
								break;
							}
						}
					}
					if ( mayImpact )
					{//possible collision
						vec4_t	asteroidColor = {0.5f,0.5f,0.5f,1.0f};
						float  x;
						float  ly;
						float asteroidScale = (cent->currentState.speed/2000.0f);//average asteroid radius?
						if ( actualDist > RADAR_ASTEROID_RANGE )
						{
							actualDist = RADAR_ASTEROID_RANGE;
						}
						distance = (actualDist/RADAR_ASTEROID_RANGE)*RADAR_RADIUS;

						x = (float)RADAR_X + (float)RADAR_RADIUS + (float)sin (angle) * distance;
						ly = y + (float)RADAR_RADIUS + (float)cos (angle) * distance;

						if ( asteroidScale > 3.0f )
						{
							asteroidScale = 3.0f;
						}
						else if ( asteroidScale < 0.2f )
						{
							asteroidScale = 0.2f;
						}
						arrowBaseScale = (9.0f*asteroidScale);
						if ( impactSoundDebounceTime < cg.time )
						{
							vec3_t	soundOrg;
							if ( surfaceDist > RADAR_ASTEROID_RANGE*0.66f )
							{
								impactSoundDebounceTime = cg.time + 1000;
							}
							else if ( surfaceDist > RADAR_ASTEROID_RANGE/3.0f )
							{
								impactSoundDebounceTime = cg.time + 400;
							}
							else
							{
								impactSoundDebounceTime = cg.time + 100;
							}
							VectorMA( cg.refdef.vieworg, -500.0f*(surfaceDist/RADAR_ASTEROID_RANGE), dirPlayer, soundOrg );
							trap->S_StartSound( soundOrg, ENTITYNUM_WORLD, CHAN_AUTO, trap->S_RegisterSound( "sound/vehicles/common/impactalarm.wav" ) );
						}
						//brighten it the closer it is
						if ( surfaceDist > RADAR_ASTEROID_RANGE*0.66f )
						{
							asteroidColor[0] = asteroidColor[1] = asteroidColor[2] = 0.7f;
						}
						else if ( surfaceDist > RADAR_ASTEROID_RANGE/3.0f )
						{
							asteroidColor[0] = asteroidColor[1] = asteroidColor[2] = 0.85f;
						}
						else
						{
							asteroidColor[0] = asteroidColor[1] = asteroidColor[2] = 1.0f;
						}
						//alpha out the longer it's been since it was considered dangerous
						if ( (cg.time-impactSoundDebounceTime) > 100 )
						{
							asteroidColor[3] = (float)((cg.time-impactSoundDebounceTime)-100)/900.0f;
						}

						trap->R_SetColor ( asteroidColor );
						CG_DrawPic ( x - 4 + xOffset, ly - 4, arrowBaseScale, arrowBaseScale, trap->R_RegisterShaderNoMip( "gfx/menus/radar/asteroid" ) );
					}
				}
				break;

			case ET_MISSILE:
				if ( cent->currentState.owner > MAX_CLIENTS //belongs to an NPC
					&& cg_entities[cent->currentState.owner].currentState.NPC_class == CLASS_VEHICLE )
				{//a rocket belonging to an NPC, FIXME: only tracking rockets!
					float  x;
					float  ly;

					x = (float)RADAR_X + (float)RADAR_RADIUS + (float)sin (angle) * distance;
					ly = y + (float)RADAR_RADIUS + (float)cos (angle) * distance;

					arrowBaseScale = 3.0f;
					if ( cg.predictedPlayerState.m_iVehicleNum )
					{//I'm in a vehicle
						//if it's targetting me, then play an alarm sound if I'm in a vehicle
						if ( cent->currentState.otherEntityNum == cg.predictedPlayerState.clientNum || cent->currentState.otherEntityNum == cg.predictedPlayerState.m_iVehicleNum )
						{
							if ( radarLockSoundDebounceTime < cg.time )
							{
								vec3_t	soundOrg;
								int		alarmSound;
								if ( actualDist > RADAR_MISSILE_RANGE * 0.66f )
								{
									radarLockSoundDebounceTime = cg.time + 1000;
									arrowBaseScale = 3.0f;
									alarmSound = trap->S_RegisterSound( "sound/vehicles/common/lockalarm1.wav" );
								}
								else if ( actualDist > RADAR_MISSILE_RANGE/3.0f )
								{
									radarLockSoundDebounceTime = cg.time + 500;
									arrowBaseScale = 6.0f;
									alarmSound = trap->S_RegisterSound( "sound/vehicles/common/lockalarm2.wav" );
								}
								else
								{
									radarLockSoundDebounceTime = cg.time + 250;
									arrowBaseScale = 9.0f;
									alarmSound = trap->S_RegisterSound( "sound/vehicles/common/lockalarm3.wav" );
								}
								if ( actualDist > RADAR_MISSILE_RANGE )
								{
									actualDist = RADAR_MISSILE_RANGE;
								}
								VectorMA( cg.refdef.vieworg, -500.0f*(actualDist/RADAR_MISSILE_RANGE), dirPlayer, soundOrg );
								trap->S_StartSound( soundOrg, ENTITYNUM_WORLD, CHAN_AUTO, alarmSound );
							}
						}
					}
					zScale = 1.0f;

					//we want to scale the thing up/down based on the relative Z (up/down) positioning
					if (cent->lerpOrigin[2] > cg.predictedPlayerState.origin[2])
					{ //higher, scale up (between 16 and 24)
						float dif = (cent->lerpOrigin[2] - cg.predictedPlayerState.origin[2]);

						//max out to 1.5x scale at 512 units above local player's height
						dif /= 1024.0f;
						if (dif > 0.5f)
						{
							dif = 0.5f;
						}
						zScale += dif;
					}
					else if (cent->lerpOrigin[2] < cg.predictedPlayerState.origin[2])
					{ //lower, scale down (between 16 and 8)
						float dif = (cg.predictedPlayerState.origin[2] - cent->lerpOrigin[2]);

						//half scale at 512 units below local player's height
						dif /= 1024.0f;
						if (dif > 0.5f)
						{
							dif = 0.5f;
						}
						zScale -= dif;
					}

					arrowBaseScale *= zScale;

					if ( cent->currentState.owner >= MAX_CLIENTS//missile owned by an NPC
						&& cg_entities[cent->currentState.owner].currentState.NPC_class == CLASS_VEHICLE//NPC is a vehicle
						&& cg_entities[cent->currentState.owner].currentState.m_iVehicleNum <= MAX_CLIENTS//Vehicle has a player driver
						&& cgs.clientinfo[cg_entities[cent->currentState.owner].currentState.m_iVehicleNum-1].infoValid ) //player driver is valid
					{
						cl = &cgs.clientinfo[cg_entities[cent->currentState.owner].currentState.m_iVehicleNum-1];
						if ( cl->team == local->team )
						{
							trap->R_SetColor ( teamColor );
						}
						else
						{
							trap->R_SetColor ( g_color_table[ColorIndex(COLOR_RED)] );
						}
					}
					else
					{
						trap->R_SetColor ( NULL );
					}
					CG_DrawPic ( x - 4 + xOffset, ly - 4, arrowBaseScale, arrowBaseScale, cgs.media.mAutomapRocketIcon );
				}
				break;

			case ET_PLAYER:
			{
				vec4_t color;

				cl = &cgs.clientinfo[ cent->currentState.number ];

				// not valid then dont draw it
				if ( !cl->infoValid )
				{
					continue;
				}

				VectorCopy4 ( teamColor, color );

				arrowBaseScale = 16.0f;
				zScale = 1.0f;

				// Pulse the radar icon after a voice message
				if ( cent->vChatTime + 2000 > cg.time )
				{
					float f = (cent->vChatTime + 2000 - cg.time) / 3000.0f;
					arrowBaseScale = 16.0f + 4.0f * f;
					color[0] = teamColor[0] + (1.0f - teamColor[0]) * f;
					color[1] = teamColor[1] + (1.0f - teamColor[1]) * f;
					color[2] = teamColor[2] + (1.0f - teamColor[2]) * f;
				}

				trap->R_SetColor ( color );

				//we want to scale the thing up/down based on the relative Z (up/down) positioning
				if (cent->lerpOrigin[2] > cg.predictedPlayerState.origin[2])
				{ //higher, scale up (between 16 and 32)
					float dif = (cent->lerpOrigin[2] - cg.predictedPlayerState.origin[2]);

					//max out to 2x scale at 1024 units above local player's height
					dif /= 1024.0f;
					if (dif > 1.0f)
					{
						dif = 1.0f;
					}
					zScale += dif;
				}
                else if (cent->lerpOrigin[2] < cg.predictedPlayerState.origin[2])
				{ //lower, scale down (between 16 and 8)
					float dif = (cg.predictedPlayerState.origin[2] - cent->lerpOrigin[2]);

					//half scale at 512 units below local player's height
					dif /= 1024.0f;
					if (dif > 0.5f)
					{
						dif = 0.5f;
					}
					zScale -= dif;
				}

				arrowBaseScale *= zScale;

				arrow_w = arrowBaseScale * RADAR_RADIUS / 128;
				arrow_h = arrowBaseScale * RADAR_RADIUS / 128;

				CG_DrawRotatePic2( RADAR_X + RADAR_RADIUS + sin (angle) * distance + xOffset,
								   y + RADAR_RADIUS + cos (angle) * distance,
								   arrow_w, arrow_h,
								   (360 - cent->lerpAngles[YAW]) + cg.predictedPlayerState.viewangles[YAW], cgs.media.mAutomapPlayerIcon );
				break;
			}
		}
#endif // __DISABLE_RADAR_STUFF__
	}

	// After having resolved the highest priority distances above, we can now draw the correct images
	for (i = 0; i < 9; ++i)
	{
		if (directionImagesPriority[i] != 0)
		{
			CG_DrawPic(RADAR_X + xOffset, y, RADAR_RADIUS * 2, RADAR_RADIUS * 2, directionImages[i]);
		}
	}

#ifdef __RADAR_DRAW_EVENTS__
	// Draw all of the radar entities.  Draw them backwards so players are drawn last
	for (i = 0; i < MAX_GENTITIES; i++)
	{
		vec3_t		dirLook;
		vec3_t		dirPlayer;
		float		angleLook;
		float		anglePlayer;
		float		angle;
		float		distance, actualDist, zScale;
		centity_t*	cent = &cg_entities[i];

		if (cent->currentState.eType != ET_SERVERMODEL) continue;
		if (cent->currentState.generic1 == 2) continue; // it's in hyperspace still

		// Get the distances first
		VectorSubtract(cg.predictedPlayerState.origin, cent->lerpOrigin, dirPlayer);
		dirPlayer[2] = 0;
		actualDist = distance = VectorNormalize(dirPlayer);

		//distance = cg_radarRange; // town is always at max distance on radar...
		//distance = Q_clamp(0.0, distance / 524288.0, 1.0);
		//distance = Q_clamp(0.0, distance / 1048576.0, 1.0);
		distance = Q_clamp(0.0, distance / cg_radarRange, 1.0);
		distance *= RADAR_RADIUS;

		AngleVectors(cg.predictedPlayerState.viewangles, dirLook, NULL, NULL);

		dirLook[2] = 0;
		anglePlayer = atan2(dirPlayer[0], dirPlayer[1]);
		VectorNormalize(dirLook);
		angleLook = atan2(dirLook[0], dirLook[1]);
		angle = angleLook - anglePlayer;


		float  x;
		float  ly;
		qhandle_t shader;

		x = (float)RADAR_X + (float)RADAR_RADIUS + (float)sin(angle) * distance;
		ly = y + (float)RADAR_RADIUS + (float)cos(angle) * distance;

		arrowBaseScale = 9.0f;
		shader = 0;
		zScale = 1.0f;

		//we want to scale the thing up/down based on the relative Z (up/down) positioning
		if (cent->lerpOrigin[2] > cg.predictedPlayerState.origin[2])
		{ //higher, scale up (between 16 and 24)
			float dif = (cent->lerpOrigin[2] - cg.predictedPlayerState.origin[2]);

			//max out to 1.5x scale at 512 units above local player's height
			dif /= 1024.0f;
			if (dif > 0.5f)
			{
				dif = 0.5f;
			}
			zScale += dif;
		}
		else if (cent->lerpOrigin[2] < cg.predictedPlayerState.origin[2])
		{ //lower, scale down (between 16 and 8)
			float dif = (cg.predictedPlayerState.origin[2] - cent->lerpOrigin[2]);

			//half scale at 512 units below local player's height
			dif /= 1024.0f;
			if (dif > 0.5f)
			{
				dif = 0.5f;
			}
			zScale -= dif;
		}

		arrowBaseScale *= zScale;

		switch (cent->currentState.teamowner)
		{
		case FACTION_EMPIRE:
			shader = trap->R_RegisterShader("gfx/radarIcons/factionIconImperial");
			break;
		case FACTION_REBEL:
			shader = trap->R_RegisterShader("gfx/radarIcons/factionIconRebel");
			break;
		case FACTION_MANDALORIAN:
			shader = trap->R_RegisterShader("gfx/radarIcons/factionIconMandalorian");
			break;
		case FACTION_MERC:
			shader = trap->R_RegisterShader("gfx/radarIcons/factionIconIvaxSyndicate");
			break;
		case FACTION_PIRATES:
			shader = trap->R_RegisterShader("gfx/radarIcons/factionIconKouhun");
			break;
		case FACTION_WILDLIFE:
		default:
			continue;
			break;
		}

		float centerIcon = (arrowBaseScale * 0.2);

		if (shader)
		{
			CG_DrawPic(((x - 4) - centerIcon) + xOffset, (ly - centerIcon) - 4, arrowBaseScale, arrowBaseScale, shader);
		}
	}
#endif //__RADAR_DRAW_EVENTS__

#ifdef __RADAR_DRAW_TOWN__
	extern vec3_t			TOWN_FORCEFIELD_ORIGIN;
	extern vec3_t			TOWN_FORCEFIELD_RADIUS;
	extern char				TOWN_MAP_ICON[256];

	if (TOWN_FORCEFIELD_ORIGIN[0] < 999990.0
		&& TOWN_FORCEFIELD_ORIGIN[1] < 999990.0
		&& TOWN_FORCEFIELD_ORIGIN[2] < 999990.0
		&& TOWN_FORCEFIELD_RADIUS[0] < 999990.0
		&& TOWN_FORCEFIELD_RADIUS[1] < 999990.0
		&& TOWN_FORCEFIELD_RADIUS[2] < 999990.0)
	{
		vec3_t		dirPlayer, dirLook;
		float		actualDist, distance, anglePlayer, angleLook, angle, zScale;

		// Get the distances first
		VectorSubtract(cg.predictedPlayerState.origin, TOWN_FORCEFIELD_ORIGIN, dirPlayer);
		dirPlayer[2] = 0;
		actualDist = distance = VectorNormalize(dirPlayer);

		//distance = 1.0; // town is always at max distance on radar...
		distance = Q_clamp(0.0, distance / cg_radarRange, 1.0);
		distance *= RADAR_RADIUS;

		AngleVectors(cg.predictedPlayerState.viewangles, dirLook, NULL, NULL);

		dirLook[2] = 0;
		anglePlayer = atan2(dirPlayer[0], dirPlayer[1]);
		VectorNormalize(dirLook);
		angleLook = atan2(dirLook[0], dirLook[1]);
		angle = angleLook - anglePlayer;

		float  x;
		float  ly;
		qhandle_t shader;

		x = (float)RADAR_X + (float)RADAR_RADIUS + (float)sin(angle) * distance;
		ly = y + (float)RADAR_RADIUS + (float)cos(angle) * distance;

		arrowBaseScale = 24.0;// cg_testvalue0.value;//9.0f;
		shader = 0;
		zScale = 1.0f;

		arrowBaseScale *= zScale;

		shader = trap->R_RegisterShader(TOWN_MAP_ICON);

		float centerIcon = (arrowBaseScale * 0.3);

		if (shader)
		{
			CG_DrawPic(((x - 4) - centerIcon) + xOffset, (ly - centerIcon) - 4, arrowBaseScale, arrowBaseScale, shader);
		}
	}
#endif //__RADAR_DRAW_TOWN__

	arrowBaseScale = 80.0f;

	arrow_w = arrowBaseScale * RADAR_RADIUS / 128;
	arrow_h = arrowBaseScale * RADAR_RADIUS / 128;

	trap->R_SetColor ( colorWhite );
	CG_DrawRotatePic2( RADAR_X + RADAR_RADIUS + xOffset, y + RADAR_RADIUS, arrow_w, arrow_h,
					   0, cgs.media.mAutomapPlayerIcon );

	return y+(RADAR_RADIUS*2);
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {
	char		*s;
	int			w;
	int			mins, seconds, tens;
	int			msec;
	int			xOffset = 0;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%i:%i%i", mins, tens, seconds );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( 635 - w + xOffset, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}


/*
=================
CG_DrawTeamOverlay
=================
*/
extern const char *CG_GetLocationString(const char *loc); //cg_main.c
static float CG_DrawTeamOverlay( float y, qboolean right, qboolean upper ) {
	int x, w, h, xx;
	int i, j, len;
	const char *p;
	vec4_t		hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	clientInfo_t *ci;
	gitem_t	*item;
	int ret_y, count;
	int xOffset = 0;

	if ( !cg_drawTeamOverlay.integer ) {
		return y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] != FACTION_EMPIRE 
		&& cg.snap->ps.persistant[PERS_TEAM] != FACTION_REBEL
		&& cg.snap->ps.persistant[PERS_TEAM] != FACTION_MANDALORIAN
		&& cg.snap->ps.persistant[PERS_TEAM] != FACTION_MERC
		&& cg.snap->ps.persistant[PERS_TEAM] != FACTION_PIRATES
		&& cg.snap->ps.persistant[PERS_TEAM] != FACTION_WILDLIFE) {
		return y; // Not on any team
	}

	plyrs = 0;

	//TODO: On basejka servers, we won't have valid teaminfo if we're spectating someone.
	//		Find a way to detect invalid info and return early?

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {
			plyrs++;
			len = CG_DrawStrlen(ci->name);
			if (len > pwidth)
				pwidth = len;
		}
	}

	if (!plyrs)
		return y;

	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_GetLocationString(CG_ConfigString(CS_LOCATIONS+i));
		if (p && *p) {
			len = CG_DrawStrlen(p);
			if (len > lwidth)
				lwidth = len;
		}
	}

	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;

	if ( right )
		x = 640 - w;
	else
		x = 0;

	h = plyrs * TINYCHAR_HEIGHT;

	if ( upper ) {
		ret_y = y + h;
	} else {
		y -= h;
		ret_y = y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == FACTION_EMPIRE ) {
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		hcolor[3] = 0.33f;
	} else if (cg.snap->ps.persistant[PERS_TEAM] == FACTION_MANDALORIAN) {
		hcolor[0] = 0.75f;
		hcolor[1] = 0.75f;
		hcolor[2] = 0.125f;
		hcolor[3] = 1.0f;
	} else if (cg.snap->ps.persistant[PERS_TEAM] == FACTION_MERC) {
		hcolor[0] = 0.125f;
		hcolor[1] = 0.75f;
		hcolor[2] = 0.125f;
		hcolor[3] = 1.0f;
	} else if (cg.snap->ps.persistant[PERS_TEAM] == FACTION_PIRATES) {
		hcolor[0] = 0.5f;
		hcolor[1] = 0.5f;
		hcolor[2] = 0.5f;
		hcolor[3] = 1.0f;
	} else if (cg.snap->ps.persistant[PERS_TEAM] == FACTION_WILDLIFE) {
		hcolor[0] = 0.9f;
		hcolor[1] = 0.9f;
		hcolor[2] = 0.125f;
		hcolor[3] = 1.0f;
	} else { // if ( cg.snap->ps.persistant[PERS_TEAM] == FACTION_REBEL )
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.33f;
	}

	trap->R_SetColor( hcolor );
	CG_DrawPic( x + xOffset, y, w, h, cgs.media.teamStatusBar );
	trap->R_SetColor( NULL );

	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {

			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;

			xx = x + TINYCHAR_WIDTH;

			CG_DrawStringExt( xx + xOffset, y,
				ci->name, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH);

			if (lwidth) {
				p = CG_GetLocationString(CG_ConfigString(CS_LOCATIONS+ci->location));
				if (!p || !*p)
					p = "unknown";
				len = CG_DrawStrlen(p);
				if (len > lwidth)
					len = lwidth;

//				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth +
//					((lwidth/2 - len/2) * TINYCHAR_WIDTH);
				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth;
				CG_DrawStringExt( xx + xOffset, y,
					p, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					TEAM_OVERLAY_MAXLOCATION_WIDTH);
			}

			CG_GetColorForHealth( ci->health, ci->armor, hcolor );

			Com_sprintf (st, sizeof(st), "%3i %3i", ci->health,	ci->armor);

			xx = x + TINYCHAR_WIDTH * 3 +
				TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;

			CG_DrawStringExt( xx + xOffset, y,
				st, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );

			// draw weapon icon
			xx += TINYCHAR_WIDTH * 3;

			if ( cg_weapons[ci->curWeapon].weaponIcon ) {
				CG_DrawPic( xx + xOffset, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					cg_weapons[ci->curWeapon].weaponIcon );
			} else {
				CG_DrawPic( xx + xOffset, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					cgs.media.deferShader );
			}

			// Draw powerup icons
			if (right) {
				xx = x;
			} else {
				xx = x + w - TINYCHAR_WIDTH;
			}
			for (j = 0; j <= PW_NUM_POWERUPS; j++) {
				if (ci->powerups & (1 << j)) {

					item = BG_FindItemForPowerup((powerup_t)j );

					if (item) {
						CG_DrawPic( xx + xOffset, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
						trap->R_RegisterShader( item->icon ) );
						if (right) {
							xx -= TINYCHAR_WIDTH;
						} else {
							xx += TINYCHAR_WIDTH;
						}
					}
				}
			}

			y += TINYCHAR_HEIGHT;
		}
	}

	return ret_y;
//#endif
}


static int CG_DrawPowerupIcons(int y)
{
	int j;
	int ico_size = 24;// 64;
	//int y = ico_size/2;
	int xOffset = 0;
	gitem_t	*item;

	trap->R_SetColor( NULL );

	if (!cg.snap)
	{
		return y;
	}

	y += 16;

	for (j = 0; j < PW_NUM_POWERUPS; j++)
	{
		if (cg.snap->ps.powerups[j] > cg.time)
		{
			int secondsleft = (cg.snap->ps.powerups[j] - cg.time)/1000;

			item = BG_FindItemForPowerup((powerup_t)j );

			if (item)
			{
				int icoShader = 0;
				if (cgs.gametype == GT_CTY && (j == PW_REDFLAG || j == PW_BLUEFLAG))
				{
					if (j == PW_REDFLAG)
					{
						icoShader = trap->R_RegisterShaderNoMip( "gfx/hud/mpi_rflag_ys" );
					}
					else
					{
						icoShader = trap->R_RegisterShaderNoMip( "gfx/hud/mpi_bflag_ys" );
					}
				}
				else
				{
					icoShader = trap->R_RegisterShader( item->icon );
				}

				CG_DrawPic( (640-(ico_size*1.1)) + xOffset, y, ico_size, ico_size, icoShader );

				y += ico_size;

				if (j != PW_REDFLAG && j != PW_BLUEFLAG && secondsleft < 999)
				{
					CG_DrawProportionalString((640-(ico_size*1.1))+(ico_size/2) + xOffset, y-8, va("%i", secondsleft), UI_CENTER | UI_BIGFONT | UI_DROPSHADOW, colorTable[CT_WHITE]);
				}

				y += (ico_size/3);
			}
		}
	}

	return y;
}


/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight( void ) {
	float	y = 0;

	trap->R_SetColor(colorTable[CT_WHITE]);

	if ( cg_drawFPS.integer ) {
		y = CG_DrawFPS( y );
	}
	else
	{// UQ1: Still move the radar down, so the icons at the top are not cut off...
		y += BIGCHAR_HEIGHT + 4;
	}

	if ( ( cgs.gametype >= GT_TEAM || cg.predictedPlayerState.m_iVehicleNum )
		&& cg_drawRadar.integer )
	{//draw Radar in Siege mode or when in a vehicle of any kind
		y = CG_DrawRadar ( y );
	}

	y = CG_DrawPowerupIcons(y);

	trap->R_SetColor(colorTable[CT_WHITE]);

	if (cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 1) {
		y = CG_DrawTeamOverlay(y, qtrue, qtrue);
	}

	if (cg_drawSnapshot.integer) {
		y = CG_DrawSnapshot(y);
	}

	y = CG_DrawEnemyInfo(y);

	//y = CG_DrawMiniScoreboard ( y ); // UQ1: Disabled...
	//y += 15; // UQ1: But still move down...

	CG_DrawMyStatus();

//[AUTOWAYPOINT]
	if (aw_percent_complete > 0)
		AIMod_AutoWaypoint_DrawProgress();
//[/AUTOWAYPOINT]
}

/*
===================
CG_DrawReward
===================
*/
#ifdef JK2AWARDS
static void CG_DrawReward( void ) {
	float	*color;
	int		i, count;
	float	x, y;
	char	buf[32];

	if ( !cg_drawRewards.integer ) {
		return;
	}

	color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
	if ( !color ) {
		if (cg.rewardStack > 0) {
			for(i = 0; i < cg.rewardStack; i++) {
				cg.rewardSound[i] = cg.rewardSound[i+1];
				cg.rewardShader[i] = cg.rewardShader[i+1];
				cg.rewardCount[i] = cg.rewardCount[i+1];
			}
			cg.rewardTime = cg.time;
			cg.rewardStack--;
			color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
			trap->S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
		} else {
			return;
		}
	}

	trap->R_SetColor( color );

	/*
	count = cg.rewardCount[0]/10;				// number of big rewards to draw

	if (count) {
		y = 4;
		x = 320 - count * ICON_SIZE;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, (ICON_SIZE*2)-4, (ICON_SIZE*2)-4, cg.rewardShader[0] );
			x += (ICON_SIZE*2);
		}
	}

	count = cg.rewardCount[0] - count*10;		// number of small rewards to draw
	*/

	if ( cg.rewardCount[0] >= 10 ) {
		y = 56;
		x = 320 - ICON_SIZE/2;
		CG_DrawPic( x, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0] );
		Com_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[0]);
		x = ( SCREEN_WIDTH - SMALLCHAR_WIDTH * CG_DrawStrlen( buf ) ) / 2;
		CG_DrawStringExt( x, y+ICON_SIZE, buf, color, qfalse, qtrue,
								SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
	}
	else {

		count = cg.rewardCount[0];

		y = 56;
		x = 320 - count * ICON_SIZE/2;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0] );
			x += ICON_SIZE;
		}
	}
	trap->R_SetColor( NULL );
}
#endif


/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define	LAG_SAMPLES		128


struct lagometer_s {
	int		frameSamples[LAG_SAMPLES];
	int		frameCount;
	int		snapshotFlags[LAG_SAMPLES];
	int		snapshotSamples[LAG_SAMPLES];
	int		snapshotCount;
} lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void ) {
	int			offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something different for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect( void ) {
#if 0 // UQ1: Disabled for now because it doesn't like high FPS... Not sure we need it anyway...
	float		x, y;
	int			cmdNum;
	usercmd_t	cmd;
	const char		*s;
	int			w;  // bk010215 - FIXME char message[1024];

	if (cg.mMapChange)
	{
		s = CG_GetStringEdString("MP_INGAME", "SERVER_CHANGING_MAPS");	// s = "Server Changing Maps";
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawBigString( 320 - w/2, 100, s, 1.0F);

		s = CG_GetStringEdString("MP_INGAME", "PLEASE_WAIT");	// s = "Please wait...";
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawBigString( 320 - w/2, 200, s, 1.0F);
		return;
	}

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap->GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap->GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		|| cmd.serverTime > cg.time ) {	// special check for map_restart // bk 0102165 - FIXME
		return;
	}

	// also add text in center of screen
	s = CG_GetStringEdString("MP_INGAME", "CONNECTION_INTERRUPTED"); // s = "Connection Interrupted"; // bk 010215 - FIXME
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 320 - w/2, 100, s, 1.0F);

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

	x = 640 - 48;
	y = 480 - 48;

	CG_DrawPic( x, y, 48, 48, trap->R_RegisterShader("gfx/2d/net.tga" ) );
#endif
}


#define	MAX_LAGOMETER_PING	900
#define	MAX_LAGOMETER_RANGE	300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( void ) {
	int		a, x, y, i;
	float	v;
	float	ax, ay, aw, ah, mid, range;
	int		color;
	float	vscale;

	if ( !cg_lagometer.integer || cgs.localServer ) {
		CG_DrawDisconnect();
		return;
	}

	//
	// draw the graph
	//
	x = 640 - 48;
	y = 480 - 144;

	trap->R_SetColor( NULL );
	CG_DrawPic( x, y, 48, 48, cgs.media.lagometerShader );

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.frameCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				trap->R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
			}
			if ( v > range ) {
				v = range;
			}
			trap->R_DrawStretchPic ( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				trap->R_SetColor( g_color_table[ColorIndex(COLOR_BLUE)] );
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			trap->R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;	// YELLOW for rate delay
					trap->R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					trap->R_SetColor( g_color_table[ColorIndex(COLOR_GREEN)] );
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			trap->R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;		// RED for dropped snapshots
				trap->R_SetColor( g_color_table[ColorIndex(COLOR_RED)] );
			}
			trap->R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap->R_SetColor( NULL );

	if ( cg_noPredict.integer || g_synchronousClients.integer ) {
		CG_DrawBigString( ax, ay, "snc", 1.0 );
	}

	CG_DrawDisconnect();
}

void CG_DrawSiegeMessage( const char *str, int objectiveScreen )
{
//	if (!( trap->Key_GetCatcher() & KEYCATCH_UI ))
	{
		trap->OpenUIMenu(UIMENU_CLOSEALL);
		trap->Cvar_Set("cg_siegeMessage", str);
		if (objectiveScreen)
		{
			trap->OpenUIMenu(UIMENU_SIEGEOBJECTIVES);
		}
		else
		{
			trap->OpenUIMenu(UIMENU_SIEGEMESSAGE);
		}
	}
}

void CG_DrawSiegeMessageNonMenu( const char *str )
{
	char	text[1024];
	if (str[0]=='@')
	{
		trap->SE_GetStringTextString(str+1, text, sizeof(text));
		str = text;
	}
	CG_CenterPrint(str, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, int y, int charWidth ) {
	char	*s;
	//[BugFix19]
	int		i = 0;
	//[/BugFix19]

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while( *s )
	{
		//[BugFix19]
		i++;
		if(i >= 50)
		{//maxed out a line of text, this will make the line spill over onto another line.
			i = 0;
			cg.centerPrintLines++;
		}
		else if (*s == '\n')
		//if (*s == '\n')
		//[/BugFix19]
			cg.centerPrintLines++;
		s++;
	}
}


/*
===================
CG_DrawCenterString
===================
*/
qboolean BG_IsWhiteSpace( char c )
{//this function simply checks to see if the given character is whitespace.
	if ( c == ' ' || c == '\n' || c == '\0' )
		return qtrue;

	return qfalse;
}
static void CG_DrawCenterString( void ) {
	char	*start;
	int		l;
	int		x, y, w;
	int h;
	float	*color;
	const float scale = 1.0; //0.5

	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centerTime.value );
	if ( !color ) {
		return;
	}

	trap->R_SetColor( color );

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 50; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		//[BugFix19]
		if(!BG_IsWhiteSpace(start[l]) && !BG_IsWhiteSpace(linebuffer[l-1]) )
		{//we might have cut a word off, attempt to find a spot where we won't cut words off at.
			int savedL = l;
			int counter = l-2;

			for(; counter >= 0; counter--)
			{
				if(BG_IsWhiteSpace(start[counter]))
				{//this location is whitespace, line break from this position
					linebuffer[counter] = 0;
					l = counter + 1;
					break;
				}
			}
			if(counter < 0)
			{//couldn't find a break in the text, just go ahead and cut off the word mid-word.
				l = savedL;
			}
		}
		//[/BugFix19]

		w = CG_Text_Width(linebuffer, scale, FONT_MEDIUM);
		h = CG_Text_Height(linebuffer, scale, FONT_MEDIUM);
		x = (SCREEN_WIDTH - w) / 2;
		CG_Text_Paint(x, y + h, scale, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);
		y += h + 6;

		//[BugFix19]
		//this method of advancing to new line from the start of the array was causing long lines without
		//new lines to be totally truncated.
		if(start[l] && start[l] == '\n')
		{//next char is a newline, advance past
			l++;
		}

		if ( !start[l] )
		{//end of string, we're done.
			break;
		}

		//advance pointer to the last character that we didn't read in.
		start = &start[l];
		//[/BugFix19]
	}

	trap->R_SetColor( NULL );
}



/*
================================================================================

CROSSHAIR

================================================================================
*/

#define HEALTH_WIDTH		50.0f
#define HEALTH_HEIGHT		5.0f

//see if we can draw some extra info on this guy based on our class
void CG_DrawSiegeInfo(centity_t *cent, float chX, float chY, float chW, float chH)
{
	siegeExtended_t *se = &cg_siegeExtendedData[cent->currentState.number];
	clientInfo_t *ci;
	const char *configstring, *v;
	siegeClass_t *siegeClass;
	vec4_t aColor;
	vec4_t cColor;
	float x;
	float y;
	float percent;
#ifndef __MMO__
	int ammoMax;
#endif //__MMO__

	assert(cent->currentState.number < MAX_CLIENTS);

	if (se->lastUpdated > cg.time)
	{ //strange, shouldn't happen
		return;
	}

	if ((cg.time - se->lastUpdated) > 10000)
	{ //if you haven't received a status update on this guy in 10 seconds, forget about it
		return;
	}

	if (cent->currentState.eFlags & EF_DEAD)
	{ //he's dead, don't display info on him
		return;
	}

	if (cent->currentState.weapon != se->weapon)
	{ //data is invalidated until it syncs back again
		return;
	}

	ci = &cgs.clientinfo[cent->currentState.number];
	if (ci->team != cg.predictedPlayerState.persistant[PERS_TEAM])
	{ //not on the same team
		return;
	}

	configstring = CG_ConfigString( cg.predictedPlayerState.clientNum + CS_PLAYERS );
	v = Info_ValueForKey( configstring, "siegeclass" );

	if (!v || !v[0])
	{ //don't have siege class in info?
		return;
	}

	siegeClass = BG_SiegeFindClassByName(v);

	if (!siegeClass)
	{ //invalid
		return;
	}

    if (!(siegeClass->classflags & (1<<CFL_STATVIEWER)))
	{ //doesn't really have the ability to see others' stats
		return;
	}

	x = chX+((chW/2)-(HEALTH_WIDTH/2));
	y = (chY+chH) + 8.0f;
	percent = ((float)se->health/(float)se->maxhealth)*HEALTH_WIDTH;

	//color of the bar
	aColor[0] = 0.0f;
	aColor[1] = 1.0f;
	aColor[2] = 0.0f;
	aColor[3] = 0.4f;

	//color of greyed out "missing health"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.4f;

	//draw the background (black)
	CG_DrawRect(x, y, HEALTH_WIDTH, HEALTH_HEIGHT, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f, y+1.0f, percent-1.0f, HEALTH_HEIGHT-1.0f, aColor);

	//then draw the other part greyed out
	CG_FillRect(x+percent, y+1.0f, HEALTH_WIDTH-percent-1.0f, HEALTH_HEIGHT-1.0f, cColor);


	{ //a weapon that takes no ammo, so show full
		percent = HEALTH_WIDTH;
	}

	//color of the bar
	aColor[0] = 1.0f;
	aColor[1] = 1.0f;
	aColor[2] = 0.0f;
	aColor[3] = 0.4f;

	//color of greyed out "missing health"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.4f;

	//draw the background (black)
	CG_DrawRect(x, y, HEALTH_WIDTH, HEALTH_HEIGHT, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f, y+1.0f, percent-1.0f, HEALTH_HEIGHT-1.0f, aColor);

	//then draw the other part greyed out
	CG_FillRect(x+percent, y+1.0f, HEALTH_WIDTH-percent-1.0f, HEALTH_HEIGHT-1.0f, cColor);
}

//draw the health bar based on current "health" and maxhealth
void CG_DrawHealthBar(centity_t *cent, float chX, float chY, float chW, float chH)
{
	vec4_t aColor;
	vec4_t cColor;
	float x = chX+((chW/2)-(HEALTH_WIDTH/2));
	float y = (chY+chH) + 8.0f;
	float percent = ((float)cent->currentState.health/(float)cent->currentState.maxhealth)*HEALTH_WIDTH;
	if (percent <= 0)
	{
		return;
	}

	//color of the bar
	if (!cent->currentState.teamowner || cgs.gametype < GT_TEAM)
	{ //not owned by a team or teamplay
		aColor[0] = 1.0f;
		aColor[1] = 1.0f;
		aColor[2] = 0.0f;
		aColor[3] = 0.4f;
	}
	else if (cent->currentState.teamowner == cg.predictedPlayerState.persistant[PERS_TEAM])
	{ //owned by my team
		aColor[0] = 0.0f;
		aColor[1] = 1.0f;
		aColor[2] = 0.0f;
		aColor[3] = 0.4f;
	}
	else
	{ //hostile
		aColor[0] = 1.0f;
		aColor[1] = 0.0f;
		aColor[2] = 0.0f;
		aColor[3] = 0.4f;
	}

	//color of greyed out "missing health"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.4f;

	//draw the background (black)
	CG_DrawRect(x, y, HEALTH_WIDTH, HEALTH_HEIGHT, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f, y+1.0f, percent-1.0f, HEALTH_HEIGHT-1.0f, aColor);

	//then draw the other part greyed out
	CG_FillRect(x+percent, y+1.0f, HEALTH_WIDTH-percent-1.0f, HEALTH_HEIGHT-1.0f, cColor);
}

//same routine (at least for now), draw progress of a "hack" or whatever
void CG_DrawHaqrBar(float chX, float chY, float chW, float chH)
{
	vec4_t aColor;
	vec4_t cColor;
	float x = chX+((chW/2)-(HEALTH_WIDTH/2));
	float y = (chY+chH) + 8.0f;
	float percent = (((float)cg.predictedPlayerState.hackingTime-(float)cg.time)/(float)cg.predictedPlayerState.hackingBaseTime)*HEALTH_WIDTH;

	if (percent > HEALTH_WIDTH ||
		percent < 1.0f)
	{
		return;
	}

	//color of the bar
	aColor[0] = 1.0f;
	aColor[1] = 1.0f;
	aColor[2] = 0.0f;
	aColor[3] = 0.4f;

	//color of greyed out done area
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, HEALTH_WIDTH, HEALTH_HEIGHT, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f, y+1.0f, percent-1.0f, HEALTH_HEIGHT-1.0f, aColor);

	//then draw the other part greyed out
	CG_FillRect(x+percent, y+1.0f, HEALTH_WIDTH-percent-1.0f, HEALTH_HEIGHT-1.0f, cColor);

	//draw the hacker icon
	CG_DrawPic(x, y-HEALTH_WIDTH, HEALTH_WIDTH, HEALTH_WIDTH, cgs.media.hackerIconShader);
}

//generic timing bar
int cg_genericTimerBar = 0;
int cg_genericTimerDur = 0;
vec4_t cg_genericTimerColor;
#define CGTIMERBAR_H			50.0f
#define CGTIMERBAR_W			10.0f
#define CGTIMERBAR_X			(SCREEN_WIDTH-CGTIMERBAR_W-120.0f)
#define CGTIMERBAR_Y			(SCREEN_HEIGHT-CGTIMERBAR_H-20.0f)
void CG_DrawGenericTimerBar(void)
{
	vec4_t aColor;
	vec4_t cColor;
	float x = CGTIMERBAR_X;
	float y = CGTIMERBAR_Y;
	float percent = ((float)(cg_genericTimerBar-cg.time)/(float)cg_genericTimerDur)*CGTIMERBAR_H;

	if (percent > CGTIMERBAR_H)
	{
		return;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	//color of the bar
	aColor[0] = cg_genericTimerColor[0];
	aColor[1] = cg_genericTimerColor[1];
	aColor[2] = cg_genericTimerColor[2];
	aColor[3] = cg_genericTimerColor[3];

	//color of greyed out "missing fuel"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, CGTIMERBAR_W, CGTIMERBAR_H, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f, y+1.0f+(CGTIMERBAR_H-percent), CGTIMERBAR_W-2.0f, CGTIMERBAR_H-1.0f-(CGTIMERBAR_H-percent), aColor);

	//then draw the other part greyed out
	CG_FillRect(x+1.0f, y+1.0f, CGTIMERBAR_W-2.0f, CGTIMERBAR_H-percent, cColor);
}

/*
=================
CG_DrawCrosshair
=================
*/

float cg_crosshairPrevPosX = 0;
float cg_crosshairPrevPosY = 0;
#define CRAZY_CROSSHAIR_MAX_ERROR_X	(100.0f*640.0f/480.0f)
#define CRAZY_CROSSHAIR_MAX_ERROR_Y	(100.0f)
void CG_LerpCrosshairPos( float *x, float *y )
{
	if ( cg_crosshairPrevPosX )
	{//blend from old pos
		float maxMove = 30.0f * ((float)cg.frametime/500.0f) * 640.0f/480.0f;
		float xDiff = (*x - cg_crosshairPrevPosX);
		if ( fabs(xDiff) > CRAZY_CROSSHAIR_MAX_ERROR_X )
		{
			maxMove = CRAZY_CROSSHAIR_MAX_ERROR_X;
		}
		if ( xDiff > maxMove )
		{
			*x = cg_crosshairPrevPosX + maxMove;
		}
		else if ( xDiff < -maxMove )
		{
			*x = cg_crosshairPrevPosX - maxMove;
		}
	}
	cg_crosshairPrevPosX = *x;

	if ( cg_crosshairPrevPosY )
	{//blend from old pos
		float maxMove = 30.0f * ((float)cg.frametime/500.0f);
		float yDiff = (*y - cg_crosshairPrevPosY);
		if ( fabs(yDiff) > CRAZY_CROSSHAIR_MAX_ERROR_Y )
		{
			maxMove = CRAZY_CROSSHAIR_MAX_ERROR_X;
		}
		if ( yDiff > maxMove )
		{
			*y = cg_crosshairPrevPosY + maxMove;
		}
		else if ( yDiff < -maxMove )
		{
			*y = cg_crosshairPrevPosY - maxMove;
		}
	}
	cg_crosshairPrevPosY = *y;
}

vec3_t cg_crosshairPos={0,0,0};
static void CG_DrawCrosshair( vec3_t worldPoint, int chEntValid ) {
	float		w, h;
	qhandle_t	hShader = 0;
	float		f;
	float		x, y;
	qboolean	corona = qfalse;
	vec4_t		ecolor = {0,0,0,0};
	centity_t	*crossEnt = NULL;
	float		chX, chY;

	if ( worldPoint )
	{
		VectorCopy( worldPoint, cg_crosshairPos );
	}

	if ( !cg_drawCrosshair.integer )
	{
		return;
	}

	if (cg.snap->ps.fallingToDeath)
	{
		return;
	}

	if ( cg.predictedPlayerState.scopeType != 0 )
	{//not while scoped
		return;
	}

	if ( cg_crosshairHealth.integer )
	{
		vec4_t		hcolor;

		CG_ColorForHealth( hcolor );
		trap->R_SetColor( hcolor );
	}
	else
	{
		//set color based on what kind of ent is under crosshair
		if ( cg.crosshairClientNum >= ENTITYNUM_WORLD )
		{
			trap->R_SetColor( NULL );
		}
		//rwwFIXMEFIXME: Write this a different way, it's getting a bit too sloppy looking
		else if (chEntValid &&
			(cg_entities[cg.crosshairClientNum].currentState.number < MAX_CLIENTS ||
			cg_entities[cg.crosshairClientNum].currentState.eType == ET_NPC ||
			cg_entities[cg.crosshairClientNum].currentState.shouldtarget ||
			cg_entities[cg.crosshairClientNum].currentState.health || //always show ents with health data under crosshair
			(cg_entities[cg.crosshairClientNum].currentState.eType == ET_MOVER && cg_entities[cg.crosshairClientNum].currentState.bolt1 && cg.predictedPlayerState.weapon == WP_SABER) ||
			(cg_entities[cg.crosshairClientNum].currentState.eType == ET_MOVER && cg_entities[cg.crosshairClientNum].currentState.teamowner)))
		{
			crossEnt = &cg_entities[cg.crosshairClientNum];

			if ( crossEnt->currentState.powerups & (1 <<PW_CLOAKED) )
			{ //don't show up for cloaked guys
				ecolor[0] = 1.0;//R
				ecolor[1] = 1.0;//G
				ecolor[2] = 1.0;//B
			}
			else if ( crossEnt->currentState.number < MAX_CLIENTS )
			{
				if (cgs.gametype >= GT_TEAM &&
					cgs.clientinfo[crossEnt->currentState.number].team == cgs.clientinfo[cg.snap->ps.clientNum].team )
				{
					//Allies are green
					ecolor[0] = 0.0;//R
					ecolor[1] = 1.0;//G
					ecolor[2] = 0.0;//B
				}
				else
				{
					if (cgs.gametype == GT_POWERDUEL &&
						cgs.clientinfo[crossEnt->currentState.number].duelTeam == cgs.clientinfo[cg.snap->ps.clientNum].duelTeam)
					{ //on the same duel team in powerduel, so he's a friend
						ecolor[0] = 0.0;//R
						ecolor[1] = 1.0;//G
						ecolor[2] = 0.0;//B
					}
					else
					{ //Enemies are red
						ecolor[0] = 1.0;//R
						ecolor[1] = 0.0;//G
						ecolor[2] = 0.0;//B
					}
				}

				if (cg.snap->ps.duelInProgress)
				{
					if (crossEnt->currentState.number != cg.snap->ps.duelIndex)
					{ //grey out crosshair for everyone but your foe if you're in a duel
						ecolor[0] = 0.4f;
						ecolor[1] = 0.4f;
						ecolor[2] = 0.4f;
					}
				}
				else if (crossEnt->currentState.bolt1)
				{ //this fellow is in a duel. We just checked if we were in a duel above, so
				  //this means we aren't and he is. Which of course means our crosshair greys out over him.
					ecolor[0] = 0.4f;
					ecolor[1] = 0.4f;
					ecolor[2] = 0.4f;
				}
			}
			else if (crossEnt->currentState.shouldtarget || crossEnt->currentState.eType == ET_NPC)
			{
				//VectorCopy( crossEnt->startRGBA, ecolor );
				if ( !ecolor[0] && !ecolor[1] && !ecolor[2] )
				{
					// We really don't want black, so set it to yellow
					ecolor[0] = 1.0f;//R
					ecolor[1] = 0.8f;//G
					ecolor[2] = 0.3f;//B
				}

				if (crossEnt->currentState.eType == ET_NPC)
				{
					int plTeam;
					if (cgs.gametype == GT_SIEGE)
					{
						plTeam = cg.predictedPlayerState.persistant[PERS_TEAM];
					}
					else
					{
						plTeam = NPCTEAM_PLAYER;
					}

					if ( crossEnt->currentState.powerups & (1 <<PW_CLOAKED) )
					{
						ecolor[0] = 1.0f;//R
						ecolor[1] = 1.0f;//G
						ecolor[2] = 1.0f;//B
					}
					else if ( !crossEnt->currentState.teamowner )
					{ //not on a team
						if (!crossEnt->currentState.teamowner ||
							crossEnt->currentState.NPC_class == CLASS_VEHICLE)
						{ //neutral
							if (crossEnt->currentState.owner < MAX_CLIENTS)
							{ //base color on who is pilotting this thing
								clientInfo_t *ci = &cgs.clientinfo[crossEnt->currentState.owner];

								if (cgs.gametype >= GT_TEAM && ci->team == cg.predictedPlayerState.persistant[PERS_TEAM])
								{ //friendly
									ecolor[0] = 0.0f;//R
									ecolor[1] = 1.0f;//G
									ecolor[2] = 0.0f;//B
								}
								else
								{ //hostile
									ecolor[0] = 1.0f;//R
									ecolor[1] = 0.0f;//G
									ecolor[2] = 0.0f;//B
								}
							}
							else
							{ //unmanned
								ecolor[0] = 1.0f;//R
								ecolor[1] = 1.0f;//G
								ecolor[2] = 0.0f;//B
							}
						}
						else
						{
							ecolor[0] = 1.0f;//R
							ecolor[1] = 0.0f;//G
							ecolor[2] = 0.0f;//B
						}
					}
					else if ( crossEnt->currentState.teamowner != plTeam )
					{// on enemy team
						ecolor[0] = 1.0f;//R
						ecolor[1] = 0.0f;//G
						ecolor[2] = 0.0f;//B
					}
					else
					{ //a friend
						ecolor[0] = 0.0f;//R
						ecolor[1] = 1.0f;//G
						ecolor[2] = 0.0f;//B
					}
				}
				else if ( crossEnt->currentState.teamowner == FACTION_EMPIRE
					|| crossEnt->currentState.teamowner == FACTION_REBEL
					|| crossEnt->currentState.teamowner == FACTION_MANDALORIAN
					|| crossEnt->currentState.teamowner == FACTION_MERC
					|| crossEnt->currentState.teamowner == FACTION_PIRATES
					|| crossEnt->currentState.teamowner == FACTION_WILDLIFE)
				{
					if (cgs.gametype < GT_TEAM)
					{ //not teamplay, just neutral then
						ecolor[0] = 1.0f;//R
						ecolor[1] = 1.0f;//G
						ecolor[2] = 0.0f;//B
					}
					else if ( crossEnt->currentState.teamowner != cgs.clientinfo[cg.snap->ps.clientNum].team )
					{ //on the enemy team
						ecolor[0] = 1.0f;//R
						ecolor[1] = 0.0f;//G
						ecolor[2] = 0.0f;//B
					}
					else
					{ //on my team
						ecolor[0] = 0.0f;//R
						ecolor[1] = 1.0f;//G
						ecolor[2] = 0.0f;//B
					}
				}
				else if (crossEnt->currentState.owner == cg.snap->ps.clientNum ||
					(cgs.gametype >= GT_TEAM && crossEnt->currentState.teamowner == cgs.clientinfo[cg.snap->ps.clientNum].team))
				{
					ecolor[0] = 0.0f;//R
					ecolor[1] = 1.0f;//G
					ecolor[2] = 0.0f;//B
				}
				else if (crossEnt->currentState.teamowner == 16 ||
					(cgs.gametype >= GT_TEAM && crossEnt->currentState.teamowner && crossEnt->currentState.teamowner != cgs.clientinfo[cg.snap->ps.clientNum].team))
				{
					ecolor[0] = 1.0f;//R
					ecolor[1] = 0.0f;//G
					ecolor[2] = 0.0f;//B
				}
			}
			else if (crossEnt->currentState.eType == ET_MOVER && crossEnt->currentState.bolt1 && cg.predictedPlayerState.weapon == WP_SABER)
			{ //can push/pull this mover. Only show it if we're using the saber.
				ecolor[0] = 0.2f;
				ecolor[1] = 0.5f;
				ecolor[2] = 1.0f;

				corona = qtrue;
			}
			else if (crossEnt->currentState.eType == ET_MOVER && crossEnt->currentState.teamowner)
			{ //a team owns this - if it's my team green, if not red, if not teamplay then yellow
				if (cgs.gametype < GT_TEAM)
				{
					ecolor[0] = 1.0f;//R
					ecolor[1] = 1.0f;//G
					ecolor[2] = 0.0f;//B
				}
                else if (cg.predictedPlayerState.persistant[PERS_TEAM] != crossEnt->currentState.teamowner)
				{ //not my team
					ecolor[0] = 1.0f;//R
					ecolor[1] = 0.0f;//G
					ecolor[2] = 0.0f;//B
				}
				else
				{ //my team
					ecolor[0] = 0.0f;//R
					ecolor[1] = 1.0f;//G
					ecolor[2] = 0.0f;//B
				}
			}
			else if (crossEnt->currentState.health)
			{
				if (!crossEnt->currentState.teamowner || cgs.gametype < GT_TEAM)
				{ //not owned by a team or teamplay
					ecolor[0] = 1.0f;
					ecolor[1] = 1.0f;
					ecolor[2] = 0.0f;
				}
				else if (crossEnt->currentState.teamowner == cg.predictedPlayerState.persistant[PERS_TEAM])
				{ //owned by my team
					ecolor[0] = 0.0f;
					ecolor[1] = 1.0f;
					ecolor[2] = 0.0f;
				}
				else
				{ //hostile
					ecolor[0] = 1.0f;
					ecolor[1] = 0.0f;
					ecolor[2] = 0.0f;
				}
			}

			ecolor[3] = 1.0f;

			trap->R_SetColor( ecolor );
		}
	}

	if ( cg.predictedPlayerState.m_iVehicleNum )
	{//I'm in a vehicle
		centity_t *vehCent = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
	    if ( vehCent
			&& vehCent->m_pVehicle
			&& vehCent->m_pVehicle->m_pVehicleInfo
			&& vehCent->m_pVehicle->m_pVehicleInfo->crosshairShaderHandle )
		{
			hShader = vehCent->m_pVehicle->m_pVehicleInfo->crosshairShaderHandle;
		}
		//bigger by default
		w = cg_crosshairSize.value*2.0f;
		h = w;
	}
	else
	{
		w = h = cg_crosshairSize.value;
	}

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
		h *= ( 1 + f );
	}

	if ( worldPoint && VectorLength( worldPoint ) )
	{
		vec3_t finalPoint;
		VectorSet(finalPoint, worldPoint[0], worldPoint[1], worldPoint[2] - (cg_thirdPersonVertOffset.value * 2.0));
		if ( !CG_WorldCoordToScreenCoordFloat(finalPoint, &x, &y ) )
		{//off screen, don't draw it
			return;
		}
		//CG_LerpCrosshairPos( &x, &y );
		x -= 320;
		y -= 240;
	}
	else
	{
		x = cg_crosshairX.integer;
		y = cg_crosshairY.integer;
	}

	if ( !hShader )
	{
		hShader = cgs.media.crosshairShader[ cg_drawCrosshair.integer % NUM_CROSSHAIRS ];
	}

	chX = x + cg.refdef.x + 0.5 * (640 - w);
	chY = y + cg.refdef.y + 0.5 * (480 - h);
	trap->R_DrawStretchPic( chX, chY, w, h, 0, 0, 1, 1, hShader );

	//draw a health bar directly under the crosshair if we're looking at something
	//that takes damage
	if (crossEnt &&
		crossEnt->currentState.maxhealth)
	{
		//CG_DrawHealthBar(crossEnt, chX, chY, w, h);
		chY += HEALTH_HEIGHT*2;
	}
	else if (crossEnt && crossEnt->currentState.number < MAX_CLIENTS)
	{
		if (cgs.gametype == GT_SIEGE)
		{
			CG_DrawSiegeInfo(crossEnt, chX, chY, w, h);
			chY += HEALTH_HEIGHT*4;
		}
		if (cg.crosshairVehNum && cg.time == cg.crosshairVehTime)
		{ //it was in the crosshair this frame
			centity_t *hisVeh = &cg_entities[cg.crosshairVehNum];

			if (hisVeh->currentState.eType == ET_NPC &&
				hisVeh->currentState.NPC_class == CLASS_VEHICLE &&
				hisVeh->currentState.maxhealth &&
				hisVeh->m_pVehicle)
			{ //draw the health for this vehicle
				CG_DrawHealthBar(hisVeh, chX, chY, w, h);
				chY += HEALTH_HEIGHT*2;
			}
		}
	}

	if (cg.predictedPlayerState.hackingTime)
	{ //hacking something
		CG_DrawHaqrBar(chX, chY, w, h);
	}

	if (cg_genericTimerBar > cg.time)
	{ //draw generic timing bar, can be used for whatever
		CG_DrawGenericTimerBar();
	}

	if ( corona ) // drawing extra bits
	{
		ecolor[3] = 0.5f;
		ecolor[0] = ecolor[1] = ecolor[2] = (1 - ecolor[3]) * ( sin( cg.time * 0.001f ) * 0.08f + 0.35f ); // don't draw full color
		ecolor[3] = 1.0f;

		trap->R_SetColor( ecolor );

		w *= 2.0f;
		h *= 2.0f;

		trap->R_DrawStretchPic( x + cg.refdef.x + 0.5 * (640 - w),
			y + cg.refdef.y + 0.5 * (480 - h),
			w, h, 0, 0, 1, 1, cgs.media.forceCoronaShader );
	}

	trap->R_SetColor( NULL );
}

qboolean CG_WorldCoordToScreenCoordFloat(vec3_t worldCoord, float *x, float *y)
{
    vec3_t trans;
    float xc, yc;
    float px, py;
    float z;

    px = tan(cg.refdef.fov_x * (M_PI / 360) );
    py = tan(cg.refdef.fov_y * (M_PI / 360) );

    VectorSubtract(worldCoord, cg.refdef.vieworg, trans);

    xc = 640 / 2.0;
    yc = 480 / 2.0;

	// z = how far is the object in our forward direction
    z = DotProduct(trans, cg.refdef.viewaxis[0]);
    if (z <= 0.001)
        return qfalse;

    *x = xc - DotProduct(trans, cg.refdef.viewaxis[1])*xc/(z*px);
    *y = yc - DotProduct(trans, cg.refdef.viewaxis[2])*yc/(z*py);

    return qtrue;
}

qboolean CG_WorldCoordToScreenCoord( vec3_t worldCoord, int *x, int *y ) {
	float xF, yF;

	if ( CG_WorldCoordToScreenCoordFloat( worldCoord, &xF, &yF ) ) {
		*x = (int)xF;
		*y = (int)yF;
		return qtrue;
	}

	return qfalse;
}

/*
====================
CG_SaberClashFlare
====================
*/
int cg_saberFlashTime = 0;
vec3_t cg_saberFlashPos = {0, 0, 0};
void CG_SaberClashFlare( void )
{
	int				t, maxTime = 150;
	vec3_t dif;
	vec3_t color;
	int x,y;
	float v, len;
	trace_t tr;

	t = cg.time - cg_saberFlashTime;

	if ( t <= 0 || t >= maxTime )
	{
		return;
	}

	// Don't do clashes for things that are behind us
	VectorSubtract( cg_saberFlashPos, cg.refdef.vieworg, dif );

	if ( DotProduct( dif, cg.refdef.viewaxis[0] ) < 0.2 )
	{
		return;
	}

	CG_Trace( &tr, cg.refdef.vieworg, NULL, NULL, cg_saberFlashPos, -1, CONTENTS_SOLID );

	if ( tr.fraction < 1.0f )
	{
		return;
	}

	len = VectorNormalize( dif );

	// clamp to a known range
	/*
	if ( len > 800 )
	{
		len = 800;
	}
	*/
	if ( len > 1200 )
	{
		return;
	}

	v = ( 1.0f - ((float)t / maxTime )) * ((1.0f - ( len / 800.0f )) * 2.0f + 0.35f);
	if (v < 0.001f)
	{
		v = 0.001f;
	}

	CG_WorldCoordToScreenCoord( cg_saberFlashPos, &x, &y );

	VectorSet( color, 0.8f, 0.8f, 0.8f );
	trap->R_SetColor( color );

	CG_DrawPic( x - ( v * 300 ), y - ( v * 300 ),
				v * 600, v * 600,
				trap->R_RegisterShader( "gfx/effects/saberFlare" ));
}

void CG_DottedLine( float x1, float y1, float x2, float y2, float dotSize, int numDots, vec4_t color, float alpha )
{
	float x, y, xDiff, yDiff, xStep, yStep;
	vec4_t colorRGBA;
	int dotNum = 0;

	VectorCopy4( color, colorRGBA );
	colorRGBA[3] = alpha;

	trap->R_SetColor( colorRGBA );

	xDiff = x2-x1;
	yDiff = y2-y1;
	xStep = xDiff/(float)numDots;
	yStep = yDiff/(float)numDots;

	for ( dotNum = 0; dotNum < numDots; dotNum++ )
	{
		x = x1 + (xStep*dotNum) - (dotSize*0.5f);
		y = y1 + (yStep*dotNum) - (dotSize*0.5f);

		CG_DrawPic( x, y, dotSize, dotSize, cgs.media.whiteShader );
	}
}

void CG_BracketEntity( centity_t *cent, float radius )
{
	trace_t tr;
	vec3_t dif;
	float	len, size, lineLength, lineWidth;
	float	x,	y;
	clientInfo_t *local;
	qboolean isEnemy = qfalse;

	VectorSubtract( cent->lerpOrigin, cg.refdef.vieworg, dif );
	len = VectorNormalize( dif );

	if ( cg.crosshairClientNum != cent->currentState.clientNum
		&& (!cg.snap||cg.snap->ps.rocketLockIndex!= cent->currentState.clientNum) )
	{//if they're the entity you're locking onto or under your crosshair, always draw bracket
		//Hmm... for now, if they're closer than 2000, don't bracket?
		if ( len < 2000.0f )
		{
			return;
		}

		CG_Trace( &tr, cg.refdef.vieworg, NULL, NULL, cent->lerpOrigin, -1, CONTENTS_OPAQUE );

		//don't bracket if can't see them
		if ( tr.fraction < 1.0f )
		{
			return;
		}
	}

	if ( !CG_WorldCoordToScreenCoordFloat(cent->lerpOrigin, &x, &y) )
	{//off-screen, don't draw it
		return;
	}

	//just to see if it's centered
	//CG_DrawPic( x-2, y-2, 4, 4, cgs.media.whiteShader );

	local = &cgs.clientinfo[cg.snap->ps.clientNum];
	if ( cent->currentState.m_iVehicleNum //vehicle has a driver
		&& (cent->currentState.m_iVehicleNum-1) < MAX_CLIENTS
		&& cgs.clientinfo[ cent->currentState.m_iVehicleNum-1 ].infoValid )
	{
		if ( cgs.gametype < GT_TEAM )
		{//ffa?
			isEnemy = qtrue;
			trap->R_SetColor ( g_color_table[ColorIndex(COLOR_RED)] );
		}
		else if ( cgs.clientinfo[ cent->currentState.m_iVehicleNum-1 ].team == local->team )
		{
			trap->R_SetColor ( g_color_table[ColorIndex(COLOR_GREEN)] );
		}
		else
		{
			isEnemy = qtrue;
			trap->R_SetColor ( g_color_table[ColorIndex(COLOR_RED)] );
		}
	}
	else if ( cent->currentState.teamowner )
	{
		if ( cgs.gametype < GT_TEAM )
		{//ffa?
			isEnemy = qtrue;
			trap->R_SetColor ( g_color_table[ColorIndex(COLOR_RED)] );
		}
		else if ( cent->currentState.teamowner != cg.predictedPlayerState.persistant[PERS_TEAM] )
		{// on enemy team
			isEnemy = qtrue;
			trap->R_SetColor ( g_color_table[ColorIndex(COLOR_RED)] );
		}
		else
		{ //a friend
			trap->R_SetColor ( g_color_table[ColorIndex(COLOR_GREEN)] );
		}
	}
	else
	{//FIXME: if we want to ever bracket anything besides vehicles (like siege objectives we want to blow up), we should handle the coloring here
		trap->R_SetColor ( NULL );
	}

	if ( len <= 1.0f )
	{//super-close, max out at 400 times radius (which is HUGE)
		size = radius*400.0f;
	}
	else
	{//scale by dist
		size = radius*(400.0f/len);
	}

	if ( size < 1.0f )
	{
		size = 1.0f;
	}

	//length scales with dist
	lineLength = (size*0.1f);
	if ( lineLength < 0.5f )
	{//always visible
		lineLength = 0.5f;
	}
	//always visible width
	lineWidth = 1.0f;

	x -= (size*0.5f);
	y -= (size*0.5f);

	/*
	if ( x >= 0 && x <= 640
		&& y >= 0 && y <= 480 )
	*/
	{//brackets would be drawn on the screen, so draw them
	//upper left corner
		//horz
        CG_DrawPic( x, y, lineLength, lineWidth, cgs.media.whiteShader );
		//vert
        CG_DrawPic( x, y, lineWidth, lineLength, cgs.media.whiteShader );
	//upper right corner
		//horz
        CG_DrawPic( x+size-lineLength, y, lineLength, lineWidth, cgs.media.whiteShader );
		//vert
        CG_DrawPic( x+size-lineWidth, y, lineWidth, lineLength, cgs.media.whiteShader );
	//lower left corner
		//horz
        CG_DrawPic( x, y+size-lineWidth, lineLength, lineWidth, cgs.media.whiteShader );
		//vert
        CG_DrawPic( x, y+size-lineLength, lineWidth, lineLength, cgs.media.whiteShader );
	//lower right corner
		//horz
        CG_DrawPic( x+size-lineLength, y+size-lineWidth, lineLength, lineWidth, cgs.media.whiteShader );
		//vert
        CG_DrawPic( x+size-lineWidth, y+size-lineLength, lineWidth, lineLength, cgs.media.whiteShader );
	}
	//Lead Indicator...
	if ( cg_drawVehLeadIndicator.integer )
	{//draw the lead indicator
		if ( isEnemy )
		{//an enemy object
			if ( cent->currentState.NPC_class == CLASS_VEHICLE )
			{//enemy vehicle
				if ( !VectorCompare( cent->currentState.pos.trDelta, vec3_origin ) )
				{//enemy vehicle is moving
					if ( cg.predictedPlayerState.m_iVehicleNum )
					{//I'm in a vehicle
						centity_t		*veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
						if ( veh //vehicle cent
							&& veh->m_pVehicle//vehicle
							&& veh->m_pVehicle->m_pVehicleInfo//vehicle stats
							&& veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID > VEH_WEAPON_BASE )//valid vehicle weapon
						{
							vehWeaponInfo_t *vehWeapon = &g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID];
							if ( vehWeapon
								&& vehWeapon->bIsProjectile//primary weapon's shot is a projectile
								&& !vehWeapon->bHasGravity//primary weapon's shot is not affected by gravity
								&& !vehWeapon->fHoming//primary weapon's shot is not homing
								&& vehWeapon->fSpeed )//primary weapon's shot has speed
							{//our primary weapon's projectile has a speed
								vec3_t vehDiff, vehLeadPos;
								float vehDist, eta;
								float leadX, leadY;

								VectorSubtract( cent->lerpOrigin, cg.predictedVehicleState.origin, vehDiff );
								vehDist = VectorNormalize( vehDiff );
								eta = (vehDist/vehWeapon->fSpeed);//how many seconds it would take for my primary weapon's projectile to get from my ship to theirs
								//now extrapolate their position that number of seconds into the future based on their velocity
								VectorMA( cent->lerpOrigin, eta, cent->currentState.pos.trDelta, vehLeadPos );
								//now we have where we should be aiming at, project that onto the screen at a 2D co-ord
								if ( !CG_WorldCoordToScreenCoordFloat(cent->lerpOrigin, &x, &y) )
								{//off-screen, don't draw it
									return;
								}
								if ( !CG_WorldCoordToScreenCoordFloat(vehLeadPos, &leadX, &leadY) )
								{//off-screen, don't draw it
									//just draw the line
									CG_DottedLine( x, y, leadX, leadY, 1, 10, g_color_table[ColorIndex(COLOR_RED)], 0.5f );
									return;
								}
								//draw a line from the ship's cur pos to the lead pos
								CG_DottedLine( x, y, leadX, leadY, 1, 10, g_color_table[ColorIndex(COLOR_RED)], 0.5f );
								//now draw the lead indicator
								trap->R_SetColor ( g_color_table[ColorIndex(COLOR_RED)] );
								CG_DrawPic( leadX-8, leadY-8, 16, 16, trap->R_RegisterShader( "gfx/menus/radar/lead" ) );
							}
						}
					}
				}
			}
		}
	}
}

qboolean CG_InFighter( void )
{
	if ( cg.predictedPlayerState.m_iVehicleNum )
	{//I'm in a vehicle
		centity_t *vehCent = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
	    if ( vehCent
			&& vehCent->m_pVehicle
			&& vehCent->m_pVehicle->m_pVehicleInfo
			&& vehCent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER )
		{//I'm in a fighter
			return qtrue;
		}
	}
	return qfalse;
}

qboolean CG_InATST( void )
{
	if ( cg.predictedPlayerState.m_iVehicleNum )
	{//I'm in a vehicle
		centity_t *vehCent = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
	    if ( vehCent
			&& vehCent->m_pVehicle
			&& vehCent->m_pVehicle->m_pVehicleInfo
			&& vehCent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER )
		{//I'm in an atst
			return qtrue;
		}
	}
	return qfalse;
}

void CG_DrawBracketedEntities( void )
{
	int i;
	for ( i = 0; i < cg.bracketedEntityCount; i++ )
	{
		centity_t *cent = &cg_entities[cg.bracketedEntities[i]];
		CG_BracketEntity( cent, CG_RadiusForCent( cent ) );
	}
}

//--------------------------------------------------------------
static void CG_DrawHolocronIcons(void)
//--------------------------------------------------------------
{
	int icon_size = 40;
	int i = 0;
	int startx = 10;
	int starty = 10;//SCREEN_HEIGHT - icon_size*3;

	int endx = icon_size;
	int endy = icon_size;

	if (cg.snap->ps.scopeType)
	{ //don't display over zoom mask
		return;
	}

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == FACTION_SPECTATOR)
	{
		return;
	}

	while (i < NUM_FORCE_POWERS)
	{
		if (cg.snap->ps.holocronBits & (1 << forcePowerSorted[i]))
		{
			CG_DrawPic( startx, starty, endx, endy, cgs.media.forcePowerIcons[forcePowerSorted[i]]);
			starty += (icon_size+2); //+2 for spacing
			if ((starty+icon_size) >= SCREEN_HEIGHT-80)
			{
				starty = 10;//SCREEN_HEIGHT - icon_size*3;
				startx += (icon_size+2);
			}
		}

		i++;
	}
}

static qboolean CG_IsDurationPower(int power)
{
	if (power == FP_HEAL ||
		power == FP_SPEED ||
		power == FP_TELEPATHY ||
		power == FP_RAGE ||
		power == FP_PROTECT ||
		power == FP_ABSORB ||
		power == FP_SEE)
	{
		return qtrue;
	}

	return qfalse;
}

//--------------------------------------------------------------
static void CG_DrawActivePowers(void)
//--------------------------------------------------------------
{
	int icon_size = 40;
	int i = 0;
	int startx = icon_size*2+16;
	int starty = SCREEN_HEIGHT - icon_size*2;

	int endx = icon_size;
	int endy = icon_size;

	if (cg.snap->ps.scopeType)
	{ //don't display over zoom mask
		return;
	}

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == FACTION_SPECTATOR)
	{
		return;
	}

	trap->R_SetColor( NULL );

	while (i < NUM_FORCE_POWERS)
	{
		if ((cg.snap->ps.fd.forcePowersActive & (1 << forcePowerSorted[i])) &&
			CG_IsDurationPower(forcePowerSorted[i]))
		{
			CG_DrawPic( startx, starty, endx, endy, cgs.media.forcePowerIcons[forcePowerSorted[i]]);
			startx += (icon_size+2); //+2 for spacing
			if ((startx+icon_size) >= SCREEN_WIDTH-80)
			{
				startx = icon_size*2+16;
				starty += (icon_size+2);
			}
		}

		i++;
	}

	//additionally, draw an icon force force rage recovery
	if (cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
	{
		CG_DrawPic( startx, starty, endx, endy, cgs.media.rageRecShader);
	}
}

//--------------------------------------------------------------
static void CG_DrawRocketLocking( int lockEntNum, int lockTime )
//--------------------------------------------------------------
{
	int		cx, cy;
	vec3_t	org;
	static	int oldDif = 0;
	centity_t *cent = &cg_entities[lockEntNum];
	vec4_t color={0.0f,0.0f,0.0f,0.0f};
	float lockTimeInterval = ((cgs.gametype==GT_SIEGE)?2400.0f:1200.0f)/16.0f;
	//FIXME: if in a vehicle, use the vehicle's lockOnTime...
	int dif = (cg.time - cg.snap->ps.rocketLockTime)/lockTimeInterval;
	int i;

	if (!cg.snap->ps.rocketLockTime)
	{
		return;
	}

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == FACTION_SPECTATOR)
	{
		return;
	}

	if ( cg.snap->ps.m_iVehicleNum )
	{//driving a vehicle
		centity_t *veh = &cg_entities[cg.snap->ps.m_iVehicleNum];
		if ( veh->m_pVehicle )
		{
			vehWeaponInfo_t *vehWeapon = NULL;
			if ( cg.predictedVehicleState.weaponstate == WEAPON_CHARGING_ALT )
			{
				if ( veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID > VEH_WEAPON_BASE
					&& veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID < MAX_VEH_WEAPONS )
				{
					vehWeapon = &g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID];
				}
			}
			else
			{
				if ( veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID > VEH_WEAPON_BASE
					&& veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID < MAX_VEH_WEAPONS )
				{
					vehWeapon = &g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID];
				}
			}
			if ( vehWeapon != NULL )
			{//we are trying to lock on with a valid vehicle weapon, so use *its* locktime, not the hard-coded one
				if ( !vehWeapon->iLockOnTime )
				{//instant lock-on
					dif = 10.0f;
				}
				else
				{//use the custom vehicle lockOnTime
					lockTimeInterval = (vehWeapon->iLockOnTime/16.0f);
					dif = (cg.time - cg.snap->ps.rocketLockTime)/lockTimeInterval;
				}
			}
		}
	}
	//We can't check to see in pmove if players are on the same team, so we resort
	//to just not drawing the lock if a teammate is the locked on ent
	if (cg.snap->ps.rocketLockIndex >= 0 &&
		cg.snap->ps.rocketLockIndex < ENTITYNUM_NONE)
	{
		clientInfo_t *ci = NULL;

		if (cg.snap->ps.rocketLockIndex < MAX_CLIENTS)
		{
			ci = &cgs.clientinfo[cg.snap->ps.rocketLockIndex];
		}
		else
		{
			ci = cg_entities[cg.snap->ps.rocketLockIndex].npcClient;
		}

		if (ci)
		{
			if (ci->team == cgs.clientinfo[cg.snap->ps.clientNum].team)
			{
				if (cgs.gametype >= GT_TEAM)
				{
					return;
				}
			}
			else if (cgs.gametype >= GT_TEAM)
			{
				centity_t *hitEnt = &cg_entities[cg.snap->ps.rocketLockIndex];
				if (hitEnt->currentState.eType == ET_NPC &&
					hitEnt->currentState.NPC_class == CLASS_VEHICLE &&
					hitEnt->currentState.owner < ENTITYNUM_WORLD)
				{ //this is a vehicle, if it has a pilot and that pilot is on my team, then...
					if (hitEnt->currentState.owner < MAX_CLIENTS)
					{
						ci = &cgs.clientinfo[hitEnt->currentState.owner];
					}
					else
					{
						ci = cg_entities[hitEnt->currentState.owner].npcClient;
					}
					if (ci && ci->team == cgs.clientinfo[cg.snap->ps.clientNum].team)
					{
						return;
					}
				}
			}
		}
	}

	if (cg.snap->ps.rocketLockTime != -1)
	{
		lastvalidlockdif = dif;
	}
	else
	{
		dif = lastvalidlockdif;
	}

	if ( !cent )
	{
		return;
	}

	VectorCopy( cent->lerpOrigin, org );

	if ( CG_WorldCoordToScreenCoord( org, &cx, &cy ))
	{
		// we care about distance from enemy to eye, so this is good enough
		float sz = Distance( cent->lerpOrigin, cg.refdef.vieworg ) / 1024.0f;

		if ( sz > 1.0f )
		{
			sz = 1.0f;
		}
		else if ( sz < 0.0f )
		{
			sz = 0.0f;
		}

		sz = (1.0f - sz) * (1.0f - sz) * 32 + 6;

		cy += sz * 0.5f;

		if ( dif < 0 )
		{
			oldDif = 0;
			return;
		}
		else if ( dif > 8 )
		{
			dif = 8;
		}

		// do sounds
		if ( oldDif != dif )
		{
			if ( dif == 8 )
			{
				if ( cg.snap->ps.m_iVehicleNum )
				{
					trap->S_StartSound( org, 0, CHAN_AUTO, trap->S_RegisterSound( "sound/vehicles/weapons/common/lock.wav" ));
				}
				else
				{
					trap->S_StartSound( org, 0, CHAN_AUTO, trap->S_RegisterSound( "sound/weapons/rocket/lock.wav" ));
				}
			}
			else
			{
				if ( cg.snap->ps.m_iVehicleNum )
				{
					trap->S_StartSound( org, 0, CHAN_AUTO, trap->S_RegisterSound( "sound/vehicles/weapons/common/tick.wav" ));
				}
				else
				{
					trap->S_StartSound( org, 0, CHAN_AUTO, trap->S_RegisterSound( "sound/weapons/rocket/tick.wav" ));
				}
			}
		}

		oldDif = dif;

		for ( i = 0; i < dif; i++ )
		{
			color[0] = 1.0f;
			color[1] = 0.0f;
			color[2] = 0.0f;
			color[3] = 0.1f * i + 0.2f;

			trap->R_SetColor( color );

			// our slices are offset by about 45 degrees.
			CG_DrawRotatePic( cx - sz, cy - sz, sz, sz, i * 45.0f, trap->R_RegisterShaderNoMip( "gfx/2d/wedge" ));
		}

		// we are locked and loaded baby
		if ( dif == 8 )
		{
			color[0] = color[1] = color[2] = sin( cg.time * 0.05f ) * 0.5f + 0.5f;
			color[3] = 1.0f; // this art is additive, so the alpha value does nothing

			trap->R_SetColor( color );

			CG_DrawPic( cx - sz, cy - sz * 2, sz * 2, sz * 2, trap->R_RegisterShaderNoMip( "gfx/2d/lock" ));
		}
	}
}

extern void CG_CalcVehMuzzle(Vehicle_t *pVeh, centity_t *ent, int muzzleNum);
qboolean CG_CalcVehicleMuzzlePoint( int entityNum, vec3_t start, vec3_t d_f, vec3_t d_rt, vec3_t d_up)
{
	centity_t *vehCent = &cg_entities[entityNum];
	if ( vehCent->m_pVehicle && vehCent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER )
	{//draw from barrels
		VectorCopy( vehCent->lerpOrigin, start );
		start[2] += vehCent->m_pVehicle->m_pVehicleInfo->height-DEFAULT_MINS_2-48;
		AngleVectors( vehCent->lerpAngles, d_f, d_rt, d_up );
		/*
		mdxaBone_t		boltMatrix;
		int				bolt;
		vec3_t			yawOnlyAngles;

		VectorSet( yawOnlyAngles, 0, vehCent->lerpAngles[YAW], 0 );

		bolt = trap->G2API_AddBolt( vehCent->ghoul2, 0, "*flash1");
		trap->G2API_GetBoltMatrix( vehCent->ghoul2, 0, bolt, &boltMatrix,
									yawOnlyAngles, vehCent->lerpOrigin, cg.time,
									NULL, vehCent->modelScale );

		// work the matrix axis stuff into the original axis and origins used.
		BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, start );
		BG_GiveMeVectorFromMatrix( &boltMatrix, POSITIVE_X, d_f );
		VectorClear( d_rt );//don't really need this, do we?
		VectorClear( d_up );//don't really need this, do we?
		*/
	}
	else
	{
		//check to see if we're a turret gunner on this vehicle
		if ( cg.predictedPlayerState.generic1 )//as a passenger
		{//passenger in a vehicle
			if ( vehCent->m_pVehicle
				&& vehCent->m_pVehicle->m_pVehicleInfo
				&& vehCent->m_pVehicle->m_pVehicleInfo->maxPassengers )
			{//a vehicle capable of carrying passengers
				int turretNum;
				for ( turretNum = 0; turretNum < MAX_VEHICLE_TURRETS; turretNum++ )
				{
					if ( vehCent->m_pVehicle->m_pVehicleInfo->turret[turretNum].iAmmoMax )
					{// valid turret
						if ( vehCent->m_pVehicle->m_pVehicleInfo->turret[turretNum].passengerNum == cg.predictedPlayerState.generic1 )
						{//I control this turret
							//Go through all muzzles, average their positions and directions and use the result for crosshair trace
							int vehMuzzle, numMuzzles = 0;
							vec3_t	muzzlesAvgPos={0},muzzlesAvgDir={0};
							int	i;

							for ( i = 0; i < MAX_VEHICLE_TURRET_MUZZLES; i++ )
							{
								vehMuzzle = vehCent->m_pVehicle->m_pVehicleInfo->turret[turretNum].iMuzzle[i];
								if ( vehMuzzle )
								{
									vehMuzzle -= 1;
									CG_CalcVehMuzzle( vehCent->m_pVehicle, vehCent, vehMuzzle );
									VectorAdd( muzzlesAvgPos, vehCent->m_pVehicle->m_vMuzzlePos[vehMuzzle], muzzlesAvgPos );
									VectorAdd( muzzlesAvgDir, vehCent->m_pVehicle->m_vMuzzleDir[vehMuzzle], muzzlesAvgDir );
									numMuzzles++;
								}
								if ( numMuzzles )
								{
									VectorScale( muzzlesAvgPos, 1.0f/(float)numMuzzles, start );
									VectorScale( muzzlesAvgDir, 1.0f/(float)numMuzzles, d_f );
									VectorClear( d_rt );
									VectorClear( d_up );
									return qtrue;
								}
							}
						}
					}
				}
			}
		}
		VectorCopy( vehCent->lerpOrigin, start );
		AngleVectors( vehCent->lerpAngles, d_f, d_rt, d_up );
	}
	return qfalse;
}

//calc the muzzle point from the e-web itself
void CG_CalcEWebMuzzlePoint(centity_t *cent, vec3_t start, vec3_t d_f, vec3_t d_rt, vec3_t d_up)
{
	int bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*cannonflash");

	assert(bolt != -1);

	if (bolt != -1)
	{
		mdxaBone_t boltMatrix;

		trap->G2API_GetBoltMatrix_NoRecNoRot(cent->ghoul2, 0, bolt, &boltMatrix, cent->lerpAngles, cent->lerpOrigin, cg.time, NULL, cent->modelScale);
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, start);
		BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, d_f);

		//these things start the shot a little inside the bbox to assure not starting in something solid
		VectorMA(start, -16.0f, d_f, start);

		//I guess
		VectorClear( d_rt );//don't really need this, do we?
		VectorClear( d_up );//don't really need this, do we?
	}
}

void CG_DrawNPCNames( void )
{// Float a NPC name above their head!
	int				i;

	// Load the list on first check...
	Load_NPC_Names();

	for (i = 0; i < MAX_GENTITIES; i++)
	{// Cycle through them...
		qboolean		skip = qfalse;
		vec3_t			origin;
		centity_t		*cent = &cg_entities[i];
		char			str1[255], str2[255];
		int				w, w2;
		float			size, x, y, x2, y2/*, x3, y3*/, dist;
		vec4_t			tclr =	{ 0.325f,	0.325f,	1.0f,	1.0f	};
		vec4_t			tclr2 = { 0.325f,	0.325f,	1.0f,	1.0f	};
		char			sanitized1[1024], sanitized2[1024];
		int				baseColor = CT_BLUE;
		float			multiplier = 1.0f;
		clientInfo_t	*ci = NULL;

		if (!cent)
			continue;

		if (cent->currentState.number == cg.snap->ps.clientNum || cent->currentState.number == cg.clientNum)
			continue; // Never show yourself (or who you are following)! lol!

		if (cent->currentState.eType != ET_NPC && cent->currentState.eType != ET_PLAYER)
			continue;

		if (cent->currentState.eType == ET_NPC)
		{
			if (!cent->npcClient || !cent->npcClient->ghoul2Model)
				continue;

			if ( !cent->npcClient->infoValid )
				continue;
		}

		if (!cent->ghoul2)
			continue;

		if (cent->cloaked)
			continue;

		if (cent->currentState.eType == ET_PLAYER && cgs.clientinfo[i].team == FACTION_SPECTATOR)
			continue;

		if (cent->currentState.eFlags & EF_DEAD)
			continue;

		if (cent->playerState->pm_type == PM_DEAD)
			continue;

		if (cent->currentState.eType == ET_FREED)
			continue;

		if (cent->currentState.eFlags & EF_NODRAW)
			continue;

		//if (cent->currentState.health <= 0)
		//	continue;

		if (cent->currentState.number < MAX_CLIENTS)
		{
			ci = &cgs.clientinfo[cent->currentState.number];
		}
		else
		{
			ci = cent->npcClient;
		}

		if (!ci)
		{
			continue;
		}

		if (!ci->infoValid)
		{
			continue;
		}

		if (ci->team == FACTION_SPECTATOR)
		{
			continue;
		}

		if (cent->playerState->persistant[PERS_TEAM] == FACTION_SPECTATOR)
			continue;

		if (cent->currentState.eType == ET_PLAYER)
		{
			if (cgs.clientinfo[i].team == FACTION_EMPIRE)
			{
				sprintf(str2, "< Imperial >");
				tclr[0] = 0.5f;
				tclr[1] = 0.5f;
				tclr[2] = 0.125f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.5f;
				tclr2[1] = 0.5f;
				tclr2[2] = 0.125f;
				tclr2[3] = 1.0f;
			}
			else if (cgs.clientinfo[i].team == FACTION_REBEL)
			{
				sprintf(str2, "< Rebel >");
				tclr[0] = 0.125f;
				tclr[1] = 0.125f;
				tclr[2] = 0.7f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.125f;
				tclr2[1] = 0.125f;
				tclr2[2] = 0.7f;
				tclr2[3] = 1.0f;
			}
			else if (cgs.clientinfo[i].team == FACTION_MANDALORIAN)
			{
				sprintf(str2, "< Mandalorian >");
				tclr[0] = 0.75f;
				tclr[1] = 0.75f;
				tclr[2] = 0.125f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.75f;
				tclr2[1] = 0.75f;
				tclr2[2] = 0.125f;
				tclr2[3] = 1.0f;
			}
			else if (cgs.clientinfo[i].team == FACTION_WILDLIFE)
			{
				sprintf(str2, "< Wildlife >");
				tclr[0] = 0.9f;
				tclr[1] = 0.9f;
				tclr[2] = 0.125f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.75f;
				tclr2[1] = 0.75f;
				tclr2[2] = 0.125f;
				tclr2[3] = 1.0f;
			}
			else if (cgs.clientinfo[i].team == FACTION_MERC)
			{
				sprintf(str2, "< Mercenary >");
				tclr[0] = 0.125f;
				tclr[1] = 0.75f;
				tclr[2] = 0.125f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.125f;
				tclr2[1] = 0.75f;
				tclr2[2] = 0.125f;
				tclr2[3] = 1.0f;
			}
			else if (cgs.clientinfo[i].team == FACTION_PIRATES)
			{
				sprintf(str2, "< Pirate >");
				tclr[0] = 0.5f;
				tclr[1] = 0.5f;
				tclr[2] = 0.5f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.5f;
				tclr2[1] = 0.5f;
				tclr2[2] = 0.5f;
				tclr2[3] = 1.0f;
			}
			else
			{
				sprintf(str2, "< Player >");
				tclr[0] = 0.7f;
				tclr[1] = 0.7f;
				tclr[2] = 0.125f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.7f;
				tclr2[1] = 0.7f;
				tclr2[2] = 0.125f;
				tclr2[3] = 1.0f;
			}

			sprintf(str1, "%s", cgs.clientinfo[ i ].cleanname);
		}
		else
		{
			switch( cent->currentState.NPC_class )
			{// UQ1: Supported Class Types...
			case CLASS_CIVILIAN:
			case CLASS_CIVILIAN_R2D2:
			case CLASS_CIVILIAN_R5D2:
			case CLASS_CIVILIAN_PROTOCOL:
			case CLASS_CIVILIAN_WEEQUAY:
				sprintf(str2, "< Civilian >");
				tclr[0] = 0.125f;
				tclr[1] = 0.125f;
				tclr[2] = 0.7f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.125f;
				tclr2[1] = 0.125f;
				tclr2[2] = 0.7f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_REBEL:
			case CLASS_JAN:
				sprintf(str2, "< Rebel >");
				tclr[0] = 0.125f;
				tclr[1] = 0.125f;
				tclr[2] = 0.7f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.125f;
				tclr2[1] = 0.125f;
				tclr2[2] = 0.7f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_NATIVE:
			case CLASS_NATIVE_GUNNER:
				sprintf(str2, "< Native >");
				tclr[0] = 0.125f;
				tclr[1] = 0.125f;
				tclr[2] = 0.7f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.125f;
				tclr2[1] = 0.125f;
				tclr2[2] = 0.7f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_JEDI:
			case CLASS_KYLE:
			case CLASS_LUKE:
			case CLASS_MONMOTHA:			
			case CLASS_MORGANKATARN:
				sprintf(str2, "< Jedi >");
				tclr[0] = 0.125f;
				tclr[1] = 0.325f;
				tclr[2] = 0.7f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.125f;
				tclr2[1] = 0.325f;
				tclr2[2] = 0.7f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_PADAWAN:
				sprintf(str2, "< Padawan >");
				tclr[0] = 0.125f;
				tclr[1] = 0.325f;
				tclr[2] = 0.7f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.125f;
				tclr2[1] = 0.325f;
				tclr2[2] = 0.7f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_GENERAL_VENDOR:
				sprintf(str2, "< General Vendor >");
				tclr[0] = 0.525f;
				tclr[1] = 0.525f;
				tclr[2] = 1.0f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.525f;
				tclr2[1] = 0.525f;
				tclr2[2] = 1.0f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_WEAPONS_VENDOR:
				sprintf(str2, "< Weapons Vendor >");
				tclr[0] = 0.525f;
				tclr[1] = 0.525f;
				tclr[2] = 1.0f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.525f;
				tclr2[1] = 0.525f;
				tclr2[2] = 1.0f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_ARMOR_VENDOR:
				sprintf(str2, "< Armor Vendor >");
				tclr[0] = 0.525f;
				tclr[1] = 0.525f;
				tclr[2] = 1.0f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.525f;
				tclr2[1] = 0.525f;
				tclr2[2] = 1.0f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_SUPPLIES_VENDOR:
				sprintf(str2, "< Supplies Vendor >");
				tclr[0] = 0.525f;
				tclr[1] = 0.525f;
				tclr[2] = 1.0f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.525f;
				tclr2[1] = 0.525f;
				tclr2[2] = 1.0f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_FOOD_VENDOR:
				sprintf(str2, "< Food Vendor >");
				tclr[0] = 0.525f;
				tclr[1] = 0.525f;
				tclr[2] = 1.0f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.525f;
				tclr2[1] = 0.525f;
				tclr2[2] = 1.0f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_MEDICAL_VENDOR:
				sprintf(str2, "< Medical Vendor >");
				tclr[0] = 0.525f;
				tclr[1] = 0.525f;
				tclr[2] = 1.0f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.525f;
				tclr2[1] = 0.525f;
				tclr2[2] = 1.0f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_GAMBLER_VENDOR:
				sprintf(str2, "< Gambling Vendor >");
				tclr[0] = 0.525f;
				tclr[1] = 0.525f;
				tclr[2] = 1.0f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.525f;
				tclr2[1] = 0.525f;
				tclr2[2] = 1.0f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_TRADE_VENDOR:
				sprintf(str2, "< Trade Vendor >");
				tclr[0] = 0.525f;
				tclr[1] = 0.525f;
				tclr[2] = 1.0f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.525f;
				tclr2[1] = 0.525f;
				tclr2[2] = 1.0f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_ODDITIES_VENDOR:
				sprintf(str2, "< Oddities Vendor >");
				tclr[0] = 0.525f;
				tclr[1] = 0.525f;
				tclr[2] = 1.0f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.525f;
				tclr2[1] = 0.525f;
				tclr2[2] = 1.0f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_DRUG_VENDOR:
				sprintf(str2, "< Drug Vendor >");
				tclr[0] = 0.525f;
				tclr[1] = 0.525f;
				tclr[2] = 1.0f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.525f;
				tclr2[1] = 0.525f;
				tclr2[2] = 1.0f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_TRAVELLING_VENDOR:
				sprintf(str2, "< Travelling Vendor >");
				tclr[0] = 0.525f;
				tclr[1] = 0.525f;
				tclr[2] = 1.0f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.525f;
				tclr2[1] = 0.525f;
				tclr2[2] = 1.0f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_MERC:
				sprintf(str2, "< Mercenary >");
				tclr[0] = 1.0f;
				tclr[1] = 0.125f;
				tclr[2] = 0.125f;
				tclr[3] = 1.0f;

				tclr2[0] = 1.0f;
				tclr2[1] = 0.125f;
				tclr2[2] = 0.125f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_STORMTROOPER:
			case CLASS_STORMTROOPER_ADVANCED:
			case CLASS_STORMTROOPER_ATST_PILOT:
			case CLASS_STORMTROOPER_ATAT_PILOT:
			case CLASS_SWAMPTROOPER:
			case CLASS_IMPWORKER:
			case CLASS_IMPERIAL:
			case CLASS_SHADOWTROOPER:
			case CLASS_COMMANDO:
				sprintf(str2, "< Imperial >");
				tclr[0] = 1.0f;
				tclr[1] = 0.125f;
				tclr[2] = 0.125f;
				tclr[3] = 1.0f;

				tclr2[0] = 1.0f;
				tclr2[1] = 0.125f;
				tclr2[2] = 0.125f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_DEATHTROOPER:
				sprintf(str2, "< Death Trooper >");
				tclr[0] = 1.0f;
				tclr[1] = 0.125f;
				tclr[2] = 0.125f;
				tclr[3] = 1.0f;

				tclr2[0] = 1.0f;
				tclr2[1] = 0.125f;
				tclr2[2] = 0.125f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_HK51:
				sprintf(str2, "< HK-51 >");
				tclr[0] = 1.0f;
				tclr[1] = 0.125f;
				tclr[2] = 0.125f;
				tclr[3] = 1.0f;

				tclr2[0] = 1.0f;
				tclr2[1] = 0.125f;
				tclr2[2] = 0.125f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_K2SO:
				sprintf(str2, "< Security Droid >");
				tclr[0] = 1.0f;
				tclr[1] = 0.125f;
				tclr[2] = 0.125f;
				tclr[3] = 1.0f;

				tclr2[0] = 1.0f;
				tclr2[1] = 0.125f;
				tclr2[2] = 0.125f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_INQUISITOR:
				sprintf(str2, "< Sith Inquisitor >");
				tclr[0] = 1.0f;
				tclr[1] = 0.325f;
				tclr[2] = 0.125f;
				tclr[3] = 1.0f;

				tclr2[0] = 1.0f;
				tclr2[1] = 0.325f;
				tclr2[2] = 0.125f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_PURGETROOPER:
				sprintf(str2, "< Purge Trooper >");
				tclr[0] = 1.0f;
				tclr[1] = 0.325f;
				tclr[2] = 0.125f;
				tclr[3] = 1.0f;

				tclr2[0] = 1.0f;
				tclr2[1] = 0.325f;
				tclr2[2] = 0.125f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_TAVION:
			case CLASS_REBORN:
			case CLASS_DESANN:
			case CLASS_ALORA:
				sprintf(str2, "< Sith >");
				tclr[0] = 1.0f;
				tclr[1] = 0.325f;
				tclr[2] = 0.125f;
				tclr[3] = 1.0f;

				tclr2[0] = 1.0f;
				tclr2[1] = 0.325f;
				tclr2[2] = 0.125f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_BOBAFETT:
				sprintf(str2, "< Mandalorian >");
				tclr[0] = 1.0f;
				tclr[1] = 0.5f;
				tclr[2] = 0.0f;
				tclr[3] = 1.0f;

				tclr[0] = 1.0f;
				tclr2[1] = 0.5f;
				tclr2[2] = 0.0f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_ATST_OLD:
			case CLASS_ATST:
			case CLASS_ATAT:
			case CLASS_ATPT:
				sprintf(str2, "< Vehicle >");
				tclr[0] = 1.0f;
				tclr[1] = 0.225f;
				tclr[2] = 0.125f;
				tclr[3] = 1.0f;

				tclr2[0] = 1.0f;
				tclr2[1] = 0.225f;
				tclr2[2] = 0.125f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_CLAW:
			case CLASS_FISH:
			case CLASS_FLIER2:
			case CLASS_GLIDER:
			case CLASS_HOWLER:
			case CLASS_REEK:
			case CLASS_NEXU:
			case CLASS_ACKLAY:
			case CLASS_LIZARD:
			case CLASS_MINEMONSTER:
			case CLASS_SWAMP:
			case CLASS_RANCOR:
			case CLASS_WAMPA:
				sprintf(str2, "< Wildlife >");
				tclr[0] = 1.0f;
				tclr[1] = 1.0f;
				tclr[2] = 0.125f;
				tclr[3] = 1.0f;

				tclr2[0] = 1.0f;
				tclr2[1] = 1.0f;
				tclr2[2] = 0.125f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_VEHICLE:
				sprintf(str2, "< Vehicle >");
				tclr[0] = 1.0f;
				tclr[1] = 0.125f;
				tclr[2] = 0.125f;
				tclr[3] = 1.0f;

				tclr2[0] = 1.0f;
				tclr2[1] = 0.125f;
				tclr2[2] = 0.125f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_BESPIN_COP:
			case CLASS_LANDO:
			case CLASS_PRISONER:
				sprintf(str2, "< Rebel >");
				tclr[0] = 0.125f;
				tclr[1] = 0.125f;
				tclr[2] = 0.7f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.125f;
				tclr2[1] = 0.125f;
				tclr2[2] = 0.7f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_RODIAN:
			case CLASS_WEEQUAY:
				sprintf(str2, "< Pirate >");
				tclr[0] = 0.5f;
				tclr[1] = 0.5f;
				tclr[2] = 0.5f;
				tclr[3] = 1.0f;

				tclr2[0] = 0.5f;
				tclr2[1] = 0.5f;
				tclr2[2] = 0.5f;
				tclr2[3] = 1.0f;
				break;
			case CLASS_GALAK:
			case CLASS_GRAN:
			case CLASS_REELO:
			case CLASS_MURJJ:
			case CLASS_TRANDOSHAN:
			case CLASS_UGNAUGHT:
			case CLASS_BARTENDER:
			case CLASS_JAWA:
				if (cent->playerState->persistant[PERS_TEAM] == NPCTEAM_ENEMY)
				{
					sprintf(str2, "< Thug >");
					tclr[0] = 0.5f;
					tclr[1] = 0.5f;
					tclr[2] = 0.125f;
					tclr[3] = 1.0f;

					tclr2[0] = 0.5f;
					tclr2[1] = 0.5f;
					tclr2[2] = 0.125f;
					tclr2[3] = 1.0f;
				}
				else if (cent->playerState->persistant[PERS_TEAM] == NPCTEAM_PLAYER)
				{
					sprintf(str2, "< Rebel >");
					tclr[0] = 0.125f;
					tclr[1] = 0.125f;
					tclr[2] = 0.7f;
					tclr[3] = 1.0f;

					tclr2[0] = 0.125f;
					tclr2[1] = 0.125f;
					tclr2[2] = 0.7f;
					tclr2[3] = 1.0f;
				}
				else
				{
					sprintf(str2, "< Civilian >");
					tclr[0] = 0.7f;
					tclr[1] = 0.7f;
					tclr[2] = 0.125f;
					tclr[3] = 1.0f;

					tclr2[0] = 0.7f;
					tclr2[1] = 0.7f;
					tclr2[2] = 0.125f;
					tclr2[3] = 1.0f;
				}
				break;

			default:
				//CG_Printf("NPC %i is not a civilian or vendor (class %i).\n", cent->currentState.number, cent->currentState.NPC_class);
				continue; // Unsupported...
				break;
			}

			if (cent->currentState.NPC_NAME_ID > 0)
			{// Was assigned a full name already! Yay!
				switch( cent->currentState.NPC_class )
				{// UQ1: Supported Class Types...
				case CLASS_CIVILIAN:
				case CLASS_GENERAL_VENDOR:
				case CLASS_WEAPONS_VENDOR:
				case CLASS_ARMOR_VENDOR:
				case CLASS_SUPPLIES_VENDOR:
				case CLASS_FOOD_VENDOR:
				case CLASS_MEDICAL_VENDOR:
				case CLASS_GAMBLER_VENDOR:
				case CLASS_TRADE_VENDOR:
				case CLASS_ODDITIES_VENDOR:
				case CLASS_DRUG_VENDOR:
				case CLASS_TRAVELLING_VENDOR:
				case CLASS_LUKE:
				case CLASS_JEDI:
				case CLASS_PADAWAN:
				case CLASS_HK51:
				case CLASS_NATIVE:
				case CLASS_NATIVE_GUNNER:
				case CLASS_KYLE:
				case CLASS_JAN:
				case CLASS_MONMOTHA:			
				case CLASS_MORGANKATARN:
				case CLASS_TAVION:
				case CLASS_ALORA:
				case CLASS_INQUISITOR:
				case CLASS_REBORN:
				case CLASS_DESANN:
				case CLASS_BOBAFETT:
				case CLASS_COMMANDO:
				case CLASS_DEATHTROOPER:
				case CLASS_BARTENDER:
				case CLASS_BESPIN_COP:
				case CLASS_GALAK:
				case CLASS_GRAN:
				case CLASS_LANDO:			
				case CLASS_REBEL:
				case CLASS_REELO:
				case CLASS_MURJJ:
				case CLASS_PRISONER:
				case CLASS_RODIAN:
				case CLASS_TRANDOSHAN:
				case CLASS_UGNAUGHT:
				case CLASS_JAWA:
					sprintf(str1, "%s", NPC_NAME_LIST[cent->currentState.NPC_NAME_ID].HumanNames);
					break;
				case CLASS_PURGETROOPER:
					sprintf(str1, "PT-%i", cent->currentState.NPC_NAME_ID);	// EVIL. for a number of reasons --eez
					break;
				case CLASS_STORMTROOPER_ADVANCED:
					sprintf(str1, "TA-%i", cent->currentState.NPC_NAME_ID);	// EVIL. for a number of reasons --eez
					break;
				case CLASS_STORMTROOPER_ATST_PILOT:
				case CLASS_STORMTROOPER_ATAT_PILOT:
					sprintf(str1, "TA-%i", cent->currentState.NPC_NAME_ID);	// EVIL. for a number of reasons --eez
					break;
				case CLASS_STORMTROOPER:
					sprintf(str1, "TK-%i", cent->currentState.NPC_NAME_ID);	// EVIL. for a number of reasons --eez
					break;
				case CLASS_SWAMPTROOPER:
					sprintf(str1, "TS-%i", cent->currentState.NPC_NAME_ID);	// EVIL. for a number of reasons --eez
					break;
				case CLASS_IMPWORKER:
					sprintf(str1, "IW-%i", cent->currentState.NPC_NAME_ID);	// EVIL. for a number of reasons --eez
					break;
				case CLASS_SHADOWTROOPER:
					sprintf(str1, "ST-%i", cent->currentState.NPC_NAME_ID);	// EVIL. for a number of reasons --eez
					break;
				case CLASS_IMPERIAL:
					sprintf(str1, "Commander %s", NPC_NAME_LIST[cent->currentState.NPC_NAME_ID].HumanNames);	// EVIL. for a number of reasons --eez
					break;
				case CLASS_ATST_OLD:				// technically droid...
				case CLASS_ATST:
					sprintf(str1, "AT-ST");
					break;
				case CLASS_ATAT:
					sprintf(str1, "AT-AT");
					break;
				case CLASS_ATPT:
					sprintf(str1, "AT-PT");
					break;
				case CLASS_CLAW:
					sprintf(str1, "Claw");
					break;
				case CLASS_FISH:
					sprintf(str1, "Sea Creature");
					break;
				case CLASS_FLIER2:
					sprintf(str1, "Flier");
					break;
				case CLASS_GLIDER:
					sprintf(str1, "Glider");
					break;
				case CLASS_HOWLER:
					sprintf(str1, "Howler");
					break;
				case CLASS_REEK:
					sprintf(str1, "Reek");
					break;
				case CLASS_NEXU:
					sprintf(str1, "Nexu");
					break;
				case CLASS_ACKLAY:
					sprintf(str1, "Acklay");
					break;
				case CLASS_LIZARD:
					sprintf(str1, "Lizard");
					break;
				case CLASS_MINEMONSTER:
					sprintf(str1, "Mine Monster");
					break;
				case CLASS_SWAMP:
					sprintf(str1, "Swamp Monster");
					break;
				case CLASS_RANCOR:
					sprintf(str1, "Rancor");
					break;
				case CLASS_WAMPA:
					sprintf(str1, "Wampa");
					break;
				case CLASS_K2SO:
					sprintf(str1, "K2-SO");
					break;
				case CLASS_R2D2:
				case CLASS_CIVILIAN_R2D2:
					sprintf(str1, "R2D2 Droid");
					break;
				case CLASS_R5D2:
				case CLASS_CIVILIAN_R5D2:
					sprintf(str1, "R5D2 Droid");
					break;
				case CLASS_PROTOCOL:
				case CLASS_CIVILIAN_PROTOCOL:
					sprintf(str1, "Protocol Droid");
					break;
				case CLASS_WEEQUAY:
				case CLASS_CIVILIAN_WEEQUAY:
					sprintf(str1, "Weequay");
					break;
				case CLASS_VEHICLE:
					sprintf(str1, "");
					break;
				default:
					//CG_Printf("NPC %i is not a civilian or vendor (class %i).\n", cent->currentState.number, cent->currentState.NPC_class);
					continue; // Unsupported...
					break;
				}
			}
			else
			{
				if (cent->currentState.eType == ET_PLAYER)
				{
					sprintf(str1, "%s", cgs.clientinfo[ cent->currentState.number ].name);
				}
				//continue;
			}
		}

		if (cent->currentState.eType == ET_NPC)
		{// If it's an npc and has no ci->name, then set it here...
			if (strcmp(ci->name, str1)) strcpy(ci->name, str1);
		}
		
		VectorCopy( cent->lerpOrigin, origin );
		origin[2] += 30;//60;

		// Account for ducking
		if ( cent->playerState->pm_flags & PMF_DUCKED )
			origin[2] -= 18;
	
		// Draw the NPC name!
		if (!CG_WorldCoordToScreenCoordFloat(origin, &x, &y))
		{
			//CG_Printf("FAILED %i screen coords are %fx%f. (%f %f %f)\n", cent->currentState.number, x, y, origin[0], origin[1], origin[2]);
			continue;
		}

		if (x < 0 || x > 640 || y < 0 || y > 480)
		{
			//CG_Printf("FAILED2 %i screen coords are %fx%f. (%f %f %f)\n", cent->currentState.number, x, y, origin[0], origin[1], origin[2]);
			continue;
		}

		VectorCopy( cent->lerpOrigin, origin );
		origin[2] += 25;//50;

		// Account for ducking
		if ( cent->playerState->pm_flags & PMF_DUCKED )
			origin[2] -= 18;

		dist = Distance(cg.snap->ps.origin, origin);
		
		if (dist > 1024.0f/*2500.0f*/) continue; // Too far...
		if (dist < 192.0f/*d_roff.value*//*350.0f*/) multiplier = 200.0f/*d_poff.value*//dist; // Cap short ranges...

		if (!CG_WorldCoordToScreenCoordFloat(origin, &x2, &y2))
		{
			//CG_Printf("FAILED %i screen coords are %fx%f. (%f %f %f)\n", cent->currentState.number, x, y, origin[0], origin[1], origin[2]);
			continue;
		}

		if (x2 < 0 || x2 > 640 || y2 < 0 || y2 > 480)
		{
			//CG_Printf("FAILED2 %i screen coords are %fx%f. (%f %f %f)\n", cent->currentState.number, x, y, origin[0], origin[1], origin[2]);
			continue;
		}

		//CG_Printf("%i screen coords are %fx%f. (%f %f %f)\n", cent->currentState.number, x, y, origin[0], origin[1], origin[2]);

		if (!CG_CheckClientVisibility(cent))
		{
			//CG_Printf("NPC is NOT visible.\n");
			//continue;
			skip = qtrue;
		}

		if (skip) continue;

		//CG_Printf("%i screen coords are %fx%f. (%f %f %f)\n", cent->currentState.number, x, y, origin[0], origin[1], origin[2]);

		CG_SanitizeString(str1, sanitized1);
		CG_SanitizeString(str2, sanitized2);
		
		size = dist * 0.0002;
		
		if (size > 0.99f) size = 0.99f;
		if (size < 0.01f) size = 0.01f;

		size = 1 - size;

		size *= 0.3;

		//y3 = y;

		w = CG_Text_Width(sanitized1, size*2, FONT_SMALL);
		y = y + CG_Text_Height(sanitized1, size*2, FONT_SMALL);
		x -= (w * 0.5f);
		
		w2 = CG_Text_Width(sanitized2, size*1.5, /*FONT_SMALL3*/FONT_SMALL);
		x2 -= (w2 * 0.5f);
		y2 = y + 10/*6*/ + CG_Text_Height(sanitized1, size*2, /*FONT_SMALL*/FONT_SMALL);

		CG_Text_Paint( x, (y*(1-size))+((30*(1-size))*(1-size))+sqrt(sqrt((1-size)*30))+((1-multiplier)*30), size*2, tclr, sanitized1, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_SMALL);
		CG_Text_Paint( x2, (y2*(1-size))+((30*(1-size))*(1-size))+sqrt(sqrt((1-size)*30))+((1-multiplier)*30), size*1.5, tclr2, sanitized2, 0, 0, ITEM_TEXTSTYLE_SHADOWED, /*FONT_SMALL3*/FONT_SMALL);

		//x3 = x;

		//CG_DrawHealthBar(cent, x3, y3, w, 100);

		//CG_Printf("Draw %s - %s as size %f.\n", sanitized1, sanitized2, size);
	}
}

#define		NUM_DAMAGES 16
int			damage_value_last[MAX_GENTITIES];
int			damage_show_time[MAX_GENTITIES][NUM_DAMAGES];
int			damage_show_value[MAX_GENTITIES][NUM_DAMAGES];
qboolean	damage_show_crit[MAX_GENTITIES][NUM_DAMAGES];

void CG_DrawDamage( void )
{// Float damage value above their heads!
	int				i;

	for (i = 0; i < MAX_GENTITIES; i++)
	{// Cycle through them...
		qboolean		skip = qfalse;
		vec3_t			origin;
		centity_t		*cent = &cg_entities[i];
		int				w, j;
		float			size, x, y, dist;
		vec4_t			colorMiss =	{ 0.625f,	0.625f,	0.625f,	1.0f	};
		vec4_t			colorNormal =	{ 0.825f,	0.825f,	0.0f,	1.0f	};
		vec4_t			colorCrit =	{ 1.0f,	1.0f,	0.0f,	1.0f	};
		int				baseColor = CT_BLUE;
		int				LOWEST_SLOT = -1;
		int				LOWEST_TIME = cg.time + 5000;
		int				NUM_DAMAGES_THIS_FRAME = 0;
		int				NUM_DAMAGES_DRAWN = 0;
		qboolean		found = qfalse;

		if (!cent)
			continue;

		if (cent->currentState.eType != ET_PLAYER && cent->currentState.eType != ET_NPC)
			continue;

		//if (!cent->ghoul2)
		//	continue;

		if (cent->cloaked)
			continue;

		/*
		if (cent->currentState.eFlags & EF_DEAD)
		{
			continue;
		}
		*/

		if (cent->currentState.eFlags & EF_NODRAW)
		{
			continue;
		}

		VectorCopy( cent->lerpOrigin, origin );
		origin[2] += 35;//30;

		dist = Distance(cg.refdef.vieworg/*cg.snap->ps.origin*/, origin);

		if (dist > 2048.0f/*1024.0f*/) continue; // Too far...

		// Account for ducking
		if ( cent->playerState->pm_flags & PMF_DUCKED )
			origin[2] -= 18;

		// Draw the NPC name!
		if (!CG_WorldCoordToScreenCoordFloat(origin, &x, &y))
		{
			continue;
		}

		if (x < 0 || x > 640 || y < 0 || y > 480)
		{
			continue;
		}

		if (!CG_CheckClientVisibility(cent))
		{
			continue;
		}
		
		// UQ1: Clean the list...
		for (j = 0; j < NUM_DAMAGES; j++)
		{
			int k = 0;

			// Is this still valid? If not then initialize...
			if (damage_show_time[i][j] < cg.time)
			{
				damage_show_time[i][j] = 0;
				damage_show_value[i][j] = 0;
				damage_show_crit[i][j] = qfalse;
			}
		}

		// How many are in use???
		for (j = 0; j < NUM_DAMAGES; j++)
		{
			if (damage_show_time[i][j] >= cg.time)
				NUM_DAMAGES_THIS_FRAME++;
		}

#if 0
		// UQ1: Sort the list...
		while (found)
		{
			for (j = 0; j < NUM_DAMAGES; j++)
			{
				int k = 0;

				// Find a value to move up the list...
				for (k = 0; k < NUM_DAMAGES; k++)
				{
					if (damage_show_time[i][k] > damage_show_time[i][j])
					{
						// We found one, move it up...
						int temp_time = damage_show_time[i][j];
						int temp_value = damage_show_value[i][j];
						qboolean temp_crit = damage_show_crit[i][j];

						damage_show_time[i][j] = damage_show_time[i][k];
						damage_show_value[i][j] = damage_show_value[i][k];
						damage_show_crit[i][j] = damage_show_crit[i][k];

						damage_show_time[i][k] = temp_time;
						damage_show_value[i][k] = temp_value;
						damage_show_crit[i][k] = temp_crit;

						found = qtrue;
					}
				}
			}
		}
#endif

		// Find where we can add another value if needed...
		for (j = 0; j < NUM_DAMAGES; j++)
		{
			if (damage_show_time[i][j] <= LOWEST_TIME)
			{
				LOWEST_SLOT = j;
				LOWEST_TIME = damage_show_time[i][j];
			}
		}

		//
		// Now they should be in order of longest time to display remaining...
		//

		if (cent->currentState.damageValue >= 0 
			&& cent->currentState.damageValue != damage_value_last[i] 
			&& !(LOWEST_SLOT == NUM_DAMAGES-1 && cent->currentState.damageValue == 0) /* This means a new character just came into range - don't draw a miss */)
		{
			// Replace the lowest time...
			damage_show_time[i][LOWEST_SLOT] = cg.time + 5000;
			damage_show_value[i][LOWEST_SLOT] = cent->currentState.damageValue;
			damage_show_crit[i][LOWEST_SLOT] = cent->currentState.damageCrit;
			damage_value_last[i] = cent->currentState.damageValue;
		}

		// The list has now been sorted... Now show all values...
		for (j = NUM_DAMAGES-1/*LOWEST_SLOT*/; j >= 0; j--)
		{
			char value[255];
			int x2 = x;
			int y2 = y;

			if (damage_show_time[i][j] < cg.time) continue;

			size = dist * 0.0002;

			if (size > 0.99f) size = 0.99f;
			if (size < 0.01f) size = 0.01f;

			size = 1 - size;

			size *= 0.3;

			if (damage_show_crit[i][j])
				size *= 2.0; // crit!

			if (damage_show_value[i][j] == 0)
				sprintf(value, "MISS");
			else
				sprintf(value, "%i", damage_show_value[i][j]);

			w = CG_Text_Width(value, size*2, FONT_SMALL2);
			
			x2 -= (w * 0.5f);

			//y2 -= (j * 12);
			y2 = y - (((cg.time + 5000) - damage_show_time[i][j]) / 10);

			if (x2 < 0 || x2 > 640) continue; // now off screen...
			if (y2 < 0 || y2 > 480) continue; // now off screen...

			if (damage_show_value[i][j] == 0) // grey
				CG_Text_Paint( x2, y2, size*2, colorMiss, value, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_SMALL2);
			else if (damage_show_crit[i][j]) // bright yellow
				CG_Text_Paint( x2, y2, size*2, colorCrit, value, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_SMALL2);
			else // darker yellow
				CG_Text_Paint( x2, y2, size*2, colorNormal, value, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_SMALL2);
		}
	}
}

/*
=================
CG_`Entity
=================
*/
#define MAX_XHAIR_DIST_ACCURACY	20000.0f
static void CG_ScanForCrosshairEntity( void ) {
	trace_t		trace;
	vec3_t		start, end;
	int			content;
	int			ignore;
	qboolean	bVehCheckTraceFromCamPos = qfalse;

	ignore = cg.predictedPlayerState.clientNum;

	if ( cg_dynamicCrosshair.integer )
	{
		vec3_t d_f, d_rt, d_up;
		//For now we still want to draw the crosshair in relation to the player's world coordinates
		//even if we have a melee weapon/no weapon.
		if ( cg.predictedPlayerState.m_iVehicleNum && (cg.predictedPlayerState.eFlags&EF_NODRAW) )
		{//we're *inside* a vehicle
			//do the vehicle's crosshair instead
			centity_t *veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
			qboolean gunner = qfalse;

			//if (veh->currentState.owner == cg.predictedPlayerState.clientNum)
			{ //the pilot
				ignore = cg.predictedPlayerState.m_iVehicleNum;
				gunner = CG_CalcVehicleMuzzlePoint(cg.predictedPlayerState.m_iVehicleNum, start, d_f, d_rt, d_up);
			}
			/*
			else
			{ //a passenger
				ignore = cg.predictedPlayerState.m_iVehicleNum;
				VectorCopy( veh->lerpOrigin, start );
				AngleVectors( veh->lerpAngles, d_f, d_rt, d_up );
				VectorMA(start, 32.0f, d_f, start); //super hack
			}
			*/
			if ( veh->m_pVehicle
				&& veh->m_pVehicle->m_pVehicleInfo
				&& veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER
				&& cg.distanceCull > MAX_XHAIR_DIST_ACCURACY
				&& !gunner )
			{
				//NOTE: on huge maps, the crosshair gets inaccurate at close range,
				//		so we'll do an extra G2 trace from the cg.refdef.vieworg
				//		to see if we hit anything closer and auto-aim at it if so
				bVehCheckTraceFromCamPos = qtrue;
			}
		}
		else if (cg.snap && cg.snap->ps.weapon == WP_EMPLACED_GUN && cg.snap->ps.emplacedIndex &&
			cg_entities[cg.snap->ps.emplacedIndex].ghoul2 && cg_entities[cg.snap->ps.emplacedIndex].currentState.weapon == WP_NONE)
		{ //locked into our e-web, calc the muzzle from it
			CG_CalcEWebMuzzlePoint(&cg_entities[cg.snap->ps.emplacedIndex], start, d_f, d_rt, d_up);
		}
		else
		{
			if (cg.snap && cg.snap->ps.weapon == WP_EMPLACED_GUN && cg.snap->ps.emplacedIndex)
			{
				vec3_t pitchConstraint;

				ignore = cg.snap->ps.emplacedIndex;

				VectorCopy(cg.refdef.viewangles, pitchConstraint);

				if (cg.renderingThirdPerson)
				{
					VectorCopy(cg.predictedPlayerState.viewangles, pitchConstraint);
				}
				else
				{
					VectorCopy(cg.refdef.viewangles, pitchConstraint);
				}

				if (pitchConstraint[PITCH] > 40)
				{
					pitchConstraint[PITCH] = 40;
				}

				AngleVectors( pitchConstraint, d_f, d_rt, d_up );
			}
			else
			{
				vec3_t pitchConstraint;

				if (cg.renderingThirdPerson)
				{
					VectorCopy(cg.predictedPlayerState.viewangles, pitchConstraint);
				}
				else
				{
					VectorCopy(cg.refdef.viewangles, pitchConstraint);
				}

				AngleVectors( pitchConstraint, d_f, d_rt, d_up );
			}
			CG_CalcMuzzlePoint(cg.snap->ps.clientNum, start);
		}

		VectorMA( start, cg.distanceCull, d_f, end );
	}
	else
	{
		VectorCopy( cg.refdef.vieworg, start );
		VectorMA( start, 131072, cg.refdef.viewaxis[0], end );
	}

	if ( cg_dynamicCrosshair.integer && cg_dynamicCrosshairPrecision.integer )
	{ //then do a trace with ghoul2 models in mind
		CG_G2Trace( &trace, start, vec3_origin, vec3_origin, end,
			ignore, CONTENTS_SOLID|CONTENTS_BODY );
		if ( bVehCheckTraceFromCamPos )
		{
			//NOTE: this MUST stay up to date with the method used in WP_VehCheckTraceFromCamPos
			centity_t *veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
			trace_t	extraTrace;
			vec3_t	viewDir2End, extraEnd;
			float	minAutoAimDist = Distance( veh->lerpOrigin, cg.refdef.vieworg ) + (veh->m_pVehicle->m_pVehicleInfo->length/2.0f) + 200.0f;

			VectorSubtract( end, cg.refdef.vieworg, viewDir2End );
			VectorNormalize( viewDir2End );
			VectorMA( cg.refdef.vieworg, MAX_XHAIR_DIST_ACCURACY, viewDir2End, extraEnd );
			CG_G2Trace( &extraTrace, cg.refdef.vieworg, vec3_origin, vec3_origin, extraEnd,
				ignore, CONTENTS_SOLID|CONTENTS_BODY );
			if ( !extraTrace.allsolid
				&& !extraTrace.startsolid )
			{
				if ( extraTrace.fraction < 1.0f )
				{
					if ( (extraTrace.fraction*MAX_XHAIR_DIST_ACCURACY) > minAutoAimDist )
					{
						if ( ((extraTrace.fraction*MAX_XHAIR_DIST_ACCURACY)-Distance( veh->lerpOrigin, cg.refdef.vieworg )) < (trace.fraction*cg.distanceCull) )
						{//this trace hit *something* that's closer than the thing the main trace hit, so use this result instead
							memcpy( &trace, &extraTrace, sizeof( trace_t ) );
						}
					}
				}
			}
		}
	}
	else
	{
		CG_Trace( &trace, start, vec3_origin, vec3_origin, end,
			ignore, CONTENTS_SOLID|CONTENTS_BODY );
	}

	centity_t *hitEnt = &cg_entities[trace.entityNum];

	if (cg.snap->ps.persistant[PERS_TEAM] != FACTION_SPECTATOR)
	{
		if (hitEnt && trace.entityNum < ENTITYNUM_WORLD && (hitEnt->currentState.eType == ET_PLAYER || hitEnt->currentState.eType == ET_NPC))
		{
			if (CG_IsMindTricked(hitEnt->currentState.trickedentindex,
				hitEnt->currentState.trickedentindex2,
				hitEnt->currentState.trickedentindex3,
				hitEnt->currentState.trickedentindex4,
				cg.snap->ps.clientNum))
			{
				if (cg.crosshairClientNum == trace.entityNum)
				{
					cg.crosshairClientNum = ENTITYNUM_NONE;
					cg.crosshairClientTime = 0;
				}

				CG_DrawCrosshair(trace.endpos, 0);

				return; //this entity is mind-tricking the current client, so don't render it
			}

			centity_t *veh = &cg_entities[trace.entityNum];
			cg.crosshairClientNum = trace.entityNum;
			cg.crosshairClientTime = cg.time;

			if (veh->currentState.eType == ET_NPC &&
				veh->currentState.NPC_class == CLASS_VEHICLE &&
				veh->currentState.owner < MAX_CLIENTS)
			{ //draw the name of the pilot then
				cg.crosshairClientNum = veh->currentState.owner;
				cg.crosshairVehNum = veh->currentState.number;
				cg.crosshairVehTime = cg.time;
			}

			CG_DrawCrosshair(trace.endpos, 1);
		}
		else
		{
			CG_DrawCrosshair(trace.endpos, 0);
		}
	}

	//if ( trace.entityNum >= MAX_CLIENTS ) {
	if (!(hitEnt && trace.entityNum < ENTITYNUM_WORLD && (hitEnt->currentState.eType == ET_PLAYER || hitEnt->currentState.eType == ET_NPC))) {
		return;
	}

	// if the player is in fog, don't show it
	content = CG_PointContents( trace.endpos, 0 );
	if ( content & CONTENTS_FOG ) {
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
}

/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	float		*color;
	vec4_t		tcolor;
	char		*name;
	int			baseColor;
	qboolean	isVeh = qfalse;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
	//rww - still do the trace, our dynamic crosshair depends on it

	if (cg.crosshairClientNum < ENTITYNUM_WORLD)
	{
		centity_t *veh = &cg_entities[cg.crosshairClientNum];

		if (veh->currentState.eType == ET_NPC &&
			veh->currentState.NPC_class == CLASS_VEHICLE &&
			veh->currentState.owner < MAX_CLIENTS)
		{ //draw the name of the pilot then
			cg.crosshairClientNum = veh->currentState.owner;
			cg.crosshairVehNum = veh->currentState.number;
			cg.crosshairVehTime = cg.time;
			isVeh = qtrue; //so we know we're drawing the pilot's name
		}
	}

	if (cg.crosshairClientNum >= MAX_CLIENTS)
	{
		return;
	}

	if (cg_entities[cg.crosshairClientNum].currentState.powerups & (1 << PW_CLOAKED))
	{
		return;
	}

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, 1000 );
	if ( !color ) {
		trap->R_SetColor( NULL );
		return;
	}

	name = cgs.clientinfo[ cg.crosshairClientNum ].cleanname;

	if (cgs.gametype >= GT_TEAM)
	{
		//if (cgs.gametype == GT_SIEGE)
		if (1)
		{ //instead of team-based we'll make it oriented based on which team we're on
			if (cgs.clientinfo[cg.crosshairClientNum].team == cg.predictedPlayerState.persistant[PERS_TEAM])
			{
				baseColor = CT_GREEN;
			}
			else
			{
				baseColor = CT_RED;
			}
		}
		else
		{
			if (cgs.clientinfo[cg.crosshairClientNum].team == FACTION_EMPIRE)
			{
				baseColor = CT_RED;
			}
			else
			{
				baseColor = CT_BLUE;
			}
		}
	}
	else
	{
		//baseColor = CT_WHITE;
		if (cgs.gametype == GT_POWERDUEL &&
			cgs.clientinfo[cg.snap->ps.clientNum].team != FACTION_SPECTATOR &&
			cgs.clientinfo[cg.crosshairClientNum].duelTeam == cgs.clientinfo[cg.predictedPlayerState.clientNum].duelTeam)
		{ //on the same duel team in powerduel, so he's a friend
			baseColor = CT_GREEN;
		}
		else
		{
			baseColor = CT_RED; //just make it red in nonteam modes since everyone is hostile and crosshair will be red on them too
		}
	}

	if (cg.snap->ps.duelInProgress)
	{
		if (cg.crosshairClientNum != cg.snap->ps.duelIndex)
		{ //grey out crosshair for everyone but your foe if you're in a duel
			baseColor = CT_BLACK;
		}
	}
	else if (cg_entities[cg.crosshairClientNum].currentState.bolt1)
	{ //this fellow is in a duel. We just checked if we were in a duel above, so
	  //this means we aren't and he is. Which of course means our crosshair greys out over him.
		baseColor = CT_BLACK;
	}

	tcolor[0] = colorTable[baseColor][0];
	tcolor[1] = colorTable[baseColor][1];
	tcolor[2] = colorTable[baseColor][2];
	tcolor[3] = color[3]*0.5f;

	if (isVeh)
	{
		char str[MAX_STRING_CHARS];
		Com_sprintf(str, MAX_STRING_CHARS, "%s (pilot)", name);
		CG_DrawProportionalString(320, 170, str, UI_CENTER, tcolor);
	}
	else
	{
		CG_DrawProportionalString(320, 170, name, UI_CENTER, tcolor);
	}

	trap->R_SetColor( NULL );
}


//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void)
{
#if 0 // UQ1: This was annoying me...
	const char* s;

	s = CG_GetStringEdString("MP_INGAME", "SPECTATOR");
	if ((cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL) &&
		cgs.duelist1 != -1 &&
		cgs.duelist2 != -1)
	{
		char text[1024];
		int size = 64;

		if (cgs.gametype == GT_POWERDUEL && cgs.duelist3 != -1)
		{
			Com_sprintf(text, sizeof(text), "%s^7 %s %s^7 %s %s", cgs.clientinfo[cgs.duelist1].name, CG_GetStringEdString("MP_INGAME", "SPECHUD_VERSUS"), cgs.clientinfo[cgs.duelist2].name, CG_GetStringEdString("MP_INGAME", "AND"), cgs.clientinfo[cgs.duelist3].name);
		}
		else
		{
			Com_sprintf(text, sizeof(text), "%s^7 %s %s", cgs.clientinfo[cgs.duelist1].name, CG_GetStringEdString("MP_INGAME", "SPECHUD_VERSUS"), cgs.clientinfo[cgs.duelist2].name);
		}
		CG_Text_Paint ( 320 - CG_Text_Width ( text, 1.0f, 3 ) / 2, 420, 1.0f, colorWhite, text, 0, 0, 0, 3 );

		trap->R_SetColor( colorTable[CT_WHITE] );
		if ( cgs.clientinfo[cgs.duelist1].modelIcon )
		{
			CG_DrawPic( 10, SCREEN_HEIGHT-(size*1.5), size, size, cgs.clientinfo[cgs.duelist1].modelIcon );
		}
		if ( cgs.clientinfo[cgs.duelist2].modelIcon )
		{
			CG_DrawPic( SCREEN_WIDTH-size-10, SCREEN_HEIGHT-(size*1.5), size, size, cgs.clientinfo[cgs.duelist2].modelIcon );
		}

// nmckenzie: DUEL_HEALTH
		if (cgs.gametype == GT_DUEL)
		{
			if ( cgs.showDuelHealths >= 1)
			{	// draw the healths on the two guys - how does this interact with power duel, though?
				CG_DrawDuelistHealth ( 10, SCREEN_HEIGHT-(size*1.5) - 12, 64, 8, 1 );
				CG_DrawDuelistHealth ( SCREEN_WIDTH-size-10, SCREEN_HEIGHT-(size*1.5) - 12, 64, 8, 2 );
			}
		}

		if (cgs.gametype != GT_POWERDUEL)
		{
			Com_sprintf(text, sizeof(text), "%i/%i", cgs.clientinfo[cgs.duelist1].score, cgs.fraglimit );
			CG_Text_Paint( 42 - CG_Text_Width( text, 1.0f, 2 ) / 2, SCREEN_HEIGHT-(size*1.5) + 64, 1.0f, colorWhite, text, 0, 0, 0, 2 );

			Com_sprintf(text, sizeof(text), "%i/%i", cgs.clientinfo[cgs.duelist2].score, cgs.fraglimit );
			CG_Text_Paint( SCREEN_WIDTH-size+22 - CG_Text_Width( text, 1.0f, 2 ) / 2, SCREEN_HEIGHT-(size*1.5) + 64, 1.0f, colorWhite, text, 0, 0, 0, 2 );
		}

		if (cgs.gametype == GT_POWERDUEL && cgs.duelist3 != -1)
		{
			if ( cgs.clientinfo[cgs.duelist3].modelIcon )
			{
				CG_DrawPic( SCREEN_WIDTH-size-10, SCREEN_HEIGHT-(size*2.8), size, size, cgs.clientinfo[cgs.duelist3].modelIcon );
			}
		}
	}
	else
	{
		CG_Text_Paint ( 320 - CG_Text_Width ( s, 1.0f, 3 ) / 2, 420, 1.0f, colorWhite, s, 0, 0, 0, 3 );
	}

	if ( cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL )
	{
		s = CG_GetStringEdString("MP_INGAME", "WAITING_TO_PLAY");	// "waiting to play";
		CG_Text_Paint ( 320 - CG_Text_Width ( s, 1.0f, 3 ) / 2, 440, 1.0f, colorWhite, s, 0, 0, 0, 3 );
	}
	else //if ( cgs.gametype >= GT_TEAM )
	{
		//s = "press ESC and use the JOIN menu to play";
		s = CG_GetStringEdString("MP_INGAME", "SPEC_CHOOSEJOIN");
		CG_Text_Paint ( 320 - CG_Text_Width ( s, 1.0f, 3 ) / 2, 440, 1.0f, colorWhite, s, 0, 0, 0, 3 );
	}
#endif
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void) {
	const char *s = NULL, *sParm = NULL;
	int sec;
	char sYes[20] = {0}, sNo[20] = {0}, sVote[20] = {0}, sCmd[100] = {0};

	if ( !cgs.voteTime )
		return;

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
		trap->S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}

	if ( !Q_strncmp( cgs.voteString, "map_restart", 11 ) )
		trap->SE_GetStringTextString( "MENUS_RESTART_MAP", sCmd, sizeof( sCmd ) );
	else if ( !Q_strncmp( cgs.voteString, "vstr nextmap", 12 ) )
		trap->SE_GetStringTextString( "MENUS_NEXT_MAP", sCmd, sizeof( sCmd ) );
	else if ( !Q_strncmp( cgs.voteString, "g_doWarmup", 10 ) )
		trap->SE_GetStringTextString( "MENUS_WARMUP", sCmd, sizeof( sCmd ) );
	else if ( !Q_strncmp( cgs.voteString, "g_gametype", 10 ) ) {
		trap->SE_GetStringTextString( "MENUS_GAME_TYPE", sCmd, sizeof( sCmd ) );

			 if ( !Q_stricmp( "Free For All", cgs.voteString+11 ) )				sParm = CG_GetStringEdString( "MENUS", "FREE_FOR_ALL" );
		else if ( !Q_stricmp( "Duel", cgs.voteString+11 ) )						sParm = CG_GetStringEdString( "MENUS", "DUEL" );
		else if ( !Q_stricmp( "Holocron FFA", cgs.voteString+11 ) )				sParm = CG_GetStringEdString( "MENUS", "HOLOCRON_FFA" );
		else if ( !Q_stricmp( "Power Duel", cgs.voteString+11 ) )				sParm = CG_GetStringEdString( "MENUS", "POWERDUEL" );
		else if ( !Q_stricmp( "Instance", cgs.voteString+11 ) )					sParm = CG_GetStringEdString( "MENUS", "INSTANCE" );
		else if ( !Q_stricmp( "Cooperative", cgs.voteString+11 ) )				sParm = CG_GetStringEdString( "MENUS", "SINGLE_PLAYER" );
		else if ( !Q_stricmp( "Team FFA", cgs.voteString+11 ) ) 				sParm = CG_GetStringEdString( "MENUS", "TEAM_FFA" );
		else if ( !Q_stricmp( "Siege", cgs.voteString+11 ) )					sParm = CG_GetStringEdString( "MENUS", "SIEGE" );
		else if ( !Q_stricmp( "Capture the Flag", cgs.voteString+11 )  )		sParm = CG_GetStringEdString( "MENUS", "CAPTURE_THE_FLAG" );
		else if ( !Q_stricmp( "Capture the Ysalamiri", cgs.voteString+11 ) )	sParm = CG_GetStringEdString( "MENUS", "CAPTURE_THE_YSALIMARI" );
		else if ( !Q_stricmp( "War Zone", cgs.voteString+11 ) )					sParm = CG_GetStringEdString( "MENUS", "WARZONE" );
	}
	else if ( !Q_strncmp( cgs.voteString, "map", 3 ) ) {
		trap->SE_GetStringTextString( "MENUS_NEW_MAP", sCmd, sizeof( sCmd ) );
		sParm = cgs.voteString+4;
	}
	else if ( !Q_strncmp( cgs.voteString, "kick", 4 ) ) {
		trap->SE_GetStringTextString( "MENUS_KICK_PLAYER", sCmd, sizeof( sCmd ) );
		sParm = cgs.voteString+5;
	}
	else
	{// custom votes like ampoll, cointoss, etc
		sParm = cgs.voteString;
	}



	trap->SE_GetStringTextString( "MENUS_VOTE", sVote, sizeof( sVote ) );
	trap->SE_GetStringTextString( "MENUS_YES", sYes, sizeof( sYes ) );
	trap->SE_GetStringTextString( "MENUS_NO", sNo, sizeof( sNo ) );

	if (sParm && sParm[0])
		s = va( "%s(%i):<%s %s> %s:%i %s:%i", sVote, sec, sCmd, sParm, sYes, cgs.voteYes, sNo, cgs.voteNo);
	else
		s = va( "%s(%i):<%s> %s:%i %s:%i",    sVote, sec, sCmd,        sYes, cgs.voteYes, sNo, cgs.voteNo);
	CG_DrawSmallString( 4, 58, s, 1.0F );
	if ( cgs.clientinfo[cg.clientNum].team != FACTION_SPECTATOR ) {
		s = CG_GetStringEdString( "MP_INGAME", "OR_PRESS_ESC_THEN_CLICK_VOTE" );	//	s = "or press ESC then click Vote";
		CG_DrawSmallString( 4, 58 + SMALLCHAR_HEIGHT + 2, s, 1.0F );
	}
}

/*
=================
CG_DrawTeamVote
=================
*/
static void CG_DrawTeamVote(void) {
	char	*s;
	int		sec, cs_offset;

	if ( cgs.clientinfo[cg.clientNum].team == FACTION_EMPIRE )
		cs_offset = 0;
	else if ( cgs.clientinfo[cg.clientNum].team == FACTION_REBEL )
		cs_offset = 1;
	else if ( cgs.clientinfo[cg.clientNum].team == FACTION_MANDALORIAN )
		cs_offset = 2;
	else if ( cgs.clientinfo[cg.clientNum].team == FACTION_MERC )
		cs_offset = 3;
	else if (cgs.clientinfo[cg.clientNum].team == FACTION_PIRATES)
		cs_offset = 4;
	else if (cgs.clientinfo[cg.clientNum].team == FACTION_WILDLIFE)
		cs_offset = 5;
	else
		return;

	if ( !cgs.teamVoteTime[cs_offset] ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.teamVoteModified[cs_offset] ) {
		cgs.teamVoteModified[cs_offset] = qfalse;
//		trap->S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.teamVoteTime[cs_offset] ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	if (strstr(cgs.teamVoteString[cs_offset], "leader"))
	{
		int i = 0;

		while (cgs.teamVoteString[cs_offset][i] && cgs.teamVoteString[cs_offset][i] != ' ')
		{
			i++;
		}

		if (cgs.teamVoteString[cs_offset][i] == ' ')
		{
			int voteIndex = 0;
			char voteIndexStr[256];

			i++;

			while (cgs.teamVoteString[cs_offset][i])
			{
				voteIndexStr[voteIndex] = cgs.teamVoteString[cs_offset][i];
				voteIndex++;
				i++;
			}
			voteIndexStr[voteIndex] = 0;

			voteIndex = atoi(voteIndexStr);

			s = va("TEAMVOTE(%i):(Make %s the new team leader) yes:%i no:%i", sec, cgs.clientinfo[voteIndex].name,
									cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
		}
		else
		{
			s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
									cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
		}
	}
	else
	{
		s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
								cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
	}
	CG_DrawSmallString( 4, 90, s, 1.0F );
}

static qboolean CG_DrawScoreboard() {
	return CG_DrawOldScoreboard();
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
//	int key;
	//if (cg_singlePlayer.integer) {
	//	CG_DrawCenterString();
	//	return;
	//}
	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();
}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow( void )
{
	const char	*s;

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) )
	{
		return qfalse;
	}

//	s = "following";
	if (cgs.gametype == GT_POWERDUEL)
	{
		clientInfo_t *ci = &cgs.clientinfo[ cg.snap->ps.clientNum ];

		if (ci->duelTeam == DUELTEAM_LONE)
		{
			s = CG_GetStringEdString("MP_INGAME", "FOLLOWINGLONE");
		}
		else if (ci->duelTeam == DUELTEAM_DOUBLE)
		{
			s = CG_GetStringEdString("MP_INGAME", "FOLLOWINGDOUBLE");
		}
		else
		{
			s = CG_GetStringEdString("MP_INGAME", "FOLLOWING");
		}
	}
	else
	{
		s = CG_GetStringEdString("MP_INGAME", "FOLLOWING");
	}

	CG_Text_Paint ( 320 - CG_Text_Width ( s, 1.0f, FONT_MEDIUM ) / 2, 60, 1.0f, colorWhite, s, 0, 0, 0, FONT_MEDIUM );

	s = cgs.clientinfo[ cg.snap->ps.clientNum ].name;
	CG_Text_Paint ( 320 - CG_Text_Width ( s, 2.0f, FONT_MEDIUM ) / 2, 80, 2.0f, colorWhite, s, 0, 0, 0, FONT_MEDIUM );

	return qtrue;
}

#if 0
static void CG_DrawTemporaryStats()
{ //placeholder for testing (draws ammo and force power)
	char s[512];

	if (!cg.snap)
	{
		return;
	}

	sprintf(s, "Force: %i", cg.snap->ps.fd.forcePower);

	CG_DrawBigString(SCREEN_WIDTH-164, SCREEN_HEIGHT-dmgIndicSize, s, 1.0f);

	sprintf(s, "Ammo: %i", cg.snap->ps.ammo[weaponData[cg.snap->ps.weapon].ammoIndex]);

	CG_DrawBigString(SCREEN_WIDTH-164, SCREEN_HEIGHT-112, s, 1.0f);

	sprintf(s, "Health: %i", cg.snap->ps.stats[STAT_HEALTH]);

	CG_DrawBigString(8, SCREEN_HEIGHT-dmgIndicSize, s, 1.0f);

	sprintf(s, "Armor: %i", cg.snap->ps.stats[STAT_ARMOR]);

	CG_DrawBigString(8, SCREEN_HEIGHT-112, s, 1.0f);
}
#endif

/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning( void ) {
#if 0
	const char	*s;
	int			w;

	if (!cg_drawStatus.integer)
	{
		return;
	}

	if ( cg_drawAmmoWarning.integer == 0 ) {
		return;
	}

	if ( !cg.lowAmmoWarning ) {
		return;
	}

	if ( cg.lowAmmoWarning == 2 ) {
		s = "OUT OF AMMO";
	} else {
		s = "LOW AMMO WARNING";
	}
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w / 2, 64, s, 1.0F);
#endif
}



/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup( void ) {
	int			w, sec, i;
	float		scale;
	const char	*s;

	sec = cg.warmup;
	if ( !sec ) {
		return;
	}

	if ( sec < 0 ) {
//		s = "Waiting for players";
		s = CG_GetStringEdString("MP_INGAME", "WAITING_FOR_PLAYERS");
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawBigString(320 - w / 2, 24, s, 1.0F);
		cg.warmupCount = 0;
		return;
	}

	if (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL)
	{
		// find the two active players
		clientInfo_t	*ci1, *ci2, *ci3;

		ci1 = NULL;
		ci2 = NULL;
		ci3 = NULL;

		if (cgs.gametype == GT_POWERDUEL)
		{
			if (cgs.duelist1 != -1)
			{
				ci1 = &cgs.clientinfo[cgs.duelist1];
			}
			if (cgs.duelist2 != -1)
			{
				ci2 = &cgs.clientinfo[cgs.duelist2];
			}
			if (cgs.duelist3 != -1)
			{
				ci3 = &cgs.clientinfo[cgs.duelist3];
			}
		}
		else
		{
			for ( i = 0 ; i < cgs.maxclients ; i++ ) {
				if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == FACTION_FREE ) {
					if ( !ci1 ) {
						ci1 = &cgs.clientinfo[i];
					} else {
						ci2 = &cgs.clientinfo[i];
					}
				}
			}
		}
		if ( ci1 && ci2 )
		{
			if (ci3)
			{
				s = va( "%s vs %s and %s", ci1->name, ci2->name, ci3->name );
			}
			else
			{
				s = va( "%s vs %s", ci1->name, ci2->name );
			}
			w = CG_Text_Width(s, 0.6f, FONT_MEDIUM);
			CG_Text_Paint(320 - w / 2, 60, 0.6f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE,FONT_MEDIUM);
		}
	} else {
			 if ( cgs.gametype == GT_FFA )				s = CG_GetStringEdString("MENUS", "FREE_FOR_ALL");//"Free For All";
		else if ( cgs.gametype == GT_HOLOCRON )			s = CG_GetStringEdString("MENUS", "HOLOCRON_FFA");//"Holocron FFA";
		else if ( cgs.gametype == GT_JEDIMASTER )		s = "Jedi Master";
		else if ( cgs.gametype == GT_TEAM )				s = CG_GetStringEdString("MENUS", "TEAM_FFA");//"Team FFA";
		else if ( cgs.gametype == GT_SIEGE )			s = CG_GetStringEdString("MENUS", "SIEGE");//"Siege";
		else if ( cgs.gametype == GT_CTF )				s = CG_GetStringEdString("MENUS", "CAPTURE_THE_FLAG");//"Capture the Flag";
		else if ( cgs.gametype == GT_CTY )				s = CG_GetStringEdString("MENUS", "CAPTURE_THE_YSALIMARI");//"Capture the Ysalamiri";
		else if ( cgs.gametype == GT_SINGLE_PLAYER )	s = "Cooperative";
		else if ( cgs.gametype == GT_INSTANCE )			s = "Group Instance";
		else if ( cgs.gametype == GT_WARZONE )			s = "War Zone";
		else											s = "";
		w = CG_Text_Width(s, 1.5f, FONT_MEDIUM);
		CG_Text_Paint(320 - w / 2, 90, 1.5f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE,FONT_MEDIUM);
	}

	sec = ( sec - cg.time ) / 1000;
	if ( sec < 0 ) {
		cg.warmup = 0;
		sec = 0;
	}
//	s = va( "Starts in: %i", sec + 1 );
	s = va( "%s: %i",CG_GetStringEdString("MP_INGAME", "STARTS_IN"), sec + 1 );
	if ( sec != cg.warmupCount ) {
		cg.warmupCount = sec;

		if (cgs.gametype != GT_SIEGE)
		{
			switch ( sec ) {
			case 0:
				trap->S_StartLocalSound( cgs.media.count1Sound, CHAN_ANNOUNCER );
				break;
			case 1:
				trap->S_StartLocalSound( cgs.media.count2Sound, CHAN_ANNOUNCER );
				break;
			case 2:
				trap->S_StartLocalSound( cgs.media.count3Sound, CHAN_ANNOUNCER );
				break;
			default:
				break;
			}
		}
	}
	scale = 0.45f;
	switch ( cg.warmupCount ) {
	case 0:
		scale = 1.25f;
		break;
	case 1:
		scale = 1.15f;
		break;
	case 2:
		scale = 1.05f;
		break;
	default:
		scale = 0.9f;
		break;
	}

	w = CG_Text_Width(s, scale, FONT_MEDIUM);
	CG_Text_Paint(320 - w / 2, 125, scale, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);
}

//==================================================================================
/*
=================
CG_DrawTimedMenus
=================
*/
void CG_DrawTimedMenus() {
	if (cg.voiceTime) {
		int t = cg.time - cg.voiceTime;
		if ( t > 2500 ) {
			Menus_CloseByName("voiceMenu");
			trap->Cvar_Set("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}

void CG_DrawFlagStatus()
{
	int myFlagTakenShader = 0;
	int theirFlagShader = 0;
	int team = 0;
	int startDrawPos = 2;
	int ico_size = 32;

	//Raz: was missing this
	trap->R_SetColor( NULL );

	if (!cg.snap)
	{
		return;
	}

	if (cgs.gametype != GT_CTF && cgs.gametype != GT_CTY)
	{
		return;
	}

	team = cg.snap->ps.persistant[PERS_TEAM];

	if (cgs.gametype == GT_CTY)
	{
		if (team == FACTION_EMPIRE)
		{
			myFlagTakenShader = trap->R_RegisterShaderNoMip( "gfx/hud/mpi_rflag_x" );
			theirFlagShader = trap->R_RegisterShaderNoMip( "gfx/hud/mpi_bflag_ys" );
		}
		else
		{
			myFlagTakenShader = trap->R_RegisterShaderNoMip( "gfx/hud/mpi_bflag_x" );
			theirFlagShader = trap->R_RegisterShaderNoMip( "gfx/hud/mpi_rflag_ys" );
		}
	}
	else
	{
		if (team == FACTION_EMPIRE)
		{
			myFlagTakenShader = trap->R_RegisterShaderNoMip( "gfx/hud/mpi_rflag_x" );
			theirFlagShader = trap->R_RegisterShaderNoMip( "gfx/hud/mpi_bflag" );
		}
		else
		{
			myFlagTakenShader = trap->R_RegisterShaderNoMip( "gfx/hud/mpi_bflag_x" );
			theirFlagShader = trap->R_RegisterShaderNoMip( "gfx/hud/mpi_rflag" );
		}
	}

	if (CG_YourTeamHasFlag())
	{
		//CG_DrawPic( startDrawPos, 330, ico_size, ico_size, theirFlagShader );
		CG_DrawPic( 2, 330-startDrawPos, ico_size, ico_size, theirFlagShader );
		startDrawPos += ico_size+2;
	}

	if (CG_OtherTeamHasFlag())
	{
		//CG_DrawPic( startDrawPos, 330, ico_size, ico_size, myFlagTakenShader );
		CG_DrawPic( 2, 330-startDrawPos, ico_size, ico_size, myFlagTakenShader );
	}
}

//draw meter showing jetpack fuel when it's not full
#define JPFUELBAR_H			100.0f
#define JPFUELBAR_W			20.0f
#define JPFUELBAR_X			(SCREEN_WIDTH-JPFUELBAR_W-8.0f)
#define JPFUELBAR_Y			260.0f
void CG_DrawJetpackFuel(void)
{
	vec4_t aColor;
	vec4_t cColor;
	float x = JPFUELBAR_X;
	float y = JPFUELBAR_Y;
	float percent = ((float)cg.snap->ps.jetpackFuel/100.0f)*JPFUELBAR_H;

	if (percent > JPFUELBAR_H)
	{
		return;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	//color of the bar
	aColor[0] = 0.5f;
	aColor[1] = 0.0f;
	aColor[2] = 0.0f;
	aColor[3] = 0.8f;

	//color of greyed out "missing fuel"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, JPFUELBAR_W, JPFUELBAR_H, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f, y+1.0f+(JPFUELBAR_H-percent), JPFUELBAR_W-1.0f, JPFUELBAR_H-1.0f-(JPFUELBAR_H-percent), aColor);

	//then draw the other part greyed out
	CG_FillRect(x+1.0f, y+1.0f, JPFUELBAR_W-1.0f, JPFUELBAR_H-percent, cColor);
}

//draw meter showing e-web health when it is in use
#define EWEBHEALTH_H			100.0f
#define EWEBHEALTH_W			20.0f
#define EWEBHEALTH_X			(SCREEN_WIDTH-EWEBHEALTH_W-8.0f)
#define EWEBHEALTH_Y			290.0f
void CG_DrawEWebHealth(void)
{
	vec4_t aColor;
	vec4_t cColor;
	float x = EWEBHEALTH_X;
	float y = EWEBHEALTH_Y;
	centity_t *eweb = &cg_entities[cg.predictedPlayerState.emplacedIndex];
	float percent = ((float)eweb->currentState.health/eweb->currentState.maxhealth)*EWEBHEALTH_H;

	if (percent > EWEBHEALTH_H)
	{
		return;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	//kind of hacky, need to pass a coordinate in here
	if (cg.snap->ps.jetpackFuel < 100)
	{
		x -= (JPFUELBAR_W+8.0f);
	}
	if (cg.snap->ps.cloakFuel < 100)
	{
		x -= (JPFUELBAR_W+8.0f);
	}

	//color of the bar
	aColor[0] = 0.5f;
	aColor[1] = 0.0f;
	aColor[2] = 0.0f;
	aColor[3] = 0.8f;

	//color of greyed out "missing fuel"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, EWEBHEALTH_W, EWEBHEALTH_H, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f, y+1.0f+(EWEBHEALTH_H-percent), EWEBHEALTH_W-1.0f, EWEBHEALTH_H-1.0f-(EWEBHEALTH_H-percent), aColor);

	//then draw the other part greyed out
	CG_FillRect(x+1.0f, y+1.0f, EWEBHEALTH_W-1.0f, EWEBHEALTH_H-percent, cColor);
}

int cgRageTime = 0;
int cgRageFadeTime = 0;
float cgRageFadeVal = 0;

int cgRageRecTime = 0;
int cgRageRecFadeTime = 0;
float cgRageRecFadeVal = 0;

int cgAbsorbTime = 0;
int cgAbsorbFadeTime = 0;
float cgAbsorbFadeVal = 0;

int cgProtectTime = 0;
int cgProtectFadeTime = 0;
float cgProtectFadeVal = 0;

int cgYsalTime = 0;
int cgYsalFadeTime = 0;
float cgYsalFadeVal = 0;

qboolean gCGHasFallVector = qfalse;
vec3_t gCGFallVector;

/*
=================
CG_Draw2D
=================
*/
extern int cgSiegeRoundState;
extern int cgSiegeRoundTime;

extern int team1Timed;
extern int team2Timed;

int cg_beatingSiegeTime = 0;

int cgSiegeRoundBeganTime = 0;
int cgSiegeRoundCountTime = 0;

static void CG_DrawSiegeTimer(int timeRemaining, qboolean isMyTeam)
{ //rwwFIXMEFIXME: Make someone make assets and use them.
  //this function is pretty much totally placeholder.
//	int x = 0;
//	int y = SCREEN_HEIGHT-160;
	int fColor = 0;
	int minutes = 0;
	int seconds = 0;
	char timeStr[1024];
	menuDef_t	*menuHUD = NULL;
	itemDef_t	*item = NULL;

	menuHUD = Menus_FindByName("mp_timer");
	if (!menuHUD)
	{
		return;
	}

	item = Menu_FindItemByName(menuHUD, "frame");

	if (item)
	{
		trap->R_SetColor( item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}

	seconds = timeRemaining;

	while (seconds >= 60)
	{
		minutes++;
		seconds -= 60;
	}

	strcpy(timeStr, va( "%i:%02i", minutes, seconds ));

	if (isMyTeam)
	{
		fColor = CT_HUD_RED;
	}
	else
	{
		fColor = CT_HUD_GREEN;
	}

//	trap->Cvar_Set("ui_siegeTimer", timeStr);

//	CG_DrawProportionalString( x+16, y+40, timeStr, UI_SMALLFONT|UI_DROPSHADOW, colorTable[fColor] );

	item = Menu_FindItemByName(menuHUD, "timer");
	if (item)
	{
		CG_DrawProportionalString(
			item->window.rect.x,
			item->window.rect.y,
			timeStr,
			UI_SMALLFONT|UI_DROPSHADOW,
			colorTable[fColor] );
	}

}

static void CG_DrawSiegeDeathTimer( int timeRemaining )
{
	int minutes = 0;
	int seconds = 0;
	char timeStr[1024];
	menuDef_t	*menuHUD = NULL;
	itemDef_t	*item = NULL;

	menuHUD = Menus_FindByName("mp_timer");
	if (!menuHUD)
	{
		return;
	}

	item = Menu_FindItemByName(menuHUD, "frame");

	if (item)
	{
		trap->R_SetColor( item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}

	seconds = timeRemaining;

	while (seconds >= 60)
	{
		minutes++;
		seconds -= 60;
	}

	if (seconds < 10)
	{
		strcpy(timeStr, va( "%i:0%i", minutes, seconds ));
	}
	else
	{
		strcpy(timeStr, va( "%i:%i", minutes, seconds ));
	}

	item = Menu_FindItemByName(menuHUD, "deathtimer");
	if (item)
	{
		CG_DrawProportionalString(
			item->window.rect.x,
			item->window.rect.y,
			timeStr,
			UI_SMALLFONT|UI_DROPSHADOW,
			item->window.foreColor );
	}

}

int cgSiegeEntityRender = 0;

static void CG_DrawSiegeHUDItem(void)
{
	void *g2;
	qhandle_t handle;
	vec3_t origin, angles;
	vec3_t mins, maxs;
	float len;
	centity_t *cent = &cg_entities[cgSiegeEntityRender];

	if (cent->ghoul2)
	{
		g2 = cent->ghoul2;
		handle = 0;
	}
	else
	{
		handle = cgs.gameModels[cent->currentState.modelindex];
		g2 = NULL;
	}

	if (handle)
	{
		trap->R_ModelBounds( handle, mins, maxs );
	}
	else
	{
		VectorSet(mins, -16, -16, -20);
		VectorSet(maxs, 16, 16, 32);
	}

	origin[2] = -0.5 * ( mins[2] + maxs[2] );
	origin[1] = 0.5 * ( mins[1] + maxs[1] );
	len = 0.5 * ( maxs[2] - mins[2] );
	origin[0] = len / 0.268;

	VectorClear(angles);
	angles[YAW] = cg.autoAngles[YAW];

	CG_Draw3DModel( 8, 8, 64, 64, handle, g2, cent->currentState.g2radius, 0, origin, angles );

	cgSiegeEntityRender = 0; //reset for next frame
}

/*====================================
chatbox functionality -rww
====================================*/
#define	CHATBOX_CUTOFF_LEN	550
#define	CHATBOX_FONT_HEIGHT	20

//utility func, insert a string into a string at the specified
//place (assuming this will not overflow the buffer)
void CG_ChatBox_StrInsert(char *buffer, int place, char *str)
{
	int insLen = strlen(str);
	int i = strlen(buffer);
	int k = 0;

	buffer[i+insLen+1] = 0; //terminate the string at its new length
	while (i >= place)
	{
		buffer[i+insLen] = buffer[i];
		i--;
	}

	i++;
	while (k < insLen)
	{
		buffer[i] = str[k];
		i++;
		k++;
	}
}

//add chatbox string
void CG_ChatBox_AddString(char *chatStr)
{
	chatBoxItem_t *chat = &cg.chatItems[cg.chatItemActive];
	float chatLen;

	if (cg_chatBox.integer<=0)
	{ //don't bother then.
		return;
	}

	memset(chat, 0, sizeof(chatBoxItem_t));

	if (strlen(chatStr) > sizeof(chat->string))
	{ //too long, terminate at proper len.
		chatStr[sizeof(chat->string)-1] = 0;
	}

	strcpy(chat->string, chatStr);
	chat->time = cg.time + cg_chatBox.integer;

	chat->lines = 1;

	chatLen = CG_Text_Width(chat->string, 1.0f, FONT_SMALL);
	if (chatLen > CHATBOX_CUTOFF_LEN)
	{ //we have to break it into segments...
        int i = 0;
		int lastLinePt = 0;
		char s[2];

		chatLen = 0;
		while (chat->string[i])
		{
			s[0] = chat->string[i];
			s[1] = 0;
			chatLen += CG_Text_Width(s, 0.65f, FONT_SMALL);

			if (chatLen >= CHATBOX_CUTOFF_LEN)
			{
				int j = i;
				while (j > 0 && j > lastLinePt)
				{
					if (chat->string[j] == ' ')
					{
						break;
					}
					j--;
				}
				if (chat->string[j] == ' ')
				{
					i = j;
				}

                chat->lines++;
				CG_ChatBox_StrInsert(chat->string, i, "\n");
				i++;
				chatLen = 0;
				lastLinePt = i+1;
			}
			i++;
		}
	}

	cg.chatItemActive++;
	if (cg.chatItemActive >= MAX_CHATBOX_ITEMS)
	{
		cg.chatItemActive = 0;
	}
}

//insert item into array (rearranging the array if necessary)
void CG_ChatBox_ArrayInsert(chatBoxItem_t **array, int insPoint, int maxNum, chatBoxItem_t *item)
{
    if (array[insPoint])
	{ //recursively call, to move everything up to the top
		if (insPoint+1 >= maxNum)
		{
			trap->Error( ERR_DROP, "CG_ChatBox_ArrayInsert: Exceeded array size");
		}
		CG_ChatBox_ArrayInsert(array, insPoint+1, maxNum, array[insPoint]);
	}

	//now that we have moved anything that would be in this slot up, insert what we want into the slot
	array[insPoint] = item;
}

//go through all the chat strings and draw them if they are not yet expired
static QINLINE void CG_ChatBox_DrawStrings(void)
{
	chatBoxItem_t *drawThese[MAX_CHATBOX_ITEMS];
	int numToDraw = 0;
	int linesToDraw = 0;
	int i = 0;
	int x = 30;
	float y = cg.scoreBoardShowing ? 475 : cg_chatBoxHeight.integer;
	float fontScale = 0.65f;

	if (!cg_chatBox.integer)
	{
		return;
	}

	memset(drawThese, 0, sizeof(drawThese));

	while (i < MAX_CHATBOX_ITEMS)
	{
		if (cg.chatItems[i].time >= cg.time)
		{
			int check = numToDraw;
			int insertionPoint = numToDraw;

			while (check >= 0)
			{
				if (drawThese[check] &&
					cg.chatItems[i].time < drawThese[check]->time)
				{ //insert here
					insertionPoint = check;
				}
				check--;
			}
			CG_ChatBox_ArrayInsert(drawThese, insertionPoint, MAX_CHATBOX_ITEMS, &cg.chatItems[i]);
			numToDraw++;
			linesToDraw += cg.chatItems[i].lines;
		}
		i++;
	}

	if (!numToDraw)
	{ //nothing, then, just get out of here now.
		return;
	}

	//move initial point up so we draw bottom-up (visually)
	y -= (CHATBOX_FONT_HEIGHT*fontScale)*linesToDraw;

	//we have the items we want to draw, just quickly loop through them now
	i = 0;
	while (i < numToDraw)
	{
		CG_Text_Paint(x, y, fontScale, colorWhite, drawThese[i]->string, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		y += ((CHATBOX_FONT_HEIGHT*fontScale)*drawThese[i]->lines);
		i++;
	}
}

static void CG_Draw2DScreenTints( void )
{
	float			rageTime, rageRecTime, absorbTime, protectTime, ysalTime;
	vec4_t			hcolor;
	if (cgs.clientinfo[cg.snap->ps.clientNum].team != FACTION_SPECTATOR)
	{
		if (cg.snap->ps.fd.forcePowersActive & (1 << FP_RAGE))
		{
			if (!cgRageTime)
			{
				cgRageTime = cg.time;
			}

			rageTime = (float)(cg.time - cgRageTime);

			rageTime /= 9000;

			if (rageTime < 0)
			{
				rageTime = 0;
			}
			if (rageTime > 0.15)
			{
				rageTime = 0.15f;
			}

			hcolor[3] = rageTime;
			hcolor[0] = 0.7f;
			hcolor[1] = 0;
			hcolor[2] = 0;

			if (!cg.renderingThirdPerson)
			{
				CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor );
			}

			cgRageFadeTime = 0;
			cgRageFadeVal = 0;
		}
		else if (cgRageTime)
		{
			if (!cgRageFadeTime)
			{
				cgRageFadeTime = cg.time;
				cgRageFadeVal = 0.15f;
			}

			rageTime = cgRageFadeVal;

			cgRageFadeVal -= (cg.time - cgRageFadeTime)*0.000005;

			if (rageTime < 0)
			{
				rageTime = 0;
			}
			if (rageTime > 0.15f)
			{
				rageTime = 0.15f;
			}

			if (cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
			{
				float checkRageRecTime = rageTime;

				if (checkRageRecTime < 0.15f)
				{
					checkRageRecTime = 0.15f;
				}

				hcolor[3] = checkRageRecTime;
				hcolor[0] = rageTime*4;
				if (hcolor[0] < 0.2f)
				{
					hcolor[0] = 0.2f;
				}
				hcolor[1] = 0.2f;
				hcolor[2] = 0.2f;
			}
			else
			{
				hcolor[3] = rageTime;
				hcolor[0] = 0.7f;
				hcolor[1] = 0;
				hcolor[2] = 0;
			}

			if (!cg.renderingThirdPerson && rageTime)
			{
				CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor );
			}
			else
			{
				if (cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
				{
					hcolor[3] = 0.15f;
					hcolor[0] = 0.2f;
					hcolor[1] = 0.2f;
					hcolor[2] = 0.2f;
					CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor );
				}
				cgRageTime = 0;
			}
		}
		else if (cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
		{
			if (!cgRageRecTime)
			{
				cgRageRecTime = cg.time;
			}

			rageRecTime = (float)(cg.time - cgRageRecTime);

			rageRecTime /= 9000;

			if (rageRecTime < 0.15f)//0)
			{
				rageRecTime = 0.15f;//0;
			}
			if (rageRecTime > 0.15f)
			{
				rageRecTime = 0.15f;
			}

			hcolor[3] = rageRecTime;
			hcolor[0] = 0.2f;
			hcolor[1] = 0.2f;
			hcolor[2] = 0.2f;

			if (!cg.renderingThirdPerson)
			{
				CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor );
			}

			cgRageRecFadeTime = 0;
			cgRageRecFadeVal = 0;
		}
		else if (cgRageRecTime)
		{
			if (!cgRageRecFadeTime)
			{
				cgRageRecFadeTime = cg.time;
				cgRageRecFadeVal = 0.15f;
			}

			rageRecTime = cgRageRecFadeVal;

			cgRageRecFadeVal -= (cg.time - cgRageRecFadeTime)*0.000005;

			if (rageRecTime < 0)
			{
				rageRecTime = 0;
			}
			if (rageRecTime > 0.15f)
			{
				rageRecTime = 0.15f;
			}

			hcolor[3] = rageRecTime;
			hcolor[0] = 0.2f;
			hcolor[1] = 0.2f;
			hcolor[2] = 0.2f;

			if (!cg.renderingThirdPerson && rageRecTime)
			{
				CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor );
			}
			else
			{
				cgRageRecTime = 0;
			}
		}

		if (cg.snap->ps.fd.forcePowersActive & (1 << FP_ABSORB))
		{
			if (!cgAbsorbTime)
			{
				cgAbsorbTime = cg.time;
			}

			absorbTime = (float)(cg.time - cgAbsorbTime);

			absorbTime /= 9000;

			if (absorbTime < 0)
			{
				absorbTime = 0;
			}
			if (absorbTime > 0.15f)
			{
				absorbTime = 0.15f;
			}

			hcolor[3] = absorbTime/2;
			hcolor[0] = 0;
			hcolor[1] = 0;
			hcolor[2] = 0.7f;

			if (!cg.renderingThirdPerson)
			{
				CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor );
			}

			cgAbsorbFadeTime = 0;
			cgAbsorbFadeVal = 0;
		}
		else if (cgAbsorbTime)
		{
			if (!cgAbsorbFadeTime)
			{
				cgAbsorbFadeTime = cg.time;
				cgAbsorbFadeVal = 0.15f;
			}

			absorbTime = cgAbsorbFadeVal;

			cgAbsorbFadeVal -= (cg.time - cgAbsorbFadeTime)*0.000005f;

			if (absorbTime < 0)
			{
				absorbTime = 0;
			}
			if (absorbTime > 0.15f)
			{
				absorbTime = 0.15f;
			}

			hcolor[3] = absorbTime/2;
			hcolor[0] = 0;
			hcolor[1] = 0;
			hcolor[2] = 0.7f;

			if (!cg.renderingThirdPerson && absorbTime)
			{
				CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor );
			}
			else
			{
				cgAbsorbTime = 0;
			}
		}

		if (cg.snap->ps.fd.forcePowersActive & (1 << FP_PROTECT))
		{
			if (!cgProtectTime)
			{
				cgProtectTime = cg.time;
			}

			protectTime = (float)(cg.time - cgProtectTime);

			protectTime /= 9000;

			if (protectTime < 0)
			{
				protectTime = 0;
			}
			if (protectTime > 0.15f)
			{
				protectTime = 0.15f;
			}

			hcolor[3] = protectTime/2;
			hcolor[0] = 0;
			hcolor[1] = 0.7f;
			hcolor[2] = 0;

			if (!cg.renderingThirdPerson)
			{
				CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor );
			}

			cgProtectFadeTime = 0;
			cgProtectFadeVal = 0;
		}
		else if (cgProtectTime)
		{
			if (!cgProtectFadeTime)
			{
				cgProtectFadeTime = cg.time;
				cgProtectFadeVal = 0.15f;
			}

			protectTime = cgProtectFadeVal;

			cgProtectFadeVal -= (cg.time - cgProtectFadeTime)*0.000005;

			if (protectTime < 0)
			{
				protectTime = 0;
			}
			if (protectTime > 0.15f)
			{
				protectTime = 0.15f;
			}

			hcolor[3] = protectTime/2;
			hcolor[0] = 0;
			hcolor[1] = 0.7f;
			hcolor[2] = 0;

			if (!cg.renderingThirdPerson && protectTime)
			{
				CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor );
			}
			else
			{
				cgProtectTime = 0;
			}
		}

		if (cg.snap->ps.rocketLockIndex != ENTITYNUM_NONE && (cg.time - cg.snap->ps.rocketLockTime) > 0)
		{
			CG_DrawRocketLocking( cg.snap->ps.rocketLockIndex, cg.snap->ps.rocketLockTime );
		}

		if (BG_HasYsalamiri(cgs.gametype, &cg.snap->ps))
		{
			if (!cgYsalTime)
			{
				cgYsalTime = cg.time;
			}

			ysalTime = (float)(cg.time - cgYsalTime);

			ysalTime /= 9000;

			if (ysalTime < 0)
			{
				ysalTime = 0;
			}
			if (ysalTime > 0.15f)
			{
				ysalTime = 0.15f;
			}

			hcolor[3] = ysalTime/2;
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0;

			if (!cg.renderingThirdPerson)
			{
				CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor );
			}

			cgYsalFadeTime = 0;
			cgYsalFadeVal = 0;
		}
		else if (cgYsalTime)
		{
			if (!cgYsalFadeTime)
			{
				cgYsalFadeTime = cg.time;
				cgYsalFadeVal = 0.15f;
			}

			ysalTime = cgYsalFadeVal;

			cgYsalFadeVal -= (cg.time - cgYsalFadeTime)*0.000005f;

			if (ysalTime < 0)
			{
				ysalTime = 0;
			}
			if (ysalTime > 0.15f)
			{
				ysalTime = 0.15f;
			}

			hcolor[3] = ysalTime/2;
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0;

			if (!cg.renderingThirdPerson && ysalTime)
			{
				CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor );
			}
			else
			{
				cgYsalTime = 0;
			}
		}
	}

	if ( (cg.refdef.viewContents&CONTENTS_LAVA) )
	{//tint screen red
		float phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		hcolor[3] = 0.5 + (0.15f*sin( phase ));
		hcolor[0] = 0.7f;
		hcolor[1] = 0;
		hcolor[2] = 0;

		CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor );
	}
	else if ( (cg.refdef.viewContents&CONTENTS_SLIME) )
	{//tint screen green
		float phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		hcolor[3] = 0.4 + (0.1f*sin( phase ));
		hcolor[0] = 0;
		hcolor[1] = 0.7f;
		hcolor[2] = 0;

		CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor );
	}
	else if ( (cg.refdef.viewContents&CONTENTS_WATER) )
	{//tint screen light blue -- FIXME: don't do this if CONTENTS_FOG? (in case someone *does* make a water shader with fog in it?)
		float phase = cg.time / 1000.0f * WAVE_FREQUENCY * M_PI * 2;
		hcolor[3] = 0.3f + (0.05f*sinf( phase ));
		hcolor[0] = 0;
		hcolor[1] = 0.2f;
		hcolor[2] = 0.8f;

		CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor );
	}
}

void CG_Draw2D( void ) {
	float			inTime = cg.invenSelectTime+WEAPON_SELECT_TIME;
	float			wpTime = cg.weaponSelectTime+WEAPON_SELECT_TIME;
	float			fallTime;
	float			bestTime;
	int				drawSelect = 0;

	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == FACTION_SPECTATOR)
	{
		cgRageTime = 0;
		cgRageFadeTime = 0;
		cgRageFadeVal = 0;

		cgRageRecTime = 0;
		cgRageRecFadeTime = 0;
		cgRageRecFadeVal = 0;

		cgAbsorbTime = 0;
		cgAbsorbFadeTime = 0;
		cgAbsorbFadeVal = 0;

		cgProtectTime = 0;
		cgProtectFadeTime = 0;
		cgProtectFadeVal = 0;

		cgYsalTime = 0;
		cgYsalFadeTime = 0;
		cgYsalFadeVal = 0;
	}

	if ( !cg_draw2D.integer ) {
		gCGHasFallVector = qfalse;
		VectorClear( gCGFallVector );
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		CG_ChatBox_DrawStrings();
		return;
	}

	CG_Draw2DScreenTints();

	if (cg.snap->ps.rocketLockIndex != ENTITYNUM_NONE && (cg.time - cg.snap->ps.rocketLockTime) > 0)
	{
		CG_DrawRocketLocking( cg.snap->ps.rocketLockIndex, cg.snap->ps.rocketLockTime );
	}

	if (cg.snap->ps.holocronBits)
	{
		CG_DrawHolocronIcons();
	}
	if (cg.snap->ps.fd.forcePowersActive || cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
	{
		CG_DrawActivePowers();
	}
	//Stoiss add Fuel bar edit
	if (cg.snap->ps.jetpackFuel < 100)
	{ //draw it as long as it isn't full
		CG_DrawJetPackBar();//CG_DrawJetpackFuel();        
	}
	if (cg.snap->ps.cloakFuel < 100)
	{ //draw it as long as it isn't full
		CG_DrawCloakBar();//CG_DrawCloakFuel();
	}
	if (cg.predictedPlayerState.emplacedIndex > 0)
	{
		centity_t *eweb = &cg_entities[cg.predictedPlayerState.emplacedIndex];

		if (eweb->currentState.weapon == WP_NONE)
		{ //using an e-web, draw its health
			CG_DrawEWebHealth();
		}
	}

	// Draw this before the text so that any text won't get clipped off
	CG_DrawZoomMask();

/*
	if (cg.cameraMode) {
		return;
	}
*/
	if ( cg.snap->ps.persistant[PERS_TEAM] == FACTION_SPECTATOR ) {
		CG_DrawSpectator();
		CG_DrawCrosshair(NULL, 0);
		//CG_DrawCrosshairNames(); // UQ1: Now use NPC style display of names...
		CG_SaberClashFlare();
	} else {
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if ( !cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0 ) {

			if ( /*cg_drawStatus.integer*/0 ) {
				//Reenable if stats are drawn with menu system again
				Menu_PaintAll();
				CG_DrawTimedMenus();
			}

			//CG_DrawTemporaryStats();

			CG_DrawAmmoWarning();

			//CG_DrawCrosshairNames(); // UQ1: Now use NPC style display of names...
			CG_ScanForCrosshairEntity(); // UQ1: ADDED because of ^^^ to still show the crosshair...

			if (cg_drawStatus.integer)
			{
				CG_DrawIconBackground();
			}

			if (inTime > wpTime)
			{
				drawSelect = 1;
				bestTime = cg.invenSelectTime;
			}
			else //only draw the most recent since they're drawn in the same place
			{
				drawSelect = 2;
				bestTime = cg.weaponSelectTime;
			}

			if (cg.forceSelectTime > bestTime)
			{
				drawSelect = 3;
			}

			switch(drawSelect)
			{
			case 1:
				CG_DrawInvenSelect();
				break;
			case 2:
				CG_DrawWeaponSelect();
				break;
			case 3:
				CG_DrawForceSelect();
				break;
			default:
				break;
			}

			if (cg_drawStatus.integer)
			{
				//Powerups now done with upperright stuff
				//CG_DrawPowerupIcons();

				CG_DrawFlagStatus();
			}

			CG_SaberClashFlare();

			if (cg_drawStatus.integer)
			{
				CG_DrawStats();
			}

			CG_DrawPickupItem();
			//Do we want to use this system again at some point?
			//CG_DrawReward();
		}

	}

	if (cg.snap->ps.fallingToDeath)
	{
		vec4_t	hcolor;

		fallTime = (float)(cg.time - cg.snap->ps.fallingToDeath);

		fallTime /= (FALL_FADE_TIME/2);

		if (fallTime < 0)
		{
			fallTime = 0;
		}
		if (fallTime > 1)
		{
			fallTime = 1;
		}

		hcolor[3] = fallTime;
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 0;

		CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor );

		if (!gCGHasFallVector)
		{
			VectorCopy(cg.snap->ps.origin, gCGFallVector);
			gCGHasFallVector = qtrue;
		}
	}
	else
	{
		if (gCGHasFallVector)
		{
			gCGHasFallVector = qfalse;
			VectorClear(gCGFallVector);
		}
	}

	// UQ1: Added. Draw NPC civilian names over their heads...
	CG_DrawNPCNames();

	// Draw damages above head...
	CG_DrawDamage();

	CG_DrawVote();
	CG_DrawTeamVote();

	CG_DrawLagometer();


	if (!cl_paused.integer) {
		CG_DrawBracketedEntities();
		CG_DrawUpperRight();
	}

	if ( !CG_DrawFollow() ) {
		CG_DrawWarmup();
	}

	if (cgSiegeRoundState)
	{
		char pStr[1024];
		int rTime = 0;

		//cgSiegeRoundBeganTime = 0;

		switch (cgSiegeRoundState)
		{
		case 1:
			CG_CenterPrint(CG_GetStringEdString("MP_INGAME", "WAITING_FOR_PLAYERS"), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
			break;
		case 2:
			rTime = (SIEGE_ROUND_BEGIN_TIME - (cg.time - cgSiegeRoundTime));

			if (rTime < 0)
			{
				rTime = 0;
			}
			if (rTime > SIEGE_ROUND_BEGIN_TIME)
			{
				rTime = SIEGE_ROUND_BEGIN_TIME;
			}

			rTime /= 1000;

			rTime += 1;

			if (rTime < 1)
			{
				rTime = 1;
			}

			if (rTime <= 3 && rTime != cgSiegeRoundCountTime)
			{
				cgSiegeRoundCountTime = rTime;

				switch (rTime)
				{
				case 1:
					trap->S_StartLocalSound( cgs.media.count1Sound, CHAN_ANNOUNCER );
					break;
				case 2:
					trap->S_StartLocalSound( cgs.media.count2Sound, CHAN_ANNOUNCER );
					break;
				case 3:
					trap->S_StartLocalSound( cgs.media.count3Sound, CHAN_ANNOUNCER );
					break;
				default:
					break;
				}
			}

			Q_strncpyz(pStr, va("%s %i...", CG_GetStringEdString("MP_INGAME", "ROUNDBEGINSIN"), rTime), sizeof(pStr));
			CG_CenterPrint(pStr, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
			//same
			break;
		default:
			break;
		}

		cgSiegeEntityRender = 0;
	}
	else if (cgSiegeRoundTime)
	{
		CG_CenterPrint("", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
		cgSiegeRoundTime = 0;

		//cgSiegeRoundBeganTime = cg.time;
		cgSiegeEntityRender = 0;
	}
	else if (cgSiegeRoundBeganTime)
	{ //Draw how much time is left in the round based on local info.
		int timedTeam = FACTION_FREE;
		int timedValue = 0;

		if (cgSiegeEntityRender)
		{ //render the objective item model since this client has it
			CG_DrawSiegeHUDItem();
		}

		if (team1Timed)
		{
			timedTeam = FACTION_EMPIRE; //team 1
			if (cg_beatingSiegeTime)
			{
				timedValue = cg_beatingSiegeTime;
			}
			else
			{
				timedValue = team1Timed;
			}
		}
		else if (team2Timed)
		{
			timedTeam = FACTION_REBEL; //team 2
			if (cg_beatingSiegeTime)
			{
				timedValue = cg_beatingSiegeTime;
			}
			else
			{
				timedValue = team2Timed;
			}
		}

		if (timedTeam != FACTION_FREE)
		{ //one of the teams has a timer
			int timeRemaining;
			qboolean isMyTeam = qfalse;

			if (cgs.siegeTeamSwitch && !cg_beatingSiegeTime)
			{ //in switchy mode but not beating a time, so count up.
				timeRemaining = (cg.time-cgSiegeRoundBeganTime);
				if (timeRemaining < 0)
				{
					timeRemaining = 0;
				}
			}
			else
			{
				timeRemaining = (((cgSiegeRoundBeganTime)+timedValue) - cg.time);
			}

			if (timeRemaining > timedValue)
			{
				timeRemaining = timedValue;
			}
			else if (timeRemaining < 0)
			{
				timeRemaining = 0;
			}

			if (timeRemaining)
			{
				timeRemaining /= 1000;
			}

			if (cg.predictedPlayerState.persistant[PERS_TEAM] == timedTeam)
			{ //the team that's timed is the one this client is on
				isMyTeam = qtrue;
			}

			CG_DrawSiegeTimer(timeRemaining, isMyTeam);
		}
	}
	else
	{
		cgSiegeEntityRender = 0;
	}

	if ( cg_siegeDeathTime )
	{
		int timeRemaining = ( cg_siegeDeathTime - cg.time );

		if ( timeRemaining < 0 )
		{
			timeRemaining = 0;
			cg_siegeDeathTime = 0;
		}

		if ( timeRemaining )
		{
			timeRemaining /= 1000;
		}

		CG_DrawSiegeDeathTimer( timeRemaining );
	}

	// don't draw center string if scoreboard is up
	cg.scoreBoardShowing = CG_DrawScoreboard();
	if ( !cg.scoreBoardShowing) {
		CG_DrawCenterString();
	}

	// always draw chat
	CG_ChatBox_DrawStrings();
}

qboolean CG_CullPointAndRadius( const vec3_t pt, float radius);
void CG_DrawMiscStaticModels( void ) {
	int i, j;
	refEntity_t ent;
	vec3_t cullorg;
	vec3_t diff;

	memset( &ent, 0, sizeof( ent ) );

	ent.reType = RT_MODEL;

	ent.frame = 0;
	ent.nonNormalizedAxes = qtrue;
	VectorSet(ent.modelScale, 1.0, 1.0, 1.0);

	// static models don't project shadows
	ent.renderfx = RF_NOSHADOW;

	for( i = 0; i < cgs.numMiscStaticModels; i++ ) {
		if (cgs.miscStaticModels[i].useInstancing)
		{
			ent.reType = RT_MODEL_INSTANCED;
			VectorCopy(cgs.miscStaticModels[i].modelScale, ent.modelScale);
		}
		else
		{
			ent.reType = RT_MODEL;
			//VectorSet(ent.modelScale, 1.0, 1.0, 1.0);
			VectorCopy(cgs.miscStaticModels[i].modelScale, ent.modelScale);
		}

		VectorCopy(cgs.miscStaticModels[i].org, cullorg);
		cullorg[2] += 1.0f;

		if ( cgs.miscStaticModels[i].zoffset ) {
			cullorg[2] += cgs.miscStaticModels[i].zoffset;
		}
		if( cgs.miscStaticModels[i].radius ) {
			if( CG_CullPointAndRadius( cullorg, cgs.miscStaticModels[i].radius ) ) {
 				continue;
			}
		}

		if( !trap->R_InPVS( cg.refdef.vieworg, cullorg, cg.refdef.areamask ) ) {
			continue;
		}

		VectorCopy( cgs.miscStaticModels[i].org, ent.origin );
		VectorCopy( cgs.miscStaticModels[i].org, ent.oldorigin );
		VectorCopy( cgs.miscStaticModels[i].org, ent.lightingOrigin );

		for( j = 0; j < 3; j++ ) {
			VectorCopy( cgs.miscStaticModels[i].axes[j], ent.axis[j] );
		}
		ent.hModel = cgs.miscStaticModels[i].model;
		ent.customShader = cgs.miscStaticModels[i].overrideShader;

		VectorSubtract(ent.origin, cg.refdef.vieworg, diff);
		if (VectorLength(diff)-(cgs.miscStaticModels[i].radius) <= cg.distanceCull) {
			AddRefEntityToScene(&ent);
		}
	}
}

static void CG_DrawTourneyScoreboard() {
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/

void CG_DrawActive( stereoFrame_t stereoView ) {
	float		separation;
	vec3_t		baseOrg;

	// optionally draw the info screen instead
	if ( !drawingSniperScopeView && !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	separation = 0;

	// clear around the rendered view if sized down
	if (!drawingSniperScopeView)
		CG_TileClear();

	// offset vieworg appropriately if we're doing stereo separation
	VectorCopy( cg.refdef.vieworg, baseOrg );
	if ( separation != 0 ) {
		VectorMA( cg.refdef.vieworg, -separation, cg.refdef.viewaxis[1], cg.refdef.vieworg );
	}

	cg.refdef.rdflags |= RDF_DRAWSKYBOX;

	CG_DrawMiscStaticModels();

	// draw 3D view
	trap->R_RenderScene( &cg.refdef );

	// restore original viewpoint if running stereo
	if ( separation != 0 ) {
		VectorCopy( baseOrg, cg.refdef.vieworg );
	}

	/*if (!drawingSniperScopeView)
	{
		// draw status bar and other floating elements
 		CG_Draw2D();
	}*/
}



