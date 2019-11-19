#include "cg_local.h"
#include "fx_local.h"

//
// 3D Blaster bolts...
//

qboolean DEFAULT_BLASTER_SHADERS_INITIALIZED = qfalse;

int numBoltGlowIndexes = 0;
qhandle_t boltBoltIndexes[1024] = { 0 };
vec3_t    boltLightColors[1024] = { 0 };


vec4_t		boltLightColorBlack = { 0, 0, 0, 1 };
vec4_t		boltLightColorRed = { 1, 0, 0, 1 };
vec4_t		boltLightColorGreen = { 0, 1, 0, 1 };
vec4_t		boltLightColorBlue = { 0, 0, 1, 1 };
vec4_t		boltLightColorYellow = { 1, 1, 0, 1 };
vec4_t		boltLightColorOrange = { 1, 0.5, 0, 1 };
vec4_t		boltLightColorMagenta = { 1, 0, 1, 1 };
vec4_t		boltLightColorCyan = { 0, 1, 1, 1 };
vec4_t		boltLightColorWhite = { 1, 1, 1, 1 };
vec3_t		boltLightColorPurple = { 1.0, 0.0, 1.0 };

void CG_MakeShaderBoltGlow(qhandle_t boltShader, qhandle_t newBoltGlowShader, vec3_t lightColor)
{// Make a list of all the glows matching the original bolts...
	boltBoltIndexes[numBoltGlowIndexes] = boltShader;
	VectorCopy(lightColor, boltLightColors[numBoltGlowIndexes]);
	numBoltGlowIndexes++;
}

void CG_RegisterDefaultBlasterShaders(void)
{// New blaster 3D bolt shader colors can be registered here and reused, rather then for each gun...
	if (DEFAULT_BLASTER_SHADERS_INITIALIZED) return;

	DEFAULT_BLASTER_SHADERS_INITIALIZED = qtrue;

	//
	//
	// Add any new bolt/glow-color/light-color combos here...
	//
	// Note: do not resuse the same shader with a different glow. Make a new shader for each if a different glow is needed...
	//

	cgs.media.whiteBlasterShot = trap->R_RegisterShader("laserbolt_white"); // the basic color of the actual bolt...
	CG_MakeShaderBoltGlow(cgs.media.whiteBlasterShot, trap->R_RegisterShader("laserbolt_white_glow"), boltLightColorWhite); // a glow shader matching the bolt color and glow color...

	cgs.media.yellowBlasterShot = trap->R_RegisterShader("laserbolt_yellow");
	CG_MakeShaderBoltGlow(cgs.media.yellowBlasterShot, trap->R_RegisterShader("laserbolt_yellow_glow"), boltLightColorYellow);

	cgs.media.redBlasterShot = trap->R_RegisterShader("laserbolt_red");
	CG_MakeShaderBoltGlow(cgs.media.redBlasterShot, trap->R_RegisterShader("laserbolt_red_glow"), boltLightColorRed);

	cgs.media.blueBlasterShot = trap->R_RegisterShader("laserbolt_blue");
	CG_MakeShaderBoltGlow(cgs.media.blueBlasterShot, trap->R_RegisterShader("laserbolt_blue_glow"), boltLightColorBlue);

	cgs.media.greenBlasterShot = trap->R_RegisterShader("laserbolt_green");
	CG_MakeShaderBoltGlow(cgs.media.greenBlasterShot, trap->R_RegisterShader("laserbolt_green_glow"), boltLightColorGreen);
	
	cgs.media.PurpleBlasterShot = trap->R_RegisterShader("laserbolt_purple");
	CG_MakeShaderBoltGlow(cgs.media.PurpleBlasterShot, trap->R_RegisterShader("laserbolt_purple_glow"), boltLightColorPurple);

	cgs.media.orangeBlasterShot = trap->R_RegisterShader("laserbolt_orange");
	CG_MakeShaderBoltGlow(cgs.media.orangeBlasterShot, trap->R_RegisterShader("laserbolt_orange_glow"), boltLightColorOrange);

	//custom gfx files for other bolts 

	cgs.media.BlasterBolt_Cap_BluePurple = trap->R_RegisterShader("BlasterBolt_Line_BluePurple");
	CG_MakeShaderBoltGlow(cgs.media.BlasterBolt_Cap_BluePurple, trap->R_RegisterShader("BlasterBolt_Cap_BluePurple"), boltLightColorPurple);

	//
	// UQ1: Adding saber colors here... Stoiss: Add any extra colors here...
	//

	cgs.media.whiteSaber = trap->R_RegisterShader("laserbolt_white"); // the basic color of the actual bolt...
	CG_MakeShaderBoltGlow(cgs.media.whiteSaber, trap->R_RegisterShader("laserbolt_white_glow"), boltLightColorWhite);

	cgs.media.yellowSaber = trap->R_RegisterShader("laserbolt_yellow");
	CG_MakeShaderBoltGlow(cgs.media.yellowSaber, trap->R_RegisterShader("laserbolt_yellow_glow"), boltLightColorYellow);

	cgs.media.redSaber = trap->R_RegisterShader("laserbolt_red");
	CG_MakeShaderBoltGlow(cgs.media.redSaber, trap->R_RegisterShader("laserbolt_red_glow"), boltLightColorRed);

	cgs.media.blueSaber = trap->R_RegisterShader("laserbolt_blue");
	CG_MakeShaderBoltGlow(cgs.media.blueSaber, trap->R_RegisterShader("laserbolt_blue_glow"), boltLightColorBlue);

	cgs.media.greenSaber = trap->R_RegisterShader("laserbolt_green");
	CG_MakeShaderBoltGlow(cgs.media.greenSaber, trap->R_RegisterShader("laserbolt_green_glow"), boltLightColorGreen);

	cgs.media.purpleSaber = trap->R_RegisterShader("laserbolt_purple");
	CG_MakeShaderBoltGlow(cgs.media.purpleSaber, trap->R_RegisterShader("laserbolt_purple_glow"), boltLightColorPurple);

	cgs.media.orangeSaber = trap->R_RegisterShader("laserbolt_orange");
	CG_MakeShaderBoltGlow(cgs.media.orangeSaber, trap->R_RegisterShader("laserbolt_orange_glow"), boltLightColorOrange);

	cgs.media.bluePurpleSaber = trap->R_RegisterShader("BlasterBolt_Line_BluePurple");
	CG_MakeShaderBoltGlow(cgs.media.bluePurpleSaber, trap->R_RegisterShader("BlasterBolt_Cap_BluePurple"), boltLightColorPurple);
}

