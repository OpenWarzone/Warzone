attribute vec3		attr_Position;
attribute vec4		attr_TexCoord0;

uniform mat4		u_ModelViewProjectionMatrix;

uniform sampler2D	u_ScreenDepthMap;
uniform sampler2D	u_SpecularMap;

uniform vec4		u_Local0; // dofValue, 0, 0, 0

varying vec2		var_TexCoords;
flat varying float	var_FocalDepth;


#define BLUR_FOCUS
//#define DOF_MANUALFOCUSDEPTH 		253.8 //253.0	//[0.0 to 1.0] Manual focus depth. 0.0 means camera is focus plane, 1.0 means sky is focus plane.
#define DOF_FOCUSPOINT	 		vec2(0.5,0.75)//vec2(0.5,0.5)	//[0.0 to 1.0] Screen coordinates of focus point. First value is horizontal, second value is vertical position.

float GetFocalDepth(vec2 focalpoint)
{
	float depthsum = clamp(texture(u_SpecularMap, vec2(0.5, 0.5)).x + 0.1, 0.0, 1.0) * 0.999;
	depthsum = pow(clamp(depthsum * 2.25, 0.0, 1.0), 1.15);
	return depthsum; 
}

void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_TexCoords = attr_TexCoord0.st;
	var_FocalDepth = GetFocalDepth(DOF_FOCUSPOINT);
}
