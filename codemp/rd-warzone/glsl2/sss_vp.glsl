attribute vec3	attr_Position;
attribute vec2	attr_TexCoord0;

uniform mat4	u_ModelViewProjectionMatrix;
uniform mat4	u_ModelViewMatrix;
uniform mat4	u_ProjectionMatrix;

varying vec3   	var_Ray;
varying vec2   	var_TexCoords;

void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_TexCoords = attr_TexCoord0.st;
	vec4 cameraRay = inverse(u_ModelViewProjectionMatrix) * vec4(var_TexCoords * 2.0 - 1.0, 1.0, 1.0);
    var_Ray = cameraRay.xyz / cameraRay.w;
}
