attribute vec3					attr_Position;
attribute vec3					attr_Normal;
attribute vec2					attr_TexCoord0;

uniform mat4					u_ModelViewProjectionMatrix;
uniform vec3					u_ViewOrigin;

uniform vec4					u_Local0; // width, height, fadeStartDistance, fadeEndDistance
uniform vec4					u_Local1; // fadeScale, widthVariance, heightVariance, facing
uniform vec4					u_Local2; // oriented

#define u_Width					u_Local0.r
#define u_Height				u_Local0.g
#define u_FadeStartDistance		u_Local0.b
#define u_FadeEndDistance		u_Local0.a

#define u_FadeScale				u_Local1.r
#define u_WidthVariance			u_Local1.g
#define u_HeightVariance		u_Local1.b
#define u_Facing				u_Local1.a

#define u_Oriented				u_Local2.r

varying vec2   					var_TexCoords;
varying vec3   					var_vertPos;
varying vec3   					var_Normal;
varying float   				var_Alpha;

#define SURFSPRITE_FACING_NORMAL	0
#define SURFSPRITE_FACING_UP		1
#define SURFSPRITE_FACING_DOWN		2
#define SURFSPRITE_FACING_ANY		3

void main()
{
	vec3 V = u_ViewOrigin - attr_Position;

	float width = u_Width * (1.0 + u_WidthVariance*0.5);
	float height = u_Height * (1.0 + u_HeightVariance*0.5);

	float distanceToCamera = length(V);
	float fadeScale = smoothstep(u_FadeStartDistance, u_FadeEndDistance, distanceToCamera);
	width += u_FadeScale * fadeScale * u_Width;

	float halfWidth = width * 0.5;

	if (u_Facing == SURFSPRITE_FACING_UP)
	{
		vec3 offsets[] = vec3[](
			vec3( halfWidth, -halfWidth, 0.0),
			vec3( halfWidth,  halfWidth, 0.0),
			vec3(-halfWidth,  halfWidth, 0.0),
			vec3(-halfWidth, -halfWidth, 0.0)
			);

		const vec2 texcoords[] = vec2[](
			vec2(1.0, 1.0),
			vec2(1.0, 0.0),
			vec2(0.0, 0.0),
			vec2(0.0, 1.0)
		);

		vec3 offset = offsets[gl_VertexID];

		if (u_Oriented > 0.0)
		{
			vec2 toCamera = normalize(V.xy);
			offset.xy = offset.x*vec2(toCamera.y, -toCamera.x);
		}

		vec4 worldPos = vec4(attr_Position + offset, 1.0);
		gl_Position = u_ModelViewProjectionMatrix * worldPos;
		var_vertPos = worldPos.xyz;
		var_Normal = attr_Normal.xyz * 2.0 - 1.0;
		var_TexCoords = texcoords[gl_VertexID];
		var_Alpha = 1.0 - fadeScale;
	}
	else
	{
		vec3 offsets[] = vec3[](
			vec3( halfWidth, 0.0, 0.0),
			vec3( halfWidth, 0.0, height),
			vec3(-halfWidth, 0.0, height),
			vec3(-halfWidth, 0.0, 0.0)
		);

		const vec2 texcoords[] = vec2[](
			vec2(1.0, 1.0),
			vec2(1.0, 0.0),
			vec2(0.0, 0.0),
			vec2(0.0, 1.0)
		);

		vec3 offset = offsets[gl_VertexID];

		if (u_Oriented > 0.0)
		{
			vec2 toCamera = normalize(V.xy);
			offset.xy = offset.x*vec2(toCamera.y, -toCamera.x);
		}
		else
		{
			// Make this sprite face in some direction
			offset.xy = offset.x*attr_Normal.xy;
		}

		vec4 worldPos = vec4(attr_Position + offset, 1.0);
		gl_Position = u_ModelViewProjectionMatrix * worldPos;
		var_vertPos = worldPos.xyz;
		var_Normal = attr_Normal.xyz * 2.0 - 1.0;
		var_TexCoords = texcoords[gl_VertexID];
		var_Alpha = 1.0 - fadeScale;
	}
}
