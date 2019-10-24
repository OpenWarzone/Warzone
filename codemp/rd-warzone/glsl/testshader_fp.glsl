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
uniform sampler2D					u_DiffuseMap; // screen/texture map
uniform sampler2D					u_ScreenDepthMap;
uniform sampler2D					u_PositionMap;
uniform sampler2D					u_SpecularMap; // color random image
uniform sampler2D					u_GlowMap;
uniform sampler2D					u_NormalMap;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform mat4						u_ProjectionMatrix;

uniform vec4						u_ViewInfo;
uniform vec2						u_Dimensions;
uniform vec4						u_Local0; // r_testvalue's
uniform vec4						u_Local1; // r_testshadervalue's


uniform vec4						u_PrimaryLightOrigin;
uniform vec3						u_PrimaryLightColor;

// Position of the camera
uniform vec3						u_ViewOrigin;
uniform float						u_Time;

varying vec2						var_TexCoords;



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



#if 0
//
// Drawing effect...
//

#define Res0 u_Dimensions//textureSize(iChannel0,0)
#define Res1 vec2(2048.0)//textureSize(iChannel1,0)

#define Time u_Time

vec4 getRand(vec2 pos)
{
	return textureLod(u_SpecularMap, pos / Res1 / Res0.y*1080., 0.0);
}

vec4 getCol(vec2 pos)
{
	// take aspect ratio into account
	vec2 uv = ((pos - Res0.xy*.5) / Res0.y*Res0.y) / Res0.xy + .5;
	vec4 c1 = texture(u_DiffuseMap, uv);
	vec4 e = smoothstep(vec4(-0.05), vec4(-0.0), vec4(uv, vec2(1) - uv));
	c1 = mix(vec4(1, 1, 1, 0), c1, e.x*e.y*e.z*e.w);
	float d = clamp(dot(c1.xyz, vec3(-.5, 1., -.5)), 0.0, 1.0);
	vec4 c2 = vec4(.7);
	return min(mix(c1, c2, 1.8*d), .7);
}

vec4 getColHT(vec2 pos)
{
	return smoothstep(.95, 1.05, getCol(pos)*.8 + .2 + getRand(pos*.7));
}

float getVal(vec2 pos)
{
	vec4 c = getCol(pos);
	return pow(dot(c.xyz, vec3(.333)), 1.)*1.;
}

vec2 getGrad(vec2 pos, float eps)
{
	vec2 d = vec2(eps, 0);
	return vec2(
		getVal(pos + d.xy) - getVal(pos - d.xy),
		getVal(pos + d.yx) - getVal(pos - d.yx)
	) / eps / 2.;
}

#define AngleNum 3

#define SampNum 6//int(u_Local0.r)//16
#define PI2 6.28318530717959

void Drawing(out vec4 fragColor, in vec2 fragCoord)
{
	vec2 pos = fragCoord + 4.0*sin(Time*1.*vec2(1, 1.7))*Res0.y / 400.;
	vec3 col = vec3(0);
	vec3 col2 = vec3(0);
	float sum = 0.;
	for (int i = 0; i<AngleNum; i++)
	{
		float ang = PI2 / float(AngleNum)*(float(i) + .8);
		vec2 v = vec2(cos(ang), sin(ang));
		for (int j = 0; j<SampNum; j++)
		{
			vec2 dpos = v.yx*vec2(1, -1)*float(j)*Res0.y / 400.;
			vec2 dpos2 = v.xy*float(j*j) / float(SampNum)*.5*Res0.y / 400.;
			vec2 g;
			float fact;
			float fact2;

			for (float s = -1.; s <= 1.; s += 2.)
			{
				vec2 pos2 = pos + s*dpos + dpos2;
				vec2 pos3 = pos + (s*dpos + dpos2).yx*vec2(1, -1)*2.;
				g = getGrad(pos2, .4);
				fact = dot(g, v) - .5*abs(dot(g, v.yx*vec2(1, -1)))/**(1.-getVal(pos2))*/;
				fact2 = dot(normalize(g + vec2(.0001)), v.yx*vec2(1, -1));

				fact = clamp(fact, 0., .05);
				fact2 = abs(fact2);

				fact *= 1. - float(j) / float(SampNum);
				col += fact;
				col2 += fact2*getColHT(pos3).xyz;
				sum += fact2;
			}
		}
	}
	col /= float(SampNum*AngleNum)*.75 / sqrt(Res0.y);
	col2 /= sum;
	col.x *= (.6 + .8*getRand(pos*.7).x);
	col.x = 1. - col.x;
	col.x *= col.x*col.x;

	float r = length(pos - Res0.xy*.5) / Res0.x;
	float vign = 1. - r*r*r;

	fragColor = vec4(vec3(col.x*col2*vign), 1.0);

	//vec2 s=sin(pos.xy*.1/sqrt(Res0.y/400.));
	//vec3 karo=vec3(1);
	//karo-=.5*vec3(.25,.1,.1)*dot(exp(-s*s*80.),vec2(1));
	//fragColor = vec4(vec3(col.x*col2*karo*vign),1);
}


