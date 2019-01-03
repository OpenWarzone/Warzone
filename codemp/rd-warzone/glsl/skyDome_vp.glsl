uniform mat4	u_ModelViewProjectionMatrix;
uniform mat4	u_NormalMatrix;
attribute vec3	attr_Position;
attribute vec3	attr_Normal;
attribute vec2	attr_TexCoord0;

uniform vec2	u_Dimensions;
uniform vec3	u_ViewOrigin;
uniform vec4    u_PrimaryLightOrigin;


out vec3 fWorldPos;
out float playerLookingAtSun;	// the dot of the player looking at the sun - should be the same for all verts
out vec3 var_Position;
out vec3 var_Normal;
out vec3 var_SunDir;
out vec3 var_ViewDir;

void main()
{		
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);

//#define playerLookAtDir normalize(attr_Position.xyz - u_ViewOrigin.xyz)
//#define sunDir normalize(u_PrimaryLightOrigin.xyz - u_ViewOrigin.xyz)
#define playerLookAtDir normalize(attr_Position.xyz)
#define sunDir normalize(u_PrimaryLightOrigin.xyz)

	playerLookingAtSun = dot(playerLookAtDir, sunDir);
	fWorldPos = attr_Position.xyz;
	var_Position = attr_Position.xyz;
	var_Normal = normalize(attr_Normal * 2.0 - 1.0);
	var_SunDir = sunDir;
	var_ViewDir = playerLookAtDir;
}
