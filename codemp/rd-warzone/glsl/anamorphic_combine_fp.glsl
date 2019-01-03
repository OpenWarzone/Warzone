uniform sampler2D		u_DiffuseMap;
uniform sampler2D		u_NormalMap; // Blur'ed bloom'ed VBO image...

uniform vec4			u_Local1;

varying vec2			var_TexCoords;

void main(void)
{
  vec3 col1 = texture2D(u_DiffuseMap, var_TexCoords.st).rgb;
  vec3 col2 = texture2D(u_NormalMap, var_TexCoords.st).rgb * u_Local1.r;
  gl_FragColor.rgb = col1 + col2;
  gl_FragColor.a	= 1.0;
}
