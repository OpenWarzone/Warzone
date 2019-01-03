attribute vec3						attr_InstancesPosition;
attribute vec2						attr_InstancesTexCoord;

uniform mat4						u_ModelViewProjectionMatrix;

uniform vec4						u_Local2; // PROCEDURAL_CLOUDS_ENABLED, PROCEDURAL_CLOUDS_CLOUDSCALE, PROCEDURAL_CLOUDS_SPEED, PROCEDURAL_CLOUDS_DARK
uniform vec4						u_Local3; // PROCEDURAL_CLOUDS_LIGHT, PROCEDURAL_CLOUDS_CLOUDCOVER, PROCEDURAL_CLOUDS_CLOUDALPHA, PROCEDURAL_CLOUDS_SKYTINT
uniform vec4						u_Local5; // dayNightEnabled, nightScale, MAP_CLOUD_LAYER_HEIGHT, 0.0
uniform vec4						u_Local9; // testvalues

uniform vec3						u_ViewOrigin;

#define MAP_CLOUD_LAYER_HEIGHT		u_Local5.b

varying vec2						var_TexCoords;
varying vec3						var_vertPos;
varying vec3						var_Normal;

void main()
{
	vec3 position = attr_InstancesPosition;
	position.z = MAP_CLOUD_LAYER_HEIGHT;
	gl_Position = u_ModelViewProjectionMatrix * vec4(position, 1.0);
	var_vertPos = position;
	var_TexCoords = attr_InstancesTexCoord;
	//var_Normal = attr_Normal * 2.0 - 1.0;
}
