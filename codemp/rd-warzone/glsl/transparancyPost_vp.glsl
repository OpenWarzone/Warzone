attribute vec3	attr_Position;
attribute vec4	attr_TexCoord0;

uniform mat4	u_ModelViewProjectionMatrix;
uniform vec3	u_ViewOrigin;

uniform vec3	u_ViewForward;
uniform vec3	u_ViewLeft;
uniform vec3	u_ViewUp;

varying vec2	var_TexCoords;
varying vec3	var_ViewDir;
varying vec3	var_position;
varying vec3	var_viewOrg;

void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_TexCoords = attr_TexCoord0.st;
}
