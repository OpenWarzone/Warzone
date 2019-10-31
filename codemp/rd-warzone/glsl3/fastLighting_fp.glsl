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
uniform samplerCube					u_CubeMap;
uniform samplerCube					u_SkyCubeMap;
uniform samplerCube					u_SkyCubeMapNight;
uniform samplerCube					u_EmissiveCubeMap;
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
uniform sampler2D					u_ShadowMap;
uniform sampler2D					u_ShadowMap2;
uniform sampler2D					u_ShadowMap3;
uniform sampler2D					u_ShadowMap4;
uniform sampler2D					u_ShadowMap5;
uniform sampler3D					u_VolumeMap;
uniform sampler2D					u_MoonMaps[4];
};
#else //!defined(USE_BINDLESS_TEXTURES)
uniform sampler2D							u_DiffuseMap;		// Screen image
uniform sampler2D							u_NormalMap;		// Flat normals
uniform sampler2D							u_PositionMap;		// positionMap
uniform sampler2D							u_RoadsControlMap;	// Screen Pshadows Map
#ifdef USE_SHADOWMAP
uniform sampler2D							u_ShadowMap;		// Screen Shadow Map
#endif //USE_SHADOWMAP
#endif //defined(USE_BINDLESS_TEXTURES)

uniform mat4								u_ModelViewProjectionMatrix;

uniform vec2								u_Dimensions;

uniform vec4								u_Local1;	// SUN_PHONG_SCALE,				MAP_USE_PALETTE_ON_SKY,			AO_METHOD,						GAMMA_CORRECTION
uniform vec4								u_Local2;	// MAP_EMISSIVE_COLOR_SCALE,	SHADOWS_ENABLED,				SHADOW_MINBRIGHT,				SHADOW_MAXBRIGHT
uniform vec4								u_Local3;	// NIGHT_SCALE,					MAP_HDR_MIN,					MAP_HDR_MAX,					0.0
uniform vec4								u_Local4;	// WETNESS,						CONTRAST,					SATURATION,						BRIGHTNESS
uniform vec4								u_Local5;	// AO_MINBRIGHT,				AO_MULTBRIGHT,					VIBRANCY,						TRUEHDR_ENABLED
uniform vec4								u_Local12;	// r_testShaderValue1,			r_testShaderValue2,				r_testShaderValue3,				r_testShaderValue4


uniform vec3								u_ViewOrigin;
uniform vec4								u_PrimaryLightOrigin;
uniform vec3								u_PrimaryLightColor;

uniform float								u_Time;

uniform float								u_MaterialSpeculars[MATERIAL_LAST];
uniform float								u_MaterialReflectiveness[MATERIAL_LAST];

uniform int									u_lightCount;
uniform vec3								u_lightPositions2[MAX_DEFERRED_LIGHTS];
uniform float								u_lightDistances[MAX_DEFERRED_LIGHTS];
uniform vec3								u_lightColors[MAX_DEFERRED_LIGHTS];
uniform float								u_lightMaxDistance;

uniform vec4								u_Mins; // mins, mins, mins, WATER_ENABLED
uniform vec4								u_Maxs;

varying vec2								var_TexCoords;


#define SUN_PHONG_SCALE						u_Local1.r
#define MAP_USE_PALETTE_ON_SKY				u_Local1.g
#define AO_TYPE								u_Local1.b
#define GAMMA_CORRECTION					u_Local1.a

#define MAP_EMISSIVE_COLOR_SCALE			u_Local2.r
#define SHADOWS_ENABLED						u_Local2.g
#define SHADOW_MINBRIGHT					u_Local2.b
#define SHADOW_MAXBRIGHT					u_Local2.a

#define NIGHT_SCALE							u_Local3.r
#define MAP_HDR_MIN							u_Local3.g
#define MAP_HDR_MAX							u_Local3.b

#define WETNESS								u_Local4.r
#define CONTRAST_STRENGTH					u_Local4.g
#define SATURATION_STRENGTH					u_Local4.b
#define BRIGHTNESS_STRENGTH					u_Local4.a

#define AO_MINBRIGHT						u_Local5.r
#define AO_MULTBRIGHT						u_Local5.g
#define VIBRANCY							u_Local5.b
#define TRUEHDR_ENABLED						u_Local5.a


