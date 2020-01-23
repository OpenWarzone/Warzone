#define _PER_PIXEL_NORMAL_
//#define _USING_GEOM_SHADER_

/* Cube and Bumpmaps */
uniform samplerCube u_SkyCubeMap;
uniform samplerCube u_SkyCubeMapNight;
uniform sampler2D   u_NormalMap;

uniform vec4		u_Local9;
uniform vec4		u_Local10;

uniform vec4		u_PrimaryLightOrigin;
uniform vec3		u_PrimaryLightColor;

uniform float		u_Time;

uniform vec3		u_ViewOrigin;

#define numWaves 10

#define WAVE_AMPLITUDE u_Local9.r//1.0//0.01
#define WAVE_LENGTH u_Local9.g//1.0//0.01
#define WAVE_SPEED u_Local9.b//1.0//0.01
#define DAMPING 0.1
#define STEEPNESS 1.0

#define TIME (u_Time * WAVE_SPEED)


struct Wave {
  float freq;  // 2*PI / wavelength
  float amp;   // amplitude
  float phase; // speed * 2*PI / wavelength
  vec2 dir;
};


// Colour of the water depth
const vec3 waterColorShallow = vec3(0.0078, 0.5176, 0.7);
const vec3 waterColorDeep = vec3(0.0059, 0.1276, 0.18);

#define gridColor waterColorShallow

// Primary Light
#define lightColor vec3(1.0)//u_PrimaryLightColor.rgb
#define lightPos u_PrimaryLightOrigin.xyz

// Other stuff
#define eyePos u_ViewOrigin
#define skybox u_SkyCubeMap
#define blinn true


#ifdef _USING_GEOM_SHADER_
in vec3 WorldPos_FS_in;
in vec3 Normal_FS_in;
#define FragPos WorldPos_FS_in
#define Normal Normal_FS_in
#else //!_USING_GEOM_SHADER_
in vec3 FragPos;
in vec3 Normal;
#endif //_USING_GEOM_SHADER_


out vec4 out_Glow;
out vec4 out_Normal;
out vec4 out_Position;
#ifdef USE_REAL_NORMALMAPS
out vec4 out_NormalDetail;
#endif //USE_REAL_NORMALMAPS

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

// R0 is a constant related to the index of refraction (IOR).
// It should be computed on the CPU and passed to the shader.
const float R0 = 0.5;

// This value modifies current fresnel term. If you want to weaken
// reflections use bigger value. If you want to empasize them use
// value smaller then 0. Default is 0.0.
const float refractionStrength = 0.0;

// Function calculating fresnel term.
// - normal - normalized normal vector
// - eyeVec - normalized eye vector
float fresnelTerm(vec3 normal, vec3 eyeVec)
{
#ifdef SIMPLIFIED_FRESNEL
		// Simplified
		return R0 + (1.0f - R0) * pow(1.0f - dot(eyeVec, normal), 5.0f);
#else //!SIMPLIFIED_FRESNEL
		float angle = 1.0f - clamp(dot(normal, eyeVec), 0.0, 1.0);
		float fresnel = angle * angle;
		fresnel = fresnel * fresnel;
		fresnel = fresnel * angle;
		return clamp(fresnel * (1.0 - clamp(R0, 0.0, 1.0)) + R0 - refractionStrength, 0.0, 1.0);
#endif //SIMPLIFIED_FRESNEL
}

float getdiffuse(vec3 n, vec3 l, float p) {
	return pow(dot(n, l) * 0.4 + 0.6, p);
}
float getspecular(vec3 n, vec3 l, vec3 e, float s) {
	float nrm = (s + 8.0) / (3.1415 * 8.0);
	return pow(max(dot(reflect(e, n), l), 0.0), s) * nrm;
}

#ifdef _PER_PIXEL_NORMAL_
vec3 computePartialGerstnerNormal(Wave w, vec3 P, float time)
{
	vec3 N = vec3(0.0, 0.0, 0.0);
	vec2 p = vec2(P.x, P.z);
	float inner = w.freq*dot(w.dir, p) + w.phase * time;
	float WA = w.freq * w.amp;
	float Q = STEEPNESS/(w.freq*w.amp*numWaves);
	//Q = 0.1f;
	N.x = w.dir.x * WA * cos(inner);
	N.z = w.dir.y * WA * cos(inner);
	N.y = Q * WA * sin(inner);
		
	return N;
}
#endif //_PER_PIXEL_NORMAL_

