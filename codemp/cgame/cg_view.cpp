// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_view.c -- setup all the parameters (position, angle, etc)
// for a 3D rendering
#include "cg_local.h"
#include "game/bg_saga.h"

#ifndef __DISABLE_PLAYERCLIP__
#define MASK_CAMERACLIP (MASK_SOLID|CONTENTS_PLAYERCLIP)
#else //__DISABLE_PLAYERCLIP__
#define MASK_CAMERACLIP (MASK_SOLID)
#endif //__DISABLE_PLAYERCLIP__
#define CAMERA_SIZE	4


#define __SCOPE_DEVELOPER__


/*
=============================================================================

  MODEL TESTING

The viewthing and gun positioning tools from Q2 have been integrated and
enhanced into a single model testing facility.

Model viewing can begin with either "testmodel <modelname>" or "testgun <modelname>".

The names must be the full pathname after the basedir, like
"models/weapons/v_launch/tris.md3" or "players/male/tris.md3"

Testmodel will create a fake entity 100 units in front of the current view
position, directly facing the viewer.  It will remain immobile, so you can
move around it to view it from different angles.

Testgun will cause the model to follow the player around and supress the real
view weapon model.  The default frame 0 of most guns is completely off screen,
so you will probably have to cycle a couple frames to see it.

"nextframe", "prevframe", "nextskin", and "prevskin" commands will change the
frame or skin of the testmodel.  These are bound to F5, F6, F7, and F8 in
q3default.cfg.

If a gun is being tested, the "gun_x", "gun_y", and "gun_z" variables will let
you adjust the positioning.

Note that none of the model testing features update while the game is paused, so
it may be convenient to test with deathmatch set to 1 so that bringing down the
console doesn't pause the game.

=============================================================================
*/

extern float mix(float x, float y, float a);

#define BASE_MIN_HEIGHT1 -48.0
#define BASE_MIN_HEIGHT2 -16.0

typedef enum
{
	A_NULL = 0,
	A_SHIFT,
	A_CTRL,
	A_ALT,
	A_CAPSLOCK,
	A_NUMLOCK,
	A_SCROLLLOCK,
	A_PAUSE,
	A_BACKSPACE,
	A_TAB,
	A_ENTER,
	A_KP_PLUS,
	A_KP_MINUS,
	A_KP_ENTER,
	A_KP_PERIOD,
	A_PRINTSCREEN,
	A_KP_0,
	A_KP_1,
	A_KP_2,
	A_KP_3,
	A_KP_4,
	A_KP_5,
	A_KP_6,
	A_KP_7,
	A_KP_8,
	A_KP_9,
	A_CONSOLE,
	A_ESCAPE,
	A_F1,
	A_F2,
	A_F3,
	A_F4,

	A_SPACE,
	A_PLING,
	A_DOUBLE_QUOTE,
	A_HASH,
	A_STRING,
	A_PERCENT,
	A_AND,
	A_SINGLE_QUOTE,
	A_OPEN_BRACKET,
	A_CLOSE_BRACKET,
	A_STAR,
	A_PLUS,
	A_COMMA,
	A_MINUS,
	A_PERIOD,
	A_FORWARD_SLASH,
	A_0,
	A_1,
	A_2,
	A_3,
	A_4,
	A_5,
	A_6,
	A_7,
	A_8,
	A_9,
	A_COLON,
	A_SEMICOLON,
	A_LESSTHAN,
	A_EQUALS,
	A_GREATERTHAN,
	A_QUESTION,

	A_AT,
	A_CAP_A,
	A_CAP_B,
	A_CAP_C,
	A_CAP_D,
	A_CAP_E,
	A_CAP_F,
	A_CAP_G,
	A_CAP_H,
	A_CAP_I,
	A_CAP_J,
	A_CAP_K,
	A_CAP_L,
	A_CAP_M,
	A_CAP_N,
	A_CAP_O,
	A_CAP_P,
	A_CAP_Q,
	A_CAP_R,
	A_CAP_S,
	A_CAP_T,
	A_CAP_U,
	A_CAP_V,
	A_CAP_W,
	A_CAP_X,
	A_CAP_Y,
	A_CAP_Z,
	A_OPEN_SQUARE,
	A_BACKSLASH,
	A_CLOSE_SQUARE,
	A_CARET,
	A_UNDERSCORE,

	A_LEFT_SINGLE_QUOTE,
	A_LOW_A,
	A_LOW_B,
	A_LOW_C,
	A_LOW_D,
	A_LOW_E,
	A_LOW_F,
	A_LOW_G,
	A_LOW_H,
	A_LOW_I,
	A_LOW_J,
	A_LOW_K,
	A_LOW_L,
	A_LOW_M,
	A_LOW_N,
	A_LOW_O,
	A_LOW_P,
	A_LOW_Q,
	A_LOW_R,
	A_LOW_S,
	A_LOW_T,
	A_LOW_U,
	A_LOW_V,
	A_LOW_W,
	A_LOW_X,
	A_LOW_Y,
	A_LOW_Z,
	A_OPEN_BRACE,
	A_BAR,
	A_CLOSE_BRACE,
	A_TILDE,
	A_DELETE,

	A_EURO,
	A_SHIFT2,
	A_CTRL2,
	A_ALT2,
	A_F5,
	A_F6,
	A_F7,
	A_F8,
	A_CIRCUMFLEX,
	A_MWHEELUP,
	A_CAP_SCARON,
	A_MWHEELDOWN,
	A_CAP_OE,
	A_MOUSE1,
	A_MOUSE2,
	A_INSERT,
	A_HOME,
	A_PAGE_UP,
	A_RIGHT_SINGLE_QUOTE,
	A_LEFT_DOUBLE_QUOTE,
	A_RIGHT_DOUBLE_QUOTE,
	A_F9,
	A_F10,
	A_F11,
	A_F12,
	A_TRADEMARK,
	A_LOW_SCARON,
	A_SHIFT_ENTER,
	A_LOW_OE,
	A_END,
	A_PAGE_DOWN,
	A_CAP_YDIERESIS,

	A_SHIFT_SPACE,
	A_EXCLAMDOWN,
	A_CENT,
	A_POUND,
	A_SHIFT_KP_ENTER,
	A_YEN,
	A_MOUSE3,
	A_MOUSE4,
	A_MOUSE5,
	A_COPYRIGHT,
	A_CURSOR_UP,
	A_CURSOR_DOWN,
	A_CURSOR_LEFT,
	A_CURSOR_RIGHT,
	A_REGISTERED,
	A_UNDEFINED_7,
	A_UNDEFINED_8,
	A_UNDEFINED_9,
	A_UNDEFINED_10,
	A_UNDEFINED_11,
	A_UNDEFINED_12,
	A_UNDEFINED_13,
	A_UNDEFINED_14,
	A_UNDEFINED_15,
	A_UNDEFINED_16,
	A_UNDEFINED_17,
	A_UNDEFINED_18,
	A_UNDEFINED_19,
	A_UNDEFINED_20,
	A_UNDEFINED_21,
	A_UNDEFINED_22,
	A_QUESTION_DOWN,

	A_CAP_AGRAVE,
	A_CAP_AACUTE,
	A_CAP_ACIRCUMFLEX,
	A_CAP_ATILDE,
	A_CAP_ADIERESIS,
	A_CAP_ARING,
	A_CAP_AE,
	A_CAP_CCEDILLA,
	A_CAP_EGRAVE,
	A_CAP_EACUTE,
	A_CAP_ECIRCUMFLEX,
	A_CAP_EDIERESIS,
	A_CAP_IGRAVE,
	A_CAP_IACUTE,
	A_CAP_ICIRCUMFLEX,
	A_CAP_IDIERESIS,
	A_CAP_ETH,
	A_CAP_NTILDE,
	A_CAP_OGRAVE,
	A_CAP_OACUTE,
	A_CAP_OCIRCUMFLEX,
	A_CAP_OTILDE,
	A_CAP_ODIERESIS,
	A_MULTIPLY,
	A_CAP_OSLASH,
	A_CAP_UGRAVE,
	A_CAP_UACUTE,
	A_CAP_UCIRCUMFLEX,
	A_CAP_UDIERESIS,
	A_CAP_YACUTE,
	A_CAP_THORN,
	A_GERMANDBLS,

	A_LOW_AGRAVE,
	A_LOW_AACUTE,
	A_LOW_ACIRCUMFLEX,
	A_LOW_ATILDE,
	A_LOW_ADIERESIS,
	A_LOW_ARING,
	A_LOW_AE,
	A_LOW_CCEDILLA,
	A_LOW_EGRAVE,
	A_LOW_EACUTE,
	A_LOW_ECIRCUMFLEX,
	A_LOW_EDIERESIS,
	A_LOW_IGRAVE,
	A_LOW_IACUTE,
	A_LOW_ICIRCUMFLEX,
	A_LOW_IDIERESIS,
	A_LOW_ETH,
	A_LOW_NTILDE,
	A_LOW_OGRAVE,
	A_LOW_OACUTE,
	A_LOW_OCIRCUMFLEX,
	A_LOW_OTILDE,
	A_LOW_ODIERESIS,
	A_DIVIDE,
	A_LOW_OSLASH,
	A_LOW_UGRAVE,
	A_LOW_UACUTE,
	A_LOW_UCIRCUMFLEX,
	A_LOW_UDIERESIS,
	A_LOW_YACUTE,
	A_LOW_THORN,
	A_LOW_YDIERESIS,

	A_JOY0,
	A_JOY1,
	A_JOY2,
	A_JOY3,
	A_JOY4,
	A_JOY5,
	A_JOY6,
	A_JOY7,
	A_JOY8,
	A_JOY9,
	A_JOY10,
	A_JOY11,
	A_JOY12,
	A_JOY13,
	A_JOY14,
	A_JOY15,
	A_JOY16,
	A_JOY17,
	A_JOY18,
	A_JOY19,
	A_JOY20,
	A_JOY21,
	A_JOY22,
	A_JOY23,
	A_JOY24,
	A_JOY25,
	A_JOY26,
	A_JOY27,
	A_JOY28,
	A_JOY29,
	A_JOY30,
	A_JOY31,

	A_AUX0,
	A_AUX1,
	A_AUX2,
	A_AUX3,
	A_AUX4,
	A_AUX5,
	A_AUX6,
	A_AUX7,
	A_AUX8,
	A_AUX9,
	A_AUX10,
	A_AUX11,
	A_AUX12,
	A_AUX13,
	A_AUX14,
	A_AUX15,
	A_AUX16,
	A_AUX17,
	A_AUX18,
	A_AUX19,
	A_AUX20,
	A_AUX21,
	A_AUX22,
	A_AUX23,
	A_AUX24,
	A_AUX25,
	A_AUX26,
	A_AUX27,
	A_AUX28,
	A_AUX29,
	A_AUX30,
	A_AUX31,

	MAX_KEYS
} fakeAscii_t;

float wantedCameraAngle = 0;
float currentCameraAngle = 0;

void CG_UpdateCamera(void)
{
	/*if (trap->Key_IsDown(A_MOUSE3))
	{// Allow camera rotation...
		trap->Cvar_Set("cg_cameraOrbit", "5");
	}
	else
	{
		trap->Cvar_Set("cg_cameraOrbit", "0");
	}*/

	// Set new view height...
	float distMod = Q_clamp(0.0, float(cg_thirdPersonRange.integer - cg_thirdPersonRangeMinimum.integer) / float(cg_thirdPersonRangeMaximum.integer - cg_thirdPersonRangeMinimum.integer), 1.0);
	float modelScale = (cg.snap && cg.snap->ps.iModelScale / 100.0f);
	int hRange = (int)mix(BASE_MIN_HEIGHT1, BASE_MIN_HEIGHT2, modelScale);
	hRange = (int)mix((float)hRange, (float)hRange + 48.0, distMod);

	char val2[16] = { { 0 } };
	sprintf(val2, "%i", hRange);
	trap->Cvar_Set("cg_thirdPersonVertOffset", val2);

	if (cg.snap && cg.snap->ps.eFlags & EF_PAIRED_ANIMATION)
	{
#define CAMERA_MOVE_SPEED 0.5
		wantedCameraAngle = (currentCameraAngle - cg_entities[cg.snap->ps.clientNum].lerpAngles[YAW]) + float(double(cg.time) * 0.0333);//360.0 - cg_entities[cg.snap->ps.clientNum].lerpAngles[YAW];

		/*if (wantedCameraAngle < 0.0)
		{
			wantedCameraAngle *= -1.0;
		}*/

		if (currentCameraAngle > wantedCameraAngle)
		{
			currentCameraAngle -= CAMERA_MOVE_SPEED;
		}
		else if (currentCameraAngle < wantedCameraAngle)
		{
			currentCameraAngle += CAMERA_MOVE_SPEED;
		}

		float diff = currentCameraAngle - wantedCameraAngle;

		if (diff < 0.0) diff *= -1.0;

		if (diff < CAMERA_MOVE_SPEED)
		{
			currentCameraAngle = wantedCameraAngle;
		}

		char val3[16] = { { 0 } };
		sprintf(val3, "%f", wantedCameraAngle - currentCameraAngle);
		trap->Cvar_Set("cg_thirdPersonAngle", val3);

		//trap->Print("cam angle %f. wanted %f.\n", currentCameraAngle, wantedCameraAngle);
	}
	else
	{
		trap->Cvar_Set("cg_thirdPersonAngle", "0");
	}

#if 0
	if (cg_entities[cg.snap->ps.clientNum].currentState.weapon == WP_SABER)
	{
#define CAMERA_CATCHUP_SPEED 0.5//cg_testvalue1.value

		wantedCameraAngle = cg_entities[cg.snap->ps.clientNum].lerpAngles[YAW];

		if (wantedCameraAngle < 0.0)
		{
			wantedCameraAngle *= -1.0;
		}
		
		if (currentCameraAngle > wantedCameraAngle)
		{
			currentCameraAngle -= CAMERA_CATCHUP_SPEED;
		}
		else if (currentCameraAngle < wantedCameraAngle)
		{
			currentCameraAngle += CAMERA_CATCHUP_SPEED;
		}

		float diff = currentCameraAngle - wantedCameraAngle;
		
		if (diff < 0.0) diff *= -1.0;

		if (diff < CAMERA_CATCHUP_SPEED)
		{
			currentCameraAngle = wantedCameraAngle;
		}

		char val3[16] = { { 0 } };
		sprintf(val3, "%f", wantedCameraAngle - currentCameraAngle);
		trap->Cvar_Set("cg_thirdPersonAngle", val3);

		//trap->Print("cam angle %f. wanted %f.\n", currentCameraAngle, wantedCameraAngle);
	}
	else
	{
		trap->Cvar_Set("cg_thirdPersonAngle", "0");
	}
#endif
}

void CG_ScrollDown_f(void)
{// Mouse scroll down... Zoom out camera... Use for other stuff later, maybe...
	// Calculate and update view range...
	int vRange = cg_thirdPersonRange.integer;

	vRange += cg_thirdPersonRangeScrollSpeed.integer;
	vRange = Q_clampi(cg_thirdPersonRangeMinimum.integer, vRange, cg_thirdPersonRangeMaximum.integer);

	char val[16] = { { 0 } };
	sprintf(val, "%i", vRange);
	trap->Cvar_Set("cg_thirdPersonRange", val);

	CG_UpdateCamera();
}