float *CG_Get3DWeaponBoltLightColor(qhandle_t boltShader)
{
	for (int i = 0; i < numBoltGlowIndexes; i++)
	{
		if (boltBoltIndexes[i] == boltShader)
		{
			return boltLightColors[i];
		}
	}

	return colorBlack;
}

qhandle_t CG_Get3DWeaponBoltColor(const struct weaponInfo_s *weaponInfo, qboolean altFire)
{
	if (!weaponInfo) return cgs.media.whiteBlasterShot; // Fallback...

	if (!altFire && !weaponInfo->bolt3DShader) return -1;
	if (!altFire && !weaponInfo->bolt3DLength) return -1;
	if (altFire && !weaponInfo->bolt3DShaderAlt) return -1;
	if (altFire && !weaponInfo->bolt3DLength) return -1;

	if (altFire)
	{
		if (weaponInfo->bolt3DShaderAlt)
		{
			return weaponInfo->bolt3DShaderAlt;
		}

		return -1; // Fall back to old system...
	}
	else
	{
		if (weaponInfo->bolt3DShader)
		{
			return weaponInfo->bolt3DShader;
		}

		return -1; // Fall back to old system...
	}

	return cgs.media.whiteBlasterShot; // Fallback...
}

float CG_Get3DWeaponBoltLength(const struct weaponInfo_s *weaponInfo, qboolean altFire)
{
	if (!weaponInfo) return 1.0; // Default size...

	if (!altFire && !weaponInfo->bolt3DShader) return 0;
	if (!altFire && !weaponInfo->bolt3DLength) return 0;
	if (altFire && !weaponInfo->bolt3DShaderAlt) return 0;
	if (altFire && !weaponInfo->bolt3DLength) return 0;

	if (altFire)
	{
		if (weaponInfo->bolt3DLengthAlt)
		{
			return weaponInfo->bolt3DLengthAlt;
		}

		return 1.0; // Default size...
	}
	else
	{
		if (weaponInfo->bolt3DLength)
		{
			return weaponInfo->bolt3DLength;
		}

		return 1.0; // Default size...
	}

	return 1.0; // Default size...
}