#define hdr_const_1 (MAP_HDR_MIN / 255.0)
#define hdr_const_2 (255.0 / MAP_HDR_MAX)

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


float RB_PBR_DefaultsForMaterial(float MATERIAL_TYPE)
{// I probably should use an array of const's instead, but this will do for now...
	float specularReflectionScale = 0.0;

	switch (int(MATERIAL_TYPE))
	{
	case MATERIAL_WATER:			// 13			// light covering of water on a surface
		specularReflectionScale = 0.1;
		break;
	case MATERIAL_SHORTGRASS:		// 5			// manicured lawn
		specularReflectionScale = 0.0055;
		break;
	case MATERIAL_LONGGRASS:		// 6			// long jungle grass
		specularReflectionScale = 0.0065;
		break;
	case MATERIAL_SAND:				// 8			// sandy beach
		specularReflectionScale = 0.0055;
		break;
	case MATERIAL_CARPET:			// 27			// lush carpet
		specularReflectionScale = 0.0015;
		break;
	case MATERIAL_GRAVEL:			// 9			// lots of small stones
		specularReflectionScale = 0.0015;
		break;
	case MATERIAL_ROCK:				// 23			//
	case MATERIAL_STONE:
		specularReflectionScale = 0.002;
		break;
	case MATERIAL_TILES:			// 26			// tiled floor
		specularReflectionScale = 0.026;
		break;
	case MATERIAL_SOLIDWOOD:		// 1			// freshly cut timber
	case MATERIAL_TREEBARK:
		specularReflectionScale = 0.0015;
		break;
	case MATERIAL_HOLLOWWOOD:		// 2			// termite infested creaky wood
		specularReflectionScale = 0.00075;
		break;
	case MATERIAL_POLISHEDWOOD:		// 3			// shiny polished wood
		specularReflectionScale = 0.026;
		break;
	case MATERIAL_SOLIDMETAL:		// 3			// solid girders
		specularReflectionScale = 0.098;
		break;
	case MATERIAL_HOLLOWMETAL:		// 4			// hollow metal machines -- UQ1: Used for weapons to force lower parallax and high reflection...
		specularReflectionScale = 0.098;
		break;
	case MATERIAL_DRYLEAVES:		// 19			// dried up leaves on the floor
		specularReflectionScale = 0.0026;
		break;
	case MATERIAL_GREENLEAVES:		// 20			// fresh leaves still on a tree
		specularReflectionScale = 0.0055;
		break;
	case MATERIAL_PROCEDURALFOLIAGE:
		specularReflectionScale = 0.0055;
		break;
	case MATERIAL_BIRD:
	case MATERIAL_FABRIC:			// 21			// Cotton sheets
		specularReflectionScale = 0.0055;
		break;
	case MATERIAL_CANVAS:			// 22			// tent material
		specularReflectionScale = 0.0045;
		break;
	case MATERIAL_MARBLE:			// 12			// marble floors
		specularReflectionScale = 0.025;
		break;
	case MATERIAL_SNOW:				// 14			// freshly laid snow
		specularReflectionScale = 0.025;
		break;
	case MATERIAL_MUD:				// 17			// wet soil
		specularReflectionScale = 0.003;
		break;
	case MATERIAL_DIRT:				// 7			// hard mud
		specularReflectionScale = 0.002;
		break;
	case MATERIAL_CONCRETE:			// 11			// hardened concrete pavement
		specularReflectionScale = 0.00175;
		break;
	case MATERIAL_FLESH:			// 16			// hung meat, corpses in the world
		specularReflectionScale = 0.0045;
		break;
	case MATERIAL_RUBBER:			// 24			// hard tire like rubber
		specularReflectionScale = 0.0015;
		break;
	case MATERIAL_PLASTIC:			// 25			//
		specularReflectionScale = 0.028;
		break;
	case MATERIAL_PLASTER:			// 28			// drywall style plaster
		specularReflectionScale = 0.0025;
		break;
	case MATERIAL_SHATTERGLASS:		// 29			// glass with the Crisis Zone style shattering
		specularReflectionScale = 0.025;
		break;
	case MATERIAL_ARMOR:			// 30			// body armor
		specularReflectionScale = 0.055;
		break;
	case MATERIAL_ICE:				// 15			// packed snow/solid ice
		specularReflectionScale = 0.045;
		break;
	case MATERIAL_GLASS:			// 10			//
	case MATERIAL_DISTORTEDGLASS:
	case MATERIAL_DISTORTEDPUSH:
	case MATERIAL_DISTORTEDPULL:
	case MATERIAL_CLOAK:
	case MATERIAL_FORCEFIELD:
		specularReflectionScale = 0.035;
		break;
	case MATERIAL_BPGLASS:			// 18			// bulletproof glass
		specularReflectionScale = 0.033;
		break;
	case MATERIAL_COMPUTER:			// 31			// computers/electronic equipment
		specularReflectionScale = 0.042;
		break;
	case MATERIAL_PUDDLE:
		specularReflectionScale = 0.098;
		break;
	case MATERIAL_LAVA:
		specularReflectionScale = 0.002;
		break;
	case MATERIAL_EFX:
	case MATERIAL_BLASTERBOLT:
	case MATERIAL_FIRE:
	case MATERIAL_SMOKE:
	case MATERIAL_MAGIC_PARTICLES:
	case MATERIAL_MAGIC_PARTICLES_TREE:
	case MATERIAL_FIREFLIES:
	case MATERIAL_PORTAL:
	case MATERIAL_MENU_BACKGROUND:
		specularReflectionScale = 0.0;
		break;
	case MATERIAL_SKYSCRAPER:
		specularReflectionScale = 0.055;
		break;
	default:
		specularReflectionScale = 0.0075;
		break;
	}

	if (int(MATERIAL_TYPE) < MATERIAL_LAST)
	{// Check for game specified overrides...
		if (u_MaterialSpeculars[int(MATERIAL_TYPE)] != 0.0)
		{
			specularReflectionScale = u_MaterialSpeculars[int(MATERIAL_TYPE)];
		}
	}

	return specularReflectionScale;
}


