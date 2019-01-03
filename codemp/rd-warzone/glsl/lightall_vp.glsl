//#define __EXPERIMETNAL_CHARACTER_EDITOR__

attribute vec2 attr_TexCoord0;

attribute vec2 attr_TexCoord1;
attribute vec4 attr_Color;

attribute vec3 attr_Position;
attribute vec3 attr_Normal;

attribute vec3 attr_Position2;
attribute vec3 attr_Normal2;
attribute vec4 attr_BoneIndexes;
attribute vec4 attr_BoneWeights;

uniform sampler2D					u_RoadsControlMap;
uniform sampler2D					u_HeightMap;

uniform vec4						u_Settings0; // useTC, useDeform, useRGBA, isTextureClamped
uniform vec4						u_Settings1; // useVertexAnim, useSkeletalAnim, useFog, is2D
uniform vec4						u_Settings2; // LIGHTDEF_USE_LIGHTMAP, LIGHTDEF_USE_GLOW_BUFFER, LIGHTDEF_USE_CUBEMAP, LIGHTDEF_USE_TRIPLANAR
uniform vec4						u_Settings3; // LIGHTDEF_USE_REGIONS, LIGHTDEF_IS_DETAIL

#define USE_TC						u_Settings0.r
#define USE_DEFORM					u_Settings0.g
#define USE_RGBA					u_Settings0.b
#define USE_TEXTURECLAMP			u_Settings0.a

#define USE_VERTEX_ANIM				u_Settings1.r
#define USE_SKELETAL_ANIM			u_Settings1.g
#define USE_FOG						u_Settings1.b
#define USE_IS2D					u_Settings1.a

#define USE_LIGHTMAP				u_Settings2.r
#define USE_GLOW_BUFFER				u_Settings2.g
#define USE_CUBEMAP					u_Settings2.b
#define USE_TRIPLANAR				u_Settings2.a

#define USE_REGIONS					u_Settings3.r
#define USE_ISDETAIL				u_Settings3.g

uniform vec4						u_Local1; // MAP_SIZE, sway, overlaySway, materialType
uniform vec4						u_Local2; // hasSteepMap, hasWaterEdgeMap, haveNormalMap, SHADER_WATER_LEVEL
uniform vec4						u_Local3; // hasSplatMap1, hasSplatMap2, hasSplatMap3, hasSplatMap4
uniform vec4						u_Local4; // stageNum, glowStrength, r_showsplat, 0.0
uniform vec4						u_Local9; // testvalue0, 1, 2, 3

uniform vec4						u_Local12; // TERRAIN_TESS_OFFSET, GRASS_DISTANCE_FROM_ROADS, 0.0, 0.0

#define SHADER_MAP_SIZE				u_Local1.r
#define SHADER_SWAY					u_Local1.g
#define SHADER_OVERLAY_SWAY			u_Local1.b
#define SHADER_MATERIAL_TYPE		u_Local1.a

#define SHADER_HAS_STEEPMAP			u_Local2.r
#define SHADER_HAS_WATEREDGEMAP		u_Local2.g
#define SHADER_HAS_NORMALMAP		u_Local2.b
#define SHADER_WATER_LEVEL			u_Local2.a

#define SHADER_HAS_SPLATMAP1		u_Local3.r
#define SHADER_HAS_SPLATMAP2		u_Local3.g
#define SHADER_HAS_SPLATMAP3		u_Local3.b
#define SHADER_HAS_SPLATMAP4		u_Local3.a

#define SHADER_STAGE_NUM			u_Local4.r
#define SHADER_GLOW_STRENGTH		u_Local4.g
#define SHADER_SHOW_SPLAT			u_Local4.b

#define TERRAIN_TESS_OFFSET			u_Local12.r
#define GRASS_DISTANCE_FROM_ROADS	u_Local12.g

uniform vec4						u_Mins;
uniform vec4						u_Maxs;

uniform float						u_Time;

uniform vec2						u_textureScale;

uniform vec3						u_ViewOrigin;

uniform int							u_TCGen0;
uniform vec3						u_TCGen0Vector0;
uniform vec3						u_TCGen0Vector1;

uniform vec3						u_LocalViewOrigin;

