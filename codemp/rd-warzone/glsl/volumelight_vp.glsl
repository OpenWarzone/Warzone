#define __USING_SHADOW_MAP__

#ifndef __USING_SHADOW_MAP__

attribute vec3				attr_Position;
attribute vec2				attr_TexCoord0;

uniform mat4				u_ModelViewProjectionMatrix;

varying vec2				var_TexCoords;

void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_TexCoords = attr_TexCoord0.st;
}

#else //__USING_SHADOW_MAP__

attribute vec4				attr_Position;
attribute vec2				attr_TexCoord0;

uniform vec3				u_ViewForward;
uniform vec3				u_ViewLeft;
uniform vec3				u_ViewUp;
uniform vec4				u_ViewInfo; // zfar / znear

varying vec2				var_DepthTex;
varying vec3				var_ViewDir;
varying vec3				var_ViewDir2;

void main()
{
	gl_Position = attr_Position;
	vec2 screenCoords = gl_Position.xy / gl_Position.w;
	var_DepthTex = attr_TexCoord0.xy;
	var_ViewDir = u_ViewForward + u_ViewLeft * -screenCoords.x + u_ViewUp * screenCoords.y;
	//var_ViewDir2 = u_ViewForward + u_ViewLeft * -0.5 + u_ViewUp * 0.5;
	//var_ViewDir2 = u_ViewForward + u_ViewLeft * 0.5 + u_ViewUp * 0.5;
	var_ViewDir2 = u_ViewForward + u_ViewLeft * 0.0 + u_ViewUp * 0.0;
}

#endif //__USING_SHADOW_MAP__
