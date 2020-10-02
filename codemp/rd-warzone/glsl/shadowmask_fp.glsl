#if defined(USE_BINDLESS_TEXTURES)
layout(std140) uniform u_bindlessTexturesBlock
{
uniform sampler2D					u_DiffuseMap;
uniform sampler2D					u_LightMap;
uniform sampler2D					u_NormalMap;
uniform sampler2D					u_DeluxeMap;
uniform sampler2D					u_SpecularMap;
uniform sampler2D					u_PositionMap;
uniform sampler2D					u_WaterPositionMap;
uniform sampler2D					u_WaterHeightMap;
uniform sampler2D					u_HeightMap;
uniform sampler2D					u_GlowMap;
uniform sampler2D					u_EnvironmentMap;
uniform sampler2D					u_TextureMap;
uniform sampler2D					u_LevelsMap;
uniform sampler2D					u_CubeMap;
uniform sampler2D					u_SkyCubeMap;
uniform sampler2D					u_SkyCubeMapNight;
uniform sampler2D					u_EmissiveCubeMap;
uniform sampler2D					u_OverlayMap;
uniform sampler2D					u_SteepMap;
uniform sampler2D					u_SteepMap1;
uniform sampler2D					u_SteepMap2;
uniform sampler2D					u_SteepMap3;
uniform sampler2D					u_WaterEdgeMap;
uniform sampler2D					u_SplatControlMap;
uniform sampler2D					u_SplatMap1;
uniform sampler2D					u_SplatMap2;
uniform sampler2D					u_SplatMap3;
uniform sampler2D					u_RoadsControlMap;
uniform sampler2D					u_RoadMap;
uniform sampler2D					u_DetailMap;
uniform sampler2D					u_ScreenImageMap;
uniform sampler2D					u_ScreenDepthMap;
uniform sampler2DShadow				u_ShadowMap;
uniform sampler2DShadow				u_ShadowMap2;
uniform sampler2DShadow				u_ShadowMap3;
uniform sampler2DShadow				u_ShadowMap4;
uniform sampler2DShadow				u_ShadowMap5;
uniform sampler2D					u_MoonMaps[4];
};
#else //!defined(USE_BINDLESS_TEXTURES)
uniform sampler2D			u_ScreenDepthMap;

uniform sampler2DShadow		u_ShadowMap;
uniform sampler2DShadow		u_ShadowMap2;
uniform sampler2DShadow		u_ShadowMap3;
uniform sampler2DShadow		u_ShadowMap4;
uniform sampler2DShadow		u_ShadowMap5;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform mat4				u_ShadowMvp;
uniform mat4				u_ShadowMvp2;
uniform mat4				u_ShadowMvp3;
uniform mat4				u_ShadowMvp4;
uniform mat4				u_ShadowMvp5;

uniform vec4				u_Settings0;			// SHADOW_MAP_SIZE, SHADOWS_FULL_SOLID, FAST_SHADOWS, SHADOW_Z_ERROR_OFFSET_NEAR
uniform vec4				u_Settings1;			// SHADOW_Z_ERROR_OFFSET_MID, SHADOW_Z_ERROR_OFFSET_MID2, SHADOW_Z_ERROR_OFFSET_MID3, SHADOW_Z_ERROR_OFFSET_FAR
uniform vec4				u_Settings2;			// SHADOW_ZFAR[0], SHADOW_ZFAR[1], SHADOW_ZFAR[2], SHADOW_ZFAR[3]

uniform vec3				u_ViewOrigin;
uniform vec4				u_ViewInfo;				// zfar / znear, zfar, depthBits, znear

varying vec2				var_DepthTex;
varying vec3				var_ViewDir;

#define						SHADOW_MAP_SIZE				u_Settings0.r
#define						SHADOWS_FULL_SOLID			u_Settings0.g
#define						FAST_SHADOWS				u_Settings0.b

#define						SHADOW_Z_ERROR_OFFSET_NEAR	u_Settings0.a
#define						SHADOW_Z_ERROR_OFFSET_MID	u_Settings1.r
#define						SHADOW_Z_ERROR_OFFSET_MID2	u_Settings1.g
#define						SHADOW_Z_ERROR_OFFSET_MID3	u_Settings1.b
#define						SHADOW_Z_ERROR_OFFSET_FAR	u_Settings1.a

