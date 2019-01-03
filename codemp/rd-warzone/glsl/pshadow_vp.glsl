attribute vec3 attr_Position;
//attribute vec3 attr_Normal;

uniform mat4   u_ModelViewProjectionMatrix;

uniform vec4      u_Local0;			// shadowMapSize, testshadervalues
uniform vec4      u_Local1;			// realLightOrigin[0], realLightOrigin[1], realLightOrigin[2], usingTessellation
uniform vec4      u_Local2;			// playerOrigin[0], playerOrigin[1], playerOrigin[2], invLightPower

#if defined(USE_TESSELLATION)
out vec3 Normal_CS_in;
out vec2 TexCoord_CS_in;
out vec4 WorldPos_CS_in;
out vec3 ViewDir_CS_in;
out vec4 Color_CS_in;
out vec4 PrimaryLightDir_CS_in;
out vec2 TexCoord2_CS_in;
out vec3 Blending_CS_in;
out float Slope_CS_in;
#endif

varying vec3   var_Position;
//varying vec3   var_Normal;


void main()
{
	var_Position  = attr_Position;
	//var_Normal    = attr_Normal * 2.0 - 1.0;

#if defined(USE_TESSELLATION)
	Normal_CS_in = vec3(0.0);
	TexCoord_CS_in = vec2(0.0);
	WorldPos_CS_in = vec4(attr_Position.xyz, 1.0);
	ViewDir_CS_in = vec3(0.0);
	Color_CS_in = vec4(0.0);
	PrimaryLightDir_CS_in = vec4(0.0);
	TexCoord2_CS_in = vec2(0.0);
	Blending_CS_in = vec3(0.0);
	Slope_CS_in = float(0.0);
	gl_Position = vec4(attr_Position, 1.0);
#else
	vec3 pos = attr_Position;
	//pos.z += u_Local1.a;
	gl_Position = u_ModelViewProjectionMatrix * vec4(pos, 1.0);
#endif
}
