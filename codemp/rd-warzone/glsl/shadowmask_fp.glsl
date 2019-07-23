uniform sampler2D			u_ScreenDepthMap;

uniform sampler2DShadow		u_ShadowMap;
uniform sampler2DShadow		u_ShadowMap2;
uniform sampler2DShadow		u_ShadowMap3;

uniform mat4				u_ShadowMvp;
uniform mat4				u_ShadowMvp2;
uniform mat4				u_ShadowMvp3;

uniform vec4				u_Settings0;			// r_shadowSamples (numBlockerSearchSamples), SHADOW_MAP_SIZE, SHADOWS_FULL_SOLID, 0.0
uniform vec4				u_Settings1;			// SHADOW_Z_ERROR_OFFSET_NEAR, SHADOW_Z_ERROR_OFFSET_MID, SHADOW_Z_ERROR_OFFSET_FAR, r_testshaderValue1->value
uniform vec3				u_ViewOrigin;
uniform vec4				u_ViewInfo;				// zfar / znear, zfar, depthBits, znear

varying vec2				var_DepthTex;
varying vec3				var_ViewDir;

#define						SHADOW_MAP_SIZE			u_Settings0.g
#define						SHADOWS_FULL_SOLID		u_Settings0.b

#define SHADOW_Z_ERROR_OFFSET_NEAR u_Settings1.r//0.0001//u_Settings1.r//0.0001
#define SHADOW_Z_ERROR_OFFSET_MID u_Settings1.g//0.00001;//u_Settings1.r;//0.001//u_Settings1.r//0.0001
#define SHADOW_Z_ERROR_OFFSET_FAR u_Settings1.b//0.01;//u_Settings1.r//0.0001
#define MAX_SHADOW_VALUE_NORMAL 0.1
#define MAX_SHADOW_VALUE_SOLID 0.9999999

float offset_lookup(sampler2DShadow shadowmap, vec4 loc, vec2 offset, float scale, float depth, int cascade)
{
	//float result = textureProj(shadowmap, vec4(loc.xy + offset * scale * loc.w, (loc.z - u_Settings0.a), loc.w)) > 0.1 ? 1.0 : 0.0;
	//return result;

	float shadowError = SHADOW_Z_ERROR_OFFSET_NEAR;

	if (cascade == 1)
	{
		shadowError = SHADOW_Z_ERROR_OFFSET_MID;
	}
	else if (cascade == 2)
	{
		shadowError = SHADOW_Z_ERROR_OFFSET_FAR;
	}

	if (SHADOWS_FULL_SOLID <= 0.0)
	{
		return textureProj(shadowmap, vec4(loc.xy + offset * scale * loc.w, (loc.z + shadowError), loc.w)) > MAX_SHADOW_VALUE_NORMAL ? 1.0 : 0.0;
	}
	else 
	{
		return textureProj(shadowmap, vec4(loc.xy + offset * scale * loc.w, (loc.z + shadowError), loc.w));
	}
}

float PCF(const sampler2DShadow shadowmap, const vec4 st, const float dist, float scale, float depth, int cascade)
{
	float mult;
	vec4 sCoord = vec4(st);

	//vec2 offset = vec2(greaterThan(fract(st.xy * 0.5), vec2(0.25)));  // mod
	vec2 offset = mod(sCoord.xy, 0.5);
	offset.y += offset.x;  // y ^= x in floating point
	//if (offset.y > 1.1) offset.y = 0;
	if (offset.y > 1.0) offset.y = 0.0;

	if (SHADOWS_FULL_SOLID <= 0.0)
	{
		float shadowCoeff = (offset_lookup(shadowmap, sCoord, offset + vec2(-1.5, 0.5), scale, depth, cascade) +
			offset_lookup(shadowmap, sCoord, offset + vec2(0.5, 0.5), scale, depth, cascade) +
			offset_lookup(shadowmap, sCoord, offset + vec2(-1.5, -1.5), scale, depth, cascade) +
			offset_lookup(shadowmap, sCoord, offset + vec2(0.5, -1.5), scale, depth, cascade) ) 
			* 0.25;

		//return shadowCoeff > SHADOW_CUTOFF ? 1.0 : 0.0; // Hmm too harsh...
		return shadowCoeff;
	}
	else
	{
		if (offset_lookup(shadowmap, sCoord, offset + vec2(-1.5, 0.5), scale, depth, cascade) <= MAX_SHADOW_VALUE_SOLID) return 0.0;
		if (offset_lookup(shadowmap, sCoord, offset + vec2(0.5, 0.5), scale, depth, cascade) <= MAX_SHADOW_VALUE_SOLID) return 0.0;
		if (offset_lookup(shadowmap, sCoord, offset + vec2(-1.5, -1.5), scale, depth, cascade) <= MAX_SHADOW_VALUE_SOLID) return 0.0;
		if (offset_lookup(shadowmap, sCoord, offset + vec2(0.5, -1.5), scale, depth, cascade) <= MAX_SHADOW_VALUE_SOLID) return 0.0;

		return 1.0;
	}
}

//////////////////////////////////////////////////////////////////////////
void main()
{
	float result = 1.0;
	float depth = texture(u_ScreenDepthMap, var_DepthTex).x;

	vec4 biasPos = vec4(u_ViewOrigin + var_ViewDir * (depth - 0.5 / u_ViewInfo.x), 1.0);
	vec4 shadowpos = u_ShadowMvp * biasPos;

	if (all(lessThan(abs(shadowpos.xyz), vec3(abs(shadowpos.w)))))
	{
		shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
		result = PCF(u_ShadowMap, shadowpos, shadowpos.z, 1.0 / SHADOW_MAP_SIZE, depth, 0);
		gl_FragColor = vec4(result, depth, 0.0, 1.0);
		return;
	}

	biasPos.xyz -= var_ViewDir * (1.0 / u_ViewInfo.x);
	shadowpos = u_ShadowMvp2 * biasPos;

	if (all(lessThan(abs(shadowpos.xyz), vec3(abs(shadowpos.w)))))
	{
		shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
		result = PCF(u_ShadowMap2, shadowpos, shadowpos.z, 1.0 / (SHADOW_MAP_SIZE * 1.0), depth, 1);
		gl_FragColor = vec4(result, depth, 0.0, 1.0);
		return;
	}

	biasPos.xyz -= var_ViewDir * (1.0 / u_ViewInfo.x);
	shadowpos = u_ShadowMvp3 * biasPos;

	if (all(lessThan(abs(shadowpos.xyz), vec3(abs(shadowpos.w)))))
	{
		shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
		result = PCF(u_ShadowMap3, shadowpos, shadowpos.z, 1.0 / (SHADOW_MAP_SIZE * 2.0), depth, 2);
		gl_FragColor = vec4(result, depth, 0.0, 1.0);
		return;
	}
	
	gl_FragColor = vec4(result, depth, 0.0, 1.0);
}