#define						MAX_SHADOW_VALUE_NORMAL		0.1
#define						MAX_SHADOW_VALUE_SOLID		0.9999999

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
		shadowError = SHADOW_Z_ERROR_OFFSET_MID2;
	}
	else if (cascade == 3)
	{
		shadowError = SHADOW_Z_ERROR_OFFSET_MID3;
	}
	else if (cascade == 4)
	{
		shadowError = SHADOW_Z_ERROR_OFFSET_FAR;
	}

#if 1
	if (SHADOWS_FULL_SOLID <= 0.0)
	{
		return textureProj(shadowmap, vec4(loc.xy + offset * scale * loc.w, (loc.z + shadowError), loc.w)) > MAX_SHADOW_VALUE_NORMAL ? 1.0 : 0.0;
	}
	else 
	{
		return textureProj(shadowmap, vec4(loc.xy + offset * scale * loc.w, (loc.z + shadowError), loc.w));
	}
#else
	float bc = 0.0;
	float bs = 0.0;
	float br = float(cascade);

	for (float x = -br; x <= br; x += 1.0)
	{
		for (float y = -br; y <= br; y += 1.0)
		{
			vec4 pos = vec4(loc.xy + offset * scale * loc.w + vec2(x,y) * scale * 0.05, (loc.z + shadowError), loc.w);
			float s = textureProj(shadowmap, pos);
			if (SHADOWS_FULL_SOLID <= 0.0)
			{
				bs += s > MAX_SHADOW_VALUE_NORMAL ? 1.0 : 0.0;
			}
			else
			{
				bs += s;
			}

			bc += 1.0;
		}
	}

	return bs / bc;
#endif
}

//#define _VSM_

#ifdef _VSM_
float linstep(float low, float high, float v)
{
	return clamp((v-low)/(high-low), 0.0, 1.0);
}

float VSM(sampler2DShadow shadowmap, vec4 uv, float compare)
{
	//vec2 moments = textureProj(shadowmap, uv).xy;
	
	/* this should be in the render phase - just testing */
	float dx = dFdx(compare);
	float dy = dFdy(compare);

	float depth = textureProj(shadowmap, uv).x;
	vec2 moments = vec2(depth, pow(depth, 2.0) + 0.25*(dx*dx + dy*dy));
	/* */

	float p = smoothstep(compare-0.0001/*0.02*/, compare, moments.x);
	float variance = max(moments.y - moments.x*moments.x, -0.001);
	float d = compare - moments.x;
	float p_max = linstep(0.2, 1.0, variance / (variance + d*d));
	return clamp(max(p, p_max), 0.0, 1.0);
}
#endif //_VSM_

float PCF(const sampler2DShadow shadowmap, const vec4 st, const float dist, float scale, float depth, int cascade)
{
#ifdef _VSM_
	return VSM(shadowmap, st, depth);
#else //!_VSM_

#if 0
	float mult;
	vec4 sCoord = vec4(st);

	vec2 offset = vec2(greaterThan(fract(st.xy * 0.5), vec2(0.25)));  // mod
	//vec2 offset = mod(sCoord.xy, 0.5);
	offset.y += offset.x;  // y ^= x in floating point
	if (offset.y > 1.1) offset.y = 0.0;
	//if (offset.y > 1.0) offset.y = 0.0;

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
#else
	float shadow = offset_lookup(shadowmap, st + vec4(0.0, 0.0, -0.008, /*depth*0.25*/0.0015), vec2(0.0), scale, depth, cascade);

	if (SHADOWS_FULL_SOLID <= 0.0)
	{
		return shadow;
	}
	else
	{
		return (shadow <= MAX_SHADOW_VALUE_SOLID) ? 0.0 : 1.0;
	}
#endif

#endif //_VSM_
}