float CG_Get3DWeaponBoltWidth(const struct weaponInfo_s *weaponInfo, qboolean altFire)
{
	if (!weaponInfo) return 1.0; // Default size...

	if (!altFire && !weaponInfo->bolt3DShader) return 0;
	if (!altFire && !weaponInfo->bolt3DLength) return 0;
	if (altFire && !weaponInfo->bolt3DShaderAlt) return 0;
	if (altFire && !weaponInfo->bolt3DLength) return 0;

	if (altFire)
	{
		if (weaponInfo->bolt3DWidthAlt)
		{
			return weaponInfo->bolt3DWidthAlt;
		}

		return 1.0; // Default size...
	}
	else
	{
		if (weaponInfo->bolt3DWidth)
		{
			return weaponInfo->bolt3DWidth;
		}

		return 1.0; // Default size...
	}

	return 1.0; // Default size...
}

void FX_WeaponBolt3D(vec3_t org, vec3_t fwd, float length, float radius, qhandle_t shader, qboolean addLight)
{
	refEntity_t ent;

	if (!shader) return;
	if (!length) return;
	if (!radius) return;

	// Draw the bolt core...
	memset(&ent, 0, sizeof(refEntity_t));
	ent.reType = RT_MODEL;

	ent.customShader = shader;

	ent.modelScale[0] = length;
	ent.modelScale[1] = radius;
	ent.modelScale[2] = radius;

	VectorCopy(org, ent.origin);
	vectoangles(fwd, ent.angles);
	AnglesToAxis(ent.angles, ent.axis);
	ScaleModelAxis(&ent);

	ent.hModel = trap->R_RegisterModel("models/warzone/lasers/laserbolt.md3");

	AddRefEntityToScene(&ent);

	if (addLight)
	{// Add light as well...
		vec3_t lightColor;
		VectorCopy(CG_Get3DWeaponBoltLightColor(shader), lightColor);
		trap->R_AddLightToScene(org, 200 + (rand() & 31), lightColor[0] /** 0.15*/, lightColor[1] /** 0.15*/, lightColor[2] /** 0.15*/);
	}
}

//
// 3D Saber bolts stuff...
//

qhandle_t CG_GetSaberBoltColor(saber_colors_t color)
{
	switch (color)
	{
	case SABER_RED:
		return cgs.media.redSaber;
		break;
	case SABER_ORANGE:
		return cgs.media.orangeSaber;
		break;
	case SABER_YELLOW:
		return cgs.media.yellowSaber;
		break;
	case SABER_GREEN:
		return cgs.media.greenSaber;
		break;
	case SABER_BLUE:
		return cgs.media.blueSaber;
		break;
	case SABER_PURPLE:
		return cgs.media.purpleSaber;
		break;
	case SABER_WHITE:
	case SABER_BLACK:
	case SABER_RGB:
	case SABER_PIMP:
	case SABER_SCRIPTED:
	default:
		return cgs.media.whiteSaber;
		break;
	}

	return cgs.media.whiteSaber; // Fallback...
}

void FX_SaberBolt3D(vec3_t org, vec3_t fwd, float length, float radius, qhandle_t shader)
{
	refEntity_t ent;

	// Draw the bolt core...
	memset(&ent, 0, sizeof(refEntity_t));
	ent.reType = RT_MODEL;

	ent.customShader = shader;

	ent.modelScale[0] = length;
	ent.modelScale[1] = radius;
	ent.modelScale[2] = radius;

	ent.renderfx |= RF_RGB_TINT;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;

	VectorCopy(org, ent.origin);
	vectoangles(fwd, ent.angles);
	AnglesToAxis(ent.angles, ent.axis);
	ScaleModelAxis(&ent);

	ent.hModel = trap->R_RegisterModel("models/warzone/lasers/laserbolt.md3");

	AddRefEntityToScene(&ent);

	// Add light as well...
	vec3_t lightColor;
	VectorCopy(CG_Get3DWeaponBoltLightColor(shader), lightColor);
	trap->R_AddLightToScene(org, 200 + (rand() & 31), lightColor[0] /** 0.15*/, lightColor[1] /** 0.15*/, lightColor[2] /** 0.15*/);
}

