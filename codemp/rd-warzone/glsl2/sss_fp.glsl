uniform sampler2D			u_DiffuseMap;			// Screen
uniform sampler2D			u_PositionMap;			// Positions
uniform sampler2D			u_NormalMap;			// Normals
uniform sampler2D			u_ScreenDepthMap;		// Depth
uniform sampler2D			u_DeluxeMap;			// Noise

uniform mat4				u_ModelViewProjectionMatrix;
uniform mat4				u_ModelViewMatrix;
uniform mat4				u_ProjectionMatrix;

uniform vec4				u_ViewInfo;				// znear, zfar, zfar / znear, fov
uniform vec2				u_Dimensions;
uniform vec3				u_ViewOrigin;
uniform vec4				u_PrimaryLightOrigin;

uniform vec2				u_vlightPositions;

uniform vec4				u_Local0;				// xx, xx, SUN_SCREEN_POSITION[0], SUN_SCREEN_POSITION[1]
uniform vec4				u_Local1;				// r_testvalue0->value, r_testvalue1->value, r_testvalue2->value, r_testvalue3->value
uniform vec4				u_Local2;				// MAP_INFO_SIZE[2], MAP_INFO_MINS[2], MAP_INFO_MAXS[2]

varying vec2   				var_TexCoords;

#define znear				u_ViewInfo.r			// camera clipping start
#define zfar				u_ViewInfo.g			// camera clipping end

vec2 px = vec2(1.0) / u_Dimensions;

//#define __ENCODE_NORMALS_RECONSTRUCT_Z__
#define __ENCODE_NORMALS_STEREOGRAPHIC_PROJECTION__
//#define __ENCODE_NORMALS_CRY_ENGINE__
//#define __ENCODE_NORMALS_EQUAL_AREA_PROJECTION__

#ifdef __ENCODE_NORMALS_STEREOGRAPHIC_PROJECTION__
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
#elif defined(__ENCODE_NORMALS_CRY_ENGINE__)
vec3 DecodeNormal(in vec2 N)
{
	vec2 encoded = N * 4.0 - 2.0;
	float f = dot(encoded, encoded);
	float g = sqrt(1.0 - f * 0.25);
	return vec3(encoded * g, 1.0 - f * 0.5);
}
vec2 EncodeNormal(in vec3 N)
{
	float f = sqrt(8.0 * N.z + 8.0);
	return N.xy / f + 0.5;
}
#elif defined(__ENCODE_NORMALS_EQUAL_AREA_PROJECTION__)
vec2 EncodeNormal(vec3 n)
{
	float f = sqrt(8.0 * n.z + 8.0);
	return n.xy / f + 0.5;
}
vec3 DecodeNormal(vec2 enc)
{
	vec2 fenc = enc * 4.0 - 2.0;
	float f = dot(fenc, fenc);
	float g = sqrt(1.0 - f / 4.0);
	vec3 n;
	n.xy = fenc*g;
	n.z = 1.0 - f / 2.0;
	return n;
}
#else //__ENCODE_NORMALS_RECONSTRUCT_Z__
vec3 DecodeNormal(in vec2 N)
{
	vec3 norm;
	norm.xy = N * 2.0 - 1.0;
	norm.z = sqrt(1.0 - dot(norm.xy, norm.xy));
	return norm;
}
vec2 EncodeNormal(vec3 n)
{
	return vec2(n.xy * 0.5 + 0.5);
}
#endif //__ENCODE_NORMALS_RECONSTRUCT_Z__

#if 0

#define BLUR_SIZE 0.0//3.0
#define BLUR_STEP 1.0//1.5

#define SHADOW_TOLERANCE 0.5

#define MAP_HEIGHT	u_Local2.r
#define MAP_MINS	u_Local2.g
#define MAP_MAXS	u_Local2.b


vec3 rescale(vec3 values, float new_min, float new_max)
{
	vec3 ret;
    float old_min = values.x;
	float old_max = values.y;

	ret.x = (new_max - new_min) / (old_max - old_min) * (values.x - old_min) + new_min;
	ret.y = (new_max - new_min) / (old_max - old_min) * (values.y - old_min) + new_min;
	ret.z = (new_max - new_min) / (old_max - old_min) * (values.z - old_min) + new_min;

	return ret;
}

