uniform sampler2D	u_GlowMap;

uniform vec4		u_PrimaryLightOrigin;
uniform vec3		u_ViewOrigin;
uniform float		u_Time;

in vec3 fWorldPos;
in float playerLookingAtSun;	// the dot of the player looking at the sun - should be the same for all verts
in vec3 var_Position;
in vec3 var_Normal;
in vec3 var_SunDir;
in vec3 var_ViewDir;

// OUT
out vec4 out_Glow;
out vec4 out_Position;
out vec4 out_Normal;

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

#if 1

float tween(float t)
{
	return clamp(t*t*t*(t*(t*6-15)+10),0,1);
}

float sunfade(float t)
{
	return clamp(t*t, 0, 1);
}

float sunglow(float t)
{
	return clamp(t*t*t*t*t, 0, 1);
}

void main()
{
	// to get here, we need to draw a sphere around the player
	// this is the frag shader for that sphere

	//calculated vars - all need to be normalized
	//vec3 SunDir = normalize(u_PrimaryLightOrigin.xyz - (fWorldPos.xyz * u_PrimaryLightOrigin.w));
	//vec3 SunDir = normalize(u_PrimaryLightOrigin.xyz - fWorldPos.xyz);
	vec3 SunDir = normalize(var_SunDir);//normalize(u_PrimaryLightOrigin.xyz - u_ViewOrigin.xyz);
	vec3 FragDir = normalize(var_ViewDir);//normalize(fWorldPos - u_ViewOrigin.xyz);
	vec3 UpDir = normalize(vec3(0.0, 1.0, 0.0)/*playerPos*/);

	float dotSU = dot(SunDir, UpDir);
	float dotSF = dot(SunDir, FragDir);
	float dotFU = dot(FragDir, UpDir);
	
	//colors
	vec3 DaySkyColor =   mix( vec3(0.25, 0.45, 0.80), vec3(0.00f, 0.25f, 0.60f), max(dotFU,0));     // Day sky color
	vec3 NightSkyColor = vec3(0.00f, 0.01f, 0.05f);     // night sky color
	//vec3 TransColor =    vec3(1.00f, 0.46f, 0.00f);     // sunrise/sunset color
	vec3 TransColor =    vec3(1.00f, 0.50f, 0.25f);     // sunrise/sunset color
	vec3 SunColor =      vec3(1.00f, 1.00f, 0.90f);		// color of the sun 
	vec3 SunGlowColor =  vec3(1.00f, 0.99f, 0.87f);		// color of the sun's glow
	vec3 HorizonGlowColor = TransColor;  // the horizon's glow at rise/set
	//vec3 HorizonGlowColor = vec3(1.00f, 0.80f, 0.40f);  // the horizon's glow at rise/set
	  
	// Things that could be done with the colors:
	// 1. Change sun color depending on sun's closeness to horizon (i.e. dotSU closeness to 0)
	// 2. Change all colors depending on elevation (aka thinner atmosphere at heights)

	vec3 tColor = vec3(0.0f, 1.0f, 0.0f);  // temporary result color

	// controls when the sunset/sunrise sky starts
	float Trans =  0.45;  // the closer to 0, the less time for those events
	float Night = -0.10;  // determines when total nighttime occurs
	float NightTransFade = 0.1; // how much before night the sun's glow starts diminishing
	float transBlendFactor  = 0.35;   // 0 < this < 1 - larger makes the sunset/rise glow closer to sky
	float horizonGlow = 0.33;       // the size of the glow of the horizon near sunrise/sunset
	float horizonSunGlow = 0.0;    // how far from the sun the horizon glow extends
	float sunHorizonFade = 0.02;    // how far up from the horizon does the sun blend
	float underHorizonFade = -0.1;  // from horizon to this line, there's a fade - below this line is solid color
	float SkyLitOffset = 0.2;		// determines how much before 'night' the sun gets brighter or stays brighter
	
	// controls how big the sun will appear in the sky
	float SunDisk = 0.999;  // closer to 1 means smaller the disk
	// sun glow is dependant on how close the player is looking at the sun
	float sunGlowScale = mix(0, playerLookingAtSun, tween(clamp( 1 - (0.4 - dotSU)/(0.4 - 0.25), 0, 1)));
	float SunGlow = mix(0.997, 0.940, sunGlowScale);  // closer to SunDisk, smaller the glow (always should be < SunDisk)
	float MoonDisk = -0.9995;  //experimental moon disk

	// controls the size of the reddish glow near the sun at sunrise/sunset
	float SunTransGlow = 0.5;  // smaller num means larger glow (should not exceed SunGlow, and actually be a lot below it)
	
	// determine sky color beforehand
	tColor = mix(NightSkyColor, DaySkyColor, clamp((dotSU - (Night - SkyLitOffset)) / (Trans - (Night - SkyLitOffset)), 0 ,1) );
	
	if (dotSU > Trans)
	{

	}
	else if (dotSU > Night + NightTransFade)
	{  // we're at sunrise/sunset
	  vec3 tBeforeSun = tColor;
	  // also, set the sky glow of the sunrise/sunset
	  if (dotSF > SunTransGlow)
	  {  // the fragment falls inside the glow side
    	// recalc transcolor based on var:
        TransColor = mix(TransColor, tColor, transBlendFactor);
		// blend smooth glow
		vec3 tTransGlowColor = mix(tColor, TransColor, sunfade((dotSF - SunTransGlow) / (1 - SunTransGlow)) );
		// and now blend depending on dotSU so the sunrise/sunset glow doesn't just 'pop' into existence
		tTransGlowColor = mix(tTransGlowColor, tColor, sunfade((dotSU - Night - NightTransFade) / (Trans - Night - NightTransFade)));
		tColor = tTransGlowColor;
	  }
	  // now if fragment is near horizon, add a glow
	  if (abs(dotFU) < horizonGlow)
	  {
	    // first make the horizon glow color less intrusive
	    HorizonGlowColor = mix(HorizonGlowColor, tBeforeSun, 0.4);
	    // then do some fun stuff with it
	    HorizonGlowColor = mix(HorizonGlowColor, tColor, (horizonSunGlow - dotSF)/(1 - horizonSunGlow));
	    HorizonGlowColor = mix(HorizonGlowColor, tColor, (abs(dotFU))/(horizonGlow));
	    HorizonGlowColor = mix(HorizonGlowColor, tColor, abs(dotFU)/(horizonGlow));
	    tColor = mix(HorizonGlowColor, tColor, (dotSU - Night - NightTransFade) / (Trans - Night - NightTransFade));
	  }
	}
	else if (dotSU > Night)
	{  // we're at sunrise/sunset
	  vec3 tBeforeSun = tColor;
	  // also, set the sky glow of the sunrise/sunset
	  if (dotSF > SunTransGlow)
	  {  // the fragment falls inside the glow side
    	// recalc transcolor based on var:
        TransColor = mix(TransColor, tColor, transBlendFactor);
		// blend smooth glow
		vec3 tTransGlowColor = mix(tColor, TransColor, tween((dotSF - SunTransGlow) / (1 - SunTransGlow)) );
		// and now blend depending on dotSU so the sunrise/sunset glow doesn't just 'pop' into existence
		tTransGlowColor = mix(tColor, tTransGlowColor, (dotSU - Night) / (NightTransFade));
		tColor = tTransGlowColor;
	  }
	  // now if fragment is near horizon, add a glow
	  if (abs(dotFU) < horizonGlow)
	  {
	    // first make the horizon glow color less intrusive
	    HorizonGlowColor = mix(HorizonGlowColor, tBeforeSun, 0.4);
	    // then do some fun stuff with it
	    HorizonGlowColor = mix(HorizonGlowColor, tColor, (horizonSunGlow - dotSF)/(1 - horizonSunGlow));
	    HorizonGlowColor = mix(HorizonGlowColor, tColor, (abs(dotFU))/(horizonGlow));
	    HorizonGlowColor = mix(HorizonGlowColor, tColor, abs(dotFU)/(horizonGlow));
	    tColor = mix(tColor, HorizonGlowColor, (dotSU - Night) / (NightTransFade));
	  }
	}
	else
	{  // it's night time
	  // TODO: lighten up near the horizon (and below) (lessening the lightening up closer to night we are)
	}

	// draw the spot of the sun on the sky, only if the fragment is above the 'horizon'
    if (dotFU > 0)
    {
        vec3 tSunColor = tColor;
        if (dotSF > SunDisk)
        {
            tSunColor = SunColor;
        }
        else if (dotSF > SunGlow)
        {
			SunGlowColor = mix(SunColor, SunGlowColor, sunglow((dotSF - SunGlow) / (SunDisk - SunGlow)) );
            tSunColor = mix(tColor, SunGlowColor, sunglow((dotSF - SunGlow) / (SunDisk - SunGlow)));
        }
        else if (dotSF < MoonDisk)
        {
            tSunColor = vec3(0.89, 0.89, 0.89);
        }
        if (dotFU < sunHorizonFade)
            tColor = mix(tColor, tSunColor, dotFU / sunHorizonFade); // performs a mix to blend the sun with the horizon
        else
            tColor = tSunColor;
    }
	else
	{// Fragment is below the horizon line 
	    vec3 darkColor = tColor/2.0;
		if (dotFU > underHorizonFade)
		    tColor = mix(darkColor, tColor, (dotFU - underHorizonFade) / abs(underHorizonFade));	// darken it a bit
		else
		    tColor = darkColor;
	}

	gl_FragColor = vec4(tColor.rgb, 1.0);
	out_Glow = vec4(0.0);
	out_Position = vec4(var_Position.xyz, 1024.0+1.0);
	out_Normal = vec4(EncodeNormal(var_Normal.xyz), 0.0, 1.0);
}


