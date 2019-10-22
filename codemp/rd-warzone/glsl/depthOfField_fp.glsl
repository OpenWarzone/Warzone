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
uniform sampler2D					u_TextureMap;
uniform sampler2D					u_ScreenDepthMap;
#endif //defined(USE_BINDLESS_TEXTURES)

uniform vec2	u_Dimensions;
uniform vec4	u_ViewInfo; // zfar / znear, zfar
uniform vec4	u_Local0; // dofValue, 0, 0, 0

varying vec2		var_TexCoords;

#define PI  3.14159265

float width = u_Dimensions.x; //texture width
float height = u_Dimensions.y; //texture height

vec2 texel = vec2(1.0/width,1.0/height);

const float focalDepth = -0.01581;
const float focalLength = 12.0;
const float fstop = 2.0;
const bool showFocus = false;

/* 
make sure that these two values are the same for your camera, otherwise distances will be wrong.
*/

float znear = u_ViewInfo.x; //camera clipping start
float zfar = u_ViewInfo.y; //camera clipping end

//------------------------------------------
//user variables

const int samples = 4; //samples on the first ring
const int rings = 4; //ring count

const bool manualdof = false; //manual dof calculation

const float ndofstart = 1.0; //near dof blur start
const float ndofdist = 2.0; //near dof blur falloff distance
const float fdofstart = 1.0; //far dof blur start
const float fdofdist = 3.0; //far dof blur falloff distance

const float CoC = 0.03;//circle of confusion size in mm (35mm film = 0.03mm)

const bool vignetting = false; //use optical lens vignetting?
const float vignout = 1.3; //vignetting outer border
const float vignin = 0.0; //vignetting inner border
const float vignfade = 22.0; //f-stops till vignete fades

bool autofocus = false; //use autofocus in shader? disable if you use external focalDepth value
const vec2 focus = vec2(0.5,0.5); // autofocus point on screen (0.0,0.0 - left lower corner, 1.0,1.0 - upper right)
const float maxblur = 1.0; //clamp value of max blur (0.0 = no blur,1.0 default)

const bool blur_distant_only = false; // only blur distant objects, not the objects closer to the camera then the focus point.
const bool blur_less_close = true; // blur objects close to the camera less.

const bool constant_distant_blur = true; // Blur all distant objects.
const float constant_distant_blur_depth = 0.01578; // UQ1: JKA Optimized value.
const float constant_distant_blur_strength = -0.011; // UQ1: JKA Optimized value.

const float threshold = 0.95; //highlight threshold;
const float gain = 100.0; //highlight gain;

const float bias = 0.5; //bokeh edge bias
const float fringe = 0.7; //bokeh chromatic aberration/fringing

const bool noise = false; //use noise instead of pattern for sample dithering
const float namount = 0.0001; //dither amount

const bool depthblur = false; //blur the depth buffer?
const float dbsize = 4.0;//1.25; //depthblursize

/*
next part is experimental
not looking good with small sample and ring count
looks okay starting from samples = 4, rings = 4
*/

const bool pentagon = true; //use pentagon as bokeh shape?
const float feather = 0.4;//3.0;//0.4; //pentagon shape feather

//------------------------------------------


float penta(vec2 coords) //pentagonal shape
{
	const float scale = float(rings) - 1.3;
	const vec4  HS0 = vec4( 1.0,         0.0,         0.0,  1.0);
	const vec4  HS1 = vec4( 0.309016994, 0.951056516, 0.0,  1.0);
	const vec4  HS2 = vec4(-0.809016994, 0.587785252, 0.0,  1.0);
	const vec4  HS3 = vec4(-0.809016994,-0.587785252, 0.0,  1.0);
	const vec4  HS4 = vec4( 0.309016994,-0.951056516, 0.0,  1.0);
	const vec4  HS5 = vec4( 0.0        ,0.0         , 1.0,  1.0);
	
	const vec4  one = vec4( 1.0 );
	
	vec4 P = vec4((coords),vec2(scale, scale)); 
	
	vec4 dist = vec4(0.0);
	float inorout = -4.0;
	
	dist.x = dot( P, HS0 );
	dist.y = dot( P, HS1 );
	dist.z = dot( P, HS2 );
	dist.w = dot( P, HS3 );
	
	dist = smoothstep( -feather, feather, dist );
	
	inorout += dot( dist, one );
	
	dist.x = dot( P, HS4 );
	dist.y = HS5.w - abs( P.z );
	
	dist = smoothstep( -feather, feather, dist );
	inorout += dist.x;
	
	return clamp( inorout, 0.0, 1.0 );
}

