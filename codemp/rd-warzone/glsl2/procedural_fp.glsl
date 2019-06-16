uniform sampler2D							u_DiffuseMap;		// Screen image
uniform sampler2D							u_NormalMap;		// Flat normals
uniform sampler2D							u_PositionMap;		// positionMap
uniform sampler2D							u_WaterPositionMap;	// water positions

uniform mat4								u_ModelViewProjectionMatrix;

uniform vec2								u_Dimensions;

uniform vec4								u_Local1; // MAP_INFO_PLAYABLE_HEIGHT, WATER_ENABLED, PROCEDURAL_MOSS_ENABLED, PROCEDURAL_SNOW_ENABLED
uniform vec4								u_Local2; // PROCEDURAL_SNOW_LUMINOSITY_CURVE, PROCEDURAL_SNOW_BRIGHTNESS, PROCEDURAL_SNOW_HEIGHT_CURVE, PROCEDURAL_SNOW_LOWEST_ELEVATION
uniform vec4								u_Local3; // r_testShaderValue1, r_testShaderValue2, r_testShaderValue3, r_testShaderValue4

uniform vec3								u_ViewOrigin;

uniform float								u_Time;

varying vec2								var_TexCoords;


#define MAP_INFO_PLAYABLE_HEIGHT			u_Local1.r
#define WATER_ENABLED						u_Local1.g
#define PROCEDURAL_MOSS_ENABLED				u_Local1.b
#define PROCEDURAL_SNOW_ENABLED				u_Local1.a

#define PROCEDURAL_SNOW_LUMINOSITY_CURVE	u_Local2.r
#define PROCEDURAL_SNOW_BRIGHTNESS			u_Local2.g
#define PROCEDURAL_SNOW_HEIGHT_CURVE		u_Local2.b
#define PROCEDURAL_SNOW_LOWEST_ELEVATION	u_Local2.a

//#define __ENCODE_NORMALS_RECONSTRUCT_Z__
#define __ENCODE_NORMALS_STEREOGRAPHIC_PROJECTION__
//#define __ENCODE_NORMALS_CRY_ENGINE__
//#define __ENCODE_NORMALS_EQUAL_AREA_PROJECTION__

#ifdef __ENCODE_NORMALS_STEREOGRAPHIC_PROJECTION__
vec2 EncodeNormal(vec3 n)
{
	float scale = 1.7777;
	vec2 enc = n.xy / (n.z + 1.0);
	enc /= scale;
	enc = enc * 0.5 + 0.5;
	return enc;
}
vec3 DecodeNormal(vec2 enc)
{
	vec3 enc2 = vec3(enc.xy, 0.0);
	float scale = 1.7777;
	vec3 nn =
		enc2.xyz*vec3(2.0 * scale, 2.0 * scale, 0.0) +
		vec3(-scale, -scale, 1.0);
	float g = 2.0 / dot(nn.xyz, nn.xyz);
	return vec3(g * nn.xy, g - 1.0);
}
#elif defined(__ENCODE_NORMALS_CRY_ENGINE__)
vec3 DecodeNormal(in vec2 N)
{
	vec2 encoded = N * 4.0 - 2.0;
	float f = dot(encoded, encoded);
	float g = sqrt(1.0 - f * 0.25);
	return vec3(encoded * g, 1.0 - f * 0.5);
}
vec2 EncodeNormal(in vec3 N)
{
	float f = sqrt(8.0 * N.z + 8.0);
	return N.xy / f + 0.5;
}
#elif defined(__ENCODE_NORMALS_EQUAL_AREA_PROJECTION__)
vec2 EncodeNormal(vec3 n)
{
	float f = sqrt(8.0 * n.z + 8.0);
	return n.xy / f + 0.5;
}
vec3 DecodeNormal(vec2 enc)
{
	vec2 fenc = enc * 4.0 - 2.0;
	float f = dot(fenc, fenc);
	float g = sqrt(1.0 - f / 4.0);
	vec3 n;
	n.xy = fenc*g;
	n.z = 1.0 - f / 2.0;
	return n;
}
#else //__ENCODE_NORMALS_RECONSTRUCT_Z__
vec3 DecodeNormal(in vec2 N)
{
	vec3 norm;
	norm.xy = N * 2.0 - 1.0;
	norm.z = sqrt(1.0 - dot(norm.xy, norm.xy));
	return norm;
}
vec2 EncodeNormal(vec3 n)
{
	return vec2(n.xy * 0.5 + 0.5);
}
#endif //__ENCODE_NORMALS_RECONSTRUCT_Z__