#else

#define iTime u_Time

const float PI = 3.1415926;
const float PI2 = PI *2.0;

vec3 sundir = normalize(u_ViewOrigin - u_PrimaryLightOrigin.xyz);
vec3 haloclr1 = vec3(254., 201., 59.) / 255.;
vec3 haloclr2 = vec3(253., 158., 45.) / 255.;

/************************************
Math
************************************/

vec2 rot(vec2 p, float a)
{
	float s = sin(a); float c = cos(a); return mat2(c, s, -s, c) * p;
}


// by iq
float noise(in vec3 v)
{
	vec3 p = floor(v);
	vec3 f = fract(v);
	f = f*f*(3.0 - 2.0*f);
	vec2 uv = (p.xy + vec2(37.0, 17.0)*p.z) + f.xy;
	vec2 rg = textureLod(u_GlowMap, (uv + 0.5) / 2048.0, 0.0).yx;
	return mix(rg.x, rg.y, f.z);
}

vec3 random3f(vec3 p)
{
	return textureLod(u_GlowMap, (p.xy + vec2(3.0, 1.0)*p.z + 0.5) / 2048.0, 0.0).xyz;
}

// by iq
vec3 voronoi(in vec3 x)
{
	vec3 p = floor(x);
	vec3 f = fract(x);

	float id = 0.0;
	vec2 res = vec2(100.0);
	for (int k = -1; k <= 1; k++)
		for (int j = -1; j <= 1; j++)
			for (int i = -1; i <= 1; i++)
			{
				vec3 b = vec3(float(i), float(j), float(k));
				vec3 r = vec3(b) - f + random3f(p + b);
				float d = dot(r, r);

				if (d < res.x)
				{
					id = dot(p + b, vec3(1.0, 57.0, 113.0));
					res = vec2(d, res.x);
				}
				else if (d < res.y)
				{
					res.y = d;
				}
			}
	return vec3(sqrt(res), abs(id));
}