float bdepth(vec2 coords) //blurring depth
{
	float d = 0.0;
	const float kernel[9] = float[9](1.0/16.0, 2.0/16.0, 1.0/16.0, 2.0/16.0, 4.0/16.0, 2.0/16.0, 1.0/16.0, 2.0/16.0, 1.0/16.0);
	vec2 offset[9];

	//kernel[0] = 1.0/16.0;   kernel[1] = 2.0/16.0;   kernel[2] = 1.0/16.0;
	//kernel[3] = 2.0/16.0;   kernel[4] = 4.0/16.0;   kernel[5] = 2.0/16.0;
	//kernel[6] = 1.0/16.0;   kernel[7] = 2.0/16.0;   kernel[8] = 1.0/16.0;
	
	vec2 wh = vec2(texel.x, texel.y) * dbsize;
	
	offset[0] = vec2(-wh.x,-wh.y);
	offset[1] = vec2( 0.0, -wh.y);
	offset[2] = vec2( wh.x -wh.y);
	
	offset[3] = vec2(-wh.x,  0.0);
	offset[4] = vec2( 0.0,   0.0);
	offset[5] = vec2( wh.x,  0.0);
	
	offset[6] = vec2(-wh.x, wh.y);
	offset[7] = vec2( 0.0,  wh.y);
	offset[8] = vec2( wh.x, wh.y);
	
	for( int i=0; i<9; i++ )
	{
		float tmp = textureLod(u_ScreenDepthMap, coords + offset[i], 0.0).r;
		d += tmp * kernel[i];
	}
	
	return d;
}


vec3 color(vec2 coords,float blur) //processing the sample
{
	vec3 col = vec3(0.0);
	
	col.r = textureLod(u_TextureMap,coords + vec2(0.0,1.0)*texel*fringe*blur, 0.0).r;
	col.g = textureLod(u_TextureMap,coords + vec2(-0.866,-0.5)*texel*fringe*blur, 0.0).g;
	col.b = textureLod(u_TextureMap,coords + vec2(0.866,-0.5)*texel*fringe*blur, 0.0).b;
	
	const vec3 lumcoeff = vec3(0.299,0.587,0.114);
	float lum = dot(col.rgb, lumcoeff);
	float thresh = max((lum-threshold)*gain, 0.0);
	return col+mix(vec3(0.0),col,thresh*blur);
}

vec2 rand(vec2 coord) //generating noise/pattern texture for dithering
{
	float noiseX = ((fract(1.0-coord.s*(width/2.0))*0.25)+(fract(coord.t*(height/2.0))*0.75))*2.0-1.0;
	float noiseY = ((fract(1.0-coord.s*(width/2.0))*0.75)+(fract(coord.t*(height/2.0))*0.25))*2.0-1.0;
	
	if (noise)
	{
		noiseX = clamp(fract(sin(dot(coord ,vec2(12.9898,78.233))) * 43758.5453),0.0,1.0)*2.0-1.0;
		noiseY = clamp(fract(sin(dot(coord ,vec2(12.9898,78.233)*2.0)) * 43758.5453),0.0,1.0)*2.0-1.0;
	}
	return vec2(noiseX,noiseY);
}

vec3 debugFocus(vec3 col, float blur, float depth)
{
	float edge = 0.002*depth; //distance based edge smoothing
	float m = clamp(smoothstep(0.0,edge,blur),0.0,1.0);
	float e = clamp(smoothstep(1.0-edge,1.0,blur),0.0,1.0);
	
	col = mix(col,vec3(1.0,0.5,0.0),(1.0-m)*0.6);
	col = mix(col,vec3(0.0,0.5,1.0),((1.0-e)-(1.0-m))*0.2);

	return col;
}

float linearize(float depth)
{
#if 0
	return -zfar * znear / (depth * (zfar - znear) - zfar);
#else
	return depth;
#endif
}

float vignette()
{
	float dist = distance(var_TexCoords.xy, vec2(0.5,0.5));
	dist = smoothstep(vignout+(fstop/vignfade), vignin+(fstop/vignfade), dist);
	return clamp(dist,0.0,1.0);
}