vec4 normalVector(vec3 color) {
	vec4 normals = vec4(color.rgb, length(color.rgb) / 3.0);
	normals.rgb = vec3(length(normals.r - normals.a), length(normals.g - normals.a), length(normals.b - normals.a));

	// Contrast...
#define normLower ( 32.0 / 255.0 )
#define normUpper (255.0 / 212.0 )
	vec3 N = clamp((clamp(normals.rgb - normLower, 0.0, 1.0)) * normUpper, 0.0, 1.0);

	return vec4(vec3(1.0) - (normalize(pow(N, vec3(4.0))) * 0.5 + 0.5), 1.0 - normals.a);
}

float drawObject(in vec3 p){
    p = abs(fract(p)-.5);
    return dot(p, vec3(.5));
}

float cellTile(in vec3 p)
{
    p /= 5.5;

    vec4 v, d; 
    d.x = drawObject(p - vec3(.81, .62, .53));
    p.xy = vec2(p.y-p.x, p.y + p.x)*.7071;
    d.y = drawObject(p - vec3(.39, .2, .11));
    p.yz = vec2(p.z-p.y, p.z + p.y)*.7071;
    d.z = drawObject(p - vec3(.62, .24, .06));
    p.xz = vec2(p.z-p.x, p.z + p.x)*.7071;
    d.w = drawObject(p - vec3(.2, .82, .64));

    v.xy = min(d.xz, d.yw), v.z = min(max(d.x, d.y), max(d.z, d.w)), v.w = max(v.x, v.y); 
   
    d.x =  min(v.z, v.w) - min(v.x, v.y);

    return d.x*2.66;
}

float aomap(vec3 p)
{
    float n = (.5-cellTile(p))*1.5;
    return p.y + dot(sin(p/2. + cos(p.yzx/2. + 3.14159/2.)), vec3(.5)) + n;
}

vec2 GetMapTC(vec3 pos)
{
	vec2 mapSize = u_Maxs.xy - u_Mins.xy;
	return (pos.xy - u_Mins.xy) / mapSize;
}

