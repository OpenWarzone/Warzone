attribute vec3 attr_Position;
attribute vec4 attr_TexCoord0;
attribute vec3 attr_Normal;
//attribute vec4 attr_Color;

uniform mat4   u_ModelViewProjectionMatrix;

varying vec2   var_Tex1;
varying vec3   var_Position;
//varying vec3   var_Normal;
//varying vec4   var_Color;

void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_Tex1 = attr_TexCoord0.st;
	var_Position = attr_Position;
	//var_Normal = attr_Normal * 2.0 + 1.0;
	//var_Color = attr_Color;
}
