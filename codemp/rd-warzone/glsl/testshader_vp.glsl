attribute vec3	attr_Position;
attribute vec2	attr_TexCoord0;

uniform mat4	u_ModelViewProjectionMatrix;
//uniform mat4	u_ModelMatrix;
//uniform mat4	u_ModelViewMatrix;
//uniform mat4	u_ProjectionMatrix;
//uniform mat4	u_ViewProjectionMatrix;
//uniform mat4	u_ViewMatrix;
//uniform mat4	u_invEyeProjectionMatrix;

uniform vec2	u_Dimensions;
uniform vec4	u_ViewInfo; // zfar / znear, zfar

uniform vec4	u_Local0;		// testvalue0, testvalue1, testvalue2, testvalue3
uniform vec4	u_Local1;		// testshadervalue1, testshadervalue2, testshadervalue3, testshadervalue4
uniform vec4	u_MapInfo;		// MAP_INFO_SIZE[0], MAP_INFO_SIZE[1], MAP_INFO_SIZE[2], 0.0

uniform vec3	u_ViewOrigin;
uniform vec4	u_PrimaryLightOrigin;
uniform vec3	u_PrimaryLightColor;

varying vec2   var_TexCoords;
varying vec3   var_vertPos;
varying vec3   var_rayDir;
varying vec3   var_sunDir;

void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);

	var_TexCoords = attr_TexCoord0.st;
	var_vertPos = attr_Position.xyz;
	var_rayDir = u_ViewOrigin.xyz - attr_Position.xyz;
	var_sunDir = u_PrimaryLightOrigin.xyz - attr_Position.xyz;
}