void CG_ScrollUp_f(void)
{// Mouse scroll up... Zoom in camera... Use for other stuff later, maybe...
	// Calculate and update view range...
	int vRange = cg_thirdPersonRange.integer;

	vRange -= cg_thirdPersonRangeScrollSpeed.integer;
	vRange = Q_clampi(cg_thirdPersonRangeMinimum.integer, vRange, cg_thirdPersonRangeMaximum.integer);

	char val[16] = { { 0 } };
	sprintf(val, "%i", vRange);
	trap->Cvar_Set("cg_thirdPersonRange", val);

	CG_UpdateCamera();
}

/*
=================
CG_TestModel_f

Creates an entity in front of the current position, which
can then be moved around
=================
*/
void CG_TestModel_f (void) {
	vec3_t		angles;

	memset( &cg.testModelEntity, 0, sizeof(cg.testModelEntity) );
	if ( trap->Cmd_Argc() < 2 ) {
		return;
	}

	Q_strncpyz (cg.testModelName, CG_Argv( 1 ), MAX_QPATH );
	cg.testModelEntity.hModel = trap->R_RegisterModel( cg.testModelName );

	if ( trap->Cmd_Argc() == 3 ) {
		cg.testModelEntity.backlerp = atof( CG_Argv( 2 ) );
		cg.testModelEntity.frame = 1;
		cg.testModelEntity.oldframe = 0;
	}
	if (! cg.testModelEntity.hModel ) {
		trap->Print( "Can't register model\n" );
		return;
	}

	VectorMA( cg.refdef.vieworg, 100, cg.refdef.viewaxis[0], cg.testModelEntity.origin );

#ifdef __OLD_VR__
	angles[PITCH] = 0;
	angles[YAW] = 180 + cg.refdefViewAngles[1];
	angles[ROLL] = 0;
#else //!__OLD_VR__
	angles[PITCH] = 0;
	angles[YAW] = 180 + cg.refdef.viewangles[1];
	angles[ROLL] = 0;
#endif //__OLD_VR__

	AnglesToAxis( angles, cg.testModelEntity.axis );
	cg.testGun = qfalse;
}

/*
=================
CG_TestGun_f

Replaces the current view weapon with the given model
=================
*/
void CG_TestGun_f (void) {
	CG_TestModel_f();
	cg.testGun = qtrue;
	//cg.testModelEntity.renderfx = RF_MINLIGHT | RF_DEPTHHACK | RF_FIRST_PERSON;

	// rww - 9-13-01 [1-26-01-sof2]
	cg.testModelEntity.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;
}