float calculateAO(in vec3 pos, in vec3 nor, in vec2 texCoords)
{
	float sca = 0.00013/*2.0*/, occ = 0.0;

	for( int i=0; i<5; i++ ) {
		float hr = 0.01 + float(i)*0.5/4.0;        
		float dd = aomap(nor * hr + pos);
		occ += (hr - dd)*sca;
		sca *= 0.7;
	}

	return clamp( 1.0 - occ, 0.0, 1.0 );    
}


vec3 TrueHDR ( vec3 color )
{
	return clamp((clamp(color.rgb - hdr_const_1, 0.0, 1.0)) * hdr_const_2, 0.0, 1.0);
}

vec3 Vibrancy ( vec3 origcolor, float vibrancyStrength )
{
	vec3	lumCoeff = vec3(0.212656, 0.715158, 0.072186);
	float	max_color = max(origcolor.r, max(origcolor.g,origcolor.b));
	float	min_color = min(origcolor.r, min(origcolor.g,origcolor.b));
	float	color_saturation = max_color - min_color;
	float	luma = dot(lumCoeff, origcolor.rgb);
	return mix(vec3(luma), origcolor.rgb, (1.0 + (vibrancyStrength * (1.0 - (sign(vibrancyStrength) * color_saturation)))));
}


//
// Full lighting... Blinn phong and basic lighting as well...
//
float wetSpecular(vec3 n,vec3 l,vec3 e,float s) {    
    float nrm = (s + 8.0) / (3.1415 * 8.0);
    return pow(max(dot(reflect(e,n),l),0.0),s) * nrm;
}

float getSpecialSauce(vec3 color)
{
	// Secret sauce diffuse adaption.
	float b = clamp(length(clamp(color.rgb, 0.0, 1.0))/3.0, 0.0, 1.0);
	b = clamp(clamp(pow(b, 3.5), 0.0, 1.0) * 512.0, 2.0, 4.0);
	return b;
}

float orenNayar( in vec3 n, in vec3 v, in vec3 ldir, float specPower )
{
    float r2 = pow(specPower, 2.0);
    float a = 1.0 - 0.5*(r2/(r2+0.57));
    float b = 0.45*(r2/(r2+0.09));

    float nl = dot(n, ldir);
    float nv = dot(n, v);

    float ga = dot(v-n*nv,n-n*nl);

	return max(0.0,nl) * (a + b*max(0.0,ga) * sqrt((1.0-nv*nv)*(1.0-nl*nl)) / max(nl, nv));
}

vec3 blinn_phong(vec3 pos, vec3 color, vec3 bump, vec3 view, vec3 light, vec3 lightColor, float specPower, vec3 lightPos, float wetness) {
	// Ambient light.
	float ambience = 0.24;

	// Nayar diffuse light.
	float base = orenNayar( bump, view, light, specPower );
	base *= getSpecialSauce(color);

	// Specular light.
	vec3 reflection = normalize(reflect(-view, bump));
    float specular = clamp(pow(clamp(dot(light, reflection), 0.0, 1.0), 15.0), 0.0, 1.0) * 0.25;

	float wetSpec = 0.0;

	if (wetness > 0.0)
	{// Increase specular strength when the ground is wet during/after rain...
		wetSpec = wetSpecular(bump, -light, view, 256.0) * wetness * 0.5;
		wetSpec += wetSpecular(bump, -light, view, 4.0) * wetness * 8.0;
		wetSpec *= 0.5;
	}
	
	return (lightColor * ambience) + (base * lightColor) + (lightColor * specular) + (lightColor * wetSpec);
}

// For all settings: 1.0 = 100% 0.5=50% 1.5 = 150%
vec3 ContrastSaturationBrightness(vec3 color, float con, float sat, float brt)
{
	// Increase or decrease theese values to adjust r, g and b color channels seperately
	const float AvgLumR = 0.5;
	const float AvgLumG = 0.5;
	const float AvgLumB = 0.5;
	
	const vec3 LumCoeff = vec3(0.2125, 0.7154, 0.0721);
	
	vec3 AvgLumin = vec3(AvgLumR, AvgLumG, AvgLumB);
	vec3 brtColor = color * brt;
	vec3 intensity = vec3(dot(brtColor, LumCoeff));
	vec3 satColor = mix(intensity, brtColor, sat);
	vec3 conColor = mix(AvgLumin, satColor, con);
	return conColor;
}

