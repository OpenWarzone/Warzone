uniform sampler2D		u_DiffuseMap;

uniform vec2			u_Dimensions;

uniform vec4			u_Local6; // PORTAL_COLOR1
uniform vec4			u_Local7; // PORTAL_COLOR2
uniform vec4			u_Local8; // IMAGE_COLOR
uniform vec4			u_Local9; // tests

#define PORTAL_COLOR1	u_Local6
#define PORTAL_COLOR2	u_Local7
#define IMAGE_COLOR		u_Local8

uniform float	u_Time;

varying vec3	var_Normal;
varying vec3	var_vertPos;
varying vec2    var_TexCoords;

out vec4 out_Glow;
out vec4 out_Position;
out vec4 out_Normal;
#ifdef __USE_REAL_NORMALMAPS__
out vec4 out_NormalDetail;
#endif //__USE_REAL_NORMALMAPS__


#define m_Normal		var_Normal
#define m_TexCoords		var_TexCoords
#define m_vertPos		var_vertPos
#define m_ViewDir		var_ViewDir


#define shaderTime (u_Time * 0.4)

#ifndef __LQ_MODE__
// When not in LQ mode, also add sparks...
#define USE_SPARKS
#endif //__LQ_MODE__

float snoise(vec3 uv, float res)
{
	const vec3 s = vec3(1e0, 1e2, 1e3);
	
	uv *= res;
	
	vec3 uv0 = floor(mod(uv, res))*s;
	vec3 uv1 = floor(mod(uv+vec3(1.), res))*s;
	
	vec3 f = fract(uv); f = f*f*(3.0-2.0*f);

	vec4 v = vec4(uv0.x+uv0.y+uv0.z, uv1.x+uv0.y+uv0.z,
		      	  uv0.x+uv1.y+uv0.z, uv1.x+uv1.y+uv0.z);

	vec4 r = fract(sin(v*1e-1)*1e3);
	float r0 = mix(mix(r.x, r.y, f.x), mix(r.z, r.w, f.x), f.y);
	
	r = fract(sin((v + uv1.z - uv0.z)*1e-1)*1e3);
	float r1 = mix(mix(r.x, r.y, f.x), mix(r.z, r.w, f.x), f.y);
	
	return mix(r0, r1, f.z)*2.-1.;
}

float noise(vec2 uv, float baseres)
{
    float n = 0.0;
#ifdef __LQ_MODE__
	for (int i = 0; i < 2; i++)
#else //!__LQ_MODE__
    for (int i = 0; i < 4; i++)
#endif //__LQ_MODE__
    {
        float v = pow(2.0, float(i));
        n += (1.5 / v) * snoise(vec3(uv + vec2(1.,1.) * (float(i) / 17.), 1), v * baseres);
    }
    
    return clamp((1.0 - n) * .5, 0., 1.) * 2.0;
}

void getPortal(out vec4 fragColor, in vec2 fragCoord)
{
	float aspectRatio = u_Dimensions.y/u_Dimensions.x;
    vec2 uv = (fragCoord/u_Dimensions.xy);
    vec2 inUV = uv;
    uv.y *= aspectRatio;
    
    // Tweaking vars
    vec4 color = vec4(PORTAL_COLOR1.rgb, 1.0);//vec4(0.125, 0.291, 0.923, 1.0);
    vec4 leaving = vec4(PORTAL_COLOR2.rgb, 1.0);//vec4(0.925, 0.791, 0.323, 1.0);
    float noise_sz = 7.0f;
    float speed = 0.4;
    vec2 center = vec2(0.5, 0.5 * aspectRatio);
    
    float dc = 1. - (distance(uv, center) * 2.75);
    
    if (dc <= 0.0)
    {// Skip all calcs, this is outside of the portal area's pixels...
        fragColor = vec4(0.0);
        return;
    }

	float pdc = pow(dc, 3.5);
    
    vec2 dir = -normalize(uv - center) * speed;
    
    float phase0 = fract(shaderTime * 0.3 + 0.5);
    float phase1 = fract(shaderTime * 0.3 + 0.0);
    
    vec2 uv0 = uv + phase0 * dir;
    vec2 uv1 = uv + phase1 * dir;
    
    // Rotation
    float as = pdc * sin(shaderTime * 0.9) * 1.2;
	float ca = cos(as);
	float sa = sin(as);					
    
    mat2 rot;
    rot[0] = vec2(ca, -sa);
    rot[1] = vec2(sa, ca);
    
    uv0 = center + ((uv0 - center) * rot);
    uv1 = center + ((uv1 - center) * rot);

    // Samplings
    float tex0 = max(noise(uv0, noise_sz), noise(uv0 * 1.2, noise_sz));
    float tex1 = max(noise(uv1, noise_sz), noise(uv1 * 1.4, noise_sz));
    
    float lerp = abs((0.5 - phase0) / 0.5);
    float samplings = mix(tex0, tex1, lerp);
    
    vec4 c = vec4(samplings, samplings, samplings, 1.0) * mix(color, leaving, pdc) * pdc;
  	c += pow(dc, 16.0) * mix(color, leaving, pow(dc, 16.0)) * 2.3;
    
    float cl = clamp(max(c.r, max(c.g, c.b)) * 32.0, 0.0, 1.0);
    
	// Blend in portal's target picture...
    c.rgb += (texture(u_DiffuseMap, inUV).rgb * IMAGE_COLOR.rgb) * cl * IMAGE_COLOR.a;
    
    // Output to screen
    fragColor.rgb = c.rgb * 1.5;

	cl = clamp(max(fragColor.r, max(fragColor.g, fragColor.b)), 0.0, 1.0);

	fragColor.a = cl;//1.0 - dc;
}

void main()
{
	getPortal( gl_FragColor, (vec2(1.0)-m_TexCoords) * u_Dimensions.xy );
	out_Glow = vec4(0.0);
	out_Position = vec4(0.0);
	out_Normal = vec4(0.0);
#ifdef __USE_REAL_NORMALMAPS__
	out_NormalDetail = vec4(0.0);
#endif //__USE_REAL_NORMALMAPS__
}
