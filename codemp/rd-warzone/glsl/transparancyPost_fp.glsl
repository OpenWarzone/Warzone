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
uniform sampler2D					u_ShadowMap;
uniform sampler2D					u_ShadowMap2;
uniform sampler2D					u_ShadowMap3;
uniform sampler2D					u_ShadowMap4;
uniform sampler2D					u_ShadowMap5;
uniform sampler3D					u_VolumeMap;
uniform sampler2D					u_MoonMaps[4];
};
#else //!defined(USE_BINDLESS_TEXTURES)
uniform sampler2D					u_DiffuseMap;			// backBufferMap
uniform sampler2D					u_PositionMap;			// position map
uniform sampler2D					u_WaterPositionMap;		// transparancy map
uniform sampler2D					u_ScreenDepthMap;		// depth map
uniform sampler2D					u_NormalMap;			// transprancy surf normals
uniform sampler2D					u_OverlayMap;			// force field texture
#endif //defined(USE_BINDLESS_TEXTURES)


uniform vec4						u_Local0;				// testvalue0, testvalue1, testvalue2, testvalue3
uniform vec2						u_Dimensions;
uniform vec3						u_ViewOrigin;
uniform float						u_Time;

varying vec2						var_TexCoords;
varying vec3						var_ViewDir;


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

float hash( const in float n ) {
	return fract(sin(n)*4378.5453);
}

