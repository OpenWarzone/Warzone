uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_NormalMap;
uniform sampler2D	u_OverlayMap; // Real normals. Alpha channel 1.0 means enabled...

uniform vec4		u_Settings0;
uniform vec2		u_Dimensions;

varying vec2		var_TexCoords;


#define _FAST_NORMAL_DETAIL_

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

float getHeight(vec2 uv) {
  return length(texture(u_DiffuseMap, uv).rgb) / 3.0;
}

#ifdef _FAST_NORMAL_DETAIL_
vec4 normalVector(vec3 color) {
	vec4 normals = vec4(color.rgb, length(color.rgb) / 3.0);
	normals.rgb = vec3(length(normals.r - normals.a), length(normals.g - normals.a), length(normals.b - normals.a));

	// Contrast...
//#define normLower ( 128.0 / 255.0 )
//#define normUpper (255.0 / 192.0 )
#define normLower ( 32.0 / 255.0 )
#define normUpper (255.0 / 212.0 )
	vec3 N = clamp((clamp(normals.rgb - normLower, 0.0, 1.0)) * normUpper, 0.0, 1.0);

	return vec4(vec3(1.0) - (normalize(pow(N, vec3(4.0))) * 0.5 + 0.5), 1.0 - normals.a);
}
#else //!_FAST_NORMAL_DETAIL_
vec4 bumpFromDepth(vec2 uv, vec2 resolution, float scale) {
  vec2 step = 1. / resolution;
    
  float height = getHeight(uv);
    
  vec2 dxy = height - vec2(
      getHeight(uv + vec2(step.x, 0.)), 
      getHeight(uv + vec2(0., step.y))
  );

  vec3 N = vec3(dxy * scale / step, 1.);

// Contrast...
#define normLower ( 128.0/*48.0*/ / 255.0 )
#define normUpper (255.0 / 192.0/*128.0*/ )
  N = clamp((clamp(N - normLower, 0.0, 1.0)) * normUpper, 0.0, 1.0);

  return vec4(normalize(N) * 0.5 + 0.5, height);
}

vec4 normalVector(vec2 coord) {
	return bumpFromDepth(coord, u_Dimensions, 0.1 /*scale*/);
}
#endif //_FAST_NORMAL_DETAIL_


vec3 TangentFromNormal ( vec3 normal )
{
	vec3 tangent;
	vec3 c1 = cross(normal, vec3(0.0, 0.0, 1.0)); 
	vec3 c2 = cross(normal, vec3(0.0, 1.0, 0.0)); 

	if( length(c1) > length(c2) )
	{
		tangent = c1;
	}
	else
	{
		tangent = c2;
	}

	return normalize(tangent);
}

void main(void)
{
	vec3 norm = texture(u_NormalMap, var_TexCoords).xyz;
	norm.xyz = DecodeNormal(norm.xy);

	if (u_Settings0.r == 2.0 || u_Settings0.r >= 4.0)
	{
#ifdef USE_REAL_NORMALMAPS
		vec4 normalDetail = textureLod(u_OverlayMap, var_TexCoords, 0.0);

		if (normalDetail.a < 1.0)
		{// Don't have real normalmap, make normals for this pixel...
#ifdef _FAST_NORMAL_DETAIL_
			vec3 color = textureLod(u_DiffuseMap, var_TexCoords, 0.0).rgb;
			normalDetail = normalVector(color);
#else //!_FAST_NORMAL_DETAIL_
			normalDetail = normalVector(var_TexCoords);
#endif //_FAST_NORMAL_DETAIL_
		}
#else //!USE_REAL_NORMALMAPS
#ifdef _FAST_NORMAL_DETAIL_
		vec3 color = textureLod(u_DiffuseMap, var_TexCoords, 0.0).rgb;
		vec4 normalDetail = normalVector(color);
#else //!_FAST_NORMAL_DETAIL_
		vec4 normalDetail = normalVector(var_TexCoords);
#endif //_FAST_NORMAL_DETAIL_
#endif //USE_REAL_NORMALMAPS

		normalDetail.rgb = normalize(clamp(normalDetail.rgb, 0.0, 1.0) * 2.0 - 1.0);
		//normalDetail.rgb *= 0.25;//u_Settings0.g;
		//norm.rgb = normalize(norm.rgb + normalDetail.rgb);
		norm.rgb = normalize(mix(norm.rgb, normalDetail.rgb, 0.25 * (length((norm.rgb * 0.5 + 0.5) - (normalDetail.rgb * 0.5 + 0.5)) / 3.0)));
	}

	if (u_Settings0.r >= 3.0)
	{
		// Gives a better overview of the seamlessness of the smooth normals, at the cost of not being able to see the difference between negative and positive directions...
		gl_FragColor = vec4(length(norm.r), length(norm.g), length(norm.b), 1.0);
	}
	else
	{
		// The standard way to show normals...
		gl_FragColor = vec4(norm.rgb * 0.5 + 0.5, 1.0);
	}
}