void main()
{
	float ambientStrength = 0.7;
	vec3 ambient = ambientStrength * lightColor;
	
#ifdef _PER_PIXEL_NORMAL_
	Wave W[10] = Wave[]
	(
		Wave( 0.050, 0.20, 1.5, vec2(-5.5, 2.0) ),
		Wave( 0.120, 0.55, 1.3, vec2(-0.7, 0.7) ),
		Wave( 0.2020, 0.01, 0.5, vec2(-1, 0) ),
		Wave( 0.120, 1.15, 1.60, vec2(1.0, 0.20) ),
		Wave( 0.020, 10.25, 1.32, vec2(1.2, 0.10) ),	
		Wave( 0.020, 0.056, 1.53, vec2(1.4, 0.50) ),	
		Wave( 0.120, -0.35, 3.60, vec2(0.50, 2.20) ),	
		Wave( 0.120, -0.05, 0.60, vec2(2.50, -2.20) ),	
		Wave( 0.050, -0.42, 1.60, vec2(-4.0, -1.20) ),	
		Wave( 0.50, 0.01, 2.330, vec2(-2.0, -1.60) )
	);

	vec3 norm = vec3(0.0, 0.0, 0.0);
	for(int i = 0; i < numWaves; i++)
	{
		W[i].freq *= WAVE_LENGTH;
		W[i].amp *= WAVE_AMPLITUDE;
		W[i].phase *= u_Local9.a;
		W[i].dir *= u_Local10.a;
		norm += computePartialGerstnerNormal(W[i], FragPos.xzy, TIME);
	}
	norm.x *= -1.0;
	norm.z *= -1.0;
	norm.y = 1.0 - norm.y;
	norm = normalize(norm).xzy;
#else //!_PER_PIXEL_NORMAL_
	vec3 norm = normalize(Normal);
#endif //_PER_PIXEL_NORMAL_

	vec3 lightDir = normalize(lightPos - FragPos);
	float diff = max(dot(norm, lightDir), 0.1);
	vec3 diffuse = diff * lightColor;
	
	float specularStrength = 1.0;
	vec3 viewDir = normalize(eyePos - FragPos);
	float spec = 0.0;

#if 0
	if (blinn)
	{
		vec3 halfwayDir = normalize(lightDir + viewDir);
		spec = pow(max(dot(norm, halfwayDir), 0.1), u_Local10.r/*128*/);
	}
	else
	{
		vec3 reflectDir = reflect(-lightDir, norm);
		spec = pow(max(dot(viewDir, reflectDir), 0.1), 32);
	}

	vec3 specular = specularStrength * spec * lightColor;
#else
	spec = getspecular(norm, lightDir, viewDir, u_Local10.g);
	vec3 specular = spec * lightColor;

	
	float lambertian2 = dot(lightDir.xyz, norm);
	float spec2 = 0.0;

	if (lambertian2 > 0.0)
	{// this is blinn phong
		const float shininess = 0.7;
		const float specularScale = 0.07;

		float fresnel2 = clamp(1.0 - dot(norm, -viewDir), 0.0, 1.0);
		fresnel2 = pow(fresnel2, 3.0) * 0.65;

		vec3 mirrorEye = (2.0 * dot(viewDir, norm) * norm - viewDir);
		vec3 halfDir2 = normalize(lightDir.xyz + mirrorEye);
		float specAngle = max(dot(halfDir2, norm), 0.0);
		spec2 = pow(specAngle, 16.0);
		specular += vec3(clamp(1.0 - fresnel2, 0.4, 1.0)) * (vec3(spec2 * shininess)) * lightColor * specularScale * 25.0 * u_Local10.b;
	}
	
#endif
	
	float ratio = 1.0 / 1.33;

#if 1
	float fresnel = dot(viewDir, norm);
#else
	float fresnel = fresnelTerm(norm, viewDir);
#endif

	vec3 refSkyDir = reflect(-viewDir, norm);
	vec3 fracSkyDir = refract(-viewDir, norm, ratio);

#if 0
	vec4 refracColor = fresnel * texture(u_SkyCubeMap, fracSkyDir) * vec4(gridColor, 1.0);
	vec4 reflecColor = (1.0 - fresnel) * texture(u_SkyCubeMap, refSkyDir);
#else
	vec4 reflecColor;
	vec4 refracColor;

	vec3 reflectDir = vec3(-refSkyDir.y, -refSkyDir.z, -refSkyDir.x);
	vec3 refractDir = vec3(-fracSkyDir.y, -fracSkyDir.z, -fracSkyDir.x);

#if 0
	if (u_Local6.a > 0.0 && u_Local6.a < 1.0)
	{// Mix between night and day colors...
		vec3 skyColorDay = texture(u_SkyCubeMap, reflectDir).rgb;
		vec3 skyColorNight = texture(u_SkyCubeMapNight, reflectDir).rgb;
		reflecColor = mix(skyColorDay, skyColorNight, clamp(u_Local6.a, 0.0, 1.0));

		skyColorDay = texture(u_SkyCubeMap, refractDir).rgb;
		skyColorNight = texture(u_SkyCubeMapNight, refractDir).rgb;
		refracColor = mix(skyColorDay, skyColorNight, clamp(u_Local6.a, 0.0, 1.0));
	}
	else if (u_Local6.a >= 1.0)
	{// Night only colors...
		reflecColor = texture(u_SkyCubeMapNight, reflectDir).rgb;
		refracColor = texture(u_SkyCubeMapNight, refractDir).rgb;
	}
	else
#endif
	{// Day only colors...
		reflecColor = texture(u_SkyCubeMap, reflectDir);
		refracColor = texture(u_SkyCubeMap, refractDir);
	}

	reflecColor = (1.0 - fresnel) * reflecColor;
	refracColor = fresnel * refracColor * vec4(gridColor, 1.0);
#endif
	
	//vec3 result = (ambient + diffuse + specular) * gridColor;
	//out_Color = vec4(result, 1);
	out_Color = vec4(ambient + diffuse + specular, 1.0) * (reflecColor + refracColor);
	out_Color.a = 1.0;
	out_Glow = vec4(0.0);
	out_Normal = vec4(EncodeNormal(norm), 0.0, 1.0);
#ifdef USE_REAL_NORMALMAPS
	out_NormalDetail = vec4(0.0);
#endif //USE_REAL_NORMALMAPS
	out_Position = vec4(FragPos.xyz, MATERIAL_WATER+1.0);
}