vec4 positionMapAtCoord ( vec2 coord, out bool changedToWater, out vec3 originalPosition )
{
	changedToWater = false;

	vec4 pos = textureLod(u_PositionMap, coord, 0.0);
	originalPosition = pos.xyz;

	if (WATER_ENABLED > 0.0)
	{
		bool isSky = (pos.a - 1.0 >= MATERIAL_SKY) ? true : false;

		vec4 wMap = textureLod(u_WaterPositionMap, coord, 0.0);

		if (wMap.a > 0.0 || (wMap.a > 0.0 && isSky))
		{
			if ((wMap.z > pos.z || isSky) && u_ViewOrigin.z > wMap.z)
			{
				pos.xyz = wMap.xyz;

				if (!isSky)
				{// So we know if this is a shoreline or water in skybox...
					changedToWater = true;
				}
				else
				{// Also change the material...
					pos.a = MATERIAL_WATER + 1.0;
				}
			}
		}
	}

	return pos;
}

float proceduralHash( const in float n ) {
	return fract(sin(n)*4378.5453);
}

float proceduralNoise(in vec3 o) 
{
	vec3 p = floor(o);
	vec3 fr = fract(o);
		
	float n = p.x + p.y*57.0 + p.z * 1009.0;

	float a = proceduralHash(n+  0.0);
	float b = proceduralHash(n+  1.0);
	float c = proceduralHash(n+ 57.0);
	float d = proceduralHash(n+ 58.0);
	
	float e = proceduralHash(n+  0.0 + 1009.0);
	float f = proceduralHash(n+  1.0 + 1009.0);
	float g = proceduralHash(n+ 57.0 + 1009.0);
	float h = proceduralHash(n+ 58.0 + 1009.0);
	
	
	vec3 fr2 = fr * fr;
	vec3 fr3 = fr2 * fr;
	
	vec3 t = 3.0 * fr2 - 2.0 * fr3;
	
	float u = t.x;
	float v = t.y;
	float w = t.z;

	// this last bit should be refactored to the same form as the rest :)
	float res1 = a + (b-a)*u +(c-a)*v + (a-b+d-c)*u*v;
	float res2 = e + (f-e)*u +(g-e)*v + (e-f+h-g)*u*v;
	
	float res = res1 * (1.0- w) + res2 * (w);
	
	return res;
}

const mat3 proceduralMat = mat3( 0.00,  0.80,  0.60,
                    -0.80,  0.36, -0.48,
                    -0.60, -0.48,  0.64 );

float proceduralSmoothNoise( vec3 p )
{
    float f;
    f  = 0.5000*proceduralNoise( p ); p = proceduralMat*p*2.02;
    f += 0.2500*proceduralNoise( p ); 
	
    return clamp(f * 1.3333333333333333333333333333333, 0.0, 1.0);
	//return proceduralNoise(p);
}

vec3 splatblend(vec3 color1, float a1, vec3 color2, float a2)
{
    float depth = 0.2;
	float ma = max(a1, a2) - depth;

    float b1 = max(a1 - ma, 0);
    float b2 = max(a2 - ma, 0);

    return ((color1.rgb * b1) + (color2.rgb * b2)) / (b1 + b2);
}

//
// Procedural texturing variation...
//
void AddProceduralMoss(inout vec4 outColor, in vec4 position, in bool changedToWater, in vec3 originalPosition)
{
	if (PROCEDURAL_MOSS_ENABLED > 0.0 && (position.a - 1.0 == MATERIAL_TREEBARK || position.a - 1.0 == MATERIAL_ROCK))
	{// add procedural moss...
		vec3 usePos = changedToWater ? originalPosition.xyz : position.xyz;
		float moss = clamp(proceduralNoise( usePos.xyz * 0.00125 ), 0.0, 1.0);

		if (moss > 0.25)
		{
			const vec3 colorLight = vec3(0.0, 0.65, 0.0);
			const vec3 colorDark = vec3(0.0, 0.0, 0.0);
			
			float mossClr = proceduralSmoothNoise(usePos.xyz * 0.25);
			vec3 mossColor = mix(colorDark, colorLight, mossClr*0.25);

			moss = pow((moss - 0.25) * 3.0, 0.35);
			outColor.rgb = splatblend(outColor.rgb, 1.0 - moss*0.75, mossColor, moss*0.25);
		}
	}
}

