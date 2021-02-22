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
uniform sampler2D u_DiffuseMap;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec4	u_ViewInfo; // zfar / znear, zfar
uniform vec3	u_ViewOrigin;
uniform vec2	u_Dimensions;
uniform float	u_Time;
uniform vec4	u_Local0; // r_testvalue0, r_testvalue1, r_testvalue2, r_testvalue3


varying vec2	var_TexCoords;

//#define SHARPEN_ENABLED
//#define HDR_ENABLED
//#define SSAO_ENABLED
//#define LF_ENABLED
#define RAIN_ENABLED

vec2 resolution = u_Dimensions;
vec2 vTexCoords = var_TexCoords;

#define iTime (u_Time * u_Local0.r)
//#define iTime (1.0-u_Time)

/*float Noise( in vec3 x, float lod_bias )
{	
    vec3 p = floor(x);
    vec3 f = fract(x);
	
	vec2 uv = (p.xy+vec2(37.0,17.0)*p.z) + f.xy;
	vec2 rg = texture( u_GlowMap, uv*(1./256.0), lod_bias ).yx;
	return mix( rg.x, rg.y, f.z );
}*/

float hash( const in float n ) {
	return fract(sin(n)*4378.5453);
}

float Noise(in vec3 o, float lod) 
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

vec4 BlendUnder(vec4 accum,vec4 col)
{
	col = clamp(col,vec4(0),vec4(1));	
	col.rgb *= col.a;
	accum += col*(1.0 - accum.a);	
	return accum;
}

vec4 March(vec4 accum, vec3 viewP, vec3 viewD, vec2 mM, float Far)
{
	//exponential stepping
#define SHQ	
//#define MEDQ	
//#define YUCKQ	
#ifdef SHQ
	#define STEPS	128	
	float slices = 512.;
#endif	
#ifdef MEDQ
	#define STEPS	64	
	float slices = 256.;
#endif	
#ifdef YUCKQ	
	#define STEPS	32	
	float slices = 128.;
#endif
	
	//float Far = 10.;
	
	float sliceStart = log2(mM.x)*(slices/log2(Far));
	float sliceEnd = log2(mM.y)*(slices/log2(Far));
			
	float last_t = mM.x;
	
	for (int i=0; i<STEPS; i++)
	{							
		sliceStart += 1.;
		float sliceI = sliceStart;// + float(i);	//advance an exponential step
		float t = exp2(sliceI*(log2(Far)/slices));	//back to linear

		vec3 p = viewP+t*viewD;
		vec3 uvw = p;
		uvw.y/=10.;
		uvw.y += iTime;
		uvw *= 30.;
		
		float h = (1.-((p.y+1.)*0.5));
		float dens = Noise(uvw,-100.);// * h;
			dens*=dens;
			dens*=dens;
		dens -= 0.25;
		dens *= (t-last_t)*1.5;
		
		accum = BlendUnder(accum,vec4(vec3(1.),dens));
			
		last_t=t;
	}
	
	vec3 p = viewP+mM.y*viewD;
	vec3 uvw = p;
	uvw *= 20.;
	uvw.y += iTime*20.;
	float dens = Noise(uvw,-100.);
	dens=sin(dens);
	dens*=dens;
	dens*=dens;
	dens*=.4;
	accum = BlendUnder(accum,vec4(1.,1.,1.,dens));
	
	return accum;
}

//==============================================================================
void main(void)
{
	vec3 fColor;

	vec3 position = texture(u_PositionMap, var_TexCoords).xzy;
	vec3 viewP = u_ViewOrigin.xzy;
	vec3 viewD = normalize(viewP - position) * u_Local0.g;
	float far = distance(viewP, position);

	//ground plane
	float floor_height = position.y;//-1.;

	float floor_intersect_t = (-viewP.y + floor_height) / (viewD.y);

	vec3 p = viewP+viewD * floor_intersect_t;
	
    vec3 c = texture(u_DiffuseMap, vTexCoords).xyz;

	//c = pow(c, vec3(2.2));
	//c *= 0.8;

	float ceil_intersect_t = (-viewP.y + 1.0) / (viewD.y);

	vec4 a = March(vec4(0), viewP, viewD, vec2(ceil_intersect_t, floor_intersect_t), far);

	c = BlendUnder(a, vec4(c, 1.0)).xyz;

	gl_FragColor = vec4(c, 1.0);
}