//////////////////////////////////////////////////////////////////////////
void main()
{
	float result = 1.0;
	float depth = texture(u_ScreenDepthMap, var_DepthTex).x;

	vec4 biasPos = vec4(u_ViewOrigin + var_ViewDir * (depth - 0.5 / u_ViewInfo.x), 1.0);
	vec4 shadowpos = u_ShadowMvp * biasPos;

	float dist = distance(biasPos.xyz, u_ViewOrigin);

	if (all(lessThan(abs(shadowpos.xyz), vec3(abs(shadowpos.w)))))
	//if (dist < u_Settings2.r)
	{
		shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
		result = PCF(u_ShadowMap, shadowpos, shadowpos.z, 1.0 / SHADOW_MAP_SIZE, depth, 0);

#if 1
		float switchDist = u_Settings2.r * 0.5;

		if (dist > switchDist)
		{// Mix with next cascade to hide the switches...
			shadowpos = u_ShadowMvp2 * biasPos;
			shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
			float result2 = PCF(u_ShadowMap2, shadowpos, shadowpos.z, 1.0 / SHADOW_MAP_SIZE, depth, 1);

			result = mix(result, result2, (dist - switchDist) / switchDist);
		}
#endif

		gl_FragColor = vec4(result, depth, 0.0, 1.0);
		return;
	}

	shadowpos = u_ShadowMvp2 * biasPos;

	if (all(lessThan(abs(shadowpos.xyz), vec3(abs(shadowpos.w)))))
	//if (dist < u_Settings2.g)
	{
		shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
		result = PCF(u_ShadowMap2, shadowpos, shadowpos.z, 1.0 / SHADOW_MAP_SIZE, depth, 1);

#if 1
		float switchDist = u_Settings2.g * 0.5;

		if (dist > switchDist)
		{// Mix with next cascade to hide the switches...
			shadowpos = u_ShadowMvp3 * biasPos;
			shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
			float result2 = PCF(u_ShadowMap3, shadowpos, shadowpos.z, 1.0 / SHADOW_MAP_SIZE, depth, 2);

			result = mix(result, result2, (dist - switchDist) / switchDist);
		}
#endif

		gl_FragColor = vec4(result, depth, 0.0, 1.0);
		return;
	}

	shadowpos = u_ShadowMvp3 * biasPos;

	if (all(lessThan(abs(shadowpos.xyz), vec3(abs(shadowpos.w)))))
	//if (dist < u_Settings2.b)
	{
		shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
		result = PCF(u_ShadowMap3, shadowpos, shadowpos.z, 1.0 / SHADOW_MAP_SIZE, depth, 2);

#if 1
		float switchDist = u_Settings2.b * 0.5;

		if (dist > switchDist)
		{// Mix with next cascade to hide the switches...
			shadowpos = u_ShadowMvp4 * biasPos;
			shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
			float result2 = PCF(u_ShadowMap4, shadowpos, shadowpos.z, 1.0 / SHADOW_MAP_SIZE, depth, 3);

			result = mix(result, result2, (dist - switchDist) / switchDist);
		}
#endif

		gl_FragColor = vec4(result, depth, 0.0, 1.0);
		return;
	}

	shadowpos = u_ShadowMvp4 * biasPos;

	if (!(FAST_SHADOWS > 1.0) && all(lessThan(abs(shadowpos.xyz), vec3(abs(shadowpos.w)))))
	//if (dist < u_Settings2.b)
	{
		shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
		result = PCF(u_ShadowMap4, shadowpos, shadowpos.z, 1.0 / SHADOW_MAP_SIZE, depth, 3);

#if 1
		float switchDist = u_Settings2.a * 0.5;

		if (dist > switchDist)
		{// Mix with next cascade to hide the switches...
			shadowpos = u_ShadowMvp5 * biasPos;
			shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
			float result2 = PCF(u_ShadowMap5, shadowpos, shadowpos.z, 1.0 / SHADOW_MAP_SIZE, depth, 4);

			float result3 = mix(result, result2, (dist - switchDist) / switchDist);
			result = min(result, result3);
		}
#endif

		gl_FragColor = vec4(result, depth, 0.0, 1.0);
		return;
	}

	if (!(FAST_SHADOWS > 0.0))
	{
		shadowpos = u_ShadowMvp5 * biasPos;

		shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
		result = PCF(u_ShadowMap5, shadowpos, shadowpos.z, 1.0 / SHADOW_MAP_SIZE, depth, 4);
		gl_FragColor = vec4(result, depth, 0.0, 1.0);
	}
	
	gl_FragColor = vec4(result, depth, 0.0, 1.0);
}
