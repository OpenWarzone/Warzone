layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform mat4			u_ModelViewProjectionMatrix;

uniform vec4			u_TesselationInfo;

#define TessLevelInner	u_TesselationInfo.g
#define TessLevelOuter	u_TesselationInfo.b

in vec3 WorldPos_GS_in[];
in vec2 TexCoord_GS_in[];
in vec2 envTC_GS_in[];
in vec3 Normal_GS_in[];
in vec3 ViewDir_GS_in[];
in vec4 Color_GS_in[];
in vec4 PrimaryLightDir_GS_in[];
in vec2 TexCoord2_GS_in[];
in vec3 Blending_GS_in[];
in float Slope_GS_in[];

out vec3 WorldPos_FS_in;
out vec2 TexCoord_FS_in;
out vec2 envTC_FS_in;
out vec3 Normal_FS_in;
out vec3 ViewDir_FS_in;
out vec4 Color_FS_in;
out vec4 PrimaryLightDir_FS_in;
out vec2 TexCoord2_FS_in;
out vec3 Blending_FS_in;
/*flat*/ out float Slope_FS_in;
 
void main()
{
    //vec3 A = WorldPos_GS_in[2] - WorldPos_GS_in[0];
    //vec3 B = WorldPos_GS_in[1] - WorldPos_GS_in[0];
    //Normal_FS_in = normalize(cross(A, B));

	for (int i = 0; i < 3; i++)
	{
		WorldPos_FS_in = WorldPos_GS_in[i];
		TexCoord_FS_in = TexCoord_GS_in[i];
		envTC_FS_in = envTC_GS_in[i];
		ViewDir_FS_in = ViewDir_GS_in[i];
		Color_FS_in = Color_GS_in[i];
		PrimaryLightDir_FS_in = PrimaryLightDir_GS_in[i];
		TexCoord2_FS_in = TexCoord2_GS_in[i];
		Blending_FS_in = Blending_GS_in[i];
		Slope_FS_in = Slope_GS_in[i];
		Normal_FS_in = Normal_GS_in[i];

		//gl_Position = gl_in[i].gl_Position;
		gl_Position = u_ModelViewProjectionMatrix * vec4(gl_in[i].gl_Position.xyz, 1);
		EmitVertex();
	}
 
    EndPrimitive();
}