void CG_Do3DSaber(vec3_t origin, vec3_t dir, float length, float lengthMax, float radius, saber_colors_t color)
{
	vec3_t		mid;

	if (length < 0.5f)
	{
		// if the thing is so short, just forget even adding me.
		return;
	}

	// Find the midpoint of the saber for lighting purposes
	VectorMA(origin, length * 0.5f, dir, mid);

	float len = length / lengthMax;
	FX_SaberBolt3D(mid, dir, cg_saberLengthMult.value * len, cg_saberRadiusMult.value, CG_GetSaberBoltColor(color));
}

#define SABER_TRAIL_TIME	40.0f
#define FX_USE_ALPHA		0x08000000

extern qboolean BG_SuperBreakWinAnim(int anim);
extern qboolean WP_SaberBladeUseSecondBladeStyle(saberInfo_t *saber, int bladeNum);

void CG_DoSaberTrails(centity_t *cent, clientInfo_t *client, vec3_t org_, vec3_t end, vec3_t *axis_, saber_colors_t scolor, saberTrail_t *saberTrail, int saberNum, int bladeNum)
{
#if 1 // Meh, Stoiss thinks we should remove them, and I think I agree...
	effectTrailArgStruct_t fx;

	if (saberTrail)
	{// Trails...
		vec3_t	rgb1;
		float	useAlpha = 0.01;

		//saberTrail->duration = 0;
		saberTrail->duration = saberMoveData[cent->currentState.saberMove].trailLength;

		int trailDur = int(float(saberTrail->duration) / 5.0f);

		if (!trailDur)
		{ //hmm.. ok, default
			if (BG_SuperBreakWinAnim(cent->currentState.torsoAnim))
			{
				trailDur = 150;
			}
			else
			{
				trailDur = SABER_TRAIL_TIME;
			}
		}

		trailDur = int(float(trailDur) * 120.0/*cg_saberTrailMult.value*/);

		// if we happen to be timescaled or running in a high framerate situation, we don't want to flood
		// the system with very small trail slices...but perhaps doing it by distance would yield better results?
		if (cg.time >= saberTrail->lastTime + 2000) saberTrail->lastTime = cg.time + 2;

		if (cg.time > saberTrail->lastTime + 2 && cg.time < saberTrail->lastTime + 2000) // 2ms
		{
			qboolean inSaberMove = (BG_SaberInAttack(cent->currentState.saberMove) || BG_SuperBreakWinAnim(cent->currentState.torsoAnim)) ? qtrue : qfalse;

			if ((BG_SuperBreakWinAnim(cent->currentState.torsoAnim) || saberMoveData[cent->currentState.saberMove].trailLength > 0 || ((cent->currentState.powerups & (1 << PW_SPEED) && cg_speedTrail.integer)) || (cent->currentState.saberInFlight && saberNum == 0))) // if we have a stale segment, don't draw until we have a fresh one
			{
				float diff = 0;

				switch (scolor)
				{
				case SABER_RED:
					VectorSet(rgb1, 255.0f, 0.0f, 0.0f);
					break;
				case SABER_ORANGE:
					VectorSet(rgb1, 255.0f, 64.0f, 0.0f);
					break;
				case SABER_YELLOW:
					VectorSet(rgb1, 255.0f, 255.0f, 0.0f);
					break;
				case SABER_GREEN:
					VectorSet(rgb1, 0.0f, 255.0f, 0.0f);
					break;
				case SABER_BLUE:
					VectorSet(rgb1, 0.0f, 64.0f, 255.0f);
					useAlpha = 0.02;
					break;
				case SABER_PURPLE:
					VectorSet(rgb1, 220.0f, 0.0f, 255.0f);
					break;
				case SABER_RGB:
				case SABER_PIMP:
				case SABER_SCRIPTED:
				case SABER_BLACK:
				case SABER_WHITE:
					VectorSet(rgb1, 255.0f, 255.0f, 255.0f);
					break;
				default:
					VectorSet(rgb1, 0.0f, 64.0f, 255.0f);
					break;
				}

				// Here we will use the happy process of filling a struct in with arguments and passing it to a trap function
				// so that we can take the struct and fill in an actual CTrail type using the data within it once we get it
				// into the effects area

				// Go from new muzzle to new end...then to old end...back down to old muzzle...finally
				// connect back to the new muzzle...this is our trail quad
				VectorCopy(org_, fx.mVerts[0].origin);
				VectorMA(end, 3.0f, axis_[0], fx.mVerts[1].origin);

				VectorCopy(saberTrail->tip, fx.mVerts[2].origin);
				VectorCopy(saberTrail->base, fx.mVerts[3].origin);

				diff = cg.time - saberTrail->lastTime;

				// I'm not sure that clipping this is really the best idea
				//This prevents the trail from showing at all in low framerate situations.
				//if ( diff <= SABER_TRAIL_TIME * 2 )
				if ((inSaberMove && diff <= 10000) || (!inSaberMove && diff <= SABER_TRAIL_TIME * 2))
				{ //don't draw it if the last time is way out of date
					float oldAlpha = 1.0f - (diff / trailDur);

					if ((!WP_SaberBladeUseSecondBladeStyle(&client->saber[saberNum], bladeNum) && client->saber[saberNum].trailStyle == 1)
						|| (WP_SaberBladeUseSecondBladeStyle(&client->saber[saberNum], bladeNum) && client->saber[saberNum].trailStyle2 == 1))
					{//motion trail
						fx.mShader = cgs.media.swordTrailShader;
						VectorSet(rgb1, 32.0f, 32.0f, 32.0f); // make the sith sword trail pretty faint
															  //trailDur *= 2.0f; // stay around twice as long?
					}
					else
					{
						//extern qhandle_t CG_GetSaberBoltColor(saber_colors_t color);
						//fx.mShader = CG_GetSaberBoltColor((saber_colors_t)scolor);
						//VectorCopy(CG_Get3DWeaponBoltLightColor(fx.mShader), rgb1);
						fx.mShader = cgs.media.saberBlurShader;
					}

					fx.mKillTime = int(float(trailDur) / 40.0/*cg_saberTrailDecay.value*/);
					fx.mSetFlags = FX_USE_ALPHA;

					// New muzzle
					VectorCopy(rgb1, fx.mVerts[0].rgb);
					fx.mVerts[0].alpha = 255.0f * useAlpha/*cg_saberTrailAlpha.value*/ * oldAlpha;

					fx.mVerts[0].ST[0] = 0.0f;
					fx.mVerts[0].ST[1] = 1.0f;
					fx.mVerts[0].destST[0] = 1.0f;
					fx.mVerts[0].destST[1] = 1.0f;

					// new tip
					VectorCopy(rgb1, fx.mVerts[1].rgb);
					fx.mVerts[1].alpha = 255.0f * useAlpha/*cg_saberTrailAlpha.value*/ * oldAlpha;

					fx.mVerts[1].ST[0] = 0.0f;
					fx.mVerts[1].ST[1] = 0.0f;
					fx.mVerts[1].destST[0] = 1.0f;
					fx.mVerts[1].destST[1] = 0.0f;

					// old tip
					VectorCopy(rgb1, fx.mVerts[2].rgb);
					fx.mVerts[2].alpha = 0.0;// 255.0f * cg_saberTrailAlpha.value * oldAlpha;

					fx.mVerts[2].ST[0] = 1.0f - oldAlpha; // NOTE: this just happens to contain the value I want
					fx.mVerts[2].ST[1] = 0.0f;
					fx.mVerts[2].destST[0] = 1.0f + fx.mVerts[2].ST[0];
					fx.mVerts[2].destST[1] = 0.0f;

					// old muzzle
					VectorCopy(rgb1, fx.mVerts[3].rgb);
					fx.mVerts[3].alpha = 0.0;//255.0f * cg_saberTrailAlpha.value * oldAlpha;

					fx.mVerts[3].ST[0] = 1.0f - oldAlpha; // NOTE: this just happens to contain the value I want
					fx.mVerts[3].ST[1] = 1.0f;
					fx.mVerts[3].destST[0] = 1.0f + fx.mVerts[2].ST[0];
					fx.mVerts[3].destST[1] = 1.0f;

					trap->FX_AddPrimitive(&fx);
				}
			}

			// we must always do this, even if we aren't active..otherwise we won't know where to pick up from
			VectorCopy(org_, saberTrail->base);
			VectorMA(end, 3.0f, axis_[0], saberTrail->tip);
			saberTrail->lastTime = cg.time;
		}
	}
#endif
}