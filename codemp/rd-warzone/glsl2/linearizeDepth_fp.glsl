uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_ScreenDepthMap;

uniform vec4		u_ViewInfo; // zmin, zmax, zmax / zmin

varying vec2		var_TexCoords;

//out vec4 out_Color;
out vec4 out_Glow;
out vec4 out_Normal;
out vec4 out_Position;
out vec4 out_NormalDetail;

#define out_Depth512 out_Color
#define out_Depth1024 out_Glow
#define out_Depth2048 out_Normal
#define out_Depth4096 out_Position
#define out_DepthZfar out_NormalDetail

float linearize(float depth, float zmin, float zmax)
{
	return 1.0 / mix(zmax/zmin, 1.0, depth);
}

void main(void)
{
	highp float depth = texture(u_ScreenDepthMap, var_TexCoords).r;
	highp float depth512 = linearize(depth, u_ViewInfo.r, 512.0);
	highp float depth1024 = linearize(depth, u_ViewInfo.r, 1024.0);
	highp float depth2048 = linearize(depth, u_ViewInfo.r, 2048.0);
	highp float depth4096 = linearize(depth, u_ViewInfo.r, 4096.0);
	highp float depthZfar = linearize(depth, u_ViewInfo.r, u_ViewInfo.g);

	out_Depth512 = vec4(depth512, depth512, depth512, 1.0);
	out_Depth1024 = vec4(depth1024, depth1024, depth1024, 1.0);
	out_Depth2048 = vec4(depth2048, depth2048, depth2048, 1.0);
	out_Depth4096 = vec4(depth4096, depth4096, depth4096, 1.0);
	out_DepthZfar = vec4(depthZfar, depthZfar, depthZfar, 1.0);
}
