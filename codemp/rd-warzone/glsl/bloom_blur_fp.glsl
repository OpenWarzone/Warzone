uniform sampler2D u_DiffuseMap;

uniform vec2	u_Dimensions;

varying vec2	var_TexCoords;

#define BLOOM_BLUR_WIDTH 3.0

void main(void)
{
  vec2 PIXEL_OFFSET = vec2(1.0 / u_Dimensions);
  const vec2 X_AXIS = vec2(1.0, 0.0);
  const vec2 Y_AXIS = vec2(0.0, 1.0);

  vec3 color = vec3(0.0, 0.0, 0.0);
  vec3 col0 = texture2D(u_DiffuseMap, var_TexCoords.xy).rgb;

  for (float width = 0.0; width < BLOOM_BLUR_WIDTH; width += 1.0)
  {
	vec3 col1 = texture2D(u_DiffuseMap, var_TexCoords.xy + ((X_AXIS.xy * width) * PIXEL_OFFSET)).rgb;
	vec3 col2 = texture2D(u_DiffuseMap, var_TexCoords.xy - ((X_AXIS.xy * width) * PIXEL_OFFSET)).rgb;
	color.rgb += (col0 / 2.0) + (col1 + col2) / 4.0;
  }

  color.rgb /= BLOOM_BLUR_WIDTH;

  vec3 color2 = vec3(0.0, 0.0, 0.0);

  for (float width = 0.0; width < BLOOM_BLUR_WIDTH; width += 1.0)
  {
	vec3 col1 = texture2D(u_DiffuseMap, var_TexCoords.xy + ((Y_AXIS.xy * width) / u_Dimensions)).rgb;
	vec3 col2 = texture2D(u_DiffuseMap, var_TexCoords.xy - ((Y_AXIS.xy * width) / u_Dimensions)).rgb;
	color2.rgb += (col0 / 2.0) + (col1 + col2) / 4.0;
  }

  color2.rgb /= BLOOM_BLUR_WIDTH;

  gl_FragColor.rgb = (color + color2) / 2.0;
  gl_FragColor.a	= 1.0;
}