void main()
{
	Drawing(gl_FragColor, var_TexCoords * Res0);
}
#elif 0
//
// SS Snow (and rain) testing
//

float iTime = u_Time;

float hash( const in float n ) {
	return fract(sin(n)*4378.5453);
}

float noise(in vec3 o, in float seed) 
{
	vec3 p = floor(o);
	vec3 fr = fract(o);
		
	float n = p.x + p.y*57.0 + p.z * seed;

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

float SmoothNoise( vec3 p, in float seed )
{
    float f;
    f  = 0.5000*noise( p, seed ); p = m*p*2.02;
    f += 0.2500*noise( p, seed ); 
	
    return f * (1.0 / (0.5000 + 0.2500));
}

void main()
{
	vec3 col = texture(u_DiffuseMap, var_TexCoords).rgb;
	vec4 position = texture(u_PositionMap, var_TexCoords);
	vec3 originalRayDir = -normalize(u_ViewOrigin - position.xyz);
	vec3 vCameraPos = u_ViewOrigin;

//#define __RAIN__
//#define __SNOW__

#ifdef __RAIN__
	// Twelve layers of rain sheets...
	float dis = 1.;
	for (int i = 0; i < 12; i++)
	{
		vec3 plane = vCameraPos + originalRayDir * dis;
		float f = SmoothNoise(vec3(plane.xy * 64.0, plane.z-(iTime*32.0)) * 0.3, 109.0) * 1.75;
		f = clamp(pow(abs(f)*.5, 29.0) * 140.0, 0.00, 1.0);
		vec3 bri = vec3(1.0/*.25*/);
		/*for (int t = 0; t < NUM_LIGHTS; t++)
		{
			vec3 v3 = lightArray[t].xyz - plane.xyz;
			float l = dot(v3, v3);
			l = max(3.0-(l*l * .02), 0.0);
			bri += l * lightColours[t];
		}*/
		col += bri*f*0.25;
		dis += 3.5;
	}
#endif //__RAIN__
#ifdef __SNOW__
	// Twelve layers of snow sheets... 4 is plenty it seems
	float dis = 2.;
	for (int i = 0; i < 4/*12*/; i++)
	{
		vec3 plane = vCameraPos + originalRayDir * dis * 128.0;
		float f = SmoothNoise(vec3(plane.xy, plane.z+(iTime*32.0)) * 0.3, 1009.0) * 1.7;
		f = clamp(pow(abs(f)*.5, 29.0) * 140.0, 0.00, 1.0);
		vec3 bri = vec3(1.0/*.25*/);
		/*for (int t = 0; t < NUM_LIGHTS; t++)
		{
			vec3 v3 = lightArray[t].xyz - plane.xyz;
			float l = dot(v3, v3);
			l = max(3.0-(l*l * .02), 0.0);
			bri += l * lightColours[t];
		}*/
		col += bri*f*4.0;
		dis += 3.5;
	}
#endif //__SNOW__

	col = clamp(col, 0.0, 1.0);

	gl_FragColor = vec4(col, 1.0);
}
#else
//
// SSRTGI
//

/*
uniform float ssrt_thickness;
uniform float near;
//uniform float ssrt_roughness_cutoff;
uniform float ssrt_brdf_cutoff;
uniform float ssrt_ray_offset;
uniform int ssrt_ray_max_steps;
*/

#define ssrt_thickness			-1.0//u_Local0.r
#define ssrt_near				-u_ViewInfo.x//u_Local0.g
#define ssrt_brdf_cutoff		0.0//u_Local0.b
#define ssrt_ray_offset			0.1//u_Local0.a

#define ssrt_ray_max_steps		500//u_Local1.r

#define in_linear_depth			u_ScreenDepthMap
#define in_color				u_DiffuseMap
#define in_lighting				u_GlowMap
#define in_position				u_PositionMap
#define in_normal				u_NormalMap

#define clip_info				vec3(u_ViewInfo.x * u_ViewInfo.y, u_ViewInfo.x - u_ViewInfo.y, u_ViewInfo.x + u_ViewInfo.y)

// TODO: Parallax envmaps
#ifdef FALLBACK_CUBEMAP
uniform samplerCube ssrt_env;
uniform float ssrt_env_exposure;
uniform mat4 ssrt_env_inv_view;
#endif



#define PI    3.14159238265
#define SQRT2 1.41421356237
#define SQRT3 1.73205080756
#define FLT_MAX (float(3.40282347e38))
#define INF (float(FLT_MAX+1))



struct material_t
{
	vec4 color;
	float metallic;
	float roughness;
	vec3 normal;
	float f0;
	vec3 emission;
};



vec3 fresnel_schlick(float cos_d, vec3 f0)
{
    return f0 + (1.0f - f0) * pow(1.0f - cos_d, 5.0f);
}

// This is pre-divided by 4.0f*cos_l*cos_v (normally, that would be in the
// nominator, but it would be divided out later anyway.)
// Uses Schlick-GGX approximation for the geometry shadowing equation.
float geometry_smith(float cos_l, float cos_v, float k)
{
    float mk = 1.0f - k;
    return 0.25f / ((cos_l * mk + k) * (cos_v * mk + k));
}

// Multiplied by pi so that it can be avoided later.
float distribution_ggx(float cos_h, float a)
{
    float a2 = a * a;
    float denom = cos_h * cos_h * (a2 - 1.0f) + 1.0f;
    return a2 / (denom * denom);
}

vec3 brdf(
    vec3 diffuse_light_color,
    vec3 specular_light_color,
    vec3 light_dir,
    vec3 surface_color,
    vec3 view_dir,
    vec3 normal,
    float roughness,
    float f0,
    float metallic
){
    vec3 h = normalize(view_dir + light_dir);
    float cos_l = max(dot(normal, light_dir), 0.0f);
    float cos_v = max(dot(normal, view_dir), 0.0f);
    float cos_h = max(dot(normal, h), 0.0f);
    float cos_d = clamp(dot(view_dir, h), 0.0f, 1.0f);

    vec3 f0_m = mix(vec3(f0), surface_color, metallic);

    float k = roughness + 1.0f;
    k = k * k * 0.125f;

    vec3 fresnel = fresnel_schlick(cos_d, f0_m);
    float geometry = geometry_smith(cos_l, cos_v, k);
    float distribution = distribution_ggx(cos_h, roughness);

    vec3 specular = fresnel * geometry * distribution;

    vec3 kd = (vec3(1.0f) - fresnel) * (1.0f - metallic);

    return (kd * surface_color * diffuse_light_color +
            specular * specular_light_color) * cos_l;
}

// TODO: Fix this, it's probably incorrect. Using distribution breaks
// everything, and since normally distribution_ggx premultiplies by pi, do
// that manually. Also, this must be multiplied by the reflected color manually.
vec3 brdf_reflection(
    vec3 light_dir,
    vec3 surface_color,
    vec3 view_dir,
    vec3 normal,
    float roughness,
    float f0,
    float metallic
){
    float cos_l = max(dot(normal, light_dir), 0.0f);
    float cos_v = max(dot(normal, view_dir), 0.0f);

    vec3 f0_m = mix(vec3(f0), surface_color, metallic);

    float k = roughness + 1.0f;
    k = k * k * 0.125f;

    vec3 fresnel = fresnel_schlick(cos_v, f0_m);
    float geometry = geometry_smith(cos_l, cos_v, k);

    return fresnel * geometry * PI * cos_l;
}



#define RAY_MIN_LEVEL 0
#define RAY_MAX_LEVEL 8000

void step_ray(out vec2 p, inout vec3 s, vec3 d, vec2 sd, vec2 level_size)
{
    p = s.xy * level_size;

    vec2 t2 = (floor(p) + sd - p) / (d.xy * level_size);
    float t = min(t2.x, t2.y);

    s += t * d;
}

float cast_ray(
    in sampler2D linear_depth,
    vec3 origin,
    vec3 dir,
    mat4 proj,
    float ray_offset,
    float ray_steps,
    float thickness,
    float near,
    out vec2 p
){
    float len = (origin.z + dir.z > near) ? (near - origin.z) / dir.z : 1.0f; 

    origin += ray_offset * dir;

    vec3 end = origin + dir * len;

    vec4 ps = proj * vec4(origin, 1.0f);
    vec3 s = ps.xyz / ps.w;

    vec4 pe = proj * vec4(end, 1.0f);
    vec3 e = pe.xyz / pe.w;

    vec3 d = e - s;
    d.z *= 2.0f;
    s.xy = s.xy * 0.5f + 0.5f;

    //ivec2 size = textureSize(linear_depth, 0);

    //int level = RAY_MIN_LEVEL;
    //vec2 level_size = size >> level;
	int level = 0;
	vec2 level_size = vec2(1.0);

    vec3 prev_s = s;

    vec2 sd = 0.5f + 0.501f * vec2(
        d.x < 0 ? -1 : 1,
        d.y < 0 ? -1 : 1
    );

    vec2 depth = vec2(0.0f);

    float hyperbolize_mul = -2.0f * clip_info.x/clip_info.y;
    float hyperbolize_constant = -clip_info.z/clip_info.y;

    while(
        //level >= RAY_MIN_LEVEL &&
        //level <= RAY_MAX_LEVEL &&
        s.z < 1.0f &&
        ray_steps > 0
    ) {
        ray_steps-=1.0f;

        prev_s = s;
        step_ray(p, s, d, sd, level_size);

        // TODO: Make sure sample doesn't fall outside texture,
        // that can cause artifacts
#ifdef DEPTH_INFINITE_THICKNESS
        depth.x = texelFetch(linear_depth, ivec2(p), level).x;
        depth.x = hyperbolize_mul/depth.x + hyperbolize_constant;

        if(max(prev_s.z, s.z) >= depth.x)
        {// Hit
            s = prev_s;
            //level--;
        }
        //else level++; // Miss
#else
        depth = texelFetch(linear_depth, ivec2(p), level).xy;
        depth.y -= thickness;
        depth = hyperbolize_mul/depth + hyperbolize_constant;

        vec2 s_depth = vec2(prev_s.z, s.z);
        if(s_depth.x > s_depth.y) s_depth = s_depth.yx;

        if(s_depth.x < depth.y && s_depth.y > depth.x)
        {// Hit
            s = prev_s;
            //level--;
        }
        //else level++; // Miss
#endif

        //level_size = (size >> level);
    }

    vec3 fade = abs(vec3(prev_s.xy * 2.0f, prev_s.z) - 1.0f);
    // TODO: Come up with a way to make sides black without
    // artifacting when target position has low resolution
    float hit_fade = 1.0f;

    return level == RAY_MIN_LEVEL-1 ?
        (1.0f - max(max(fade.x, fade.y), fade.z)) * hit_fade:
        0.0f;
}



vec3 ssrt_reflection(
    vec3 view,
    in material_t mat,
    vec3 o
){
    //if(mat.roughness > ssrt_roughness_cutoff) return vec3(0);

    ivec2 p = ivec2(gl_FragCoord.xy);

    vec3 d = normalize(reflect(-view, mat.normal));

    vec3 att = brdf_reflection(
        d,
        mat.color.rgb,
        view,
        mat.normal,
        mat.roughness,
        mat.f0,
        mat.metallic
    );

	if (u_Local1.a == 2.0)
	{
		return att;
	}
    
    if(max(max(att.r, att.g), att.b) < ssrt_brdf_cutoff) return vec3(0);
    else
    {
        vec2 tp;
        vec3 intersection;
        float fade = min(
            4.0f*cast_ray(
                in_linear_depth, o, d, u_ProjectionMatrix, ssrt_ray_offset,
                ssrt_ray_max_steps, ssrt_thickness, ssrt_near, tp
            ),
            1.0f
        );

		tp = vec2(tp.x, u_Dimensions.y-tp.y); // glow tex is inverted

#ifdef FALLBACK_CUBEMAP
        vec4 color = texture(
            ssrt_env,
            (ssrt_env_inv_view * vec4(d, 0)).xyz
        ) * ssrt_env_exposure;
        color = mix(color, texelFetch(in_lighting, ivec2(tp), 0), fade);
#else
        if(fade == 0.0f) return vec3(0);
        vec4 color = texelFetch(in_lighting, ivec2(tp), 0) * fade;
#endif

		if (u_Local1.a == 3.0)
		{
			return color.rgb;
		}

        return color.rgb * att;
    }
}

vec3 ssrt_refraction(
    vec3 view,
    in material_t mat,
    vec3 o,
    float eta
){
    //if(mat.roughness > ssrt_roughness_cutoff) return vec3(0);

    ivec2 p = ivec2(gl_FragCoord.xy);

    vec3 d = normalize(refract(view, -mat.normal, eta));

    vec2 tp;
    vec3 intersection;
    float fade = clamp(
        4.0f*cast_ray(
            in_linear_depth, o, d, u_ProjectionMatrix, ssrt_ray_offset,
            ssrt_ray_max_steps, ssrt_thickness, ssrt_near, tp
        ),
        0.0f,
        1.0f
    );

	tp = vec2(tp.x, u_Dimensions.y-tp.y); // glow tex is inverted

#ifdef FALLBACK_CUBEMAP
    vec4 color = texture(
        ssrt_env,
        (ssrt_env_inv_view * vec4(d, 0)).xyz
    ) * ssrt_env_exposure;
    color = mix(color, texelFetch(in_lighting, ivec2(tp), 0), fade);
#else
    if(fade == 0.0f) return vec3(0);
    vec4 color = texelFetch(in_lighting, ivec2(tp), 0) * fade;
#endif
    return color.rgb;
}

void main()
{
	material_t mat;

	/*
	vec4 color;
	float metallic;
	float roughness;
	vec3 normal;
	float f0;
	vec3 emission;
	*/

	mat.color = vec4(texture(in_color, var_TexCoords).rgb, 1.0);
	mat.metallic = 0.5;
	mat.roughness = 0.5;
	mat.normal = DecodeNormal(texture(in_normal, var_TexCoords).xy);
	mat.f0 = 0.5;
	mat.emission = texture(in_lighting, var_TexCoords).rgb;

	vec4 pMap = textureLod(in_position, var_TexCoords, 0.0);
	vec3 view = normalize(u_ViewOrigin.xyz - pMap.xyz);
	//vec3 view = normalize(-pMap.xyz);

	vec3 reflection = ssrt_reflection(view, mat, pMap.xyz);

	if (u_Local1.a >= 1.0)
		gl_FragColor = vec4(reflection.rgb, 1.0);
	else
		gl_FragColor = vec4(mat.color.rgb + reflection.rgb, 1.0);
}

#endif