uniform vec4						u_DiffuseTexMatrix;
uniform vec4						u_DiffuseTexOffTurb;

uniform int							u_ColorGen;
uniform int							u_AlphaGen;
uniform vec3						u_AmbientLight;
uniform vec3						u_DirectedLight;
uniform vec3						u_ModelLightDir;
uniform float						u_PortalRange;

uniform int							u_DeformGen;
uniform float						u_DeformParams[7];

uniform vec4						u_FogDistance;
uniform vec4						u_FogDepth;
uniform float						u_FogEyeT;
uniform vec4						u_FogColorMask;

uniform mat4						u_ModelViewProjectionMatrix;
//uniform mat4						u_ViewProjectionMatrix;
uniform mat4						u_ModelMatrix;
uniform mat4						u_NormalMatrix;

uniform vec4						u_BaseColor;
uniform vec4						u_VertColor;

uniform float						u_VertexLerp;
uniform mat4						u_BoneMatrices[MAX_GLM_BONEREFS];
#ifdef __EXPERIMETNAL_CHARACTER_EDITOR__
uniform float						u_BoneScales[20];
#endif //__EXPERIMETNAL_CHARACTER_EDITOR__

uniform vec4						u_PrimaryLightOrigin;
uniform vec3						u_PrimaryLightColor;
uniform float						u_PrimaryLightRadius;

#if defined(USE_TESSELLATION) || defined(USE_ICR_CULLING)
out vec3 Normal_CS_in;
out vec2 TexCoord_CS_in;
out vec4 WorldPos_CS_in;
out vec3 ViewDir_CS_in;
out vec4 Color_CS_in;
out vec4 PrimaryLightDir_CS_in;
out vec2 TexCoord2_CS_in;
out vec3 Blending_CS_in;
out float Slope_CS_in;
#endif

varying vec2	var_TexCoords;
varying vec2	var_TexCoords2;
varying vec4	var_Color;
varying vec3	var_Normal;
varying vec3	var_ViewDir;
varying vec4	var_PrimaryLightDir;
varying vec3	var_vertPos;
varying vec3	var_Blending;
varying float	var_Slope;

#define HASHSCALE1 .1031

float random(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * HASHSCALE1);
	p3 += dot(p3, p3.yzx + 19.19);
	return fract((p3.x + p3.y) * p3.z);
}

// 2D Noise based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
float noise(in vec2 st) {
	vec2 i = floor(st);
	vec2 f = fract(st);

	// Four corners in 2D of a tile
	float a = random(i);
	float b = random(i + vec2(1.0, 0.0));
	float c = random(i + vec2(0.0, 1.0));
	float d = random(i + vec2(1.0, 1.0));

	// Smooth Interpolation

	// Cubic Hermine Curve.  Same as SmoothStep()
	vec2 u = f*f*(3.0 - 2.0*f);
	// u = smoothstep(0.,1.,f);

	// Mix 4 coorners percentages
	return mix(a, b, u.x) +
		(c - a)* u.y * (1.0 - u.x) +
		(d - b) * u.x * u.y;
}

float GetRoadFactor(vec2 pixel)
{
	float roadScale = 1.0;

	//if (SHADER_HAS_SPLATMAP4 > 0.0)
	{// Also grab the roads map, if we have one...
		float road = texture(u_RoadsControlMap, pixel).r;

		if (road > GRASS_DISTANCE_FROM_ROADS)
		{
			roadScale = 0.0;
		}
		else if (road > 0.0)
		{
			roadScale = 1.0 - clamp(road / GRASS_DISTANCE_FROM_ROADS, 0.0, 1.0);
		}
		else
		{
			roadScale = 1.0;
		}
	}
	//else
	//{
	//	roadScale = 1.0;
	//}

	return 1.0 - clamp(roadScale * 0.6 + 0.4, 0.0, 1.0);
}

float GetHeightmap(vec2 pixel)
{
	return texture(u_HeightMap, pixel).r;
}

vec2 GetMapTC(vec3 pos)
{
	vec2 mapSize = u_Maxs.xy - u_Mins.xy;
	return (pos.xy - u_Mins.xy) / mapSize;
}