void CG_TestModelNextFrame_f (void) {
	cg.testModelEntity.frame++;
	trap->Print( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelPrevFrame_f (void) {
	cg.testModelEntity.frame--;
	if ( cg.testModelEntity.frame < 0 ) {
		cg.testModelEntity.frame = 0;
	}
	trap->Print( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelNextSkin_f (void) {
	cg.testModelEntity.skinNum++;
	trap->Print( "skin %i\n", cg.testModelEntity.skinNum );
}

void CG_TestModelPrevSkin_f (void) {
	cg.testModelEntity.skinNum--;
	if ( cg.testModelEntity.skinNum < 0 ) {
		cg.testModelEntity.skinNum = 0;
	}
	trap->Print( "skin %i\n", cg.testModelEntity.skinNum );
}

static void CG_AddTestModel (void) {
	int		i;

	// re-register the model, because the level may have changed
	cg.testModelEntity.hModel = trap->R_RegisterModel( cg.testModelName );
	if (! cg.testModelEntity.hModel ) {
		trap->Print ("Can't register model\n");
		return;
	}

	// if testing a gun, set the origin reletive to the view origin
	if ( cg.testGun ) {
		VectorCopy( cg.refdef.vieworg, cg.testModelEntity.origin );
		VectorCopy( cg.refdef.viewaxis[0], cg.testModelEntity.axis[0] );
		VectorCopy( cg.refdef.viewaxis[1], cg.testModelEntity.axis[1] );
		VectorCopy( cg.refdef.viewaxis[2], cg.testModelEntity.axis[2] );

		// allow the position to be adjusted
		for (i=0 ; i<3 ; i++) {
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[0][i] * cg_gunX.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[1][i] * cg_gunY.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[2][i] * cg_gunZ.value;
		}
	}

	AddRefEntityToScene( &cg.testModelEntity );
}



//============================================================================


/*
=================
CG_CalcVrect

Sets the coordinates of the rendered window
=================
*/
static void CG_CalcVrect (void) {
	int		size;

	// the intermission should allways be full screen
	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		size = 100;
	} else {
		// bound normal viewsize
		if ( cg_viewsize.integer < 30 ) {
			trap->Cvar_Set( "cg_viewsize", "30" );
			trap->Cvar_Update( &cg_viewsize );
			size = 30;
		} else if ( cg_viewsize.integer > 100 ) {
			trap->Cvar_Set( "cg_viewsize", "100" );
			trap->Cvar_Update( &cg_viewsize );
			size = 100;
		} else {
			size = cg_viewsize.integer;
		}

	}
	cg.refdef.width = cgs.glconfig.vidWidth*size/100;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight*size/100;
	cg.refdef.height &= ~1;

	cg.refdef.x = (cgs.glconfig.vidWidth - cg.refdef.width)/2;
	cg.refdef.y = (cgs.glconfig.vidHeight - cg.refdef.height)/2;
}

//==============================================================================

#ifdef __OLD_VR__
/*
===============
CG_OffsetVRView

===============
*/
#define	FOCUS_DISTANCE	512
static void CG_OffsetVRView(void) {
	vec3_t		forward, right, up;
	vec3_t      focusUp, focusRight;
	vec3_t		view;
	//vec3_t      headVec;
	vec3_t		focusAngles;
	//trace_t		trace;
	static vec3_t	mins = { -4, -4, -4 };
	static vec3_t	maxs = { 4, 4, 4 };
	vec3_t		focusPoint;
	float		focusDist;
	//float		forwardScale, sideScale;

	centity_t *predictedPlayerEntity = &cg_entities[cg.predictedPlayerState.clientNum];

#if 0
	cg.refdef.vieworg[0] += (predictedPlayerEntity->headOrigin[0] - predictedPlayerEntity->lerpOrigin[0]);
	cg.refdef.vieworg[1] += (predictedPlayerEntity->headOrigin[1] - predictedPlayerEntity->lerpOrigin[1]);
	cg.refdef.vieworg[2] += (predictedPlayerEntity->headOrigin[2] - predictedPlayerEntity->lerpOrigin[2]);
#endif

	//*** SET HEAD TRACKING INPUT HERE!
	if (OVRDetected)
	{
		cg.refdefViewAngles[ROLL] = cg.predictedPlayerState.headangles[ROLL]; //*** 
		cg.refdefViewAngles[PITCH] = cg.predictedPlayerState.headangles[PITCH]; //*** Might need to adjust to rotate along axis of ROLL
		cg.refdefViewAngles[YAW] += cg.predictedPlayerState.headangles[YAW]; //****
	}

	VectorCopy(cg.refdefViewAngles, focusAngles);

	// if dead, look at killer
	if (cg.predictedPlayerState.stats[STAT_HEALTH] <= 0)
	{
		focusAngles[YAW] = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
		cg.refdefViewAngles[YAW] = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
		cg.refdef.vieworg[2] += 12;
	}

	AngleVectors(focusAngles, forward, focusRight, focusUp);

	VectorMA(cg.refdef.vieworg, FOCUS_DISTANCE, forward, focusPoint);

	VectorCopy(cg.refdef.vieworg, view);

	//*** Add 4 units to make the view at eye level
	matrix3_t headAxis;
	AnglesToAxis(focusAngles, headAxis);
	VectorMA(view, 4, /*predictedPlayerEntity->headAxis[ROLL]*/headAxis[ROLL], view);

	AngleVectors(cg.refdefViewAngles, forward, right, up);

	VectorCopy(view, cg.refdef.vieworg);

	// select pitch to look at focus point from vieword
	VectorSubtract(focusPoint, cg.refdef.vieworg, focusPoint);
	focusDist = sqrt(focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1]);
	if (focusDist < 1) {
		focusDist = 1;	// should never happen
	}

	cg.refdefViewAngles[PITCH] = -180 / M_PI * atan2(focusPoint[2], focusDist);
}
#endif //__OLD_VR__

//==============================================================================
//==============================================================================
// this causes a compiler bug on mac MrC compiler
static void CG_StepOffset( void ) {
	int		timeDelta;

	// smooth out stair climbing
	timeDelta = cg.time - cg.stepTime;
	if ( timeDelta < STEP_TIME ) {
		cg.refdef.vieworg[2] -= cg.stepChange
			* (STEP_TIME - timeDelta) / STEP_TIME;
	}
}

#define CAMERA_DAMP_INTERVAL	50

static vec3_t	cameramins = { -CAMERA_SIZE, -CAMERA_SIZE, -CAMERA_SIZE };
static vec3_t	cameramaxs = { CAMERA_SIZE, CAMERA_SIZE, CAMERA_SIZE };
vec3_t	camerafwd, cameraup;

vec3_t	cameraFocusAngles,			cameraFocusLoc;
vec3_t	cameraIdealTarget,			cameraIdealLoc;
vec3_t	cameraCurTarget={0,0,0},	cameraCurLoc={0,0,0};
vec3_t	cameraOldLoc={0,0,0},		cameraNewLoc={0,0,0};
int		cameraLastFrame=0;

float	cameraLastYaw=0;
float	cameraStiffFactor=0.0f;

/*
===============
Notes on the camera viewpoint in and out...

cg.refdef.vieworg
--at the start of the function holds the player actor's origin (center of player model).
--it is set to the final view location of the camera at the end of the camera code.
cg.refdef.viewangles
--at the start holds the client's view angles
--it is set to the final view angle of the camera at the end of the camera code.

===============
*/

extern qboolean gCGHasFallVector;
extern vec3_t gCGFallVector;

/*
===============
CG_CalcTargetThirdPersonViewLocation

===============
*/
static void CG_CalcIdealThirdPersonViewTarget(void)
{
	// Initialize IdealTarget
	if (gCGHasFallVector)
	{
		VectorCopy(gCGFallVector, cameraFocusLoc);
	}
	else
	{
		VectorCopy(cg.refdef.vieworg, cameraFocusLoc);
	}

	// Add in the new viewheight
	cameraFocusLoc[2] += cg.snap->ps.viewheight;

	// Add in a vertical offset from the viewpoint, which puts the actual target above the head, regardless of angle.
//	VectorMA(cameraFocusLoc, thirdPersonVertOffset, cameraup, cameraIdealTarget);

	// Add in a vertical offset from the viewpoint, which puts the actual target above the head, regardless of angle.
	VectorCopy( cameraFocusLoc, cameraIdealTarget );

	{
		float vertOffset = cg_thirdPersonVertOffset.value;

		if (cg.snap && cg.snap->ps.m_iVehicleNum)
		{
			centity_t *veh = &cg_entities[cg.snap->ps.m_iVehicleNum];
			if (veh->m_pVehicle &&
				veh->m_pVehicle->m_pVehicleInfo->cameraOverride)
			{ //override the range with what the vehicle wants it to be
				if ( veh->m_pVehicle->m_pVehicleInfo->cameraPitchDependantVertOffset )
				{
					if ( cg.snap->ps.viewangles[PITCH] > 0 )
					{
						vertOffset = 130+cg.predictedPlayerState.viewangles[PITCH]*-10;
						if ( vertOffset < -170 )
						{
							vertOffset = -170;
						}
					}
					else if ( cg.snap->ps.viewangles[PITCH] < 0 )
					{
						vertOffset = 130+cg.predictedPlayerState.viewangles[PITCH]*-5;
						if ( vertOffset > 130 )
						{
							vertOffset = 130;
						}
					}
					else
					{
						vertOffset = 30;
					}
				}
				else
				{
					vertOffset = veh->m_pVehicle->m_pVehicleInfo->cameraVertOffset;
				}
			}
			else if ( veh->m_pVehicle
				&& veh->m_pVehicle->m_pVehicleInfo
				&& veh->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL )
			{
				vertOffset = 0;
			}
		}
		cameraIdealTarget[2] += vertOffset;
	}
	//VectorMA(cameraFocusLoc, cg_thirdPersonVertOffset.value, cameraup, cameraIdealTarget);
}



/*
===============
CG_CalcTargetThirdPersonViewLocation

===============
*/
static void CG_CalcIdealThirdPersonViewLocation(void)
{
	float thirdPersonRange = cg_thirdPersonRange.value;

	if (cg.snap && cg.snap->ps.m_iVehicleNum)
	{
		centity_t *veh = &cg_entities[cg.snap->ps.m_iVehicleNum];
		if (veh->m_pVehicle &&
			veh->m_pVehicle->m_pVehicleInfo->cameraOverride)
		{ //override the range with what the vehicle wants it to be
			thirdPersonRange = veh->m_pVehicle->m_pVehicleInfo->cameraRange;
			if ( veh->playerState->hackingTime )
			{
				thirdPersonRange += fabs(((float)veh->playerState->hackingTime)/MAX_STRAFE_TIME) * 100.0f;
			}
		}
	}

	if ( cg.snap
		&& (cg.snap->ps.eFlags2&EF2_HELD_BY_MONSTER)
		&& cg.snap->ps.hasLookTarget
		&& cg_entities[cg.snap->ps.lookTarget].currentState.NPC_class == CLASS_RANCOR )//only possibility for now, may add Wampa and sand creature later
	{//stay back
		//thirdPersonRange = 180.0f;
		thirdPersonRange = 120.0f;
	}

	VectorMA(cameraIdealTarget, -(thirdPersonRange), camerafwd, cameraIdealLoc);
}



static void CG_ResetThirdPersonViewDamp(void)
{
	trace_t trace;

	// Cap the pitch within reasonable limits
	if (cameraFocusAngles[PITCH] > 89.0)
	{
		cameraFocusAngles[PITCH] = 89.0;
	}
	else if (cameraFocusAngles[PITCH] < -89.0)
	{
		cameraFocusAngles[PITCH] = -89.0;
	}

	AngleVectors(cameraFocusAngles, camerafwd, NULL, cameraup);

	// Set the cameraIdealTarget
	CG_CalcIdealThirdPersonViewTarget();

	// Set the cameraIdealLoc
	CG_CalcIdealThirdPersonViewLocation();

	// Now, we just set everything to the new positions.
	VectorCopy(cameraIdealLoc, cameraCurLoc);
	VectorCopy(cameraIdealTarget, cameraCurTarget);

	// First thing we do is trace from the first person viewpoint out to the new target location.
	CG_Trace(&trace, cameraFocusLoc, cameramins, cameramaxs, cameraCurTarget, cg.snap->ps.clientNum, MASK_CAMERACLIP);
	if (trace.fraction <= 1.0)
	{
		VectorCopy(trace.endpos, cameraCurTarget);
	}

	// Now we trace from the new target location to the new view location, to make sure there is nothing in the way.
	CG_Trace(&trace, cameraCurTarget, cameramins, cameramaxs, cameraCurLoc, cg.snap->ps.clientNum, MASK_CAMERACLIP);
	if (trace.fraction <= 1.0)
	{
		VectorCopy(trace.endpos, cameraCurLoc);
	}

	cameraLastFrame = cg.time;
	cameraLastYaw = cameraFocusAngles[YAW];
	cameraStiffFactor = 0.0f;
}

// This is called every frame.
static void CG_UpdateThirdPersonTargetDamp(void)
{
	trace_t trace;
	vec3_t	targetdiff;
	float	dampfactor, dtime, ratio;

	// Set the cameraIdealTarget
	// Automatically get the ideal target, to avoid jittering.
	CG_CalcIdealThirdPersonViewTarget();

	if ( cg.predictedVehicleState.hyperSpaceTime
		&& (cg.time-cg.predictedVehicleState.hyperSpaceTime) < HYPERSPACE_TIME )
	{//hyperspacing, no damp
		VectorCopy(cameraIdealTarget, cameraCurTarget);
	}
	else if (cg_thirdPersonTargetDamp.value>=1.0||cg.thisFrameTeleport||cg.predictedPlayerState.m_iVehicleNum)
	{	// No damping.
		VectorCopy(cameraIdealTarget, cameraCurTarget);
	}
	else if (cg_thirdPersonTargetDamp.value>=0.0)
	{
		// Calculate the difference from the current position to the new one.
		VectorSubtract(cameraIdealTarget, cameraCurTarget, targetdiff);

		// Now we calculate how much of the difference we cover in the time allotted.
		// The equation is (Damp)^(time)
		dampfactor = 1.0-cg_thirdPersonTargetDamp.value;	// We must exponent the amount LEFT rather than the amount bled off
		dtime = (float)(cg.time-cameraLastFrame) * (1.0/(float)CAMERA_DAMP_INTERVAL);	// Our dampfactor is geared towards a time interval equal to "1".

		// Note that since there are a finite number of "practical" delta millisecond values possible,
		// the ratio should be initialized into a chart ultimately.
		if ( cg_smoothCamera.integer )
			ratio = powf(dampfactor, dtime);
		else
			ratio = Q_powf( dampfactor, dtime );

		// This value is how much distance is "left" from the ideal.
		VectorMA(cameraIdealTarget, -ratio, targetdiff, cameraCurTarget);
		/////////////////////////////////////////////////////////////////////////////////////////////////////////
	}

	// Now we trace to see if the new location is cool or not.

	// First thing we do is trace from the first person viewpoint out to the new target location.
	CG_Trace(&trace, cameraFocusLoc, cameramins, cameramaxs, cameraCurTarget, cg.snap->ps.clientNum, MASK_CAMERACLIP);
	if (trace.fraction < 1.0)
	{
		VectorCopy(trace.endpos, cameraCurTarget);
	}

	// Note that previously there was an upper limit to the number of physics traces that are done through the world
	// for the sake of camera collision, since it wasn't calced per frame.  Now it is calculated every frame.
	// This has the benefit that the camera is a lot smoother now (before it lerped between tested points),
	// however two full volume traces each frame is a bit scary to think about.
}

// This can be called every interval, at the user's discretion.
extern void CG_CalcEntityLerpPositions( centity_t *cent ); //cg_ents.c
static void CG_UpdateThirdPersonCameraDamp(void)
{
	trace_t trace;
	vec3_t	locdiff;
	float dampfactor, dtime, ratio;

	// Set the cameraIdealLoc
	CG_CalcIdealThirdPersonViewLocation();


	// First thing we do is calculate the appropriate damping factor for the camera.
	dampfactor=0.0;
	if ( cg.predictedVehicleState.hyperSpaceTime
		&& (cg.time-cg.predictedVehicleState.hyperSpaceTime) < HYPERSPACE_TIME )
	{//hyperspacing - don't damp camera
		dampfactor = 1.0f;
	}
	else if (cg_thirdPersonCameraDamp.value != 0.0)
	{
		float pitch;
		float dFactor;

		if (!cg.predictedPlayerState.m_iVehicleNum)
		{
			dFactor = cg_thirdPersonCameraDamp.value;
		}
		else
		{
			dFactor = 1.0f;
		}

		// Note that the camera pitch has already been capped off to 89.
		pitch = Q_fabs(cameraFocusAngles[PITCH]);

		// The higher the pitch, the larger the factor, so as you look up, it damps a lot less.
		pitch /= 115.0;
		dampfactor = (1.0-dFactor)*(pitch*pitch);

		dampfactor += dFactor;

		// Now we also multiply in the stiff factor, so that faster yaw changes are stiffer.
		if (cameraStiffFactor > 0.0f)
		{	// The cameraStiffFactor is how much of the remaining damp below 1 should be shaved off, i.e. approach 1 as stiffening increases.
			dampfactor += (1.0-dampfactor)*cameraStiffFactor;
		}
	}

	if (dampfactor>=1.0||cg.thisFrameTeleport)
	{	// No damping.
		VectorCopy(cameraIdealLoc, cameraCurLoc);
	}
	else if (dampfactor>=0.0)
	{
		// Calculate the difference from the current position to the new one.
		VectorSubtract(cameraIdealLoc, cameraCurLoc, locdiff);

		// Now we calculate how much of the difference we cover in the time allotted.
		// The equation is (Damp)^(time)
		dampfactor = 1.0-dampfactor;	// We must exponent the amount LEFT rather than the amount bled off
		dtime = (float)(cg.time-cameraLastFrame) * (1.0/(float)CAMERA_DAMP_INTERVAL);	// Our dampfactor is geared towards a time interval equal to "1".

		// Note that since there are a finite number of "practical" delta millisecond values possible,
		// the ratio should be initialized into a chart ultimately.
		if ( cg_smoothCamera.integer )
			ratio = powf(dampfactor, dtime);
		else
			ratio = Q_powf( dampfactor, dtime );

		// This value is how much distance is "left" from the ideal.
		VectorMA(cameraIdealLoc, -ratio, locdiff, cameraCurLoc);
		/////////////////////////////////////////////////////////////////////////////////////////////////////////
	}

	// Now we trace from the new target location to the new view location, to make sure there is nothing in the way.
	CG_Trace(&trace, cameraCurTarget, cameramins, cameramaxs, cameraCurLoc, cg.snap->ps.clientNum, MASK_CAMERACLIP);

	if (trace.fraction < 1.0)
	{
		if (trace.entityNum < ENTITYNUM_WORLD &&
			cg_entities[trace.entityNum].currentState.solid == SOLID_BMODEL &&
			cg_entities[trace.entityNum].currentState.eType == ET_MOVER)
		{ //get a different position for movers -rww
			centity_t *mover = &cg_entities[trace.entityNum];

			//this is absolutely hackiful, since we calc view values before we add packet ents and lerp,
			//if we hit a mover we want to update its lerp pos and force it when we do the trace against
			//it.
			if (mover->currentState.pos.trType != TR_STATIONARY &&
				mover->currentState.pos.trType != TR_LINEAR)
			{
				int curTr = mover->currentState.pos.trType;
				vec3_t curTrB;

				VectorCopy(mover->currentState.pos.trBase, curTrB);

				//calc lerporigin for this client frame
				CG_CalcEntityLerpPositions(mover);

				//force the calc'd lerp to be the base and say we are stationary so we don't try to extrapolate
				//out further.
				mover->currentState.pos.trType = TR_STATIONARY;
				VectorCopy(mover->lerpOrigin, mover->currentState.pos.trBase);

				//retrace
				CG_Trace(&trace, cameraCurTarget, cameramins, cameramaxs, cameraCurLoc, cg.snap->ps.clientNum, MASK_CAMERACLIP);

				//copy old data back in
				mover->currentState.pos.trType = (trType_t)curTr;
				VectorCopy(curTrB, mover->currentState.pos.trBase);
			}
			if (trace.fraction < 1.0f)
			{ //still hit it, so take the proper trace endpos and use that.
				VectorCopy(trace.endpos, cameraCurLoc);
			}
		}
		else
		{
			VectorCopy(trace.endpos, cameraCurLoc);
		}
	}

	// Note that previously there was an upper limit to the number of physics traces that are done through the world
	// for the sake of camera collision, since it wasn't calced per frame.  Now it is calculated every frame.
	// This has the benefit that the camera is a lot smoother now (before it lerped between tested points),
	// however two full volume traces each frame is a bit scary to think about.
}




/*
===============`
CG_OffsetThirdPersonView

===============
*/
extern vmCvar_t cg_thirdPersonHorzOffset;
extern qboolean BG_UnrestrainedPitchRoll( playerState_t *ps, Vehicle_t *pVeh );
static void CG_OffsetThirdPersonView( void )
{
	vec3_t diff;
	float thirdPersonHorzOffset = cg_thirdPersonHorzOffset.value;
	float deltayaw;

	if (cg.snap && cg.snap->ps.m_iVehicleNum)
	{
		centity_t *veh = &cg_entities[cg.snap->ps.m_iVehicleNum];
		if (veh->m_pVehicle &&
			veh->m_pVehicle->m_pVehicleInfo->cameraOverride)
		{ //override the range with what the vehicle wants it to be
			thirdPersonHorzOffset = veh->m_pVehicle->m_pVehicleInfo->cameraHorzOffset;
			if ( veh->playerState->hackingTime )
			{
				thirdPersonHorzOffset += (((float)veh->playerState->hackingTime)/MAX_STRAFE_TIME) * -80.0f;
			}
		}
	}

	cameraStiffFactor = 0.0;

	// Set camera viewing direction.
	VectorCopy( cg.refdef.viewangles, cameraFocusAngles );

	// if dead, look at killer
	if ( cg.snap
		&& (cg.snap->ps.eFlags2&EF2_HELD_BY_MONSTER)
		&& cg.snap->ps.hasLookTarget
		&& cg_entities[cg.snap->ps.lookTarget].currentState.NPC_class == CLASS_RANCOR )//only possibility for now, may add Wampa and sand creature later
	{//being held
		//vec3_t monsterPos, dir2Me;
		centity_t	*monster = &cg_entities[cg.snap->ps.lookTarget];
		VectorSet( cameraFocusAngles, 0, AngleNormalize180(monster->lerpAngles[YAW]+180), 0 );
		//make the look angle the vector from his mouth to me
		/*
		VectorCopy( monster->lerpOrigin, monsterPos );
		monsterPos[2] = cg.snap->ps.origin[2];
		VectorSubtract( monsterPos, cg.snap->ps.origin, dir2Me );
		vectoangles( dir2Me, cameraFocusAngles );
		*/
	}
	else if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 )
	{
		cameraFocusAngles[YAW] = cg.snap->ps.stats[STAT_DEAD_YAW];
	}
	else
	{	// Add in the third Person Angle.
		cameraFocusAngles[YAW] += cg_thirdPersonAngle.value;
		{
			float pitchOffset = cg_thirdPersonPitchOffset.value;
			if (cg.snap && cg.snap->ps.m_iVehicleNum)
			{
				centity_t *veh = &cg_entities[cg.snap->ps.m_iVehicleNum];
				if (veh->m_pVehicle &&
					veh->m_pVehicle->m_pVehicleInfo->cameraOverride)
				{ //override the range with what the vehicle wants it to be
					if ( veh->m_pVehicle->m_pVehicleInfo->cameraPitchDependantVertOffset )
					{
						if ( cg.snap->ps.viewangles[PITCH] > 0 )
						{
							pitchOffset = cg.predictedPlayerState.viewangles[PITCH]*-0.75;
						}
						else if ( cg.snap->ps.viewangles[PITCH] < 0 )
						{
							pitchOffset = cg.predictedPlayerState.viewangles[PITCH]*-0.75;
						}
						else
						{
							pitchOffset = 0;
						}
					}
					else
					{
						pitchOffset = veh->m_pVehicle->m_pVehicleInfo->cameraPitchOffset;
					}
				}
			}
			/*if ( 0 && cg.predictedPlayerState.m_iVehicleNum //in a vehicle
				&& BG_UnrestrainedPitchRoll( &cg.predictedPlayerState, cg_entities[cg.predictedPlayerState.m_iVehicleNum].m_pVehicle ) )//can roll/pitch without restriction
			{
				float pitchPerc = ((90.0f-fabs(cameraFocusAngles[ROLL]))/90.0f);
				cameraFocusAngles[PITCH] += pitchOffset*pitchPerc;
				if ( cameraFocusAngles[ROLL] > 0 )
				{
					cameraFocusAngles[YAW] -= pitchOffset-(pitchOffset*pitchPerc);
				}
				else
				{
					cameraFocusAngles[YAW] += pitchOffset-(pitchOffset*pitchPerc);
				}
			}
			else*/
			{
				cameraFocusAngles[PITCH] += pitchOffset;
			}
		}
	}

	// The next thing to do is to see if we need to calculate a new camera target location.

	// If we went back in time for some reason, or if we just started, reset the sample.
	if (cameraLastFrame == 0 || cameraLastFrame > cg.time)
	{
		CG_ResetThirdPersonViewDamp();
	}
	else
	{
		// Cap the pitch within reasonable limits
		if ( cg.predictedPlayerState.m_iVehicleNum //in a vehicle
			&& BG_UnrestrainedPitchRoll( &cg.predictedPlayerState, cg_entities[cg.predictedPlayerState.m_iVehicleNum].m_pVehicle ) )//can roll/pitch without restriction
		{//no clamp on pitch
			//FIXME: when pitch >= 90 or <= -90, camera rotates oddly... need to CrossProduct not just vectoangles
		}
		else
		{
			if (cameraFocusAngles[PITCH] > 80.0)
			{
				cameraFocusAngles[PITCH] = 80.0;
			}
			else if (cameraFocusAngles[PITCH] < -80.0)
			{
				cameraFocusAngles[PITCH] = -80.0;
			}
		}

		AngleVectors(cameraFocusAngles, camerafwd, NULL, cameraup);

		deltayaw = fabs(cameraFocusAngles[YAW] - cameraLastYaw);
		if (deltayaw > 180.0f)
		{ // Normalize this angle so that it is between 0 and 180.
			deltayaw = fabs(deltayaw - 360.0f);
		}
		cameraStiffFactor = deltayaw / (float)(cg.time-cameraLastFrame);
		if (cameraStiffFactor < 1.0)
		{
			cameraStiffFactor = 0.0;
		}
		else if (cameraStiffFactor > 2.5)
		{
			cameraStiffFactor = 0.75;
		}
		else
		{	// 1 to 2 scales from 0.0 to 0.5
			cameraStiffFactor = (cameraStiffFactor-1.0f)*0.5f;
		}
		cameraLastYaw = cameraFocusAngles[YAW];

		// Move the target to the new location.
		CG_UpdateThirdPersonTargetDamp();
		CG_UpdateThirdPersonCameraDamp();
	}

	// Now interestingly, the Quake method is to calculate a target focus point above the player, and point the camera at it.
	// We won't do that for now.

	// We must now take the angle taken from the camera target and location.
	/*VectorSubtract(cameraCurTarget, cameraCurLoc, diff);
	VectorNormalize(diff);
	vectoangles(diff, cg.refdef.viewangles);*/
	VectorSubtract(cameraCurTarget, cameraCurLoc, diff);
	{
		float dist = VectorNormalize(diff);
		//under normal circumstances, should never be 0.00000 and so on.
		if ( !dist || (diff[0] == 0 || diff[1] == 0) )
		{//must be hitting something, need some value to calc angles, so use cam forward
			VectorCopy( camerafwd, diff );
		}
	}
	/*if ( 0 && cg.predictedPlayerState.m_iVehicleNum //in a vehicle
		&& BG_UnrestrainedPitchRoll( &cg.predictedPlayerState, cg_entities[cg.predictedPlayerState.m_iVehicleNum].m_pVehicle ) )//can roll/pitch without restriction
	{//FIXME: this causes camera jerkiness, need to blend the roll?
		float sav_Roll = cg.refdef.viewangles[ROLL];
		vectoangles(diff, cg.refdef.viewangles);
		cg.refdef.viewangles[ROLL] = sav_Roll;
	}
	else*/
	{
		vectoangles(diff, cg.refdef.viewangles);
	}

	// Temp: just move the camera to the side a bit
	if ( thirdPersonHorzOffset != 0.0f )
	{
		AnglesToAxis( cg.refdef.viewangles, cg.refdef.viewaxis );
		VectorMA( cameraCurLoc, thirdPersonHorzOffset, cg.refdef.viewaxis[1], cameraCurLoc );
	}

	// ...and of course we should copy the new view location to the proper spot too.
	VectorCopy(cameraCurLoc, cg.refdef.vieworg);

	cameraLastFrame=cg.time;
}

void CG_GetVehicleCamPos( vec3_t camPos )
{
	VectorCopy( cg.refdef.vieworg, camPos );
}

/*
===============
CG_OffsetThirdPersonView

===============
*//*
#define	FOCUS_DISTANCE	512
static void CG_OffsetThirdPersonView( void ) {
	vec3_t		forward, right, up;
	vec3_t		view;
	vec3_t		focusAngles;
	trace_t		trace;
	static vec3_t	mins = { -4, -4, -4 };
	static vec3_t	maxs = { 4, 4, 4 };
	vec3_t		focusPoint;
	float		focusDist;
	float		forwardScale, sideScale;

	cg.refdef.vieworg[2] += cg.predictedPlayerState.viewheight;

	VectorCopy( cg.refdef.viewangles, focusAngles );

	// if dead, look at killer
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		focusAngles[YAW] = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
		cg.refdef.viewangles[YAW] = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
	}

	if ( focusAngles[PITCH] > 45 ) {
		focusAngles[PITCH] = 45;		// don't go too far overhead
	}
	AngleVectors( focusAngles, forward, NULL, NULL );

	VectorMA( cg.refdef.vieworg, FOCUS_DISTANCE, forward, focusPoint );

	VectorCopy( cg.refdef.vieworg, view );

	view[2] += 8;

	cg.refdef.viewangles[PITCH] *= 0.5;

	AngleVectors( cg.refdef.viewangles, forward, right, up );

	forwardScale = cos( cg_thirdPersonAngle.value / 180 * M_PI );
	sideScale = sin( cg_thirdPersonAngle.value / 180 * M_PI );
	VectorMA( view, -cg_thirdPersonRange.value * forwardScale, forward, view );
	VectorMA( view, -cg_thirdPersonRange.value * sideScale, right, view );

	// trace a ray from the origin to the viewpoint to make sure the view isn't
	// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything

	if (!cg_cameraMode.integer) {
		CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_CAMERACLIP);

		if ( trace.fraction != 1.0 ) {
			VectorCopy( trace.endpos, view );
			view[2] += (1.0 - trace.fraction) * 32;
			// try another trace to this position, because a tunnel may have the ceiling
			// close enogh that this is poking out

			CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_CAMERACLIP);
			VectorCopy( trace.endpos, view );
		}
	}


	VectorCopy( view, cg.refdef.vieworg );

	// select pitch to look at focus point from vieword
	VectorSubtract( focusPoint, cg.refdef.vieworg, focusPoint );
	focusDist = sqrt( focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1] );
	if ( focusDist < 1 ) {
		focusDist = 1;	// should never happen
	}
	cg.refdef.viewangles[PITCH] = -180 / M_PI * atan2( focusPoint[2], focusDist );
	cg.refdef.viewangles[YAW] -= cg_thirdPersonAngle.value;
}


// this causes a compiler bug on mac MrC compiler
static void CG_StepOffset( void ) {
	int		timeDelta;

	// smooth out stair climbing
	timeDelta = cg.time - cg.stepTime;
	if ( timeDelta < STEP_TIME ) {
		cg.refdef.vieworg[2] -= cg.stepChange
			* (STEP_TIME - timeDelta) / STEP_TIME;
	}
}*/

/*
===============
CG_OffsetFirstPersonView

===============
*/
static void CG_OffsetFirstPersonView( void ) {
#if 0
	float			*origin;
	float			*angles;
	float			bob;
	float			ratio;
	float			delta;
	float			speed;
	float			f;
	vec3_t			predictedVelocity;
	int				timeDelta;
	int				kickTime;

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		return;
	}

	origin = cg.refdef.vieworg;
	angles = cg.refdef.viewangles;

	// if dead, fix the angle and don't add any kick
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) {
		angles[ROLL] = 40;
		angles[PITCH] = -15;
		angles[YAW] = cg.snap->ps.stats[STAT_DEAD_YAW];
		origin[2] += cg.predictedPlayerState.viewheight;
		return;
	}

	// add angles based on weapon kick
	kickTime = (cg.time - cg.kick_time);
	if ( kickTime < 800 )
	{//kicks are always 1 second long.  Deal with it.
		float kickPerc = 0.0f;
		if ( kickTime <= 200 )
		{//winding up
			kickPerc = kickTime/200.0f;
		}
		else
		{//returning to normal
			kickTime = 800 - kickTime;
			kickPerc = kickTime/600.0f;
		}
		VectorMA( angles, kickPerc, cg.kick_angles, angles );
	}
	// add angles based on damage kick
	if ( cg.damageTime ) {
		ratio = cg.time - cg.damageTime;
		if ( ratio < DAMAGE_DEFLECT_TIME ) {
			ratio /= DAMAGE_DEFLECT_TIME;
			angles[PITCH] += ratio * cg.v_dmg_pitch;
			angles[ROLL] += ratio * cg.v_dmg_roll;
		} else {
			ratio = 1.0 - ( ratio - DAMAGE_DEFLECT_TIME ) / DAMAGE_RETURN_TIME;
			if ( ratio > 0 ) {
				angles[PITCH] += ratio * cg.v_dmg_pitch;
				angles[ROLL] += ratio * cg.v_dmg_roll;
			}
		}
	}

	// add pitch based on fall kick