vec3 GetScreenPixel(inout vec2 texCoords)
{
	return texture(u_DiffuseMap, texCoords).rgb;
}

void main(void)
{
	vec2 texCoords = var_TexCoords;
	vec4 position = textureLod(u_PositionMap, texCoords, 0.0);
	vec4 color = vec4(texture(u_DiffuseMap, texCoords).rgb, 1.0);
	vec4 outColor = color;

	if (position.a - 1.0 >= MATERIAL_SKY
		|| position.a - 1.0 == MATERIAL_SUN
		|| position.a - 1.0 == MATERIAL_GLASS
		|| position.a - 1.0 == MATERIAL_DISTORTEDGLASS
		|| position.a - 1.0 == MATERIAL_DISTORTEDPUSH
		|| position.a - 1.0 == MATERIAL_DISTORTEDPULL
		|| position.a - 1.0 == MATERIAL_CLOAK
		|| position.a - 1.0 == MATERIAL_FORCEFIELD
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
		if (MAP_USE_PALETTE_ON_SKY > 0.0)
		{
			if (!(CONTRAST_STRENGTH == 1.0 && SATURATION_STRENGTH == 1.0 && BRIGHTNESS_STRENGTH == 1.0))
			{// C/S/B enabled...
				outColor.rgb = ContrastSaturationBrightness(outColor.rgb, CONTRAST_STRENGTH, SATURATION_STRENGTH, BRIGHTNESS_STRENGTH);
			}
		}

		if (TRUEHDR_ENABLED > 0.0)
		{// TrueHDR enabled...
			outColor.rgb = TrueHDR(outColor.rgb);
		}

		if (VIBRANCY > 0.0)
		{// Vibrancy enabled...
			outColor.rgb = Vibrancy(outColor.rgb, VIBRANCY);
		}

		outColor.rgb = clamp(outColor.rgb, 0.0, 1.0);
		gl_FragColor = outColor;

		gl_FragColor.rgb = pow(gl_FragColor.rgb, vec3(1.0 / GAMMA_CORRECTION));
		gl_FragColor.a = 1.0;
		return;
	}


	float lightsReflectionFactor = RB_PBR_DefaultsForMaterial(position.a-1.0) * 4.0;
	bool isPuddle = (position.a - 1.0 == MATERIAL_PUDDLE) ? true : false;


	//
	// Grab and set up our normal value, from lightall buffers, or fallback to offseting the flat normal buffer by pixel luminances, etc...
	//

	vec4 norm = textureLod(u_NormalMap, texCoords, 0.0);
	norm.xyz = DecodeNormal(norm.xy);

	vec3 flatNorm = norm.xyz = normalize(norm.xyz);
	vec4 normalDetail = normalVector(outColor.rgb);

	// Simply offset the normal value based on the detail value... It looks good enough, but true PBR would probably want to use the tangent/bitangent below instead...
	normalDetail.rgb = normalize(clamp(normalDetail.xyz, 0.0, 1.0) * 2.0 - 1.0);

	norm.xyz = normalize(mix(norm.xyz, normalDetail.xyz, 0.25 * (length((norm.xyz * 0.5 + 0.5) - (normalDetail.xyz * 0.5 + 0.5)) / 3.0)));

	//
	// Set up the general directional stuff, to use throughout the shader...
	//

	vec3 N = norm.xyz;
	vec3 E = normalize(u_ViewOrigin.xyz - position.xyz);
	vec3 sunDir = normalize(u_ViewOrigin.xyz - u_PrimaryLightOrigin.xyz);

	float b = clamp(length(outColor.rgb/3.0), 0.0, 1.0);
	b = clamp(pow(b, 64.0), 0.0, 1.0);
	vec3 of = clamp(pow(vec3(b*65536.0), -N), 0.0, 1.0) * 0.25;
	vec3 bump = normalize(N+of);

	float wetness = 0.0;

	if (WETNESS > 0.0)
	{
		if ((position.a - 1.0 == MATERIAL_PUDDLE)
			|| (position.a - 1.0 == MATERIAL_TREEBARK)
			|| (position.a - 1.0 == MATERIAL_GREENLEAVES)
			|| (position.a - 1.0 == MATERIAL_PROCEDURALFOLIAGE)
			|| (position.a - 1.0 == MATERIAL_SHORTGRASS)
			|| (position.a - 1.0 == MATERIAL_LONGGRASS)
			|| (position.a - 1.0 == MATERIAL_ROCK)
			|| (position.a - 1.0 == MATERIAL_SAND)
			|| (position.a - 1.0 == MATERIAL_DIRT)
			|| (position.a - 1.0 == MATERIAL_MUD))
		{
			// Hmm. this really should check for splat mapped pixels, but I dont have buffer room, and dont want to add a new screen buffer
			// unless I must... For now i'll use materials that generally get splat mapped, but should not be used indoors...
			wetness = WETNESS;

		}
	}

	//
	// Now all the hard work...
	//

	float finalShadow = 1.0;

#if defined(USE_SHADOWMAP)
	if (SHADOWS_ENABLED > 0.0 && NIGHT_SCALE < 1.0)
	{
		float shadowValue = texture(u_ShadowMap, texCoords).r;
		float selfShadow = max(dot(flatNorm, -sunDir.rgb), 0.0);

		if (position.a - 1.0 == MATERIAL_PROCEDURALFOLIAGE)
		{
			selfShadow = clamp(selfShadow + 0.5, 0.0, 1.0);
		}

		shadowValue = min(shadowValue, selfShadow);

		shadowValue = pow(shadowValue, 1.5);

#define sm_cont_1 ( 64.0 / 255.0)
#define sm_cont_2 (255.0 / 200.0)
		shadowValue = clamp((clamp(shadowValue - sm_cont_1, 0.0, 1.0)) * sm_cont_2, 0.0, 1.0);
		finalShadow = clamp(shadowValue + SHADOW_MINBRIGHT, SHADOW_MINBRIGHT, SHADOW_MAXBRIGHT);
	}
#endif //defined(USE_SHADOWMAP)

	vec3 specularColor = vec3(0.0);
	vec3 skyColor = vec3(0.0);

	if (SUN_PHONG_SCALE > 0.0 && lightsReflectionFactor > 0.0)
	{// If r_blinnPhong is <= 0.0 then this is pointless...
		float phongFactor = (SUN_PHONG_SCALE * 12.0);

		float PshadowValue = 1.0 - texture(u_RoadsControlMap, texCoords).a;

		if (phongFactor > 0.0 && NIGHT_SCALE < 1.0 && finalShadow > 0.0)
		{// this is blinn phong
			vec3 lightColor = Vibrancy(u_PrimaryLightColor.rgb, 1.0);
			float lightMult = lightsReflectionFactor;

			if (lightMult > 0.0)
			{
				lightColor *= lightMult;
				lightColor *= clamp(1.0 - NIGHT_SCALE, 0.0, 1.0); // Day->Night scaling of sunlight...

				lightColor.rgb *= lightsReflectionFactor * phongFactor * 8.0;

				vec3 addColor = blinn_phong(position.xyz, outColor.rgb, bump, E, -sunDir, clamp(lightColor, 0.0, 1.0), 1.0, u_PrimaryLightOrigin.xyz, wetness);

				if (position.a - 1.0 == MATERIAL_GREENLEAVES || position.a - 1.0 == MATERIAL_PROCEDURALFOLIAGE)
				{// Light bleeding through tree leaves...
					float ndotl = clamp(dot(bump, sunDir), 0.0, 1.0);
					float diffuse = pow(ndotl, 2.0) * 1.25;

					addColor += lightColor * diffuse;
				}

				float puddleMult = 1.0;
				if (isPuddle) puddleMult = 0.25;
				outColor.rgb = outColor.rgb + max(addColor * PshadowValue * finalShadow * puddleMult, vec3(0.0));
			}
		}

		if (u_lightCount > 0.0)
		{
			phongFactor = 12.0;

			vec3 addedLight = vec3(0.0);

			for (int li = 0; li < u_lightCount; li++)
			{
				vec3 lightPos = u_lightPositions2[li].xyz;
				vec3 lightDir = normalize(lightPos - position.xyz);
				float lightDist = distance(lightPos, position.xyz);

				float lightPlayerDist = distance(lightPos.xyz, u_ViewOrigin.xyz);

				float lightDistMult = 1.0 - clamp((lightPlayerDist / MAX_DEFERRED_LIGHT_RANGE), 0.0, 1.0);
				lightDistMult = pow(lightDistMult, 2.0);

				if (lightDistMult > 0.0)
				{
					// Attenuation...
					float lightFade = 1.0 - clamp((lightDist * lightDist) / (u_lightDistances[li] * u_lightDistances[li]), 0.0, 1.0);
					lightFade = pow(lightFade, 2.0);

					if (lightFade > 0.0)
					{
						float lightStrength = lightDistMult * lightFade * lightsReflectionFactor * 0.5;
						float maxLightsScale = mix(0.01, 1.0, clamp(pow(1.0 - (lightPlayerDist / u_lightMaxDistance), 0.5), 0.0, 1.0));
						lightStrength *= maxLightsScale;

						if (lightStrength > 0.0)
						{
							vec3 lightColor = (u_lightColors[li].rgb / length(u_lightColors[li].rgb)) * MAP_EMISSIVE_COLOR_SCALE * maxLightsScale;
							float selfShadow = clamp(pow(clamp(dot(-lightDir.rgb, bump.rgb), 0.0, 1.0), 8.0) * 0.6 + 0.6, 0.0, 1.0);

							float lightSpecularPower = mix(0.1, 0.5, clamp(lightsReflectionFactor, 0.0, 1.0)) * clamp(lightStrength * phongFactor, 0.0, 1.0);

							addedLight.rgb += blinn_phong(position.xyz, outColor.rgb, bump, E, lightDir, clamp(lightColor, 0.0, 1.0), lightSpecularPower, lightPos, wetness) * lightFade * selfShadow;

							if (position.a - 1.0 == MATERIAL_GREENLEAVES || position.a - 1.0 == MATERIAL_PROCEDURALFOLIAGE)
							{// Light bleeding through tree leaves...
								float ndotl = clamp(dot(bump, -lightDir), 0.0, 1.0);
								float diffuse = pow(ndotl, 2.0) * 4.0;

								addedLight.rgb += lightColor * diffuse * lightFade * selfShadow * lightSpecularPower;
							}
						}
					}
				}
			}

			outColor.rgb = outColor.rgb + max(addedLight * 0.5 * PshadowValue, vec3(0.0));
		}
	}

	if (AO_TYPE >= 1.0)
	{// Fast AO enabled...
		float ao = calculateAO(sunDir, N * 10000.0, texCoords);
		float selfShadow = clamp(pow(clamp(dot(-sunDir.rgb, bump.rgb), 0.0, 1.0), 8.0), 0.0, 1.0);
		ao = clamp(((ao + selfShadow) / 2.0) * AO_MULTBRIGHT + AO_MINBRIGHT, AO_MINBRIGHT, 1.0);
		outColor.rgb *= ao;
	}


	finalShadow = mix(finalShadow, 1.0, NIGHT_SCALE); // Dampen out shadows at sunrise/sunset...
	outColor.rgb *= finalShadow;


	if (!(CONTRAST_STRENGTH == 1.0 && SATURATION_STRENGTH == 1.0 && BRIGHTNESS_STRENGTH == 1.0))
	{// C/S/B enabled...
		outColor.rgb = ContrastSaturationBrightness(outColor.rgb, CONTRAST_STRENGTH, SATURATION_STRENGTH, BRIGHTNESS_STRENGTH);
	}

	if (TRUEHDR_ENABLED > 0.0)
	{// TrueHDR enabled...
		outColor.rgb = TrueHDR( outColor.rgb );
	}

	if (VIBRANCY > 0.0)
	{// Vibrancy enabled...
		outColor.rgb = Vibrancy( outColor.rgb, VIBRANCY );
	}

	outColor.rgb = clamp(outColor.rgb, 0.0, 1.0);
	gl_FragColor = outColor;
	
	gl_FragColor.rgb = pow(gl_FragColor.rgb, vec3(1.0 / GAMMA_CORRECTION));
	gl_FragColor.a = 1.0;
}

