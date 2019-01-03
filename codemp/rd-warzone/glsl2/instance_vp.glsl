#extension GL_EXT_draw_instanced : enable

attribute vec3		attr_Position;
attribute vec3		attr_Normal;
attribute vec4		attr_TexCoord0;

uniform mat4		u_ModelViewProjectionMatrix;

uniform vec3		u_InstancePositions[MAX_INSTANCED_MODEL_INSTANCES];
uniform vec3		u_InstanceScales[MAX_INSTANCED_MODEL_INSTANCES];

varying vec2		var_Tex1;
varying vec3		var_VertPos;
varying vec3		var_Normal;

void main()
{
	var_VertPos = (attr_Position * u_InstanceScales[gl_InstanceID]) + u_InstancePositions[gl_InstanceID];
	gl_Position = u_ModelViewProjectionMatrix * vec4(var_VertPos, 1.0);
	var_Tex1 = attr_TexCoord0.st;
	var_Normal = attr_Normal * 2.0 - 1.0;
}