#if 0
	ratio = ( cg.time - cg.landTime) / FALL_TIME;
	if (ratio < 0)
		ratio = 0;
	angles[PITCH] += ratio * cg.fall_value;
#endif

	// add angles based on velocity
	VectorCopy( cg.predictedPlayerState.velocity, predictedVelocity );

	delta = DotProduct ( predictedVelocity, cg.refdef.viewaxis[0]);
	angles[PITCH] += delta * cg_runPitch.value;

	delta = DotProduct ( predictedVelocity, cg.refdef.viewaxis[1]);
	angles[ROLL] -= delta * cg_runRoll.value;

	// add angles based on bob

	// make sure the bob is visible even at low speeds
	speed = cg.xyspeed > 200 ? cg.xyspeed : 200;

	delta = cg.bobfracsin * cg_bobPitch.value * speed;
	if (cg.predictedPlayerState.pm_flags & PMF_DUCKED)
		delta *= 3;		// crouching
	angles[PITCH] += delta;
	delta = cg.bobfracsin * cg_bobRoll.value * speed;
	if (cg.predictedPlayerState.pm_flags & PMF_DUCKED)
		delta *= 3;		// crouching accentuates roll
	if (cg.bobcycle & 1)
		delta = -delta;
	angles[ROLL] += delta;

//===================================

	// add view height
	origin[2] += cg.predictedPlayerState.viewheight;

	// smooth out duck height changes
	timeDelta = cg.time - cg.duckTime;
	if ( timeDelta < DUCK_TIME) {
		cg.refdef.vieworg[2] -= cg.duckChange
			* (DUCK_TIME - timeDelta) / DUCK_TIME;
	}

	// add bob height
	bob = cg.bobfracsin * cg.xyspeed * cg_bobUp.value;
	if (bob > 6) {
		bob = 6;
	}

	origin[2] += bob;


	// add fall height
	delta = cg.time - cg.landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		f = delta / LAND_DEFLECT_TIME;
		cg.refdef.vieworg[2] += cg.landChange * f;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - ( delta / LAND_RETURN_TIME );
		cg.refdef.vieworg[2] += cg.landChange * f;
	}

	// add step offset
	CG_StepOffset();

	// add kick offset

	VectorAdd (origin, cg.kick_origin, origin);

	// pivot the eye based on a neck length
#if 0
	{
#define	NECK_LENGTH		8
	vec3_t			forward, up;

	cg.refdef.vieworg[2] -= NECK_LENGTH;
	AngleVectors( cg.refdef.viewangles, forward, NULL, up );
	VectorMA( cg.refdef.vieworg, 3, forward, cg.refdef.vieworg );
	VectorMA( cg.refdef.vieworg, NECK_LENGTH, up, cg.refdef.vieworg );
	}
#endif

#else //!0

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		return;
	}

	cg.refdef.vieworg[2] += cg.predictedPlayerState.viewheight * cg_entities[cg.predictedPlayerState.clientNum].modelScale[2];

#ifdef __OLD_VR__
	// HMM DUNNO....
	//angles = cg.refdefViewAngles;
	VectorCopy(cg.refdefViewAngles, cg.refdef.viewangles);
#endif //__OLD_VR__

#endif //0
}

static void CG_OffsetFighterView( void )
{
	vec3_t vehFwd, vehRight, vehUp, backDir;
	vec3_t	camOrg, camBackOrg;
	float horzOffset = cg_thirdPersonHorzOffset.value;
	float vertOffset = cg_thirdPersonVertOffset.value;
	float pitchOffset = cg_thirdPersonPitchOffset.value;
	float yawOffset = cg_thirdPersonAngle.value;
	float range = cg_thirdPersonRange.value;
	trace_t	trace;
	centity_t *veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];

	AngleVectors( cg.refdef.viewangles, vehFwd, vehRight, vehUp );

	if ( veh->m_pVehicle &&
		veh->m_pVehicle->m_pVehicleInfo->cameraOverride )
	{ //override the horizontal offset with what the vehicle wants it to be
		horzOffset = veh->m_pVehicle->m_pVehicleInfo->cameraHorzOffset;
		vertOffset = veh->m_pVehicle->m_pVehicleInfo->cameraVertOffset;
		//NOTE: no yaw offset?
		pitchOffset = veh->m_pVehicle->m_pVehicleInfo->cameraPitchOffset;
		range = veh->m_pVehicle->m_pVehicleInfo->cameraRange;
		if ( veh->playerState->hackingTime )
		{
			horzOffset += (((float)veh->playerState->hackingTime)/MAX_STRAFE_TIME) * -80.0f;
			range += fabs(((float)veh->playerState->hackingTime)/MAX_STRAFE_TIME) * 100.0f;
		}
	}

	//Set camera viewing position
	VectorMA( cg.refdef.vieworg, horzOffset, vehRight, camOrg );
	VectorMA( camOrg, vertOffset, vehUp, camOrg );

	//trace to that pos
	CG_Trace(&trace, cg.refdef.vieworg, cameramins, cameramaxs, camOrg, cg.snap->ps.clientNum, MASK_CAMERACLIP);
	if ( trace.fraction < 1.0 )
	{
		VectorCopy( trace.endpos, camOrg );
	}

	// Set camera viewing direction.
	cg.refdef.viewangles[YAW] += yawOffset;
	cg.refdef.viewangles[PITCH] += pitchOffset;

	//Now bring the cam back from that pos and angles at range
	AngleVectors( cg.refdef.viewangles, backDir, NULL, NULL );
	VectorScale( backDir, -1, backDir );

	VectorMA( camOrg, range, backDir, camBackOrg );

	//trace to that pos
	CG_Trace(&trace, camOrg, cameramins, cameramaxs, camBackOrg, cg.snap->ps.clientNum, MASK_CAMERACLIP);
	VectorCopy( trace.endpos, camOrg );

	//FIXME: do we need to smooth the org?
	// ...and of course we should copy the new view location to the proper spot too.
	VectorCopy(camOrg, cg.refdef.vieworg);
}

//======================================================================

//
// Zoom controls
//

qboolean drawingSniperScopeView = qfalse;
float lastfov = 0.0;

void CG_AdjustZoomVal(float val, int type)
{
	cg.zoomval += val;

	if (cg_scopeZoomMin.value != 0.0 || cg_scopeZoomMax.value != 0.0)
	{// Cvar overrides for debugging...
		if (cg.zoomval > cg_scopeZoomMax.value)
		{
			cg.zoomval = cg_scopeZoomMax.value;
		}

		if (cg.zoomval < cg_scopeZoomMin.value)
		{
			cg.zoomval = cg_scopeZoomMin.value;
		}
		return;
	}

	if (cg.zoomval > scopeData[type].scopeZoomMax)
	{
		cg.zoomval = scopeData[type].scopeZoomMax;
	}

	if (cg.zoomval < scopeData[type].scopeZoomMin)
	{
		cg.zoomval = scopeData[type].scopeZoomMin;
	}
}

void CG_ZoomIn_f(void)
{
	// Gordon: fixed being able to "latch" your zoom by weaponcheck + quick zoomin
	// OSP - change for zoom view in demos
	if (scopeData[cg.predictedPlayerState.scopeType].instantZoom)
	{
		CG_AdjustZoomVal(-100, cg.predictedPlayerState.scopeType); // 100 is more then any used, so max zoom in instantly
	}
	else
	{
		CG_AdjustZoomVal(-scopeData[cg.predictedPlayerState.scopeType].scopeZoomSpeed, cg.predictedPlayerState.scopeType);
	}

	//trap->Print("Zoom in to %f.\n", cg.zoomval);
}

void CG_ZoomOut_f(void)
{
	if (scopeData[cg.predictedPlayerState.scopeType].instantZoom)
	{
		CG_AdjustZoomVal(100, cg.predictedPlayerState.scopeType); // 100 is more then any used, so max zoom in instantly
	}
	else
	{
		CG_AdjustZoomVal(scopeData[cg.predictedPlayerState.scopeType].scopeZoomSpeed, cg.predictedPlayerState.scopeType);
	}

	//trap->Print("Zoom out to %f.\n", cg.zoomval);
}

