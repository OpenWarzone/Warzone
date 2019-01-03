uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_DeluxeMap;
uniform sampler2D	u_GlowMap;

varying vec2		var_TexCoords;

void main (void)
{
	vec4 color = texture2D(u_DiffuseMap, var_TexCoords);

	//gl_FragColor = vec4(color.rgb, 1.0);
	//gl_FragColor = vec4(texture2D(u_DeluxeMap, var_TexCoords).rgb, 1.0);
	//return;

	//color.rgb = clamp(color.rgb, 0.0, 1.0);


	//float maxBright = max(color.x, max(color.y, color.z));
	//if (maxBright > 1.0) color.rgb /= maxBright;


	//vec3 brightness = texture2D(u_GlowMap, vec2(0.5)).rgb; //adaptation luminance
	//brightness = (brightness/(brightness+1.0));
	//brightness = vec3(max(brightness.x, max(brightness.y, brightness.z)));
	vec3	lumCoeff = vec3(0.212656, 0.715158, 0.072186);  				//Calculate luma with these values
	float	max_color = clamp(max(color.r, max(color.g, color.b)), 0.0, 1.0); 	//Find the strongest color
	float	min_color = clamp(min(color.r, min(color.g, color.b)), 0.0, 1.0); 	//Find the weakest color
	float	luma = clamp(dot(lumCoeff, color.rgb), 0.0, 1.0); 							//Calculate luma (grey)
	vec3	brightness = vec3(luma);
	//brightness = (brightness/(brightness+1.0));
	//brightness = vec3(max(brightness.x, max(brightness.y, brightness.z)));

	//maxBright = max(brightness.x, max(brightness.y, brightness.z));
	//if (maxBright > 1.0) brightness /= maxBright;
	

	vec3	palette;
	vec2	uvsrc = vec2(0.0);

	uvsrc.x = clamp(color.r, 0.0, 1.0);
	uvsrc.y = brightness.r;
	palette.r = texture2D(u_DeluxeMap, uvsrc).r;
	uvsrc.x = clamp(color.g, 0.0, 1.0);
	uvsrc.y = brightness.g;
	palette.g = texture2D(u_DeluxeMap, uvsrc).g;
	uvsrc.x = clamp(color.b, 0.0, 1.0);
	uvsrc.y = brightness.b;
	palette.b = texture2D(u_DeluxeMap, uvsrc).b;
	//color.rgb = (color.rgb + (palette.rgb * color.rgb)) / 2.0;
	//color.rgb = color.rgb * palette.rgb;
	color.rgb = (color.rgb + (palette.rgb * color.rgb)) / 2.0;

	gl_FragColor = vec4(color.rgb, 1.0);
}
