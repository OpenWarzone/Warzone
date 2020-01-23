#define THREE_WAY_MIST_CLUMPS // 3 way probably gives better coverage, at extra cost... otherwise 2 way X shape... 

#define MAX_FOLIAGES					85

layout(triangles) in;
//layout(triangles, invocations = 8) in;
layout(triangle_strip, max_vertices = MAX_FOLIAGES) out;

uniform mat4								u_ModelViewProjectionMatrix;

uniform vec4								u_Local1; // MAP_SIZE, sway, overlaySway, materialType
uniform vec4								u_Local2; // hasSteepMap, hasWaterEdgeMap, haveNormalMap, SHADER_WATER_LEVEL
uniform vec4								u_Local3; // hasSplatMap1, hasSplatMap2, hasSplatMap3, hasSplatMap4
uniform vec4								u_Local8; // MIST_SURFACE_MINIMUM_SIZE, MIST_LOD_START_RANGE, MIST_HEIGHT, MIST_SURFACE_SIZE_DIVIDER
uniform vec4								u_Local9; // testvalue0, 1, 2, 3
uniform vec4								u_Local10; // MIST_DISTANCE, TERRAIN_TESS_OFFSET, MIST_DENSITY, MIST_MAX_SLOPE

#define SHADER_MAP_SIZE						u_Local1.r
#define SHADER_SWAY							u_Local1.g
#define SHADER_OVERLAY_SWAY					u_Local1.b
#define SHADER_MATERIAL_TYPE				u_Local1.a

#define SHADER_WATER_LEVEL					u_Local2.a

#define SHADER_HAS_SPLATMAP1				u_Local3.r
#define SHADER_HAS_SPLATMAP2				u_Local3.g
#define SHADER_HAS_SPLATMAP3				u_Local3.b
#define SHADER_HAS_SPLATMAP4				u_Local3.a

#define MIST_SURFACE_MINIMUM_SIZE			u_Local8.r
#define MIST_LOD_START_RANGE				u_Local8.g
#define MIST_HEIGHT							u_Local8.b
#define MIST_SURFACE_SIZE_DIVIDER			u_Local8.a

#define MIST_DISTANCE						u_Local10.r
#define TERRAIN_TESS_OFFSET					u_Local10.g
#define MIST_DENSITY						u_Local10.b
#define MIST_MAX_SLOPE						u_Local10.a

#define MAP_WATER_LEVEL						SHADER_WATER_LEVEL // TODO: Use water map

uniform vec3								u_ViewOrigin;

uniform float								u_Time;


smooth out vec2								vTexCoord;
smooth out vec3								vVertPosition;
flat out vec3								vBasePosition;
//smooth out vec2								vVertNormal;

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

mat4 rotationMatrix(vec3 axis, float angle) 
{ 
    axis = normalize(axis); 
    float s = sin(angle); 
    float c = cos(angle); 
    float oc = 1.0 - c; 
     
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0, 
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0, 
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0, 
                0.0,                                0.0,                                0.0,                                1.0); 
}


const float PIover180 = 3.1415/180.0; 

const vec3 vBaseDir[] = vec3[] (
	vec3(1.0, 0.0, 0.0),
#ifdef THREE_WAY_MIST_CLUMPS
	vec3(float(cos(45.0*PIover180)), float(sin(45.0*PIover180)), 0.0f),
	vec3(float(cos(-45.0*PIover180)), float(sin(-45.0*PIover180)), 0.0f)
#else //!THREE_WAY_MIST_CLUMPS
	vec3(float(cos(90.0*PIover180)), float(sin(90.0*PIover180)), 0.0f)
#endif //THREE_WAY_MIST_CLUMPS
);

vec3 vLocalSeed;

// This function returns random number from zero to one
float randZeroOne()
{
	uint n = floatBitsToUint(vLocalSeed.y * 214013.0 + vLocalSeed.x * 2531011.0 + vLocalSeed.z * 141251.0);
	n = n * (n * n * 15731u + 789221u);
	n = (n >> 9u) | 0x3F800000u;

	float fRes = 2.0 - uintBitsToFloat(n);
	vLocalSeed = vec3(vLocalSeed.x + 147158.0 * fRes, vLocalSeed.y*fRes + 415161.0 * fRes, vLocalSeed.z + 324154.0*fRes);
	return fRes;
}