/*
==============
CG_Zoom
==============
*/
void CG_Zoom( void )
{
	if ( (cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.demoPlayback ) {
		cg.predictedPlayerState.eFlags = cg.snap->ps.eFlags;
		cg.predictedPlayerState.weapon = cg.snap->ps.weapon;
		cg.predictedPlayerState.scopeType = cg.snap->ps.scopeType;
	}

	if (cg_scopeZoomMin.value != 0.0 || cg_scopeZoomMax.value != 0.0)
	{// Cvar overrides for debugging...
		if( cg.predictedPlayerState.scopeType >= SCOPE_BINOCULARS ) {
			if (cg.zoomval < cg_scopeZoomMin.value)
			{
				cg.zoomval = cg_scopeZoomMin.value;
			}

			if (cg.zoomval > cg_scopeZoomMax.value)
			{
				cg.zoomval = cg_scopeZoomMax.value;
			}
		} else {
			cg.zoomval = 0;
		}
		return;
	}

	if( cg.predictedPlayerState.scopeType >= SCOPE_BINOCULARS ) {
		if (cg.zoomval < scopeData[cg.predictedPlayerState.scopeType].scopeZoomMin)
		{
			cg.zoomval = scopeData[cg.predictedPlayerState.scopeType].scopeZoomMin;
		}

		if (cg.zoomval > scopeData[cg.predictedPlayerState.scopeType].scopeZoomMax)
		{
			cg.zoomval = scopeData[cg.predictedPlayerState.scopeType].scopeZoomMax;
		}
	} else {
		cg.zoomval = 0;
	}
}

/*
====================
CG_CalcFov

Fixed fov at intermissions, otherwise account for fov variable and zooms.
====================
*/
#define	WAVE_AMPLITUDE	1
#define	WAVE_FREQUENCY	0.4
float zoomFov; //this has to be global client-side

static int CG_CalcFov( void ) {
	float	x;
	float	phase;
	float	v;
	float	fov_x, fov_y;
	int		inwater;
	float	cgFov;

	CG_Zoom();

	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 && !(cg.snap->ps.pm_flags & PMF_FOLLOW) ) 
	{
		cg.zoomedBinoc = qfalse;
		cg.zoomTime = 0;
		cg.zoomval = 0;
	}

	cgFov = cg_fov.value;

	if (cgFov < 1)
	{
		cgFov = 1;
	}

	if (cgFov > 180)
	{
		cgFov = 180;
	}

	if ( !drawingSniperScopeView || cg.predictedPlayerState.pm_type == PM_INTERMISSION) {
		// not drawing the inner zoom view, or intermission, use standard fov.
		fov_x = cgFov;
	} else {
		// user selectable
		fov_x = cgFov;

		//trap->Print("Zoom is %f.\n", cg.zoomval);

		// account for zooms
		if(cg.zoomval) {
			// Zoom multiplier is set...

			zoomFov = cg.zoomval;	// (SA) use user scrolled amount

			if (cg_scopeZoomMin.value != 0.0 || cg_scopeZoomMax.value != 0.0)
			{// Cvar overrides for debugging...
				if ( zoomFov < cg_scopeZoomMin.value ) {
					zoomFov = cg_scopeZoomMin.value;
				} 

				if ( zoomFov > cg_scopeZoomMax.value ) {
					zoomFov = cg_scopeZoomMax.value;
				}
			}
			else
			{
				if ( zoomFov < scopeData[cg.predictedPlayerState.scopeType].scopeZoomMin ) {
					zoomFov = scopeData[cg.predictedPlayerState.scopeType].scopeZoomMin;
				} 

				if ( zoomFov > scopeData[cg.predictedPlayerState.scopeType].scopeZoomMax ) {
					zoomFov = scopeData[cg.predictedPlayerState.scopeType].scopeZoomMax;
				}
			}

			fov_x = fov_x / zoomFov;

			lastfov = fov_x;
		} else {
			// Nothing set yet, so set to minimum zoom for this scope...
			fov_x = lastfov;

			if (cg_scopeZoomMin.value != 0.0 || cg_scopeZoomMax.value != 0.0)
			{// Cvar overrides for debugging...
				zoomFov = fov_x * cg_scopeZoomMin.value;
			}
			else
			{
				zoomFov = fov_x * scopeData[cg.predictedPlayerState.scopeType].scopeZoomMin;
			}
		}
	}

	if ( cg_fovAspectAdjust.integer ) {
		// Based on LordHavoc's code for Darkplaces
		// http://www.quakeworld.nu/forum/topic/53/what-does-your-qw-look-like/page/30
		const float baseAspect = 0.75f; // 3/4
		const float aspect = (float)cgs.glconfig.vidWidth/(float)cgs.glconfig.vidHeight;
		const float desiredFov = fov_x;

		fov_x = atan( tan( desiredFov*M_PI / 360.0f ) * baseAspect*aspect )*360.0f / M_PI;
	}

	x = cg.refdef.width / tan( fov_x / 360 * M_PI );
	fov_y = atan2( cg.refdef.height, x );
	fov_y = fov_y * 360 / M_PI;

	// warp if underwater
	cg.refdef.viewContents = CG_PointContents( cg.refdef.vieworg, -1 );
	if ( cg.refdef.viewContents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ){
		phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		v = WAVE_AMPLITUDE * sin( phase );
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
		cg.refdef.rdflags |= RDF_UNDERWATER;
	}
	else {
		inwater = qfalse;
		cg.refdef.rdflags &= ~RDF_UNDERWATER;
	}

	if (!drawingSniperScopeView && cg.predictedPlayerState.scopeType >= SCOPE_BINOCULARS)
	{// Tell the renderer to blur the screen outside of the scope rectile...
		if (!(cg.refdef.rdflags & RDF_BLUR))
			cg.refdef.rdflags |= RDF_BLUR;
	}
	/*
	else if (cg.clientNum == cg.predictedPlayerState.clientNum && cg.predictedPlayerState.eFlags & EF_DEAD)
	{// Tell the renderer to blur the screen when dead...
		if (!(cg.refdef.rdflags & RDF_BLUR))
			cg.refdef.rdflags |= RDF_BLUR;
	}
	*/
	else
	{// Make sure the screen is not blurred...
		if (cg.refdef.rdflags & RDF_BLUR)
			cg.refdef.rdflags &= ~RDF_BLUR;
	}

	// set it
	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;

	if (cg.predictedPlayerState.scopeType && drawingSniperScopeView)
	{// Adjust rotation speed while zooming... Very slow at high zooms, a little slower then normal at low zooms...
		cg.zoomSensitivity = 0.25 + ((scopeData[cg.predictedPlayerState.scopeType].scopeZoomMax - cg.zoomval) * 0.5);
	}
	else if ( !cg.zoomed || !drawingSniperScopeView ) {
		cg.zoomSensitivity = 1;
	} else {
		cg.zoomSensitivity = cg.refdef.fov_y / 75.0;
	}

	return inwater;
}


/*
================
CG_DrawScopeView

================
*/

/*
extern void CG_DrawMiscStaticModels( void );

void CG_DrawScopeView( float x, float y, float w, float h ) {
	refdef_t		refdef, refdefBackup;

	drawingSniperScopeView = qtrue;

	CG_CalcFov();
	
	memcpy( &refdef, &cg.refdef, sizeof(refdef_t));
	memcpy( &refdefBackup, &cg.refdef, sizeof(refdef_t));

	refdef.x = x * cgs.screenXScale;
	refdef.y = y * cgs.screenYScale;
	refdef.width = w * cgs.screenXScale;
	refdef.height = h * cgs.screenYScale;

	refdef.time = cg.time;

	//refdef.rdflags = 0;

	memcpy( &cg.refdef, &refdef, sizeof(refdef_t));

	if ( !cg.hyperspace)
	{ //rww - also had to add this to add effects being rendered in portal sky areas properly.
		trap->FX_AddScheduledEffects(qtrue);
	}

	CG_AddPacketEntities(qtrue);

	trap->R_RenderScene( &refdef );

	memcpy( &cg.refdef, &refdefBackup, sizeof(refdef_t));

	drawingSniperScopeView = qfalse;
}
*/

/*
===============
CG_DamageBlendBlob

===============
*/
static void CG_DamageBlendBlob( void )
{
	int			t;
	int			maxTime;
	refEntity_t		ent;

	if ( !cg.damageValue ) {
		return;
	}

	maxTime = DAMAGE_TIME;
	t = cg.time - cg.damageTime;
	if ( t <= 0 || t >= maxTime ) {
		return;
	}

	memset( &ent, 0, sizeof( ent ) );
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_FIRST_PERSON;

	VectorMA( cg.refdef.vieworg, 8, cg.refdef.viewaxis[0], ent.origin );
	VectorMA( ent.origin, cg.damageX * -8, cg.refdef.viewaxis[1], ent.origin );
	VectorMA( ent.origin, cg.damageY * 8, cg.refdef.viewaxis[2], ent.origin );

	ent.radius = cg.damageValue * 3 * ( 1.0 - ((float)t / maxTime) );

	if (cg.snap->ps.damageType == 0)
	{ //pure health
		ent.customShader = cgs.media.viewPainShader;
		ent.shaderRGBA[0] = 180 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[1] = 50 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[2] = 50 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[3] = 255;
	}
	else if (cg.snap->ps.damageType == 1)
	{ //pure shields
		ent.customShader = cgs.media.viewPainShader_Shields;
		ent.shaderRGBA[0] = 50 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[1] = 180 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[2] = 50 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[3] = 255;
	}
	else
	{ //shields and health
		ent.customShader = cgs.media.viewPainShader_ShieldsAndHealth;
		ent.shaderRGBA[0] = 180 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[1] = 180 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[2] = 50 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[3] = 255;
	}
	AddRefEntityToScene( &ent );
}

int cg_actionCamLastTime = 0;
vec3_t cg_actionCamLastPos;

//action cam routine -rww
static qboolean CG_ThirdPersonActionCam(void)
{
    centity_t *cent = &cg_entities[cg.snap->ps.clientNum];
	clientInfo_t *ci = &cgs.clientinfo[cg.snap->ps.clientNum];
	trace_t tr;
	vec3_t positionDir;
	vec3_t desiredAngles;
	vec3_t desiredPos;
	vec3_t v;
	const float smoothFactor = 0.1f*timescale.value;
	int i;

	if (!cent->ghoul2)
	{ //if we don't have a g2 instance this frame for whatever reason then do nothing
		return qfalse;
	}

	if (cent->currentState.weapon != WP_SABER)
	{ //just being safe, should not ever happen
		return qfalse;
	}

	if ((cg.time - ci->saber[0].blade[0].trail.lastTime) > 300)
	{ //too long since we last got the blade position
		return qfalse;
	}

	//get direction from base to ent origin
	VectorSubtract(ci->saber[0].blade[0].trail.base, cent->lerpOrigin, positionDir);
	VectorNormalize(positionDir);

	// Update height offsets for model scales and 3rd person range...
	CG_UpdateCamera();

	//position the cam based on the direction and saber position
	VectorMA(cent->lerpOrigin, cg_thirdPersonRange.value*2, positionDir, desiredPos);

	//trace to the desired pos to see how far that way we can actually go before we hit something
	//the endpos will be valid for our desiredpos no matter what
	CG_Trace(&tr, cent->lerpOrigin, NULL, NULL, desiredPos, cent->currentState.number, MASK_SOLID);
	VectorCopy(tr.endpos, desiredPos);

	if ((cg.time - cg_actionCamLastTime) > 300)
	{
		//do a third person offset first and grab the initial point from that
		CG_OffsetThirdPersonView();
		VectorCopy(cg.refdef.vieworg, cg_actionCamLastPos);
	}

	cg_actionCamLastTime = cg.time;

	//lerp the vieworg to the desired pos from the last valid
	VectorSubtract(desiredPos, cg_actionCamLastPos, v);

	if (VectorLength(v) > 64.0f)
	{ //don't bother moving yet if not far from the last pos
		for (i = 0; i < 3; i++)
		{
			cg_actionCamLastPos[i] = (cg_actionCamLastPos[i] + (v[i]*smoothFactor));
			cg.refdef.vieworg[i] = cg_actionCamLastPos[i];
		}
	}
	else
	{
		VectorCopy(cg_actionCamLastPos, cg.refdef.vieworg);
	}

	//Make sure the point is alright
	CG_Trace(&tr, cent->lerpOrigin, NULL, NULL, cg.refdef.vieworg, cent->currentState.number, MASK_SOLID);
	VectorCopy(tr.endpos, cg.refdef.vieworg);

	VectorSubtract(cent->lerpOrigin, cg.refdef.vieworg, positionDir);
	vectoangles(positionDir, desiredAngles);

	//just set the angles for now
	VectorCopy(desiredAngles, cg.refdef.viewangles);
	return qtrue;
}