void main() 
{
	if (u_Local0.x >= 2.0) autofocus = true;

	//scene depth calculation
	
	float depth;
	
	if (depthblur)
	{
		depth = linearize(bdepth(var_TexCoords.xy));
	}
	else
	{
		depth = linearize(textureLod(u_ScreenDepthMap,var_TexCoords.xy, 0.0).x);
	}
	
	//focal plane calculation
	
	float fDepth = focalDepth;
	
	if (autofocus)
	{
		fDepth = linearize(textureLod(u_ScreenDepthMap,focus, 0.0).x);
	}

	if (!autofocus && depth <= fDepth)
	{
		gl_FragColor.rgb = textureLod(u_TextureMap, var_TexCoords.xy, 0.0).rgb;
		gl_FragColor.a = 1.0;
		return;
	}
	else if (autofocus && blur_distant_only && depth <= fDepth && (constant_distant_blur && 0.0 - depth > constant_distant_blur_depth))
	{
		gl_FragColor.rgb = textureLod(u_TextureMap, var_TexCoords.xy, 0.0).rgb;
		gl_FragColor.a = 1.0;
		return;
	}

	//dof blur factor calculation
	
	float blur = 0.0;
	
	if (manualdof)
	{    
		float a = depth-fDepth; //focal plane
		float b = (a-fdofstart)/fdofdist; //far DoF
		float c = (-a-ndofstart)/ndofdist; //near Dof
		blur = (a>0.0)?b:c;
	}
	else
	{
		float f = focalLength; //focal length in mm
		float d = fDepth*1000.0; //focal plane in mm
		float o = depth*1000.0; //depth in mm
		
		float a = (o*f)/(o-f); 
		float b = (d*f)/(d-f); 
		float c = (d-f)/(d*fstop*CoC); 
		
		blur = abs(a-b)*c;
	}

	if (blur_less_close && depth <= fDepth)
	{
		blur /= 12.0;
	}
	else if (!autofocus)
	{
		float blur_dist = (depth / fDepth);
		//blur *= ((blur_dist + blur_dist) / 1.5);
		float mult = ((blur_dist + blur_dist) / 0.5);
		blur *= (mult / blur_dist);
	}
	else
	{
		float blur_dist = (depth / fDepth);
		blur *= ((blur_dist + blur_dist) / 3.0);
	}

	if (autofocus && constant_distant_blur && 0.0 - depth <= constant_distant_blur_depth)
	{
		float blur2 = 0.5;
		float blur_dist = constant_distant_blur_strength / depth;
		blur2 *= blur_dist;

		if (blur2 > blur) blur = blur2;
	}

	blur = clamp(blur,0.0,1.0);
	
	// calculation of pattern for ditering
	
	vec2 noise = rand(var_TexCoords.xy)*namount*blur;
	
	// getting blur x and y step factor
	
	float w = (1.0/width)*blur*maxblur+noise.x;
	float h = (1.0/height)*blur*maxblur+noise.y;
	
	// calculation of final color
	
	vec3 col = vec3(0.0);
	
	if(blur < 0.05) //some optimization thingy
	{
		col = textureLod(u_TextureMap, var_TexCoords.xy, 0.0).rgb;
	}
	else
	{
		col = textureLod(u_TextureMap, var_TexCoords.xy, 0.0).rgb;
		float s = 1.0;
		int ringsamples;
		
		for (int i = 1; i <= rings; i += 1)
		{   
			ringsamples = i * samples;
			
			for (int j = 0 ; j < ringsamples ; j += 1)   
			{
				float step = PI*2.0 / float(ringsamples);
				float pw = (cos(float(j)*step)*float(i));
				float ph = (sin(float(j)*step)*float(i));
				float p = 1.0;
				if (pentagon)
				{ 
					p = penta(vec2(pw,ph));
				}
				col += color(var_TexCoords.xy + vec2(pw*w,ph*h),blur)*mix(1.0,(float(i))/(float(rings)),bias)*p;  
				s += 1.0*mix(1.0,(float(i))/(float(rings)),bias)*p;   
			}
		}
		col /= s; //divide by sample count
	}
	
	if (showFocus)
	{
		col = debugFocus(col, blur, depth);
	}
	
	if (vignetting)
	{
		col *= vignette();
	}
	
	//gl_FragColor.rgb = textureLod(u_ScreenDepthMap, texcoord, 0.0).rgb;
	gl_FragColor.rgb = col;
	gl_FragColor.a = 1.0;
}
