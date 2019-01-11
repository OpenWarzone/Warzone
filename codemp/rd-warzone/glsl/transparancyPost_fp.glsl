uniform mat4		u_ModelViewProjectionMatrix;

uniform sampler2D	u_DiffuseMap;			// backBufferMap
uniform sampler2D	u_PositionMap;			// position map
uniform sampler2D	u_WaterPositionMap;		// transparancy map

uniform vec4		u_Local0;				// testvalue0, testvalue1, testvalue2, testvalue3
uniform vec2		u_Dimensions;

varying vec2		var_TexCoords;
varying vec3		var_ViewDir;

float hash( const in float n ) {
	return fract(sin(n)*4378.5453);
}

float noise(in vec3 o) 
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

const mat3 m = mat3( 0.00,  0.80,  0.60,
                    -0.80,  0.36, -0.48,
                    -0.60, -0.48,  0.64 );

float SmoothNoise( vec3 p )
{
    float f;
    f  = 0.5000*noise( p ); p = m*p*2.02;
    f += 0.2500*noise( p ); 
	
    return f * (1.0 / (0.5000 + 0.2500));
}

void main ( void )
{
	vec4 transparancyMap = texture(u_WaterPositionMap, var_TexCoords);
	vec3 color = textureLod(u_DiffuseMap, var_TexCoords, 0.0).rgb;

	if (transparancyMap.a == 2.0)
	{// distorted glass...
		vec2 uv = var_TexCoords;
		float factor = pow(SmoothNoise(transparancyMap.xyz * 0.05), 8.0) * 0.2;
		uv += factor / 8.0;
		color.rgb = texture(u_DiffuseMap, uv, factor * 1.5).rgb;
		gl_FragColor = vec4(color, 1.0);
		return;
	}
	else if (transparancyMap.a == 3.0)
	{// distorted push effect...
		vec2 uv = var_TexCoords;
		float factor = pow(transparancyMap.x * 0.5, 1.333);
		uv += (factor / 8.0) * normalize(transparancyMap.zy);
		color.rgb = texture(u_DiffuseMap, uv, factor * 1.5).rgb;
		gl_FragColor = vec4(color, 1.0);
		return;
	}
	else if (transparancyMap.a == 4.0)
	{// distorted pull effect...
		vec2 uv = var_TexCoords;
		float factor = pow(transparancyMap.x * 0.5, 1.333);
		uv += (factor / 8.0) * normalize(transparancyMap.zy);
		color.rgb = texture(u_DiffuseMap, uv, factor * 1.5).rgb;
		gl_FragColor = vec4(color, 1.0);
		return;
	}
	else if (transparancyMap.a == 5.0)
	{// cloak...
		const vec3 keyColor = vec3(0.051,0.639,0.149);

		vec2 uv = var_TexCoords;
		vec3 colorDelta = transparancyMap.xzy/*texture(u_DetailMap, uv).rgb*/ - keyColor.rgb;
		float factor = length(colorDelta) * u_Local0.r;
		uv += (factor * colorDelta.rb) / 8.0;
		color.rgb = texture(u_DiffuseMap, uv, factor * 1.5).rgb;

		gl_FragColor = vec4(color, 1.0);
		return;
	}

	gl_FragColor = vec4(color, 1.0);
}