vec3_t	cg_lastTurretViewAngles={0};
qboolean CG_CheckPassengerTurretView( void )
{
	if ( cg.predictedPlayerState.m_iVehicleNum //in a vehicle
		&& cg.predictedPlayerState.generic1 )//as a passenger
	{//passenger in a vehicle
		centity_t *vehCent = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
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
						int boltIndex = -1;
						qboolean hackPosAndAngle = qfalse;
						if ( vehCent->m_pVehicle->m_iGunnerViewTag[turretNum] != -1 )
						{
							boltIndex = vehCent->m_pVehicle->m_iGunnerViewTag[turretNum];
						}
						else
						{//crap... guess?
							hackPosAndAngle = qtrue;
							if ( vehCent->m_pVehicle->m_pVehicleInfo->turret[turretNum].yawBone )
							{
								boltIndex = trap->G2API_AddBolt( vehCent->ghoul2, 0, vehCent->m_pVehicle->m_pVehicleInfo->turret[turretNum].yawBone );
							}
							else if ( vehCent->m_pVehicle->m_pVehicleInfo->turret[turretNum].pitchBone )
							{
								boltIndex = trap->G2API_AddBolt( vehCent->ghoul2, 0, vehCent->m_pVehicle->m_pVehicleInfo->turret[turretNum].pitchBone );
							}
							else
							{//well, no way of knowing, so screw it
								return qfalse;
							}
						}
						if ( boltIndex != -1 )
						{
							mdxaBone_t boltMatrix;
							vec3_t fwd, up;
							trap->G2API_GetBoltMatrix_NoRecNoRot(vehCent->ghoul2, 0, boltIndex, &boltMatrix, vehCent->lerpAngles,
								vehCent->lerpOrigin, cg.time, NULL, vehCent->modelScale);
							BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, cg.refdef.vieworg);
							if ( hackPosAndAngle )
							{
								//FIXME: these are assumptions, externalize?  BETTER YET: give me a controller view bolt/tag for each turret
								BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, fwd);
								BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, up);
								VectorMA( cg.refdef.vieworg, 8.0f, fwd, cg.refdef.vieworg );
								VectorMA( cg.refdef.vieworg, 4.0f, up, cg.refdef.vieworg );
							}
							else
							{
								BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, fwd);
							}
							{
								vec3_t	newAngles, deltaAngles;
								vectoangles( fwd, newAngles );
								AnglesSubtract( newAngles, cg_lastTurretViewAngles, deltaAngles );
								VectorMA( cg_lastTurretViewAngles, 0.5f*(float)cg.frametime/100.0f, deltaAngles, cg.refdef.viewangles );
							}
							return qtrue;
						}
					}
				}
			}
		}
	}
	return qfalse;
}
/*
===============
CG_CalcViewValues

Sets cg.refdef view values
===============
*/
void CG_EmplacedView(vec3_t angles);
static int CG_CalcViewValues( void ) {
	qboolean manningTurret = qfalse;
	playerState_t	*ps;

	if (!drawingSniperScopeView)
	{
		memset( &cg.refdef, 0, sizeof( cg.refdef ) );

		// strings for in game rendering
		// Q_strncpyz( cg.refdef.text[0], "Park Ranger", sizeof(cg.refdef.text[0]) );
		// Q_strncpyz( cg.refdef.text[1], "19", sizeof(cg.refdef.text[1]) );

		// calculate size of 3D view
		CG_CalcVrect();
	}

	ps = &cg.predictedPlayerState;
/*
	if (cg.cameraMode) {
		vec3_t origin, angles;
		if (trap->getCameraInfo(cg.time, &origin, &angles)) {
			VectorCopy(origin, cg.refdef.vieworg);
			angles[ROLL] = 0;
			VectorCopy(angles, cg.refdef.viewangles);
			AnglesToAxis( cg.refdef.viewangles, cg.refdef.viewaxis );
			return CG_CalcFov();
		} else {
			cg.cameraMode = qfalse;
		}
	}
*/
	// intermission view
	if ( ps->pm_type == PM_INTERMISSION ) {
		VectorCopy( ps->origin, cg.refdef.vieworg );

#ifdef __OLD_VR__
		VectorCopy(ps->viewangles, cg.refdefViewAngles);
		AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
#else //!__OLD_VR__
		VectorCopy(ps->viewangles, cg.refdef.viewangles);
		AnglesToAxis(cg.refdef.viewangles, cg.refdef.viewaxis);
#endif //__OLD_VR__
		
		return CG_CalcFov();
	}

	cg.bobcycle = ( ps->bobCycle & 128 ) >> 7;
	cg.bobfracsin = fabs( sin( ( ps->bobCycle & 127 ) / 127.0 * M_PI ) );
	cg.xyspeed = sqrt( ps->velocity[0] * ps->velocity[0] +
		ps->velocity[1] * ps->velocity[1] );

	if (cg.xyspeed > 270)
	{
		cg.xyspeed = 270;
	}

	manningTurret = CG_CheckPassengerTurretView();
	if ( !manningTurret )
	{//not manning a turret on a vehicle
		VectorCopy( ps->origin, cg.refdef.vieworg );
#ifdef __OLD_VR__
		VectorCopy(ps->viewangles, cg.refdefViewAngles);
#endif //__OLD_VR__

#ifdef VEH_CONTROL_SCHEME_4
		if ( cg.predictedPlayerState.m_iVehicleNum )//in a vehicle
		{
			Vehicle_t *pVeh = cg_entities[cg.predictedPlayerState.m_iVehicleNum].m_pVehicle;
			if ( BG_UnrestrainedPitchRoll( &cg.predictedPlayerState, pVeh ) )//can roll/pitch without restriction
			{//use the vehicle's viewangles to render view!
				VectorCopy( cg.predictedVehicleState.viewangles, cg.refdef.viewangles );
			}
			else if ( pVeh //valid vehicle data pointer
				&& pVeh->m_pVehicleInfo//valid vehicle info
				&& pVeh->m_pVehicleInfo->type == VH_FIGHTER )//fighter
			{
				VectorCopy( cg.predictedVehicleState.viewangles, cg.refdef.viewangles );
				cg.refdef.viewangles[PITCH] = AngleNormalize180( cg.refdef.viewangles[PITCH] );
			}
			else
			{
				VectorCopy( ps->viewangles, cg.refdef.viewangles );
			}
		}
#else// VEH_CONTROL_SCHEME_4
		if ( cg.predictedPlayerState.m_iVehicleNum //in a vehicle
			&& BG_UnrestrainedPitchRoll( &cg.predictedPlayerState, cg_entities[cg.predictedPlayerState.m_iVehicleNum].m_pVehicle ) )//can roll/pitch without restriction
		{//use the vehicle's viewangles to render view!
			VectorCopy( cg.predictedVehicleState.viewangles, cg.refdef.viewangles );
		}
#endif// VEH_CONTROL_SCHEME_4
		else
		{
			VectorCopy( ps->viewangles, cg.refdef.viewangles );
		}
	}
	VectorCopy( cg.refdef.viewangles, cg_lastTurretViewAngles );

	// Update height offsets for model scales and 3rd person range...
	CG_UpdateCamera();

	if (cg_cameraOrbit.integer) {
		if (cg.time > cg.nextOrbitTime) {
			cg.nextOrbitTime = cg.time + cg_cameraOrbitDelay.integer;
			cg_thirdPersonAngle.value += cg_cameraOrbit.value;
		}
	}
	// add error decay
	if ( cg_errorDecay.value > 0 ) {
		int		t;
		float	f;

		t = cg.time - cg.predictedErrorTime;
		f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
		if ( f > 0 && f < 1 ) {
			VectorMA( cg.refdef.vieworg, f, cg.predictedError, cg.refdef.vieworg );
		} else {
			cg.predictedErrorTime = 0;
		}
	}

	if (cg.snap->ps.weapon == WP_EMPLACED_GUN &&
		cg.snap->ps.emplacedIndex)
	{ //constrain the view properly for emplaced guns
		CG_EmplacedView(cg_entities[cg.snap->ps.emplacedIndex].currentState.angles);
	}

	if ( !manningTurret )
	{
		if ( cg.predictedPlayerState.m_iVehicleNum //in a vehicle
			&& BG_UnrestrainedPitchRoll( &cg.predictedPlayerState, cg_entities[cg.predictedPlayerState.m_iVehicleNum].m_pVehicle ) )//can roll/pitch without restriction
		{//use the vehicle's viewangles to render view!
			CG_OffsetFighterView();
		}
		else if ( cg.renderingThirdPerson ) {
			// back away from character
			if (cg_thirdPersonSpecialCam.integer &&
				BG_SaberInSpecial(cg.snap->ps.saberMove))
			{ //the action cam
				if (!CG_ThirdPersonActionCam())
				{ //couldn't do it for whatever reason, resort back to third person then
					CG_OffsetThirdPersonView();
				}
			}
			else
			{
				CG_OffsetThirdPersonView();
			}
		} else {
			// offset for local bobbing and kicks
			CG_OffsetFirstPersonView();
		}
	}

#ifdef __OLD_VR__
	CG_OffsetVRView();

	// position eye relative to origin
	AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
#else //!__OLD_VR__
	// position eye relative to origin
	AnglesToAxis( cg.refdef.viewangles, cg.refdef.viewaxis );
#endif //__OLD_VR__

	if ( cg.hyperspace ) {
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	}

	// field of view
	return CG_CalcFov();
}


/*
=====================
CG_PowerupTimerSounds
=====================
*/
static void CG_PowerupTimerSounds( void ) {
	int		i;
	int		t;

	// powerup timers going away
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		t = cg.snap->ps.powerups[i];
		if ( t <= cg.time ) {
			continue;
		}
		if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
			continue;
		}
		if ( ( t - cg.time ) / POWERUP_BLINK_TIME != ( t - cg.oldTime ) / POWERUP_BLINK_TIME ) {
			//trap->S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_ITEM, cgs.media.wearOffSound );
		}
	}
}

/*
==============
CG_DrawSkyBoxPortal
==============
*/
extern qboolean cg_skyOri;
extern vec3_t cg_skyOriPos;
extern float cg_skyOriScale;
extern qboolean cg_noFogOutsidePortal;
void CG_DrawSkyBoxPortal(const char *cstr)
{
	refdef_t backuprefdef;
	float fov_x;
	float fov_y;
	float x;
	char *token;
	float f = 0;

	backuprefdef = cg.refdef;

	COM_BeginParseSession ("CG_DrawSkyBoxPortal");

	token = COM_ParseExt(&cstr, qfalse);
	if (!token || !token[0])
	{
		trap->Error( ERR_DROP, "CG_DrawSkyBoxPortal: error parsing skybox configstring\n");
	}
	cg.refdef.vieworg[0] = atof(token);

	token = COM_ParseExt(&cstr, qfalse);
	if (!token || !token[0])
	{
		trap->Error( ERR_DROP, "CG_DrawSkyBoxPortal: error parsing skybox configstring\n");
	}
	cg.refdef.vieworg[1] = atof(token);

	token = COM_ParseExt(&cstr, qfalse);
	if (!token || !token[0])
	{
		trap->Error( ERR_DROP, "CG_DrawSkyBoxPortal: error parsing skybox configstring\n");
	}
	cg.refdef.vieworg[2] = atof(token);

	token = COM_ParseExt(&cstr, qfalse);
	if (!token || !token[0])
	{
		trap->Error( ERR_DROP, "CG_DrawSkyBoxPortal: error parsing skybox configstring\n");
	}
	fov_x = atoi(token);

	if (!fov_x)
	{
		fov_x = cg_fov.value;
	}

	// setup fog the first time, ignore this part of the configstring after that
	token = COM_ParseExt(&cstr, qfalse);
	if (!token || !token[0])
	{
		trap->Error( ERR_DROP, "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog state\n");
	}

	if ( cg.predictedPlayerState.pm_type == PM_INTERMISSION )
	{
		// if in intermission, use a fixed value
		fov_x = cg_fov.value;
	}
	else
	{
		fov_x = cg_fov.value;

		if ( fov_x < 1 )
		{
			fov_x = 1;
		}
		else if ( fov_x > 160 )
		{
			fov_x = 160;
		}

		if (cg.predictedPlayerState.scopeType)
		{
			fov_x = zoomFov;
		}

		// do smooth transitions for zooming
		if (cg.predictedPlayerState.scopeType)
		{ //zoomed/zooming in
			f = ( cg.time - cg.zoomTime ) / (float)ZOOM_OUT_TIME;
			if ( f > 1.0 ) {
				fov_x = zoomFov;
			} else {
				fov_x = fov_x + f * ( zoomFov - fov_x );
			}
		}
		else
		{ //zooming out
			f = ( cg.time - cg.zoomTime ) / (float)ZOOM_OUT_TIME;
			if ( f <= 1.0 ) {
				fov_x = zoomFov + f * ( fov_x - zoomFov);
			}
		}
	}

	x = cg.refdef.width / tan( fov_x / 360 * M_PI );
	fov_y = atan2( cg.refdef.height, x );
	fov_y = fov_y * 360 / M_PI;

	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;

	cg.refdef.rdflags |= RDF_SKYBOXPORTAL;
	cg.refdef.rdflags |= RDF_DRAWSKYBOX;

	cg.refdef.time = cg.time;

	if ( !cg.hyperspace)
	{ //rww - also had to add this to add effects being rendered in portal sky areas properly.
		trap->FX_AddScheduledEffects(qtrue);
	}

	CG_AddPacketEntities(qtrue); //rww - There was no proper way to put real entities inside the portal view before.
									//This will put specially flagged entities in the render.

	if (cg_skyOri)
	{ //ok, we want to orient the sky refdef vieworg based on the normal vieworg's relation to the ori pos
		vec3_t dif;

		VectorSubtract(backuprefdef.vieworg, cg_skyOriPos, dif);
		VectorScale(dif, cg_skyOriScale, dif);
		VectorAdd(cg.refdef.vieworg, dif, cg.refdef.vieworg);
	}

	if (cg_noFogOutsidePortal)
	{ //make sure no fog flag is stripped first, and make sure it is set on the normal refdef
		cg.refdef.rdflags &= ~RDF_NOFOG;
		backuprefdef.rdflags |= RDF_NOFOG;
	}

	// draw the skybox
	trap->R_RenderScene( &cg.refdef );

	cg.refdef = backuprefdef;
}

/*
=====================
CG_AddBufferedSound
=====================
*/
void CG_AddBufferedSound( sfxHandle_t sfx ) {
	if ( !sfx )
		return;
	cg.soundBuffer[cg.soundBufferIn] = sfx;
	cg.soundBufferIn = (cg.soundBufferIn + 1) % MAX_SOUNDBUFFER;
	if (cg.soundBufferIn == cg.soundBufferOut) {
		cg.soundBufferOut++;
	}
}

/*
=====================
CG_PlayBufferedSounds
=====================
*/
static void CG_PlayBufferedSounds( void ) {
	if ( cg.soundTime < cg.time ) {
		if (cg.soundBufferOut != cg.soundBufferIn && cg.soundBuffer[cg.soundBufferOut]) {
			trap->S_StartLocalSound(cg.soundBuffer[cg.soundBufferOut], CHAN_ANNOUNCER);
			cg.soundBuffer[cg.soundBufferOut] = 0;
			cg.soundBufferOut = (cg.soundBufferOut + 1) % MAX_SOUNDBUFFER;
			cg.soundTime = cg.time + 750;
		}
	}
}

void CG_UpdateSoundTrackers()
{
	int num;

	for ( num = 0 ; num < ENTITYNUM_NONE ; num++ )
	{
		centity_t *cent = &cg_entities[num];

		if (cent && (cent->currentState.eFlags & EF_SOUNDTRACKER) && cent->currentState.number == num)
			//make sure the thing is valid at least.
		{ //keep sound for this entity updated in accordance with its attached entity at all times
			if (cg.snap && cent->currentState.trickedentindex == cg.snap->ps.clientNum)
			{ //this is actually the player, so center the sound origin right on top of us
				VectorCopy(cg.refdef.vieworg, cent->lerpOrigin);
				trap->S_UpdateEntityPosition( cent->currentState.number, cent->lerpOrigin );
			}
			else
			{
				trap->S_UpdateEntityPosition( cent->currentState.number, cg_entities[cent->currentState.trickedentindex].lerpOrigin );
			}
		}

		if (cent->currentState.number == num)
		{
			//update all looping sounds..
			CG_S_UpdateLoopingSounds(num);
		}
	}
}

//=========================================================================

/*
================================
Screen Effect stuff starts here
================================
*/
#define	CAMERA_DEFAULT_FOV			90.0f
#define MAX_SHAKE_INTENSITY			16.0f

cgscreffects_t cgScreenEffects;

void CG_SE_UpdateShake( vec3_t origin, vec3_t angles )
{
	vec3_t	moveDir;
	float	intensity_scale, intensity;
	int		i;

	if ( cgScreenEffects.shake_duration <= 0 )
		return;

	if ( cg.time > ( cgScreenEffects.shake_start + cgScreenEffects.shake_duration ) )
	{
		cgScreenEffects.shake_intensity = 0;
		cgScreenEffects.shake_duration = 0;
		cgScreenEffects.shake_start = 0;
		return;
	}

	cgScreenEffects.FOV = CAMERA_DEFAULT_FOV;
	cgScreenEffects.FOV2 = CAMERA_DEFAULT_FOV;

	//intensity_scale now also takes into account FOV with 90.0 as normal
	intensity_scale = 1.0f - ( (float) ( cg.time - cgScreenEffects.shake_start ) / (float) cgScreenEffects.shake_duration ) * (((cgScreenEffects.FOV+cgScreenEffects.FOV2)/2.0f)/90.0f);

	intensity = cgScreenEffects.shake_intensity * intensity_scale;

	for ( i = 0; i < 3; i++ )
	{
		moveDir[i] = ( crandom() * intensity );
	}

	//Move the camera
	VectorAdd( origin, moveDir, origin );

	for ( i=0; i < 2; i++ ) // Don't do ROLL
		moveDir[i] = ( crandom() * intensity );

	//Move the angles
	VectorAdd( angles, moveDir, angles );
}

