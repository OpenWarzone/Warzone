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
#if 0
	//if (u_Local0.r == 1.0 || u_Local0.r == 3.0)
	//	return DOF_MANUALFOCUSDEPTH;

 	float depthsum = 0.0;
	
	depthsum+=texture(u_ScreenDepthMap,focalpoint).x * 0.999;

#ifdef BLUR_FOCUS
	depthsum+=texture(u_ScreenDepthMap,focalpoint+vec2(-0.1, -0.1)).x * 0.999;
	depthsum+=texture(u_ScreenDepthMap,focalpoint+vec2(0.1, 0.1)).x * 0.999;
	depthsum+=texture(u_ScreenDepthMap,focalpoint+vec2(-0.1, 0.1)).x * 0.999;
	depthsum+=texture(u_ScreenDepthMap,focalpoint+vec2(0.1, -0.1)).x * 0.999;
#if 1 // More focus checking, to blend over time better...
	depthsum+=texture(u_ScreenDepthMap,focalpoint+vec2(-0.2, -0.2)).x * 0.999 * 0.5;
	depthsum+=texture(u_ScreenDepthMap,focalpoint+vec2(0.2, 0.2)).x * 0.999 * 0.5;
	depthsum+=texture(u_ScreenDepthMap,focalpoint+vec2(-0.2, 0.2)).x * 0.999 * 0.5;
	depthsum+=texture(u_ScreenDepthMap,focalpoint+vec2(0.2, -0.2)).x * 0.999 * 0.5;
	depthsum = depthsum/7.0;
#else
	depthsum = depthsum/5.0;
#endif
#endif //BLUR_FOCUS
#else
	float depthsum = clamp(texture(u_SpecularMap, vec2(0.5, 0.5)).x + 0.1, 0.0, 1.0) * 0.999;
#endif

	return depthsum; 
}

void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_TexCoords = attr_TexCoord0.st;
	var_FocalDepth = GetFocalDepth(DOF_FOCUSPOINT);
}