float getHeightValue(vec2 coords)
{
	float n = 0.33;
	//return max(n, texture(u_DiffuseMap, coords).x);
	//return length(texture(u_DiffuseMap, coords).xyz) / 3.0;

	if (coords.x < 0.0 || coords.x > 1.0) return 0.0;
	if (coords.y < 0.0 || coords.y > 1.0) return 0.0;
	//return max(n, 1.0 - textureLod(u_ScreenDepthMap, coords, 0.0).x);

	//float height = textureLod(u_PositionMap, coords, 0.0).z;
	//float mapHeight = MAP_MAXS - MAP_MINS;
	//float addHeight = mapHeight - MAP_MAXS;

	//float height = (textureLod(u_PositionMap, coords, 0.0).z / MAP_HEIGHT) * 0.5 + 0.5;

	float height = textureLod(u_PositionMap, coords, 0.0).z;
	height = rescale(vec3(MAP_MINS, MAP_MAXS, height), 0.0, 1.0).z * 0.5 + 0.5;
	if (u_Local1.g > 0.0) height = 1.0 - height;
	if (u_Local1.b > 0.0) height = max(n, height);
	return height;
}

float get_shadow(vec3 n, vec3 l)
{
	return max(0.0, dot(n,l));
}

float shadow(vec3 wPos, vec3 lVector, float NdL)
{
	float bias = 0.01;
	vec3 p;
	float shadow = 1.0;

	if (NdL > 0.0)
	{
		for (float i = 0.0; i <= 1.0; i += 0.01) 
		{
			p = wPos + lVector * i;
			float h = getHeightValue(vec2(p.x, p.y));
            
			float diff = clamp(pow(p.z/h, u_Local1.a), 0.0, 1.0);

			shadow *= diff;	
                
			/*if (p.z < h - bias)
			{
				shadow = 0.0;
				break;
			}*/
		}
	}
	return shadow;
}

vec3 getNormal(vec2 coords, float intensity)
{
#if 0
	vec3 a = vec3(coords.x - px.x, 0.0, getHeightValue(vec2(coords.x - px.x, coords.y)) * intensity);
    vec3 b = vec3(coords.x + px.x, 0.0, getHeightValue(vec2(coords.x + px.x, coords.y)) * intensity);
    vec3 c = vec3(0.0, coords.y + px.y, getHeightValue(vec2(coords.x, coords.y + px.y)) * intensity);
    vec3 d = vec3(0.0, coords.y - px.y, getHeightValue(vec2(coords.x, coords.y - px.y)) *intensity);

	return normalize(cross(b-a, c-d));
#else
	vec4 norm = textureLod(u_NormalMap, coords, 0.0);
	norm.xyz = DecodeNormal(norm.xy);
	return norm.xyz;
#endif
}