void CG_SE_UpdateMusic(void)
{
	if (cgScreenEffects.music_volume_multiplier < 0.1)
	{
		cgScreenEffects.music_volume_multiplier = 1.0;
		return;
	}

	if (cgScreenEffects.music_volume_time < cg.time)
	{
		if (cgScreenEffects.music_volume_multiplier != 1.0f || cgScreenEffects.music_volume_set)
		{
			char musMultStr[512];

			cgScreenEffects.music_volume_multiplier += 0.1f;
			if (cgScreenEffects.music_volume_multiplier > 1.0f)
			{
				cgScreenEffects.music_volume_multiplier = 1.0f;
			}

			Com_sprintf(musMultStr, sizeof(musMultStr), "%f", cgScreenEffects.music_volume_multiplier);
			trap->Cvar_Set("s_musicMult", musMultStr);

			if (cgScreenEffects.music_volume_multiplier == 1.0f)
			{
				cgScreenEffects.music_volume_set = qfalse;
			}
			else
			{
				cgScreenEffects.music_volume_time = cg.time + 200;
			}
		}

		return;
	}

	if (!cgScreenEffects.music_volume_set)
	{ //if the volume_time is >= cg.time, we should have a volume multiplier set
		char musMultStr[512];

		Com_sprintf(musMultStr, sizeof(musMultStr), "%f", cgScreenEffects.music_volume_multiplier);
		trap->Cvar_Set("s_musicMult", musMultStr);
		cgScreenEffects.music_volume_set = qtrue;
	}
}

/*
=================
CG_CalcScreenEffects

Currently just for screen shaking (and music volume management)
=================
*/
void CG_CalcScreenEffects(void)
{
	CG_SE_UpdateShake(cg.refdef.vieworg, cg.refdef.viewangles);
	CG_SE_UpdateMusic();
}

void CGCam_Shake( float intensity, int duration )
{
	if ( intensity > MAX_SHAKE_INTENSITY )
		intensity = MAX_SHAKE_INTENSITY;

	cgScreenEffects.shake_intensity = intensity;
	cgScreenEffects.shake_duration = duration;


	cgScreenEffects.shake_start = cg.time;
}

void CG_DoCameraShake( vec3_t origin, float intensity, int radius, int time )
{
	//FIXME: When exactly is the vieworg calculated in relation to the rest of the frame?s

	vec3_t	dir;
	float	dist, intensityScale;
	float	realIntensity;

	VectorSubtract( cg.refdef.vieworg, origin, dir );
	dist = VectorNormalize( dir );

	//Use the dir to add kick to the explosion

	if ( dist > radius )
		return;

	intensityScale = 1 - ( dist / (float) radius );
	realIntensity = intensity * intensityScale;

	CGCam_Shake( realIntensity, time );
}

void CGCam_SetMusicMult( float multiplier, int duration )
{
	if (multiplier < 0.1f)
	{
		multiplier = 0.1f;
	}

	if (multiplier > 1.0f)
	{
		multiplier = 1.0f;
	}

	cgScreenEffects.music_volume_multiplier = multiplier;
	cgScreenEffects.music_volume_time = cg.time + duration;
	cgScreenEffects.music_volume_set = qfalse;
}

/*
================================
Screen Effect stuff ends here
================================
*/

/*
=================
CG_EmplacedView

Keep view reasonably constrained in relation to gun -rww
=================
*/
int BG_EmplacedView(vec3_t baseAngles, vec3_t angles, float *newYaw, float constraint);

void CG_EmplacedView(vec3_t angles)
{
	float yaw;
	int override;

	override = BG_EmplacedView(cg.refdef.viewangles, angles, &yaw,
		cg_entities[cg.snap->ps.emplacedIndex].currentState.origin2[0]);

	if (override)
	{
        cg.refdef.viewangles[YAW] = yaw;
		AnglesToAxis(cg.refdef.viewangles, cg.refdef.viewaxis);

		if (override == 2)
		{
			trap->SetClientForceAngle(cg.time + 5000, cg.refdef.viewangles);
		}
	}

	//we want to constrain the predicted player state viewangles as well
	override = BG_EmplacedView(cg.predictedPlayerState.viewangles, angles, &yaw,
		cg_entities[cg.snap->ps.emplacedIndex].currentState.origin2[0]);
	if (override)
	{
        cg.predictedPlayerState.viewangles[YAW] = yaw;
	}
}

//specially add cent's for automap
static void CG_AddRefentForAutoMap(centity_t *cent)
{
	refEntity_t ent;
	vec3_t flat;

	if (cent->currentState.eFlags & EF_NODRAW)
	{
		return;
	}

	memset(&ent, 0, sizeof(refEntity_t));
	ent.reType = RT_MODEL;

	VectorCopy(cent->lerpAngles, flat);
	flat[PITCH] = flat[ROLL] = 0.0f;

	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(flat, ent.angles);
	AnglesToAxis(flat, ent.axis);

	if (cent->ghoul2 &&
		(cent->currentState.eType == ET_PLAYER ||
		cent->currentState.eType == ET_NPC ||
		cent->currentState.modelGhoul2))
	{ //using a ghoul2 model
		ent.ghoul2 = cent->ghoul2;
		ent.radius = cent->currentState.g2radius;

		if (!ent.radius)
		{
			ent.radius = 64.0f;
		}
	}
	else
	{ //then assume a standard indexed model
		ent.hModel = cgs.gameModels[cent->currentState.modelindex];
	}

	AddRefEntityToScene(&ent);
}

//add all entities that would be on the radar
void CG_AddRadarAutomapEnts(void)
{
	int i = 0;

	//first add yourself
	CG_AddRefentForAutoMap(&cg_entities[cg.predictedPlayerState.clientNum]);

	while (i < cg.radarEntityCount)
	{
		CG_AddRefentForAutoMap(&cg_entities[cg.radarEntities[i]]);
		i++;
	}
}

/*
================
CG_DrawAutoMap

Draws the automap scene. -rww
================
*/
float cg_autoMapZoom = 512.0f;
float cg_autoMapZoomMainOffset = 0.0f;
vec3_t cg_autoMapAngle = {90.0f, 0.0f, 0.0f};
autoMapInput_t cg_autoMapInput;
int cg_autoMapInputTime = 0;
#define	SIDEFRAME_WIDTH			16
#define	SIDEFRAME_HEIGHT		32
void CG_DrawAutoMap(void)
{
	clientInfo_t	*local;
	refdef_t		refdef;
	trace_t			tr;
	vec3_t			fwd;
	vec3_t			playerMins, playerMaxs;
	int				vWidth, vHeight;
	float			hScale, vScale;
	float			x, y, w, h;

	if (!r_autoMap.integer)
	{ //don't do anything then
		return;
	}

	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 )
	{ //don't show when dead
		return;
	}

	if ( (cg.predictedPlayerState.pm_flags & PMF_FOLLOW) || cg.predictedPlayerState.persistant[PERS_TEAM] == FACTION_SPECTATOR )
	{ //don't show when spec
		return;
	}

	local = &cgs.clientinfo[ cg.predictedPlayerState.clientNum ];
	if ( !local->infoValid )
	{ //don't show if bad ci
		return;
	}

	if (cgs.gametype < GT_TEAM)
	{ //don't show in non-team gametypes
		return;
	}

	if (cg_autoMapInputTime >= cg.time)
	{
		if (cg_autoMapInput.up)
		{
			cg_autoMapZoom -= cg_autoMapInput.up;
			if (cg_autoMapZoom < cg_autoMapZoomMainOffset+64.0f)
			{
				cg_autoMapZoom = cg_autoMapZoomMainOffset+64.0f;
			}
		}

		if (cg_autoMapInput.down)
		{
			cg_autoMapZoom += cg_autoMapInput.down;
			if (cg_autoMapZoom > cg_autoMapZoomMainOffset+4096.0f)
			{
				cg_autoMapZoom = cg_autoMapZoomMainOffset+4096.0f;
			}
		}

		if (cg_autoMapInput.yaw)
		{
			cg_autoMapAngle[YAW] += cg_autoMapInput.yaw;
		}

		if (cg_autoMapInput.pitch)
		{
			cg_autoMapAngle[PITCH] += cg_autoMapInput.pitch;
		}

		if (cg_autoMapInput.goToDefaults)
		{
			cg_autoMapZoom = 512.0f;
			VectorSet(cg_autoMapAngle, 90.0f, 0.0f, 0.0f);
		}
	}

	memset( &refdef, 0, sizeof( refdef ) );

	refdef.rdflags = (RDF_NOWORLDMODEL|RDF_AUTOMAP);

	VectorCopy(cg.predictedPlayerState.origin, refdef.vieworg);
	VectorCopy(cg_autoMapAngle, refdef.viewangles);

	//scale out in the direction of the view angles base on the zoom factor
	AngleVectors(refdef.viewangles, fwd, 0, 0);
	VectorMA(refdef.vieworg, -cg_autoMapZoom, fwd, refdef.vieworg);

	AnglesToAxis(refdef.viewangles, refdef.viewaxis);

	refdef.fov_x = 50;
	refdef.fov_y = 50;

	//guess this doesn't need to be done every frame, but eh
	trap->R_GetRealRes(&vWidth, &vHeight);

	//set scaling values so that the 640x480 will result at 1.0/1.0
	hScale = vWidth/640.0f;
	vScale = vHeight/480.0f;

	x = r_autoMapX.value;
	y = r_autoMapY.value;
	w = r_autoMapW.value;
	h = r_autoMapH.value;

	refdef.x = x*hScale;
	refdef.y = y*vScale;
	refdef.width = w*hScale;
	refdef.height = h*vScale;

	CG_DrawPic(x-SIDEFRAME_WIDTH, y, SIDEFRAME_WIDTH, h, cgs.media.wireframeAutomapFrame_left);
	CG_DrawPic(x+w, y, SIDEFRAME_WIDTH, h, cgs.media.wireframeAutomapFrame_right);
	CG_DrawPic(x-SIDEFRAME_WIDTH, y-SIDEFRAME_HEIGHT, w+(SIDEFRAME_WIDTH*2), SIDEFRAME_HEIGHT, cgs.media.wireframeAutomapFrame_top);
	CG_DrawPic(x-SIDEFRAME_WIDTH, y+h, w+(SIDEFRAME_WIDTH*2), SIDEFRAME_HEIGHT, cgs.media.wireframeAutomapFrame_bottom);

	refdef.time = cg.time;

	trap->R_ClearScene();
	CG_AddRadarAutomapEnts();

	if (cg.predictedPlayerState.m_iVehicleNum &&
		cg_entities[cg.predictedPlayerState.m_iVehicleNum].currentState.eType == ET_NPC &&
		cg_entities[cg.predictedPlayerState.m_iVehicleNum].currentState.NPC_class == CLASS_VEHICLE &&
		cg_entities[cg.predictedPlayerState.m_iVehicleNum].m_pVehicle &&
		cg_entities[cg.predictedPlayerState.m_iVehicleNum].m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
	{ //constantly adjust to current height
		trap->R_AutomapElevationAdjustment(cg.predictedPlayerState.origin[2]);
	}
	else
	{
		//Trace down and set the ground elevation as the main automap elevation point
		VectorSet(playerMins, -15, -15, DEFAULT_MINS_2);
		VectorSet(playerMaxs, 15, 15, DEFAULT_MAXS_2);

		VectorCopy(cg.predictedPlayerState.origin, fwd);
		fwd[2] -= 4096.0f;
		CG_Trace(&tr, cg.predictedPlayerState.origin, playerMins, playerMaxs, fwd, cg.predictedPlayerState.clientNum, MASK_SOLID);

		if (!tr.startsolid && !tr.allsolid)
		{
			trap->R_AutomapElevationAdjustment(tr.endpos[2]);
		}
	}
	trap->R_RenderScene( &refdef );
}

//=========================================================================

/*
**  Frustum code
*/

// some culling bits
typedef struct plane_s {
	vec3_t normal;
	float dist;
} plane_t;

static plane_t frustum[4];

//
//	CG_SetupFrustum
//
void CG_SetupFrustum( void ) {
	int i;
	float xs, xc;
	float ang;

	ang = cg.refdef.fov_x / 180 * M_PI * 0.5f;
	xs = sin( ang );
	xc = cos( ang );

	VectorScale( cg.refdef.viewaxis[0], xs, frustum[0].normal );
	VectorMA( frustum[0].normal, xc, cg.refdef.viewaxis[1], frustum[0].normal );

	VectorScale( cg.refdef.viewaxis[0], xs, frustum[1].normal );
	VectorMA( frustum[1].normal, -xc, cg.refdef.viewaxis[1], frustum[1].normal );

	ang = cg.refdef.fov_y / 180 * M_PI * 0.5f;
	xs = sin( ang );
	xc = cos( ang );

	VectorScale( cg.refdef.viewaxis[0], xs, frustum[2].normal );
	VectorMA( frustum[2].normal, xc, cg.refdef.viewaxis[2], frustum[2].normal );

	VectorScale( cg.refdef.viewaxis[0], xs, frustum[3].normal );
	VectorMA( frustum[3].normal, -xc, cg.refdef.viewaxis[2], frustum[3].normal );

	for ( i = 0 ; i < 4 ; i++ ) {
		frustum[i].dist = DotProduct( cg.refdef.vieworg, frustum[i].normal );
	}
}

//
//	CG_CullPoint - returns true if culled
//
qboolean CG_CullPoint( vec3_t pt ) {
	int i;
	plane_t *frust;

	// check against frustum planes
	for ( i = 0 ; i < 4 ; i++ ) {
		frust = &frustum[i];

		if ( ( DotProduct( pt, frust->normal ) - frust->dist ) < 0 ) {
			return( qtrue );
		}
	}

	return( qfalse );
}

qboolean CG_CullPointAndRadius( const vec3_t pt, float radius ) {
	int i;
	plane_t *frust;

	// check against frustum planes
	for ( i = 0 ; i < 4 ; i++ ) {
		frust = &frustum[i];

		if ( ( DotProduct( pt, frust->normal ) - frust->dist ) < -radius ) {
			return( qtrue );
		}
	}

	return( qfalse );
}

//=========================================================================

/*
=================
CG_DrawActiveFrame

Generates and draws a game scene and status information at the given time.
=================
*/
static qboolean cg_rangedFogging = qfalse; //so we know if we should go back to normal fog
float cg_linearFogOverride = 0.0f; //designer-specified override for linear fogging style

extern void BG_VehicleTurnRateForSpeed( Vehicle_t *pVeh, float speed, float *mPitchOverride, float *mYawOverride );
extern qboolean PM_InKnockDown( playerState_t *ps );

extern qboolean cgQueueLoad;
extern void CG_ActualLoadDeferredPlayers( void );

static int cg_siegeClassIndex = -2;

extern void CG_Draw2D( void );
extern void InventoryWindow ( void );
//extern void RenderGenericWeaponView ( void );

int sceneRenderID = 0;

