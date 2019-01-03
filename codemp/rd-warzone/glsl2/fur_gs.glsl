layout(triangles) in;

layout(triangle_strip, max_vertices = 73/*87*/) out;

uniform mat4	u_ModelViewProjectionMatrix;

uniform vec4	u_Local10;

#define FUR_LAYERS 29
//#define FUR_LENGTH 0.1
#define FUR_LENGTH u_Local10.r

in vec3 v_g_normal[3];
in vec2 v_g_texCoord[3];
in vec3 v_g_PrimaryLightDir[3];
in vec3 v_g_ViewDir[3];

out vec4 v_position;
out vec3 v_normal;
out vec2 v_texCoord;
out vec3 v_PrimaryLightDir;

out float v_furStrength;

void main(void)
{
	vec3 normal;

	const float FUR_DELTA = 1.0 / float(FUR_LAYERS);
	
	float d = 0.0;

	for (int furLayer = 0; furLayer < FUR_LAYERS; furLayer++)
	{
		d += FUR_DELTA;
		
		for(int i = 0; i < gl_in.length(); i++)
		{
			normal = normalize(v_g_normal[i]);

			vec3 E = normalize(v_g_ViewDir[i]);
			v_normal = normal * E;//u_normalMatrix * normal; 

			v_texCoord = v_g_texCoord[i];
			
			// If the distance of the layer is getting bigger to the original surface, the layer gets more transparent.   
			v_furStrength = 1.0 - d;
			
			// Displace a layer along the surface normal.
			v_position = (gl_in[i].gl_Position + vec4(normal * d * FUR_LENGTH, 0.0));
			gl_Position = u_ModelViewProjectionMatrix * v_position;
	
			EmitVertex();
		}
		
		EndPrimitive();
	}
}