int randomInt(int min, int max)
{
	float fRandomFloat = randZeroOne();
	return int(float(min) + fRandomFloat*float(max - min));
}

void main()
{
	//
	//
	//

	//face center------------------------
	vec3 Vert1 = gl_in[0].gl_Position.xyz;
	vec3 Vert2 = gl_in[1].gl_Position.xyz;
	vec3 Vert3 = gl_in[2].gl_Position.xyz;

	vec3 vGrassFieldPos = (Vert1 + Vert2 + Vert3) / 3.0;   //Center of the triangle - copy for later
														   //-----------------------------------

	vLocalSeed = vGrassFieldPos;

	float VertDist2 = distance(u_ViewOrigin, vGrassFieldPos);

	if (VertDist2 >= MIST_DISTANCE)
	{// Too far from viewer... Cull...
		return;
	}

	int lodLevel = 0;

	if (VertDist2 >= MIST_LOD_START_RANGE)
	{
		if (VertDist2 >= MIST_LOD_START_RANGE * 4.0)
		{
			lodLevel = 3;
		}
		else if (VertDist2 >= MIST_LOD_START_RANGE * 2.0)
		{
			lodLevel = 2;
		}
		else
		{
			lodLevel = 1;
		}
	}

	vec2 tcOffsetBegin = vec2(0.0);
	vec2 tcOffsetEnd = vec2(1.0);

	float randDir = sin(randZeroOne()*0.7f)*0.1f;

#ifdef THREE_WAY_MIST_CLUMPS
	for (int i = 0; i < 3; i++)
#else //!THREE_WAY_MIST_CLUMPS
	for (int i = 0; i < 2; i++)
#endif //THREE_WAY_MIST_CLUMPS
	{// Draw either 2 or 3 copies at each position at different angles...
		if (lodLevel == 3 && i > 0) continue;
		if (lodLevel == 2 && i > 1) continue;
		if (lodLevel == 1 && i > 2) continue;

		vec3 direction = (rotationMatrix(vec3(0, 1, 0), randDir)*vec4(vBaseDir[i], 1.0)).xyz;

		//vGrassFieldPos.z += MIST_HEIGHT * 0.5;

		vec3 P = vGrassFieldPos.xyz;

		vec3 va = P - (direction * MIST_HEIGHT*2.0);
		vec3 vb = P + (direction * MIST_HEIGHT*2.0);
		vec3 vc = va + vec3(0.0, 0.0, MIST_HEIGHT);
		vec3 vd = vb + vec3(0.0, 0.0, MIST_HEIGHT);

		//vec3 baseNorm = normalize(cross(normalize(vb - va), normalize(vc - va)));
		//vec3 I = normalize(vec3(normalize(P.xyz - u_ViewOrigin.xyz).xy, 0.0));
		//vec3 Nf = normalize(faceforward(baseNorm, I, baseNorm));


		//vVertNormal = EncodeNormal(Nf);


		vVertPosition = va.xyz;
		gl_Position = u_ModelViewProjectionMatrix * vec4(vVertPosition, 1.0);
		vTexCoord = vec2(0.0, 1.0);
		vBasePosition = vGrassFieldPos.xyz;
		EmitVertex();

		vVertPosition = vb.xyz;
		gl_Position = u_ModelViewProjectionMatrix * vec4(vVertPosition, 1.0);
		vTexCoord = vec2(1.0, 1.0);
		vBasePosition = vGrassFieldPos.xyz;
		EmitVertex();

		vVertPosition = vc.xyz;
		gl_Position = u_ModelViewProjectionMatrix * vec4(vVertPosition, 1.0);
		vTexCoord = vec2(0.0, 0.0);
		vBasePosition = vGrassFieldPos.xyz;
		EmitVertex();

		vVertPosition = vd.xyz;
		gl_Position = u_ModelViewProjectionMatrix * vec4(vVertPosition, 1.0);
		vTexCoord = vec2(1.0, 0.0);
		vBasePosition = vGrassFieldPos.xyz;
		EmitVertex();

		EndPrimitive();
	}
}
