layout(vertices = MAX_PATCH_VERTICES) out;
//layout(vertices = 3) out;

uniform vec4			u_TesselationInfo;
uniform vec3			u_ViewOrigin;
uniform float			u_zFar;

#define TessLevelInner	u_TesselationInfo.g
#define TessLevelOuter	u_TesselationInfo.b

#define ID gl_InvocationID

// attributes of the input CPs
in vec4 WorldPos_CS_in[];
in vec3 Normal_CS_in[];
in vec2 TexCoord_CS_in[];
in vec3 ViewDir_CS_in[];
in vec4 Color_CS_in[];
in vec4 PrimaryLightDir_CS_in[];
in vec2 TexCoord2_CS_in[];
in vec3 Blending_CS_in[];
in float Slope_CS_in[];


// PN patch data
struct PnPatch
{
	float b210;
	float b120;
	float b021;
	float b012;
	float b102;
	float b201;
	float b111;
	float n110;
	float n011;
	float n101;
};

// tessellation levels
#define gTessellationLevelInner u_TesselationInfo.g
#define gTessellationLevelOuter u_TesselationInfo.b

out vec4 WorldPos_ES_in[MAX_PATCH_VERTICES];
out vec3 iNormal[MAX_PATCH_VERTICES];
out vec2 iTexCoord[MAX_PATCH_VERTICES];
out PnPatch iPnPatch[MAX_PATCH_VERTICES];
out vec4 Color_ES_in[MAX_PATCH_VERTICES];
out vec4 PrimaryLightDir_ES_in[MAX_PATCH_VERTICES];
out vec2 TexCoord2_ES_in[MAX_PATCH_VERTICES];
out vec3 Blending_ES_in[MAX_PATCH_VERTICES];
out float Slope_ES_in[MAX_PATCH_VERTICES];

float GetTessLevel(float Distance0, float Distance1)
{
	return mix(1.0, gTessellationLevelInner, clamp(((Distance0 + Distance1) / 2.0) / 6.0, 0.0, 1.0));
}

float wij(int i, int j)
{
	return dot(gl_in[j].gl_Position.xyz - gl_in[i].gl_Position.xyz, iNormal[i]/*Normal_CS_in[i]*/);
}

float vij(int i, int j)
{
	vec3 Pj_minus_Pi = gl_in[j].gl_Position.xyz - gl_in[i].gl_Position.xyz;
	vec3 Ni_plus_Nj = iNormal[i] + iNormal[j];
	return 2.0*dot(Pj_minus_Pi, Ni_plus_Nj) / dot(Pj_minus_Pi, Pj_minus_Pi);
}

