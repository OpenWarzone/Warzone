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
uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_NormalMap;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec2		u_Dimensions;
uniform vec4		u_ViewInfo; // zmin, zmax, zmax / zmin

varying vec2		var_TexCoords;


vec2 pixelSize = vec2(1.0) / u_Dimensions;

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

vec3 normals(vec2 tex)//get normal vector from normalmap
{
	vec3 norm = textureLod(u_NormalMap, tex, 0.0).xyz;
	//norm.z = sqrt(1.0-dot(norm.xy, norm.xy)); // reconstruct Z from X and Y
	//norm.xyz = normalize(norm.xyz * 2.0 - 1.0);
	norm.xyz = DecodeNormal(norm.xy);
	return norm.xyz;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//cel shader
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void main (void)
{
	vec2 tex = var_TexCoords.xy;
	
	vec4 res = textureLod(u_DiffuseMap, tex, 0.0);
	
	vec3 Gx[3];
	Gx[0] = vec3(-1.0, 0.0, 1.0);
	Gx[1] = vec3(-2.0, 0.0, 2.0);
	Gx[2] = vec3(-1.0, 0.0, 1.0);

	vec3 Gy[3]; 
	Gy[0] = vec3( 1.0, 2.0, 1.0);
	Gy[1] = vec3( 0.0, 0.0, 0.0);
	Gy[2] = vec3( -1.0, -2.0, -1.0);

	vec3 dotx = vec3(0.0);
	vec3 doty = vec3(0.0);

	for(int i = 0; i < 3; i ++)
	{		
		dotx += Gx[i].x * normals(vec2((tex.x - pixelSize.x), (tex.y + ( (-1.0 + float(i)) * pixelSize.y))));
		dotx += Gx[i].y * normals(vec2( tex.x, (tex.y + ( (-1.0 + float(i)) * pixelSize.y))));
		dotx += Gx[i].z * normals(vec2((tex.x + pixelSize.x), (tex.y + ( (-1.0 + i) * pixelSize.y))));
		
		doty += Gy[i].x * normals(vec2((tex.x - pixelSize.x), (tex.y + ( (-1.0 + i) * pixelSize.y))));
		doty += Gy[i].y * normals(vec2( tex.x, (tex.y + ( (-1.0 + float(i)) * pixelSize.y))));
		doty += Gy[i].z * normals(vec2((tex.x + pixelSize.x), (tex.y + ( (-1.0 + float(i)) * pixelSize.y))));
	}
	
	res.xyz -= step(1.0, sqrt( dot(dotx, dotx) + dot(doty, doty) ) * 1.7/*u_ViewInfo.a*/ );
	
	gl_FragColor = res;
}