void CG_DrawActiveFrame( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback ) {
	int		inwater;
	const char *cstr;
	float mSensitivity = cg.zoomSensitivity;
	float mPitchOverride = 0.0f;
	float mYawOverride = 0.0f;
	static centity_t *veh = NULL;
#ifdef VEH_CONTROL_SCHEME_4
	float mSensitivityOverride = 0.0f;
	qboolean bUseFighterPitch = qfalse;
	qboolean	isFighter = qfalse;
#endif

	if (!drawingSniperScopeView)
	{
		if (cgQueueLoad)
		{ //do this before you start messing around with adding ghoul2 refents and crap
			CG_ActualLoadDeferredPlayers();
			cgQueueLoad = qfalse;
		}

		cg.time = serverTime;
		cg.demoPlayback = demoPlayback;

		if (cg.snap && ui_myteam.integer != cg.snap->ps.persistant[PERS_TEAM])
		{
			trap->Cvar_Set ( "ui_myteam", va("%i", cg.snap->ps.persistant[PERS_TEAM]) );
		}
		if (cgs.gametype == GT_SIEGE &&
			cg.snap &&
			cg_siegeClassIndex != cgs.clientinfo[cg.snap->ps.clientNum].siegeIndex)
		{
			cg_siegeClassIndex = cgs.clientinfo[cg.snap->ps.clientNum].siegeIndex;
			if (cg_siegeClassIndex == -1)
			{
				trap->Cvar_Set("ui_mySiegeClass", "<none>");
			}
			else
			{
				trap->Cvar_Set("ui_mySiegeClass", bgSiegeClasses[cg_siegeClassIndex].name);
			}
		}

		// update cvars
		CG_UpdateCvars();

		// if we are only updating the screen as a loading
		// pacifier, don't even try to read snapshots
		if ( cg.infoScreenText[0] != 0 ) {
			CG_DrawInformation();
			return;
		}

		trap->FX_AdjustTime( cg.time );

		CG_RunLightStyles();

		// any looped sounds will be respecified as entities
		// are added to the render list
		trap->S_ClearLoopingSounds();

		// clear all the render lists
		trap->R_ClearScene();

		// set up cg.snap and possibly cg.nextSnap
		CG_ProcessSnapshots();

		trap->ROFF_UpdateEntities();

		// if we haven't received any snapshots yet, all
		// we can draw is the information screen
		if ( !cg.snap || ( cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) )
		{
#if 0
			// Transition from zero to negative one on the snapshot timeout.
			// The reason we do this is because the first client frame is responsible for
			// some farily slow processing (such as weather) and we dont want to include
			// that processing time into our calculations
			if ( !cg.snapshotTimeoutTime )
			{
				cg.snapshotTimeoutTime = -1;
			}
			// Transition the snapshot timeout time from -1 to the current time in
			// milliseconds which will start the timeout.
			else if ( cg.snapshotTimeoutTime == -1 )
			{
				cg.snapshotTimeoutTime = trap->Milliseconds ( );
			}

			// If we have been waiting too long then just error out
			if ( cg.snapshotTimeoutTime > 0 && (trap->Milliseconds ( ) - cg.snapshotTimeoutTime > cg_snapshotTimeout.integer * 1000) )
			{
				Com_Error ( ERR_DROP, CG_GetStringEdString("MP_SVGAME", "SNAPSHOT_TIMEOUT"));
				return;
			}
#endif
			CG_DrawInformation();
			return;
		}
	}

	/* BEGIN: experimental framerate scaling */
	extern int CG_GetCurrentFPS(void);
	int fps = CG_GetCurrentFPS();

	qboolean renderCurrentScene = qtrue;

	/*
	if (fps <= r_fpsTarget.integer * r_fpsTargetMaxSkipFrames.integer)
	{
		sceneRenderID++;

		if (sceneRenderID <= r_fpsTargetMaxSkipFrames.integer)
		{
			renderCurrentScene = qfalse;
		}
		else
		{
			sceneRenderID = 0;
		}
	}
	*/
	if (fps < r_fpsTarget.integer)
	{
		if (sceneRenderID <= r_fpsTargetMaxSkipFrames.integer)
		{
			renderCurrentScene = qfalse;
			sceneRenderID++;
		}
		else
		{
			sceneRenderID = 0;
		}
	}
	else
	{
		sceneRenderID = 0;
	}
	
	/* END: experimental framerate scaling */

	// let the client system know what our weapon and zoom settings are
	if (cg.snap && cg.snap->ps.saberLockTime > cg.time)
	{
		mSensitivity = 0.01f;
	}
	else if (cg.predictedPlayerState.weapon == WP_EMPLACED_GUN)
	{ //lower sens for emplaced guns and vehicles
		mSensitivity = 0.2f;
	}
#ifdef VEH_CONTROL_SCHEME_4
	else if (cg.predictedPlayerState.m_iVehicleNum//in a vehicle
		&& !cg.predictedPlayerState.generic1 )//not as a passenger
	{
		centity_t *cent = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
		if ( cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo
			&& cent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER )
		{
			BG_VehicleTurnRateForSpeed( cent->m_pVehicle, cent->currentState.speed, &mPitchOverride, &mYawOverride );
			//mSensitivityOverride = 5.0f;//old default value
			mSensitivityOverride = 0.0f;
			bUseFighterPitch = qtrue;
			trap->SetUserCmdValue( cg.weaponSelect, mSensitivity, mPitchOverride, mYawOverride, mSensitivityOverride, cg.forceSelect, cg.itemSelect, bUseFighterPitch );
			isFighter = qtrue;
		}
	}

	if ( !isFighter )
#endif //VEH_CONTROL_SCHEME_4
	{
		if (!drawingSniperScopeView)
		{
			if (cg.predictedPlayerState.m_iVehicleNum)
			{
				veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
			}
			if (veh &&
				veh->currentState.eType == ET_NPC &&
				veh->currentState.NPC_class == CLASS_VEHICLE &&
				veh->m_pVehicle &&
				veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER &&
				bg_fighterAltControl.integer)
			{
				trap->SetUserCmdValue( cg.weaponSelect, mSensitivity, mPitchOverride, mYawOverride, 0.0f, cg.forceSelect, cg.itemSelect, qtrue );
				veh = NULL; //this is done because I don't want an extra assign each frame because I am so perfect and super efficient.
			}
			else
			{
				trap->SetUserCmdValue( cg.weaponSelect, mSensitivity, mPitchOverride, mYawOverride, 0.0f, cg.forceSelect, cg.itemSelect, qfalse );
			}
		}
	}

	if (!drawingSniperScopeView)
	{
		// this counter will be bumped for every valid scene we generate
		cg.clientFrame++;

		// update cg.predictedPlayerState
		CG_PredictPlayerState();

		// decide on third person view
		cg.renderingThirdPerson = qfalse;

		if ((cg_thirdPerson.integer || (cg.snap->ps.stats[STAT_HEALTH] <= 0)) && !(cg.refdef.rdflags & RDF_UNDERWATER))
			cg.renderingThirdPerson = qtrue;

		if (cg.snap->ps.stats[STAT_HEALTH] > 0)
		{
			if (cg.predictedPlayerState.weapon == WP_EMPLACED_GUN && cg.predictedPlayerState.emplacedIndex)
			{ //force third person for e-web and emplaced use
				cg.renderingThirdPerson = qtrue;
			}
			else if (cg.predictedPlayerState.weapon == WP_SABER || cg.predictedPlayerState.weapon == WP_MELEE)
			{//force thirdperson for sabers/melee
				cg.renderingThirdPerson = qtrue;
			}
			else if (cg.predictedPlayerState.fallingToDeath || cg.predictedPlayerState.m_iVehicleNum)
			{
				cg.renderingThirdPerson = qtrue;
			}
			/*
			else if (cg.predictedPlayerState.weapon == WP_SABER || cg.predictedPlayerState.weapon == WP_MELEE ||
			BG_InGrappleMove(cg.predictedPlayerState.torsoAnim) || BG_InGrappleMove(cg.predictedPlayerState.legsAnim) ||
			cg.predictedPlayerState.forceHandExtend == HANDEXTEND_KNOCKDOWN || cg.predictedPlayerState.fallingToDeath ||
			cg.predictedPlayerState.m_iVehicleNum || PM_InKnockDown(&cg.predictedPlayerState))
			{
			if (cg_fpls.integer && cg.predictedPlayerState.weapon == WP_SABER)
			{ //force to first person for fpls
			cg.renderingThirdPerson = qfalse;
			}
			else
			{
			cg.renderingThirdPerson = qtrue;
			}
			}
			*/
			else if (cg.snap->ps.scopeType)
			{ //always force first person when zoomed
				cg.renderingThirdPerson = qfalse;
			}

			if (cg.refdef.rdflags & RDF_UNDERWATER)
			{ //always force first person when underwater
				cg.renderingThirdPerson = qfalse;
			}
		}

		if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
		{ //always first person for spec
			cg.renderingThirdPerson = qfalse;
		}

		if (cg.snap->ps.persistant[PERS_TEAM] == FACTION_SPECTATOR)
		{
			cg.renderingThirdPerson = qfalse;
		}
	}

	// build cg.refdef
	inwater = CG_CalcViewValues();
	CG_SetupFrustum();

	if (CG_CheckRangedFog())
	{
		cg_rangedFogging = qtrue;
	}
	else if (cg_linearFogOverride)
	{
		trap->R_SetRangedFog(-cg_linearFogOverride);
	}
	else if (cg.predictedPlayerState.scopeType)
	{ //zooming with binoculars or sniper, set the fog range based on the zoom level -rww
		cg_rangedFogging = qtrue;
		//smaller the fov the less fog we have between the view and cull dist
		trap->R_SetRangedFog(cg.refdef.fov_x*64.0f);
	}
	else if (cg_rangedFogging)
	{ //disable it
		cg_rangedFogging = qfalse;
		trap->R_SetRangedFog(0.0f);
	}

	cstr = CG_ConfigString(CS_SKYBOXORG);

	if (cstr && cstr[0])
	{ //we have a skyportal
		CG_DrawSkyBoxPortal(cstr);
	}

	if (renderCurrentScene)
	{
		CG_CalcScreenEffects();

		// first person blend blobs, done after AnglesToAxis
		if (!cg.renderingThirdPerson && cg.predictedPlayerState.pm_type != PM_SPECTATOR) {
			CG_DamageBlendBlob();
		}

		// build the render lists
		if (!cg.hyperspace) {
			CG_AddPacketEntities(qfalse);			// adter calcViewValues, so predicted player state is correct
			CG_AddMarks();
			CG_AddLocalEntities();
			CG_AddAtmosphericEffects();  	// Add rain/snow etc.
		}

		if (!drawingSniperScopeView && cg.predictedPlayerState.weapon != WP_MODULIZED_WEAPON)
		{
			CG_AddViewWeapon(&cg.predictedPlayerState);
			//RenderGenericWeaponView();
		}
	}

	if (/*renderCurrentScene &&*/ !cg.hyperspace)
	{
		trap->FX_AddScheduledEffects(qfalse);
	}

	if (!drawingSniperScopeView)
	{
		// add buffered sounds
		CG_PlayBufferedSounds();
	}

	// finish up the rest of the refdef
	if (renderCurrentScene && cg.testModelEntity.hModel ) {
		CG_AddTestModel();
	}

	cg.refdef.time = cg.time;
	memcpy( cg.refdef.areamask, cg.snap->areamask, sizeof( cg.refdef.areamask ) );

	if (!drawingSniperScopeView)
	{
		// warning sounds when powerup is wearing off
		CG_PowerupTimerSounds();

		// if there are any entities flagged as sound trackers and attached to other entities, update their sound pos
		CG_UpdateSoundTrackers();
	}

	if (gCGHasFallVector)
	{
		vec3_t lookAng;

		VectorSubtract(cg.snap->ps.origin, cg.refdef.vieworg, lookAng);
		VectorNormalize(lookAng);
		vectoangles(lookAng, lookAng);

		VectorCopy(gCGFallVector, cg.refdef.vieworg);
		AnglesToAxis(lookAng, cg.refdef.viewaxis);
	}

	if (!drawingSniperScopeView)
	{
		//This is done from the vieworg to get origin for non-attenuated sounds
		cstr = CG_ConfigString( CS_GLOBAL_AMBIENT_SET );

		if (cstr && cstr[0])
		{
			trap->S_UpdateAmbientSet( cstr, cg.refdef.vieworg );
		}

		// update audio positions
		trap->S_Respatialize( cg.snap->ps.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis, inwater );
	}

	// make sure the lagometerSample and frame timing isn't done twice when in stereo
	cg.frametime = cg.time - cg.oldTime;
	if ( cg.frametime < 0 ) {
		cg.frametime = 0;
	}
	cg.oldTime = cg.time;
		
	if (!drawingSniperScopeView)
		CG_AddLagometerFrameInfo();

	if (timescale.value != cg_timescaleFadeEnd.value) {
		if (timescale.value < cg_timescaleFadeEnd.value) {
			timescale.value += cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if (timescale.value > cg_timescaleFadeEnd.value)
				timescale.value = cg_timescaleFadeEnd.value;
		}
		else {
			timescale.value -= cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if (timescale.value < cg_timescaleFadeEnd.value)
				timescale.value = cg_timescaleFadeEnd.value;
		}
		if (cg_timescaleFadeSpeed.value) {
			trap->Cvar_Set("timescale", va("%f", timescale.value));
		}
	}

	// actually issue the rendering calls
	if (renderCurrentScene)
	{
		CG_DrawActive(stereoView);
	}
	
	if ( !drawingSniperScopeView && cg.predictedPlayerState.scopeType >= SCOPE_BINOCULARS )
	{
		//
		// Draw the view inside the scope...
		//

		if (renderCurrentScene)
		{
			refdef_t		refdefBackup;

			memcpy(&refdefBackup, &cg.refdef, sizeof(refdef_t));

			drawingSniperScopeView = qtrue;

			if (scopeData[cg.predictedPlayerState.scopeType].scopeViewX == 0
				&& scopeData[cg.predictedPlayerState.scopeType].scopeViewY == 0
				&& scopeData[cg.predictedPlayerState.scopeType].scopeViewW == 0
				&& scopeData[cg.predictedPlayerState.scopeType].scopeViewH == 0)
			{// Unknown scope size for this, use the developer cvars to set size...
				// UQ1: Using 640x480 screen size. Adjusted by the CG_DrawScopeView to match real size...
#ifdef __SCOPE_DEVELOPER__
			// *IMPORTANT* !!! Disable __SCOPE_DEVELOPER__ as soon as scopes have all had their size values set in scopeData !!!
				cg.refdef.x = cg_scopeX.integer * cgs.screenXScale;
				cg.refdef.y = cg_scopeY.integer * cgs.screenYScale;
				cg.refdef.width = cg_scopeW.integer * cgs.screenXScale;
				cg.refdef.height = cg_scopeH.integer * cgs.screenYScale;
#else //!__SCOPE_DEVELOPER__
			// When not in scope developer mode, 0,0,0,0 means this scope never draws a second view inside the scope picture(s)...
#endif //__SCOPE_DEVELOPER__
			}
			else
			{// Known scope. Use set scopeData values...
				// UQ1: Using 640x480 screen size. Adjusted by the CG_DrawScopeView to match real size...
				cg.refdef.x = scopeData[cg.predictedPlayerState.scopeType].scopeViewX * cgs.screenXScale;
				cg.refdef.y = scopeData[cg.predictedPlayerState.scopeType].scopeViewY * cgs.screenYScale;
				cg.refdef.width = scopeData[cg.predictedPlayerState.scopeType].scopeViewW * cgs.screenXScale;
				cg.refdef.height = scopeData[cg.predictedPlayerState.scopeType].scopeViewH * cgs.screenYScale;
			}

			CG_DrawActiveFrame(serverTime, stereoView, demoPlayback);

			memcpy(&cg.refdef, &refdefBackup, sizeof(refdef_t));
		}

		drawingSniperScopeView = qfalse;
		return;
	}

	// draw status bar and other floating elements
	// UQ1: Now drawn here instead of CG_DrawActive() so we can overlay the zoom view above before drawing the scope graphics over that...
	if (renderCurrentScene || sceneRenderID == 1) // seems 2d is delayed?
	{
		CG_Draw2D();
		CG_DrawAutoMap();

		if (cg_stats.integer) {
			trap->Print("cg.clientFrame:%i\n", cg.clientFrame);
		}

		//InventoryWindow();
	}
}