void main()
{
	//vec3 normal = normalize(cross(gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz)); //calculate normal for this face

	// Calculate the distance from the camera to the three control points
	float EyeToVertexDistance0 = distance(u_ViewOrigin.xyz, WorldPos_CS_in[0].xyz);
	float EyeToVertexDistance1 = distance(u_ViewOrigin.xyz, WorldPos_CS_in[1].xyz);
	float EyeToVertexDistance2 = distance(u_ViewOrigin.xyz, WorldPos_CS_in[2].xyz);

	/*if (EyeToVertexDistance0 > u_zFar && EyeToVertexDistance1 > u_zFar && EyeToVertexDistance2 > u_zFar)
	{// Skip it all...
		gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = gl_TessLevelOuter[2] = gl_TessLevelInner[0] = 0.0;
		return;
	}*/
																																			  // get data
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	WorldPos_ES_in[gl_InvocationID] = WorldPos_CS_in[gl_InvocationID];
	iNormal[gl_InvocationID]					= Normal_CS_in[gl_InvocationID];
	//iNormal[gl_InvocationID] = normal;
	iTexCoord[gl_InvocationID] = TexCoord_CS_in[gl_InvocationID];
	Color_ES_in[gl_InvocationID] = Color_CS_in[gl_InvocationID];
	PrimaryLightDir_ES_in[gl_InvocationID] = PrimaryLightDir_CS_in[gl_InvocationID];
	TexCoord2_ES_in[gl_InvocationID] = TexCoord2_CS_in[gl_InvocationID];
	Blending_ES_in[gl_InvocationID] = Blending_CS_in[gl_InvocationID];
	Slope_ES_in[gl_InvocationID] = Slope_CS_in[gl_InvocationID];

	// set base 
	float P0 = gl_in[0].gl_Position[gl_InvocationID];
	float P1 = gl_in[1].gl_Position[gl_InvocationID];
	float P2 = gl_in[2].gl_Position[gl_InvocationID];
	float N0 = iNormal[0][gl_InvocationID];
	float N1 = iNormal[1][gl_InvocationID];
	float N2 = iNormal[2][gl_InvocationID];

	// compute control points
	iPnPatch[gl_InvocationID].b210 = (2.0*P0 + P1 - wij(0, 1)*N0) / 3.0;
	iPnPatch[gl_InvocationID].b120 = (2.0*P1 + P0 - wij(1, 0)*N1) / 3.0;
	iPnPatch[gl_InvocationID].b021 = (2.0*P1 + P2 - wij(1, 2)*N1) / 3.0;
	iPnPatch[gl_InvocationID].b012 = (2.0*P2 + P1 - wij(2, 1)*N2) / 3.0;
	iPnPatch[gl_InvocationID].b102 = (2.0*P2 + P0 - wij(2, 0)*N2) / 3.0;
	iPnPatch[gl_InvocationID].b201 = (2.0*P0 + P2 - wij(0, 2)*N0) / 3.0;
	float E = (iPnPatch[gl_InvocationID].b210
		+ iPnPatch[gl_InvocationID].b120
		+ iPnPatch[gl_InvocationID].b021
		+ iPnPatch[gl_InvocationID].b012
		+ iPnPatch[gl_InvocationID].b102
		+ iPnPatch[gl_InvocationID].b201) / 6.0;
	float V = (P0 + P1 + P2) / 3.0;
	iPnPatch[gl_InvocationID].b111 = E + (E - V)*0.5;
	iPnPatch[gl_InvocationID].n110 = N0 + N1 - vij(0, 1)*(P1 - P0);
	iPnPatch[gl_InvocationID].n011 = N1 + N2 - vij(1, 2)*(P2 - P1);
	iPnPatch[gl_InvocationID].n101 = N2 + N0 - vij(2, 0)*(P0 - P2);

	// set tess levels
	gl_TessLevelOuter[gl_InvocationID] = gTessellationLevelOuter;
	gl_TessLevelInner[0] = gTessellationLevelInner;

	// Calculate the tessellation levels
	gl_TessLevelOuter[0] = GetTessLevel(EyeToVertexDistance1, EyeToVertexDistance2);
	gl_TessLevelOuter[1] = GetTessLevel(EyeToVertexDistance2, EyeToVertexDistance0);
	gl_TessLevelOuter[2] = GetTessLevel(EyeToVertexDistance0, EyeToVertexDistance1);
	gl_TessLevelInner[0] = gl_TessLevelOuter[2];

	//float dist = GetTessLevel(EyeToVertexDistance1, EyeToVertexDistance2) + GetTessLevel(EyeToVertexDistance2, EyeToVertexDistance0) + GetTessLevel(EyeToVertexDistance0, EyeToVertexDistance1);
	//dist /= 3.0;
	//gl_TessLevelInner[0] = dist;
	//gl_TessLevelInner[1] = dist;
	//gl_TessLevelOuter[0] = dist;
	//gl_TessLevelOuter[1] = dist;
	//gl_TessLevelOuter[2] = dist;
	//gl_TessLevelOuter[3] = dist;

	//gl_TessLevelOuter[0] = 1.0;//gTessellationLevelInner;
	//gl_TessLevelOuter[1] = 1.0;//gTessellationLevelInner;
	//gl_TessLevelOuter[2] = 1.0;//gTessellationLevelInner;
	//gl_TessLevelInner[0] = gTessellationLevelInner;
	//gl_TessLevelInner[1] = gTessellationLevelInner;
}
