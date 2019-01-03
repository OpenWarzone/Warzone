precision mediump float;

//incoming Position attribute from our SpriteBatch
attribute vec3 attr_Position;
attribute vec2 attr_TexCoord0;

uniform mat4	u_ModelViewProjectionMatrix;

varying vec2	vTexCoord0;

void main(void) {
   gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
   vTexCoord0 = attr_TexCoord0.xy;
}
