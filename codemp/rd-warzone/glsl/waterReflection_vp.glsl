attribute vec3	attr_Position;
attribute vec4	attr_TexCoord0;

uniform mat4	u_ModelViewProjectionMatrix;
uniform vec3	u_ViewOrigin;

varying vec2	var_TexCoords;
varying vec3	var_ViewDir;
varying vec3	var_position;
varying vec3	var_viewOrg;

void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_TexCoords = attr_TexCoord0.st;
	var_ViewDir = u_ViewOrigin - vec3(u_ModelViewProjectionMatrix * vec4(attr_Position.xyz, 1.0)).xyz;
	//var_ViewDir = u_ViewOrigin;
	//var_viewOrg = u_ViewOrigin;
	//var_viewOrg = vec3(u_ModelViewProjectionMatrix * vec4(u_ViewOrigin, 1.0)).xyz;
	var_viewOrg = vec3(var_TexCoords.xy, vec3(u_ModelViewProjectionMatrix * vec4(u_ViewOrigin.xyz, 1.0)).z);
	var_position = vec3(u_ModelViewProjectionMatrix * vec4(attr_Position.xyz, 1.0)).xyz;
}
