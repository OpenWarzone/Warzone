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
uniform sampler2D					u_MoonMaps[4];
};
#else //!defined(USE_BINDLESS_TEXTURES)
uniform sampler2D						u_ShadowMap;
#endif //defined(USE_BINDLESS_TEXTURES)

//uniform vec3							u_LightForward;
uniform vec3							u_LightUp;
uniform vec3							u_LightRight;
uniform vec4							u_LightOrigin;
uniform float							u_LightRadius;
uniform vec3							u_ViewOrigin;

uniform vec4							u_Local0;			// shadowMapSize, testshadervalues
uniform vec4							u_Local1;			// realLightOrigin[0], realLightOrigin[1], realLightOrigin[2], usingTessellation
uniform vec4							u_Local2;			// playerOrigin[0], playerOrigin[1], playerOrigin[2], invLightPower

#if defined(USE_TESSELLATION) || defined(USE_TESSELLATION_3D)
	in precise vec3				WorldPos_FS_in;
	#define m_vertPos			WorldPos_FS_in
#else //!defined(USE_TESSELLATION) || defined(USE_TESSELLATION_3D)
	varying vec3				var_Position;
	//varying vec3				var_Normal;
	#define m_vertPos			var_Position
#endif //defined(USE_TESSELLATION) || defined(USE_TESSELLATION_3D)

out vec4 out_Glow;
out vec4 out_Position;
out vec4 out_Normal;
#ifdef __USE_REAL_NORMALMAPS__
out vec4 out_NormalDetail;
#endif //__USE_REAL_NORMALMAPS__

void main()
{
	vec3 lightToPos = m_vertPos.xyz - u_LightOrigin.xyz;

	float lightDist = distance(m_vertPos.xyz, u_Local1.xyz);

	float intensity = 1.0 - clamp(lightDist / u_LightRadius, 0.0, 1.0);
	intensity = clamp(pow(intensity, 5.0), 0.0, 1.0);

	intensity = mix(intensity, 0.0, u_Local2.a);

	// Only draw shadows away from the light, not toward it, dah!
	vec3 lightToPosN = normalize(u_Local1.xyz - m_vertPos.xyz);
	vec3 entityToPosN = normalize(u_Local2.xyz - m_vertPos.xyz);
	float dt = (max(dot(lightToPosN, entityToPosN), 0.0) > 0.0) ? 1.0 : 0.0;
	//dt *= (distance(u_Local2.xyz, m_vertPos.xyz) < lightDist) ? 1.0 : 0.0;
	intensity *= dt;

	vec2 st = vec2(-dot(u_LightRight, lightToPos), dot(u_LightUp, lightToPos));
	st = st * 0.5 + 0.5;

	if (!(st.x >= 0.0 && st.x <= 1.0 && st.y >= 0.0 && st.y <= 1.0))
	{
		intensity = 0.0;
	}

	if (m_vertPos.z > u_Local2.z + 64.0)
	{// Light dir should always be facing down... Why the fuck is rend2 trying to draw shadows above the light???
		intensity = 0.0;
	}

	if (intensity > 0.0)
	{
#if defined(USE_PCF)
		float pcf = float(texture(u_ShadowMap, st + vec2(-1.0 / u_Local0.r, -1.0 / u_Local0.r)).r != 1.0);
		pcf += float(texture(u_ShadowMap, st + vec2(1.0 / u_Local0.r, -1.0 / u_Local0.r)).r != 1.0);
		pcf += float(texture(u_ShadowMap, st + vec2(-1.0 / u_Local0.r, 1.0 / u_Local0.r)).r != 1.0);
		pcf += float(texture(u_ShadowMap, st + vec2(1.0 / u_Local0.r, 1.0 / u_Local0.r)).r != 1.0);
		pcf /= 4.0;
#else
		float pcf = float(texture(u_ShadowMap, st).r != 1.0);
#endif

		intensity *= pcf;
		//intensity = pcf * (1.0 - length(st));
		intensity = clamp(intensity, 0.0, 0.4);
	}

	out_Color.rgb = vec3(.0, .0, .0);
	out_Color.a = intensity;
	out_Glow = vec4(0.0);
	out_Position = vec4(0.0);
	out_Normal = vec4(0.0);
#ifdef __USE_REAL_NORMALMAPS__
	out_NormalDetail = vec4(0.0);
#endif //__USE_REAL_NORMALMAPS__
}