/************************************
sun & sky & clouds
************************************/

float grow(float x, float bias, float k)
{
	return 1.0 - exp(-abs(x - bias)*k);
}
float fbmCloud(vec3 p)
{
	float a = 0.5;
	p *= 0.5;
	p.x += iTime*0.5;
	p.y += iTime*0.2;
	float f = 1.0;
	float v = 0.0;
	for (int i = 0; i < 5; ++i)
	{
		v += a* noise(p*f);
		a *= 0.5;
		f *= 2.;
	}
	v = max(0.0, v - 0.5);
	return v;
}

float calcDomeRay(vec3 ro, vec3 rd, out vec3 domePos)
{
	float r = 500.0; // skydome radius
	float h = 30.0;   // sky height 
	vec3 o = ro;
	vec3 p = o;  p.y -= r - h; // p : skydome center;
	float a = dot(rd, rd);
	vec3 op = 2.0*o - p;
	float b = dot(op, rd);
	float c = dot(op, op) - r*r;
	float bac = b*b - a*c;
	float t = -1.0;
	if (bac > 0.0)
	{
		t = (-b + sqrt(bac)) / a;
	}
	if (t < 0.0) t = -1.0;
	domePos = ro + rd*t;
	return t;
}

vec3 renderCloud(vec3 ro, vec3 rd, vec3 bg)
{
	float sundot = clamp(dot(sundir, rd), 0., 1.);
	vec3 halo1 = haloclr1 * pow(sundot, 50.0);
	vec3 halo2 = haloclr2 * pow(sundot, 20.0);
	// cloud
	vec4 sum = vec4(bg, 0.0);
	vec3 domePos;
	float domeT;
	if (rd.y > -0.1)
	{
		float domeT = calcDomeRay(ro, rd, domePos);
		if (domeT > -0.5)
		{
			float t = 0.0;
			for (int i = 0; i < 4; ++i)
			{
				vec3 pos = domePos + rd * t;
				float ratio = 0.2;
				float d1 = fbmCloud(pos*ratio);;
				float d2 = fbmCloud((pos*ratio + sundir*1.0));

				float dif = clamp(d1 - d2, 0.0, 1.0);
				// diff lighting
				vec4 clr = vec4(vec3(0.3), 0.0) + vec4(haloclr2 *dif *5.0, d1);
				clr.rgb += halo2*5.0 + halo1 *2.0;				// hack
				clr.w *= exp(-distance(domePos, ro)*.025);	// hack
				clr.rgb = clr.rgb * clr.a;
				sum = sum + clr * (1.0 - sum.a);

				t += 1.0;
			}
		}
	}
	return sum.rgb;
}

