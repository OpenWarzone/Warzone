layout(vertices = MAX_PATCH_VERTICES) out;

uniform vec4			u_TesselationInfo;
uniform vec3			u_ViewOrigin;
uniform float			u_zFar;

#define TessLevelInner	u_TesselationInfo.g
#define TessLevelOuter	u_TesselationInfo.b
#define TessMinSize		u_TesselationInfo.a

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

// tessellation levels
out vec4 WorldPos_ES_in[MAX_PATCH_VERTICES];
out vec3 iNormal[MAX_PATCH_VERTICES];
out vec2 iTexCoord[MAX_PATCH_VERTICES];
out vec4 Color_ES_in[MAX_PATCH_VERTICES];
out vec4 PrimaryLightDir_ES_in[MAX_PATCH_VERTICES];
out vec2 TexCoord2_ES_in[MAX_PATCH_VERTICES];
out vec3 Blending_ES_in[MAX_PATCH_VERTICES];
out float Slope_ES_in[MAX_PATCH_VERTICES];

void main()
{
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	WorldPos_ES_in[gl_InvocationID] = WorldPos_CS_in[gl_InvocationID];
	iNormal[gl_InvocationID] = Normal_CS_in[gl_InvocationID];
	iTexCoord[gl_InvocationID] = TexCoord_CS_in[gl_InvocationID];
	Color_ES_in[gl_InvocationID] = Color_CS_in[gl_InvocationID];
	PrimaryLightDir_ES_in[gl_InvocationID] = PrimaryLightDir_CS_in[gl_InvocationID];
	TexCoord2_ES_in[gl_InvocationID] = TexCoord2_CS_in[gl_InvocationID];
	Blending_ES_in[gl_InvocationID] = Blending_CS_in[gl_InvocationID];
	Slope_ES_in[gl_InvocationID] = Slope_CS_in[gl_InvocationID];

	float gTessellationLevelInner = u_TesselationInfo.g;
	float gTessellationLevelOuter = u_TesselationInfo.b;

	vec3 Vert1 = gl_in[0].gl_Position.xyz;
	vec3 Vert2 = gl_in[1].gl_Position.xyz;
	vec3 Vert3 = gl_in[2].gl_Position.xyz;

	if (distance(Vert1, u_ViewOrigin) > u_zFar && distance(Vert2, u_ViewOrigin) > u_zFar && distance(Vert3, u_ViewOrigin) > u_zFar)
	{// Skip it all...
		gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = gl_TessLevelOuter[2] = gl_TessLevelInner[0] = 0.0;
		return;
	}

	float vSize = (distance(Vert1, Vert2) + distance(Vert1, Vert3) + distance(Vert2, Vert3)) / 3.0;

	if (vSize < TessMinSize)//512.0)
	{// Don't even bother... Only tessellate big stuff...
		gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = gl_TessLevelOuter[2] = gl_TessLevelInner[0] = 1.0;
		return;
	}

	float sizeMult = clamp(float(vSize) / TessMinSize/*512.0*/, 0.0, 1.0); // Scale by vert size, so tiny verts don't get a crapload of grass...

	float uTessLevel = float(gTessellationLevelOuter) * sizeMult;

	// LODs, tessellate less the further from the camera this vert is...
	vec3 center = (Vert1 + Vert2 + Vert3) / 3.0;
	float dist = round(distance(center, u_ViewOrigin));
	float distFactor = 1.0;

	if (dist > 4096.0)
	{// Closer then 4096.0 gets full tess...
		distFactor = dist / 4096.0;
	}

	uTessLevel = max(uTessLevel / distFactor, 1.0);

	// set tess levels
	gl_TessLevelOuter[gl_InvocationID] = uTessLevel;
	gl_TessLevelInner[0] = uTessLevel;

	// Calculate the distance from the camera to the three control points
	//float EyeToVertexDistance0 = distance(u_ViewOrigin.xyz, WorldPos_CS_in[0].xyz);
	//float EyeToVertexDistance1 = distance(u_ViewOrigin.xyz, WorldPos_CS_in[1].xyz);
	//float EyeToVertexDistance2 = distance(u_ViewOrigin.xyz, WorldPos_CS_in[2].xyz);

	// Calculate the tessellation levels
	gl_TessLevelOuter[0] = uTessLevel;// GetTessLevel(EyeToVertexDistance1, EyeToVertexDistance2);
	gl_TessLevelOuter[1] = uTessLevel;// GetTessLevel(EyeToVertexDistance2, EyeToVertexDistance0);
	gl_TessLevelOuter[2] = uTessLevel;// GetTessLevel(EyeToVertexDistance0, EyeToVertexDistance1);
	gl_TessLevelInner[0] = gl_TessLevelOuter[2];
}