vec4 SSS( in vec2 fragCoord )
{
	vec3 sceneColor = texture(u_DiffuseMap, fragCoord.xy).rgb;
	vec4 position = textureLod(u_PositionMap, fragCoord, 0.0).xyzw;

	if (position.a-1.0 == MATERIAL_SKY || position.a-1.0 == MATERIAL_SUN)
	{// Skybox... Skip...
		if (u_Local1.r <= 0.0 && u_Local1.g <= 0.0 && u_Local1.b <= 0.0 && u_Local1.a <= 0.0)
			return vec4(sceneColor, 1.0);
	}

	// Normalized coords and aspect ratio correction
	vec2 uv = fragCoord.xy;
	//float aspect = 1.0;
	//uv.x *= aspect;

	float normalIntensity = u_Local1.a;

	//Light
	vec2 daLight = u_Local0.ba;
	daLight -= uv;
	//vec3 lightVector = normalize(vec3(daLight.x, daLight.y, SHADOW_TOLERANCE));
	vec3 lightVector = normalize(u_PrimaryLightOrigin.xyz - u_ViewOrigin);


	if (u_Local1.r >= 3)
	{
		float h = getHeightValue(uv);
		return vec4(h, h, h, 1.0);
	}
	else if (u_Local1.r >= 2)
	{
		return vec4(lightVector.rgb * 0.5 + 0.5, 1.0);
	}
	else if (u_Local1.r >= 1)
	{
		vec3 normal = getNormal(uv, normalIntensity/*0.2*/);
		return vec4(normal.rgb * 0.5 + 0.5, 1.0);
	}

	float shadowValue = 0.0;
    float shadowNum = 0.0;
    
    for (float x = -BLUR_SIZE; x <= BLUR_SIZE; x+=BLUR_STEP)
    {
     	for (float y = -BLUR_SIZE; y <= BLUR_SIZE; y+=BLUR_STEP)
    	{   
            vec2 thisuv = uv;
			//thisuv.x *= aspect;
    		thisuv += px * vec2(x,y);
            
            float weight = 1.0 - clamp(length(vec2(x,y)) / (BLUR_SIZE*BLUR_SIZE), 0.2, 1.0);
            
            if (thisuv.x < 0.0 || thisuv.x > 1.0 || thisuv.y < 0.0 || thisuv.y > 1.0)
            {
                shadowValue += 1.0 * weight;
            	shadowNum += weight;
                continue;
            }
   
   		 	vec3 worldPos = vec3(vec2(thisuv), getHeightValue(thisuv));
   		 	vec3 normal = getNormal(thisuv, normalIntensity/*0.2*/);
    
    		float NdotL = max(dot(normal, lightVector), 0.0);
    
    		float thisShadowValue = get_shadow(normal, lightVector)*0.7;
    		thisShadowValue *= shadow(worldPos, lightVector, NdotL);
    
    		float ambient = get_shadow(normal, vec3(0.0,0.0,1.0))*0.3;
    		thisShadowValue += ambient;
            
            shadowValue += thisShadowValue * weight;
            shadowNum += weight;
        }
    }
    
    shadowValue /= shadowNum;

	vec3 color = sceneColor * clamp(2.0 * shadowValue, 0.0, 1.0);

	return vec4(color, 1.0);
}

#else
vec4 SSS(in vec2 fragCoord)
{
	vec3 sceneColor = texture(u_DiffuseMap, fragCoord.xy).rgb;
	float originalDepth = texture(u_ScreenDepthMap, fragCoord.xy).r;

	if (originalDepth == 1.0)
	{// Ignore sky...
		return vec4(sceneColor, 1.0);
	}

	//const int samples = 20;
	float occPower = 0.1;// u_Local1.r;
	float maxAllow = 0.0015;// u_Local1.g; // 0.00007
	float minAllow = 0.000015;// u_Local1.b;
	int samples = 200;// int(u_Local1.a);

	//vec2 dir = -(fragCoord.xy - u_vlightPositions.xy) / float(samples);
	vec2 dir = (fragCoord.xy - vec2(fragCoord.x, 0.0)) / float(samples);
	
	float occlusion = 1.0;

	for (int samp = 1; samp <= samples; samp++)
	{
		vec2 coord = fragCoord.xy + (dir * samp);

		if (coord.x < 0.0 || coord.x > 1.0 || coord.y < 0.0 || coord.y > 1.0)
		{
			continue;
		}

		float depth = texture(u_ScreenDepthMap, coord).r;

		if (depth <= originalDepth)
		{// This is between us and the light...
			float diff = length(depth - originalDepth) + 0.00001;

			if (diff <= maxAllow && diff >= minAllow)
			{
				//float dist = (diff - minAllow) / maxAllow;
				//float dist2 = 1.0 - (diff / minAllow);
				float center = ((minAllow + maxAllow) / 2.0);
				float dist = 1.0 - (length(diff - center) / center);
				float thisOcclusion = 1.0 - (pow(diff, occPower) * dist);
				occlusion = min(occlusion, thisOcclusion);
			}
		}
	}

	sceneColor *= occlusion;

	return vec4(sceneColor, 1.0);
}
#endif

void main() 
{
	gl_FragColor = SSS(var_TexCoords);
}