vec3 renderAtmosphere(vec3 rd)
{
	vec3 atm = vec3(0.0);
	float ry = max(0.0, rd.y);
	atm.r = mix(0.25, 0., grow(ry, 0.0, 5.0));
	atm.g = mix(0.06, 0., grow(ry, 0.1, 0.5));
	atm.b = mix(0., 0.3, grow(ry, 0., 0.5));
	return atm;
}

vec3 renderSky(vec3 rd)
{
	// sun 
	float sundot = clamp(dot(sundir, rd), 0., 1.);
	vec3 core = vec3(1.) * pow(sundot, 250.0);

	vec3 halo1 = haloclr1 * pow(sundot, 50.0);
	vec3 halo2 = haloclr2 * pow(sundot, 20.0);
	vec3 sun = core + halo1 *0.5 + halo2 *0.9;

	// atm 
	vec3 atm = renderAtmosphere(rd);
	return sun + atm;
}


vec3 renderSkyCloudDome(vec3 ro, vec3 rd)
{
	vec3 sky = renderSky(rd);
	vec3 cloud = renderCloud(ro, rd, sky);
	return cloud;
}

void main()
{
	//vec3 rd = normalize(u_ViewOrigin - var_Position);
	vec3 rd = normalize(var_Position);
	//vec3 ro = vec3(-0.5 + 3.5*cos(PI2 *mx), 0.0 + 2.0*my, 0.5 + 3.5*sin(PI2 *mx));
	//vec3 ro = vec3(1.0, 1.0, 0.0);// normalize(-u_ViewOrigin);
	vec3 ro = vec3(0.0);
	vec3 color = renderSkyCloudDome(ro, rd);
	gl_FragColor = vec4(color.rgb, 1.0);
	out_Glow = vec4(0.0);
	out_Position = vec4(var_Position.xyz, 1024.0 + 1.0);
	out_Normal = vec4(EncodeNormal(var_Normal.xyz), 0.0, 1.0);
}

#endif