float noise(in vec3 o) 
{
	vec3 p = floor(o);
	vec3 fr = fract(o);
		
	float n = p.x + p.y*57.0 + p.z * 1009.0;

	float a = hash(n+  0.0);
	float b = hash(n+  1.0);
	float c = hash(n+ 57.0);
	float d = hash(n+ 58.0);
	
	float e = hash(n+  0.0 + 1009.0);
	float f = hash(n+  1.0 + 1009.0);
	float g = hash(n+ 57.0 + 1009.0);
	float h = hash(n+ 58.0 + 1009.0);
	
	
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

const mat3 m = mat3( 0.00,  0.80,  0.60,
                    -0.80,  0.36, -0.48,
                    -0.60, -0.48,  0.64 );

float SmoothNoise( vec3 p )
{
    float f;
    f  = 0.5000*noise( p ); p = m*p*2.02;
    f += 0.2500*noise( p ); 
	
    return f * (1.0 / (0.5000 + 0.2500));
}

float _IntersectPower = 0.18;
float _RimStrength = 1.69;
float _DistortStrength = 0.80;
float _DistortTimeFactor = 4.195;//0.195;

vec4 ForceField (vec3 pMap, vec4 transparancyMap, vec4 inColor)
{
	vec4 outColor;


	//float sceneZ = texture(u_ScreenDepthMap, var_TexCoords).r;
	//vec4 normals;
	//normals.rgb = texture(u_NormalMap, var_TexCoords).rgb;
	//normals.a = normals.b;
	//normals.rgb = DecodeNormal(normals.rg);

	//float partZ = 1.0 - distance(u_ViewOrigin, transparancyMap.xyz) / distance(u_ViewOrigin, pMap.xyz);

	//float diff = sceneZ - partZ;
	//float intersect = (1.0 - diff) * _IntersectPower;

	vec3 viewDir = normalize(u_ViewOrigin - pMap.xyz);
	//float rim = 1.0 - abs(dot(normals.xyz, viewDir)) * _RimStrength;
	//vec3 normals = viewDir * 0.5 + 0.5;
	//float rim = 1.0 - length(dot(normals, viewDir)) * _RimStrength;
	//float glow = max(intersect, rim);

	vec3 offset = viewDir * 0.5 + 0.5;
	offset *= -48.0;
	vec3 off;
	off.x = noise(offset.xyz*8.0 - u_Time * _DistortTimeFactor);
	off.y = noise(offset.xzy*8.0 - u_Time * _DistortTimeFactor);
	off.z = noise(offset.zxy*8.0 - u_Time * _DistortTimeFactor);
	offset += off*2.0;
	vec4 color = textureProj(u_OverlayMap, offset, 2.0/*partZ*/);

	float toff = pow((1.0 - color.r), 0.75) * 0.025;
	vec4 refractionColor = texture(u_DiffuseMap, var_TexCoords + toff);
	outColor = mix(inColor, refractionColor /** glow*/ + color, 0.5/*pow(glow, 0.75)*/);
	
	
	//
	// Hit FX...
	//
	
#if 0
	vec3 hit3 = vec3(-1.0, -3.0, -0.01);
	vec3 hit2 = vec3(-0.6972498297691345, 1.0605579614639282, -0.27839863300323486);
	vec3 hit = vec3(0.0,0.0,2.0);

	if( dot( normalize(hit), normalize(normals.xyz) ) > 0.99 ) outColor = vec4(1.0,0.0,0.0,1.0);
	if( dot( normalize(hit2), normalize(normals.xyz) ) > 0.995 ) outColor = vec4(1.0,0.0,0.0,1.0);

	//float hAngle = dot( normalize(hit3), normalize(v_normal) );
	//float angleStart = fract(u_Time * 0.001 * 0.4) * 2.0 - 1.0; //Map 0 to 1 to -1 to 1
	//float thickness = 0.03;
	//if( hAngle <= angleStart && hAngle >= angleStart-thickness ) gl_FragColor += vec4(1.0,0.0,0.0,1.0);
#endif
		
#if 0
	vec3 positionChange = vec3(0.0, 0.0, 0.0);
	vec3 objPos = mul(WorldToObject, vec4(IN.worldPos, 1.0)).xyz;
	
	for (int i = 0; i < _PointsSize; ++i) {
		float amount = max(0, frac(1.0 - max(0.0, (_Points[i].w * _ImpactSize) - distance(_Points[i].xyz, objPos.xyz)) / _ImpactSize) * (1.0 - _Points[i].w));
		emissive += amount;
		positionChange += amount * normalize(objPos.xyz - _Points[i].xyz);
	}

	vec4 screenRefraction = ObjectToClipPos(float4(positionChange, 1.0));
	screenRefraction = normalize(screenRefraction) * _ImpactRingRefraction;
	
	vec4 c = tex2Dproj (_GrabTexture, UNITY_PROJ_COORD(IN.grabUV + IN.refract + float4(screenRefraction.xy, 0, 0))) * inColor;
#endif

	return outColor;
}

void main ( void )
{
	vec4 transparancyMap = texture(u_WaterPositionMap, var_TexCoords);
	vec3 pMap = texture(u_PositionMap, var_TexCoords).rgb;
	vec3 color = textureLod(u_DiffuseMap, var_TexCoords, 0.0).rgb;

	if (distance(transparancyMap.xyz, u_ViewOrigin) < distance(pMap.xyz, u_ViewOrigin))
	{
		if (transparancyMap.a == 2.0)
		{// distorted glass...
			vec2 uv = var_TexCoords;
			float factor = pow(SmoothNoise(transparancyMap.xyz * 0.05), 8.0) * 0.2;
			uv += factor / 8.0;
			color.rgb = texture(u_DiffuseMap, uv, factor * 1.5).rgb;
			gl_FragColor = vec4(color, 1.0);
			return;
		}
		else if (transparancyMap.a == 3.0)
		{// distorted push effect...
			vec2 uv = var_TexCoords;
			float factor = pow(transparancyMap.x * 0.5, 1.333);
			uv += (factor / 8.0) * normalize(transparancyMap.zy);
			color.rgb = texture(u_DiffuseMap, uv, factor * 1.5).rgb;
			gl_FragColor = vec4(color, 1.0);
			return;
		}
		else if (transparancyMap.a == 4.0)
		{// distorted pull effect...
			vec2 uv = var_TexCoords;
			float factor = pow(transparancyMap.x * 0.5, 1.333);
			uv += (factor / 8.0) * normalize(transparancyMap.zy);
			color.rgb = texture(u_DiffuseMap, uv, factor * 1.5).rgb;
			gl_FragColor = vec4(color, 1.0);
			return;
		}
		else if (transparancyMap.a == 5.0)
		{// cloak...
			const vec3 keyColor = vec3(0.051,0.639,0.149);

			vec2 uv = var_TexCoords;
			vec3 colorDelta = transparancyMap.xzy - keyColor.rgb;
			float factor = length(colorDelta) * u_Local0.r;
			uv += (factor * colorDelta.rb) / 8.0;
			color.rgb = texture(u_DiffuseMap, uv, factor * 1.5).rgb;
			gl_FragColor = vec4(color, 1.0);
			return;
		}
		else if (transparancyMap.a == 6.0)
		{// force fields...
			gl_FragColor = vec4(ForceField(pMap, transparancyMap, vec4(color, 1.0)).rgb, 1.0);
			return;
		}
	}

	gl_FragColor = vec4(color, 1.0);
}
