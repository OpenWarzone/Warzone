uniform sampler2D	u_DiffuseMap;

uniform vec2		u_Dimensions;

uniform vec4		u_Local7; // MOON_ROTATION_RATE, MOON_ATMOSPHERE_COLOR[0], MOON_ATMOSPHERE_COLOR[1], MOON_ATMOSPHERE_COLOR[2]
uniform vec4		u_Local8; // MOON_COLOR[0], MOON_COLOR[1], MOON_COLOR[2], MOON_GLOW_STRENGTH
uniform vec4		u_Local9; // testvalues

uniform float		u_Time;

varying vec3		var_Normal;
varying vec3		var_vertPos;
varying vec2		var_TexCoords;


out vec4 out_Glow;
out vec4 out_Position;
out vec4 out_Normal;
#ifdef USE_REAL_NORMALMAPS
out vec4 out_NormalDetail;
#endif //USE_REAL_NORMALMAPS

#define m_Normal		var_Normal
#define m_TexCoords		var_TexCoords
#define m_vertPos		var_vertPos
#define m_ViewDir		var_ViewDir

#define time (u_Time*u_Local7.r)

#define X c += texture(u_DiffuseMap, p*.1 - time*.002); p *= .4; c *= .57;

void main()
{
	vec2 p = u_Dimensions.xy;
	vec2 w = m_TexCoords.xy * u_Dimensions.xy;
	float d = length(p = (w.xy*2.-p) / p.y*1.25);
    
	vec4 b = vec4(u_Local8.rgb, 1.0); // Planet coloring...
    vec4 c = b+b;
    
	p = p * asin(d) / d + 5.;    
    p = p * p.y + time;
    
	X X X X X // ;)
	
	gl_FragColor = clamp((c.g+(b-c.g)*c.r) * (1.5-d*d), 0.0, 1.0);

	if (d <= 0.9999)
	{// Moon...
		gl_FragColor.a = 1.0;
		out_Glow = vec4(0.0);
		out_Position = vec4(0.0);
		out_Normal = vec4(0.0);
#ifdef USE_REAL_NORMALMAPS
		out_NormalDetail = vec4(0.0);
#endif //USE_REAL_NORMALMAPS
	}
	else if (d > 0.9)
    {// Atmosphere...
        float atmos = clamp(1.5-d*d, 0.0, 1.0);
        d -= 0.9;
        d *= 7.0;
        d += 0.3;
     	gl_FragColor = mix(vec4(vec3(atmos)*u_Local7.bga, 1.0), vec4(0.0), d);
		out_Glow = vec4(0.0);
		out_Position = vec4(0.0);
		out_Normal = vec4(0.0);
#ifdef USE_REAL_NORMALMAPS
		out_NormalDetail = vec4(0.0);
#endif //USE_REAL_NORMALMAPS
    }
	else
	{// Space (zero alpha)...
		gl_FragColor = vec4(0.0);
		out_Glow = vec4(0.0);
		out_Position = vec4(0.0);
		out_Normal = vec4(0.0);
#ifdef USE_REAL_NORMALMAPS
		out_NormalDetail = vec4(0.0);
#endif //USE_REAL_NORMALMAPS
	}
}
