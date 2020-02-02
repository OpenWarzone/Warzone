invariant gl_Position;

attribute vec3 attr_Position;

uniform mat4   u_ModelViewProjectionMatrix;

void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
}