void main(void)
{
	vec4 outColor = vec4(texture(u_DiffuseMap, var_TexCoords).rgb, 1.0);
	bool changedToWater = false;
	vec3 originalPosition;
	vec4 position = positionMapAtCoord(var_TexCoords, changedToWater, originalPosition);

	if (position.a - 1.0 >= MATERIAL_SKY
		|| position.a - 1.0 == MATERIAL_SUN
		|| position.a - 1.0 == MATERIAL_GLASS
		|| position.a - 1.0 == MATERIAL_DISTORTEDGLASS
		|| position.a - 1.0 == MATERIAL_DISTORTEDPUSH
		|| position.a - 1.0 == MATERIAL_DISTORTEDPULL
		|| position.a - 1.0 == MATERIAL_CLOAK
		|| position.a - 1.0 == MATERIAL_EFX
		|| position.a - 1.0 == MATERIAL_BLASTERBOLT
		|| position.a - 1.0 == MATERIAL_FIRE
		|| position.a - 1.0 == MATERIAL_SMOKE
		|| position.a - 1.0 == MATERIAL_MAGIC_PARTICLES
		|| position.a - 1.0 == MATERIAL_MAGIC_PARTICLES_TREE
		|| position.a - 1.0 == MATERIAL_FIREFLIES
		|| position.a - 1.0 == MATERIAL_PORTAL
		|| position.a - 1.0 == MATERIAL_MENU_BACKGROUND)
	{// Skybox... Skip...
		outColor.rgb = clamp(outColor.rgb, 0.0, 1.0);
		gl_FragColor = outColor;
		return;
	}

	// Add any procedural moss...
	AddProceduralMoss(outColor, position, changedToWater, originalPosition);

	if (PROCEDURAL_SNOW_ENABLED > 0.0)
	{// add procedural snow...
		if (position.z >= PROCEDURAL_SNOW_LOWEST_ELEVATION)
		{
			float snowHeightFactor = 1.0;

			if (PROCEDURAL_SNOW_LOWEST_ELEVATION > -999999.0)
			{// Elevation is enabled...
				float elevationRange = MAP_INFO_PLAYABLE_HEIGHT - PROCEDURAL_SNOW_LOWEST_ELEVATION;
				float pixelElevation = position.z - PROCEDURAL_SNOW_LOWEST_ELEVATION;
			
				snowHeightFactor = clamp(pow(clamp(pixelElevation / elevationRange, 0.0, 1.0) * 4.0, 2.0), 0.0, 1.0);
			}

			//
			// Grab and set up our normal value, from lightall buffers, or fallback to offseting the flat normal buffer by pixel luminances, etc...
			//

			vec4 norm = textureLod(u_NormalMap, var_TexCoords, 0.0);
			norm.xyz = DecodeNormal(norm.xy);

			float b = clamp(length(outColor.rgb/3.0), 0.0, 1.0);
			b = clamp(pow(b, 64.0), 0.0, 1.0);
			vec3 of = clamp(pow(vec3(b*65536.0), -norm.xyz), 0.0, 1.0) * 0.25;
			vec3 bump = normalize(norm.xyz+of);
			
			float snow = clamp(dot(normalize(bump.xyz), vec3(0.0, 0.0, 1.0)), 0.0, 1.0);

			if (position.a - 1.0 == MATERIAL_GREENLEAVES)
				snow = pow(snow * 0.25 + 0.75, 1.333);
			else
				snow = pow(snow, 0.4);

			snow *= snowHeightFactor;
		
			if (snow > 0.0)
			{
				vec3 snowColor = vec3(PROCEDURAL_SNOW_BRIGHTNESS);
				float snowColorFactor = clamp(pow(max(outColor.r, max(outColor.g, outColor.b)), PROCEDURAL_SNOW_LUMINOSITY_CURVE), 0.0, 1.0);
				float snowMix = clamp(mix(snow*snowColorFactor, 1.0, snowHeightFactor * PROCEDURAL_SNOW_HEIGHT_CURVE), 0.0, 1.0);
				outColor.rgb = splatblend(outColor.rgb, 1.0 - snowMix, snowColor, snowMix);
			}
		}
	}

	outColor.rgb = clamp(outColor.rgb, 0.0, 1.0);
	gl_FragColor = outColor;
}

