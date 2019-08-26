uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_DeluxeMap;
uniform sampler2D	u_LevelsMap;

uniform vec4		u_Local0;			// MAP_COLOR_CORRECTION_METHOD, r_testvalue0->value, r_testvalue1->value, r_testvalue2->value

varying vec2		var_TexCoords;

void main (void)
{
	vec3 color = clamp(texture(u_DiffuseMap, var_TexCoords).rgb, 0.0, 1.0);
	//vec3 minAvgMax = texture(u_LevelsMap, var_TexCoords).rgb;
	vec3 brightness = vec3(0.0);

	if (u_Local0.r == 4.0)
	{
		brightness = vec3(texture(u_LevelsMap, var_TexCoords).b);
	}
	else if (u_Local0.r == 3.0)
	{
		brightness = vec3(texture(u_LevelsMap, var_TexCoords).g);
	}
	else if (u_Local0.r == 2.0)
	{
		brightness = vec3(texture(u_LevelsMap, var_TexCoords).r);
	}
	else if (u_Local0.r == 1.0)
	{
		vec3	lumCoeff = vec3(0.212656, 0.715158, 0.072186);  					// Calculate luma with these values
		float	max_color = clamp(max(color.r, max(color.g, color.b)), 0.0, 1.0); 	// Find the strongest color
		float	min_color = clamp(min(color.r, min(color.g, color.b)), 0.0, 1.0); 	// Find the weakest color
		float	luma = clamp(dot(lumCoeff, color.rgb), 0.0, 1.0); 					// Calculate luma (grey)
		brightness = vec3(luma);
	}
	else
	{
		brightness = vec3(max(color.r, max(color.g, color.b)));
	}

	vec3	palette = vec3(0.0);
	vec2	uvsrc = vec2(0.0);

	uvsrc.x = color.r;
	uvsrc.y = brightness.r;
	palette.r = texture(u_DeluxeMap, uvsrc).r;
	uvsrc.x = color.g;
	uvsrc.y = brightness.g;
	palette.g = texture(u_DeluxeMap, uvsrc).g;
	uvsrc.x = color.b;
	uvsrc.y = brightness.b;
	palette.b = texture(u_DeluxeMap, uvsrc).b;

	color.rgb = palette;

	gl_FragColor = vec4(color.rgb, 1.0);
}
