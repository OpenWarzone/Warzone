uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_DeluxeMap;
uniform sampler2D	u_GlowMap;

varying vec2		var_TexCoords;

void main (void)
{

	/*
	//adaptation in time
	color.rgb=saturate(color.rgb);
	float3	brightness=tex2D(_s4, 0.5);//adaptation luminance
	brightness=(brightness/(brightness+1.0));//new version
	brightness=max(brightness.x, max(brightness.y, brightness.z));//new version
	float3	palette;
	float4	uvsrc=0.0;
	uvsrc.y=brightness.r;
	uvsrc.x=color.r;
	palette.r=tex2Dlod(_s7, uvsrc).r;
	uvsrc.x=color.g;
	uvsrc.y=brightness.g;
	palette.g=tex2Dlod(_s7, uvsrc).g;
	uvsrc.x=color.b;
	uvsrc.y=brightness.b;
	palette.b=tex2Dlod(_s7, uvsrc).b;
	color.rgb=palette.rgb;
	*/

	vec3 color = clamp(texture(u_DiffuseMap, var_TexCoords).rgb, 0.0, 1.0);

	//gl_FragColor = vec4(color.rgb, 1.0);
	//gl_FragColor = vec4(texture2D(u_DeluxeMap, var_TexCoords).rgb, 1.0);
	//return;


#if 1
	vec3 brightness = texture(u_GlowMap, vec2(0.5)).rgb; //adaptation luminance
	//brightness = (brightness / (brightness+1.0));//new version
	//brightness = vec3(max(brightness.x, max(brightness.y, brightness.z)));//new version
#else
	vec3	lumCoeff = vec3(0.212656, 0.715158, 0.072186);  				//Calculate luma with these values
	float	max_color = clamp(max(color.r, max(color.g, color.b)), 0.0, 1.0); 	//Find the strongest color
	float	min_color = clamp(min(color.r, min(color.g, color.b)), 0.0, 1.0); 	//Find the weakest color
	float	luma = clamp(dot(lumCoeff, color.rgb), 0.0, 1.0); 							//Calculate luma (grey)
	vec3	brightness = vec3(luma);
	//brightness = (brightness / (brightness+1.0));//new version
	//brightness = vec3(max(brightness.x, max(brightness.y, brightness.z)));//new version
#endif

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
	//color.rgb = (color.rgb + (palette.rgb * color.rgb)) / 2.0;
	//color.rgb = color.rgb * palette.rgb;
	//color.rgb = (color.rgb + (palette.rgb * color.rgb)) / 2.0;
	color.rgb = palette;

	gl_FragColor = vec4(color.rgb, 1.0);
}
