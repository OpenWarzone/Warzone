attribute vec3 attr_Position;
attribute vec2 attr_TexCoord0;

uniform mat4   u_ModelViewProjectionMatrix;

varying vec2	var_TexCoords;

void main(void) {
	var_TexCoords   = vec2( (gl_VertexID << 1) & 2, gl_VertexID & 2 );
	gl_Position = vec4( var_TexCoords * 2.0 - 1.0, 0.0, 1.0 );
}