float LDHeightForPosition(vec3 pos)
{
	return noise(vec2(pos.xy * 0.00875));
}

float OffsetForPosition(vec3 pos)
{
	vec2 pixel = GetMapTC(pos);
	float roadScale = GetRoadFactor(pixel);
	float SmoothRand = LDHeightForPosition(pos);
	float offsetScale = SmoothRand * clamp(1.0 - roadScale, 0.75, 1.0);

	float offset = max(offsetScale, roadScale) - 0.5;
	return offset * TERRAIN_TESS_OFFSET;//uTessAlpha;
}

vec3 DeformPosition(const vec3 pos, const vec3 normal, const vec2 st)
{
	float base =      u_DeformParams[0];
	float amplitude = u_DeformParams[1];
	float phase =     u_DeformParams[2];
	float frequency = u_DeformParams[3];
	float spread =    u_DeformParams[4];

	if (u_DeformGen == DGEN_PROJECTION_SHADOW)
	{
		vec3 ground = vec3(
			u_DeformParams[0],
			u_DeformParams[1],
			u_DeformParams[2]);
		float groundDist = u_DeformParams[3];
		vec3 lightDir = vec3(
			u_DeformParams[4],
			u_DeformParams[5],
			u_DeformParams[6]);

		float d = dot(lightDir, ground);

		lightDir = lightDir * max(0.5 - d, 0.0) + ground;
		d = 1.0 / dot(lightDir, ground);

		vec3 lightPos = lightDir * d;

		return pos - lightPos * dot(pos, ground) + groundDist;
	}
	else if (u_DeformGen == DGEN_BULGE)
	{
		phase *= st.x;
	}
	else // if (u_DeformGen <= DGEN_WAVE_INVERSE_SAWTOOTH)
	{
		phase += dot(pos.xyz, vec3(spread));
	}

	float value = phase + (u_Time * frequency);
	float func;

	if (u_DeformGen == DGEN_WAVE_SIN)
	{
		func = sin(value * 2.0 * M_PI);
	}
	else if (u_DeformGen == DGEN_WAVE_SQUARE)
	{
		func = sign(fract(0.5 - value));
	}
	else if (u_DeformGen == DGEN_WAVE_TRIANGLE)
	{
		func = abs(fract(value + 0.75) - 0.5) * 4.0 - 1.0;
	}
	else if (u_DeformGen == DGEN_WAVE_SAWTOOTH)
	{
		func = fract(value);
	}
	else if (u_DeformGen == DGEN_WAVE_INVERSE_SAWTOOTH)
	{
		func = (1.0 - fract(value));
	}
	else // if (u_DeformGen == DGEN_BULGE)
	{
		func = sin(value);
	}

	return pos + normal * (base + func * amplitude);
}

vec2 GenTexCoords(int TCGen, vec3 position, vec3 normal, vec3 TCGenVector0, vec3 TCGenVector1)
{
	vec2 tex = attr_TexCoord0;

	switch (TCGen)
	{
		case TCGEN_LIGHTMAP:
		case TCGEN_LIGHTMAP1:
		case TCGEN_LIGHTMAP2:
		case TCGEN_LIGHTMAP3:
			tex = attr_TexCoord1;
		break;

		case TCGEN_ENVIRONMENT_MAPPED:
		{
			vec3 viewer = normalize(u_LocalViewOrigin - position);
			vec2 ref = reflect(viewer, normal).yz;
			tex.s = ref.x * -0.5 + 0.5;
			tex.t = ref.y *  0.5 + 0.5;
		}
		break;

		case TCGEN_VECTOR:
		{
			tex = vec2(dot(position, TCGenVector0), dot(position, TCGenVector1));
		}
		break;
	}

	return tex;
}

vec2 ModTexCoords(vec2 st, vec3 position, vec4 texMatrix, vec4 offTurb)
{
	float amplitude = offTurb.z;
	float phase = offTurb.w * 2.0 * M_PI;
	vec2 st2;
	st2.x = st.x * texMatrix.x + (st.y * texMatrix.z + offTurb.x);
	st2.y = st.x * texMatrix.y + (st.y * texMatrix.w + offTurb.y);

	vec2 offsetPos = vec2(position.x + position.z, position.y);

	vec2 texOffset = sin(offsetPos * (2.0 * M_PI / 1024.0) + vec2(phase));

	return st2 + texOffset * amplitude;
}


