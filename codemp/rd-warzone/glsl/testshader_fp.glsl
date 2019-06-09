uniform sampler2D	u_DiffuseMap; // screen/texture map
uniform sampler2D	u_ScreenDepthMap;
uniform sampler2D	u_PositionMap;
uniform sampler2D	u_SpecularMap; // color random image

uniform vec4		u_ViewInfo;
uniform vec2		u_Dimensions;
uniform vec4		u_Local0; // r_testvalue's
uniform vec4		u_Local1; // r_testshadervalue's


uniform vec4		u_PrimaryLightOrigin;
uniform vec3		u_PrimaryLightColor;

// Position of the camera
uniform vec3		u_ViewOrigin;
uniform float		u_Time;

varying vec2		var_TexCoords;

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
	//vec3 originalRayDir = -normalize(position.xyz);
	vec3 vCameraPos = u_ViewOrigin;//normalize(u_ViewOrigin);//vec3(0.0);//u_ViewOrigin;

//#define __RAIN__
#define __SNOW__

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

vec3 HaarmPeterDuikerFilmicToneMapping(in vec3 x)
{
	x = max( vec3(0.0), x - 0.004 );
	return pow( abs( ( x * ( 6.2 * x + 0.5 ) ) / ( x * ( 6.2 * x + 1.7 ) + 0.06 ) ), vec3(2.2) );
}

vec3 CustomToneMapping(in vec3 x)
{
	const float A = 0.665f;
	const float B = 0.09f;
	const float C = 0.004f;
	const float D = 0.445f;
	const float E = 0.26f;
	const float F = 0.025f;
	const float G = 0.16f;//0.145f;
	const float H = 1.1844f;//1.15f;

    // gamma space or not?
	return (((x*(A*x+B)+C)/(x*(D*x+E)+F))-G) / H;
}

vec3 ColorFilmicToneMapping(in vec3 x)
{
	// Filmic tone mapping
	const vec3 A = vec3(0.55f, 0.50f, 0.45f);	// Shoulder strength
	const vec3 B = vec3(0.30f, 0.27f, 0.22f);	// Linear strength
	const vec3 C = vec3(0.10f, 0.10f, 0.10f);	// Linear angle
	const vec3 D = vec3(0.10f, 0.07f, 0.03f);	// Toe strength
	const vec3 E = vec3(0.01f, 0.01f, 0.01f);	// Toe Numerator
	const vec3 F = vec3(0.30f, 0.30f, 0.30f);	// Toe Denominator
	const vec3 W = vec3(2.80f, 2.90f, 3.10f);	// Linear White Point Value
	const vec3 F_linearWhite = ((W*(A*W+C*B)+D*E)/(W*(A*W+B)+D*F))-(E/F);
	vec3 F_linearColor = ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-(E/F);

    // gamma space or not?
	return pow(clamp(F_linearColor * 1.25 / F_linearWhite, 0.0, 1.0), vec3(1.25));
}

#define sphericalAmount u_Local0.g/*1.0*/ //[0.0:2.0] //-Amount of spherical tonemapping applied...sort of

vec3 SphericalPass( vec3 color )
{
	vec3 signedColor = clamp(color.rgb, 0.0, 1.0) * 2.0 - 1.0;
	vec3 sphericalColor = sqrt(vec3(1.0) - signedColor.rgb * signedColor.rgb);
	sphericalColor = sphericalColor * 0.5 + 0.5;
	sphericalColor *= color.rgb;
	color.rgb += sphericalColor.rgb * sphericalAmount;
	color.rgb *= 0.95;
	return color;
}

void main()
{
	vec3 col = texture(u_DiffuseMap, var_TexCoords).rgb;
	if (u_Local0.r == 1)
		col = HaarmPeterDuikerFilmicToneMapping(col);
	if (u_Local0.r == 2)
		col = CustomToneMapping(col);
	if (u_Local0.r == 3)
		col = ColorFilmicToneMapping(col);
	if (u_Local0.r == 4)
		col = SphericalPass(col);

	gl_FragColor = vec4(col, 1.0);
}
#endif
