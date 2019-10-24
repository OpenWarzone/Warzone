attribute vec3					attr_Position;
attribute vec4					attr_TexCoord0;

uniform mat4					u_ModelViewProjectionMatrix;
uniform vec2					u_Dimensions;


out vec2						var_TexCoords;
out vec4						VSDvpos;
out vec2						VSDtexcoord;
out flat vec3					VSDtexcoord2viewADD;
out flat vec3					VSDtexcoord2viewMUL;
out flat vec4					VSDview2texcoord;


/*=============================================================================
	Statics
=============================================================================*/

float BUFFER_WIDTH				= u_Dimensions.x;
float BUFFER_HEIGHT				= u_Dimensions.y;
float BUFFER_RCP_WIDTH			= 1.0 / u_Dimensions.x;
float BUFFER_RCP_HEIGHT			= 1.0 / u_Dimensions.y;

vec2 PIXEL_SIZE 				= vec2(BUFFER_RCP_WIDTH, BUFFER_RCP_HEIGHT);
vec2 ASPECT_RATIO 				= vec2(1.0, BUFFER_HEIGHT / BUFFER_WIDTH);
vec2 SCREEN_SIZE 				= vec2(BUFFER_WIDTH, BUFFER_HEIGHT);

const float FOV					= 80; //vertical FoV

float rcp (float x)
{
	return 1.0 / x;
}

vec2 rcp (vec2 x)
{
	return vec2(1.0) / x;
}

void main()
{
	gl_Position					= u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_TexCoords				= attr_TexCoord0.st;
	
	VSDtexcoord					= var_TexCoords;
	VSDvpos						= vec4(VSDtexcoord * vec2(2.0, -2.0) + vec2(-1.0, 1.0), 0.0, 1.0);
	VSDtexcoord2viewADD			= vec3(-1.0, -1.0, 1.0);
	VSDtexcoord2viewMUL			= vec3(2.0, 2.0, 0.0);
	VSDtexcoord2viewADD			= vec3(-tan(radians(FOV * 0.5)).xx, 1.0) * ASPECT_RATIO.yxx;
	VSDtexcoord2viewMUL			= vec3(-2.0 * VSDtexcoord2viewADD.xy,0.0);
	VSDview2texcoord.xy			= rcp(VSDtexcoord2viewMUL.xy);
	VSDview2texcoord.zw			= -VSDtexcoord2viewADD.xy * VSDview2texcoord.xy;
}
