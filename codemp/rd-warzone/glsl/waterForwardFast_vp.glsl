invariant gl_Position;

attribute vec3		attr_InstancesPosition;
attribute vec2		attr_InstancesTexCoord;

attribute vec3		attr_Normal;

uniform mat4		u_ModelViewProjectionMatrix;

uniform vec4		u_Local9; // testvalue1-3, MAP_WATER_LEVEL
uniform vec4		u_Local10;

varying vec3		var_vertPos;
varying vec3		var_Normal;

#define MAP_WATER_LEVEL u_Local9.a

void main()
{
	vec3 position = attr_InstancesPosition;
	position.z = MAP_WATER_LEVEL;
	gl_Position = u_ModelViewProjectionMatrix * vec4(position, 1.0);
	var_vertPos = position;
	var_Normal = attr_Normal * 2.0 - 1.0;
}