vec4 CalcColor(vec3 position, vec3 normal)
{
	vec4 color = u_VertColor * attr_Color + u_BaseColor;
	
	if (USE_RGBA > 0.0)
	{
		if (u_ColorGen == CGEN_LIGHTING_DIFFUSE)
		{
			float incoming = clamp(dot(normal, u_ModelLightDir), 0.0, 1.0);
			color.rgb = clamp(u_DirectedLight * incoming + u_AmbientLight, 0.0, 1.0);
		}

		if (u_ColorGen == CGEN_LIGHTING_WARZONE)
		{
			vec3 light = normalize(u_PrimaryLightOrigin.xyz - (position * u_PrimaryLightOrigin.w));
			float diffuse = clamp(pow(dot(normal, light), 16.0), 0.25, 1.0);
			vec3 reflected = -reflect(light, normal);
			vec3 viewer = normalize(u_LocalViewOrigin - position);
			float spec = clamp(pow(dot(reflected, viewer), 64.0), 0.25, 1.0);
			vec3 lColor = mix(u_PrimaryLightColor, vec3(1.0), 0.8);
			color.rgb = clamp(lColor * (diffuse + spec + 0.5), 0.0, 1.0);
		}
	
		if (u_AlphaGen == AGEN_LIGHTING_SPECULAR)
		{
			vec3 viewer = u_LocalViewOrigin - position;
			//vec3 lightDir = normalize(vec3(-960.0, 1980.0, 96.0) - position);
			vec3 lightDir = normalize(u_PrimaryLightOrigin.xyz);// normalize(u_PrimaryLightOrigin.xyz - position);
			vec3 reflected = -reflect(lightDir, normal);
		
			color.a = clamp(dot(reflected, normalize(viewer)), 0.0, 1.0);
			color.a *= color.a;
			color.a *= color.a;
		}
		else if (u_AlphaGen == AGEN_PORTAL)
		{
			//vec3 viewer = u_LocalViewOrigin - position;
			vec3 viewer = u_ViewOrigin - position;
			color.a = clamp(length(viewer) / u_PortalRange, 0.0, 1.0);
		}
	}
	
	return color;
}

#if 0
float CalcFog(vec3 position)
{
	float s = dot(vec4(position, 1.0), u_FogDistance) * 8.0;
	float t = dot(vec4(position, 1.0), u_FogDepth);

	float eyeOutside = float(u_FogEyeT < 0.0);
	float fogged = float(t < eyeOutside);

	t += 1e-6;
	t *= fogged / (t - u_FogEyeT * eyeOutside);

	return s * t;
}
#endif

