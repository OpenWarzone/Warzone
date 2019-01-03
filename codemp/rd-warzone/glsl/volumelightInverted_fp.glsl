uniform sampler2D			u_DiffuseMap;
uniform sampler2D			u_GlowMap;
uniform sampler2D			u_SpecularMap;
uniform sampler2D			u_ScreenDepthMap;

uniform vec2				u_vlightPositions;
uniform vec3				u_vlightColors;

uniform vec4				u_Local0;
uniform vec4				u_Local1; // nightScale
uniform vec4				u_ViewInfo; // zmin, zmax, zmax / zmin, SUN_ID

varying vec2				var_TexCoords;

// General options...
#define VOLUMETRIC_STRENGTH		u_Local0.b

void main ( void )
{
	if (u_Local1.r >= 1.0)
	{// Night...
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	vec3	totalColor = vec3(0.0, 0.0, 0.0);

	totalColor.rgb += u_vlightColors * /*0.05*/0.025;
	totalColor.rgb *= VOLUMETRIC_STRENGTH * 1.5125;

	if (u_Local1.r > 0.0)
	{// Sunset, Sunrise, and Night times... Scale down screen color, before adding lighting...
		vec3 nightColor = vec3(0.0);
		totalColor.rgb = mix(totalColor.rgb, nightColor, u_Local1.r);
	}

	gl_FragColor = vec4(totalColor, 1.0);
}