void main()
{
	vec3 position;
	vec3 normal;

	/*
	vec3 heightMap = vec3(0.0);

	if (USE_IS2D <= 0.0)
	{
		vec2 coord;
		coord = vec2(attr_Position.xy / u_Local9.r);
		heightMap = (texture(u_HeightMap, coord).xyz * 2.0 - 1.0) * u_Local9.g;
	}
	*/

	if (USE_VERTEX_ANIM == 1.0)
	{
		//position  = mix(attr_Position,    attr_Position2,    u_VertexLerp);
		//normal    = mix(attr_Normal,      attr_Normal2,      u_VertexLerp) * 2.0 - 1.0;
		position  = attr_Position;
		normal    = attr_Normal * 2.0 - 1.0;
	}
	else if (USE_SKELETAL_ANIM == 1.0)
	{
		vec4 position4 = vec4(0.0);
		vec4 normal4 = vec4(0.0);
		vec4 originalPosition = vec4(attr_Position, 1.0);
		vec4 originalNormal = vec4(attr_Normal * 2.0 - 1.0, 0.0);

		for (int i = 0; i < 4; i++)
		{
			int boneIndex = int(attr_BoneIndexes[i]);

			if (boneIndex >= MAX_GLM_BONEREFS)
			{// Skip...

			}
			else
			{
#ifdef __EXPERIMETNAL_CHARACTER_EDITOR__
				vec4 boneScale = vec4(u_BoneScales[boneIndex], u_BoneScales[boneIndex], u_BoneScales[boneIndex], 1.0);
				position4 += (u_BoneMatrices[boneIndex] * originalPosition) * attr_BoneWeights[i] * boneScale; // Could do X,Y,Z model scaling here...
				normal4 += (u_BoneMatrices[boneIndex] * originalNormal) * attr_BoneWeights[i] * boneScale; // Could do X,Y,Z model scaling here...
#else //!__EXPERIMETNAL_CHARACTER_EDITOR__
				position4 += (u_BoneMatrices[boneIndex] * originalPosition) * attr_BoneWeights[i]; // Could do X,Y,Z model scaling here...
				normal4 += (u_BoneMatrices[boneIndex] * originalNormal) * attr_BoneWeights[i]; // Could do X,Y,Z model scaling here...
#endif //__EXPERIMETNAL_CHARACTER_EDITOR__
			}
		}

		position = position4.xyz;
		normal = normalize(normal4.xyz);
	}
	else
	{
		position  = attr_Position;
		normal    = attr_Normal * 2.0 - 1.0;
	}

	//position.xyz += heightMap;

	/*if (TERRAIN_TESS_OFFSET != 0.0 && (USE_SKELETAL_ANIM == 1.0 || USE_VERTEX_ANIM == 1.0))
	{// When on terrain that is tessellated, offset the z to match the terrain tess height...
		position.z += OffsetForPosition(position);
	}*/

	vec2 texCoords = attr_TexCoord0.st;

#if !defined(USE_TESSELLATION) && !defined(USE_ICR_CULLING)
	if (USE_DEFORM == 1.0)
	{
		position = DeformPosition(position, normal, attr_TexCoord0.st);
	}

	gl_Position = u_ModelViewProjectionMatrix * vec4(position, 1.0);

	vec3 preMMPos = position.xyz;

	position = (u_ModelMatrix * vec4(position, 1.0)).xyz;
	normal = (u_ModelMatrix * vec4(normal, 0.0)).xyz;
#endif

	if (USE_TC == 1.0)
	{
		texCoords = GenTexCoords(u_TCGen0, position, normal, u_TCGen0Vector0, u_TCGen0Vector1);
		texCoords = ModTexCoords(texCoords, position, u_DiffuseTexMatrix, u_DiffuseTexOffTurb);
	}


	if (!(u_textureScale.x <= 0.0 && u_textureScale.y <= 0.0) && !(u_textureScale.x == 1.0 && u_textureScale.y == 1.0))
	{
		texCoords *= u_textureScale;
	}

	var_Color = CalcColor(position, normal);

#if 0
	if (USE_FOG == 1.0)
	{
		var_Color *= vec4(1.0) - u_FogColorMask * sqrt(clamp(CalcFog(position), 0.0, 1.0));
	}
#endif

	var_TexCoords = texCoords;


	if (USE_LIGHTMAP > 0.0)
	{
		var_TexCoords2 = attr_TexCoord1.st;
	}
	else
	{
		var_TexCoords2 = vec2(0.0);
	}

	var_PrimaryLightDir.xyz = u_PrimaryLightOrigin.xyz - (position * u_PrimaryLightOrigin.w);
	var_ViewDir = u_ViewOrigin - position;
	var_Normal = normal.xyz;
	var_Slope = 0.0;

#if defined(USE_TESSELLATION) || defined(USE_ICR_CULLING)
	WorldPos_CS_in = vec4(position.xyz, 1.0);
	TexCoord_CS_in = var_TexCoords.xy;
	Normal_CS_in = var_Normal.xyz;
	ViewDir_CS_in = var_ViewDir;
	Color_CS_in = var_Color;
	PrimaryLightDir_CS_in = var_PrimaryLightDir;
	TexCoord2_CS_in = var_TexCoords2;
	Blending_CS_in = var_Blending;
	Slope_CS_in = var_Slope;
	gl_Position = vec4(position.xyz, 1.0);
#endif

	var_vertPos = position.xyz;

	if (var_Color.a >= 0.99) var_Color.a = 1.0; // Allow for rounding errors... Don't let them stop pixel culling...
}